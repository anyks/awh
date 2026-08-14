/**
 * @file soap.cpp
 * @date 2026-08-02
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация кодека договора SOAP для управления службами UPnP — сборка вызова
 *        действия службы, разбор ответа и разбор отказа с кодом ошибки UPnP
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/portmap/soap.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Приставка названия ответа на вызов действия службы
	 *
	 * @details Договор велит называть ответ названием действия с этой приставкой:
	 * иного признака принадлежности ответа вызову он не имеет
	 *
	 */
	constexpr const char * ANSWER_SUFFIX = "Response";

	/**
	 * @brief Метод поиска вложенного узла по местному имени
	 *
	 * @details Сличение ведётся лишь по местному имени, без учёта пространства имён.
	 * Так и задумано: ответ получен у одной определённой службы по её же адресу
	 * управления, а приставки пространств имён устройства записывают вольно
	 *
	 * @param node  узел, среди вложенных которого ведётся поиск
	 * @param local местное имя искомого узла
	 * @return      найденный узел дерева разметки
	 *
	 */
	codec::xml::node_t child(const codec::xml::node_t & node, const string_view local) noexcept {
		/**
		 * Выполняем перебор всех непосредственно вложенных узлов
		 */
		for(codec::xml::node_t item = node.first(); item.valid(); item = item.next()){
			/**
			 * Если вложенный узел узлом разметки не является
			 */
			if(item.kind() != codec::xml::kind_t::ELEMENT)
				// Выполняем переход к следующему вложенному узлу
				continue;
			/**
			 * Если местное имя вложенного узла совпадает с искомым
			 */
			if(item.name().local.compare(local) == 0)
				// Выводим обнаруженный узел дерева разметки
				return item;
		}
		// Выводим непригодный узел дерева разметки
		return codec::xml::node_t();
	}
	/**
	 * @brief Метод получения первого вложенного узла разметки
	 *
	 * @param node узел, среди вложенных которого ведётся поиск
	 * @return     первый вложенный узел разметки
	 *
	 */
	codec::xml::node_t first(const codec::xml::node_t & node) noexcept {
		/**
		 * Выполняем перебор всех непосредственно вложенных узлов
		 */
		for(codec::xml::node_t item = node.first(); item.valid(); item = item.next()){
			/**
			 * Если вложенный узел узлом разметки является
			 */
			if(item.kind() == codec::xml::kind_t::ELEMENT)
				// Выводим обнаруженный узел дерева разметки
				return item;
		}
		// Выводим непригодный узел дерева разметки
		return codec::xml::node_t();
	}
	/**
	 * @brief Метод проверки пригодности значения к записи в заголовок обмена
	 *
	 * @details Заголовок обмена HTTP отделяется от следующего концом строки, и знак
	 *          этот внутри значения заголовка разбивает запрос надвое: за местом
	 *          разрыва встаёт то, что вписал туда выдавший значение. Обозначение
	 *          службы приходит описанием устройства из сети, а описание это выдаёт
	 *          сам маршрутизатор - доверять ему построение нашего же запроса нельзя
	 *
	 * @note Отвергаются все управляющие знаки, а не одни лишь концы строки: договор
	 *       обмена дозволяет значению заголовка знаки видимые, пробел и знак
	 *       горизонтального хода, а прочим управляющим знакам там места нет вовсе
	 *
	 * @param value проверяемое значение заголовка обмена
	 * @return      признак пригодности значения к записи в заголовок
	 *
	 */
	static bool printable(const string_view value) noexcept {
		/**
		 * Выполняем перебор всех знаков проверяемого значения
		 */
		for(const char letter : value){
			// Получаем код очередного знака проверяемого значения
			const uint8_t code = static_cast <uint8_t> (letter);
			/**
			 * Если знак является управляющим и знаком горизонтального хода не является
			 */
			if(((code < 0x20) && (code != 0x09)) || (code == 0x7F))
				// Выводим непригодность значения к записи в заголовок обмена
				return false;
		}
		// Выводим пригодность значения к записи в заголовок обмена
		return true;
	}
};

