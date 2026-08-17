/**
 * @file reader.cpp
 * @date 2026-08-17
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
 * @brief Потоковое чтение текста YAML — разбор блочных построений стопою отступов с
 *        выдачей событий по мере чтения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/yaml/reader.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён контейнера YAML
 */
using namespace awh::codec::yaml;

/**
 * @brief Внутренние помощники потокового чтения
 *
 */
namespace {
	/**
	 * Начало меток типов, описанием заданных
	 *
	 * @note Сокращение `!!` раскрывается именно им, и описанием оно закреплено: правится
	 *       начало это лишь директивой `%TAG !! ...`, объявленной самим текстом
	 */
	static constexpr string_view STANDARD_TAG = "tag:yaml.org,2002:";
	/**
	 * @brief Функция проверки знака на принадлежность к пробельным
	 *
	 * @param letter проверяемый знак
	 * @return       признак принадлежности знака к пробельным
	 *
	 */
	static bool spacing(const char letter) noexcept {
		// Выводим признак принадлежности знака к пробельным
		return ((letter == ' ') || (letter == '\t'));
	}
	/**
	 * @brief Функция снятия пробельной обвязки с конца записи
	 *
	 * @param text обрезаемая запись
	 * @return     запись без пробельной обвязки в конце
	 *
	 */
	static string_view trimmed(const string_view text) noexcept {
		// Длина записи без пробельной обвязки
		size_t length = text.size();
		/**
		 * Выполняем снятие пробельных знаков с конца записи
		 */
		while((length > 0) && spacing(text[length - 1]))
			// Выполняем укорочение записи на один знак
			length--;
		// Выводим запись без пробельной обвязки в конце
		return text.substr(0, length);
	}
	/**
	 * @brief Функция снятия пробельной обвязки с обеих сторон записи
	 *
	 * @param text обрезаемая запись
	 * @return     запись без пробельной обвязки
	 *
	 */
	static string_view stripped(const string_view text) noexcept {
		// Смещение начала записи без пробельной обвязки
		size_t offset = 0;
		/**
		 * Выполняем снятие пробельных знаков с начала записи
		 */
		while((offset < text.size()) && spacing(text[offset]))
			// Выполняем переход к следующему знаку записи
			offset++;
		// Выводим запись без пробельной обвязки
		return trimmed(text.substr(offset));
	}
	/**
	 * @brief Функция проверки знака на пригодность имени метки
	 *
	 * @details Описание дозволяет имени метки всякий знак, кроме пробельных и кроме
	 *          знаков, поточные построения размечающих: имя обязано кончаться там, где
	 *          кончается оно и внутри скобок, иначе `[*метка,ещё]` прочлось бы одним именем
	 *
	 * @param letter проверяемый знак
	 * @return       признак пригодности знака имени метки
	 *
	 */
	static bool naming(const char letter) noexcept {
		/**
		 * Если знак пробельный
		 */
		if(spacing(letter))
			// Выводим признак непригодности знака имени метки
			return false;
		/**
		 * Определяем знак, имени метки не принадлежащий
		 */
		switch(letter){
			// Если знак размечает поточное построение
			case ',':
			case '[':
			case ']':
			case '{':
			case '}':
				// Выводим признак непригодности знака имени метки
				return false;
		}
		// Выводим признак пригодности знака имени метки
		return true;
	}
	/**
	 * @brief Функция проверки знака на пригодность записи метки типа
	 *
	 * @details Метка типа есть запись единообразного указателя, и знаки её описанием
	 *          заданы перечнем. Восклицательный знак в перечень тот не входит нарочно:
	 *          им отделяется сокращение метки от остатка её
	 *
	 * @param letter проверяемый знак
	 * @return       признак пригодности знака записи метки типа
	 *
	 */
	static bool tagging(const char letter) noexcept {
		/**
		 * Если знак есть буква либо цифра
		 */
		if(((letter >= 'a') && (letter <= 'z')) || ((letter >= 'A') && (letter <= 'Z')) ||
		   ((letter >= '0') && (letter <= '9')))
			// Выводим признак пригодности знака записи метки типа
			return true;
		/**
		 * Определяем знак, записи указателя принадлежащий
		 */
		switch(letter){
			// Если знак дозволен описанием записи указателя
			case '-': case ';': case '/': case '?': case ':': case '@': case '&':
			case '=': case '+': case '$': case ',': case '_': case '.': case '~':
			case '*': case '\'': case '(': case ')': case '%':
				// Выводим признак пригодности знака записи метки типа
				return true;
		}
		// Выводим признак непригодности знака записи метки типа
		return false;
	}
	/**
	 * @brief Функция разбора шестнадцатеричной записи знака Юникода
	 *
	 * @param text   разбираемая запись знака
	 * @param offset смещение начала записи
	 * @param count  количество разрядов записи
	 * @param code   получаемый знак Юникода
	 * @return       признак успешного разбора записи
	 *
	 */
	static bool hexadecimal(const string_view text, const size_t offset, const size_t count, uint32_t & code) noexcept {
		/**
		 * Если разрядов записи в строке не хватает
		 */
		if((offset + count) > text.size())
			// Выводим признак неудачного разбора записи
			return false;
		// Собираемый знак Юникода
		uint32_t result = 0;
		/**
		 * Выполняем перебор всех разрядов записи знака
		 */
		for(size_t i = 0; i < count; i++){
			// Получаем очередной разряд записи знака
			const char letter = text[offset + i];
			// Величина, отвечающая разряду записи
			uint32_t value = 0;
			/**
			 * Если разряд записан десятичной цифрой
			 */
			if((letter >= '0') && (letter <= '9'))
				// Получаем величину, отвечающую цифре
				value = static_cast <uint32_t> (letter - '0');
			/**
			 * Если разряд записан строчной буквой
			 */
			else if((letter >= 'a') && (letter <= 'f'))
				// Получаем величину, отвечающую букве
				value = static_cast <uint32_t> ((letter - 'a') + 10);
			/**
			 * Если разряд записан прописной буквой
			 */
			else if((letter >= 'A') && (letter <= 'F'))
				// Получаем величину, отвечающую букве
				value = static_cast <uint32_t> ((letter - 'A') + 10);
			/**
			 * Если знак разрядом записи не является вовсе
			 */
			else return false;
			// Выполняем добавление разряда к собираемому знаку
			result = ((result << 4) | value);
		}
		// Запоминаем разобранный знак Юникода
		code = result;
		// Выводим признак успешного разбора записи
		return true;
	}
}

/**
 * @brief Конструктор
 *
 */
awh::codec::yaml::Reader::Reader() noexcept :
 _state(state_t::READY), _error(error_t::NONE), _reading(0), _offset(0), _line(0), _position(0),
 _started(false), _opened(false), _filled(false), _blocking(false), _block(style_t::LITERAL),
 _chomp(chomp_t::CLIP), _marked(NO_INDENT), _outer(0), _margin(0), _inner(0), _opening(0),
 _breaks(0), _expected(false), _entered(false), _pending(0), _schema(schema_t::CORE), _plaining(false),
 _folds(0), _required(0), _phase(flow_t::ENTRY), _directed(false), _versioned(false) {}
/**
 * @brief Конструктор
 *
 * @param settings настройки разбора текста
 *
 */
awh::codec::yaml::Reader::Reader(const settings_t & settings) noexcept :
 _settings(settings), _state(state_t::READY), _error(error_t::NONE), _reading(0), _offset(0),
 _line(0), _position(0), _started(false), _opened(false), _filled(false),
 _blocking(false), _block(style_t::LITERAL), _chomp(chomp_t::CLIP), _marked(NO_INDENT),
 _outer(0), _margin(0), _inner(0), _opening(0), _breaks(0), _expected(false), _entered(false), _pending(0),
 _schema(settings.schema), _plaining(false), _folds(0), _required(0),
 _phase(flow_t::ENTRY), _directed(false), _versioned(false) {
	/**
	 * Если кодировка исходного текста навязана извне
	 */
	if(this->_settings.encoding != encoding_t::NONE)
		// Выполняем навязывание кодировки приведению
		this->_decoder.encoding(this->_settings.encoding);
}
/**
 * @brief Метод получения настроек разбора текста
 *
 * @return настройки разбора текста
 *
 */
const awh::codec::yaml::Reader::settings_t & awh::codec::yaml::Reader::settings() const noexcept {
	// Выводим настройки разбора текста
	return this->_settings;
}
/**
 * @brief Метод установки настроек разбора текста
 *
 * @param settings устанавливаемые настройки разбора
 * @return         признак принятия настроек разбора
 *
 */
bool awh::codec::yaml::Reader::settings(const settings_t & settings) noexcept {
	/**
	 * Если текст уже разбирается
	 *
	 * @note Смена правил разбора посреди текста развела бы начало его с концом: первая
	 *       половина оказалась бы прочитана одними правилами, вторая иными
	 */
	if(this->_state == state_t::PARSING)
		// Выводим признак непринятия настроек разбора
		return false;
	// Запоминаем настройки разбора текста
	this->_settings = settings;
	// Запоминаем схему разрешения видов, над документом действующую
	this->_schema = settings.schema;
	// Выполняем навязывание кодировки приведению
	this->_decoder.encoding(this->_settings.encoding);
	// Выводим признак принятия настроек разбора
	return true;
}
/**
 * @brief Метод сброса состояния потокового чтения
 *
 */
void awh::codec::yaml::Reader::clear() noexcept {
	// Выполняем сброс состояния потокового чтения
	this->_state = state_t::READY;
	// Выполняем сброс кода ошибки разбора текста
	this->_error = error_t::NONE;
	// Выполняем сброс положения отказа разбора
	this->_location = location_t();
	// Выполняем сброс накопителя поданного текста
	this->_buffer.clear();
	// Выполняем сброс смещения начала неразобранного текста
	this->_offset = 0;
	// Выполняем сброс хранилища содержимого собранных событий
	this->_storage.clear();
	// Выполняем сброс очереди собранных событий
	this->_events.clear();
	// Выполняем сброс номера очередного события очереди
	this->_reading = 0;
	// Выполняем сброс событий разбираемой строки
	this->_staged.clear();
	// Выполняем сброс события, выданного последним
	this->_current = item_t();
	// Выполняем сброс значения, выданного последним событием
	this->_content = content_t();
	// Выполняем сброс стопы открытых уровней вложенности
	this->_levels.clear();
	// Выполняем сброс номера разбираемой строки
	this->_line = 0;
	// Выполняем сброс смещения от начала текста
	this->_position = 0;
	// Выполняем сброс признака выдачи начала потока
	this->_started = false;
	// Выполняем сброс признака открытия документа
	this->_opened = false;
	// Выполняем сброс признака наполнения документа
	this->_filled = false;
	// Выполняем сброс признака сборки блочного значения
	this->_blocking = false;
	// Выполняем сброс собираемого содержимого блочного значения
	this->_block_text.clear();
	// Выполняем сброс количества пустых строк блочного значения
	this->_breaks = 0;
	// Выполняем сброс отступа содержимого блочного значения
	this->_inner = 0;
	// Выполняем сброс признака ожидания значения пары
	this->_expected = false;
	// Выполняем сброс признака принадлежности ожидаемого значения записи перечня
	this->_entered = false;
	// Выполняем сброс отступа, на котором ожидается значение пары
	this->_pending = 0;
	// Выполняем возврат схемы разрешения видов к назначенной настройками
	this->_schema = this->_settings.schema;
	// Выполняем сброс имени метки, узла своего ожидающей
	this->_anchor.clear();
	// Выполняем сброс метки типа, узла своего ожидающей
	this->_tag.clear();
	// Выполняем сброс признака сборки простого значения
	this->_plaining = false;
	// Выполняем сброс собираемого содержимого простого значения
	this->_plain.clear();
	// Выполняем сброс количества пустых строк простого значения
	this->_folds = 0;
	// Выполняем сброс положения начала простого значения
	this->_origin = location_t();
	// Выполняем сброс стопы открытых поточных построений
	this->_flow.clear();
	// Выполняем сброс состояния разбора поточного построения
	this->_phase = flow_t::ENTRY;
	// Выполняем сброс имён меток, документом объявленных
	this->_anchors.clear();
	// Выполняем сброс сокращений меток типов, директивами объявленных
	this->_handles.clear();
	// Выполняем сброс признака предпосланных документу директив
	this->_directed = false;
	// Выполняем сброс признака объявленного директивой наречия
	this->_versioned = false;
	// Выполняем сброс состояния приведения кодировки
	this->_decoder.reset();
}
/**
 * @brief Метод получения состояния потокового чтения
 *
 * @return состояние потокового чтения текста
 *
 */
state_t awh::codec::yaml::Reader::state() const noexcept {
	// Выводим состояние потокового чтения текста
	return this->_state;
}
/**
 * @brief Метод получения кода ошибки разбора текста
 *
 * @return код ошибки разбора текста
 *
 */
error_t awh::codec::yaml::Reader::error() const noexcept {
	// Выводим код ошибки разбора текста
	return this->_error;
}
/**
 * @brief Метод получения положения отказа в исходном тексте
 *
 * @return положение отказа разбора в исходном тексте
 *
 */
const location_t & awh::codec::yaml::Reader::location() const noexcept {
	// Выводим положение отказа разбора в исходном тексте
	return this->_location;
}
/**
 * @brief Метод получения опознанной кодировки исходного текста
 *
 * @return опознанная кодировка исходного текста
 *
 */
encoding_t awh::codec::yaml::Reader::encoding() const noexcept {
	// Выводим опознанную кодировку исходного текста
	return this->_decoder.encoding();
}
/**
 * @brief Метод получения вида последнего события разбора
 *
 * @return вид последнего события разбора
 *
 */
event_t awh::codec::yaml::Reader::event() const noexcept {
	// Выводим вид последнего события разбора
	return this->_current.event;
}
/**
 * @brief Метод получения значения последнего события разбора
 *
 * @return значение последнего события разбора
 *
 */
const content_t & awh::codec::yaml::Reader::value() const noexcept {
	// Выводим значение последнего события разбора
	return this->_content;
}
/**
 * @brief Метод объявления отказа разбора
 *
 * @param error  код ошибки разбора
 * @param column положение отказа в разбираемой строке
 * @return       признак прекращения разбора
 *
 */
bool awh::codec::yaml::Reader::fail(const error_t error, const size_t column) noexcept {
	// Запоминаем код ошибки разбора текста
	this->_error = error;
	// Запоминаем состояние прекращения разбора
	this->_state = state_t::FAILED;
	// Запоминаем смещение отказа от начала текста
	this->_location.offset = (this->_position + column);
	// Запоминаем номер строки, где отказ произошёл
	this->_location.line = this->_line;
	// Запоминаем положение отказа в строке
	this->_location.column = static_cast <uint32_t> (column + 1);
	// Запоминаем глубину вложенности, где отказ произошёл
	this->_location.depth = static_cast <uint32_t> (this->_levels.size());
	/**
	 * Выполняем сброс событий разбираемой строки
	 *
	 * @note Сбрасываются лишь события строки, на которой отказ и случился: выдать их
	 *       значило бы оставить потребителя с началом построения без конца его. События,
	 *       собранные строками прежними, остаются в очереди - они прочитаны верно, и
	 *       отзывать их поздно, ибо при подаче кусками они уже выданы
	 */
	this->_staged.clear();
	// Выводим признак прекращения разбора
	return false;
}
/**
 * @brief Метод постановки собранного события в очередь выдачи
 *
 * @param event  вид собранного события
 * @param column положение события в разбираемой строке
 * @return       ссылка на поставленное событие
 *
 */
awh::codec::yaml::Reader::item_t & awh::codec::yaml::Reader::emit(const event_t event, const size_t column) noexcept {
	// Выполняем постановку события в накопитель разбираемой строки
	this->_staged.emplace_back();
	// Получаем поставленное событие
	item_t & result = this->_staged.back();
	// Запоминаем вид собранного события
	result.event = event;
	// Запоминаем смещение события от начала текста
	result.location.offset = (this->_position + column);
	// Запоминаем номер строки, где событие собрано
	result.location.line = this->_line;
	// Запоминаем положение события в строке
	result.location.column = static_cast <uint32_t> (column + 1);
	// Запоминаем глубину вложенности, где событие собрано
	result.location.depth = static_cast <uint32_t> (this->_levels.size());
	// Запоминаем признак того, что событие собрано внутри поточного построения
	result.flow = !this->_flow.empty();
	// Выводим ссылку на поставленное событие
	return result;
}
/**
 * @brief Метод переноса собранных событий строки в очередь выдачи
 *
 */
