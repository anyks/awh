/**
 * @file: common.cpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация общих определений контейнера TOML — описания кодов ошибок разбора,
 *        названия кодировок исходного текста и типов значений, знаки конца строки и
 *        признак знака, допустимого в имени ключа без кавычек
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <codec/toml/common.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения описания кода ошибки
 *
 * @param error код ошибки разбора или записи
 * @return      описание кода ошибки
 *
 */
const char * awh::codec::toml::message(const error_t error) noexcept {
	/**
	 * Определяем код ошибки разбора или записи
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE):
			// Выводим описание кода ошибки
			return "no error";
		// Если обнаружена внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL):
			// Выводим описание кода ошибки
			return "internal parser error";
		// Если текст оборвался посреди записи
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF):
			// Выводим описание кода ошибки
			return "unexpected end of document";
		// Если знак недопустим в тексте настроек
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER):
			// Выводим описание кода ошибки
			return "invalid character in document";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			// Выводим описание кода ошибки
			return "byte sequence does not match the declared encoding";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING):
			// Выводим описание кода ошибки
			return "declared encoding is not supported";
		// Если объявление таблицы построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_TABLE):
			// Выводим описание кода ошибки
			return "malformed table header";
		// Если объявление таблицы не закрыто квадратной скобкой
		case static_cast <uint8_t> (error_t::UNCLOSED_TABLE):
			// Выводим описание кода ошибки
			return "unterminated table header";
		// Если имя ключа пусто
		case static_cast <uint8_t> (error_t::EMPTY_KEY):
			// Выводим описание кода ошибки
			return "empty key name";
		// Если имя ключа содержит недопустимые знаки
		case static_cast <uint8_t> (error_t::INVALID_KEY):
			// Выводим описание кода ошибки
			return "malformed key name";
		// Если ключ с таким именем уже объявлен
		case static_cast <uint8_t> (error_t::DUPLICATE_KEY):
			// Выводим описание кода ошибки
			return "duplicate key";
		// Если таблица с таким именем уже объявлена
		case static_cast <uint8_t> (error_t::DUPLICATE_TABLE):
			// Выводим описание кода ошибки
			return "duplicate table";
		// Если объявление переопределяет уже заданное значение
		case static_cast <uint8_t> (error_t::REDEFINE_TABLE):
			// Выводим описание кода ошибки
			return "table redefines an existing value";
		// Если встроенная таблица дополняется после её закрытия
		case static_cast <uint8_t> (error_t::EXTEND_INLINE_TABLE):
			// Выводим описание кода ошибки
			return "inline table cannot be extended";
		// Если набор таблиц дополняет имя, набором таблиц не являющееся
		case static_cast <uint8_t> (error_t::APPEND_TO_TABLE):
			// Выводим описание кода ошибки
			return "array of tables appends to a non-array value";
		// Если строка не содержит знака равенства
		case static_cast <uint8_t> (error_t::MISSING_EQUALS):
			// Выводим описание кода ошибки
			return "expected equals sign after key";
		// Если значение за знаком равенства отсутствует
		case static_cast <uint8_t> (error_t::MISSING_VALUE):
			// Выводим описание кода ошибки
			return "expected value after equals sign";
		// Если значение построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_VALUE):
			// Выводим описание кода ошибки
			return "malformed value";
		// Если строковое значение не закрыто кавычкой
		case static_cast <uint8_t> (error_t::UNTERMINATED_STRING):
			// Выводим описание кода ошибки
			return "unterminated string value";
		// Если управляющая последовательность построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_ESCAPE):
			// Выводим описание кода ошибки
			return "malformed escape sequence";
		// Если запись числа построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_NUMBER):
			// Выводим описание кода ошибки
			return "malformed number";
		// Если число выходит за отведённый ему отрезок значений
		case static_cast <uint8_t> (error_t::NUMBER_OVERFLOW):
			// Выводим описание кода ошибки
			return "number is out of range";
		// Если отметка времени построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_DATETIME):
			// Выводим описание кода ошибки
			return "malformed date or time";
		// Если перечень не закрыт квадратной скобкой
		case static_cast <uint8_t> (error_t::UNCLOSED_ARRAY):
			// Выводим описание кода ошибки
			return "unterminated array";
		// Если встроенная таблица не закрыта фигурной скобкой
		case static_cast <uint8_t> (error_t::UNCLOSED_INLINE_TABLE):
			// Выводим описание кода ошибки
			return "unterminated inline table";
		// Если обнаружено содержимое за завершённой записью
		case static_cast <uint8_t> (error_t::UNEXPECTED_CONTENT):
			// Выводим описание кода ошибки
			return "unexpected content after the record";
		// Если длина логической строки превышает допустимую
		case static_cast <uint8_t> (error_t::LINE_TOO_LONG):
			// Выводим описание кода ошибки
			return "line length limit exceeded";
		// Если длина имени ключа превышает допустимую
		case static_cast <uint8_t> (error_t::KEY_TOO_LONG):
			// Выводим описание кода ошибки
			return "key length limit exceeded";
		// Если превышена допустимая глубина вложенности значений
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED):
			// Выводим описание кода ошибки
			return "value nesting depth exceeded";
		// Если превышено допустимое количество частей имени ключа
		case static_cast <uint8_t> (error_t::PARTS_EXCEEDED):
			// Выводим описание кода ошибки
			return "key part count exceeded";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT):
			// Выводим описание кода ошибки
			return "configured parser limit exceeded";
		// Если настройки записи противоречат толкованию читающего
		case static_cast <uint8_t> (error_t::CONFLICTING_SETTINGS):
			// Выводим описание кода ошибки
			return "writer settings conflict with reader interpretation";
	}
	// Выводим описание неизвестного кода ошибки
	return "unknown error";
}
/**
 * @brief Метод получения названия кодировки
 *
 * @param encoding кодировка исходного текста
 * @return         название кодировки
 *
 */
const char * awh::codec::toml::name(const encoding_t encoding) noexcept {
	/**
	 * Определяем кодировку исходного текста
	 */
	switch(static_cast <uint8_t> (encoding)){
		// Если кодировкой является UTF-8
		case static_cast <uint8_t> (encoding_t::UTF8):
			// Выводим название кодировки
			return "UTF-8";
		// Если кодировкой является UTF-16 с обратным порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF16LE):
			// Выводим название кодировки
			return "UTF-16LE";
		// Если кодировкой является UTF-16 с прямым порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF16BE):
			// Выводим название кодировки
			return "UTF-16BE";
	}
	// Выводим название неопределённой кодировки
	return "unknown";
}
/**
 * @brief Метод получения названия типа значения
 *
 * @param type тип значения текста настроек
 * @return     название типа значения
 *
 */
const char * awh::codec::toml::name(const type_t type) noexcept {
	/**
	 * Определяем тип значения текста настроек
	 */
	switch(static_cast <uint8_t> (type)){
		// Если значение является последовательностью знаков
		case static_cast <uint8_t> (type_t::STRING):
			// Выводим название типа значения
			return "string";
		// Если значение является целым числом
		case static_cast <uint8_t> (type_t::INTEGER):
			// Выводим название типа значения
			return "integer";
		// Если значение является числом с плавающей точкой
		case static_cast <uint8_t> (type_t::FLOAT):
			// Выводим название типа значения
			return "float";
		// Если значение является логическим
		case static_cast <uint8_t> (type_t::BOOLEAN):
			// Выводим название типа значения
			return "boolean";
		// Если значение является отметкой времени со смещением часового пояса
		case static_cast <uint8_t> (type_t::OFFSET_DATETIME):
			// Выводим название типа значения
			return "offset date-time";
		// Если значение является отметкой времени без смещения часового пояса
		case static_cast <uint8_t> (type_t::LOCAL_DATETIME):
			// Выводим название типа значения
			return "local date-time";
		// Если значение является местной датой
		case static_cast <uint8_t> (type_t::LOCAL_DATE):
			// Выводим название типа значения
			return "local date";
		// Если значение является местным временем
		case static_cast <uint8_t> (type_t::LOCAL_TIME):
			// Выводим название типа значения
			return "local time";
		// Если значение является перечнем
		case static_cast <uint8_t> (type_t::ARRAY):
			// Выводим название типа значения
			return "array";
		// Если значение является таблицей
		case static_cast <uint8_t> (type_t::TABLE):
			// Выводим название типа значения
			return "table";
	}
	// Выводим название неопределённого типа значения
	return "none";
}
/**
 * @brief Метод получения последовательности знаков конца строки
 *
 * @param newline вид знака конца строки
 * @return        последовательность знаков конца строки
 *
 */
string_view awh::codec::toml::newline(const newline_t newline) noexcept {
	/**
	 * Если знаком конца строки является возврат каретки с переводом строки
	 */
	if(newline == newline_t::CRLF)
		// Выводим последовательность знаков конца строки
		return "\r\n";
	// Выводим последовательность знаков конца строки
	return "\n";
}
/**
 * @brief Метод проверки знака на допустимость в имени ключа без кавычек
 *
 * @param letter проверяемый знак имени
 * @return       результат проверки
 *
 */
bool awh::codec::toml::bare(const char letter) noexcept {
	/**
	 * Выводим результат проверки знака
	 *
	 * @note Набор задан описанием и знаками US-ASCII ограничен: буквы обоих регистров,
	 *       цифры, знак подчёркивания и знак переноса. Привлекать сюда правила местности
	 *       незачем - имя ключа запись протокольная, а не человеческая речь
	 */
	return (ascii::isAlpha(letter) || ascii::isDigit(letter) || (letter == '_') || (letter == '-'));
}
