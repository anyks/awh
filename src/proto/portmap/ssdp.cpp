/**
 * @file: ssdp.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация кодека договора SSDP (UPnP Device Architecture) — сборка запросов
 *        обнаружения устройств, разбор ответов и оповещений, описания кодов причины отказа
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/lib.hpp>
#include <proto/portmap/ssdp.hpp>

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
	 * @brief Разделитель строк, отведённый договором
	 *
	 */
	constexpr const char * BREAK = "\r\n";

	/**
	 * @brief Срок годности сведений, принимаемый при отсутствии указания
	 *
	 * @details Указание срока договором обязательным не является, и устройства
	 * нередко его опускают. Значение взято из описания договора
	 *
	 */
	constexpr uint32_t DEFAULT_MAX_AGE = 0x708;

};

/**
 * @brief Конструктор
 *
 */
awh::proto::portmap::SSDP::Answer::Answer() noexcept :
 kind(kind_t::NONE), notice(notice_t::NONE), maxAge(DEFAULT_MAX_AGE) {}

/**
 * @brief Метод сборки запроса обнаружения устройств
 *
 * @param target обозначение искомой службы
 * @param delay  отведённый устройству срок на ответ в секундах
 * @param six    признак сборки запроса для сети IPv6
 * @return       собранный текст запроса
 *
 */
string awh::proto::portmap::SSDP::search(const string_view target, const uint8_t delay, const string_view group) const noexcept {
	// Собираемый текст запроса
	string result;
	/**
	 * Если обозначение искомой службы не передано
	 */
	if(target.empty()){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (delay), group), log_t::flag_t::WARNING, message(error_t::MISSING_TARGET));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error_t::MISSING_TARGET));
		#endif
		// Выводим пустой текст запроса
		return result;
	}
	// Отводим место под собираемый текст запроса
	result.reserve(0x100);
	// Записываем строку действия запроса
	result.append("M-SEARCH * HTTP/1.1").append(::BREAK);
	/**
	 * Записываем групповой адрес и порт обнаружения устройств
	 *
	 * @note Адрес записывается тот же, на который запрос и рассылается: договор
	 *       велит устройству сличать их, отвергая запрос с чужим адресом
	 */
	result.append("HOST: ");
	/**
	 * Отсекаем зону группового адреса
	 *
	 * @note Зона принадлежит машине отправителя, а не сети: для устройства она смысла
	 *       не имеет, а записанная в поле сделала бы адрес несовпадающим
	 */
	const string_view address = group.substr(0, group.find('%'));
	/**
	 * Если запрос собирается для сети IPv6
	 *
	 * @note Адрес IPv6 в поле обязан быть заключён в квадратные скобки: без них
	 *       двоеточия адреса не отличить от разделителя порта
	 */
	if(address.find(':') != string_view::npos)
		// Записываем групповой адрес сети IPv6
		result.append("[").append(address).append("]");
	// Записываем групповой адрес сети IPv4
	else result.append(address);
	// Записываем порт обнаружения устройств
	result.append(":").append(std::to_string(PORT)).append(::BREAK);
	/**
	 * Записываем указание на немедленную рассылку запроса
	 *
	 * @note Указание отведено договором как есть, вместе с кавычками: оно
	 *       досталось договору от расширения HTTP и своего содержания не имеет
	 */
	result.append("MAN: \"ssdp:discover\"").append(::BREAK);
	// Записываем отведённый устройству срок на ответ
	result.append("MX: ").append(std::to_string(static_cast <uint32_t> (delay > 0 ? delay : DEFAULT_DELAY))).append(::BREAK);
	// Записываем обозначение искомой службы
	result.append("ST: ").append(target).append(::BREAK);
	// Записываем сведения о самом обращающемся
	result.append("USER-AGENT: ").append(AWH_NAME).append("/").append(AWH_VERSION).append(" UPnP/1.1").append(::BREAK);
	// Записываем завершение заголовка запроса
	result.append(::BREAK);
	// Выводим собранный текст запроса
	return result;
}
/**
 * @brief Метод разбора сообщения договора
 *
 * @param text   разбираемое сообщение
 * @param answer ссылка на разобранное сообщение
 * @param error  ссылка на код причины отказа
 * @return       признак успешного разбора
 *
 */