void awh::codec::yaml::Reader::commit() noexcept {
	/**
	 * Выполняем перенос всех собранных событий строки в очередь выдачи
	 */
	for(size_t i = 0; i < this->_staged.size(); i++){
		/**
		 * Если все события очереди уже выданы
		 *
		 * @note Перечень сбрасывается лишь тогда, когда выдавать из него нечего: память
		 *       его тем переиспользуется, а выданные события уходят разом, а не по одному
		 */
		if(this->_reading >= this->_events.size()){
			// Выполняем сброс очереди выданных событий
			this->_events.clear();
			// Выполняем сброс номера очередного события очереди
			this->_reading = 0;
		}
		// Выполняем перенос очередного собранного события
		this->_events.emplace_back(this->_staged.at(i));
	}
	// Выполняем сброс накопителя событий разобранной строки
	this->_staged.clear();
}
/**
 * @brief Метод постановки события примечания
 *
 * @param line     разбираемая строка
 * @param position положение знака примечания в строке
 *
 */
void awh::codec::yaml::Reader::remark(const string_view line, const size_t position) noexcept {
	/**
	 * Если выдача примечаний не затребована
	 */
	if(!this->_settings.emitComments)
		// Выходим из постановки события примечания
		return;
	// Выполняем постановку события примечания
	item_t & item = this->emit(event_t::COMMENT, position);
	// Получаем содержимое примечания без знака его и без пробельной обвязки
	const string_view comment = stripped(line.substr(position + 1));
	// Запоминаем смещение содержимого события в хранилище знаков
	item.offset = this->_storage.size();
	// Запоминаем длину содержимого события
	item.length = comment.size();
	// Выполняем перенос содержимого события в хранилище знаков
	this->_storage.append(comment);
}
/**
 * @brief Метод постановки события скалярного значения
 *
 * @param text   содержимое скалярного значения
 * @param style  вид записи значения в исходном тексте
 * @param column положение значения в разбираемой строке
 * @return       признак успешной постановки события
 *
 */
bool awh::codec::yaml::Reader::scalar(const string & text, const style_t style, const size_t column) noexcept {
	// Получаем предел длины скалярного значения
	const size_t limit = ((this->_settings.scalar > 0) ? this->_settings.scalar : MAX_SCALAR);
	/**
	 * Если длина скалярного значения предел превышает
	 */
	if(text.size() > limit)
		// Выводим отказ превышения длины скалярного значения
		return this->fail(error_t::SCALAR_TOO_LONG, column);
	// Вид значения, разрешённый меткой типа либо действующей схемой
	type_t type = type_t::UNDEFINED;
	/**
	 * Если разрешить вид скалярного значения не удалось
	 *
	 * @note Разрешение идёт прежде постановки события: отказ метки типа не вправе
	 *       оставить в накопителе строки событие полусобранное
	 */
	if(!this->typing(text, style, column, type))
		// Выводим признак неудачной постановки события
		return false;
	// Выполняем постановку события скалярного значения
	item_t & item = this->emit(event_t::SCALAR, column);
	// Запоминаем вид записи значения в исходном тексте
	item.style = style;
	// Запоминаем вид значения, разрешённый меткой типа либо действующей схемой
	item.type = type;
	// Выполняем перенос накопленных свойств узла в собранное событие
	this->attach(item);
	// Запоминаем смещение содержимого события в хранилище знаков
	item.offset = this->_storage.size();
	// Запоминаем длину содержимого события
	item.length = text.size();
	// Выполняем перенос содержимого события в хранилище знаков
	this->_storage.append(text);
	// Запоминаем признак наполнения открытого документа
	this->_filled = true;
	// Выводим признак успешной постановки события
	return true;
}
/**
 * @brief Метод закрытия открытых уровней глубже заданного отступа
 *
 * @param indent отступ, до которого закрываются уровни
 * @param column положение закрытия в разбираемой строке
 * @return       признак успешного закрытия уровней
 *
 */
bool awh::codec::yaml::Reader::collapse(const uint32_t indent, const size_t column) noexcept {
	/**
	 * Выполняем закрытие всех уровней глубже заданного отступа
	 */
	while(!this->_levels.empty() && (this->_levels.back().indent > indent)){
		// Получаем вид закрываемого уровня вложенности
		const nesting_t kind = this->_levels.back().kind;
		// Выполняем снятие закрываемого уровня со стопы
		this->_levels.pop_back();
		// Выполняем постановку события закрытия уровня
		this->emit(((kind == nesting_t::MAPPING) ? event_t::MAPPING_END : event_t::SEQUENCE_END), column);
	}
	// Выводим признак успешного закрытия уровней
	return true;
}
/**
 * @brief Метод сличения метки типа с видом открываемого построения
 *
 * @param kind   вид открываемого построения
 * @param column положение открытия в разбираемой строке
 * @return       признак соответствия метки типа виду построения
 *
 */
bool awh::codec::yaml::Reader::matched(const nesting_t kind, const size_t column) noexcept {
	/**
	 * Если открываемому построению метка типа не предпослана
	 */
	if(this->_tag.empty())
		// Выводим признак соответствия метки типа виду построения
		return true;
	// Получаем окончание метки типа, вид её задающее
	const string_view suffix = ((this->_tag.compare(0, STANDARD_TAG.size(), STANDARD_TAG) == 0) ?
	 string_view(this->_tag).substr(STANDARD_TAG.size()) : string_view());
	/**
	 * Если метка типа задаёт построение, открываемому противное
	 *
	 * @note Метка `!!seq` над отображением есть не мелкая небрежность записи, а
	 *       расхождение объявленного с записанным: держащий документ построил бы по
	 *       метке одно, а по содержимому иное
	 */
	if((suffix == "seq") && (kind != nesting_t::SEQUENCE))
		// Выводим отказ несоответствия содержимого метке типа
		return this->fail(error_t::TAG_MISMATCH, column);
	/**
	 * Если метка типа задаёт отображение, открываемому построению противное
	 */
	else if((suffix == "map") && (kind != nesting_t::MAPPING))
		// Выводим отказ несоответствия содержимого метке типа
		return this->fail(error_t::TAG_MISMATCH, column);
	/**
	 * Если метка типа задаёт скалярное значение
	 */
	else if((suffix == "str") || (suffix == "int") || (suffix == "float") ||
	        (suffix == "bool") || (suffix == "null") || (suffix == "binary") || (suffix == "timestamp"))
		// Выводим отказ несоответствия содержимого метке типа
		return this->fail(error_t::TAG_MISMATCH, column);
	// Выводим признак соответствия метки типа виду построения
	return true;
}
/**
 * @brief Метод открытия уровня вложенности заданного вида
 *
 * @param kind    вид открываемого уровня вложенности
 * @param indent  отступ, на котором уровень открывается
 * @param implied признак открытия уровня значением пары на отступе имени её
 * @param column  положение открытия в разбираемой строке
 * @return       признак успешного открытия уровня
 *
 */
bool awh::codec::yaml::Reader::expand(const nesting_t kind, const uint32_t indent, const bool implied, const size_t column) noexcept {
	// Получаем предел глубины вложенности
	const size_t limit = ((this->_settings.depth > 0) ? this->_settings.depth : MAX_DEPTH);
	/**
	 * Если глубина вложенности предел превышает
	 */
	if(this->_levels.size() >= limit)
		// Выводим отказ превышения глубины вложенности
		return this->fail(error_t::DEPTH_EXCEEDED, column);
	/**
	 * Если метка типа, уровню предпосланная, виду его противна
	 */
	if(!this->matched(kind, column))
		// Выводим признак неудачного открытия уровня
		return false;
	// Выполняем постановку события открытия уровня
	item_t & item = this->emit(((kind == nesting_t::MAPPING) ? event_t::MAPPING_START : event_t::SEQUENCE_START), column);
	// Выполняем перенос накопленных свойств узла в собранное событие
	this->attach(item);
	// Выполняем открытие уровня вложенности
	this->_levels.emplace_back(kind, indent, implied);
	// Запоминаем признак наполнения открытого документа
	this->_filled = true;
	// Выводим признак успешного открытия уровня
	return true;
}
/**
 * @brief Метод закрытия открытого документа
 *
 * @param column положение закрытия в разбираемой строке
 * @return       признак успешного закрытия документа
 *
 */
bool awh::codec::yaml::Reader::finish(const size_t column) noexcept {
	/**
	 * Если документ не открыт вовсе
	 */
	if(!this->_opened)
		// Выводим признак успешного закрытия документа
		return true;
	/**
	 * Если пара осталась без значения своего
	 *
	 * @details Выдаётся оно прежде закрытия уровней, а не за ними: значение принадлежит
	 *          отображению, и выданное за концом его оказалось бы вне всякого построения.
	 *          Нашло это дерево документа - там значение легло вторым корнем потока
	 *
	 * @note Пустое значение описанием дозволено: `ключ:` знаменует пару с пустым
	 *       значением, и выдать её надлежит именно так
	 */
	if(this->_expected){
		// Выполняем сброс признака ожидания значения пары
		this->_expected = false;
		/**
		 * Если поставить событие пустого значения не удалось
		 */
		if(!this->scalar(string(), style_t::PLAIN, column))
			// Выводим признак неудачного закрытия документа
			return false;
		// Устанавливаем вид пустого значения последнему событию
		this->_staged.back().type = type_t::NUL;
	}
	/**
	 * Если закрыть открытые уровни вложенности не удалось
	 */
	if(!this->collapse(0, column))
		// Выводим признак неудачного закрытия документа
		return false;
	/**
	 * Если стопа уровней всё же не пуста
	 *
	 * @note Уровень нулевого отступа закрытием по отступу не снимается: снимается он
	 *       здесь, при закрытии документа
	 */
	while(!this->_levels.empty()){
		// Получаем вид закрываемого уровня вложенности
		const nesting_t kind = this->_levels.back().kind;
		// Выполняем снятие закрываемого уровня со стопы
		this->_levels.pop_back();
		// Выполняем постановку события закрытия уровня
		this->emit(((kind == nesting_t::MAPPING) ? event_t::MAPPING_END : event_t::SEQUENCE_END), column);
	}
	// Выполняем постановку события закрытия документа
	this->emit(event_t::DOCUMENT_END, column);
	// Выполняем сброс признака открытия документа
	this->_opened = false;
	// Выполняем сброс признака наполнения документа
	this->_filled = false;
	/**
	 * Выполняем сброс всего, объявленного закрытым документом
	 *
	 * @details Метки, сокращения меток типов и наречие объявляются документом и живут
	 *          ровно столько, сколько живёт он сам: ссылка из второго документа на метку
	 *          первого описанием запрещена, и знать о ней читающему уже незачем
	 */
	this->_anchors.clear();
	// Выполняем сброс сокращений меток типов, директивами объявленных
	this->_handles.clear();
	// Выполняем сброс признака объявленного директивой наречия
	this->_versioned = false;
	// Выполняем возврат схемы разрешения видов к назначенной настройками
	this->_schema = this->_settings.schema;
	// Выполняем сброс имени метки, узла своего так и не дождавшейся
	this->_anchor.clear();
	// Выполняем сброс метки типа, узла своего так и не дождавшейся
	this->_tag.clear();
	// Выводим признак успешного закрытия документа
	return true;
}
/**
 * @brief Метод снятия ограды со скалярного значения
 *
 * @param text   разбираемая запись значения вместе с оградою
 * @param style  вид ограды разбираемого значения
 * @param column положение значения в разбираемой строке
 * @param result строка, куда помещается содержимое значения
 * @return       признак успешного снятия ограды
 *
 */
bool awh::codec::yaml::Reader::unquote(const string_view text, const style_t style, const size_t column, string & result) noexcept {
	// Выполняем сброс собираемого содержимого значения
	result.clear();
	/**
	 * Если значение записано без ограды
	 */
	if(style == style_t::PLAIN){
		// Выполняем перенос записи значения как она есть
		result.assign(text);
		// Выводим признак успешного снятия ограды
		return true;
	}
	// Получаем содержимое значения без ограды
	const string_view inner = text.substr(1, (text.size() - 2));
	/**
	 * Если значение обнесено одинарной оградой
	 *
	 * @note Отменяющих последовательностей одинарная ограда не знает вовсе: единственная
	 *       её условность - удвоение кавычки, каким кавычка и записывается
	 */
	if(style == style_t::SINGLE){
		// Смещение разбираемого знака записи
		size_t offset = 0;
		/**
		 * Выполняем перебор всех знаков содержимого значения
		 */
		while(offset < inner.size()){
			// Получаем очередной знак содержимого значения
			const char letter = inner[offset];
			/**
			 * Если знак является удвоенной кавычкой
			 */
			if((letter == '\'') && ((offset + 1) < inner.size()) && (inner[offset + 1] == '\'')){
				// Выполняем запись одной кавычки в содержимое значения
				result.push_back('\'');
				// Выполняем переход за удвоенную кавычку
				offset += 2;
				// Выполняем переход к следующему знаку записи
				continue;
			}
			// Выполняем перенос знака в содержимое значения
			result.push_back(letter);
			// Выполняем переход к следующему знаку записи
			offset++;
		}
		// Выводим признак успешного снятия ограды
		return true;
	}
	// Смещение разбираемого знака записи
	size_t offset = 0;
	/**
	 * Выполняем перебор всех знаков содержимого значения
	 */
	while(offset < inner.size()){
		// Получаем очередной знак содержимого значения
		const char letter = inner[offset];
		/**
		 * Если знак отменяющей последовательностью не открывается
		 */
		if(letter != '\\'){
			// Выполняем перенос знака в содержимое значения
			result.push_back(letter);
			// Выполняем переход к следующему знаку записи
			offset++;
			// Выполняем переход к следующему знаку записи
			continue;
		}
		/**
		 * Если отменяющая последовательность оборвана концом значения
		 */
		if((offset + 1) >= inner.size())
			// Выводим отказ ошибочного построения отменяющей последовательности
			return this->fail(error_t::INVALID_ESCAPE, (column + offset));
		// Получаем знак, отменяющей последовательностью заданный
		const char escaped = inner[offset + 1];
		// Выполняем переход за знак отмены и заданный им знак
		offset += 2;
		/**
		 * Определяем знак, отменяющей последовательностью заданный
		 */
		switch(escaped){
			// Если последовательность задаёт нулевой знак
			case '0': result.push_back('\0'); break;
			// Если последовательность задаёт звонок
			case 'a': result.push_back('\a'); break;
			// Если последовательность задаёт возврат на знак
			case 'b': result.push_back('\b'); break;
			// Если последовательность задаёт горизонтальную подачу
			case 't': result.push_back('\t'); break;
			// Если последовательность задаёт горизонтальную подачу знаком её
			case '\t': result.push_back('\t'); break;
			// Если последовательность задаёт перевод строки
			case 'n': result.push_back('\n'); break;
			// Если последовательность задаёт вертикальную подачу
			case 'v': result.push_back('\v'); break;
			// Если последовательность задаёт подачу страницы
			case 'f': result.push_back('\f'); break;
			// Если последовательность задаёт возврат каретки
			case 'r': result.push_back('\r'); break;
			// Если последовательность задаёт знак ухода
			case 'e': result.push_back('\x1b'); break;
			// Если последовательность задаёт пробел
			case ' ': result.push_back(' '); break;
			// Если последовательность задаёт кавычку
			case '"': result.push_back('"'); break;
			// Если последовательность задаёт косую черту
			case '/': result.push_back('/'); break;
			// Если последовательность задаёт знак отмены
			case '\\': result.push_back('\\'); break;
			/**
			 * Если последовательность задаёт знак смены строки
			 */
			case 'N': {
				/**
				 * Если записать знак смены строки не удалось
				 */
				if(!encode(0x85, result))
					// Выводим отказ ошибочного построения отменяющей последовательности
					return this->fail(error_t::INVALID_ESCAPE, (column + offset));
			} break;
			/**
			 * Если последовательность задаёт неразрывный пробел
			 */
			case '_': {
				/**
				 * Если записать неразрывный пробел не удалось
				 */
				if(!encode(0xA0, result))
					// Выводим отказ ошибочного построения отменяющей последовательности
					return this->fail(error_t::INVALID_ESCAPE, (column + offset));
			} break;
			/**
			 * Если последовательность задаёт разделитель строк
			 */
			case 'L': {
				/**
				 * Если записать разделитель строк не удалось
				 */
				if(!encode(0x2028, result))
					// Выводим отказ ошибочного построения отменяющей последовательности
					return this->fail(error_t::INVALID_ESCAPE, (column + offset));
			} break;
			/**
			 * Если последовательность задаёт разделитель абзацев
			 */
			case 'P': {
				/**
				 * Если записать разделитель абзацев не удалось
				 */
				if(!encode(0x2029, result))
					// Выводим отказ ошибочного построения отменяющей последовательности
					return this->fail(error_t::INVALID_ESCAPE, (column + offset));
			} break;
			/**
			 * Если последовательность задаёт знак шестнадцатеричной записью
			 */
			case 'x':
			case 'u':
			case 'U': {
				// Получаем количество разрядов записи знака
				const size_t count = ((escaped == 'x') ? 2 : ((escaped == 'u') ? 4 : 8));
				// Разбираемый знак Юникода
				uint32_t code = 0;
				/**
				 * Если разобрать запись знака не удалось
				 */
				if(!hexadecimal(inner, offset, count, code))
					// Выводим отказ ошибочного построения записи знака Юникода
					return this->fail(error_t::INVALID_UNICODE, (column + offset));
				// Выполняем переход за разряды записи знака
				offset += count;
				/**
				 * Если записать разобранный знак не удалось
				 *
				 * @note Отказ этот означает суррогат либо знак за пределом набора Юникода:
				 *       записать их последовательностью UTF-8 нельзя вовсе
				 */
				if(!encode(code, result))
					// Выводим отказ ошибочного построения записи знака Юникода
					return this->fail(error_t::INVALID_UNICODE, (column + offset));
			} break;
			/**
			 * Если последовательность не опознана вовсе
			 */
			default: return this->fail(error_t::INVALID_ESCAPE, (column + offset));
		}
	}
	// Выводим признак успешного снятия ограды
	return true;
}
/**
 * @brief Метод поиска конца скалярного значения в разбираемой строке
 *
 * @param line   разбираемая строка
 * @param offset смещение начала значения в строке
 * @param style  вид записи значения, определяемый первым знаком
 * @param length длина записи значения вместе с оградою
 * @return       признак того, что значение прочитано целиком
 *
 */
