/**
 * @file common.cpp
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
 * @brief Файл реализации общих объявлений бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the common declarations of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/common.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <limits>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция получения текстового описания кода отказа
 *
 * @param error код отказа разбора
 * @return      текстовое описание кода отказа
 *
 */
const char * awh::codec::abc::message(const error_t error) noexcept {
	/**
	 * Определяем код отказа разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE):
			return "no error";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL):
			return "internal parsing error";
		// Если запись оборвалась посреди значения
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF):
			return "record has ended in the middle of a value";
		// Если метка не опознана
		case static_cast <uint8_t> (error_t::UNKNOWN_TAG):
			return "tag is not recognised";
		// Если метка отведена под будущее
		case static_cast <uint8_t> (error_t::RESERVED_TAG):
			return "tag is reserved for the future";
		// Если объявленная длина недопустима
		case static_cast <uint8_t> (error_t::INVALID_LENGTH):
			return "declared length is inadmissible";
		// Если объявленная длина превышает остаток записи
		case static_cast <uint8_t> (error_t::LENGTH_OVERFLOW):
			return "declared length exceeds the remainder of the record";
		// Если строка не отвечает кодировке UTF-8
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			return "string does not conform to the UTF-8 encoding";
		// Если число не представимо затребованным видом
		case static_cast <uint8_t> (error_t::NUMBER_OUT_OF_RANGE):
			return "number is not representable by the demanded kind";
		// Если запись числа неограниченной ширины повреждена
		case static_cast <uint8_t> (error_t::INVALID_BIGNUM):
			return "record of a number of an unlimited width is corrupted";
		// Если запись десятичного числа повреждена
		case static_cast <uint8_t> (error_t::INVALID_DECIMAL):
			return "record of a decimal number is corrupted";
		// Если конец вместимого встречен вне вместимого неопределённой длины
		case static_cast <uint8_t> (error_t::UNBALANCED_BREAK):
			return "end of a container is met outside of a container of an indefinite length";
		// Если отображение оборвалось на имени
		case static_cast <uint8_t> (error_t::MISSING_VALUE):
			return "mapping has ended on a name, there is no value after it";
		// Если имя поля отображения объявлено повторно
		case static_cast <uint8_t> (error_t::DUPLICATE_KEY):
			return "name of a field of a mapping is declared repeatedly";
		// Если глубина вложенности превышает допустимую
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED):
			return "depth of the nesting exceeds the admissible one";
		// Если длина строкового значения превышает допустимую
		case static_cast <uint8_t> (error_t::STRING_TOO_LONG):
			return "length of a string value exceeds the admissible one";
		// Если длина двоичного значения превышает допустимую
		case static_cast <uint8_t> (error_t::BLOB_TOO_LONG):
			return "length of a binary value exceeds the admissible one";
		// Если количество узлов документа превышает допустимое
		case static_cast <uint8_t> (error_t::TOO_MANY_NODES):
			return "number of the nodes of the document exceeds the admissible one";
		// Если за окончанием документа стоят октеты
		case static_cast <uint8_t> (error_t::TRAILING_OCTETS):
			return "octets after the end of the document";
		// Если запись пуста, а документ затребован
		case static_cast <uint8_t> (error_t::EMPTY_RECORD):
			return "record is empty while a document is demanded";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT):
			return "limit set by the settings of the parsing is exceeded";
		// Если именем поля отображения стоит вместимое
		case static_cast <uint8_t> (error_t::INVALID_KEY):
			return "a container stands as the name of a field of a mapping";
		// Если имена полей отображения идут не по возрастанию
		case static_cast <uint8_t> (error_t::UNORDERED_KEY):
			return "names of the fields of a mapping do not go in the ascending order";
		// Если неопределённая длина встречена при строгом виде записи
		case static_cast <uint8_t> (error_t::INDEFINITE_REFUSED):
			return "an indefinite length at the strict kind of the record";
		// Если вместимое не закрыто либо закрыто лишний раз
		case static_cast <uint8_t> (error_t::UNBALANCED_CONTAINER):
			return "container is not closed or is closed once too many";
		// Если значений вместимого больше объявленного
		case static_cast <uint8_t> (error_t::CONTAINER_OVERFLOW):
			return "there are more values of a container than declared";
	}
	// Выводим результат по умолчанию
	return "unknown error";
}
/**
 * @brief Функция получения названия вида узла
 *
 * @param kind вид узла документа
 * @return     название вида узла
 *
 */
const char * awh::codec::abc::name(const kind_t kind) noexcept {
	/**
	 * Определяем вид узла документа
	 */
	switch(static_cast <uint8_t> (kind)){
		// Если узел не определён
		case static_cast <uint8_t> (kind_t::NONE):
			return "none";
		// Если узел является пустым значением
		case static_cast <uint8_t> (kind_t::NUL):
			return "null";
		// Если узел является логическим значением
		case static_cast <uint8_t> (kind_t::BOOL):
			return "bool";
		// Если узел является числом
		case static_cast <uint8_t> (kind_t::NUMBER):
			return "number";
		// Если узел является строкой
		case static_cast <uint8_t> (kind_t::STRING):
			return "string";
		// Если узел является двоичными данными
		case static_cast <uint8_t> (kind_t::BLOB):
			return "blob";
		// Если узел является отметкой времени
		case static_cast <uint8_t> (kind_t::TIME):
			return "time";
		// Если узел является опознавателем
		case static_cast <uint8_t> (kind_t::UUID):
			return "uuid";
		// Если узел является массивом
		case static_cast <uint8_t> (kind_t::ARRAY):
			return "array";
		// Если узел является отображением
		case static_cast <uint8_t> (kind_t::MAP):
			return "map";
	}
	// Выводим результат по умолчанию
	return "none";
}
/**
 * @brief Функция получения названия вида значения
 *
 * @param type вид значения документа
 * @return     название вида значения
 *
 */
const char * awh::codec::abc::name(const type_t type) noexcept {
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint32_t> (type)){
		// Если значения нет вовсе
		case static_cast <uint32_t> (type_t::UNDEFINED):
			return "undefined";
		// Если значение является пустым
		case static_cast <uint32_t> (type_t::NUL):
			return "null";
		// Если значение является логическим
		case static_cast <uint32_t> (type_t::BOOL):
			return "bool";
		// Если значение является строкой
		case static_cast <uint32_t> (type_t::STRING):
			return "string";
		// Если значение является двоичными данными
		case static_cast <uint32_t> (type_t::BLOB):
			return "blob";
		// Если значение является массивом
		case static_cast <uint32_t> (type_t::ARRAY):
			return "array";
		// Если значение является отображением
		case static_cast <uint32_t> (type_t::MAP):
			return "map";
		// Если значение является отметкой времени
		case static_cast <uint32_t> (type_t::TIME):
			return "time";
		// Если значение является опознавателем
		case static_cast <uint32_t> (type_t::UUID):
			return "uuid";
		// Если значение является целым со знаком шириною в один октет
		case static_cast <uint32_t> (type_t::INT8):
			return "int8";
		// Если значение является целым со знаком шириною в два октета
		case static_cast <uint32_t> (type_t::INT16):
			return "int16";
		// Если значение является целым со знаком шириною в четыре октета
		case static_cast <uint32_t> (type_t::INT32):
			return "int32";
		// Если значение является целым со знаком шириною в восемь октетов
		case static_cast <uint32_t> (type_t::INT64):
			return "int64";
		// Если значение является целым без знака шириною в один октет
		case static_cast <uint32_t> (type_t::UINT8):
			return "uint8";
		// Если значение является целым без знака шириною в два октета
		case static_cast <uint32_t> (type_t::UINT16):
			return "uint16";
		// Если значение является целым без знака шириною в четыре октета
		case static_cast <uint32_t> (type_t::UINT32):
			return "uint32";
		// Если значение является целым без знака шириною в восемь октетов
		case static_cast <uint32_t> (type_t::UINT64):
			return "uint64";
		// Если значение является дробным одинарной точности
		case static_cast <uint32_t> (type_t::FLOAT):
			return "float";
		// Если значение является дробным двойной точности
		case static_cast <uint32_t> (type_t::DOUBLE):
			return "double";
		// Если значение является целым, не вместимым ни в один родной вид
		case static_cast <uint32_t> (type_t::EXTENDED):
			return "extended";
		// Если значение является десятичным
		case static_cast <uint32_t> (type_t::DECIMAL):
			return "decimal";
	}
	// Выводим результат по умолчанию
	return "undefined";
}
/**
 * @brief Функция получения вида узла по виду значения
 *
 * @param type вид значения документа
 * @return     вид узла документа
 *
 */
