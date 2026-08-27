/**
 * @file document.cpp
 * @date 2026-08-18
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
 * \~russian
 * @brief Файл реализации дерева документа бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the tree of a document of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <cmath>
#include <codec/abc/document.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <limits>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Пространство имён рабочего кода приложения
 */
namespace {
	/**
	 * @brief Шаблон затребованного вида числа
	 *
	 * @tparam T затребованный вид числа
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция приведения дробного числа к затребованному виду
	 *
	 * @details Договор приведения общий у всех кодеков рамки: `NaN` даёт нуль, число за
	 *          пределами вида даёт предел, а дробная часть округляется по правилам
	 *          математики с уводом половины ОТ НУЛЯ — «1.5» даёт 2, «-1.5» даёт -2.
	 *          Прежде ABC отвечал на такое извлечение отказом, соблюдая вид хранения;
	 *          отменено владельцем 20.08.2026 вместе с прочими кодеками
	 *
	 * @param value приводимое дробное число
	 * @return      приведённое число
	 *
	 */
	static T convert(const double value) noexcept {
		/**
		 * Если затребован дробный вид, выводим приведённое число как оно есть
		 *
		 * @note Ветвь эта равняет вызов на девять таких же у прочих кодеков рамки:
		 *       договор приведения общий, и разниться копии его не вправе. Кодек ABC
		 *       дробного вида отсюда покамест не требует - дробное значение снимается
		 *       разрядной записью напрямую, - но вызов, затребовавший его без этой
		 *       ветви, получил бы число, округлённое молча
		 */
		if(is_floating_point <T>::value)
			// Выводим приведённое число как оно есть
			return static_cast <T> (value);
		/**
		 * Если число не является числом вовсе
		 *
		 * @note Приведение `NaN` к целому есть неопределённое поведение при любом пределе
		 */
		if(::isnan(value))
			// Выводим нулевое число
			return static_cast <T> (0);
		/**
		 * Если целая часть числа лежит ниже предела затребованного вида
		 *
		 * @note Пределы сличаются дробным видом, а не целым: предел `int64_t` целым видом
		 *       точно не представим дробным, и сличение целых дало бы промах на единицу
		 */
		if(value <= static_cast <double> (numeric_limits <T>::lowest()))
			// Выводим нижний предел затребованного вида
			return numeric_limits <T>::lowest();
		/**
		 * Если целая часть числа лежит выше предела затребованного вида
		 */
		if(value >= static_cast <double> (numeric_limits <T>::max()))
			// Выводим верхний предел затребованного вида
			return numeric_limits <T>::max();
		/**
		 * Выводим приведённое число, округлив дробную часть
		 *
		 * @warning Округление стоит ПОСЛЕ сличения с пределами вида нарочно: округлить
		 *          прежде значило бы приводить к целому виду число, ему не отвечающее,
		 *          а это поведение неопределённое
		 */
		return static_cast <T> (::round(value));
	}
};

/**
 * @brief Метод объявления отказа разбора документа
 *
 * @param error объявляемый код отказа
 * @return      признак успешности, всегда ложь
 *
 */
