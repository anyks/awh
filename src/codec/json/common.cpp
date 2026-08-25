/**
 * @file common.cpp
 * @date 2026-08-14
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
 * @brief Реализация общих объявлений контейнера JSON — описания кодов отказов, названия
 *        видов узлов, проверка записи числа и разбор необходимости экранирования
 *
 * \~english
 * @brief Implementation of the common declarations of the JSON container — the descriptions of the codes of the refusals, the names
 *        of the kinds of the nodes, the check of the record of a number and the analysis of the necessity of the escaping
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/json/common.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>

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
const char * awh::codec::json::message(const error_t error) noexcept {
	/**
	 * Определяем код отказа разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE):
			return "no error";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL):
			return "internal parser error";
		// Если текст оборвался посреди значения
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF):
			return "unexpected end of text";
		// Если знак недопустим в этом месте текста
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER):
			return "invalid character";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			return "invalid byte sequence for the declared encoding";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING):
			return "unsupported encoding";
		// Если строка не закрыта до конца текста
		case static_cast <uint8_t> (error_t::UNTERMINATED_STRING):
			return "unterminated string";
		// Если отменяющая последовательность не опознана
		case static_cast <uint8_t> (error_t::INVALID_ESCAPE):
			return "invalid escape sequence";
		// Если запись \uXXXX содержит недопустимые знаки
		case static_cast <uint8_t> (error_t::INVALID_UNICODE):
			return "invalid unicode escape";
		// Если суррогат не образует пары
		case static_cast <uint8_t> (error_t::UNPAIRED_SURROGATE):
			return "unpaired surrogate";
		// Если управляющий знак стоит внутри строки без экранирования
		case static_cast <uint8_t> (error_t::CONTROL_IN_STRING):
			return "unescaped control character in string";
		// Если запись числа не отвечает стандарту
		case static_cast <uint8_t> (error_t::INVALID_NUMBER):
			return "invalid number";
		// Если число не представимо затребованным видом
		case static_cast <uint8_t> (error_t::NUMBER_OUT_OF_RANGE):
			return "number out of range";
		// Если вместо true, false либо null стоит иное
		case static_cast <uint8_t> (error_t::INVALID_LITERAL):
			return "invalid literal";
		// Если за окончанием документа стоят знаки
		case static_cast <uint8_t> (error_t::TRAILING_CHARACTERS):
			return "trailing characters after document";
		// Если ожидалось значение
		case static_cast <uint8_t> (error_t::EXPECTED_VALUE):
			return "value expected";
		// Если ожидалось имя поля объекта
		case static_cast <uint8_t> (error_t::EXPECTED_KEY):
			return "object key expected";
		// Если ожидалось двоеточие после имени поля
		case static_cast <uint8_t> (error_t::EXPECTED_COLON):
			return "colon expected";
		// Если ожидалась запятая либо закрывающая скобка
		case static_cast <uint8_t> (error_t::EXPECTED_COMMA):
			return "comma or closing bracket expected";
		// Если запятая стоит перед закрывающей скобкой при строгом разборе
		case static_cast <uint8_t> (error_t::TRAILING_COMMA):
			return "trailing comma";
		// Если имя поля объекта объявлено повторно
		case static_cast <uint8_t> (error_t::DUPLICATE_KEY):
			return "duplicate object key";
		// Если глубина вложенности превышает допустимую
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED):
			return "maximum nesting depth exceeded";
		// Если длина строкового значения превышает допустимую
		case static_cast <uint8_t> (error_t::STRING_TOO_LONG):
			return "string is too long";
		// Если длина записи числа превышает допустимую
		case static_cast <uint8_t> (error_t::NUMBER_TOO_LONG):
			return "number is too long";
		// Если количество узлов документа превышает допустимое
		case static_cast <uint8_t> (error_t::TOO_MANY_NODES):
			return "too many nodes";
		// Если примечание встречено при строгом разборе
		case static_cast <uint8_t> (error_t::COMMENT_NOT_ALLOWED):
			return "comments are not allowed";
		// Если примечание не закрыто до конца текста
		case static_cast <uint8_t> (error_t::UNTERMINATED_COMMENT):
			return "unterminated comment";
		// Если текст пуст, а документ затребован
		case static_cast <uint8_t> (error_t::EMPTY_TEXT):
			return "empty text";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT):
			return "parser limit exceeded";
		// Если файл документа открыть не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_OPENED):
			return "cannot open the document file";
		// Если подача продолжена после объявленного конца текста
		case static_cast <uint8_t> (error_t::TEXT_ALREADY_ENDED):
			return "feeding continued after the text was declared complete";
		// Если текст документа записать в файл не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_WRITTEN):
			return "cannot write the document file";
	}
	// Выводим описание неизвестного кода отказа
	return "unknown error";
}
/**
 * @brief Функция получения названия вида узла
 *
 * @param kind вид узла документа
 * @return     название вида узла
 *
 */