awh::codec::abc::kind_t awh::codec::abc::kind(const type_t type) noexcept {
	// Если вид значения является числом любого вида
	if(static_cast <uint32_t> (type) & static_cast <uint32_t> (type_t::NUMBER))
		// Выводим вид узла числа
		return kind_t::NUMBER;
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint32_t> (type)){
		// Если значение является пустым
		case static_cast <uint32_t> (type_t::NUL):
			return kind_t::NUL;
		// Если значение является логическим
		case static_cast <uint32_t> (type_t::BOOL):
			return kind_t::BOOL;
		// Если значение является строкой
		case static_cast <uint32_t> (type_t::STRING):
			return kind_t::STRING;
		// Если значение является двоичными данными
		case static_cast <uint32_t> (type_t::BLOB):
			return kind_t::BLOB;
		// Если значение является отметкой времени
		case static_cast <uint32_t> (type_t::TIME):
			return kind_t::TIME;
		// Если значение является опознавателем
		case static_cast <uint32_t> (type_t::UUID):
			return kind_t::UUID;
		// Если значение является массивом
		case static_cast <uint32_t> (type_t::ARRAY):
			return kind_t::ARRAY;
		// Если значение является отображением
		case static_cast <uint32_t> (type_t::MAP):
			return kind_t::MAP;
	}
	// Выводим результат по умолчанию
	return kind_t::NONE;
}
/**
 * @brief Функция сборки ведущего октета значения
 *
 * @param major  крупный вид проволочной записи
 * @param detail подробность метки
 * @return       собранный ведущий октет
 *
 */