bool awh::codec::abc::Document::fail(const error_t error) noexcept {
	// Выполняем установку кода отказа
	this->_error = error;
	/**
	 * Если объект логирования отдан, доносим об отказе.
	 *
	 * @warning Сброс кода отказа сюда НЕ идёт: воронка эта объявляет отказ, а сброс
	 *          лишь снимает прежний, и донесение о нём наполняло бы журнал записями
	 *          «no error» на всякий успешный вызов. Проверено на себе
	 */
	if((error != error_t::NONE) && (this->_log != nullptr)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("ABC: %s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (error)),
			 log_t::flag_t::WARNING, abc::message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("ABC: %s", log_t::flag_t::WARNING, abc::message(error));
		#endif
	}
	// Сообщаем, что работа отвечена отказом
	return false;
}
/**
 * @brief Метод сброса состояния документа
 *
 */
void awh::codec::abc::Document::clear() noexcept {
	// Выполняем очистку вместилища узлов дерева
	this->_nodes.clear();
	// Выполняем очистку хранилища содержимого
	this->_storage.clear();
	// Выполняем очистку указателей имён полей
	this->_index.clear();
	// Выполняем сброс кода отказа разбора
	this->_error = error_t::NONE;
}
/**
 * @brief Структура состояния сборки дерева документа
 *
 * @details Состояние это живёт одним разбором и документу не принадлежит: держи он
 *          стек открытых вместимых своим полем, поле это стояло бы пустым у всякого
 *          собранного документа
 *
 */
typedef struct Building {
	// Документ, чьё дерево собирается
	awh::codec::abc::Document * self;
	// Стек номеров узлов открытых вместимых
	vector <uint32_t> stack;
	/**
	 * Номер узла значения, собираемого кусками, либо предел, если такого нет.
	 *
	 * Значение, собранное кусками, ложится в дерево ОДНИМ узлом: куски суть части
	 * значения, а не значения, и потребителю дерева они видны быть не должны
	 */
	uint32_t segment;
	// Признак отказа сборки дерева документа
	bool failed;
} building_t;

/**
 * @brief Метод приёма события разбора, выданного прямо из чтения
 *
 * @param context указание на состояние сборки дерева документа
 * @param reader  разбиратель бинарной записи
 * @param event   вид принимаемого события разбора
 *
 */
void awh::codec::abc::Document::assemble(void * context, reader_t & reader, const event_t event) noexcept {
	// Выполняем получение состояния сборки дерева документа
	building_t * state = reinterpret_cast <building_t *> (context);
	// Выполняем получение документа, чьё дерево собирается
	Document * self = state->self;
	/**
	 * Если сборка дерева по очередному событию разбора отвечена отказом
	 */
	if(!Document::digest(state, self, reader, event)){
		// Выполняем установку признака отказа сборки дерева
		state->failed = true;
		/**
		 * Выполняем прекращение разбора.
		 *
		 * Возвращать отказ обработчику некуда, а подача записи обязана прекратиться
		 * немедля: причина отказа документом уже записана
		 */
		reader.abort();
	}
}
/**
 * @brief Функция поверки места в хранилище октетов дерева документа
 *
 * @details Узел дерева держит смещение и длину содержимого тридцатью двумя разрядами
 *          НАМЕРЕННО: дерево тем и живо, что тесно, а восемьдесят разрядов на узел
 *          стоили бы памяти на всяком документе ради невиданного. Но выход за предел
 *          обязан быть ОТКАЗОМ, а не усечением: усечённое смещение указывает в чужое
 *          место хранилища, и узел молча выдаёт не своё содержимое
 *
 * @note Довод этот ЗАМЕРЕН 26.08.2026, а не выведен рассуждением. Щуп кладёт запись в
 *       4.25 ГиБ (семнадцать двоичных значений по 256 МиБ) и разбирает её в дерево:
 *       со сторожем разбор отвечает отказом «declared length is inadmissible», построив
 *       16 узлов; со снятым сторожем разбор ПРОХОДИТ, отдаёт 18 узлов и не жалуется
 *       вовсе - смещения завернулись молча. Проверкою набора это не закрепить: прогон
 *       стоит двадцати шести секунд и восьми гигабайт памяти
 *
 * @param held объём хранилища, уже занятый содержимым
 * @param size объём содержимого, какое требуется положить
 * @return     признак того, что содержимое в хранилище умещается
 *
 */
static bool room(const size_t held, const size_t size) noexcept {
	// Выводим признак того, что содержимое умещается в предел смещения узла
	return (size <= (static_cast <size_t> (numeric_limits <uint32_t>::max()) - held));
}
/**
 * @brief Метод сборки дерева документа по событию разбора
 *
 * @param context указание на состояние сборки дерева документа
 * @param self    документ, чьё дерево собирается
 * @param reader  разбиратель бинарной записи
 * @param event   вид принимаемого события разбора
 * @return        признак успешности сборки дерева
 *
 */
bool awh::codec::abc::Document::digest(void * context, Document * self, const reader_t & reader, const event_t event) noexcept {
	// Выполняем получение состояния сборки дерева документа
	building_t * state = reinterpret_cast <building_t *> (context);
	/**
	 * Если событием является конец значения, собираемого кусками
	 */
	if((event == event_t::STRING_END) || (event == event_t::BLOB_END)){
		// Выполняем сброс номера узла значения, собираемого кусками
		state->segment = numeric_limits <uint32_t>::max();
		// Сообщаем, что событие принято
		return true;
	}
	/**
	 * Если снят очередной кусок собираемого значения
	 */
	if(state->segment != numeric_limits <uint32_t>::max()){
		// Выполняем получение значения снятого куска
		const reader_t::value_t chunk = reader.value();
		// Если кусок значения в хранилище октетов не умещается
		if(!::room(self->_storage.size(), chunk.data.size())){
			// Выполняем установку кода отказа недопустимой длины
			self->fail(error_t::INVALID_LENGTH);
			// Сообщаем, что сборка дерева отвечена отказом
			return false;
		}
		// Выполняем перенос содержимого куска в хранилище
		self->_storage.append(chunk.data);
		// Выполняем наращивание длины содержимого собираемого значения
		self->_nodes.at(state->segment).length(self->_nodes.at(state->segment).length() +
		 static_cast <uint32_t> (chunk.data.size()));
		// Сообщаем, что событие принято
		return true;
	}
	// Если событие является окончанием записи либо документа
	if((event == event_t::FINISH) || (event == event_t::DOCUMENT))
		// Сообщаем, что событие принято
		return true;
	// Если событием является конец вместимого
	if((event == event_t::ARRAY_END) || (event == event_t::MAP_END)){
		// Если закрывать нечего
		if(state->stack.empty()){
			// Выполняем установку кода внутреннего отказа
			self->fail(error_t::INTERNAL);
			// Сообщаем, что разбор отвечен отказом
			return false;
		}
		// Выполняем получение номера узла закрываемого вместимого
		const uint32_t index = state->stack.back();
		// Выполняем снятие номера узла со стека
		state->stack.pop_back();
		// Выполняем установку размаха поддерева закрытого вместимого
		self->_nodes.at(index).extent(static_cast <uint32_t> (self->_nodes.size()) - index);
		// Сообщаем, что событие принято
		return true;
	}
	// Выполняем получение значения текущего события
	const reader_t::value_t value = reader.value();
	// Заводимый узел дерева документа
	node_t node;
	/**
	 * Если событием является начало значения, собираемого кусками
	 */
	if((event == event_t::STRING_BEGIN) || (event == event_t::BLOB_BEGIN)){
		// Если хранилище октетов исчерпано, смещение содержимого узлу негодно
		if(!::room(self->_storage.size(), 0)){
			// Выполняем установку кода отказа недопустимой длины
			self->fail(error_t::INVALID_LENGTH);
			// Сообщаем, что сборка дерева отвечена отказом
			return false;
		}
		// Выполняем установку вида значения узла
		node.type = value.type;
		// Выполняем установку смещения содержимого в хранилище
		node.offset = static_cast <uint32_t> (self->_storage.size());
		// Выполняем установку пустой длины содержимого
		node.length(0);
		// Если узел стоит внутри вместимого, учитываем его ребёнком
		if(!state->stack.empty())
			// Выполняем учёт ребёнка вместившего вместимого
			self->_nodes.at(state->stack.back()).length(self->_nodes.at(state->stack.back()).length() + 1);
		// Выполняем установку номера узла значения, собираемого кусками
		state->segment = static_cast <uint32_t> (self->_nodes.size());
		// Выполняем добавление узла в дерево документа
		self->_nodes.push_back(node);
		// Сообщаем, что событие принято
		return true;
	}
	// Выполняем установку вида значения узла
	node.type = value.type;
	// Выполняем установку признака имени поля отображения
	node.keyed = (event == event_t::KEY);
	/**
	 * Определяем вид значения текущего события
	 */
	switch(static_cast <uint32_t> (value.type)){
		// Если значение является пустым
		case static_cast <uint32_t> (type_t::NUL): break;
		/**
		 * Если значение является логическим
		 */
		case static_cast <uint32_t> (type_t::BOOL): {
			// Выполняем установку логического значения
			node.length(value.boolean ? 1 : 0);
		} break;
		/**
		 * Если значение является строкой, двоичными данными либо опознавателем
		 */
		case static_cast <uint32_t> (type_t::STRING):
		case static_cast <uint32_t> (type_t::BLOB):
		case static_cast <uint32_t> (type_t::UUID): {
			// Если содержимое значения в хранилище октетов не умещается
			if(!::room(self->_storage.size(), value.data.size())){
				// Выполняем установку кода отказа недопустимой длины
				self->fail(error_t::INVALID_LENGTH);
				// Сообщаем, что сборка дерева отвечена отказом
				return false;
			}
			// Выполняем установку смещения содержимого в хранилище
			node.offset = static_cast <uint32_t> (self->_storage.size());
			// Выполняем установку длины содержимого
			node.length(static_cast <uint32_t> (value.data.size()));
			// Выполняем перенос содержимого в хранилище
			self->_storage.append(value.data);
		} break;
		/**
		 * Если значение является целым неограниченной ширины либо десятичным
		 */
		case static_cast <uint32_t> (type_t::EXTENDED):
		case static_cast <uint32_t> (type_t::DECIMAL): {
			// Если величина вместе с порядком в хранилище октетов не умещается
			if(!::room(self->_storage.size(), value.data.size() + 8)){
				// Выполняем установку кода отказа недопустимой длины
				self->fail(error_t::INVALID_LENGTH);
				// Сообщаем, что сборка дерева отвечена отказом
				return false;
			}
			// Выполняем установку смещения содержимого в хранилище
			node.offset = static_cast <uint32_t> (self->_storage.size());
			// Выполняем установку длины октетов величины
			node.length(static_cast <uint32_t> (value.data.size()));
			// Выполняем установку признака отрицательности величины
			node.negative = value.negative;
			// Разрядная запись десятичного порядка величины
			const uint64_t exponent = static_cast <uint64_t> (value.exponent);
			/**
			 * Выполняем перенос десятичного порядка в хранилище. Порядок кладётся
			 * впереди величины оттого, что поля под него у узла нет: восемь октетов
			 * содержимого заняты длиною величины, а расширять узел ради одного вида
			 * значения дороже, чем положить порядок в хранилище
			 */
			for(uint8_t i = 0; i < 8; i++)
				// Выполняем перенос очередного октета порядка в хранилище
				self->_storage.push_back(static_cast <char> ((exponent >> (i * 8)) & 0xFF));
			// Выполняем перенос октетов величины в хранилище
			self->_storage.append(value.data);
		} break;
		/**
		 * Если значение является открытым расширением
		 */
		case static_cast <uint32_t> (type_t::CUSTOM): {
			// Если расширение вместе с номером подвида в хранилище не умещается
			if(!::room(self->_storage.size(), value.data.size() + 8)){
				// Выполняем установку кода отказа недопустимой длины
				self->fail(error_t::INVALID_LENGTH);
				// Сообщаем, что сборка дерева отвечена отказом
				return false;
			}
			// Выполняем установку смещения содержимого в хранилище
			node.offset = static_cast <uint32_t> (self->_storage.size());
			// Выполняем установку длины октетов расширения
			node.length(static_cast <uint32_t> (value.data.size()));
			// Номер подвида расширения, заведённый потребителем
			const uint64_t subtype = value.number;
			/**
			 * Выполняем перенос номера подвида в хранилище. Кладётся он впереди
			 * октетов расширения тем же порядком, что и десятичный порядок величины:
			 * поля под него у узла нет, а расширять узел ради одного вида значения
			 * дороже, чем положить номер в хранилище
			 */
			for(uint8_t i = 0; i < 8; i++)
				// Выполняем перенос очередного октета номера подвида в хранилище
				self->_storage.push_back(static_cast <char> ((subtype >> (i * 8)) & 0xFF));
			// Выполняем перенос октетов расширения в хранилище
			self->_storage.append(value.data);
		} break;
		/**
		 * Если значение является вместимым
		 */
		case static_cast <uint32_t> (type_t::ARRAY):
		case static_cast <uint32_t> (type_t::MAP): {
			// Выполняем установку количества значений вместимого
			node.length(0);
		} break;
		/**
		 * Если значение является числом родного вида либо отметкой времени
		 */
		default: {
			// Разрядная запись значения
			uint64_t bits = 0;
			// Если значение является дробным
			if(static_cast <uint32_t> (value.type) & static_cast <uint32_t> (type_t::REAL)){
				// Снимаемое дробное значение
				const double real = value.real;
				// Выполняем снятие разрядной записи дробного значения
				::memcpy(&bits, &real, sizeof(bits));
			// Если значение является целым со знаком либо отметкой времени
			} else if((static_cast <uint32_t> (value.type) & static_cast <uint32_t> (type_t::SIGNED)) ||
			          (value.type == type_t::TIME))
				// Выполняем снятие разрядной записи целого со знаком
				bits = static_cast <uint64_t> (value.integer);
			// Выполняем снятие разрядной записи целого без знака
			else bits = value.number;
			// Выполняем перенос разрядной записи значения в поле содержимого
			::memcpy(node.content, &bits, sizeof(bits));
		} break;
	}
	/**
	 * Если узел стоит внутри вместимого, учитываем его ребёнком. Имя поля
	 * отображения учитывается наравне со значением: имя есть такой же узел, и
	 * отображение из `N` пар несёт `2N` детей
	 */
	if(!state->stack.empty())
		// Выполняем учёт ребёнка вместившего вместимого
		self->_nodes.at(state->stack.back()).length(self->_nodes.at(state->stack.back()).length() + 1);
	// Выполняем получение номера заводимого узла
	const uint32_t index = static_cast <uint32_t> (self->_nodes.size());
	// Выполняем добавление узла в дерево документа
	self->_nodes.push_back(node);
	// Если узел является вместимым
	if((event == event_t::ARRAY_BEGIN) || (event == event_t::MAP_BEGIN))
		// Выполняем добавление номера узла в стек открытых вместимых
		state->stack.push_back(index);
	// Сообщаем, что событие принято
	return true;
}
/**
 * @brief Метод разбора записи в дерево документа
 *
 * @param buffer   буфер разбираемой записи
 * @param size     размер разбираемой записи в октетах
 * @param settings настройки разбора записи
 * @return         признак успешности разбора
 *
 */
bool awh::codec::abc::Document::parse(const void * buffer, const size_t size, const reader_t::settings_t & settings) noexcept {
	// Выполняем сброс состояния документа
	this->clear();
	// Читатель бинарной записи
	reader_t reader(this->_log);
	// Выполняем установку настроек разбора записи
	reader.settings(settings);
	// Состояние сборки дерева документа
	building_t state{this, {}, numeric_limits <uint32_t>::max(), false};
	/**
	 * Выполняем установку обработчика прямой выдачи событий разбора.
	 *
	 * События приходят прямо в сборку дерева, минуя очередь выдачи разбирателя:
	 * событие ложилось в неё, а затем снималось с неё же копией, - работа двойная
	 */
	reader.handler(& Document::assemble, & state);
	// Выполняем подачу записи разбирателю
	const bool result = reader.feed(buffer, size, true);
	// Выполняем снятие обработчика прямой выдачи событий разбора
	reader.handler(nullptr, nullptr);
	/**
	 * Если подача записи отвечена отказом
	 */
	if(!result){
		/**
		 * Если отказ объявлен не сборкой дерева, причина берётся у разбирателя: сборка
		 * дерева причину свою записала сама, и затирать её кодом отказа разбора нельзя
		 */
		if(!state.failed)
			// Выполняем установку кода отказа разбора
			this->_error = reader.error();
		// Сообщаем, что разбор отвечен отказом
		return false;
	}
	// Если стек открытых вместимых не опустел
	if(!state.stack.empty()){
		// Выполняем установку кода внутреннего отказа
		this->fail(error_t::INTERNAL);
		// Сообщаем, что разбор отвечен отказом
		return false;
	}
	// Если дерево документа осталось пустым
	if(this->_nodes.empty()){
		// Выполняем установку кода отказа пустой записи
		this->fail(error_t::EMPTY_RECORD);
		// Сообщаем, что разбор отвечен отказом
		return false;
	}
	// Сообщаем, что разбор успешен
	return true;
}

/**
 * @brief Метод разбора записи в дерево документа
 *
 * @param buffer буфер разбираемой записи
 * @param size   размер разбираемой записи в октетах
 * @return       признак успешности разбора
 *
 */
bool awh::codec::abc::Document::parse(const void * buffer, const size_t size) noexcept {
	// Настройки разбора записи по умолчанию
	const reader_t::settings_t settings;
	// Выполняем разбор записи в дерево документа
	return this->parse(buffer, size, settings);
}
/**
 * @brief Метод сборки записи из дерева документа
 *
 * @param writer сборщик бинарной записи
 * @return       признак успешности сборки
 *
 */
bool awh::codec::abc::Document::build(writer_t & writer) const noexcept {
	// Выполняем сброс кода отказа сборки записи
	this->_error = error_t::NONE;
	/**
	 * Если дерево документа пусто
	 */
	if(this->_nodes.empty()){
		// Выполняем установку кода отказа сборки записи
		this->_error = error_t::EMPTY_RECORD;
		// Сообщаем, что сборка отвечена отказом
		return false;
	}
	// Стек количеств оставшихся детей открытых вместимых
	vector <pair <bool, uint32_t>> stack;
	/**
	 * Выполняем обход всех узлов дерева документа
	 */
	for(size_t index = 0; index < this->_nodes.size(); index++){
		// Выполняем получение очередного узла дерева документа
		const node_t & node = this->_nodes.at(index);
		// Признак успешности укладки значения
		bool result = true;
		/**
		 * Определяем вид значения узла документа
		 */
		switch(static_cast <uint32_t> (node.type)){
			// Если значение является пустым
			case static_cast <uint32_t> (type_t::NUL): result = writer.nul(); break;
			// Если значение является логическим
			case static_cast <uint32_t> (type_t::BOOL): result = writer.boolean(node.length() != 0); break;
			/**
			 * Если значение является строкой
			 */
			case static_cast <uint32_t> (type_t::STRING): {
				// Выполняем укладку строки
				result = writer.text(string_view(this->_storage.data() + node.offset, node.length()));
			} break;
			/**
			 * Если значение является двоичными данными
			 */
			case static_cast <uint32_t> (type_t::BLOB): {
				// Выполняем укладку двоичных данных
				result = writer.blob(this->_storage.data() + node.offset, node.length());
			} break;
			/**
			 * Если значение является опознавателем
			 */
			case static_cast <uint32_t> (type_t::UUID): {
				// Выполняем укладку опознавателя
				result = writer.uuid(this->_storage.data() + node.offset, node.length());
			} break;
			/**
			 * Если значение является целым неограниченной ширины либо десятичным
			 */
			case static_cast <uint32_t> (type_t::EXTENDED):
			case static_cast <uint32_t> (type_t::DECIMAL): {
				// Разрядная запись десятичного порядка величины
				uint64_t bits = 0;
				/**
				 * Выполняем снятие десятичного порядка величины из хранилища
				 */
				for(uint8_t i = 0; i < 8; i++)
					// Выполняем снятие очередного октета порядка
					bits |= (static_cast <uint64_t> (static_cast <uint8_t> (this->_storage.at(node.offset + i))) << (i * 8));
				// Выполняем укладку десятичного числа
				result = writer.decimal(this->_storage.data() + node.offset + 8, node.length(),
				 node.negative, static_cast <int64_t> (bits));
			} break;
			/**
			 * Если значение является открытым расширением
			 */
			case static_cast <uint32_t> (type_t::CUSTOM): {
				// Номер подвида расширения, заведённый потребителем
				uint64_t subtype = 0;
				/**
				 * Выполняем снятие номера подвида расширения из хранилища
				 */
				for(uint8_t i = 0; i < 8; i++)
					// Выполняем снятие очередного октета номера подвида
					subtype |= (static_cast <uint64_t> (static_cast <uint8_t> (this->_storage.at(node.offset + i))) << (i * 8));
				// Выполняем укладку открытого расширения
				result = writer.custom(subtype, this->_storage.data() + node.offset + 8, node.length());
			} break;
			/**
			 * Если значение является вместимым
			 */
			case static_cast <uint32_t> (type_t::ARRAY):
			case static_cast <uint32_t> (type_t::MAP): {
				// Признак того, что вместимое является отображением
				const bool mapping = (node.type == type_t::MAP);
				// Если вместимое является отображением
				if(mapping)
					// Выполняем укладку начала отображения
					result = writer.mapBegin(static_cast <uint64_t> (node.length() / 2));
				// Выполняем укладку начала массива
				else result = writer.arrayBegin(static_cast <uint64_t> (node.length()));
				// Если укладка начала вместимого успешна
				if(result)
					// Выполняем добавление вместимого в стек открытых
					stack.push_back(make_pair(mapping, node.length()));
			} break;
			/**
			 * Если значение является отметкой времени
			 */
			case static_cast <uint32_t> (type_t::TIME): {
				// Разрядная запись отметки времени
				uint64_t bits = 0;
				// Выполняем снятие разрядной записи отметки времени
				::memcpy(&bits, node.content, sizeof(bits));
				// Выполняем укладку отметки времени
				result = writer.timestamp(static_cast <int64_t> (bits));
			} break;
			/**
			 * Если значение является числом родного вида
			 */
			default: {
				// Разрядная запись значения
				uint64_t bits = 0;
				// Выполняем снятие разрядной записи значения
				::memcpy(&bits, node.content, sizeof(bits));
				// Если значение является дробным
				if(static_cast <uint32_t> (node.type) & static_cast <uint32_t> (type_t::REAL)){
					// Снимаемое дробное значение
					double real = 0.0;
					// Выполняем снятие дробного значения из разрядной записи
					::memcpy(&real, &bits, sizeof(real));
					/**
					 * Если значение уложено было одинарной точностью, кладём его ею же
					 *
					 * @note Без этого дробное одинарной точности росло при перекладке вдвое -
					 *       четыре октета обращались в восемь, - и запись, разобранная деревом
					 *       и уложенная наново, переставала совпадать с исходной октет в октет
					 */
					if(static_cast <uint32_t> (node.type) & static_cast <uint32_t> (type_t::FLOAT))
						// Выполняем укладку дробного значения одинарной точности
						result = writer.number(static_cast <float> (real));
					// Выполняем укладку дробного значения двойной точности
					else result = writer.number(real);
				// Если значение является целым со знаком
				} else if(static_cast <uint32_t> (node.type) & static_cast <uint32_t> (type_t::SIGNED))
					// Выполняем укладку целого со знаком
					result = writer.number(static_cast <int64_t> (bits));
				// Выполняем укладку целого без знака
				else result = writer.number(bits);
			} break;
		}
		/**
		 * Если укладка значения отвечена отказом
		 */
		if(!result){
			// Выполняем перенос повода отказа от сборки записи
			this->_error = writer.error();
			// Сообщаем, что сборка отвечена отказом
			return false;
		}
		/**
		 * Признак того, что уложенное значение завершено. Вместимое, детей не имеющее,
		 * завершено сразу: значения, какое бы его закрыло, за ним не последует, и без
		 * этого оно осталось бы открытым до конца обхода
		 */
		bool settled = !node.container();
		// Если вместимое детей не имеет
		if(node.container() && (node.length() == 0)){
			// Признак того, что закрываемое вместимое является отображением
			const bool mapping = stack.back().first;
			// Выполняем снятие вместимого со стека открытых
			stack.pop_back();
			/**
			 * Если закрытие вместимого отвечено отказом
			 */
			if(!(mapping ? writer.mapEnd() : writer.arrayEnd())){
				// Выполняем перенос повода отказа от сборки записи
				this->_error = writer.error();
				// Сообщаем, что сборка отвечена отказом
				return false;
			}
			// Выполняем установку признака завершённости уложенного значения
			settled = true;
		}
		// Если уложенное значение не завершено
		if(!settled)
			// Продолжаем обход узлов дерева документа
			continue;
		/**
		 * Выполняем закрытие всех вместимых, чьи дети исчерпаны
		 */
		while(!stack.empty()){
			// Выполняем учёт уложенного значения вместимого
			stack.back().second--;
			// Если дети вместимого ещё не исчерпаны
			if(stack.back().second > 0)
				// Выходим из обхода стека вместимых
				break;
			// Признак того, что закрываемое вместимое является отображением
			const bool mapping = stack.back().first;
			// Выполняем снятие вместимого со стека открытых
			stack.pop_back();
			/**
			 * Если закрытие вместимого отвечено отказом
			 */
			if(!(mapping ? writer.mapEnd() : writer.arrayEnd())){
				// Выполняем перенос повода отказа от сборки записи
				this->_error = writer.error();
				// Сообщаем, что сборка отвечена отказом
				return false;
			}
		}
	}
	/**
	 * Если вместимые остались незакрытыми
	 */
	if(!stack.empty()){
		// Выполняем установку кода отказа сборки записи
		this->_error = error_t::UNBALANCED_CONTAINER;
		// Сообщаем, что сборка отвечена отказом
		return false;
	}
	// Сообщаем об успешности сборки записи
	return true;
}
/**
 * @brief Метод заведения указателя имён полей отображения
 *
 * @param index номер узла отображения в дереве документа
 * @return      заведённый указатель имён полей
 *
 */
const unordered_map <string_view, uint32_t> & awh::codec::abc::Document::naming(const uint32_t index) const noexcept {
	// Выполняем поиск уже заведённого указателя имён полей
	auto i = this->_index.find(index);
	// Если указатель имён полей уже заведён
	if(i != this->_index.end())
		// Выводим заведённый указатель имён полей
		return i->second;
	// Заводимый указатель имён полей отображения
	unordered_map <string_view, uint32_t> naming;
	// Выполняем получение номера первого ребёнка отображения
	uint32_t child = (index + 1);
	// Выполняем получение предела обхода детей отображения
	const uint32_t bound = (index + this->_nodes.at(index).extent());
	/**
	 * Выполняем обход всех детей отображения
	 */
	while(child < bound){
		// Выполняем получение узла имени поля отображения
		const node_t & key = this->_nodes.at(child);
		// Выполняем получение номера узла значения поля отображения
		const uint32_t value = (child + key.extent());
		// Если значение поля отображения за предел обхода вышло
		if(value >= bound)
			// Выходим из обхода детей отображения
			break;
		// Если имя поля отображения является строкой
		if(key.type == type_t::STRING)
			// Выполняем добавление имени поля в указатель
			naming.emplace(string_view(this->_storage.data() + key.offset, key.length()), value);
		// Выполняем переход к следующему имени поля отображения
		child = (value + this->_nodes.at(value).extent());
	}
	// Выводим заведённый указатель имён полей
	return this->_index.emplace(index, std::move(naming)).first->second;
}
/**
 * @brief Метод извлечения корня дерева документа
 *
 * @return ссылка на корень дерева документа
 *
 */
awh::codec::abc::Document::value_t awh::codec::abc::Document::root() const noexcept {
	// Если дерево документа пусто
	if(this->_nodes.empty())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выводим ссылку на корень дерева документа
	return value_t(this, 0, static_cast <uint32_t> (this->_nodes.size()));
}
/**
 * @brief Метод извлечения количества узлов дерева документа
 *
 * @return количество узлов дерева документа
 *
 */
size_t awh::codec::abc::Document::nodes() const noexcept {
	// Выводим количество узлов дерева документа
	return this->_nodes.size();
}
/**
 * @brief Метод извлечения кода отказа разбора записи
 *
 * @return код отказа разбора записи
 *
 */
awh::codec::abc::error_t awh::codec::abc::Document::error() const noexcept {
	// Выводим код отказа разбора записи
	return this->_error;
}
/**
 * @brief Метод проверки действительности ссылки
 *
 * @return признак действительности ссылки
 *
 */
bool awh::codec::abc::Document::Value::valid() const noexcept {
	// Выводим признак действительности ссылки на значение
	return ((this->_doc != nullptr) && (this->_index < this->_doc->_nodes.size()));
}
/**
 * @brief Метод извлечения вида значения
 *
 * @return вид значения документа
 *
 */
awh::codec::abc::type_t awh::codec::abc::Document::Value::type() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим вид отсутствующего значения
		return type_t::UNDEFINED;
	// Выводим вид значения документа
	return this->_doc->_nodes.at(this->_index).type;
}
/**
 * @brief Метод извлечения вида узла
 *
 * @return вид узла документа
 *
 */
