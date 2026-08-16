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
 _state(state_t::READY), _error(error_t::NONE), _offset(0), _line(0), _position(0),
 _started(false), _opened(false), _filled(false), _blocking(false), _block(style_t::LITERAL),
 _chomp(chomp_t::CLIP), _marked(NO_INDENT), _outer(0), _margin(0), _inner(0), _opening(0),
 _breaks(0), _expected(false), _pending(0) {}
/**
 * @brief Конструктор
 *
 * @param settings настройки разбора текста
 *
 */
awh::codec::yaml::Reader::Reader(const settings_t & settings) noexcept :
 _settings(settings), _state(state_t::READY), _error(error_t::NONE), _offset(0),
 _line(0), _position(0), _started(false), _opened(false), _filled(false),
 _blocking(false), _block(style_t::LITERAL), _chomp(chomp_t::CLIP), _marked(NO_INDENT),
 _outer(0), _margin(0), _inner(0), _opening(0), _breaks(0), _expected(false), _pending(0) {
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
	// Выполняем сброс отступа, на котором ожидается значение пары
	this->_pending = 0;
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
	while(!this->_staged.empty()){
		// Выполняем перенос очередного собранного события
		this->_events.emplace_back(this->_staged.front());
		// Выполняем снятие перенесённого события с накопителя строки
		this->_staged.pop_front();
	}
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
	// Выполняем постановку события скалярного значения
	item_t & item = this->emit(event_t::SCALAR, column);
	// Запоминаем вид записи значения в исходном тексте
	item.style = style;
	/**
	 * Запоминаем вид значения, разрешённый действующей схемой
	 *
	 * @note Значение, обнесённое оградою, схеме не подлежит вовсе - оно строка всегда:
	 *       ограда для того и ставится, чтобы `12` осталось строкой
	 */
	item.type = ((style == style_t::PLAIN) ? resolve(text, this->_settings.schema) : type_t::STRING);
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
	// Выполняем постановку события открытия уровня
	this->emit(((kind == nesting_t::MAPPING) ? event_t::MAPPING_START : event_t::SEQUENCE_START), column);
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
	/**
	 * Если пара осталась без значения своего
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
	// Выполняем постановку события закрытия документа
	this->emit(event_t::DOCUMENT_END, column);
	// Выполняем сброс признака открытия документа
	this->_opened = false;
	// Выполняем сброс признака наполнения документа
	this->_filled = false;
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
	 * Если содержимое открывается построением, ещё не заведённым
	 *
	 * @details Поточные скобки, блочные значения, метки, ссылки и метки типов заводятся
	 *          следующими этапами работ. Отвечать на них молчаливым разбором наугад
	 *          нельзя: разбор выдал бы дерево, исходному тексту не отвечающее, и
	 *          потребитель узнал бы об этом не сразу
	 */
	switch(leading){
		// Если содержимое открывается меткой либо ссылкой
		case '&':
		case '*':
		// Если содержимое открывается меткой типа
		case '!':
		// Если содержимое открывается вопросом составного имени
		case '?':
			// Выводим отказ недопустимого знака в этом месте текста
			return this->fail(error_t::INVALID_CHARACTER, offset);
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
		// Смещение разбираемого знака строки
		size_t position = offset;
		/**
		 * Если разобрать поточное построение не удалось
		 */
		if(!this->flowing(line, position, 0))
			// Выводим признак неудачного разбора содержимого
			return false;
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
		 * Если содержимое строки исчерпано либо за ним стоит одно примечание
		 */
		if((position >= line.size()) || (line[position] == '#')){
			// Запоминаем признак ожидания значения пары
			this->_expected = true;
			// Запоминаем отступ, на котором ожидается значение пары
			this->_pending = indent;
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
	// Запоминаем отступ строки, заголовок несущей
	this->_outer = indent;
	// Запоминаем положение заголовка блочного значения в строке
	this->_opening = offset;
	/**
	 * Запоминаем отступ содержимого блочного значения
	 *
	 * @note Отступ, заголовком заданный, отсчитывается от отступа строки заголовка;
	 *       отступ же, заголовком не заданный, берётся по первой непустой строке
	 *       содержимого и здесь ещё неизвестен
	 */
	this->_inner = ((this->_marked != NO_INDENT) ? (indent + this->_marked) : 0);
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
 * @param depth  глубина вложенности поточных построений
 * @return       признак успешного разбора значения
 *
 */
bool awh::codec::yaml::Reader::flowed(const string_view line, size_t & offset, const uint32_t depth) noexcept {
	/**
	 * Выполняем пропуск пробельных знаков перед значением
	 */
	while((offset < line.size()) && spacing(line[offset]))
		// Выполняем переход к следующему знаку строки
		offset++;
	/**
	 * Если значение оборвано концом строки
	 *
	 * @note Поточное построение, на многие строки растянутое, заводится следующим этапом
	 *       работ: пока скобка обязана закрыться в той же строке, где открылась
	 */
	if(offset >= line.size())
		// Выводим отказ незакрытого поточного построения
		return this->fail(error_t::UNCLOSED_FLOW, offset);
	// Получаем первый знак записи значения
	const char leading = line[offset];
	/**
	 * Если значение является вложенным поточным построением
	 */
	if((leading == '[') || (leading == '{'))
		// Выполняем разбор вложенного поточного построения
		return this->flowing(line, offset, (depth + 1));
	/**
	 * Если значение открывается построением, ещё не заведённым
	 */
	switch(leading){
		// Если значение открывается меткой либо ссылкой
		case '&':
		case '*':
		// Если значение открывается меткой типа
		case '!':
		// Если значение открывается вопросом составного имени
		case '?':
			// Выводим отказ недопустимого знака в этом месте текста
			return this->fail(error_t::INVALID_CHARACTER, offset);
	}
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
 * @param offset смещение начала построения в строке, по выходе - смещение за ним
 * @param depth  глубина вложенности поточных построений
 * @return       признак успешного разбора построения
 *
 */
bool awh::codec::yaml::Reader::flowing(const string_view line, size_t & offset, const uint32_t depth) noexcept {
	// Получаем предел глубины вложенности
	const size_t limit = ((this->_settings.depth > 0) ? this->_settings.depth : MAX_DEPTH);
	/**
	 * Если глубина вложенности предел превышает
	 */
	if((this->_levels.size() + depth) >= limit)
		// Выводим отказ превышения глубины вложенности
		return this->fail(error_t::DEPTH_EXCEEDED, offset);
	// Получаем знак, поточное построение открывающий
	const char leading = line[offset];
	// Признак того, что построение является перечнем значений
	const bool sequence = (leading == '[');
	// Получаем знак, поточное построение закрывающий
	const char closer = (sequence ? ']' : '}');
	// Выполняем постановку события открытия построения
	this->emit((sequence ? event_t::SEQUENCE_START : event_t::MAPPING_START), offset);
	// Запоминаем признак наполнения открытого документа
	this->_filled = true;
	// Выполняем переход за открывающую скобку построения
	offset++;
	// Признак того, что построение пусто
	bool empty = true;
	/**
	 * Выполняем разбор всех значений поточного построения
	 */
	while(true){
		/**
		 * Выполняем пропуск пробельных знаков перед значением
		 */
		while((offset < line.size()) && spacing(line[offset]))
			// Выполняем переход к следующему знаку строки
			offset++;
		/**
		 * Если построение оборвано концом строки
		 */
		if(offset >= line.size())
			// Выводим отказ незакрытого поточного построения
			return this->fail(error_t::UNCLOSED_FLOW, offset);
		/**
		 * Если построение закрывается скобкой
		 */
		if(line[offset] == closer){
			// Выполняем переход за закрывающую скобку построения
			offset++;
			// Выполняем постановку события закрытия построения
			this->emit((sequence ? event_t::SEQUENCE_END : event_t::MAPPING_END), offset);
			// Выводим признак успешного разбора построения
			return true;
		}
		/**
		 * Если построение закрывается скобкой чужого вида
		 */
		if((line[offset] == ']') || (line[offset] == '}'))
			// Выводим отказ незакрытого поточного построения
			return this->fail(error_t::UNCLOSED_FLOW, offset);
		/**
		 * Если значению предшествует запятая, а построение пусто
		 */
		if((line[offset] == ',') && empty)
			// Выводим отказ ожидания значения
			return this->fail(error_t::EXPECTED_VALUE, offset);
		/**
		 * Если разобрать очередное значение не удалось
		 */
		if(!this->flowed(line, offset, depth))
			// Выводим признак неудачного разбора построения
			return false;
		// Запоминаем признак наполнения построения
		empty = false;
		/**
		 * Выполняем пропуск пробельных знаков за значением
		 */
		while((offset < line.size()) && spacing(line[offset]))
			// Выполняем переход к следующему знаку строки
			offset++;
		/**
		 * Если значение оборвано концом строки
		 */
		if(offset >= line.size())
			// Выводим отказ незакрытого поточного построения
			return this->fail(error_t::UNCLOSED_FLOW, offset);
		/**
		 * Если за значением стоит разделитель имени пары
		 */
		if(line[offset] == ':'){
			/**
			 * Если построение является перечнем значений
			 *
			 * @note Пара внутри перечня описанием дозволена как отображение об одной паре,
			 *       однако построение это редко и вводит в заблуждение; заводится оно
			 *       следующим этапом работ вместе с составными именами
			 */
			if(sequence)
				// Выводим отказ недопустимого знака в этом месте текста
				return this->fail(error_t::INVALID_CHARACTER, offset);
			// Выполняем переход за разделитель имени пары
			offset++;
			/**
			 * Если разобрать значение пары не удалось
			 */
			if(!this->flowed(line, offset, depth))
				// Выводим признак неудачного разбора построения
				return false;
			/**
			 * Выполняем пропуск пробельных знаков за значением пары
			 */
			while((offset < line.size()) && spacing(line[offset]))
				// Выполняем переход к следующему знаку строки
				offset++;
			/**
			 * Если значение оборвано концом строки
			 */
			if(offset >= line.size())
				// Выводим отказ незакрытого поточного построения
				return this->fail(error_t::UNCLOSED_FLOW, offset);
		/**
		 * Если построение является отображением, а имя пары значения не имеет
		 *
		 * @note Отображение `{a, b}` описанием дозволено: значения пар пусты. Выдаём их
		 *       пустыми значениями, ибо пара без значения есть пара
		 */
		} else if(!sequence){
			/**
			 * Если поставить событие пустого значения не удалось
			 */
			if(!this->scalar(string(), style_t::PLAIN, offset))
				// Выводим признак неудачного разбора построения
				return false;
			// Устанавливаем вид пустого значения последнему событию
			this->_staged.back().type = type_t::NUL;
		}
		/**
		 * Если за значением стоит запятая
		 */
		if(line[offset] == ',')
			// Выполняем переход за разделитель значений
			offset++;
		/**
		 * Если за значением стоит содержимое, построению не принадлежащее
		 */
		else if(line[offset] != closer)
			// Выводим отказ ожидания запятой либо закрывающей скобки
			return this->fail(error_t::EXPECTED_COMMA, offset);
	}
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
	 *
	 * @note Директивы заводятся следующим этапом работ вместе с метками типов: разбирать
	 *       их наугад нельзя, ибо директива %TAG меняет толкование меток всего документа
	 */
	if(line[offset] == '%')
		// Выводим отказ недопустимого знака в этом месте текста
		return this->fail(error_t::INVALID_CHARACTER, offset);
	/**
	 * Если документ ещё не открыт
	 */
	if(!this->_opened){
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
	 * Если закрыть уровни глубже отступа строки не удалось
	 */
	if(!this->collapse(indent, offset))
		// Выводим признак неудачного разбора строки
		return false;
	/**
	 * Признак того, что строка объявляет очередное значение перечня
	 *
	 * @note Знать это надлежит прежде выдачи пустого значения пары: перечень, стоящий на
	 *       отступе имени своей пары, есть значение её, а не пустота вместо него
	 */
	const bool entry = ((line[offset] == '-') && (((offset + 1) >= line.size()) || spacing(line[offset + 1])));
	/**
	 * Если ожидалось значение пары, а строка стоит на отступе имени её, перечнем не являясь
	 */
	if(this->_expected && !entry && (indent <= this->_pending)){
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
	/**
	 * Если привести поданный кусок к UTF-8 не удалось
	 */
	if(!this->_decoder.convert(buffer, size, end, this->_buffer))
		// Выводим отказ приведения кодировки исходного текста
		return this->fail(this->_decoder.error(), 0);
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
			if(!end)
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
	 * Если текст окончен
	 */
	if(end){
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
	if(this->_events.empty()){
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
	this->_current = this->_events.front();
	// Выполняем снятие полученного события с очереди
	this->_events.pop_front();
	// Устанавливаем вид значения полученного события
	this->_content.type = this->_current.type;
	// Устанавливаем вид записи значения полученного события
	this->_content.style = this->_current.style;
	// Устанавливаем положение значения в исходном тексте
	this->_content.location = this->_current.location;
	// Устанавливаем содержимое значения полученного события
	this->_content.text = string_view(this->_storage.data() + this->_current.offset, this->_current.length);
	// Выводим признак того, что событие получено
	return true;
}