/**
 * @brief Метод сборки вызова действия службы
 *
 * @param service   обозначение вида службы, у которой вызывается действие
 * @param action    название вызываемого действия службы
 * @param arguments перечень доводов вызова действия
 * @return          собранный текст вызова действия службы
 *
 */
string awh::proto::portmap::SOAP::request(const string_view service, const string_view action, const vector <argument_t> & arguments) const noexcept {
	/**
	 * Если обозначение службы либо название действия не переданы
	 */
	if(service.empty() || action.empty()){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(string(service), string(action)), log_t::flag_t::WARNING, message(error_t::INVALID_ACTION));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error_t::INVALID_ACTION));
		#endif
		// Выводим пустой текст вызова действия службы
		return string();
	}
	/**
	 * Настройки записи текста разметки
	 *
	 * @note Запись ведётся плотной намеренно: отступы добавляют между узлами
	 *       пробельное содержимое, а часть служб принимает его за значение довода
	 */
	codec::xml::writer_t::settings_t settings;
	/**
	 * Выполняем запись узлов без содержимого парой меток
	 *
	 * @note Договор разметки самозакрывающуюся метку и пару меток различает лишь
	 *       на письме, однако службы устройств писались под чужую поверку, которая
	 *       пустой довод записывает парой меток. Часть встроенных программ
	 *       самозакрывающуюся метку не разбирает и отвечает отказом
	 */
	settings.collapse = false;
	// Создаём объект записи текста разметки
	codec::xml::writer_t writer(settings);
	// Выполняем запись объявления разметки
	writer.declaration();
	// Выполняем открытие конверта запроса
	writer.open("Envelope", NAMESPACE);
	// Выполняем запись правил записи содержимого конверта
	writer.attribute("encodingStyle", ENCODING, NAMESPACE);
	// Выполняем открытие тела запроса
	writer.open("Body", NAMESPACE);
	// Выполняем открытие вызываемого действия службы
	writer.open(action, service);
	/**
	 * Выполняем перебор всех доводов вызова действия
	 *
	 * @note Доводы записываются без пространства имён: договор UPnP велит именовать
	 *       их местными именами внутри узла действия
	 */
	for(const argument_t & argument : arguments)
		// Выполняем запись очередного довода вызова действия
		writer.element(argument.name, argument.value);
	// Выполняем закрытие вызываемого действия службы
	writer.close();
	// Выполняем закрытие тела запроса
	writer.close();
	// Выполняем закрытие конверта запроса
	writer.close();
	/**
	 * Если собранный текст вызова не завершён
	 */
	if(!writer.complete()){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(string(service), string(action)), log_t::flag_t::WARNING, codec::xml::message(writer.error()));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, codec::xml::message(writer.error()));
		#endif
		// Выводим пустой текст вызова действия службы
		return string();
	}
	// Выводим собранный текст вызова действия службы
	return writer.text();
}
/**
 * @brief Метод сборки обозначения вызываемого действия службы
 *
 * @param service обозначение вида службы, у которой вызывается действие
 * @param action  название вызываемого действия службы
 * @return        собранное обозначение вызываемого действия службы
 *
 */
string awh::proto::portmap::SOAP::action(const string_view service, const string_view action) const noexcept {
	// Собираемое обозначение вызываемого действия службы
	string result;
	/**
	 * Если обозначение службы либо название действия не переданы
	 */
	if(service.empty() || action.empty())
		// Выводим пустое обозначение вызываемого действия службы
		return result;
	/**
	 * Если обозначение службы либо название действия к записи в заголовок непригодны
	 *
	 * @warning Обозначение службы берётся из описания устройства, полученного из сети:
	 *          конец строки в нём разбил бы наш же запрос надвое, а за местом разрыва
	 *          встало бы то, что вписал туда выдавший описание
	 */
	if(!::printable(service) || !::printable(action)){
		// Выводим сообщение об ошибке
		this->_log->print("%s", log_t::flag_t::WARNING, "service or action is not printable");
		// Выводим пустое обозначение вызываемого действия службы
		return result;
	}
	// Отводим место под собираемое обозначение
	result.reserve(service.length() + action.length() + 3);
	/**
	 * Выполняем сборку обозначения вызываемого действия службы
	 *
	 * @note Кавычки договором отведены как есть: без них часть служб обозначение
	 *       не распознаёт и отвечает отказом
	 */
	result.append("\"").append(service).append("#").append(action).append("\"");
	// Выводим собранное обозначение вызываемого действия службы
	return result;
}
/**
 * @brief Метод разбора ответа службы
 *
 * @param text   разбираемый ответ службы
 * @param answer ссылка на разобранный ответ службы
 * @param error  ссылка на код причины отказа
 * @return       признак успешного разбора
 *
 */