awh::codec::abc::kind_t awh::codec::abc::Document::Value::kind() const noexcept {
	// Выводим вид узла документа
	return abc::kind(this->type());
}
/**
 * @brief Метод проверки принадлежности значения к виду
 *
 * @param type вид значения, сборный либо точный
 * @return     признак принадлежности значения к виду
 *
 */
bool awh::codec::abc::Document::Value::is(const type_t type) const noexcept {
	// Выводим признак принадлежности значения к виду
	return ((static_cast <uint32_t> (this->type()) & static_cast <uint32_t> (type)) != 0);
}
/**
 * @brief Метод извлечения количества значений вместимого
 *
 * @return количество значений вместимого
 *
 */
size_t awh::codec::abc::Document::Value::size() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим отсутствие значений
		return 0;
	// Выполняем получение узла значения документа
	const node_t & node = this->_doc->_nodes.at(this->_index);
	// Если узел вместимым не является
	if(!node.container())
		// Выводим отсутствие значений
		return 0;
	// Если узел является отображением
	if(node.type == type_t::MAP)
		// Выводим количество пар отображения
		return static_cast <size_t> (node.length() / 2);
	// Выводим количество значений массива
	return static_cast <size_t> (node.length());
}
/**
 * @brief Метод извлечения значения вместимого по его номеру
 *
 * @param index номер значения вместимого
 * @return      ссылка на значение вместимого
 *
 */