bool awh::proto::portmap::SSDP::parse(const string_view text, answer_t & answer, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	// Выполняем сброс разобранного сообщения
	answer = answer_t();
	/**
	 * Если разбираемое сообщение пусто
	 */
	if(text.empty()){
		// Запоминаем код причины отказа
		error = error_t::EMPTY;
		/**
		 * Если включён режим отладки
		 *
		 * @note Отказ разбора записывается лишь при отладке намеренно: на групповой
		 *       адрес приходят оповещения всех устройств сети вперемешку с мусором,
		 *       и запись о каждом из них засоряла бы журнал
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text.length()), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если разбираемое сообщение длиннее допустимого договором
	 */
	if(text.length() > MAX_MESSAGE_SIZE){
		// Запоминаем код причины отказа
		error = error_t::TOO_LARGE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text.length()), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Определяем направление разбираемого сообщения по его началу
	 *
	 * @note Направление разборщику задаётся заранее, а на открытый порт приходят
	 *       вперемешку и ответы устройств, и их оповещения, и собственный запрос,
	 *       вернувшийся с группового адреса. Различить их можно лишь по приставке
	 *       строки действия: у ответа она начинается изданием договора
	 */
	const bool response = ((text.length() > 5) && (text.compare(0, 5, "HTTP/") == 0));
	// Создаём объект разбора сообщения договора HTTP
	http::parser_http_t parser((response ? http::direct_t::RESPONSE : http::direct_t::REQUEST), this->_fmk, this->_log);
	/**
	 * Устанавливаем обработчик полей заголовка сообщения
	 *
	 * @note Поля разбираются готовым разборщиком договора HTTP: договор SSDP им и
	 *       построен, и заводить под него собственный разбор незачем
	 */
	parser.on(http::parser_http_t::header_callback_t([&answer, this](const uint32_t, const string_view name, const string_view value, const http::parser_t::part_t) noexcept -> bool {
		/**
		 * Если полем является обозначение искомой службы
		 *
		 * @note Обозначение службы приходит в разных полях: в ответе оно записано
		 *       полем искомого, а в оповещении - полем сообщаемого
		 */
		if(this->_fmk->compare(name, "ST") || this->_fmk->compare(name, "NT"))
			// Запоминаем обозначение службы, о которой сообщает устройство
			answer.target.assign(value);
		/**
		 * Если полем является обозначение самого устройства
		 */
		else if(this->_fmk->compare(name, "USN"))
			// Запоминаем обозначение самого устройства и его службы
			answer.usn.assign(value);
		/**
		 * Если полем является адрес описания устройства
		 */
		else if(this->_fmk->compare(name, "LOCATION"))
			// Запоминаем адрес описания устройства
			answer.location.assign(value);
		/**
		 * Если полем являются сведения об устройстве
		 */
		else if(this->_fmk->compare(name, "SERVER"))
			// Запоминаем сведения об устройстве и его встроенной программе
			answer.server.assign(value);
		/**
		 * Если полем является вид оповещения устройства
		 */
		else if(this->_fmk->compare(name, "NTS")) {
			/**
			 * Если устройство объявилось в сети
			 */
			if(this->_fmk->compare(value, "ssdp:alive"))
				// Запоминаем вид оповещения устройства
				answer.notice = notice_t::ALIVE;
			/**
			 * Если устройство покидает сеть
			 */
			else if(this->_fmk->compare(value, "ssdp:byebye"))
				// Запоминаем вид оповещения устройства
				answer.notice = notice_t::BYEBYE;
			/**
			 * Если устройство сменило свои сведения
			 */
			else if(this->_fmk->compare(value, "ssdp:update"))
				// Запоминаем вид оповещения устройства
				answer.notice = notice_t::UPDATE;
		/**
		 * Если полем является срок годности полученных сведений
		 */
		} else if(this->_fmk->compare(name, "CACHE-CONTROL")) {
			// Выполняем поиск указания срока годности сведений
			const size_t pos = value.find('=');
			/**
			 * Если указание срока годности сведений обнаружено
			 */
			if(pos != string_view::npos){
				// Получаем указание срока годности сведений
				string_view life = value.substr(pos + 1);
				/**
				 * Выполняем отбрасывание пробельной обвязки указания
				 *
				 * @note Пробелы вокруг знака равенства договором не отведены, но
				 *       устройства их ставят: терять из-за этого срок годности незачем
				 */
				while(!life.empty() && ((life.front() == ' ') || (life.front() == '\t')))
					// Выполняем переход к следующему знаку указания
					life.remove_prefix(1);
				// Получаем указанный срок годности сведений
				const uint32_t maxAge = this->_fmk->atoi <uint32_t> (life);
				/**
				 * Если указанный срок годности сведений разобран
				 *
				 * @note Нулевой срок означает, что разбор не удался: устройство,
				 *       пропадающее в тот же миг, смысла не имеет
				 */
				if(maxAge > 0)
					// Запоминаем срок годности полученных сведений
					answer.maxAge = maxAge;
			}
		}
		// Выводим признак продолжения разбора сообщения
		return true;
	}));
	// Выполняем разбор сообщения договора
	parser.parse(text.data(), text.length());
	/**
	 * Если разбор сообщения выполнить не удалось
	 */
	if(parser.error() != http::parser_http_t::error_t::NONE){
		// Запоминаем код причины отказа
		error = error_t::MALFORMED;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (parser.error())), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если провайдер разобранного сообщения не получен
	 */
	if(parser.message().provider == nullptr){
		// Запоминаем код причины отказа
		error = error_t::MALFORMED;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text.length()), log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если разобранное сообщение является ответом устройства
	 */
	if(response){
		// Запоминаем вид полученного сообщения
		answer.kind = kind_t::RESPONSE;
		/**
		 * Если устройство ответило отказом
		 *
		 * @note Отказ здесь не испорченное сообщение, а осмысленный ответ: устройство
		 *       обнаружено, но искомой службы не имеет либо занято
		 */
		if(static_cast <const http::response_t *> (parser.message().provider.get())->code != 200){
			// Запоминаем код причины отказа
			error = error_t::BAD_STATUS;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <const http::response_t *> (parser.message().provider.get())->code), log_t::flag_t::WARNING, message(error));
			#endif
			// Выводим признак неудачного разбора
			return false;
		}
	/**
	 * Если разобранное сообщение является запросом
	 */
	} else {
		/**
		 * Определяем действие разобранного запроса
		 */
		switch(static_cast <uint8_t> (static_cast <const http::request_t *> (parser.message().provider.get())->method)){
			/**
			 * Если запрос является оповещением устройства
			 */
			case static_cast <uint8_t> (http::method_t::NOTIFY):
				// Запоминаем вид полученного сообщения
				answer.kind = kind_t::NOTIFY;
			break;
			/**
			 * Если запрос является запросом обнаружения устройств
			 *
			 * @note Собственный запрос приходит обратно и на свою же машину: разбирать
			 *       его следует, чтобы отличить от ответа, но пригодным устройством он
			 *       не является
			 */
			case static_cast <uint8_t> (http::method_t::MSEARCH):
				// Запоминаем вид полученного сообщения
				answer.kind = kind_t::SEARCH;
			break;
			/**
			 * Если действие в сообщении договору не принадлежит
			 */
			default: {
				// Запоминаем код причины отказа
				error = error_t::UNKNOWN_METHOD;
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (static_cast <const http::request_t *> (parser.message().provider.get())->method)), log_t::flag_t::WARNING, message(error));
				#endif
				// Выводим признак неудачного разбора
				return false;
			}
		}
	}
	/**
	 * Если сообщение является ответом либо объявлением устройства
	 *
	 * @note Оповещение об уходе адреса описания не несёт, и требовать его незачем:
	 *       ушедшее устройство описывать нечем
	 */
	if((answer.kind == kind_t::RESPONSE) || ((answer.kind == kind_t::NOTIFY) && (answer.notice != notice_t::BYEBYE))){
		/**
		 * Если в сообщении нет обозначения службы
		 */
		if(answer.target.empty()){
			// Запоминаем код причины отказа
			error = error_t::MISSING_TARGET;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(answer.usn), log_t::flag_t::WARNING, message(error));
			#endif
			// Выводим признак неудачного разбора
			return false;
		}
		/**
		 * Если в сообщении нет адреса описания устройства
		 *
		 * @note Без адреса описания обнаруженное устройство бесполезно: перечень
		 *       его служб взять неоткуда
		 */
		if(answer.location.empty()){
			// Запоминаем код причины отказа
			error = error_t::MISSING_LOCATION;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(answer.target), log_t::flag_t::WARNING, message(error));
			#endif
			// Выводим признак неудачного разбора
			return false;
		}
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод проверки пригодности обнаруженного устройства
 *
 * @param answer разобранное сообщение договора
 * @param target обозначение искомой службы
 * @return       признак пригодности обнаруженного устройства
 *
 */
