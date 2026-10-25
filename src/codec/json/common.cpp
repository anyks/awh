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
 * @brief Реализация общих объявлений контейнера JSON — описания кодов отказов, названия
 *        видов узлов, проверка записи числа и разбор необходимости экранирования
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/json/common.hpp>
#include <codec/json/encoding.hpp>

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
	 *
	 * @note Сличение ведётся по САМОМУ перечню, а не по приведённому к байту значению,
	 *       и ветви `default` здесь нет нарочно: так собиратель сам сообщает о коде,
	 *       описания не получившем, - предупреждением `-Wswitch` с именем этого кода.
	 *       Приведение к байту, стоявшее здесь прежде, защиту эту снимало вовсе, и коды,
	 *       дописанные в перечень, оставались без описания молча. Проверено щупом:
	 *       дописанный код вызывает предупреждение по имени
	 *
	 * @warning Возвращать приведение нельзя: сторожем тут выступает собиратель, а не
	 *          проверка. Проверка описаний перечень перебрать не может - о том, что код
	 *          объявлен, ей узнать неоткуда
	 *
	 * Определяем код отказа разбора
	 *
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE):
			// Выводим текстовое описание кода отказа
			return "no error";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL):
			// Выводим текстовое описание кода отказа
			return "internal parser error";
		// Если текст оборвался посреди значения
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF):
			// Выводим текстовое описание кода отказа
			return "unexpected end of text";
		// Если знак недопустим в этом месте текста
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER):
			// Выводим текстовое описание кода отказа
			return "invalid character";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			// Выводим текстовое описание кода отказа
			return "invalid byte sequence for the declared encoding";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING):
			// Выводим текстовое описание кода отказа
			return "unsupported encoding";
		// Если строка не закрыта до конца текста
		case static_cast <uint8_t> (error_t::UNTERMINATED_STRING):
			// Выводим текстовое описание кода отказа
			return "unterminated string";
		// Если отменяющая последовательность не опознана
		case static_cast <uint8_t> (error_t::INVALID_ESCAPE):
			// Выводим текстовое описание кода отказа
			return "invalid escape sequence";
		// Если запись \uXXXX содержит недопустимые знаки
		case static_cast <uint8_t> (error_t::INVALID_UNICODE):
			// Выводим текстовое описание кода отказа
			return "invalid unicode escape";
		// Если суррогат не образует пары
		case static_cast <uint8_t> (error_t::UNPAIRED_SURROGATE):
			// Выводим текстовое описание кода отказа
			return "unpaired surrogate";
		// Если управляющий знак стоит внутри строки без экранирования
		case static_cast <uint8_t> (error_t::CONTROL_IN_STRING):
			// Выводим текстовое описание кода отказа
			return "unescaped control character in string";
		// Если запись числа не отвечает стандарту
		case static_cast <uint8_t> (error_t::INVALID_NUMBER):
			// Выводим текстовое описание кода отказа
			return "invalid number";
		// Если значение установленными настройками записи непредставимо
		case static_cast <uint8_t> (error_t::UNWRITABLE_VALUE):
			// Выводим текстовое описание кода отказа
			return "value is not representable by the writing settings";
		// Если число не представимо затребованным видом
		case static_cast <uint8_t> (error_t::NUMBER_OUT_OF_RANGE):
			// Выводим текстовое описание кода отказа
			return "number out of range";
		// Если вместо true, false либо null стоит иное
		case static_cast <uint8_t> (error_t::INVALID_LITERAL):
			// Выводим текстовое описание кода отказа
			return "invalid literal";
		// Если за окончанием документа стоят знаки
		case static_cast <uint8_t> (error_t::TRAILING_CHARACTERS):
			// Выводим текстовое описание кода отказа
			return "trailing characters after document";
		// Если ожидалось значение
		case static_cast <uint8_t> (error_t::EXPECTED_VALUE):
			// Выводим текстовое описание кода отказа
			return "value expected";
		// Если ожидалось имя поля объекта
		case static_cast <uint8_t> (error_t::EXPECTED_KEY):
			// Выводим текстовое описание кода отказа
			return "object key expected";
		// Если ожидалось двоеточие после имени поля
		case static_cast <uint8_t> (error_t::EXPECTED_COLON):
			// Выводим текстовое описание кода отказа
			return "colon expected";
		// Если ожидалась запятая либо закрывающая скобка
		case static_cast <uint8_t> (error_t::EXPECTED_COMMA):
			// Выводим текстовое описание кода отказа
			return "comma or closing bracket expected";
		// Если запятая стоит перед закрывающей скобкой при строгом разборе
		case static_cast <uint8_t> (error_t::TRAILING_COMMA):
			// Выводим текстовое описание кода отказа
			return "trailing comma";
		// Если имя поля объекта объявлено повторно
		case static_cast <uint8_t> (error_t::DUPLICATE_KEY):
			// Выводим текстовое описание кода отказа
			return "duplicate object key";
		// Если глубина вложенности превышает допустимую
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED):
			// Выводим текстовое описание кода отказа
			return "maximum nesting depth exceeded";
		// Если длина строкового значения превышает допустимую
		case static_cast <uint8_t> (error_t::STRING_TOO_LONG):
			// Выводим текстовое описание кода отказа
			return "string is too long";
		// Если длина записи числа превышает допустимую
		case static_cast <uint8_t> (error_t::NUMBER_TOO_LONG):
			// Выводим текстовое описание кода отказа
			return "number is too long";
		// Если количество узлов документа превышает допустимое
		case static_cast <uint8_t> (error_t::TOO_MANY_NODES):
			// Выводим текстовое описание кода отказа
			return "too many nodes";
		// Если примечание встречено при строгом разборе
		case static_cast <uint8_t> (error_t::COMMENT_NOT_ALLOWED):
			// Выводим текстовое описание кода отказа
			return "comments are not allowed";
		// Если примечание не закрыто до конца текста
		case static_cast <uint8_t> (error_t::UNTERMINATED_COMMENT):
			// Выводим текстовое описание кода отказа
			return "unterminated comment";
		// Если текст пуст, а документ затребован
		case static_cast <uint8_t> (error_t::EMPTY_TEXT):
			// Выводим текстовое описание кода отказа
			return "empty text";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT):
			// Выводим текстовое описание кода отказа
			return "parser limit exceeded";
		// Если файл документа открыть не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_OPENED):
			// Выводим текстовое описание кода отказа
			return "cannot open the document file";
		// Если подача продолжена после объявленного конца текста
		case static_cast <uint8_t> (error_t::TEXT_ALREADY_ENDED):
			// Выводим текстовое описание кода отказа
			return "feeding continued after the text was declared complete";
		// Если текст документа записать в файл не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_WRITTEN):
			// Выводим текстовое описание кода отказа
			return "cannot write the document file";
		// Если файл документа прочитать не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_READ):
			// Выводим текстовое описание кода отказа
			return "cannot read the document file";
		// Если кодировка сменена посреди подачи текста
		case static_cast <uint8_t> (error_t::ENCODING_ALREADY_CHOSEN):
			// Выводим текстовое описание кода отказа
			return "the encoding cannot be changed in the middle of the feed";
		// Если разбираемый текст не помещается в разрядность хранилища
		case static_cast <uint8_t> (error_t::STORAGE_EXHAUSTED):
			// Выводим текстовое описание кода отказа
			return "the text does not fit the width of the parser storage";
		// Если массив не закрыт до конца текста
		case static_cast <uint8_t> (error_t::UNCLOSED_ARRAY):
			// Выводим текстовое описание кода отказа
			return "unclosed array";
		// Если объект не закрыт до конца текста
		case static_cast <uint8_t> (error_t::UNCLOSED_OBJECT):
			// Выводим текстовое описание кода отказа
			return "unclosed object";
		// Если корень документа уже несёт значение
		case static_cast <uint8_t> (error_t::MULTIPLE_ROOTS):
			// Выводим текстовое описание кода отказа
			return "the document root already holds a value";
		// Если ни одно вместилище не открыто
		case static_cast <uint8_t> (error_t::NO_CONTAINER_OPEN):
			// Выводим текстовое описание кода отказа
			return "no container is open";
		// Если имя поля записано вне объекта
		case static_cast <uint8_t> (error_t::KEY_OUTSIDE_OBJECT):
			// Выводим текстовое описание кода отказа
			return "a field name is allowed only inside an object and only once";
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
			// Выводим название вида узла
			return "none";
		// Если узел является пустым значением
		case static_cast <uint8_t> (kind_t::NIL):
			// Выводим название вида узла
			return "null";
		// Если узел является логическим значением
		case static_cast <uint8_t> (kind_t::BOOL):
			// Выводим название вида узла
			return "boolean";
		// Если узел является числом
		case static_cast <uint8_t> (kind_t::NUMBER):
			// Выводим название вида узла
			return "number";
		// Если узел является строкой
		case static_cast <uint8_t> (kind_t::STRING):
			// Выводим название вида узла
			return "string";
		// Если узел является массивом
		case static_cast <uint8_t> (kind_t::ARRAY):
			// Выводим название вида узла
			return "array";
		// Если узел является объектом
		case static_cast <uint8_t> (kind_t::OBJECT):
			// Выводим название вида узла
			return "object";
	}
	// Выводим название неизвестного вида узла
	return "unknown";
}
/**
 * @brief Функция получения названия кодировки исходного текста
 *
 * @param encoding кодировка исходного текста
 * @return         название кодировки
 *
 */