awh::codec::abc::Document::value_t awh::codec::abc::Document::Value::at(const size_t index) const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выполняем получение узла значения документа
	const node_t & node = this->_doc->_nodes.at(this->_index);
	// Если узел вместимым не является
	if(!node.container())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Если затребованного значения у вместимого нет
	if(index >= this->size())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Признак того, что вместимое является отображением
	const bool mapping = (node.type == type_t::MAP);
	// Выполняем получение количества узлов, пропускаемых до затребованного
	const size_t skip = (mapping ? ((index * 2) + 1) : index);
	// Выполняем получение номера первого ребёнка вместимого
	uint32_t child = (this->_index + 1);
	/**
	 * Выполняем пропуск узлов, стоящих до затребованного
	 */
	for(size_t i = 0; i < skip; i++)
		// Выполняем пропуск поддерева очередного узла целиком
		child += this->_doc->_nodes.at(child).extent();
	// Выводим ссылку на затребованное значение вместимого
	return value_t(this->_doc, child, (this->_index + node.extent()));
}
/**
 * @brief Метод извлечения имени поля отображения по его номеру
 *
 * @param index номер пары отображения
 * @return      ссылка на имя поля отображения
 *
 */
awh::codec::abc::Document::value_t awh::codec::abc::Document::Value::key(const size_t index) const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выполняем получение узла значения документа
	const node_t & node = this->_doc->_nodes.at(this->_index);
	// Если узел отображением не является
	if(node.type != type_t::MAP)
		// Выводим недействительную ссылку на значение
		return value_t();
	// Если затребованной пары у отображения нет
	if(index >= this->size())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выполняем получение номера первого ребёнка отображения
	uint32_t child = (this->_index + 1);
	/**
	 * Выполняем пропуск пар, стоящих до затребованной
	 */
	for(size_t i = 0; i < (index * 2); i++)
		// Выполняем пропуск поддерева очередного узла целиком
		child += this->_doc->_nodes.at(child).extent();
	// Выводим ссылку на имя затребованного поля отображения
	return value_t(this->_doc, child, (this->_index + node.extent()));
}
/**
 * @brief Метод извлечения первого значения вместимого
 *
 * @return ссылка на первое значение вместимого
 *
 */
