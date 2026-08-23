/**
 * @file common.cpp
 * @date 2026-08-09
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
 * @brief Реализация общих определений контейнера INI — описания кодов ошибок разбора,
 *        названия кодировок исходного текста и их определение, признаки знаков наречия,
 *        сличение имён разделов и разбор значений свойств числом
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <limits>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <num/lexical/lexical.hpp>
#include <codec/ini/common.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Шаблонная функция приведения дробного числа к затребованному виду
	 *
	 * @details Приведение это перенято у контейнера JSON дословно: договор извлечения
	 * общий у всех кодеков рамки, и расхождение в нём всплыло бы у потребителя,
	 * читающего два из них, - всплыло бы не сразу и не там, где заведено
	 *
	 * @note Отказа приведение не даёт ни при каком числе: признак успешности разбора
	 *       отведён одному лишь случаю, когда значение числом не является вовсе
	 *
	 * @tparam T     вид, к какому приводится число
	 * @param  value приводимое дробное число
	 * @return       приведённое число
	 *
	 */
	template <typename T>
	T convert(const double value) noexcept {
		/**
		 * Если затребован дробный вид
		 */
		if(std::is_floating_point <T>::value)
			// Выводим приведённое число как оно есть
			return static_cast <T> (value);
		/**
		 * Если число не является числом вовсе
		 *
		 * @note Приведение `NaN` к целому есть неопределённое поведение при любом пределе:
		 *       x86-64 выдаёт наименьшее целое, ARM64 - ноль. Выдаётся ноль, и выдаётся он
		 *       успехом, а не отказом
		 */
		if(::isnan(value))
			// Выводим нулевое число
			return static_cast <T> (0);
		/**
		 * Если целая часть числа лежит ниже предела затребованного вида
		 *
		 * @note Пределы сличаются дробным видом, а не целым: предел `int64_t` целым видом
		 *       точно не представим дробным, и сличение целых дало бы промах на единицу
		 */
		if(value <= static_cast <double> (std::numeric_limits <T>::lowest()))
			// Выводим нижний предел затребованного вида
			return std::numeric_limits <T>::lowest();
		/**
		 * Если целая часть числа лежит выше предела затребованного вида
		 */
		if(value >= static_cast <double> (std::numeric_limits <T>::max()))
			// Выводим верхний предел затребованного вида
			return std::numeric_limits <T>::max();
		/**
		 * Выводим приведённое число, округлённое по правилам математики
		 *
		 * @note Округление здесь, а не усечение, решено владельцем 20.08.2026: усечение
		 *       выдавало единицу за `1.5`, тогда как ближайшее целое ей - двойка. Половина
		 *       уводится от нуля, оттого `-1.5` выдаётся как `-2`, а не как `-1`
		 */
		return static_cast <T> (::round(value));
	}

	/**
	 * @brief Метод сличения последовательностей знаков без учёта регистра
	 *
	 * @details Сличение ведётся по правилам US-ASCII: прочие знаки сличаются как есть.
	 * Названия кодировок и записи логического значения знаками US-ASCII ограничены, и
	 * привлекать сюда правила местности незачем
	 *
	 * @param first  первая последовательность знаков для сличения
	 * @param second вторая последовательность знаков для сличения
	 * @return       результат сличения
	 *
	 */
	static bool compare(const string_view first, const string_view second) noexcept {
		/**
		 * Если длины последовательностей знаков не совпадают
		 */
		if(first.length() != second.length())
			// Выводим отрицательный результат сличения
			return false;
		/**
		 * Выполняем перебор всех знаков последовательности
		 */
		for(size_t i = 0; i < first.length(); i++){
			/**
			 * Если очередные знаки не совпадают
			 */
			if(!ascii::equals(first[i], second[i]))
				// Выводим отрицательный результат сличения
				return false;
		}
		// Выводим положительный результат сличения
		return true;
	}
	/**
	 * @brief Метод отбрасывания пробельной обвязки значения свойства
	 *
	 * @details Пробельными считаются пробел, знак табуляции, возврат каретки и перевод
	 * строки. Обвязка в значении появляется сама собой при записи с выравниванием, и
	 * отбрасывать её приходится всегда
	 *
	 * @param text обрабатываемое значение свойства
	 * @return     значение без пробельной обвязки по краям
	 *
	 */
	static string_view trim(const string_view text) noexcept {
		// Начало значащей части значения
		size_t begin = 0;
		// Конец значащей части значения
		size_t end = text.length();
		/**
		 * Выполняем отбрасывание пробельных знаков в начале значения
		 */
		while((begin < end) && ascii::isSpace(text[begin]))
			// Выполняем переход к следующему знаку значения
			begin++;
		/**
		 * Выполняем отбрасывание пробельных знаков в конце значения
		 */
		while((end > begin) && ascii::isSpace(text[end - 1]))
			// Выполняем переход к предыдущему знаку значения
			end--;
		// Выводим значение без пробельной обвязки
		return text.substr(begin, end - begin);
	}
};