bool awh::proto::portmap::SSDP::suitable(const answer_t & answer, const string_view target) const noexcept {
	/**
	 * Если сообщение ответом либо оповещением не является
	 */
	if((answer.kind != kind_t::RESPONSE) && (answer.kind != kind_t::NOTIFY))
		// Выводим признак непригодности обнаруженного устройства
		return false;
	/**
	 * Если устройство покидает сеть
	 */
	if(answer.notice == notice_t::BYEBYE)
		// Выводим признак непригодности обнаруженного устройства
		return false;
	/**
	 * Если адреса описания устройства сообщение не несёт
	 */
	if(answer.location.empty())
		// Выводим признак непригодности обнаруженного устройства
		return false;
	/**
	 * Если поиск вёлся по всем устройствам сети
	 */
	if(target.empty() || (target.compare(TARGET_ALL) == 0))
		// Выводим признак пригодности обнаруженного устройства
		return true;
	/**
	 * Выводим итог сличения обозначения службы с искомым
	 *
	 * @note Сличение ведётся без учёта регистра: обозначения службы устройства
	 *       записывают вольно, а отвергать маршрутизатор из-за разницы в написании
	 *       незачем
	 */
	return this->_fmk->compare(answer.target, target);
}
/**
 * @brief Метод получения описания кода причины отказа кодека SSDP
 *
 * @param error код причины отказа кодека
 * @return      описание кода причины отказа на английском языке
 *
 */