awh::codec::abc::Document::value_t awh::codec::abc::Document::Value::begin() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выполняем получение узла значения документа
	const node_t & node = this->_doc->_nodes.at(this->_index);
	// Если узел вместимым не является либо вместимое пусто
	if(!node.container() || (node.length() == 0))
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выводим ссылку на первое значение вместимого
	return value_t(this->_doc, (this->_index + 1), (this->_index + node.extent()));
}
/**
 * @brief Метод перехода к следующему значению вместимого
 *
 * @return ссылка на следующее значение вместимого
 *
 */
awh::codec::abc::Document::value_t awh::codec::abc::Document::Value::next() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выполняем получение номера следующего значения вместимого
	const uint32_t index = (this->_index + this->_doc->_nodes.at(this->_index).extent());
	// Если следующее значение вышло за границу вместимого
	if(index >= this->_bound)
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выводим ссылку на следующее значение вместимого
	return value_t(this->_doc, index, this->_bound);
}
/**
 * @brief Метод проверки того, что значение является именем поля отображения
 *
 * @return признак того, что значение является именем поля отображения
 *
 */
bool awh::codec::abc::Document::Value::keyed() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим признак того, что значение именем поля не является
		return false;
	// Выводим признак того, что значение является именем поля отображения
	return this->_doc->_nodes.at(this->_index).keyed;
}
/**
 * @brief Метод извлечения значения поля отображения по имени
 *
 * @param name имя поля отображения
 * @return     ссылка на значение поля отображения
 *
 */
