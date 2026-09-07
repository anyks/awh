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
	/**
	 * Определяем источник сообщения
	 *
	 * @note Имена взяты из RFC 5424, таблица 1, и совпадают с принятыми у служб
	 *       журналов: по ним настраиваются правила отбора, и расхождение сделало бы
	 *       правила несовместимыми
	 */
	switch(static_cast <uint8_t> (facility)){
		// Если сообщение составлено ядром системы
		case static_cast <uint8_t> (facility_t::KERNEL): return "kern";
		// Если сообщение составлено на уровне пользователя
		case static_cast <uint8_t> (facility_t::USER): return "user";
		// Если сообщение составлено почтовой службой
		case static_cast <uint8_t> (facility_t::MAIL): return "mail";
		// Если сообщение составлено службой системы
		case static_cast <uint8_t> (facility_t::DAEMON): return "daemon";
		// Если сообщение составлено службой опознания
		case static_cast <uint8_t> (facility_t::AUTH): return "auth";
		// Если сообщение составлено самой службой журнала
		case static_cast <uint8_t> (facility_t::SYSLOG): return "syslog";
		// Если сообщение составлено службой печати
		case static_cast <uint8_t> (facility_t::PRINTER): return "lpr";
		// Если сообщение составлено службой новостей
		case static_cast <uint8_t> (facility_t::NEWS): return "news";
		// Если сообщение составлено службой UUCP
		case static_cast <uint8_t> (facility_t::UUCP): return "uucp";
		// Если сообщение составлено службой часов
		case static_cast <uint8_t> (facility_t::CLOCK): return "cron";
		// Если сообщение составлено службой опознания, набор второй
		case static_cast <uint8_t> (facility_t::SECURITY): return "authpriv";
		// Если сообщение составлено службой передачи файлов
		case static_cast <uint8_t> (facility_t::FTP): return "ftp";
		// Если сообщение составлено службой сетевого времени
		case static_cast <uint8_t> (facility_t::NTP): return "ntp";
		// Если сообщение есть запись наблюдения за журналом
		case static_cast <uint8_t> (facility_t::AUDIT): return "audit";
		// Если сообщение есть тревога наблюдения за журналом
		case static_cast <uint8_t> (facility_t::ALERT): return "alert";
		// Если сообщение составлено службой часов, набор второй
		case static_cast <uint8_t> (facility_t::CLOCK2): return "cron2";
		// Если сообщение отнесено к местному употреблению, набор нулевой
		case static_cast <uint8_t> (facility_t::LOCAL0): return "local0";
		// Если сообщение отнесено к местному употреблению, набор первый
		case static_cast <uint8_t> (facility_t::LOCAL1): return "local1";
		// Если сообщение отнесено к местному употреблению, набор второй
		case static_cast <uint8_t> (facility_t::LOCAL2): return "local2";
		// Если сообщение отнесено к местному употреблению, набор третий
		case static_cast <uint8_t> (facility_t::LOCAL3): return "local3";
		// Если сообщение отнесено к местному употреблению, набор четвёртый
		case static_cast <uint8_t> (facility_t::LOCAL4): return "local4";
		// Если сообщение отнесено к местному употреблению, набор пятый
		case static_cast <uint8_t> (facility_t::LOCAL5): return "local5";
		// Если сообщение отнесено к местному употреблению, набор шестой
		case static_cast <uint8_t> (facility_t::LOCAL6): return "local6";
		// Если сообщение отнесено к местному употреблению, набор седьмой
		case static_cast <uint8_t> (facility_t::LOCAL7): return "local7";
	}
	// Выводим пустую строку для значения, за предел таблицы выходящего
	return "";
}
/**
 * @brief Метод получения имени степени важности сообщения
 *
 * @param severity степень важности сообщения
 * @return         имя степени важности сообщения
 */
const char * awh::codec::syslog::name(const severity_t severity) noexcept {
	/**
	 * Определяем степень важности сообщения
	 */
	switch(static_cast <uint8_t> (severity)){
		// Если система непригодна к работе
		case static_cast <uint8_t> (severity_t::EMERGENCY): return "emerg";
		// Если вмешательство требуется немедленно
		case static_cast <uint8_t> (severity_t::ALERT): return "alert";
		// Если состояние тяжёлое
		case static_cast <uint8_t> (severity_t::CRITICAL): return "crit";
		// Если наступило условие отказа
		case static_cast <uint8_t> (severity_t::ERROR): return "err";
		// Если наступило условие предостережения
		case static_cast <uint8_t> (severity_t::WARNING): return "warning";
		// Если состояние обычное, но внимания достойное
		case static_cast <uint8_t> (severity_t::NOTICE): return "notice";
		// Если сообщение осведомительное
		case static_cast <uint8_t> (severity_t::INFO): return "info";
		// Если сообщение отладочное
		case static_cast <uint8_t> (severity_t::DEBUG): return "debug";
	}
	// Выводим пустую строку для значения, за предел таблицы выходящего
	return "";
}
