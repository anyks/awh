/**
 * @file common.cpp
 * @date 2026-09-07
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
 * @brief Реализация общих определений контейнера SysLog — текстов сообщений об ошибках разбора,
 *        имён источников сообщения и имён степеней важности
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/syslog/common.hpp>
#include <codec/syslog/dictionary.hpp>

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
const char * awh::codec::syslog::message(const error_t error) noexcept {
	/**
	 * Определяем код ошибки разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE): return "no error";
		// Если запись не открывается приставкой приоритета
		case static_cast <uint8_t> (error_t::MISSING_PRIORITY): return "record does not open with the priority prefix";
		// Если приоритет построен ошибочно либо выходит за предел
		case static_cast <uint8_t> (error_t::INVALID_PRIORITY): return "priority is built erroneously or goes beyond the limit";
		// Если номер описания записи построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_VERSION): return "number of the description of the record is built erroneously";
		// Если номер описания записи не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_VERSION): return "number of the description of the record is not supported";
		// Если полей заголовка меньше положенного
		case static_cast <uint8_t> (error_t::INCOMPLETE_HEADER): return "fields of the header are fewer than required";
		// Если обязательное поле заголовка пусто
		case static_cast <uint8_t> (error_t::EMPTY_HEADER_FIELD): return "mandatory field of the header is empty";
		// Если дата сообщения ни одному из описаний не отвечает
		case static_cast <uint8_t> (error_t::INVALID_TIMESTAMP): return "date of the message answers to none of the descriptions";
		// Если имя узла содержит знак, описанием не дозволенный
		case static_cast <uint8_t> (error_t::INVALID_HOSTNAME): return "name of the host contains a character not allowed by the description";
		// Если опознаватель работы построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_PROCESS): return "identifier of the process is built erroneously";
		// Если скобка структурированных данных не закрыта
		case static_cast <uint8_t> (error_t::UNCLOSED_STRUCTURE): return "bracket of the structured data is not closed";
		// Если опознаватель структурированных данных построен ошибочно
		case static_cast <uint8_t> (error_t::INVALID_STRUCTURE_ID): return "identifier of the structured data is built erroneously";
		// Если имя поля структурированных данных построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_PARAM_NAME): return "name of the field of the structured data is built erroneously";
		// Если значение поля структурированных данных не взято в кавычки
		case static_cast <uint8_t> (error_t::UNQUOTED_PARAM_VALUE): return "value of the field of the structured data is not taken in quotes";
		// Если кавычка значения структурированных данных не закрыта
		case static_cast <uint8_t> (error_t::UNCLOSED_PARAM_VALUE): return "quote of the value of the structured data is not closed";
		// Если опознаватель структурированных данных объявлен дважды
		case static_cast <uint8_t> (error_t::DUPLICATE_STRUCTURE): return "identifier of the structured data is declared twice";
		// Если длина имени превышает допустимую
		case static_cast <uint8_t> (error_t::NAME_TOO_LONG): return "length of the name exceeds the permissible one";
		// Если длина поля заголовка превышает допустимую
		case static_cast <uint8_t> (error_t::FIELD_TOO_LONG): return "length of the field of the header exceeds the permissible one";
		// Если длина записи превышает допустимую
		case static_cast <uint8_t> (error_t::RECORD_TOO_LONG): return "length of the record exceeds the permissible one";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT): return "limit set by the settings of the parsing is exceeded";
		// Если описание записи определить не удалось
		case static_cast <uint8_t> (error_t::UNKNOWN_STANDARD): return "description of the record could not be determined";
		// Если поле с таким именем записью не объявлено
		case static_cast <uint8_t> (error_t::UNKNOWN_FIELD): return "field with such a name is not declared by the record";
		// Если значение такого вида запись SysLog выразить не может
		case static_cast <uint8_t> (error_t::UNREPRESENTABLE_VALUE): return "value of such a kind cannot be expressed by a SysLog record";
		// Если вложенное значение записи SysLog неведомо
		case static_cast <uint8_t> (error_t::NESTED_VALUE): return "nested value is unknown to a SysLog record";
		// Если файл записей открыть не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_OPENED): return "file of the records could not be opened";
	}
	// Выводим описание неизвестного кода отказа
	return "unknown error";
}
/**
 * @brief Метод получения имени источника сообщения
 *
 * @param facility источник сообщения
 * @return         имя источника сообщения
 */
const char * awh::codec::syslog::name(const facility_t facility) noexcept {
	// Выполняем розыск источника сообщения в словаре
	const entry_t * entry = facilities::at(static_cast <uint8_t> (facility));
	/**
	 * Выводим краткое имя источника сообщения
	 *
	 * @note Имена держатся словарём и ТОЛЬКО им: держать их вторым списком тут
	 *       значило бы завести две таблицы, расходящиеся молча при правке одной.
	 *       Запись словаря строится из строкового литерала, и знак конца строки у
	 *       неё есть всегда - выдача указателя на его начало безопасна
	 */
	return ((entry != nullptr) ? entry->name.data() : "");
}

/**
 * @brief Метод получения имени степени важности сообщения
 *
 * @param severity степень важности сообщения
 * @return         имя степени важности сообщения
 */
const char * awh::codec::syslog::name(const severity_t severity) noexcept {
	// Выполняем розыск степени важности сообщения в словаре
	const entry_t * entry = severities::at(static_cast <uint8_t> (severity));
	// Выводим краткое имя степени важности сообщения
	return ((entry != nullptr) ? entry->name.data() : "");
}
