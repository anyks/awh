/**
 * @file common.cpp
 * @date 2026-09-04
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
 * @brief Реализация общих определений контейнера CEF — текстов сообщений об ошибках разбора
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/common.hpp>

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include <sys/macro/suppress.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения текста сообщения об ошибке разбора
 *
 * @param error код ошибки разбора
 * @return      текст сообщения об ошибке разбора
 */
const char * awh::codec::cef::message(const error_t error) noexcept {
	/**
	 * Определяем код ошибки разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE): return "no error";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL): return "internal parsing error";
		// Если текст оборвался посреди записи
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF): return "unexpected end of the text in the middle of a record";
		// Если знак недопустим в записи
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER): return "character is not admissible in a record";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING): return "byte sequence does not correspond to the announced encoding";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING): return "announced encoding is not supported";
		// Если запись не содержит слова «CEF:»
		case static_cast <uint8_t> (error_t::MISSING_SIGNATURE): return "record does not contain the word CEF:";
		// Если номер редакции записи построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_VERSION): return "number of the version of the record is built erroneously";
		// Если номер редакции записи не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_VERSION): return "number of the version of the record is not supported";
		// Если полей заголовка меньше положенного
		case static_cast <uint8_t> (error_t::INCOMPLETE_HEADER): return "header holds fewer fields than required";
		// Если обязательное поле заголовка пусто
		case static_cast <uint8_t> (error_t::EMPTY_HEADER_FIELD): return "obligatory field of the header is empty";
		// Если важность события построена ошибочно либо выходит за предел
		case static_cast <uint8_t> (error_t::INVALID_SEVERITY): return "severity of the event is erroneous or out of the limit";
		// Если пара расширения не содержит знака равенства
		case static_cast <uint8_t> (error_t::MISSING_SEPARATOR): return "pair of the extension does not contain an equals sign";
		// Если имя ключа расширения пусто
		case static_cast <uint8_t> (error_t::EMPTY_KEY): return "name of the key of the extension is empty";
		// Если имя ключа расширения содержит недопустимые знаки
		case static_cast <uint8_t> (error_t::INVALID_KEY): return "name of the key of the extension contains inadmissible characters";
		// Если ключ расширения словарю неизвестен
		case static_cast <uint8_t> (error_t::UNKNOWN_KEY): return "key of the extension is unknown to the dictionary";
		// Если длина имени превышает допустимую
		case static_cast <uint8_t> (error_t::NAME_TOO_LONG): return "length of the name exceeds the admissible one";
		// Если длина поля заголовка превышает допустимую
		case static_cast <uint8_t> (error_t::FIELD_TOO_LONG): return "length of the field of the header exceeds the admissible one";
		// Если длина записи превышает допустимую
		case static_cast <uint8_t> (error_t::RECORD_TOO_LONG): return "length of the record exceeds the admissible one";
		// Если построение отменяющей последовательности ошибочно
		case static_cast <uint8_t> (error_t::INVALID_ESCAPE): return "escaping sequence is built erroneously";
		// Если значение не отвечает виду, словарём заданному
		case static_cast <uint8_t> (error_t::TYPE_MISMATCH): return "value does not correspond to the kind given by the dictionary";
		// Если значение не является адресом сети
		case static_cast <uint8_t> (error_t::INVALID_ADDRESS): return "value is not a network address";
		// Если значение не является меткой времени
		case static_cast <uint8_t> (error_t::INVALID_TIMESTAMP): return "value is not a timestamp";
		// Если значение не является числом
		case static_cast <uint8_t> (error_t::INVALID_NUMBER): return "value is not a number";
		// Если имя поля задано меткой, а самого поля в записи нет
		case static_cast <uint8_t> (error_t::DANGLING_LABEL): return "label gives a name to a field absent from the record";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT): return "limit given by the settings of the parsing is exceeded";
		// Если разбираемый текст не помещается в разрядность хранилища
		case static_cast <uint8_t> (error_t::STORAGE_EXHAUSTED): return "parsed text does not fit into the capacity of the storage";
		// Если поле с таким именем записью не объявлено
		case static_cast <uint8_t> (error_t::UNKNOWN_FIELD): return "field with such a name is not declared by the record";
		// Если значение такого вида запись CEF выразить не может
		case static_cast <uint8_t> (error_t::UNREPRESENTABLE_VALUE): return "value of such a kind cannot be expressed by the CEF notation";
		// Если вложенное значение записи CEF неведомо
		case static_cast <uint8_t> (error_t::NESTED_VALUE): return "nested value is unknown to the CEF notation";
		// Если настройки записи противоречат толкованию читающего
		case static_cast <uint8_t> (error_t::CONFLICTING_SETTINGS): return "settings of the writing contradict the interpretation of the reader";
		// Если файл записей открыть не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_OPENED): return "file of the records could not be opened";
	}
	// Выводим текст неизвестной ошибки
	return "unknown error";
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