/**
 * @brief Метод проверки совпадения имени
 *
 * @param section    имя раздела для сличения
 * @param subsection имя подраздела для сличения
 * @return           результат проверки
 *
 */
bool awh::codec::ini::Name::is(const string_view section, const string_view subsection) const noexcept {
	// Выполняем сличение имени раздела и имени подраздела
	return ((this->section.compare(section) == 0) && (this->subsection.compare(subsection) == 0));
}
/**
 * @brief Оператор сравнения
 *
 * @param name имя для сравнения
 * @return     результат сравнения
 *
 */
bool awh::codec::ini::Name::operator == (const Name & name) const noexcept {
	// Выполняем сличение по имени раздела и имени подраздела
	return this->is(name.section, name.subsection);
}
/**
 * @brief Оператор сравнения
 *
 * @param name имя для сравнения
 * @return     результат сравнения
 *
 */
bool awh::codec::ini::Name::operator != (const Name & name) const noexcept {
	// Выводим обратный результат сличения имён
	return !this->is(name.section, name.subsection);
}
/**
 * @brief Метод получения описания кода ошибки разбора
 *
 * @param error код ошибки разбора
 * @return      описание кода ошибки на английском языке
 *
 */
const char * awh::codec::ini::message(const error_t error) noexcept {
	/**
	 * Определяем код ошибки разбора
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
			return "unexpected end of input";
		// Если знак недопустим в тексте настроек
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER):
			// Выводим описание кода ошибки
			return "invalid character";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			// Выводим описание кода ошибки
			return "invalid byte sequence for the declared encoding";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING):
			// Выводим описание кода ошибки
			return "unsupported encoding";
		// Если объявление раздела построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_SECTION):
			// Выводим описание кода ошибки
			return "malformed section header";
		// Если объявление раздела не закрыто квадратной скобкой
		case static_cast <uint8_t> (error_t::UNCLOSED_SECTION):
			// Выводим описание кода ошибки
			return "unclosed section header";
		// Если имя раздела пусто
		case static_cast <uint8_t> (error_t::EMPTY_SECTION):
			// Выводим описание кода ошибки
			return "empty section name";
		// Если раздел с таким именем уже объявлен
		case static_cast <uint8_t> (error_t::DUPLICATE_SECTION):
			// Выводим описание кода ошибки
			return "duplicate section";
		// Если имя подраздела построено ошибочно
		case static_cast <uint8_t> (error_t::INVALID_SUBSECTION):
			// Выводим описание кода ошибки
			return "malformed subsection name";
		// Если имя свойства содержит недопустимые знаки
		case static_cast <uint8_t> (error_t::INVALID_KEY):
			// Выводим описание кода ошибки
			return "invalid property name";
		// Если имя свойства пусто
		case static_cast <uint8_t> (error_t::EMPTY_KEY):
			// Выводим описание кода ошибки
			return "empty property name";
		// Если свойство с таким именем в разделе уже объявлено
		case static_cast <uint8_t> (error_t::DUPLICATE_KEY):
			// Выводим описание кода ошибки
			return "duplicate property";
		// Если длина имени превышает допустимую
		case static_cast <uint8_t> (error_t::NAME_TOO_LONG):
			// Выводим описание кода ошибки
			return "name is too long";
		// Если строка не содержит разделителя имени и значения
		case static_cast <uint8_t> (error_t::MISSING_SEPARATOR):
			// Выводим описание кода ошибки
			return "missing name and value separator";
		// Если свойство объявлено до первого раздела
		case static_cast <uint8_t> (error_t::KEY_OUTSIDE_SECTION):
			// Выводим описание кода ошибки
			return "property outside of any section";
		// Если значение в кавычках не закрыто до конца строки
		case static_cast <uint8_t> (error_t::UNTERMINATED_QUOTE):
			// Выводим описание кода ошибки
			return "unterminated quoted value";
		// Если управляющая последовательность построена ошибочно
		case static_cast <uint8_t> (error_t::INVALID_ESCAPE):
			// Выводим описание кода ошибки
			return "invalid escape sequence";
		// Если за закрывающей скобкой объявления раздела обнаружено содержимое
		case static_cast <uint8_t> (error_t::UNEXPECTED_CONTENT):
			// Выводим описание кода ошибки
			return "unexpected content after section header";
		// Если длина логической строки превышает допустимую
		case static_cast <uint8_t> (error_t::LINE_TOO_LONG):
			// Выводим описание кода ошибки
			return "line is too long";
		// Если превышена допустимая глубина вложенности подразделов
		case static_cast <uint8_t> (error_t::DEPTH_EXCEEDED):
			// Выводим описание кода ошибки
			return "subsection depth exceeded";
		// Если превышено допустимое количество строк продолжения
		case static_cast <uint8_t> (error_t::CONTINUATION_EXCEEDED):
			// Выводим описание кода ошибки
			return "line continuation limit exceeded";
		// Если обнаружено обращение к необъявленному значению
		case static_cast <uint8_t> (error_t::UNKNOWN_REFERENCE):
			// Выводим описание кода ошибки
			return "reference to an undefined property";
		// Если значение ссылается само на себя
		case static_cast <uint8_t> (error_t::RECURSIVE_REFERENCE):
			// Выводим описание кода ошибки
			return "recursive property reference";
		// Если превышена допустимая глубина вложенности обращений
		case static_cast <uint8_t> (error_t::REFERENCE_DEPTH):
			// Выводим описание кода ошибки
			return "property reference depth exceeded";
		// Если превышен допустимый объём подстановки значений
		case static_cast <uint8_t> (error_t::EXPANSION_EXCEEDED):
			// Выводим описание кода ошибки
			return "property expansion limit exceeded";
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
 * @return         общепринятое название кодировки
 *
 */
const char * awh::codec::ini::name(const encoding_t encoding) noexcept {
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
			return "WINDOWS-1252";
	}
	// Выводим название неопределённой кодировки
	return "";
}
/**
 * @brief Метод определения кодировки по её названию
 *
 * @param text название кодировки в любом регистре
 * @return     определённая кодировка исходного текста
 *
 */
