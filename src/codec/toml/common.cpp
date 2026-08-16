/**
 * @file common.cpp
 * @date 2026-08-12
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
 * @brief Реализация общих определений контейнера TOML — описания кодов ошибок разбора,
 *        названия кодировок исходного текста и типов значений, знаки конца строки и
 *        признак знака, допустимого в имени ключа без кавычек
 *
 * @copyright Copyright © 2026
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
/**
 * @brief Метод проверки знака Юникода на допустимость в имени ключа без кавычек
 *
 * @param code проверяемый знак имени
 * @return     результат проверки
 *
 */
bool awh::codec::toml::named(const uint32_t code) noexcept {
	/**
	 * Границы наборов знаков Юникода, дозволенных имени без кавычек
	 *
	 * @note Набор взят из черновика следующей версии описания дословно: складывать его
	 *       по разрядам Юникода нельзя - разряды эти от издания к изданию пополняются,
	 *       и имя, законное сегодня, завтра читалось бы иначе
	 */
	static const uint32_t BOUNDS[][2] = {
		{0x0000B2, 0x0000B3}, {0x0000B9, 0x0000B9}, {0x0000BC, 0x0000BE},
		{0x0000C0, 0x0000D6}, {0x0000D8, 0x0000F6}, {0x0000F8, 0x00037D},
		{0x00037F, 0x001FFF}, {0x00200C, 0x00200D}, {0x00203F, 0x002040},
		{0x002070, 0x00218F}, {0x002460, 0x0024FF}, {0x002C00, 0x002FEF},
		{0x003001, 0x00D7FF}, {0x00F900, 0x00FDCF}, {0x00FDF0, 0x00FFFD},
		{0x010000, 0x0EFFFF}
	};
	/**
	 * Выполняем перебор всех наборов знаков Юникода
	 */
	for(auto & bounds : BOUNDS){
		/**
		 * Если знак имени в набор укладывается
		 */
		if((code >= bounds[0]) && (code <= bounds[1]))
			// Выводим признак допустимости знака имени
			return true;
	}
	// Выводим признак недопустимости знака имени
	return false;
}
/**
 * @brief Метод получения длины последовательности UTF-8 по её ведущему байту
 *
 * @param leading ведущий байт последовательности знака
 * @return        количество байт последовательности, ноль - байт ведущим не является
 *
 */
size_t awh::codec::toml::sequence(const uint8_t leading) noexcept {
	/**
	 * Если знак записан одним байтом
	 */
	if(leading < 0x80)
		// Выводим количество байт последовательности знака
		return 1;
	/**
	 * Если знак записан двумя байтами
	 */
	else if((leading & 0xE0) == 0xC0)
		// Выводим количество байт последовательности знака
		return 2;
	/**
	 * Если знак записан тремя байтами
	 */
	else if((leading & 0xF0) == 0xE0)
		// Выводим количество байт последовательности знака
		return 3;
	/**
	 * Если знак записан четырьмя байтами
	 */
	else if((leading & 0xF8) == 0xF0)
		// Выводим количество байт последовательности знака
		return 4;
	// Выводим признак ошибочного построения последовательности
	return 0;
}
/**
 * @brief Метод разбора очередной последовательности UTF-8
 *
 * @param text   последовательность знаков, из которой ведётся чтение
 * @param offset положение начала знака в последовательности
 * @param code   получаемый знак Юникода, при неудачном разборе не выставляется
 * @param length количество байт, разбором пройденных, при нехватке байт нулевое
 * @return       исход разбора последовательности
 *
 */