awh::codec::abc::Document::value_t awh::codec::abc::Document::Value::get(const string_view name) const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим недействительную ссылку на значение
		return value_t();
	// Если узел отображением не является
	if(this->_doc->_nodes.at(this->_index).type != type_t::MAP)
		// Выводим недействительную ссылку на значение
		return value_t();
	// Выполняем получение указателя имён полей отображения
	const unordered_map <string_view, uint32_t> & naming = this->_doc->naming(this->_index);
	// Выполняем поиск затребованного имени поля отображения
	auto i = naming.find(name);
	// Если затребованное имя поля отображения найдено
	if(i != naming.end())
		// Выводим ссылку на значение затребованного поля
		return value_t(this->_doc, i->second,
		 (this->_index + this->_doc->_nodes.at(this->_index).extent()));
	// Выводим недействительную ссылку на значение
	return value_t();
}
/**
 * @brief Метод проверки наличия поля отображения по имени
 *
 * @param name имя поля отображения
 * @return     признак наличия поля отображения
 *
 */
bool awh::codec::abc::Document::Value::has(const string_view name) const noexcept {
	// Выводим признак наличия поля отображения
	return this->get(name).valid();
}
/**
 * @brief Метод извлечения логического значения
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Document::Value::value(bool & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = false;
	// Если значение логическим не является
	if(this->type() != type_t::BOOL)
		// Сообщаем, что извлечение отвечено отказом
		return false;
	// Выполняем установку извлекаемого значения
	result = (this->_doc->_nodes.at(this->_index).length() != 0);
	// Сообщаем, что извлечение успешно
	return true;
}
/**
 * @brief Метод извлечения числа видом целого без знака
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Document::Value::value(uint64_t & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = 0;
	// Выполняем получение вида значения документа
	const type_t type = this->type();
	// Разрядная запись значения
	uint64_t bits = 0;
	// Если значение числом родного вида не является
	if(!((static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::INT)) ||
	     (static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::REAL))))
		// Сообщаем, что извлечение отвечено отказом
		return false;
	// Выполняем снятие разрядной записи значения
	::memcpy(&bits, this->_doc->_nodes.at(this->_index).content, sizeof(bits));
	// Если значение является целым без знака
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::UNSIGNED)){
		// Выполняем установку извлекаемого значения
		result = bits;
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым со знаком
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::SIGNED)){
		/**
		 * Выполняем установку извлекаемого значения переносом младших разрядов
		 *
		 * @note Отрицательное число видом без знака не представимо, и переносится оно
		 *       младшими разрядами: договор извлечения общий у кодеков рамки
		 */
		result = bits;
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Снимаемое дробное значение
	double real = 0.0;
	// Выполняем снятие дробного значения из разрядной записи
	::memcpy(&real, &bits, sizeof(real));
	// Выполняем установку извлекаемого значения приведением к затребованному виду
	result = ::convert <uint64_t> (real);
	// Сообщаем, что извлечение успешно
	return true;
}
/**
 * @brief Метод извлечения числа видом целого со знаком
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Document::Value::value(int64_t & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = 0;
	// Выполняем получение вида значения документа
	const type_t type = this->type();
	// Разрядная запись значения
	uint64_t bits = 0;
	// Если значение является отметкой времени
	if(type == type_t::TIME){
		// Выполняем снятие разрядной записи отметки времени
		::memcpy(&bits, this->_doc->_nodes.at(this->_index).content, sizeof(bits));
		// Выполняем установку извлекаемого значения
		result = static_cast <int64_t> (bits);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение числом родного вида не является
	if(!((static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::INT)) ||
	     (static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::REAL))))
		// Сообщаем, что извлечение отвечено отказом
		return false;
	// Выполняем снятие разрядной записи значения
	::memcpy(&bits, this->_doc->_nodes.at(this->_index).content, sizeof(bits));
	// Если значение является целым со знаком
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::SIGNED)){
		// Выполняем установку извлекаемого значения
		result = static_cast <int64_t> (bits);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым без знака
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::UNSIGNED)){
		/**
		 * Выполняем установку извлекаемого значения переносом младших разрядов
		 *
		 * @note Число, за отрезок затребованного вида выходящее, переносится младшими
		 *       разрядами, а не отвечается отказом: договор извлечения общий у кодеков
		 */
		result = static_cast <int64_t> (bits);
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Снимаемое дробное значение
	double real = 0.0;
	// Выполняем снятие дробного значения из разрядной записи
	::memcpy(&real, &bits, sizeof(real));
	// Выполняем установку извлекаемого значения приведением к затребованному виду
	result = ::convert <int64_t> (real);
	// Сообщаем, что извлечение успешно
	return true;
}
/**
 * @brief Метод извлечения числа видом дробного
 *
 * @param result извлекаемое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::abc::Document::Value::value(double & result) const noexcept {
	// Выполняем сброс извлекаемого значения
	result = 0.0;
	// Выполняем получение вида значения документа
	const type_t type = this->type();
	// Если значение числом родного вида не является
	if(!((static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::INT)) ||
	     (static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::REAL))))
		// Сообщаем, что извлечение отвечено отказом
		return false;
	// Разрядная запись значения
	uint64_t bits = 0;
	// Выполняем снятие разрядной записи значения
	::memcpy(&bits, this->_doc->_nodes.at(this->_index).content, sizeof(bits));
	// Если значение является дробным
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::REAL)){
		// Выполняем снятие дробного значения из разрядной записи
		::memcpy(&result, &bits, sizeof(result));
		// Сообщаем, что извлечение успешно
		return true;
	}
	// Если значение является целым со знаком
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::SIGNED))
		// Выполняем установку извлекаемого значения
		result = static_cast <double> (static_cast <int64_t> (bits));
	// Выполняем установку извлекаемого значения
	else result = static_cast <double> (bits);
	// Сообщаем, что извлечение успешно
	return true;
}
/**
 * @brief Метод извлечения содержимого значения
 *
 * @return содержимое значения
 *
 */