const char * awh::codec::json::name(const encoding_t encoding) noexcept {
	/**
	 * Определяем кодировку исходного текста
	 *
	 * @warning Приведение кодировки к числу здесь ставить нельзя: оно лишает собиратель
	 *          возможности видеть перечень, и кодировка, в него дописанная, ушла бы БЕЗ
	 *          названия молча. Ветвь `default` глушит сторожа тем же порядком
	 *
	 */
	switch(static_cast <uint8_t> (encoding)){
		// Если кодировка не определена
		case static_cast <uint8_t> (encoding_t::NONE):
			// Выводим название кодировки
			return "none";
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
		// Если кодировкой является ISO-8859-1
		case static_cast <uint8_t> (encoding_t::LATIN1):
			// Выводим название кодировки
			return "ISO-8859-1";
		// Если кодировкой является US-ASCII
		case static_cast <uint8_t> (encoding_t::ASCII):
			// Выводим название кодировки
			return "US-ASCII";
		// Если кодировкой является Windows-1252
		case static_cast <uint8_t> (encoding_t::CP1252):
			// Выводим название кодировки
			return "windows-1252";
		// Если кодировкой является UTF-32 с обратным порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF32LE):
			// Выводим название кодировки
			return "UTF-32LE";
		// Если кодировкой является UTF-32 с прямым порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF32BE):
			// Выводим название кодировки
			return "UTF-32BE";
		// Если кодировка однобайтовая, взятая таблицею
		case static_cast <uint8_t> (encoding_t::SINGLE):
			// Выводим обозначение вида кодировки
			return "single-byte";
	}
	// Выводим название неизвестной кодировки
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
	switch(static_cast <uint8_t> (type)){
		// Если значения нет вовсе
		case static_cast <uint16_t> (type_t::UNDEFINED):
			// Выводим название вида значения
			return "undefined";
		// Если значение является пустым
		case static_cast <uint16_t> (type_t::NIL):
			// Выводим название вида значения
			return "null";
		// Если значение является логическим
		case static_cast <uint16_t> (type_t::BOOL):
			// Выводим название вида значения
			return "boolean";
		// Если значение является строкой
		case static_cast <uint16_t> (type_t::STRING):
			// Выводим название вида значения
			return "string";
		// Если значение является массивом
		case static_cast <uint16_t> (type_t::ARRAY):
			// Выводим название вида значения
			return "array";
		// Если значение является объектом
		case static_cast <uint16_t> (type_t::OBJECT):
			// Выводим название вида значения
			return "object";
		// Если значение является целым со знаком шириною в один байт
		case static_cast <uint16_t> (type_t::INT8):
			// Выводим название вида значения
			return "int8";
		// Если значение является целым со знаком шириною в два байта
		case static_cast <uint16_t> (type_t::INT16):
			// Выводим название вида значения
			return "int16";
		// Если значение является целым со знаком шириною в четыре байта
		case static_cast <uint16_t> (type_t::INT32):
			// Выводим название вида значения
			return "int32";
		// Если значение является целым со знаком шириною в восемь байтов
		case static_cast <uint16_t> (type_t::INT64):
			// Выводим название вида значения
			return "int64";
		// Если значение является целым без знака шириною в один байт
		case static_cast <uint16_t> (type_t::UINT8):
			// Выводим название вида значения
			return "uint8";
		// Если значение является целым без знака шириною в два байта
		case static_cast <uint16_t> (type_t::UINT16):
			// Выводим название вида значения
			return "uint16";
		// Если значение является целым без знака шириною в четыре байта
		case static_cast <uint16_t> (type_t::UINT32):
			// Выводим название вида значения
			return "uint32";
		// Если значение является целым без знака шириною в восемь байтов
		case static_cast <uint16_t> (type_t::UINT64):
			// Выводим название вида значения
			return "uint64";
		// Если значение является дробным одинарной точности
		case static_cast <uint16_t> (type_t::FLOAT):
			// Выводим название вида значения
			return "float";
		// Если значение является дробным двойной точности
		case static_cast <uint16_t> (type_t::DOUBLE):
			// Выводим название вида значения
			return "double";
		// Если значение является числом, не вместимым ни в один родной вид
		case static_cast <uint16_t> (type_t::EXTENDED):
			// Выводим название вида значения
			return "extended";
		/**
		 * Если видом является составное имя, вид собою не называющее
		 *
		 * @note Перечень этот - набор РАЗРЯДОВ, и составные имена собирают по нескольку
		 *       разрядов сразу. Видом хранения отдельного значения они не бывают никогда,
		 *       а имени своего не имеют - выдаётся им название неизвестного вида, ровно
		 *       как было и до перечисления
		 *
		 * @warning Перечислены они НАМЕРЕННО вместо `default`: ветвь `default` глушит
		 *          `-Wswitch`, и вид, в перечень дописанный, ушёл бы БЕЗ названия молча.
		 *          Приведение вида к числу глушит сторожа тем же порядком - возвращать
		 *          его нельзя
		 *
		 */
		case static_cast <uint16_t> (type_t::INT):
		case static_cast <uint16_t> (type_t::REAL):
		case static_cast <uint16_t> (type_t::NUMBER):
		case static_cast <uint16_t> (type_t::SIGNED):
		case static_cast <uint16_t> (type_t::UNSIGNED): break;
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
	 *
	 */
	if(mask & static_cast <uint16_t> (type_t::NUMBER))
		// Выводим вид узла числа
		return kind_t::NUMBER;
	/**
	 * Определяем вид значения документа
	 */
	switch(mask){
		// Если значение является пустым
		case static_cast <uint16_t> (type_t::NIL):
			// Выводим вид узла документа
			return kind_t::NIL;
		// Если значение является логическим
		case static_cast <uint16_t> (type_t::BOOL):
			// Выводим вид узла документа
			return kind_t::BOOL;
		// Если значение является строкой
		case static_cast <uint16_t> (type_t::STRING):
			// Выводим вид узла документа
			return kind_t::STRING;
		// Если значение является массивом
		case static_cast <uint16_t> (type_t::ARRAY):
			// Выводим вид узла документа
			return kind_t::ARRAY;
		// Если значение является объектом
		case static_cast <uint16_t> (type_t::OBJECT):
			// Выводим вид узла документа
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
 *          после неё и порядок без цифр стандартом запрещены.
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
		 * Если байт начинает последовательность UTF-8
		 *
		 * @details Годность последовательности судится ТЕМ ЖЕ `decode()`, каким судит её
		 *          запись: негодную она при умолчательном обращении `REPLACE` перезаписывает знаком
		 *          замены, и дословный перенос отрезка на ней рвётся. Прежде проверка эта о негодных
		 *          последовательностях не знала вовсе и разрешала переносить дословно текст, какой
		 *          запись перезаписывает, - щуп дал восемь таких расхождений на пятнадцати строках.
		 *
		 * @note При обращении `PASS` байты уходят как есть, и перенос был бы допустим; ответ
		 *       здесь строже нужного намеренно: осторожность стоит упущенного отрезка, а
		 *       обратное стоило бы негодных байтов в готовом тексте
		 *
		 */
		if(letter > 0x7F){
			// Кодовая точка разбираемой последовательности
			uint32_t code = 0;
			// Признак годности разобранной последовательности
			bool valid = false;
			// Выполняем разбор очередной последовательности
			const size_t length = decode(text, i, code, valid);
			/**
			 * Если последовательность негодна
			 */
			if(!valid)
				// Выводим признак необходимости экранирования
				return true;
			/**
			 * Если знак вне US-ASCII записывается кодовым значением
			 */
			if(escape == escape_t::ASCII)
				// Выводим признак необходимости экранирования
				return true;
			// Пропускаем разобранную последовательность целиком
			i += (length - 1);
			// Переходим к следующему знаку проверяемого значения
			continue;
		}
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
	}
	// Выводим признак отсутствия необходимости экранирования
	return false;
}

/**
 * @brief Конструктор
 *
 */
awh::codec::json::Location::Location() noexcept : depth(0) {}