awh::codec::ini::encoding_t awh::codec::ini::encoding(const string_view text) noexcept {
	/**
	 * Если название кодировки не передано
	 */
	if(text.empty())
		// Выводим неопределённую кодировку
		return encoding_t::NONE;
	/**
	 * Если кодировкой является UTF-8
	 */
	if(::compare(text, "UTF-8") || ::compare(text, "UTF8"))
		// Выводим определённую кодировку
		return encoding_t::UTF8;
	/**
	 * Если кодировкой является UTF-16 с обратным порядком байтов
	 */
	if(::compare(text, "UTF-16LE") || ::compare(text, "UTF16LE"))
		// Выводим определённую кодировку
		return encoding_t::UTF16LE;
	/**
	 * Если кодировкой является UTF-16 с прямым порядком байтов
	 */
	if(::compare(text, "UTF-16BE") || ::compare(text, "UTF16BE"))
		// Выводим определённую кодировку
		return encoding_t::UTF16BE;
	/**
	 * Если кодировка объявлена как UTF-16 без указания порядка байтов
	 *
	 * @note Порядок байтов в таком случае определяется меткой в начале текста,
	 *       поэтому название разбирается в кодировку с прямым порядком лишь как
	 *       основание по умолчанию
	 */
	if(::compare(text, "UTF-16") || ::compare(text, "UTF16"))
		// Выводим определённую кодировку
		return encoding_t::UTF16BE;
	/**
	 * Если кодировкой является ISO-8859-1
	 *
	 */
	if(::compare(text, "ISO-8859-1") || ::compare(text, "ISO8859-1") || ::compare(text, "LATIN1") || ::compare(text, "L1"))
		// Выводим определённую кодировку
		return encoding_t::LATIN1;
	/**
	 * Если кодировкой является Windows-1252
	 *
	 * @note Кодировка эта с ISO-8859-1 не совпадает и отдельной ветвью разбирается
	 *       намеренно: там, где у ISO-8859-1 управляющие знаки области C1, у неё
	 *       знаки печатные - денежный знак евро, кавычки-лапки, тире. Разбирать её
	 *       как ISO-8859-1 значило бы отвечать отказом на всякий текст настроек
	 *       MS Windows, где эти знаки есть
	 */
	if(::compare(text, "WINDOWS-1252") || ::compare(text, "CP1252") || ::compare(text, "WINDOWS1252"))
		// Выводим определённую кодировку
		return encoding_t::CP1252;
	/**
	 * Если кодировкой является US-ASCII
	 */
	if(::compare(text, "US-ASCII") || ::compare(text, "ASCII") || ::compare(text, "ANSI_X3.4-1968"))
		// Выводим определённую кодировку
		return encoding_t::ASCII;
	// Выводим неопределённую кодировку
	return encoding_t::NONE;
}
/**
 * @brief Метод проверки знака на признак начала примечания
 *
 * @param letter проверяемый знак
 * @param marker признаваемые знаки начала примечания
 * @return       результат проверки
 *
 */