const char * awh::proto::portmap::message(const ssdp_t::error_t error) noexcept {
	/**
	 * Определяем код причины отказа кодека
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (ssdp_t::error_t::NONE):
			// Выводим описание кода причины отказа
			return "no error";
		// Если разбираемое сообщение пусто
		case static_cast <uint8_t> (ssdp_t::error_t::EMPTY):
			// Выводим описание кода причины отказа
			return "message is empty";
		// Если сообщение длиннее допустимого договором
		case static_cast <uint8_t> (ssdp_t::error_t::TOO_LARGE):
			// Выводим описание кода причины отказа
			return "message exceeds the protocol size limit";
		// Если сообщение построено ошибочно
		case static_cast <uint8_t> (ssdp_t::error_t::MALFORMED):
			// Выводим описание кода причины отказа
			return "malformed message";
		// Если действие в сообщении договору не принадлежит
		case static_cast <uint8_t> (ssdp_t::error_t::UNKNOWN_METHOD):
			// Выводим описание кода причины отказа
			return "unknown request method";
		// Если устройство ответило отказом
		case static_cast <uint8_t> (ssdp_t::error_t::BAD_STATUS):
			// Выводим описание кода причины отказа
			return "device responded with an error status";
		// Если в сообщении нет обозначения службы
		case static_cast <uint8_t> (ssdp_t::error_t::MISSING_TARGET):
			// Выводим описание кода причины отказа
			return "search target is missing";
		// Если в сообщении нет адреса описания устройства
		case static_cast <uint8_t> (ssdp_t::error_t::MISSING_LOCATION):
			// Выводим описание кода причины отказа
			return "device description location is missing";
	}
	// Выводим описание неизвестного кода причины отказа
	return "unknown error";
}