string_view awh::codec::abc::Document::Value::data() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим пустое содержимое значения
		return string_view();
	// Выполняем получение узла значения документа
	const node_t & node = this->_doc->_nodes.at(this->_index);
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint32_t> (node.type)){
		// Если значение хранится отрезком октетов
		case static_cast <uint32_t> (type_t::STRING):
		case static_cast <uint32_t> (type_t::BLOB):
		case static_cast <uint32_t> (type_t::UUID):
			// Выводим содержимое значения
			return string_view(this->_doc->_storage.data() + node.offset, node.length());
		// Если значение является числом неограниченной ширины
		case static_cast <uint32_t> (type_t::EXTENDED):
		case static_cast <uint32_t> (type_t::DECIMAL):
			// Выводим октеты величины числа, стоящие за десятичным порядком
			return string_view(this->_doc->_storage.data() + node.offset + 8, node.length());
		// Если значение является открытым расширением
		case static_cast <uint32_t> (type_t::CUSTOM):
			// Выводим октеты расширения, стоящие за номером его подвида
			return string_view(this->_doc->_storage.data() + node.offset + 8, node.length());
	}
	// Выводим пустое содержимое значения
	return string_view();
}
/**
 * @brief Метод извлечения десятичного порядка величины
 *
 * @return десятичный порядок величины
 *
 */