bool awh::codec::ini::commented(const char letter, const marker_t marker) noexcept {
	/**
	 * Определяем признаваемые знаки начала примечания
	 */
	switch(static_cast <uint8_t> (marker)){
		// Если примечание начинает точка с запятой
		case static_cast <uint8_t> (marker_t::SEMICOLON):
			// Выводим результат проверки знака
			return (letter == ';');
		// Если примечание начинает знак решётки
		case static_cast <uint8_t> (marker_t::HASH):
			// Выводим результат проверки знака
			return (letter == '#');
		// Если примечание начинает любой из двух знаков
		case static_cast <uint8_t> (marker_t::BOTH):
			// Выводим результат проверки знака
			return ((letter == ';') || (letter == '#'));
	}
	// Выводим отрицательный результат проверки знака
	return false;
}
/**
 * @brief Метод проверки знака на признак разделителя имени и значения
 *
 * @param letter    проверяемый знак
 * @param separator признаваемые знаки разделителя имени и значения
 * @return          результат проверки
 *
 */
bool awh::codec::ini::separated(const char letter, const separator_t separator) noexcept {
	/**
	 * Определяем признаваемые знаки разделителя имени и значения
	 */
	switch(static_cast <uint8_t> (separator)){
		// Если имя и значение разделяет знак равенства
		case static_cast <uint8_t> (separator_t::EQUALS):
			// Выводим результат проверки знака
			return (letter == '=');
		// Если имя и значение разделяет двоеточие
		case static_cast <uint8_t> (separator_t::COLON):
			// Выводим результат проверки знака
			return (letter == ':');
		// Если имя и значение разделяет любой из двух знаков
		case static_cast <uint8_t> (separator_t::BOTH):
			// Выводим результат проверки знака
			return ((letter == '=') || (letter == ':'));
	}
	// Выводим отрицательный результат проверки знака
	return false;
}
/**
 * @brief Метод получения знака конца строки
 *
 * @param newline вид знака конца строки
 * @return        последовательность знаков конца строки
 *
 */
string_view awh::codec::ini::newline(const newline_t newline) noexcept {
	/**
	 * Определяем вид знака конца строки
	 */
	switch(static_cast <uint8_t> (newline)){
		// Если знаком конца строки является возврат каретки с переводом строки
		case static_cast <uint8_t> (newline_t::CRLF):
			// Выводим последовательность знаков конца строки
			return "\r\n";
		// Если знаком конца строки является одиночный возврат каретки
		case static_cast <uint8_t> (newline_t::CR):
			// Выводим последовательность знаков конца строки
			return "\r";
	}
	// Выводим последовательность знаков конца строки
	return "\n";
}
/**
 * @brief Метод разбора целого числа со знаком из значения свойства
 *
 * @param text   разбираемое значение свойства
 * @param result ссылка на результат разбора
 * @return       признак успешного разбора
 *
 */
bool awh::codec::ini::integer(const string_view text, int64_t & result) noexcept {
	// Получаем значение без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если разбирать нечего
	 */
	if(value.empty())
		// Выводим признак неудачного разбора
		return false;
	// Выполняем разбор целого числа со знаком
	const lexical_t::result_t <char> res = lexical_t::fromChars(value.data(), value.data() + value.length(), result);
	/**
	 * Выводим признак успешного разбора, если число разобрано целиком
	 *
	 * @note Остаток за числом отвергается намеренно: «52abc» числом не является,
	 *       и приведение такого значения к 52 скрыло бы ошибку в файле настроек
	 */
	return (static_cast <bool> (res) && (res.ptr == (value.data() + value.length())));
}
/**
 * @brief Метод разбора целого числа без знака из значения свойства
 *
 * @param text   разбираемое значение свойства
 * @param result ссылка на результат разбора
 * @return       признак успешного разбора
 *
 */