const char * awh::codec::json::name(const kind_t kind) noexcept {
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
			return "boolean";
		// Если узел является числом
		case static_cast <uint8_t> (kind_t::NUMBER):
			return "number";
		// Если узел является строкой
		case static_cast <uint8_t> (kind_t::STRING):
			return "string";
		// Если узел является массивом
		case static_cast <uint8_t> (kind_t::ARRAY):
			return "array";
		// Если узел является объектом
		case static_cast <uint8_t> (kind_t::OBJECT):
			return "object";
	}
	// Выводим название неизвестного вида узла
	return "unknown";
}
/**
 * @brief Функция получения названия вида значения
 *
 * @param type вид значения документа
 * @return     название вида значения
 *
 */
const char * awh::codec::json::name(const type_t type) noexcept {
	/**
	 * Определяем вид значения документа
	 */
	switch(static_cast <uint16_t> (type)){
		// Если значения нет вовсе
		case static_cast <uint16_t> (type_t::UNDEFINED):
			return "undefined";
		// Если значение является пустым
		case static_cast <uint16_t> (type_t::NUL):
			return "null";
		// Если значение является логическим
		case static_cast <uint16_t> (type_t::BOOL):
			return "boolean";
		// Если значение является строкой
		case static_cast <uint16_t> (type_t::STRING):
			return "string";
		// Если значение является массивом
		case static_cast <uint16_t> (type_t::ARRAY):
			return "array";
		// Если значение является объектом
		case static_cast <uint16_t> (type_t::OBJECT):
			return "object";
		// Если значение является целым со знаком шириною в один байт
		case static_cast <uint16_t> (type_t::INT8):
			return "int8";
		// Если значение является целым со знаком шириною в два байта
		case static_cast <uint16_t> (type_t::INT16):
			return "int16";
		// Если значение является целым со знаком шириною в четыре байта
		case static_cast <uint16_t> (type_t::INT32):
			return "int32";
		// Если значение является целым со знаком шириною в восемь байтов
		case static_cast <uint16_t> (type_t::INT64):
			return "int64";
		// Если значение является целым без знака шириною в один байт
		case static_cast <uint16_t> (type_t::UINT8):
			return "uint8";
		// Если значение является целым без знака шириною в два байта
		case static_cast <uint16_t> (type_t::UINT16):
			return "uint16";
		// Если значение является целым без знака шириною в четыре байта
		case static_cast <uint16_t> (type_t::UINT32):
			return "uint32";
		// Если значение является целым без знака шириною в восемь байтов
		case static_cast <uint16_t> (type_t::UINT64):
			return "uint64";
		// Если значение является дробным одинарной точности
		case static_cast <uint16_t> (type_t::FLOAT):
			return "float";
		// Если значение является дробным двойной точности
		case static_cast <uint16_t> (type_t::DOUBLE):
			return "double";
		// Если значение является числом, не вместимым ни в один родной вид
		case static_cast <uint16_t> (type_t::EXTENDED):
			return "extended";
	}
	// Выводим название неизвестного вида значения
	return "unknown";
}
/**
 * @brief Функция получения вида узла по виду значения
 *
 * @param type вид значения документа
 * @return     вид узла документа
 *
 */
awh::codec::json::kind_t awh::codec::json::kind(const type_t type) noexcept {
	// Получаем разряды вида значения
	const uint16_t mask = static_cast <uint16_t> (type);
	/**
	 * Если значение является числом любого вида
	 *
	 * @note Число проверяется первым: чисел видов больше всех прочих вместе взятых,
	 *       и разбор их - самая частая работа документа
	 */
	if(mask & static_cast <uint16_t> (type_t::NUMBER))
		// Выводим вид узла числа
		return kind_t::NUMBER;
	/**
	 * Определяем вид значения документа
	 */
	switch(mask){
		// Если значение является пустым
		case static_cast <uint16_t> (type_t::NUL):
			return kind_t::NUL;
		// Если значение является логическим
		case static_cast <uint16_t> (type_t::BOOL):
			return kind_t::BOOL;
		// Если значение является строкой
		case static_cast <uint16_t> (type_t::STRING):
			return kind_t::STRING;
		// Если значение является массивом
		case static_cast <uint16_t> (type_t::ARRAY):
			return kind_t::ARRAY;
		// Если значение является объектом
		case static_cast <uint16_t> (type_t::OBJECT):
			return kind_t::OBJECT;
	}
	// Выводим отсутствие вида узла
	return kind_t::NONE;
}
/**
 * @brief Функция проверки записи числа на соответствие стандарту
 *
 * @details Проверка ведётся тем же порядком, каким её ведёт разбор, и намеренно
 *          повторяет его строгость: ведущий нуль, ведущий плюс, точка без цифры
 *          после неё и порядок без цифр стандартом запрещены
 *
 * @param text проверяемая запись числа
 * @return     признак соответствия записи стандарту
 *
 */