bool awh::proto::portmap::SOAP::parse(const string_view text, answer_t & answer, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	// Выполняем сброс разобранного ответа службы
	answer = answer_t();
	/**
	 * Если разбираемый ответ пуст
	 */
	if(text.empty()){
		// Запоминаем код причины отказа
		error = error_t::EMPTY;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text.length()), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если разбираемый ответ длиннее допустимого
	 */
	if(text.length() > MAX_ANSWER_SIZE){
		// Запоминаем код причины отказа
		error = error_t::TOO_LARGE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text.length()), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Объект дерева разметки ответа службы
	codec::xml::document_t document;
	/**
	 * Если разбор ответа службы выполнить не удалось
	 */
	if(!document.parse(text)){
		// Запоминаем код причины отказа
		error = error_t::MALFORMED;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(document.errorLocation().line, document.errorLocation().column),
				log_t::flag_t::WARNING, codec::xml::message(document.error())
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, codec::xml::message(document.error()));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем конверт ответа службы
	const codec::xml::node_t envelope = document.element();
	/**
	 * Если конвертом ответ службы не является
	 */
	if(!envelope.valid() || (envelope.name().local.compare("Envelope") != 0)){
		// Запоминаем код причины отказа
		error = error_t::MISSING_ENVELOPE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(envelope.name().local), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем тело ответа службы
	const codec::xml::node_t body = ::child(envelope, "Body");
	/**
	 * Если тела в конверте ответа нет
	 */
	if(!body.valid()){
		// Запоминаем код причины отказа
		error = error_t::MISSING_BODY;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(envelope.name().uri), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем отказ службы, если он в теле ответа записан
	const codec::xml::node_t fault = ::child(body, "Fault");
	/**
	 * Если служба ответила отказом
	 *
	 * @note Отказ разбирается успешно: сообщение построено верно, а причина отказа
	 *       лежит в коде ошибки. Отличать отказ службы от испорченного ответа
	 *       необходимо - отказ означает, что служба до нас дошла и нас поняла
	 */
	if(fault.valid()){
		// Запоминаем признак того, что служба ответила отказом
		answer.fault = true;
		// Запоминаем название ответа службы
		answer.action.assign(fault.name().local);
		/**
		 * Получаем описание отказа службы
		 *
		 * @note Код ошибки UPnP лежит не в самом отказе, а в его подробностях: сам
		 *       договор SOAP видов отказа не задаёт, и UPnP заводит свой
		 */
		const codec::xml::node_t detail = ::child(::child(fault, "detail"), "UPnPError");
		/**
		 * Если подробности отказа службы записаны
		 */
		if(detail.valid()){
			// Запоминаем описание ошибки, выданное службой
			answer.description = ::child(detail, "errorDescription").text();
			/**
			 * Выполняем разбор кода ошибки, выданного службой
			 *
			 * @note Разбор числом ведётся с проверкой: отсутствие кода и код нуль
			 *       следует различать, а не считать одним и тем же
			 */
			if(!::child(detail, "errorCode").value(answer.code))
				// Сбрасываем код ошибки, выданный службой
				answer.code = 0;
		/**
		 * Если подробностей отказа службы нет
		 */
		} else {
			// Запоминаем описание отказа, выданное службой
			answer.description = ::child(fault, "faultstring").text();
		}
		// Выводим признак успешного разбора
		return true;
	}
	// Получаем ответ службы на вызов действия
	const codec::xml::node_t response = ::first(body);
	/**
	 * Если в теле ответа нет ни ответа службы, ни отказа
	 */
	if(!response.valid()){
		// Запоминаем код причины отказа
		error = error_t::MISSING_ANSWER;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(envelope.name().uri), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем название ответа службы
	const string_view name = response.name().local;
	/**
	 * Если название ответа записано с приставкой ответа на вызов действия
	 *
	 * @note Приставка отбрасывается намеренно: так название в разобранном ответе
	 *       совпадает с названием вызванного действия, и сличать их можно напрямую
	 */
	if((name.length() > ::std::char_traits <char>::length(::ANSWER_SUFFIX)) &&
	   (name.compare(name.length() - ::std::char_traits <char>::length(::ANSWER_SUFFIX), ::std::char_traits <char>::length(::ANSWER_SUFFIX), ::ANSWER_SUFFIX) == 0))
		// Запоминаем название действия без приставки ответа
		answer.action.assign(name.substr(0, name.length() - ::std::char_traits <char>::length(::ANSWER_SUFFIX)));
	// Запоминаем название ответа службы как есть
	else answer.action.assign(name);
	/**
	 * Выполняем перебор всех значений, выданных службой
	 */
	for(codec::xml::node_t item = response.first(); item.valid(); item = item.next()){
		/**
		 * Если вложенный узел узлом разметки не является
		 */
		if(item.kind() != codec::xml::kind_t::ELEMENT)
			// Выполняем переход к следующему вложенному узлу
			continue;
		// Выполняем добавление очередного значения к разобранному ответу
		answer.arguments.emplace_back(item.name().local, item.text());
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод получения значения из разобранного ответа службы
 *
 * @param answer разобранный ответ службы
 * @param name   название искомого значения
 * @return       значение из ответа службы либо пустая последовательность
 *
 */
string_view awh::proto::portmap::SOAP::value(const answer_t & answer, const string_view name) const noexcept {
	/**
	 * Выполняем перебор всех значений, выданных службой
	 */
	for(const argument_t & argument : answer.arguments){
		/**
		 * Если название значения совпадает с искомым
		 */
		if(argument.name.compare(name) == 0)
			// Выводим обнаруженное значение из ответа службы
			return argument.value;
	}
	// Выводим пустую последовательность знаков
	return string_view();
}
/**
 * @brief Метод получения описания кода причины отказа кодека SOAP
 *
 * @param error код причины отказа кодека
 * @return      описание кода причины отказа на английском языке
 *
 */
const char * awh::proto::portmap::message(const soap_t::error_t error) noexcept {
	/**
	 * Определяем код причины отказа кодека
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (soap_t::error_t::NONE):
			// Выводим описание кода причины отказа
			return "no error";
		// Если разбираемый ответ пуст
		case static_cast <uint8_t> (soap_t::error_t::EMPTY):
			// Выводим описание кода причины отказа
			return "SOAP answer is empty";
		// Если ответ длиннее допустимого
		case static_cast <uint8_t> (soap_t::error_t::TOO_LARGE):
			// Выводим описание кода причины отказа
			return "SOAP answer is too large";
		// Если ответ построен ошибочно
		case static_cast <uint8_t> (soap_t::error_t::MALFORMED):
			// Выводим описание кода причины отказа
			return "malformed SOAP answer";
		// Если в ответе нет конверта договора
		case static_cast <uint8_t> (soap_t::error_t::MISSING_ENVELOPE):
			// Выводим описание кода причины отказа
			return "SOAP envelope is missing";
		// Если в конверте ответа нет тела
		case static_cast <uint8_t> (soap_t::error_t::MISSING_BODY):
			// Выводим описание кода причины отказа
			return "SOAP body is missing";
		// Если в теле ответа нет ни ответа службы, ни отказа
		case static_cast <uint8_t> (soap_t::error_t::MISSING_ANSWER):
			// Выводим описание кода причины отказа
			return "SOAP body contains neither a response nor a fault";
		// Если название действия построено ошибочно
		case static_cast <uint8_t> (soap_t::error_t::INVALID_ACTION):
			// Выводим описание кода причины отказа
			return "invalid service action name";
	}
	// Выводим описание неизвестного кода причины отказа
	return "unknown error";
}