uint8_t awh::codec::abc::tag(const major_t major, const uint8_t detail) noexcept {
	// Выполняем сборку ведущего октета, укладывая крупный вид в три старших разряда
	return static_cast <uint8_t> ((static_cast <uint8_t> (major) << 5) | (detail & 0x1F));
}
/**
 * @brief Функция извлечения крупного вида из ведущего октета
 *
 * @param tag ведущий октет значения
 * @return    крупный вид проволочной записи
 *
 */
awh::codec::abc::major_t awh::codec::abc::major(const uint8_t tag) noexcept {
	// Выводим крупный вид, снятый с трёх старших разрядов ведущего октета
	return static_cast <major_t> (tag >> 5);
}
/**
 * @brief Функция извлечения подробности из ведущего октета
 *
 * @param tag ведущий октет значения
 * @return    подробность метки
 *
 */
uint8_t awh::codec::abc::detail(const uint8_t tag) noexcept {
	// Выводим подробность, снятую с пяти младших разрядов ведущего октета
	return static_cast <uint8_t> (tag & 0x1F);
}
/**
 * @brief Функция получения ширины записи, ведомой подробностью метки
 *
 * @param detail подробность метки
 * @param result ширина ведомой записи в октетах
 * @return       признак опознания подробности
 *
 */
bool awh::codec::abc::width(const uint8_t detail, uint8_t & result) noexcept {
	// Выполняем сброс ширины ведомой записи
	result = 0;
	// Если подробность несёт само значение
	if(detail <= INLINE_LIMIT)
		// Сообщаем, что ведомой записи за меткой нет
		return true;
	/**
	 * Определяем подробность метки
	 */
	switch(detail){
		// Если за меткой стоит один октет
		case (INLINE_LIMIT + 1): {
			// Выполняем установку ширины ведомой записи
			result = 1;
			// Сообщаем, что подробность опознана
			return true;
		}
		// Если за меткой стоят два октета
		case (INLINE_LIMIT + 2): {
			// Выполняем установку ширины ведомой записи
			result = 2;
			// Сообщаем, что подробность опознана
			return true;
		}
		// Если за меткой стоят четыре октета
		case (INLINE_LIMIT + 3): {
			// Выполняем установку ширины ведомой записи
			result = 4;
			// Сообщаем, что подробность опознана
			return true;
		}
		// Если за меткой стоят восемь октетов
		case (INLINE_LIMIT + 4): {
			// Выполняем установку ширины ведомой записи
			result = 8;
			// Сообщаем, что подробность опознана
			return true;
		}
	}
	// Сообщаем, что подробность ширины не ведёт
	return false;
}
/**
 * @brief Функция получения наименьшей подробности, вмещающей значение
 *
 * @param value укладываемое значение
 * @return      подробность метки
 *
 */
uint8_t awh::codec::abc::fit(const uint64_t value) noexcept {
	// Если значение укладывается в саму метку
	if(value <= static_cast <uint64_t> (INLINE_LIMIT))
		// Выводим значение подробностью метки
		return static_cast <uint8_t> (value);
	// Если значение укладывается в один октет
	else if(value <= static_cast <uint64_t> (numeric_limits <uint8_t>::max()))
		// Выводим подробность записи шириною в один октет
		return static_cast <uint8_t> (INLINE_LIMIT + 1);
	// Если значение укладывается в два октета
	else if(value <= static_cast <uint64_t> (numeric_limits <uint16_t>::max()))
		// Выводим подробность записи шириною в два октета
		return static_cast <uint8_t> (INLINE_LIMIT + 2);
	// Если значение укладывается в четыре октета
	else if(value <= static_cast <uint64_t> (numeric_limits <uint32_t>::max()))
		// Выводим подробность записи шириною в четыре октета
		return static_cast <uint8_t> (INLINE_LIMIT + 3);
	// Выводим подробность записи шириною в восемь октетов
	return static_cast <uint8_t> (INLINE_LIMIT + 4);
}