bool awh::codec::json::numeric(const string & text) noexcept {
	/**
	 * Если запись числа пуста
	 */
	if(text.empty())
		// Выводим признак несоответствия записи стандарту
		return false;
	// Положение разбираемого знака в записи числа
	size_t index = 0;
	// Размер разбираемой записи числа
	const size_t size = text.size();
	/**
	 * Если запись числа начинается со знака минуса
	 */
	if(text[index] == '-')
		// Выполняем переход к следующему знаку записи числа
		index++;
	/**
	 * Если за знаком минуса не осталось знаков
	 */
	if(index >= size)
		// Выводим признак несоответствия записи стандарту
		return false;
	/**
	 * Если целая часть числа начинается с нуля
	 */
	if(text[index] == '0')
		// Выполняем переход к следующему знаку записи числа
		index++;
	/**
	 * Если целая часть числа начинается с цифры
	 */
	else if((text[index] >= '1') && (text[index] <= '9')) {
		/**
		 * Выполняем разбор цифр целой части числа
		 */
		while((index < size) && (text[index] >= '0') && (text[index] <= '9'))
			// Выполняем переход к следующему знаку записи числа
			index++;
	/**
	 * Если целая часть числа отсутствует
	 */
	} else
		// Выводим признак несоответствия записи стандарту
		return false;
	/**
	 * Если за целой частью числа следует точка
	 */
	if((index < size) && (text[index] == '.')){
		// Выполняем переход к следующему знаку записи числа
		index++;
		/**
		 * Если за точкой не следует цифра
		 */
		if((index >= size) || (text[index] < '0') || (text[index] > '9'))
			// Выводим признак несоответствия записи стандарту
			return false;
		/**
		 * Выполняем разбор цифр дробной части числа
		 */
		while((index < size) && (text[index] >= '0') && (text[index] <= '9'))
			// Выполняем переход к следующему знаку записи числа
			index++;
	}
	/**
	 * Если за числом следует буква порядка
	 */
	if((index < size) && ((text[index] == 'e') || (text[index] == 'E'))){
		// Выполняем переход к следующему знаку записи числа
		index++;
		/**
		 * Если за буквой порядка следует знак
		 */
		if((index < size) && ((text[index] == '+') || (text[index] == '-')))
			// Выполняем переход к следующему знаку записи числа
			index++;
		/**
		 * Если за буквой порядка не следует цифра
		 */
		if((index >= size) || (text[index] < '0') || (text[index] > '9'))
			// Выводим признак несоответствия записи стандарту
			return false;
		/**
		 * Выполняем разбор цифр порядка числа
		 */
		while((index < size) && (text[index] >= '0') && (text[index] <= '9'))
			// Выполняем переход к следующему знаку записи числа
			index++;
	}
	// Выводим признак соответствия записи стандарту
	return (index == size);
}
/**
 * @brief Функция проверки необходимости экранирования строки
 *
 * @param text   проверяемое значение
 * @param escape правило экранирования при записи
 * @return       признак необходимости экранирования
 *
 */
bool awh::codec::json::escapable(const string & text, const escape_t escape) noexcept {
	/**
	 * Выполняем перебор всех знаков проверяемого значения
	 */
	for(size_t i = 0; i < text.size(); i++){
		// Получаем очередной байт проверяемого значения
		const uint8_t letter = static_cast <uint8_t> (text[i]);
		/**
		 * Если знак экранируется по предписанию стандарта
		 */
		if((letter < 0x20) || (letter == '"') || (letter == '\\'))
			// Выводим признак необходимости экранирования
			return true;
		/**
		 * Если экранируется косая черта, а знак ею и является
		 */
		if((escape != escape_t::MINIMAL) && (letter == '/'))
			// Выводим признак необходимости экранирования
			return true;
		/**
		 * Если знак вне US-ASCII записывается кодовым значением
		 */
		if((escape == escape_t::ASCII) && (letter > 0x7F))
			// Выводим признак необходимости экранирования
			return true;
	}
	// Выводим признак отсутствия необходимости экранирования
	return false;
}