bool awh::codec::ini::integer(const string_view text, uint64_t & result) noexcept {
	// Получаем значение без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если разбирать нечего
	 */
	if(value.empty())
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Если значение записано со знаком числа
	 *
	 * @note Число со знаком в тип без знака не приводится даже тогда, когда знак
	 *       положительный: запрошенный тип и есть указание на ожидаемую запись
	 */
	if((value.front() == '-') || (value.front() == '+'))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем разбор целого числа без знака
	const lexical_t::result_t <char> res = lexical_t::fromChars(value.data(), value.data() + value.length(), result);
	// Выводим признак успешного разбора, если число разобрано целиком
	return (static_cast <bool> (res) && (res.ptr == (value.data() + value.length())));
}
/**
 * @brief Метод разбора числа с плавающей точкой из значения свойства
 *
 * @param text   разбираемое значение свойства
 * @param result ссылка на результат разбора
 * @return       признак успешного разбора
 *
 */
bool awh::codec::ini::real(const string_view text, double & result) noexcept {
	// Получаем значение без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если разбирать нечего
	 */
	if(value.empty())
		// Выводим признак неудачного разбора
		return false;
	// Выполняем разбор числа с плавающей точкой
	const lexical_t::result_t <char> res = lexical_t::fromChars(value.data(), value.data() + value.length(), result);
	/**
	 * Если число за предел вида вышло
	 *
	 * @note Отказ по этому коду снят: «1e400» есть число, и разбор его удался - выдана
	 *       бесконечность, а «1e-400» выдано нулём. Отвергать их значило бы разойтись с
	 *       прочими кодеками рамки, где такая запись извлекается пределом. Прочие коды
	 *       отказа остаются отказом: ими помечена запись, числом не являющаяся
	 */
	if(res.ec == errc::result_out_of_range)
		// Выводим признак успешного разбора, если число разобрано целиком
		return (res.ptr == (value.data() + value.length()));
	// Выводим признак успешного разбора, если число разобрано целиком
	return (static_cast <bool> (res) && (res.ptr == (value.data() + value.length())));
}
/**
 * @brief Метод разбора логического значения из значения свойства
 *
 * @param text   разбираемое значение свойства
 * @param result ссылка на результат разбора
 * @param forms  признаваемая запись логического значения
 * @return       признак успешного разбора
 *
 */