awh::codec::toml::utf8_t awh::codec::toml::inspect(const string_view text, const size_t offset, uint32_t & code, size_t & length) noexcept {
	// Выполняем сброс количества пройденных разбором байт
	length = 0;
	/**
	 * Если читать знак неоткуда
	 */
	if(offset >= text.length())
		// Выводим признак нехватки байт последовательности
		return utf8_t::TRUNCATED;
	// Получаем ведущий байт знака Юникода
	const uint8_t leading = static_cast <uint8_t> (text[offset]);
	// Получаем количество байт последовательности знака Юникода
	const size_t count = sequence(leading);
	/**
	 * Если ведущий байт построен ошибочно
	 */
	if(count == 0){
		// Запоминаем количество байт, разбором пройденных
		length = 1;
		// Выводим признак ошибочного построения последовательности
		return utf8_t::BROKEN;
	}
	/**
	 * Если знак записан одним байтом
	 */
	if(count == 1){
		// Запоминаем знак Юникода
		code = leading;
		// Запоминаем количество байт, разбором пройденных
		length = 1;
		// Выводим признак успешного разбора последовательности
		return utf8_t::VALID;
	}
	/**
	 * Если байтов знака в последовательности не хватает
	 */
	if((offset + count) > text.length())
		// Выводим признак нехватки байт последовательности
		return utf8_t::TRUNCATED;
	// Разряды знака Юникода, ведущим байтом заданные
	static constexpr uint32_t LEADING[] = {0x00, 0x00, 0x1F, 0x0F, 0x07};
	// Запоминаем разряды знака, ведущим байтом заданные
	uint32_t result = (leading & LEADING[count]);
	/**
	 * Выполняем перебор всех продолжающих байт знака Юникода
	 */
	for(size_t i = 1; i < count; i++){
		// Получаем очередной продолжающий байт знака Юникода
		const uint8_t letter = static_cast <uint8_t> (text[offset + i]);
		/**
		 * Если байт продолжающим не является
		 */
		if((letter & 0xC0) != 0x80){
			// Запоминаем количество байт, разбором пройденных
			length = i;
			// Выводим признак ошибочного построения последовательности
			return utf8_t::BROKEN;
		}
		// Выполняем добавление разрядов знака, байтом заданных
		result = ((result << 6) | (letter & 0x3F));
	}
	// Запоминаем количество байт, разбором пройденных
	length = count;
	/**
	 * Наименьшее кодовое значение, записи данной длины отведённое
	 *
	 * @note Запись длиннее необходимой - знак иной, нежели тот, что она изображает:
	 *       признать её значило бы принять два написания одного имени
	 */
	static constexpr uint32_t MINIMAL[] = {0x000000, 0x000000, 0x000080, 0x000800, 0x010000};
	/**
	 * Если знак записан длиннее необходимого либо в набор Юникода не укладывается
	 */
	if((result < MINIMAL[count]) || (result > MAX_CODEPOINT) || ((result >= 0xD800) && (result <= 0xDFFF)))
		// Выводим признак ошибочного построения последовательности
		return utf8_t::BROKEN;
	// Запоминаем прочитанный знак Юникода
	code = result;
	// Выводим признак успешного разбора последовательности
	return utf8_t::VALID;
}
/**
 * @brief Метод получения очередного знака Юникода последовательности UTF-8
 *
 * @param text   последовательность знаков, из которой ведётся чтение
 * @param offset положение начала знака в последовательности
 * @param code   получаемый знак Юникода
 * @return       количество байт знака, ноль - знак битый либо оборванный
 *
 */
size_t awh::codec::toml::decode(const string_view text, const size_t offset, uint32_t & code) noexcept {
	// Количество байт, разбором пройденных
	size_t length = 0;
	/**
	 * Если разобрать очередную последовательность не удалось
	 */
	if(inspect(text, offset, code, length) != utf8_t::VALID)
		// Выводим количество байт битого знака
		return 0;
	// Выводим количество байт знака Юникода
	return length;
}
/**
 * @brief Метод проверки даты на существование её в календаре
 *
 * @param year  проверяемый год
 * @param month проверяемый месяц
 * @param day   проверяемый день месяца
 * @return      результат проверки
 *
 */
bool awh::codec::toml::calendar(const uint16_t year, const uint8_t month, const uint8_t day) noexcept {
	/**
	 * Если месяц за пределы года выходит
	 */
	if((month < 1) || (month > 12))
		// Выводим отрицательный результат проверки
		return false;
	/**
	 * Количество дней в месяцах года
	 *
	 * @note Февралю отведено двадцать девять дней, а год невисокосный отсекается
	 *       отдельно: держать две таблицы ради одного месяца незачем
	 */
	static const uint8_t DAYS[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	/**
	 * Если день месяца за пределы месяца выходит
	 */
	if((day < 1) || (day > DAYS[month - 1]))
		// Выводим отрицательный результат проверки
		return false;
	/**
	 * Если днём является двадцать девятое февраля года невисокосного
	 */
	if((month == 2) && (day == 29) && (((year % 4) != 0) || (((year % 100) == 0) && ((year % 400) != 0))))
		// Выводим отрицательный результат проверки
		return false;
	// Выводим положительный результат проверки
	return true;
}