bool awh::codec::yaml::Reader::bounds(const string_view line, const size_t offset, style_t & style, size_t & length) noexcept {
	/**
	 * Если читать значение неоткуда
	 */
	if(offset >= line.size()){
		// Запоминаем вид записи значения
		style = style_t::PLAIN;
		// Запоминаем длину записи значения
		length = 0;
		// Выводим признак того, что значение прочитано целиком
		return true;
	}
	// Получаем первый знак записи значения
	const char leading = line[offset];
	/**
	 * Если значение обнесено оградою
	 */
	if((leading == '\'') || (leading == '"')){
		// Запоминаем вид записи значения
		style = ((leading == '\'') ? style_t::SINGLE : style_t::DOUBLE);
		// Смещение разбираемого знака записи
		size_t position = (offset + 1);
		/**
		 * Выполняем перебор всех знаков записи значения
		 */
		while(position < line.size()){
			// Получаем очередной знак записи значения
			const char letter = line[position];
			/**
			 * Если ограда двойная, а знак открывает отменяющую последовательность
			 */
			if((style == style_t::DOUBLE) && (letter == '\\')){
				// Выполняем переход за отменяющую последовательность
				position += 2;
				// Выполняем переход к следующему знаку записи
				continue;
			}
			/**
			 * Если знак закрывает ограду значения
			 */
			if(letter == leading){
				/**
				 * Если ограда одинарная, а кавычка удвоена
				 */
				if((style == style_t::SINGLE) && ((position + 1) < line.size()) && (line[position + 1] == '\'')){
					// Выполняем переход за удвоенную кавычку
					position += 2;
					// Выполняем переход к следующему знаку записи
					continue;
				}
				// Запоминаем длину записи значения вместе с оградою
				length = ((position + 1) - offset);
				// Выводим признак того, что значение прочитано целиком
				return true;
			}
			// Выполняем переход к следующему знаку записи
			position++;
		}
		// Выводим признак того, что ограда значения не закрыта
		return false;
	}
	// Запоминаем вид записи значения
	style = style_t::PLAIN;
	// Смещение разбираемого знака записи
	size_t position = offset;
	/**
	 * Выполняем перебор всех знаков записи значения
	 */
	while(position < line.size()){
		// Получаем очередной знак записи значения
		const char letter = line[position];
		/**
		 * Если знак открывает примечание, стоя за пробельным знаком
		 */
		if((letter == '#') && (position > offset) && spacing(line[position - 1]))
			// Выходим из перебора знаков записи значения
			break;
		/**
		 * Если знак разделяет имя пары и значение её
		 *
		 * @note Разделяет он лишь тогда, когда за ним стоит пробельный знак либо конец
		 *       строки: `a:b` есть значение целиком, а `a: b` есть пара
		 */
		if((letter == ':') && (((position + 1) >= line.size()) || spacing(line[position + 1])))
			// Выходим из перебора знаков записи значения
			break;
		// Выполняем переход к следующему знаку записи
		position++;
	}
	// Запоминаем длину записи значения без пробельной обвязки
	length = trimmed(line.substr(offset, (position - offset))).size();
	// Выводим признак того, что значение прочитано целиком
	return true;
}
/**
 * @brief Метод переноса накопленных свойств узла в собранное событие
 *
 * @param item событие, свойства узла принимающее
 *
 */
void awh::codec::yaml::Reader::attach(item_t & item) noexcept {
	/**
	 * Если событию предпослана метка
	 */
	if(!this->_anchor.empty()){
		// Запоминаем смещение имени метки в хранилище знаков
		item.anchor.offset = this->_storage.size();
		// Запоминаем длину имени метки
		item.anchor.length = this->_anchor.size();
		// Выполняем перенос имени метки в хранилище знаков
		this->_storage.append(this->_anchor);
		// Выполняем сброс имени метки, узла своего дождавшейся
		this->_anchor.clear();
	}
	/**
	 * Если событию предпослана метка типа
	 */
	if(!this->_tag.empty()){
		// Запоминаем смещение метки типа в хранилище знаков
		item.tag.offset = this->_storage.size();
		// Запоминаем длину метки типа
		item.tag.length = this->_tag.size();
		// Выполняем перенос метки типа в хранилище знаков
		this->_storage.append(this->_tag);
		// Выполняем сброс метки типа, узла своего дождавшейся
		this->_tag.clear();
	}
}
/**
 * @brief Метод разрешения вида скалярного значения
 *
 * @param text   содержимое скалярного значения
 * @param style  вид записи значения в исходном тексте
 * @param column положение значения в разбираемой строке
 * @param type   разрешённый вид скалярного значения
 * @return       признак успешного разрешения вида
 *
 */