int64_t awh::codec::abc::Document::Value::exponent() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим отсутствие десятичного порядка
		return 0;
	// Выполняем получение узла значения документа
	const node_t & node = this->_doc->_nodes.at(this->_index);
	// Если значение числом неограниченной ширины не является
	if((node.type != type_t::EXTENDED) && (node.type != type_t::DECIMAL))
		// Выводим отсутствие десятичного порядка
		return 0;
	// Разрядная запись десятичного порядка величины
	uint64_t bits = 0;
	/**
	 * Выполняем снятие десятичного порядка величины из хранилища
	 */
	for(uint8_t i = 0; i < 8; i++)
		// Выполняем снятие очередного октета порядка
		bits |= (static_cast <uint64_t> (static_cast <uint8_t> (this->_doc->_storage.at(node.offset + i))) << (i * 8));
	// Выводим десятичный порядок величины
	return static_cast <int64_t> (bits);
}
/**
 * @brief Метод извлечения номера подвида открытого расширения
 *
 * @return номер подвида открытого расширения
 *
 */
uint64_t awh::codec::abc::Document::Value::subtype() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим отсутствие номера подвида расширения
		return 0;
	// Выполняем получение узла значения документа
	const node_t & node = this->_doc->_nodes.at(this->_index);
	// Если значение открытым расширением не является
	if(node.type != type_t::CUSTOM)
		// Выводим отсутствие номера подвида расширения
		return 0;
	// Разрядная запись номера подвида расширения
	uint64_t bits = 0;
	/**
	 * Выполняем снятие номера подвида расширения из хранилища
	 */
	for(uint8_t i = 0; i < 8; i++)
		// Выполняем снятие очередного октета номера подвида
		bits |= (static_cast <uint64_t> (static_cast <uint8_t> (this->_doc->_storage.at(node.offset + i))) << (i * 8));
	// Выводим номер подвида расширения
	return bits;
}
/**
 * @brief Метод проверки того, что величина меньше нуля
 *
 * @return признак того, что величина меньше нуля
 *
 */
bool awh::codec::abc::Document::Value::negative() const noexcept {
	// Если ссылка на значение недействительна
	if(!this->valid())
		// Выводим признак неотрицательности величины
		return false;
	// Выводим признак того, что величина меньше нуля
	return this->_doc->_nodes.at(this->_index).negative;
}