bool awh::codec::ini::boolean(const string_view text, bool & result, const boolean_t forms) noexcept {
	// Получаем значение без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если разбирать нечего
	 */
	if(value.empty())
		// Выводим признак неудачного разбора
		return false;
	/**
	 * Если запрошена строгая запись логического значения
	 *
	 * @note Регистр записи при строгом разборе учитывается намеренно: строгой она
	 *       названа по договору XSD, а он иных написаний не допускает
	 */
	if(forms == boolean_t::STRICT){
		/**
		 * Если значением является истина
		 */
		if((value.compare("true") == 0) || (value.compare("1") == 0)){
			// Запоминаем разобранное логическое значение
			result = true;
			// Выводим признак успешного разбора
			return true;
		}
		/**
		 * Если значением является ложь
		 */
		if((value.compare("false") == 0) || (value.compare("0") == 0)){
			// Запоминаем разобранное логическое значение
			result = false;
			// Выводим признак успешного разбора
			return true;
		}
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если значением является истина
	 *
	 * @note Регистр записи при расширенном разборе не учитывается: файл настроек
	 *       пишет человек, и «On» вместе с «ON» встречаются в нём наравне с «on»
	 */
	if(::compare(value, "true") || ::compare(value, "yes") || ::compare(value, "on") || (value.compare("1") == 0)){
		// Запоминаем разобранное логическое значение
		result = true;
		// Выводим признак успешного разбора
		return true;
	}
	/**
	 * Если значением является ложь
	 */
	if(::compare(value, "false") || ::compare(value, "no") || ::compare(value, "off") || (value.compare("0") == 0)){
		// Запоминаем разобранное логическое значение
		result = false;
		// Выводим признак успешного разбора
		return true;
	}
	// Выводим признак неудачного разбора
	return false;
}
/**
 * @brief Шаблон типа числа результата разбора
 *
 * @tparam T тип числа результата разбора
 *
 */
template <typename T>
/**
 * @brief Метод разбора числа из значения свойства
 *
 * @param text   разбираемое значение свойства
 * @param result ссылка на результат разбора
 * @param forms  признаваемая запись логического значения
 * @return       признак успешного разбора
 *
 */
bool awh::codec::ini::numeric(const string_view text, T & result, const boolean_t forms) noexcept {
	/**
	 * Если запрошено логическое значение
	 *
	 * @note Сличение ведётся прежде целых чисел намеренно: логический тип языком
	 *       причислен к целым, и без этого «on» бы отвергалось
	 */
	if constexpr(is_same <T, bool>::value)
		// Выполняем разбор логического значения
		return boolean(text, result, forms);
	/**
	 * Если запрошено число с плавающей точкой
	 */
	else if constexpr(is_floating_point <T>::value) {
		// Значение числа с плавающей точкой наибольшей точности
		double value = 0.;
		/**
		 * Если разбор числа с плавающей точкой выполнить не удалось
		 */
		if(!real(text, value))
			// Выводим признак неудачного разбора
			return false;
		/**
		 * Запоминаем разобранное число
		 *
		 * @note Проверка выхода за предел запрошенного вида здесь прежде стояла отказом.
		 *       Отменена она владельцем 20.08.2026 доводом о том, что приведение языка не
		 *       отказывает нигде, а признак успешности разбора отведён одному лишь случаю,
		 *       когда значение числом не является вовсе
		 */
		result = static_cast <T> (value);
		// Выводим признак успешного разбора
		return true;
	/**
	 * Если запрошено целое число
	 */
	} else {
		{
			// Значение целого числа со знаком наибольшей разрядности
			int64_t value = 0;
			/**
			 * Если разбор целого числа со знаком удался
			 *
			 * @note Вид со знаком пробуется первым и при запросе вида без знака: запись
			 *       «-1» тем самым переносится младшими разрядами, ровно как это делает
			 *       приведение языка, а не сводится к нулю зажимом дробного пути
			 */
			if(integer(text, value)){
				// Запоминаем разобранное число
				result = static_cast <T> (value);
				// Выводим признак успешного разбора
				return true;
			}
		}
		{
			// Значение целого числа без знака наибольшей разрядности
			uint64_t value = 0;
			/**
			 * Если разбор целого числа без знака удался
			 *
			 * @note Заход этот берёт записи, в вид со знаком не вместившиеся, - вроде
			 *       «18446744073709551615»
			 */
			if(integer(text, value)){
				// Запоминаем разобранное число
				result = static_cast <T> (value);
				// Выводим признак успешного разбора
				return true;
			}
		}
		// Значение числа с плавающей точкой наибольшей точности
		double value = 0.;
		/**
		 * Если разбор числа с плавающей точкой выполнить не удалось
		 */
		if(!real(text, value))
			// Выводим признак неудачного разбора
			return false;
		/**
		 * Запоминаем разобранное число приведением дробного
		 *
		 * @note Заход этот берёт записи дробные - «1.5», «1e3» - и записи, ни в один целый
		 *       вид не вместившиеся. Отказ им прежде выдавался оттого, что целого разбора
		 *       они не проходят вовсе
		 */
		result = convert <T> (value);
		// Выводим признак успешного разбора
		return true;
	}
}

/**
 * Выполняем порождение метода разбора числа для всех поддерживаемых типов
 *
 * @note Перечень типов задан заранее и порождается здесь явно: реализация остаётся в
 *       файле реализации, а заголовочный файл несёт лишь объявление. Запрос типа,
 *       в перечень не входящего, отвечает отказом сборки на этапе связывания
 */
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <bool> (const string_view, bool &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <int8_t> (const string_view, int8_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <uint8_t> (const string_view, uint8_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <int16_t> (const string_view, int16_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <uint16_t> (const string_view, uint16_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <int32_t> (const string_view, int32_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <uint32_t> (const string_view, uint32_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <int64_t> (const string_view, int64_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <uint64_t> (const string_view, uint64_t &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <float> (const string_view, float &, const boolean_t) noexcept;
template __AWH_SHARED_EXPORT__ bool awh::codec::ini::numeric <double> (const string_view, double &, const boolean_t) noexcept;