bool awh::codec::yaml::Reader::typing(const string_view text, const style_t style, const size_t column, type_t & type) noexcept {
	/**
	 * Если метка типа значению не предпослана
	 */
	if(this->_tag.empty()){
		/**
		 * Запоминаем вид значения, разрешённый действующей схемой
		 *
		 * @note Значение, обнесённое оградою, схеме не подлежит вовсе - оно строка всегда:
		 *       ограда для того и ставится, чтобы `12` осталось строкой
		 */
		type = ((style == style_t::PLAIN) ? resolve(text, this->_schema) : type_t::STRING);
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Если метка типа началом своим описанию не отвечает
	 *
	 * @note Метка своя - местная либо чужого объявления - схеме не подлежит: разрешать её
	 *       читающему нечем, и содержимое остаётся строкою, каким записано
	 */
	if(this->_tag.compare(0, STANDARD_TAG.size(), STANDARD_TAG) != 0){
		// Запоминаем вид значения, меткою своей не разрешаемый
		type = type_t::STRING;
		// Выводим признак успешного разрешения вида
		return true;
	}
	// Получаем окончание метки типа, вид значения задающее
	const string_view suffix = string_view(this->_tag).substr(STANDARD_TAG.size());
	/**
	 * Если метка типа задаёт строку
	 */
	if(suffix == "str"){
		// Запоминаем вид значения, меткою типа заданный
		type = type_t::STRING;
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Если метка типа задаёт двоичное содержимое
	 *
	 * @note Запись base64 здесь не сверяется: сверяет её извлечение содержимого, ибо там
	 *       она и разбирается, а разбирать её дважды - работа впустую
	 */
	if(suffix == "binary"){
		// Запоминаем вид значения, меткою типа заданный
		type = type_t::BINARY;
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Если метка типа задаёт отметку времени
	 */
	if(suffix == "timestamp"){
		// Запоминаем вид значения, меткою типа заданный
		type = type_t::STAMP;
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Если метка типа задаёт построение, скалярным значением не являющееся
	 */
	if((suffix == "seq") || (suffix == "map"))
		// Выводим отказ несоответствия содержимого метке типа
		return this->fail(error_t::TAG_MISMATCH, column);
	/**
	 * Схема, которою сверяется содержимое с меткою типа
	 *
	 * @note Схема защитная объявляет строкою всё подряд, и сверять ею содержимое с меткою
	 *       значило бы отвергнуть `!!int 12`. Метка сказана прямо, и разрешение под неё
	 *       идёт схемою ядровой - либо схемою наречия 1.1, коли текст её объявил
	 */
	const schema_t schema = ((this->_schema == schema_t::LEGACY) ? schema_t::LEGACY : schema_t::CORE);
	// Получаем вид значения, разрешённый схемою по записи его
	const type_t resolved = resolve(text, schema);
	/**
	 * Если метка типа задаёт пустое значение
	 */
	if(suffix == "null"){
		/**
		 * Если содержимое пустым значением не является
		 */
		if((resolved != type_t::NUL) && !text.empty())
			// Выводим отказ несоответствия содержимого метке типа
			return this->fail(error_t::TAG_MISMATCH, column);
		// Запоминаем вид значения, меткою типа заданный
		type = type_t::NUL;
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Если метка типа задаёт логическое значение
	 */
	if(suffix == "bool"){
		/**
		 * Если содержимое логическим значением не является
		 */
		if(resolved != type_t::BOOL)
			// Выводим отказ несоответствия содержимого метке типа
			return this->fail(error_t::TAG_MISMATCH, column);
		// Запоминаем вид значения, меткою типа заданный
		type = type_t::BOOL;
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Если метка типа задаёт целое число
	 */
	if(suffix == "int"){
		/**
		 * Если содержимое числом не является вовсе
		 *
		 * @details Сверяется здесь именно числовая природа записи, а не целость её: разбор
		 *          записи числа - работа извлечения, и он там и делается. Требовать целости
		 *          здесь значило бы завести второй свод правил чтения чисел рядом с первым,
		 *          а два свода расходятся всегда. Запись `.inf`, разрешённая дробным видом
		 *          прямо, сюда всё же не пройдёт: целым она не бывает никогда
		 */
		if(!(static_cast <uint32_t> (resolved) & static_cast <uint32_t> (type_t::INT)))
			// Выводим отказ несоответствия содержимого метке типа
			return this->fail(error_t::TAG_MISMATCH, column);
		// Запоминаем вид значения, меткою типа заданный
		type = type_t::INT;
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Если метка типа задаёт дробное число
	 */
	if(suffix == "float"){
		/**
		 * Если содержимое числом не является вовсе
		 */
		if(!(static_cast <uint32_t> (resolved) & static_cast <uint32_t> (type_t::NUMBER)))
			// Выводим отказ несоответствия содержимого метке типа
			return this->fail(error_t::TAG_MISMATCH, column);
		/**
		 * Запоминаем вид значения, меткою типа заданный
		 *
		 * @note Целое, меткою `!!float` помеченное, дробным и становится: метка сказана
		 *       прямо, и `!!float 1` есть единица дробная, а не целая
		 */
		type = type_t::REAL;
		// Выводим признак успешного разрешения вида
		return true;
	}
	/**
	 * Запоминаем вид значения, меткою описания не разрешаемый
	 *
	 * @note Метка начала описанного, но окончания неведомого, разрешению не подлежит:
	 *       наречия последующие вправе завести свои, и отвергать их нельзя
	 */
	type = type_t::STRING;
	// Выводим признак успешного разрешения вида
	return true;
}
/**
 * @brief Метод разбора свойств узла, стоящих прежде него
 *
 * @param line   разбираемая строка
 * @param offset смещение начала свойств в строке, по выходе - смещение за ними
 * @return       признак успешного разбора свойств узла
 *
 */
bool awh::codec::yaml::Reader::property(const string_view line, size_t & offset) noexcept {
	/**
	 * Выполняем разбор всех свойств, узлу предпосланных
	 */
	while(offset < line.size()){
		// Получаем первый знак разбираемого свойства
		const char leading = line[offset];
		/**
		 * Если свойство объявляет метку узла
		 */
		if(leading == '&'){
			/**
			 * Если метка узлу уже предпослана
			 */
			if(!this->_anchor.empty())
				// Выводим отказ недопустимого знака в этом месте текста
				return this->fail(error_t::INVALID_CHARACTER, offset);
			// Получаем смещение начала имени метки
			const size_t begin = (offset + 1);
			// Смещение разбираемого знака строки
			size_t position = begin;
			/**
			 * Выполняем чтение имени метки до первого знака, ему не принадлежащего
			 */
			while((position < line.size()) && naming(line[position]))
				// Выполняем переход к следующему знаку строки
				position++;
			/**
			 * Если имя метки пусто
			 */
			if(position == begin)
				// Выводим отказ недопустимого знака в этом месте текста
				return this->fail(error_t::INVALID_CHARACTER, offset);
			/**
			 * Если длина имени метки предел превышает
			 */
			if((position - begin) > MAX_ANCHOR)
				// Выводим отказ превышения длины имени метки
				return this->fail(error_t::ANCHOR_TOO_LONG, offset);
			// Запоминаем имя метки, узла своего ожидающей
			this->_anchor.assign(line.substr(begin, (position - begin)));
			/**
			 * Запоминаем метку среди объявленных документом
			 *
			 * @note Повторное объявление имени описанием дозволено и отказом не является:
			 *       ссылки последующие указывают на объявление последнее, а прежнее
			 *       остаётся у ссылок, прежде него стоявших
			 */
			this->_anchors.emplace(this->_anchor);
			// Выполняем переход за разобранное свойство
			offset = position;
		/**
		 * Если свойство объявляет метку типа
		 */
		} else if(leading == '!') {
			/**
			 * Если метка типа узлу уже предпослана
			 */
			if(!this->_tag.empty())
				// Выводим отказ недопустимого знака в этом месте текста
				return this->fail(error_t::INVALID_CHARACTER, offset);
			// Смещение разбираемого знака строки
			size_t position = (offset + 1);
			/**
			 * Если метка типа записана указателем дословно
			 */
			if((position < line.size()) && (line[position] == '<')){
				// Разыскиваем знак окончания дословной записи
				const size_t closing = line.find('>', position);
				/**
				 * Если дословная запись не закрыта либо пуста
				 */
				if((closing == string_view::npos) || (closing == (position + 1)))
					// Выводим отказ ошибочного построения метки типа
					return this->fail(error_t::INVALID_TAG, offset);
				// Запоминаем метку типа, узла своего ожидающую
				this->_tag.assign(line.substr((position + 1), (closing - position - 1)));
				// Выполняем переход за разобранное свойство
				offset = (closing + 1);
			/**
			 * Если метка типа записана сокращением
			 */
			} else {
				// Получаем смещение начала окончания метки типа
				size_t begin = position;
				/**
				 * Выполняем чтение записи метки до первого знака, ей не принадлежащего
				 */
				while((position < line.size()) && tagging(line[position]))
					// Выполняем переход к следующему знаку строки
					position++;
				// Сокращение метки типа, начало её задающее
				string handle("!");
				/**
				 * Если прочитанное есть сокращение метки типа, а не окончание её
				 *
				 * @note Отличить одно от другого можно лишь знаком, следом стоящим:
				 *       восклицательный знак за прочитанным делает его сокращением
				 */
				if((position < line.size()) && (line[position] == '!')){
					// Запоминаем сокращение метки типа вместе с обоими знаками его
					handle.assign(line.substr(offset, (position - offset + 1)));
					// Выполняем переход за сокращение метки типа
					position++;
					// Запоминаем смещение начала окончания метки типа
					begin = position;
					/**
					 * Выполняем чтение окончания метки типа
					 */
					while((position < line.size()) && tagging(line[position]))
						// Выполняем переход к следующему знаку строки
						position++;
				}
				// Начало метки типа, сокращением её задаваемое
				string_view prefix(handle);
				/**
				 * Если сокращение объявлено директивою документа
				 */
				const auto i = this->_handles.find(handle);
				/**
				 * Если сокращение объявлено директивою документа
				 */
				if(i != this->_handles.end())
					// Запоминаем начало метки типа, директивою объявленное
					prefix = i->second;
				/**
				 * Если сокращение есть сокращение описания
				 */
				else if(handle.compare("!!") == 0)
					// Запоминаем начало метки типа, описанием заданное
					prefix = STANDARD_TAG;
				/**
				 * Если сокращение объявлено не было вовсе
				 *
				 * @note Сокращение `!` объявления не требует: оно есть начало меток местных,
				 *       и объявлено оно всегда. Прочие же объявляются директивой `%TAG`, и
				 *       без объявления читающий не знает, во что их раскрывать
				 */
				else if(handle.compare("!") != 0)
					// Выводим отказ необъявленного сокращения метки типа
					return this->fail(error_t::UNKNOWN_TAG_HANDLE, offset);
				/**
				 * Если запись метки оборвалась знаком, ей не принадлежащим
				 *
				 * @note Знаки записи метки типа описанием заданы перечнем, и знаков вне
				 *       перечня того - хотя бы и буквы иной письменности - метка нести не
				 *       вправе: записываются они долею с точкою кода, а не собою
				 */
				if((position == begin) && (position < line.size()) && !spacing(line[position]))
					// Выводим отказ ошибочного построения метки типа
					return this->fail(error_t::INVALID_TAG, offset);
				// Собираем метку типа из начала её и окончания
				this->_tag.assign(prefix);
				// Выполняем присоединение окончания метки типа
				this->_tag.append(line.substr(begin, (position - begin)));
				// Выполняем переход за разобранное свойство
				offset = position;
			}
		// Если знак свойством узла не является
		} else break;
		/**
		 * Если за разобранным свойством содержимое строки исчерпано
		 */
		if(offset >= line.size())
			// Выводим признак успешного разбора свойств узла
			return true;
		/**
		 * Если за разобранным свойством стоит знак, пробельным не являющийся
		 *
		 * @note Свойства отделяются пробельными знаками и от узла своего, и друг от друга:
		 *       запись `&метка!тип` описанию противна, и разбирать её наугад нельзя
		 */
		if(!spacing(line[offset]))
			// Выводим отказ недопустимого знака в этом месте текста
			return this->fail(error_t::INVALID_CHARACTER, offset);
		/**
		 * Выполняем пропуск пробельных знаков за разобранным свойством
		 */
		while((offset < line.size()) && spacing(line[offset]))
			// Выполняем переход к следующему знаку строки
			offset++;
	}
	// Выводим признак успешного разбора свойств узла
	return true;
}
/**
 * @brief Метод разбора ссылки на объявленную метку
 *
 * @param line   разбираемая строка
 * @param offset смещение начала ссылки в строке, по выходе - смещение за нею
 * @return       признак успешного разбора ссылки
 *
 */
bool awh::codec::yaml::Reader::referred(const string_view line, size_t & offset) noexcept {
	/**
	 * Если ссылке предпослано свойство узла
	 *
	 * @note Ссылка узлом не является - она указывает на узел, прежде объявленный, - и
	 *       своих свойств иметь не вправе: свойства эти были бы отнесены к узлу чужому
	 */
	if(!this->_anchor.empty() || !this->_tag.empty())
		// Выводим отказ недопустимого знака в этом месте текста
		return this->fail(error_t::INVALID_CHARACTER, offset);
	// Получаем смещение начала имени метки, ссылкою указанной
	const size_t begin = (offset + 1);
	// Смещение разбираемого знака строки
	size_t position = begin;
	/**
	 * Выполняем чтение имени метки до первого знака, ему не принадлежащего
	 */
	while((position < line.size()) && naming(line[position]))
		// Выполняем переход к следующему знаку строки
		position++;
	/**
	 * Если имя метки пусто
	 */
	if(position == begin)
		// Выводим отказ недопустимого знака в этом месте текста
		return this->fail(error_t::INVALID_CHARACTER, offset);
	// Получаем имя метки, ссылкою указанной
	const string_view name = line.substr(begin, (position - begin));
	/**
	 * Если метка с таким именем документом не объявлена
	 *
	 * @note Описание велит метке стоять прежде ссылки на неё, и ссылка вперёд объявления
	 *       есть отказ: раскрыть её потребитель не сможет ничем
	 */
	if(this->_anchors.find(string(name)) == this->_anchors.end())
		// Выводим отказ ссылки на необъявленную метку
		return this->fail(error_t::UNKNOWN_ALIAS, offset);
	// Выполняем постановку события ссылки на объявленную метку
	item_t & item = this->emit(event_t::ALIAS, offset);
	// Запоминаем смещение содержимого события в хранилище знаков
	item.offset = this->_storage.size();
	// Запоминаем длину содержимого события
	item.length = name.size();
	// Выполняем перенос содержимого события в хранилище знаков
	this->_storage.append(name);
	// Запоминаем признак наполнения открытого документа
	this->_filled = true;
	// Выполняем переход за разобранную ссылку
	offset = position;
	// Выводим признак успешного разбора ссылки
	return true;
}
/**
 * @brief Метод разбора директивы, документу предпосланной
 *
 * @param line   разбираемая строка
 * @param offset смещение начала директивы в строке
 * @return       признак успешного разбора директивы
 *
 */
bool awh::codec::yaml::Reader::directive(const string_view line, const size_t offset) noexcept {
	/**
	 * Если документ уже открыт
	 *
	 * @note Директива правит толкование документа целиком, и стоять посреди него она не
	 *       вправе: половина документа оказалась бы прочитана одними правилами, половина иными
	 */
	if(this->_opened)
		// Выводим отказ ошибочного построения директивы
		return this->fail(error_t::INVALID_DIRECTIVE, offset);
	// Получаем смещение начала имени директивы
	const size_t begin = (offset + 1);
	// Смещение разбираемого знака строки
	size_t position = begin;
	/**
	 * Выполняем чтение имени директивы до первого пробельного знака
	 */
	while((position < line.size()) && !spacing(line[position]))
		// Выполняем переход к следующему знаку строки
		position++;
	// Получаем имя разбираемой директивы
	const string_view name = line.substr(begin, (position - begin));
	/**
	 * Если имя директивы пусто
	 */
	if(name.empty())
		// Выводим отказ ошибочного построения директивы
		return this->fail(error_t::INVALID_DIRECTIVE, offset);
	// Получаем остаток строки за именем директивы
	string_view rest = line.substr(position);
	/**
	 * Разыскиваем знак примечания в остатке строки
	 */
	for(size_t i = 0; i < rest.size(); i++){
		/**
		 * Если знак примечания стоит за пробельным знаком
		 */
		if((rest[i] == '#') && ((i == 0) || spacing(rest[i - 1]))){
			// Выполняем снятие примечания с остатка строки
			rest = rest.substr(0, i);
			// Выходим из поиска знака примечания
			break;
		}
	}
	// Выполняем снятие пробельной обвязки с остатка строки
	rest = stripped(rest);
	// Запоминаем признак предпосланных документу директив
	this->_directed = true;
	/**
	 * Если директива объявляет наречие текста
	 */
	if(name.compare("YAML") == 0){
		/**
		 * Если наречие объявлено директивою уже
		 */
		if(this->_versioned)
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		// Разыскиваем разделитель старшего и младшего чисел наречия
		const size_t separator = rest.find('.');
		/**
		 * Если разделителя чисел наречия в записи нет вовсе
		 */
		if(separator == string_view::npos)
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		// Получаем старшее число объявленного наречия
		const string_view major = rest.substr(0, separator);
		// Получаем младшее число объявленного наречия
		const string_view minor = rest.substr(separator + 1);
		/**
		 * Если хоть одно из чисел наречия числом не является
		 */
		if(major.empty() || minor.empty() ||
		   (major.find_first_not_of("0123456789") != string_view::npos) ||
		   (minor.find_first_not_of("0123456789") != string_view::npos))
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		/**
		 * Если объявленное наречие чтением не поддерживается
		 *
		 * @note Поддерживаются наречия 1.1 и 1.2, и они же покрывают всё, что встречается
		 *       в живых текстах: наречия 1.0 не писал почти никто, а наречий старше 1.2 нет
		 */
		if((major.compare("1") != 0) || ((minor.compare("1") != 0) && (minor.compare("2") != 0)))
			// Выводим отказ неподдерживаемого наречия текста
			return this->fail(error_t::UNSUPPORTED_VERSION, offset);
		// Запоминаем признак объявленного директивой наречия
		this->_versioned = true;
		/**
		 * Если текст объявлен наречием 1.1, а схема разрешения потребителем не назначена
		 *
		 * @details Наречие 1.1 разрешает виды иначе: `yes` там есть истина, `0777` есть
		 *          восьмеричное число, а `12:30` есть число шестидесятиричное. Объявив
		 *          наречие, текст сказал об этом прямо, и читать его схемою ядровой значило
		 *          бы прочесть не то, что писано. Схему же, потребителем назначенную,
		 *          текст перебивать не вправе: назначил её потребитель осознанно
		 */
		if((minor.compare("1") == 0) && (this->_settings.schema == schema_t::CORE))
			// Запоминаем схему наречия 1.1 действующей над документом
			this->_schema = schema_t::LEGACY;
		// Выводим признак успешного разбора директивы
		return true;
	}
	/**
	 * Если директива объявляет сокращение метки типа
	 */
	if(name.compare("TAG") == 0){
		// Разыскиваем разделитель сокращения и начала метки типа
		const size_t separator = rest.find(' ');
		/**
		 * Если разделителя сокращения и начала метки в записи нет вовсе
		 */
		if(separator == string_view::npos)
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		// Получаем объявляемое сокращение метки типа
		const string_view handle = rest.substr(0, separator);
		// Получаем начало метки типа, сокращением задаваемое
		const string_view prefix = stripped(rest.substr(separator + 1));
		/**
		 * Если сокращение построено ошибочно
		 *
		 * @note Сокращение обносится восклицательными знаками с обеих сторон, и знак один
		 *       есть сокращение меток местных - оно тоже дозволено
		 */
		if((handle.size() < 1) || (handle.front() != '!') || (handle.back() != '!'))
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		/**
		 * Если начало метки типа пусто
		 */
		if(prefix.empty())
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		/**
		 * Если сокращение объявлено директивою уже
		 *
		 * @note Второе объявление того же сокращения описанием запрещено прямо: неведомо,
		 *       которое из двух начал имел в виду писавший
		 */
		if(!this->_handles.emplace(string(handle), string(prefix)).second)
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		// Выводим признак успешного разбора директивы
		return true;
	}
	// Выводим признак успешного разбора директивы
	return true;
}
/**
 * @brief Метод разбора содержимого строки за отступом
 *
 * @param line   разбираемая строка без знака конца строки
 * @param offset смещение начала содержимого в строке
 * @param indent отступ, на котором содержимое стоит
 * @return       признак успешного разбора содержимого
 *
 */
bool awh::codec::yaml::Reader::content(const string_view line, const size_t offset, const uint32_t indent) noexcept {
	/**
	 * Если содержимое строки исчерпано
	 */
	if(offset >= line.size())
		// Выводим признак успешного разбора содержимого
		return true;
	// Получаем первый знак содержимого строки
	const char leading = line[offset];
	/**
	 * Если содержимое открывается вопросом составного имени
	 *
	 * @details Составные имена заводятся вместе с держащим документ целиком: имя, само
	 *          построением являющееся, нуждается в дереве, а потоковому чтению деть его
	 *          некуда. Отвечать на него молчаливым разбором наугад нельзя: разбор выдал бы
	 *          дерево, исходному тексту не отвечающее, и потребитель узнал бы о том не сразу
	 */
	if(leading == '?')
		// Выводим отказ недопустимого знака в этом месте текста
		return this->fail(error_t::INVALID_CHARACTER, offset);
	/**
	 * Если содержимое открывается ссылкой на объявленную метку
	 */
	if(leading == '*'){
		/**
		 * Если ожидалось значение пары, объявленной прежде
		 */
		if(this->_expected)
			// Выполняем сброс признака ожидания значения пары
			this->_expected = false;
		// Смещение разбираемого знака строки
		size_t position = offset;
		/**
		 * Если разобрать ссылку не удалось
		 */
		if(!this->referred(line, position))
			// Выводим признак неудачного разбора содержимого
			return false;
		/**
		 * Выполняем пропуск пробельных знаков за ссылкой
		 */
		while((position < line.size()) && spacing(line[position]))
			// Выполняем переход к следующему знаку строки
			position++;
		/**
		 * Если за ссылкой стоит примечание
		 */
		if((position < line.size()) && (line[position] == '#'))
			// Выполняем постановку события примечания
			this->remark(line, position);
		/**
		 * Если за ссылкой стоит содержимое, примечанием не являющееся
		 *
		 * @note Ссылка именем пары описанием дозволена, но нуждается она в дереве: имя
		 *       пары пришлось бы раскрыть прежде постановки её, а раскрывать потоковому
		 *       чтению нечем. Оттого разделитель пары за ссылкой отвергается
		 */
		else if(position < line.size())
			// Выводим отказ содержимого за завершённой записью
			return this->fail(error_t::TRAILING_CHARACTERS, position);
		// Выводим признак успешного разбора содержимого
		return true;
	}
	/**
	 * Если содержимое открывается свойствами узла
	 */
	if((leading == '&') || (leading == '!')){
		// Смещение разбираемого знака строки
		size_t position = offset;
		/**
		 * Если разобрать свойства узла не удалось
		 */
		if(!this->property(line, position))
			// Выводим признак неудачного разбора содержимого
			return false;
		/**
		 * Если за свойствами узла содержимое строки исчерпано
		 *
		 * @note Свойства вправе стоять и в строке, узлу своему предшествующей: узел ждёт их
		 *       строкою ниже, и накопленное дожидается его
		 */
		if(position >= line.size())
			// Выводим признак успешного разбора содержимого
			return true;
		/**
		 * Если за свойствами узла стоит одно примечание
		 */
		if(line[position] == '#'){
			// Выполняем постановку события примечания
			this->remark(line, position);
			// Выводим признак успешного разбора содержимого
			return true;
		}
		// Выполняем разбор содержимого за свойствами узла
		return this->content(line, position, indent);
	}
	/**
	 * Если содержимое открывается заголовком блочного значения
	 */
	if((leading == '|') || (leading == '>'))
		// Выполняем разбор заголовка блочного значения
		return this->opening(line, offset, this->_margin);
	/**
	 * Если содержимое открывается поточным построением
	 */
	if((leading == '[') || (leading == '{')){
		/**
		 * Если ожидалось значение пары, объявленной прежде
		 */
		if(this->_expected)
			// Выполняем сброс признака ожидания значения пары
			this->_expected = false;
		// Получаем предел глубины вложенности
		const size_t limit = ((this->_settings.depth > 0) ? this->_settings.depth : MAX_DEPTH);
		/**
		 * Если глубина вложенности предел превышает
		 */
		if(this->_levels.size() >= limit)
			// Выводим отказ превышения глубины вложенности
			return this->fail(error_t::DEPTH_EXCEEDED, offset);
		/**
		 * Если метка типа, построению предпосланная, виду его противна
		 */
		if(!this->matched(((leading == '[') ? nesting_t::SEQUENCE : nesting_t::MAPPING), offset))
			// Выводим признак неудачного разбора содержимого
			return false;
		// Выполняем постановку события открытия поточного построения
		item_t & item = this->emit(((leading == '[') ? event_t::SEQUENCE_START : event_t::MAPPING_START), offset);
		// Выполняем перенос накопленных свойств узла в собранное событие
		this->attach(item);
		// Запоминаем признак наполнения открытого документа
		this->_filled = true;
		// Выполняем открытие поточного построения
		this->_flow.emplace_back((leading == '[') ? nesting_t::SEQUENCE : nesting_t::MAPPING);
		// Запоминаем ожидание очередного значения построения
		this->_phase = flow_t::ENTRY;
		// Смещение разбираемого знака строки
		size_t position = (offset + 1);
		/**
		 * Если разобрать поточное построение не удалось
		 */
		if(!this->flowing(line, position))
			// Выводим признак неудачного разбора содержимого
			return false;
		/**
		 * Если построение скобками ещё не закрыто
		 *
		 * @note Построение продолжается следующею строкой, и остатка строки этой нет вовсе:
		 *       разбор её дошёл до конца
		 */
		if(!this->_flow.empty())
			// Выводим признак успешного разбора содержимого
			return true;
		/**
		 * Выполняем пропуск пробельных знаков за поточным построением
		 */
		while((position < line.size()) && spacing(line[position]))
			// Выполняем переход к следующему знаку строки
			position++;
		/**
		 * Если за построением стоит примечание
		 */
		if((position < line.size()) && (line[position] == '#'))
			// Выполняем постановку события примечания
			this->remark(line, position);
		/**
		 * Если за построением стоит содержимое, примечанием не являющееся
		 */
		else if(position < line.size())
			// Выводим отказ содержимого за завершённой записью
			return this->fail(error_t::TRAILING_CHARACTERS, position);
		// Выводим признак успешного разбора содержимого
		return true;
	}
	/**
	 * Если содержимое объявляет очередное значение перечня
	 */
	if((leading == '-') && (((offset + 1) >= line.size()) || spacing(line[offset + 1]))){
		/**
		 * Признак того, что перечень открывается значением пары на отступе имени её
		 *
		 * @note Построение это описанием дозволено: пара `ключ:` со значениями перечня
		 *       отступа не требует вовсе. Закрытием по отступу такой уровень не снимается,
		 *       и снимает его следующая пара того же отображения
		 */
		const bool implied = (this->_expected && (indent <= this->_pending));
		/**
		 * Если ожидалось значение пары, объявленной прежде
		 */
		if(implied)
			// Выполняем сброс признака ожидания значения пары
			this->_expected = false;
		/**
		 * Если уровень перечня на этом отступе ещё не открыт
		 */
		if(this->_levels.empty() || (this->_levels.back().indent < indent) ||
		   (this->_levels.back().kind != nesting_t::SEQUENCE)){
			/**
			 * Если на этом отступе открыт уровень отображения, а перечень значением его не является
			 */
			if(!implied && !this->_levels.empty() && (this->_levels.back().indent == indent) &&
			   (this->_levels.back().kind == nesting_t::MAPPING))
				// Выводим отказ смешения перечня и отображения на одном уровне
				return this->fail(error_t::MIXED_COLLECTION, offset);
			/**
			 * Если открыть уровень перечня не удалось
			 */
			if(!this->expand(nesting_t::SEQUENCE, indent, implied, offset))
				// Выводим признак неудачного разбора содержимого
				return false;
		}
		// Получаем смещение содержимого за объявлением значения перечня
		size_t position = (offset + 1);
		/**
		 * Выполняем пропуск пробельных знаков за объявлением значения перечня
		 */
		while((position < line.size()) && spacing(line[position]))
			// Выполняем переход к следующему знаку строки
			position++;
		/**
		 * Запоминаем признак ожидания значения записи перечня
		 *
		 * @details Признак ставится прежде разбора остатка строки, ровно как у имени пары:
		 *          остаток вправе не нести значения вовсе либо нести одни свойства узла, и
		 *          значение придёт лишь строкою ниже. Снимают признак этот сами разборы
		 *          значений, каждый у себя. Нашёл это ворошитель на перезаписи: без него
		 *          запись `- ` пустоты своей не выдавала вовсе, и запись перечня пропадала
		 */
		this->_expected = true;
		// Запоминаем признак принадлежности ожидаемого значения записи перечня
		this->_entered = true;
		// Запоминаем отступ, на котором ожидается значение записи перечня
		this->_pending = indent;
		// Выполняем разбор содержимого за объявлением значения перечня
		return this->content(line, position, static_cast <uint32_t> (position));
	}
	// Вид записи первого прочитанного значения
	style_t style = style_t::PLAIN;
	// Длина записи первого прочитанного значения
	size_t length = 0;
	/**
	 * Если ограда первого значения не закрыта
	 *
	 * @note Многострочные значения заводятся следующим этапом работ: пока ограда обязана
	 *       закрыться в той же строке, где открылась
	 */
	if(!this->bounds(line, offset, style, length))
		// Выводим отказ незакрытой ограды скалярного значения
		return this->fail(error_t::UNTERMINATED_SCALAR, offset);
	// Получаем смещение содержимого за первым значением
	size_t position = (offset + length);
	/**
	 * Выполняем пропуск пробельных знаков за первым значением
	 */
	while((position < line.size()) && spacing(line[position]))
		// Выполняем переход к следующему знаку строки
		position++;
	/**
	 * Если за первым значением стоит разделитель имени пары
	 */
	if((position < line.size()) && (line[position] == ':') &&
	   (((position + 1) >= line.size()) || spacing(line[position + 1]))){
		/**
		 * Если ожидалось значение пары, объявленной прежде
		 *
		 * @note Пара, стоящая на отступе имени прежней пары, знаменует не значение её, а
		 *       следующую пару того же отображения
		 */
		if(this->_expected && (indent <= this->_pending))
			// Выполняем сброс признака ожидания значения пары
			this->_expected = false;
		/**
		 * Если на этом отступе открыт перечень, бывший значением прежней пары
		 *
		 * @note Закрытием по отступу он не снимается - отступ у него тот же, что у имён
		 *       отображения, - и снять его обязана именно следующая пара
		 */
		while(!this->_levels.empty() && (this->_levels.back().indent == indent) &&
		      (this->_levels.back().kind == nesting_t::SEQUENCE) && this->_levels.back().implied){
			// Выполняем снятие закрываемого уровня со стопы
			this->_levels.pop_back();
			// Выполняем постановку события закрытия перечня
			this->emit(event_t::SEQUENCE_END, offset);
		}
		/**
		 * Если уровень отображения на этом отступе ещё не открыт
		 */
		if(this->_levels.empty() || (this->_levels.back().indent < indent) ||
		   (this->_levels.back().kind != nesting_t::MAPPING)){
			/**
			 * Если на этом отступе открыт уровень перечня
			 */
			if(!this->_levels.empty() && (this->_levels.back().indent == indent) &&
			   (this->_levels.back().kind != nesting_t::MAPPING))
				// Выводим отказ смешения перечня и отображения на одном уровне
				return this->fail(error_t::MIXED_COLLECTION, offset);
			/**
			 * Если открыть уровень отображения не удалось
			 */
			if(!this->expand(nesting_t::MAPPING, indent, false, offset))
				// Выводим признак неудачного разбора содержимого
				return false;
		}
		// Собираемое содержимое имени пары
		string name;
		/**
		 * Если снять ограду с имени пары не удалось
		 */
		if(!this->unquote(line.substr(offset, length), style, offset, name))
			// Выводим признак неудачного разбора содержимого
			return false;
		/**
		 * Если поставить событие имени пары не удалось
		 */
		if(!this->scalar(name, style, offset))
			// Выводим признак неудачного разбора содержимого
			return false;
		// Выполняем переход за разделитель имени пары
		position++;
		/**
		 * Выполняем пропуск пробельных знаков за разделителем имени пары
		 */
		while((position < line.size()) && spacing(line[position]))
			// Выполняем переход к следующему знаку строки
			position++;
		/**
		 * Запоминаем признак ожидания значения пары
		 *
		 * @details Признак ставится прежде разбора остатка строки, а не вместо него:
		 *          остаток вправе нести одни свойства узла - метку да метку типа, - и
		 *          значение придёт лишь строкою ниже. Снимают признак этот сами разборы
		 *          значений, каждый у себя, и остаётся он стоять ровно тогда, когда
		 *          значения в строке не оказалось. Нашёл это ворошитель: без признака
		 *          запись `имя: &метка` со значением ниже читалась смешением построений
		 */
		this->_expected = true;
		// Выполняем сброс признака принадлежности ожидаемого значения записи перечня
		this->_entered = false;
		// Запоминаем отступ, на котором ожидается значение пары
		this->_pending = indent;
		/**
		 * Если содержимое строки исчерпано либо за ним стоит одно примечание
		 */
		if((position >= line.size()) || (line[position] == '#')){
			/**
			 * Если за разделителем имени пары стоит примечание
			 */
			if(position < line.size())
				// Выполняем постановку события примечания
				this->remark(line, position);
			// Выводим признак успешного разбора содержимого
			return true;
		}
		// Выполняем разбор значения пары, стоящего в той же строке
		return this->content(line, position, static_cast <uint32_t> (position));
	}
	/**
	 * Если значение стоит на отступе, значения пары не ожидающем
	 *
	 * @note Скалярное значение вне пары и вне перечня есть содержимое документа целиком,
	 *       и второго такого в одном документе быть не может
	 */
	if(this->_expected)
		// Выполняем сброс признака ожидания значения пары
		this->_expected = false;
	/**
	 * Если значение пусто
	 */
	if(length == 0){
		/**
		 * Если за значением стоит примечание
		 */
		if((position < line.size()) && (line[position] == '#')){
			// Выполняем постановку события примечания
			this->remark(line, position);
			// Выводим признак успешного разбора содержимого
			return true;
		}
		// Выводим признак успешного разбора содержимого
		return true;
	}
	// Собираемое содержимое значения
	string value;
	/**
	 * Если снять ограду со значения не удалось
	 */
	if(!this->unquote(line.substr(offset, length), style, offset, value))
		// Выводим признак неудачного разбора содержимого
		return false;
	/**
	 * Если значение записано без ограды и добежало до конца строки
	 *
	 * @details Простое значение вправе продолжиться строкою ниже, и знать о том сейчас
	 *          нечем: узнаётся это лишь отступом строки следующей. Выдача оттого
	 *          откладывается до неё. Значение с оградою продолжения не имеет - ограда
	 *          закрылась, - а значение с примечанием за ним завершено примечанием
	 */
	if((style == style_t::PLAIN) && (position >= line.size()))
		// Выполняем откладывание выдачи простого значения
		return this->deferred(value, offset);
	/**
	 * Если поставить событие значения не удалось
	 */
	if(!this->scalar(value, style, offset))
		// Выводим признак неудачного разбора содержимого
		return false;
	/**
	 * Если за значением стоит примечание, а выдача примечаний затребована
	 */
	if((position < line.size()) && (line[position] == '#')){
		// Выполняем постановку события примечания
		this->remark(line, position);
	/**
	 * Если за значением стоит содержимое, значением не являющееся
	 */
	} else if((position < line.size()) && (line[position] != '#'))
		// Выводим отказ содержимого за завершённой записью
		return this->fail(error_t::TRAILING_CHARACTERS, position);
	// Выводим признак успешного разбора содержимого
	return true;
}
/**
 * @brief Метод разбора заголовка блочного значения
 *
 * @param line   разбираемая строка
 * @param offset смещение заголовка блочного значения в строке
 * @param indent отступ строки, заголовок несущей
 * @return       признак успешного разбора заголовка
 *
 */
bool awh::codec::yaml::Reader::opening(const string_view line, const size_t offset, const uint32_t indent) noexcept {
	/**
	 * Если ожидалось значение пары, объявленной прежде
	 *
	 * @note Блочное значение и есть то значение, какого пара ожидала: признак ожидания
	 *       снимается здесь, у себя, тем же порядком, каким снимают его прочие разборы
	 */
	if(this->_expected)
		// Выполняем сброс признака ожидания значения пары
		this->_expected = false;
	// Запоминаем вид собираемого блочного значения
	this->_block = ((line[offset] == '|') ? style_t::LITERAL : style_t::FOLDED);
	// Выполняем сброс правила усечения переводов строк
	this->_chomp = chomp_t::CLIP;
	// Выполняем сброс отступа, заголовком заданного
	this->_marked = NO_INDENT;
	// Смещение разбираемого знака заголовка
	size_t position = (offset + 1);
	// Признак того, что правило усечения уже задано
	bool chomped = false;
	/**
	 * Выполняем перебор всех знаков заголовка блочного значения
	 */
	while((position < line.size()) && !spacing(line[position]) && (line[position] != '#')){
		// Получаем очередной знак заголовка блочного значения
		const char letter = line[position];
		/**
		 * Если знак задаёт правило усечения переводов строк
		 */
		if((letter == '-') || (letter == '+')){
			/**
			 * Если правило усечения уже задано
			 */
			if(chomped)
				// Выводим отказ ошибочного построения заголовка
				return this->fail(error_t::INVALID_BLOCK_HEADER, position);
			// Запоминаем правило усечения переводов строк
			this->_chomp = ((letter == '-') ? chomp_t::STRIP : chomp_t::KEEP);
			// Запоминаем признак того, что правило усечения задано
			chomped = true;
		/**
		 * Если знак задаёт отступ содержимого блочного значения
		 *
		 * @note Указатель отступа - цифра от единицы до девяти, и отсчитывается он от
		 *       отступа строки, заголовок несущей. Нуль указателем не является: отступа
		 *       нулевой ширины у содержимого быть не может
		 */
		} else if((letter >= '1') && (letter <= '9')){
			/**
			 * Если отступ уже задан
			 */
			if(this->_marked != NO_INDENT)
				// Выводим отказ ошибочного построения заголовка
				return this->fail(error_t::INVALID_BLOCK_HEADER, position);
			// Запоминаем отступ, заголовком заданный
			this->_marked = static_cast <uint8_t> (letter - '0');
		/**
		 * Если знак заголовку не принадлежит вовсе
		 */
		} else return this->fail(error_t::INVALID_BLOCK_HEADER, position);
		// Выполняем переход к следующему знаку заголовка
		position++;
	}
	/**
	 * Выполняем пропуск пробельных знаков за заголовком
	 */
	while((position < line.size()) && spacing(line[position]))
		// Выполняем переход к следующему знаку строки
		position++;
	/**
	 * Если за заголовком стоит примечание
	 */
	if((position < line.size()) && (line[position] == '#'))
		// Выполняем постановку события примечания
		this->remark(line, position);
	/**
	 * Если за заголовком стоит содержимое, примечанием не являющееся
	 */
	else if(position < line.size())
		// Выводим отказ содержимого за завершённой записью
		return this->fail(error_t::TRAILING_CHARACTERS, position);
	// Запоминаем признак сборки блочного значения
	this->_blocking = true;
	/**
	 * Запоминаем отступ, глубже которого обязано стоять содержимое блочного значения
	 *
	 * @details Отсчёт ведётся не от начала строки, а от отступа объемлющего построения:
	 *          написание `- имя: |` кладёт отображение на отступ два, строку же открывает
	 *          черта на отступе ноль. Отсчёт от строки брал бы содержимым и следующую пару
	 *          того же отображения. Нашёл это ворошитель сличением перезаписи
	 */
	this->_outer = ((this->_levels.empty() || (this->_levels.back().indent < indent)) ?
	 indent : this->_levels.back().indent);
	// Запоминаем положение заголовка блочного значения в строке
	this->_opening = offset;
	/**
	 * Запоминаем положение заголовка блочного значения в исходном тексте
	 *
	 * @details Событие значения ставится там, где содержимое его окончилось, а стоит оно
	 *          там, где заголовок его начался: потребителю указывать надлежит на начало
	 *          записи. Поле это делится с откладыванием простого значения - собираться
	 *          вместе они не могут, ибо оба суть записи значения
	 */
	this->_origin.offset = (this->_position + offset);
	// Запоминаем номер строки, где заголовок стоит
	this->_origin.line = this->_line;
	// Запоминаем положение заголовка в строке
	this->_origin.column = static_cast <uint32_t> (offset + 1);
	// Запоминаем глубину вложенности, где заголовок стоит
	this->_origin.depth = static_cast <uint32_t> (this->_levels.size());
	/**
	 * Запоминаем отступ содержимого блочного значения
	 *
	 * @note Отступ, заголовком заданный, отсчитывается от отступа объемлющего построения -
	 *       того же самого, глубже которого содержимое обязано стоять, - а не от начала
	 *       строки; отступ же, заголовком не заданный, берётся по первой непустой строке
	 *       содержимого и здесь ещё неизвестен
	 */
	this->_inner = ((this->_marked != NO_INDENT) ? (this->_outer + this->_marked) : 0);
	// Выполняем сброс собираемого содержимого блочного значения
	this->_block_text.clear();
	// Выполняем сброс количества пустых строк, содержимого не дождавшихся
	this->_breaks = 0;
	// Выполняем сброс признака ожидания значения пары
	this->_expected = false;
	// Выводим признак успешного разбора заголовка
	return true;
}
/**
 * @brief Метод присоединения очередной строки к блочному значению
 *
 * @param line     присоединяемая строка
 * @param attached признак присоединения строки к блочному значению
 * @return         признак успешного присоединения строки
 *
 */
bool awh::codec::yaml::Reader::blocking(const string_view line, bool & attached) noexcept {
	// Выполняем сброс признака присоединения строки
	attached = false;
	// Смещение начала содержимого строки за отступом
	size_t offset = 0;
	/**
	 * Выполняем пропуск отступа присоединяемой строки
	 */
	while((offset < line.size()) && (line[offset] == ' '))
		// Выполняем переход к следующему знаку строки
		offset++;
	/**
	 * Если строка пуста
	 *
	 * @note Пустая строка блочного значения не завершает: она вправе стоять и внутри
	 *       содержимого, и перед строкой, отступом его задающей. Учитывается она счётом
	 *       и дописывается лишь тогда, когда содержимое продолжилось
	 */
	if(offset >= line.size()){
		// Выполняем учёт пустой строки блочного значения
		this->_breaks++;
		// Запоминаем признак присоединения строки
		attached = true;
		// Выводим признак успешного присоединения строки
		return true;
	}
	// Получаем отступ присоединяемой строки
	const uint32_t indent = static_cast <uint32_t> (offset);
	/**
	 * Если отступ строки блочного значения не глубже отступа заголовка
	 *
	 * @note Строка эта содержимому не принадлежит, и блочное значение ею завершается
	 */
	if(indent <= this->_outer)
		// Выводим признак успешного присоединения строки
		return true;
	/**
	 * Если отступ содержимого ещё не определён
	 */
	if(this->_inner == 0)
		// Запоминаем отступ содержимого по первой непустой строке его
		this->_inner = indent;
	/**
	 * Если отступ строки мельче отступа содержимого
	 *
	 * @note Строка мельче отступом содержимому не принадлежит, а стоя глубже отступа
	 *       заголовка, она не принадлежит и объемлющему построению: разобрать её нечем
	 */
	if(indent < this->_inner)
		return this->fail(error_t::INVALID_INDENTATION, offset);
	/**
	 * Если содержимое блочного значения уже собрано хотя бы одной строкой
	 */
	if(!this->_block_text.empty()){
		/**
		 * Если вид блочного значения велит хранить переводы строк
		 *
		 * @note Свёртка складывает строки пробелом, а пустая строка даёт перевод: оттого
		 *       пустых строк дописывается на одну меньше, а последний перевод заменяет
		 *       собою пробел свёртки
		 */
		if((this->_block == style_t::LITERAL) || (this->_breaks > 0) || (indent > this->_inner))
			// Выполняем запись перевода строки в содержимое блочного значения
			this->_block_text.push_back('\n');
		// Выполняем запись пробела свёртки в содержимое блочного значения
		else this->_block_text.push_back(' ');
	}
	/**
	 * Выполняем запись пустых строк, содержимого дождавшихся
	 */
	for(size_t i = 0; i < this->_breaks; i++)
		// Выполняем запись перевода строки в содержимое блочного значения
		this->_block_text.push_back('\n');
	// Выполняем сброс количества пустых строк
	this->_breaks = 0;
	// Выполняем запись содержимого строки без отступа блочного значения
	this->_block_text.append(line.substr(this->_inner));
	// Запоминаем признак присоединения строки
	attached = true;
	// Выводим признак успешного присоединения строки
	return true;
}
/**
 * @brief Метод завершения собираемого блочного значения
 *
 * @param column положение завершения в разбираемой строке
 * @return       признак успешного завершения блочного значения
 *
 */
bool awh::codec::yaml::Reader::closing(const size_t column) noexcept {
	/**
	 * Если блочное значение не собирается вовсе
	 */
	if(!this->_blocking)
		// Выводим признак успешного завершения блочного значения
		return true;
	// Выполняем сброс признака сборки блочного значения
	this->_blocking = false;
	// Собираемое содержимое блочного значения
	string result(this->_block_text);
	/**
	 * Определяем правило усечения переводов строк
	 */
	switch(static_cast <uint8_t> (this->_chomp)){
		/**
		 * Если переводы строк снимаются все
		 */
		case static_cast <uint8_t> (chomp_t::STRIP): break;
		/**
		 * Если оставляется один перевод строки
		 */
		case static_cast <uint8_t> (chomp_t::CLIP): {
			/**
			 * Если содержимое блочного значения не пусто
			 */
			if(!result.empty())
				// Выполняем запись одного перевода строки
				result.push_back('\n');
		} break;
		/**
		 * Если переводы строк сохраняются все
		 */
		case static_cast <uint8_t> (chomp_t::KEEP): {
			/**
			 * Если содержимое блочного значения не пусто
			 */
			if(!result.empty())
				// Выполняем запись перевода строки, содержимое завершающего
				result.push_back('\n');
			/**
			 * Выполняем запись всех пустых строк, содержимого не дождавшихся
			 */
			for(size_t i = 0; i < this->_breaks; i++)
				// Выполняем запись перевода строки
				result.push_back('\n');
		} break;
	}
	// Выполняем сброс количества пустых строк
	this->_breaks = 0;
	// Выполняем сброс собираемого содержимого блочного значения
	this->_block_text.clear();
	/**
	 * Если поставить событие блочного значения не удалось
	 */
	if(!this->scalar(result, this->_block, this->_opening))
		// Выводим признак неудачного завершения блочного значения
		return false;
	// Устанавливаем вид строкового значения последнему событию
	this->_staged.back().type = type_t::STRING;
	// Устанавливаем положение заголовка значения поставленному событию
	this->_staged.back().location = this->_origin;
	// Не даём указателю на положение завершения пропасть без дела
	(void) column;
	// Выводим признак успешного завершения блочного значения
	return true;
}
/**
 * @brief Метод разбора значения внутри поточного построения
 *
 * @param line   разбираемая строка
 * @param offset смещение начала значения в строке, по выходе - смещение за ним
 * @return       признак успешного разбора значения
 *
 */
bool awh::codec::yaml::Reader::flowed(const string_view line, size_t & offset) noexcept {
	// Получаем первый знак записи значения
	const char leading = line[offset];
	/**
	 * Если значение открывается свойствами узла
	 */
	if((leading == '&') || (leading == '!')){
		/**
		 * Если разобрать свойства узла не удалось
		 */
		if(!this->property(line, offset))
			// Выводим признак неудачного разбора значения
			return false;
		/**
		 * Если за свойствами узла содержимое строки исчерпано
		 *
		 * @note Свойства вправе стоять и в строке, узлу своему предшествующей: узел ждёт их
		 *       строкою ниже, и накопленное дожидается его
		 */
		if(offset >= line.size())
			// Выводим признак успешного разбора значения
			return true;
		// Выполняем разбор содержимого за свойствами узла
		return this->flowed(line, offset);
	}
	/**
	 * Если значение является ссылкой на объявленную метку
	 */
	if(leading == '*')
		// Выполняем разбор ссылки на объявленную метку
		return this->referred(line, offset);
	/**
	 * Если значение открывается вопросом составного имени
	 *
	 * @details Составные имена заводятся вместе с держащим документ целиком: имя, само
	 *          построением являющееся, нуждается в дереве, а потоковому чтению деть его
	 *          некуда
	 */
	if(leading == '?')
		// Выводим отказ недопустимого знака в этом месте текста
		return this->fail(error_t::INVALID_CHARACTER, offset);
	// Вид записи разбираемого значения
	style_t style = style_t::PLAIN;
	// Смещение конца записи значения
	size_t position = offset;
	/**
	 * Если значение обнесено оградою
	 */
	if((leading == '\'') || (leading == '"')){
		// Длина записи значения вместе с оградою
		size_t length = 0;
		/**
		 * Если ограда значения не закрыта
		 */
		if(!this->bounds(line, offset, style, length))
			// Выводим отказ незакрытой ограды скалярного значения
			return this->fail(error_t::UNTERMINATED_SCALAR, offset);
		// Запоминаем смещение конца записи значения
		position = (offset + length);
	/**
	 * Если значение записано без ограды
	 */
	} else {
		/**
		 * Выполняем перебор всех знаков записи значения
		 *
		 * @note Правила окончания внутри поточного построения иные, нежели в блочном:
		 *       значение оканчивается запятой, закрывающей скобкой либо разделителем
		 *       имени пары, а не одним лишь концом строки
		 */
		while(position < line.size()){
			// Получаем очередной знак записи значения
			const char letter = line[position];
			/**
			 * Если знак оканчивает значение поточного построения
			 */
			if((letter == ',') || (letter == ']') || (letter == '}'))
				// Выходим из перебора знаков записи значения
				break;
			/**
			 * Если знак разделяет имя пары и значение её
			 */
			if((letter == ':') && (((position + 1) >= line.size()) || spacing(line[position + 1]) ||
			   (line[position + 1] == ',') || (line[position + 1] == ']') || (line[position + 1] == '}')))
				// Выходим из перебора знаков записи значения
				break;
			/**
			 * Если знак открывает примечание, стоя за пробельным знаком
			 */
			if((letter == '#') && (position > offset) && spacing(line[position - 1]))
				// Выходим из перебора знаков записи значения
				break;
			// Выполняем переход к следующему знаку записи
			position++;
		}
	}
	// Получаем запись значения без пробельной обвязки
	const string_view text = ((style == style_t::PLAIN) ?
		trimmed(line.substr(offset, (position - offset))) : line.substr(offset, (position - offset)));
	// Собираемое содержимое значения
	string value;
	/**
	 * Если снять ограду со значения не удалось
	 */
	if(!this->unquote(text, style, offset, value))
		// Выводим признак неудачного разбора значения
		return false;
	/**
	 * Если поставить событие значения не удалось
	 */
	if(!this->scalar(value, style, offset))
		// Выводим признак неудачного разбора значения
		return false;
	// Запоминаем смещение за разобранным значением
	offset = position;
	// Выводим признак успешного разбора значения
	return true;
}
/**
 * @brief Метод разбора поточного построения
 *
 * @param line   разбираемая строка
 * @param offset смещение начала разбора в строке, по выходе - смещение за ним
 * @return       признак успешного разбора построения
 *
 */
bool awh::codec::yaml::Reader::flowing(const string_view line, size_t & offset) noexcept {
	// Получаем предел глубины вложенности
	const size_t limit = ((this->_settings.depth > 0) ? this->_settings.depth : MAX_DEPTH);
	/**
	 * Выполняем разбор строки до исчерпания её либо до закрытия всех построений
	 */
	while(true){
		/**
		 * Выполняем пропуск пробельных знаков
		 */
		while((offset < line.size()) && spacing(line[offset]))
			// Выполняем переход к следующему знаку строки
			offset++;
		/**
		 * Если содержимое строки исчерпано
		 *
		 * @note Построение, скобками не закрытое, продолжается следующею строкой, и разбор
		 *       его прерывается здесь ровно на том месте, где остановился: стопа скобок
		 *       держится полем, а не возвратностью вызовов
		 */
		if(offset >= line.size())
			// Выводим признак успешного разбора построения
			return true;
		/**
		 * Если стопа открытых построений опустела
		 *
		 * @note Разбор окончен, и остаток строки разбирается уже обычным порядком - тем,
		 *       кто разбор поточный и затеял
		 */
		if(this->_flow.empty())
			// Выводим признак успешного разбора построения
			return true;
		// Получаем очередной разбираемый знак строки
		const char letter = line[offset];
		/**
		 * Если разбираемый знак открывает примечание
		 *
		 * @note Примечание внутри построения дозволено, и оканчивается им строка: остаток
		 *       её содержимым не является
		 */
		if((letter == '#') && ((offset == 0) || spacing(line[offset - 1]))){
			// Выполняем постановку события примечания
			this->remark(line, offset);
			// Выполняем переход за примечание, строку окончившее
			offset = line.size();
			// Выводим признак успешного разбора построения
			return true;
		}
		// Получаем открытое поточное построение
		bracket_t & bracket = this->_flow.back();
		// Признак того, что построение является перечнем значений
		const bool sequence = (bracket.kind == nesting_t::SEQUENCE);
		// Получаем знак, поточное построение закрывающий
		const char closer = (sequence ? ']' : '}');
		/**
		 * Если ожидается очередное значение построения
		 */
		if(this->_phase == flow_t::ENTRY){
			/**
			 * Если построение закрывается скобкой
			 */
			if(letter == closer){
				/**
				 * Если имя пары осталось без значения своего
				 */
				if(!sequence && bracket.filled && !bracket.valued){
					/**
					 * Если поставить событие пустого значения не удалось
					 */
					if(!this->scalar(string(), style_t::PLAIN, offset))
						// Выводим признак неудачного разбора построения
						return false;
					// Устанавливаем вид пустого значения последнему событию
					this->_staged.back().type = type_t::NUL;
				}
				// Выполняем переход за закрывающую скобку построения
				offset++;
				// Выполняем снятие закрытого построения со стопы
				this->_flow.pop_back();
				// Выполняем постановку события закрытия построения
				this->emit((sequence ? event_t::SEQUENCE_END : event_t::MAPPING_END), offset);
				// Запоминаем ожидание запятой либо закрывающей скобки
				this->_phase = flow_t::AFTER;
				// Выполняем переход к разбору следующего знака строки
				continue;
			}
			/**
			 * Если построение закрывается скобкой чужого вида
			 */
			if((letter == ']') || (letter == '}'))
				// Выводим отказ незакрытого поточного построения
				return this->fail(error_t::UNCLOSED_FLOW, offset);
			/**
			 * Если значение подменено запятой
			 */
			if(letter == ','){
				/**
				 * Если построение значений ещё не несёт
				 */
				if(!bracket.filled)
					// Выводим отказ ожидания значения
					return this->fail(error_t::EXPECTED_VALUE, offset);
				/**
				 * Если поставить событие пустого значения не удалось
				 */
				if(!this->scalar(string(), style_t::PLAIN, offset))
					// Выводим признак неудачного разбора построения
					return false;
				// Устанавливаем вид пустого значения последнему событию
				this->_staged.back().type = type_t::NUL;
				// Запоминаем ожидание запятой либо закрывающей скобки
				this->_phase = flow_t::AFTER;
				// Выполняем переход к разбору следующего знака строки
				continue;
			}
			/**
			 * Если значение открывается свойствами узла
			 *
			 * @details Свойства снимаются здесь, а не разбором значения: за ними вправе
			 *          стоять вложенное построение, а разбор значения построений не знает и
			 *          прочёл бы скобку началом записи без ограды. Снятые свойства ждут узла
			 *          своего и достаются ему, скаляр то будет либо построение
			 */
			if((letter == '&') || (letter == '!')){
				/**
				 * Если разобрать свойства узла не удалось
				 */
				if(!this->property(line, offset))
					// Выводим признак неудачного разбора построения
					return false;
				// Выполняем переход к разбору следующего знака строки
				continue;
			}
			/**
			 * Если значение является вложенным поточным построением
			 */
			if((letter == '[') || (letter == '{')){
				/**
				 * Если глубина вложенности предел превышает
				 */
				if((this->_levels.size() + this->_flow.size()) >= limit)
					// Выводим отказ превышения глубины вложенности
					return this->fail(error_t::DEPTH_EXCEEDED, offset);
				/**
				 * Если метка типа, построению предпосланная, виду его противна
				 */
				if(!this->matched(((letter == '[') ? nesting_t::SEQUENCE : nesting_t::MAPPING), offset))
					// Выводим признак неудачного разбора построения
					return false;
				// Запоминаем признак наполнения открытого построения
				bracket.filled = true;
				// Выполняем постановку события открытия вложенного построения
				item_t & item = this->emit(((letter == '[') ? event_t::SEQUENCE_START : event_t::MAPPING_START), offset);
				// Выполняем перенос накопленных свойств узла в собранное событие
				this->attach(item);
				// Запоминаем признак наполнения открытого документа
				this->_filled = true;
				// Выполняем открытие вложенного поточного построения
				this->_flow.emplace_back((letter == '[') ? nesting_t::SEQUENCE : nesting_t::MAPPING);
				// Выполняем переход за открывающую скобку построения
				offset++;
				// Выполняем переход к разбору следующего знака строки
				continue;
			}
			/**
			 * Если разобрать очередное значение не удалось
			 */
			if(!this->flowed(line, offset))
				// Выводим признак неудачного разбора построения
				return false;
			// Запоминаем признак наполнения открытого построения
			this->_flow.back().filled = true;
			// Запоминаем ожидание запятой либо закрывающей скобки
			this->_phase = flow_t::AFTER;
			// Выполняем переход к разбору следующего знака строки
			continue;
		}
		/**
		 * Если за значением стоит разделитель имени пары
		 */
		if(letter == ':'){
			/**
			 * Если построение является перечнем значений
			 *
			 * @note Пара внутри перечня описанием дозволена как отображение об одной паре,
			 *       однако построение это редко и вводит в заблуждение; заводится оно
			 *       вместе с составными именами
			 */
			if(sequence)
				// Выводим отказ недопустимого знака в этом месте текста
				return this->fail(error_t::INVALID_CHARACTER, offset);
			/**
			 * Если имя пары прочитано уже вместе со значением её
			 */
			if(bracket.valued)
				// Выводим отказ недопустимого знака в этом месте текста
				return this->fail(error_t::INVALID_CHARACTER, offset);
			// Запоминаем признак разбора значения пары
			bracket.valued = true;
			// Выполняем переход за разделитель имени пары
			offset++;
			// Запоминаем ожидание очередного значения построения
			this->_phase = flow_t::ENTRY;
			// Выполняем переход к разбору следующего знака строки
			continue;
		}
		/**
		 * Если построение закрывается скобкой
		 */
		if(letter == closer){
			/**
			 * Если имя пары осталось без значения своего
			 *
			 * @note Отображение `{a, b}` описанием дозволено: значения пар пусты. Выдаём их
			 *       пустыми значениями, ибо пара без значения есть пара
			 */
			if(!sequence && !bracket.valued){
				/**
				 * Если поставить событие пустого значения не удалось
				 */
				if(!this->scalar(string(), style_t::PLAIN, offset))
					// Выводим признак неудачного разбора построения
					return false;
				// Устанавливаем вид пустого значения последнему событию
				this->_staged.back().type = type_t::NUL;
			}
			// Выполняем переход за закрывающую скобку построения
			offset++;
			// Выполняем снятие закрытого построения со стопы
			this->_flow.pop_back();
			// Выполняем постановку события закрытия построения
			this->emit((sequence ? event_t::SEQUENCE_END : event_t::MAPPING_END), offset);
			// Выполняем переход к разбору следующего знака строки
			continue;
		}
		/**
		 * Если за значением стоит разделитель значений
		 */
		if(letter == ','){
			/**
			 * Если имя пары осталось без значения своего
			 */
			if(!sequence && !bracket.valued){
				/**
				 * Если поставить событие пустого значения не удалось
				 */
				if(!this->scalar(string(), style_t::PLAIN, offset))
					// Выводим признак неудачного разбора построения
					return false;
				// Устанавливаем вид пустого значения последнему событию
				this->_staged.back().type = type_t::NUL;
			}
			// Выполняем сброс признака разбора значения пары
			bracket.valued = false;
			// Выполняем переход за разделитель значений
			offset++;
			// Запоминаем ожидание очередного значения построения
			this->_phase = flow_t::ENTRY;
			// Выполняем переход к разбору следующего знака строки
			continue;
		}
		/**
		 * Выводим отказ ожидания запятой либо закрывающей скобки
		 *
		 * @note Отказом этим отвергается и скобка чужого вида, за значением стоящая: место
		 *       отказа названо им точнее, нежели одним лишь незакрытым построением
		 */
		return this->fail(error_t::EXPECTED_COMMA, offset);
	}
}
/**
 * @brief Метод откладывания выдачи простого значения
 *
 * @param text   содержимое простого значения
 * @param column положение значения в разбираемой строке
 * @return       признак успешного откладывания выдачи
 *
 */
bool awh::codec::yaml::Reader::deferred(const string & text, const size_t column) noexcept {
	// Запоминаем признак сборки простого значения
	this->_plaining = true;
	// Запоминаем собранное содержимое простого значения
	this->_plain.assign(text);
	// Выполняем сброс количества пустых строк значения
	this->_folds = 0;
	/**
	 * Запоминаем отступ, который обязано превышать продолжение значения
	 *
	 * @details Отступ этот берётся у построения, значение объемлющего, а не у строки, в
	 *          которой значение началось: запись `ключ:` со значением строкою ниже ставит
	 *          продолжение его на тот же отступ, что и начало, и мерить продолжение
	 *          началом значило бы оборвать значение на первой же строке
	 */
	this->_required = (this->_levels.empty() ? 0 : this->_levels.back().indent);
	// Запоминаем смещение начала значения от начала текста
	this->_origin.offset = (this->_position + column);
	// Запоминаем номер строки, где значение началось
	this->_origin.line = this->_line;
	// Запоминаем положение начала значения в строке
	this->_origin.column = static_cast <uint32_t> (column + 1);
	// Запоминаем глубину вложенности, где значение началось
	this->_origin.depth = static_cast <uint32_t> (this->_levels.size());
	// Выводим признак успешного откладывания выдачи
	return true;
}
/**
 * @brief Метод выдачи собранного простого значения
 *
 * @return признак успешной выдачи значения
 *
 */
bool awh::codec::yaml::Reader::settle() noexcept {
	/**
	 * Если простое значение не собирается вовсе
	 */
	if(!this->_plaining)
		// Выводим признак успешной выдачи значения
		return true;
	// Выполняем сброс признака сборки простого значения
	this->_plaining = false;
	// Получаем собранное содержимое простого значения
	const string text(this->_plain);
	// Выполняем сброс собираемого содержимого значения
	this->_plain.clear();
	// Выполняем сброс количества пустых строк значения
	this->_folds = 0;
	/**
	 * Если поставить событие простого значения не удалось
	 */
	if(!this->scalar(text, style_t::PLAIN, 0))
		// Выводим признак неудачной выдачи значения
		return false;
	/**
	 * Устанавливаем положение начала значения поставленному событию
	 *
	 * @note Событие ставится там, где значение окончилось, а стоит оно там, где началось:
	 *       потребителю указывать надлежит на начало записи, а не на конец её
	 */
	this->_staged.back().location = this->_origin;
	/**
	 * Выполняем перенос собранных событий в очередь выдачи
	 *
	 * @details Значение это принадлежит строке прошлой, и та разобрана уже целиком:
	 *          отказ строки нынешней отозвать его не вправе. Без переноса этого отказ
	 *          сбросил бы накопитель строки вместе со значением, прошлою строкой
	 *          собранным, и откладывание выдачи меняло бы исход разбора
	 */
	this->commit();
	// Выводим признак успешной выдачи значения
	return true;
}
/**
 * @brief Метод присоединения очередной строки к простому значению
 *
 * @param line     присоединяемая строка
 * @param attached признак присоединения строки к простому значению
 * @return         признак успешного присоединения строки
 *
 */
bool awh::codec::yaml::Reader::plaining(const string_view line, bool & attached) noexcept {
	// Выполняем сброс признака присоединения строки
	attached = false;
	// Смещение начала содержимого строки за отступом
	size_t offset = 0;
	/**
	 * Выполняем пропуск отступа присоединяемой строки
	 */
	while((offset < line.size()) && (line[offset] == ' '))
		// Выполняем переход к следующему знаку строки
		offset++;
	/**
	 * Если строка пуста
	 *
	 * @details Пустая строка значения не завершает: описание велит ей стать переводом
	 *          строки в собранном содержимом, а завершить значение вправе лишь строка
	 *          содержательная, отступом мельче. Оттого пустые строки лишь считаются, а
	 *          учтутся они приходом содержимого - либо пропадут, коли содержимого не будет
	 */
	if(offset >= line.size()){
		// Выполняем учёт пустой строки, содержимого ещё не дождавшейся
		this->_folds++;
		// Запоминаем признак присоединения строки
		attached = true;
		// Выводим признак успешного присоединения строки
		return true;
	}
	/**
	 * Если отступ строки отступа объемлющего построения не превышает
	 */
	if(static_cast <uint32_t> (offset) <= this->_required)
		// Выводим признак успешного присоединения строки
		return true;
	/**
	 * Если строка несёт одно лишь примечание
	 *
	 * @note Примечание простое значение завершает: описание примечаний внутри значения
	 *       не знает вовсе
	 */
	if(line[offset] == '#')
		// Выводим признак успешного присоединения строки
		return true;
	/**
	 * Если строка открывается объявлением значения перечня либо составного имени
	 *
	 * @details Записи эти внутри простого значения описанием запрещены прямо, и
	 *          продолжением значения они быть не могут. Прочесть их построением тоже
	 *          нельзя - построение стояло бы внутри значения, - и остаётся один отказ
	 */
	if(((line[offset] == '-') || (line[offset] == '?')) &&
	   (((offset + 1) >= line.size()) || spacing(line[offset + 1])))
		// Выводим отказ недопустимого знака в этом месте текста
		return this->fail(error_t::INVALID_CHARACTER, offset);
	/**
	 * Выполняем поиск разделителя имени пары в присоединяемой строке
	 */
	for(size_t i = offset; i < line.size(); i++){
		/**
		 * Если строка несёт разделитель имени пары
		 *
		 * @note Простое значение разделителя этого нести не вправе вовсе: неведомо, пара
		 *       ли это внутри значения либо значение с двоеточием, и описание запрещает
		 *       запись эту прямо
		 */
		if((line[i] == ':') && (((i + 1) >= line.size()) || spacing(line[i + 1])))
			// Выводим отказ недопустимого знака в этом месте текста
			return this->fail(error_t::INVALID_CHARACTER, i);
	}
	// Длина содержимого строки без примечания за ним
	size_t length = line.size();
	/**
	 * Выполняем поиск примечания за содержимым присоединяемой строки
	 */
	for(size_t i = offset; i < length; i++){
		/**
		 * Если знак открывает примечание, стоя за пробельным знаком
		 */
		if((line[i] == '#') && (i > offset) && spacing(line[i - 1])){
			// Запоминаем длину содержимого строки без примечания
			length = i;
			// Выходим из поиска примечания
			break;
		}
	}
	/**
	 * Если пустые строки содержимого дождались
	 *
	 * @note Свёртка описанием задана так: перевод строки один обращается пробелом, а
	 *       каждый следующий остаётся переводом
	 */
	if(this->_folds > 0)
		// Выполняем присоединение переводов строк к собираемому значению
		this->_plain.append(this->_folds, '\n');
	// Если пустых строк не было вовсе
	else this->_plain.append(1, ' ');
	// Выполняем сброс количества пустых строк значения
	this->_folds = 0;
	// Выполняем присоединение содержимого строки к собираемому значению
	this->_plain.append(trimmed(line.substr(offset, (length - offset))));
	// Запоминаем признак присоединения строки
	attached = true;
	/**
	 * Если за содержимым строки стоит примечание
	 */
	if(length < line.size()){
		/**
		 * Если выдать собранное простое значение не удалось
		 *
		 * @note Примечание значение завершает, и выдать его надлежит прежде постановки
		 *       события примечания: иначе примечание оказалось бы прежде значения, за
		 *       которым стоит
		 */
		if(!this->settle())
			// Выводим признак неудачного присоединения строки
			return false;
		// Выполняем постановку события примечания
		this->remark(line, length);
	}
	// Выводим признак успешного присоединения строки
	return true;
}
/**
 * @brief Метод разбора одной логической строки текста
 *
 * @param line разбираемая строка без знака конца строки
 * @return     признак успешного разбора строки
 *
 */
bool awh::codec::yaml::Reader::record(const string_view line) noexcept {
	// Выполняем учёт разбираемой строки
	this->_line++;
	/**
	 * Если разбирается поточное построение, скобками ещё не закрытое
	 *
	 * @note Строка эта построению и принадлежит целиком: ни отступ её, ни черта начала
	 *       документа смысла здесь не имеют, ибо внутри скобок отступ не значит ничего
	 */
	if(!this->_flow.empty()){
		// Смещение начала разбора строки
		size_t offset = 0;
		/**
		 * Если разобрать продолжение поточного построения не удалось
		 */
		if(!this->flowing(line, offset))
			// Выводим признак неудачного разбора строки
			return false;
		/**
		 * Если построение скобками закрыто, а строка исчерпана не вся
		 */
		if(this->_flow.empty()){
			/**
			 * Выполняем пропуск пробельных знаков за поточным построением
			 */
			while((offset < line.size()) && spacing(line[offset]))
				// Выполняем переход к следующему знаку строки
				offset++;
			/**
			 * Если за построением стоит примечание
			 */
			if((offset < line.size()) && (line[offset] == '#'))
				// Выполняем постановку события примечания
				this->remark(line, offset);
			/**
			 * Если за построением стоит содержимое, примечанием не являющееся
			 */
			else if(offset < line.size())
				// Выводим отказ содержимого за завершённой записью
				return this->fail(error_t::TRAILING_CHARACTERS, offset);
		}
		// Выполняем перенос собранных событий строки в очередь выдачи
		this->commit();
		// Выводим признак успешного разбора строки
		return true;
	}
	/**
	 * Если собирается простое значение, могущее прирасти
	 */
	if(this->_plaining){
		// Признак присоединения строки к простому значению
		bool attached = false;
		/**
		 * Если присоединить строку к простому значению не удалось
		 */
		if(!this->plaining(line, attached))
			// Выводим признак неудачного разбора строки
			return false;
		/**
		 * Если строка присоединена к простому значению
		 */
		if(attached){
			// Выполняем перенос собранных событий строки в очередь выдачи
			this->commit();
			// Выводим признак успешного разбора строки
			return true;
		}
		/**
		 * Если выдать собранное простое значение не удалось
		 *
		 * @note Строка, значению не принадлежащая, завершает его и разбирается затем
		 *       обычным порядком - здесь же, ниже
		 */
		if(!this->settle())
			// Выводим признак неудачного разбора строки
			return false;
	}
	/**
	 * Если собирается блочное значение
	 */
	if(this->_blocking){
		// Признак присоединения строки к блочному значению
		bool attached = false;
		/**
		 * Если присоединить строку к блочному значению не удалось
		 */
		if(!this->blocking(line, attached))
			// Выводим признак неудачного разбора строки
			return false;
		/**
		 * Если строка присоединена к блочному значению
		 */
		if(attached){
			// Выполняем перенос собранных событий строки в очередь выдачи
			this->commit();
			// Выводим признак успешного разбора строки
			return true;
		}
		/**
		 * Если завершить блочное значение не удалось
		 *
		 * @note Строка, блочному значению не принадлежащая, завершает его и разбирается
		 *       затем обычным порядком - здесь же, ниже
		 */
		if(!this->closing(0))
			// Выводим признак неудачного разбора строки
			return false;
	}
	// Смещение начала содержимого строки за отступом
	size_t offset = 0;
	/**
	 * Выполняем пропуск отступа разбираемой строки
	 */
	while((offset < line.size()) && (line[offset] == ' '))
		// Выполняем переход к следующему знаку строки
		offset++;
	/**
	 * Если отступ содержит знак горизонтальной подачи
	 *
	 * @note Описание запрещает его в отступе прямо: ширина его толкуется по-разному, и
	 *       смысл текста зависел бы от настроек показывающего его
	 */
	if((offset < line.size()) && (line[offset] == '\t')){
		// Смещение проверяемого знака строки
		size_t position = offset;
		/**
		 * Выполняем пропуск пробельных знаков за отступом
		 */
		while((position < line.size()) && spacing(line[position]))
			// Выполняем переход к следующему знаку строки
			position++;
		/**
		 * Если за пробельными знаками стоит содержимое
		 */
		if((position < line.size()) && (line[position] != '#'))
			// Выводим отказ знака горизонтальной подачи в отступе
			return this->fail(error_t::TAB_IN_INDENTATION, offset);
		// Запоминаем смещение начала содержимого строки
		offset = position;
	}
	// Получаем отступ разбираемой строки
	const uint32_t indent = static_cast <uint32_t> (offset);
	/**
	 * Запоминаем отступ разбираемой строки
	 *
	 * @note Отступ этот нужен блочному значению: содержимое его обязано стоять глубже
	 *       отступа строки, заголовок несущей, а не глубже столбца самого заголовка
	 */
	this->_margin = indent;
	/**
	 * Если строка пуста
	 */
	if(offset >= line.size()){
		/**
		 * Если выдача пустых строк затребована
		 */
		if(this->_settings.emitBlanks && this->_opened)
			// Выполняем постановку события пустой строки
			this->emit(event_t::BLANK, offset);
		// Выполняем перенос собранных событий строки в очередь выдачи
		this->commit();
		// Выводим признак успешного разбора строки
		return true;
	}
	/**
	 * Если строка несёт одно лишь примечание
	 */
	if(line[offset] == '#'){
		// Выполняем постановку события примечания
		this->remark(line, offset);
		// Выполняем перенос собранных событий строки в очередь выдачи
		this->commit();
		// Выводим признак успешного разбора строки
		return true;
	}
	/**
	 * Если строка объявляет начало нового документа
	 */
	if((line.compare(offset, 3, "---") == 0) && (((offset + 3) >= line.size()) || spacing(line[offset + 3]))){
		/**
		 * Если закрыть открытый документ не удалось
		 */
		if(!this->finish(offset))
			// Выводим признак неудачного разбора строки
			return false;
		// Выполняем постановку события начала документа
		this->emit(event_t::DOCUMENT_START, offset);
		// Запоминаем признак открытия документа
		this->_opened = true;
		// Выполняем сброс признака наполнения документа
		this->_filled = false;
		/**
		 * Выполняем сброс признака предпосланных директив
		 *
		 * @note Директивы, черте предшествовавшие, относятся к документу, ею открытому, и
		 *       ожидание черты они исполнили
		 */
		this->_directed = false;
		// Получаем смещение содержимого за объявлением документа
		size_t position = (offset + 3);
		/**
		 * Выполняем пропуск пробельных знаков за объявлением документа
		 */
		while((position < line.size()) && spacing(line[position]))
			// Выполняем переход к следующему знаку строки
			position++;
		/**
		 * Если разобрать содержимое за объявлением документа не удалось
		 */
		if(!this->content(line, position, static_cast <uint32_t> (position)))
			// Выводим признак неудачного разбора строки
			return false;
		// Выполняем перенос собранных событий строки в очередь выдачи
		this->commit();
		// Выводим признак успешного разбора строки
		return true;
	}
	/**
	 * Если строка объявляет конец документа
	 */
	if((line.compare(offset, 3, "...") == 0) && (((offset + 3) >= line.size()) || spacing(line[offset + 3]))){
		/**
		 * Если закрыть открытый документ не удалось
		 */
		if(!this->finish(offset))
			// Выводим признак неудачного разбора строки
			return false;
		// Выполняем перенос собранных событий строки в очередь выдачи
		this->commit();
		// Выводим признак успешного разбора строки
		return true;
	}
	/**
	 * Если строка объявляет директиву
	 */
	if(line[offset] == '%'){
		/**
		 * Если разобрать директиву не удалось
		 */
		if(!this->directive(line, offset))
			// Выводим признак неудачного разбора строки
			return false;
		// Выполняем перенос собранных событий строки в очередь выдачи
		this->commit();
		// Выводим признак успешного разбора строки
		return true;
	}
	/**
	 * Если документ ещё не открыт
	 */
	if(!this->_opened){
		/**
		 * Если документу предпосланы директивы
		 *
		 * @note Описание велит документу с директивами открываться чертою прямо: без неё
		 *       неведомо, где кончаются директивы и начинается содержимое, и `%TAG`
		 *       оказался бы отнесён неизвестно к чему
		 */
		if(this->_directed)
			// Выводим отказ ошибочного построения директивы
			return this->fail(error_t::INVALID_DIRECTIVE, offset);
		// Выполняем постановку события начала документа
		this->emit(event_t::DOCUMENT_START, offset);
		// Запоминаем признак открытия документа
		this->_opened = true;
		// Выполняем сброс признака наполнения документа
		this->_filled = false;
	/**
	 * Если документ открыт и содержимое его уже прочитано
	 */
	} else if(this->_filled && this->_levels.empty() && !this->_expected)
		// Выводим отказ содержимого за завершённой записью
		return this->fail(error_t::TRAILING_CHARACTERS, offset);
	/**
	 * Если ожидалось значение пары, а отступ строки глубже отступа имени её
	 */
	if(this->_expected && (indent > this->_pending))
		// Выполняем сброс признака ожидания значения пары
		this->_expected = false;
	/**
	 * Признак того, что строка объявляет очередное значение перечня
	 *
	 * @note Знать это надлежит прежде выдачи пустого значения пары: перечень, стоящий на
	 *       отступе имени своей пары, есть значение её, а не пустота вместо него
	 */
	const bool entry = ((line[offset] == '-') && (((offset + 1) >= line.size()) || spacing(line[offset + 1])));
	/**
	 * Признак того, что перечень строки есть значение пары, объявленной прежде
	 *
	 * @note Значением её он является лишь на отступе имени её самой: черта, стоящая
	 *       отступом мельче, к паре отношения не имеет вовсе, и пустоту значения её
	 *       надлежит выдать прежде закрытия уровней
	 */
	const bool implied = (entry && !this->_entered && (indent == this->_pending));
	/**
	 * Если ожидалось значение пары, а строка стоит на отступе имени её, перечнем не являясь
	 *
	 * @details Выдаётся оно прежде закрытия уровней, а не за ним: значение принадлежит
	 *          отображению, имя своё несущему, и выданное за концом его легло бы именем
	 *          пары уровня объемлющего. Нашёл это ворошитель сличением перезаписи: запись
	 *          `''` становилась именем следующей пары, а значение её - значением пустоты
	 *
	 * @note Ожидание записи перечня чертою не снимается: черта на отступе ожидания есть
	 *       запись следующая, и пустоту прежней записи надлежит выдать прежде неё
	 */
	if(this->_expected && !implied && (indent <= this->_pending)){
		// Выполняем сброс признака ожидания значения пары
		this->_expected = false;
		/**
		 * Если поставить событие пустого значения не удалось
		 */
		if(!this->scalar(string(), style_t::PLAIN, offset))
			// Выводим признак неудачного разбора строки
			return false;
		// Устанавливаем вид пустого значения последнему событию
		this->_staged.back().type = type_t::NUL;
	}
	/**
	 * Если закрыть уровни глубже отступа строки не удалось
	 */
	if(!this->collapse(indent, offset))
		// Выводим признак неудачного разбора строки
		return false;
	/**
	 * Если содержимое документа прочитано, а строка несёт ещё
	 *
	 * @details Проверка эта повторяет ту, что стоит прежде закрытия уровней, и повторяет
	 *          не зря: написание ` - a` со строкою `- b` за ним закрывает перечень отступа
	 *          мельче и открывает новый - а документ корень имеет один, и второго ему не
	 *          положено. Прежде закрытия уровень ещё открыт, и проверка та молчит. Нашёл
	 *          это ворошитель сличением дословной перезаписи: дерево клало второй перечень
	 *          вторым корнем, и счёт документов расходился с числом их в тексте
	 */
	if(this->_filled && this->_levels.empty() && !this->_expected && !this->_plaining)
		// Выводим отказ содержимого за завершённой записью
		return this->fail(error_t::TRAILING_CHARACTERS, offset);
	/**
	 * Если разобрать содержимое строки за отступом не удалось
	 */
	if(!this->content(line, offset, indent))
		// Выводим признак неудачного разбора строки
		return false;
	// Выполняем перенос собранных событий строки в очередь выдачи
	this->commit();
	// Выводим признак успешного разбора строки
	return true;
}
/**
 * @brief Метод подачи очередного куска исходного текста
 *
 * @param buffer подаваемый кусок исходного текста
 * @param size   размер подаваемого куска
 * @param end    признак того, что текст окончен
 * @return       признак успешного разбора поданного куска
 *
 */
bool awh::codec::yaml::Reader::feed(const void * buffer, const size_t size, const bool end) noexcept {
	/**
	 * Если разбор уже прекращён отказом
	 */
	if(this->_state == state_t::FAILED)
		// Выводим признак неудачного разбора поданного куска
		return false;
	/**
	 * Если текст уже разобран до конца
	 */
	if(this->_state == state_t::FINISHED)
		// Выводим отказ содержимого за завершённой записью
		return this->fail(error_t::TRAILING_CHARACTERS, 0);
	// Запоминаем состояние разбора текста
	this->_state = state_t::PARSING;
	/**
	 * Если начало потока ещё не выдано
	 */
	if(!this->_started){
		// Выполняем постановку события начала потока
		this->emit(event_t::STREAM_START, 0);
		// Выполняем перенос собранного события в очередь выдачи
		this->commit();
		// Запоминаем признак выдачи начала потока
		this->_started = true;
	}
	// Признак успешного приведения поданного куска к UTF-8
	const bool converted = this->_decoder.convert(buffer, size, end, this->_buffer);
	/**
	 * Признак того, что текст окончен и приведён целиком
	 *
	 * @details Приведение переносит в накопитель всё, что успело прочесть до битой
	 * последовательности, и строки те разобрать надлежит: при подаче по байту они
	 * разобраны были бы прежними кусками, и отказ пришёл бы уже за ними. Строка же
	 * последняя, переводом не закрытая, разбору не подлежит - прирасти ей уже нечем,
	 * но и признать текст оконченным нельзя: окончен он отказом, а не концом своим
	 */
	const bool complete = (end && converted);
	/**
	 * Выполняем разбор всех строк, поданных целиком
	 */
	while(this->_offset < this->_buffer.size()){
		// Разыскиваем знак конца очередной строки
		const size_t position = this->_buffer.find('\n', this->_offset);
		/**
		 * Если знака конца строки в накопителе нет вовсе
		 */
		if(position == string::npos){
			/**
			 * Если текст ещё не окончен
			 *
			 * @note Строка вправе прирасти следующим куском, и разбирать её сейчас значило
			 *       бы ставить исход разбора в зависимость от нарезки текста
			 */
			if(!complete)
				// Выходим из разбора строк накопителя
				break;
			// Получаем последнюю строку накопителя
			string_view line(this->_buffer.data() + this->_offset, (this->_buffer.size() - this->_offset));
			/**
			 * Если строка оканчивается возвратом каретки
			 */
			if(!line.empty() && (line.back() == '\r'))
				// Выполняем снятие возврата каретки с конца строки
				line = line.substr(0, (line.size() - 1));
			// Запоминаем смещение начала неразобранного текста
			const size_t consumed = this->_offset;
			// Запоминаем смещение разбираемой строки от начала текста
			this->_position = static_cast <uint64_t> (consumed);
			// Выполняем переход за разобранную строку
			this->_offset = this->_buffer.size();
			/**
			 * Если разобрать последнюю строку не удалось
			 */
			if(!this->record(line))
				// Выводим признак неудачного разбора поданного куска
				return false;
			// Выходим из разбора строк накопителя
			break;
		}
		// Получаем очередную строку накопителя
		string_view line(this->_buffer.data() + this->_offset, (position - this->_offset));
		/**
		 * Если строка оканчивается возвратом каретки
		 */
		if(!line.empty() && (line.back() == '\r'))
			// Выполняем снятие возврата каретки с конца строки
			line = line.substr(0, (line.size() - 1));
		// Запоминаем смещение разбираемой строки от начала текста
		this->_position = static_cast <uint64_t> (this->_offset);
		// Выполняем переход за разобранную строку
		this->_offset = (position + 1);
		/**
		 * Если разобрать очередную строку не удалось
		 */
		if(!this->record(line))
			// Выводим признак неудачного разбора поданного куска
			return false;
	}
	/**
	 * Если привести поданный кусок к UTF-8 не удалось
	 */
	if(!converted)
		// Выводим отказ приведения кодировки исходного текста
		return this->fail(this->_decoder.error(), 0);
	/**
	 * Если текст окончен
	 */
	if(end){
		/**
		 * Если поточное построение скобками так и не закрыто
		 *
		 * @note Отказ этот объявляется здесь, а не строкою ниже: закрытие документа выдало
		 *       бы события закрытия построений, которых текст не содержит
		 */
		if(!this->_flow.empty())
			// Выводим отказ незакрытого поточного построения
			return this->fail(error_t::UNCLOSED_FLOW, 0);
		/**
		 * Если выдать собранное простое значение не удалось
		 */
		if(!this->settle())
			// Выводим признак неудачного разбора поданного куска
			return false;
		/**
		 * Если завершить собираемое блочное значение не удалось
		 */
		if(!this->closing(0))
			// Выводим признак неудачного разбора поданного куска
			return false;
		/**
		 * Если закрыть открытый документ не удалось
		 */
		if(!this->finish(0))
			// Выводим признак неудачного разбора поданного куска
			return false;
		// Выполняем постановку события конца потока
		this->emit(event_t::STREAM_END, 0);
		// Выполняем перенос собранных событий в очередь выдачи
		this->commit();
		// Запоминаем состояние окончания разбора
		this->_state = state_t::FINISHED;
	}
	// Выводим признак успешного разбора поданного куска
	return true;
}
/**
 * @brief Метод подачи исходного текста целиком
 *
 * @param text подаваемый исходный текст
 * @return     признак успешного разбора поданного текста
 *
 */
bool awh::codec::yaml::Reader::feed(const string_view text) noexcept {
	// Выполняем подачу исходного текста целиком
	return this->feed(text.data(), text.size(), true);
}
/**
 * @brief Метод получения очередного события разбора
 *
 * @return признак того, что событие получено
 *
 */
bool awh::codec::yaml::Reader::next() noexcept {
	/**
	 * Если очередь собранных событий пуста
	 */
	if(this->_reading >= this->_events.size()){
		// Выполняем сброс события, выданного последним
		this->_current = item_t();
		// Выполняем сброс значения, выданного последним событием
		this->_content = content_t();
		/**
		 * Если текст разобран до конца
		 */
		if(this->_state == state_t::FINISHED)
			// Устанавливаем событие окончания разбора текста
			this->_current.event = event_t::FINISH;
		// Выводим признак того, что событий больше нет
		return false;
	}
	// Получаем очередное событие разбора
	this->_current = this->_events.at(this->_reading);
	// Выполняем снятие полученного события с очереди
	this->_reading++;
	// Устанавливаем вид значения полученного события
	this->_content.type = this->_current.type;
	// Устанавливаем вид записи значения полученного события
	this->_content.style = this->_current.style;
	// Устанавливаем положение значения в исходном тексте
	this->_content.location = this->_current.location;
	// Запоминаем признак того, что значение собрано внутри поточного построения
	this->_content.flow = this->_current.flow;
	// Устанавливаем содержимое значения полученного события
	this->_content.text = string_view(this->_storage.data() + this->_current.offset, this->_current.length);
	// Устанавливаем имя метки, событию предпосланной
	this->_content.anchor = string_view(this->_storage.data() + this->_current.anchor.offset, this->_current.anchor.length);
	// Устанавливаем метку типа, событию предпосланную
	this->_content.tag = string_view(this->_storage.data() + this->_current.tag.offset, this->_current.tag.length);
	// Выводим признак того, что событие получено
	return true;
}
