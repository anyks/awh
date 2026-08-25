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
 * @brief Реализация общих определений контейнера CSV — описания кодов ошибок разбора,
 *        названия кодировок исходного текста и их определение, пригодность знака в
 *        разделители, необходимость кавычек и приведение содержимого поля числом
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <num/lexical/lexical.hpp>
#include <codec/csv/common.hpp>

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
	 * @brief Метод сличения последовательностей знаков без учёта регистра
	 *
	 * @details Сличение ведётся по правилам US-ASCII: прочие знаки сличаются как есть.
	 * Записи логического значения знаками US-ASCII ограничены, и привлекать сюда
	 * правила местности незачем
	 *
	 * @param first  первая последовательность знаков для сличения
	 * @param second вторая последовательность знаков для сличения
	 * @return       результат сличения
	 *
	 */
	static bool compare(const string_view first, const string_view second) noexcept {
		/**
		 * Если длины последовательностей расходятся
		 */
		if(first.length() != second.length())
			// Выводим признак несовпадения
			return false;
		/**
		 * Выполняем перебор всех знаков последовательности
		 */
		for(size_t i = 0; i < first.length(); i++){
			/**
			 * Если знаки в приведении к нижнему регистру расходятся
			 */
			if(!ascii::equals(first[i], second[i]))
				// Выводим признак несовпадения
				return false;
		}
		// Выводим признак совпадения
		return true;
	}
	/**
	 * @brief Метод снятия пробельной обвязки с последовательности знаков
	 *
	 * @param text последовательность знаков для снятия обвязки
	 * @return     последовательность знаков без обвязки
	 *
	 */
	static string_view trim(const string_view text) noexcept {
		// Положение первого знака, не являющегося пробельным
		size_t begin = 0;
		// Положение за последним знаком, не являющимся пробельным
		size_t end = text.length();
		/**
		 * Выполняем поиск первого знака, не являющегося пробельным
		 */
		while((begin < end) && ascii::isSpace(text[begin]))
			// Выполняем смещение начала последовательности
			begin++;
		/**
		 * Выполняем поиск последнего знака, не являющегося пробельным
		 */
		while((end > begin) && ascii::isSpace(text[end - 1]))
			// Выполняем смещение конца последовательности
			end--;
		// Выводим последовательность знаков без обвязки
		return text.substr(begin, end - begin);
	}
}

/**
 * @brief Метод получения сообщения об ошибке разбора
 *
 * @param error код ошибки разбора
 * @return      сообщение об ошибке
 *
 */
const char * awh::codec::csv::message(const error_t error) noexcept {
	/**
	 * Определяем код ошибки разбора
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (error_t::NONE):
			return "no error";
		// Если произошла внутренняя ошибка разбора
		case static_cast <uint8_t> (error_t::INTERNAL):
			return "internal parser error";
		// Если текст оборвался посреди записи
		case static_cast <uint8_t> (error_t::UNEXPECTED_EOF):
			return "unexpected end of input";
		// Если знак недопустим в тексте
		case static_cast <uint8_t> (error_t::INVALID_CHARACTER):
			return "invalid character";
		// Если последовательность байтов не отвечает объявленной кодировке
		case static_cast <uint8_t> (error_t::INVALID_ENCODING):
			return "invalid byte sequence for the declared encoding";
		// Если объявленная кодировка не поддерживается
		case static_cast <uint8_t> (error_t::UNSUPPORTED_ENCODING):
			return "unsupported encoding";
		// Если поле в кавычках не закрыто до конца текста
		case static_cast <uint8_t> (error_t::UNTERMINATED_QUOTE):
			return "unterminated quoted field";
		// Если внутри поля без кавычек встретилась одиночная кавычка
		case static_cast <uint8_t> (error_t::UNESCAPED_QUOTE):
			return "unescaped quote in unquoted field";
		// Если за закрывающей кавычкой поля обнаружены знаки
		case static_cast <uint8_t> (error_t::TRAILING_CHARACTERS):
			return "characters after closing quote";
		// Если перед открывающей кавычкой поля обнаружены знаки
		case static_cast <uint8_t> (error_t::LEADING_CHARACTERS):
			return "characters before opening quote";
		// Если длина поля превышает допустимую
		case static_cast <uint8_t> (error_t::FIELD_TOO_LONG):
			return "field is too long";
		// Если длина записи превышает допустимую
		case static_cast <uint8_t> (error_t::RECORD_TOO_LONG):
			return "record is too long";
		// Если количество полей в записи превышает допустимое
		case static_cast <uint8_t> (error_t::TOO_MANY_FIELDS):
			return "too many fields in record";
		// Если количество полей записи расходится с количеством полей заголовка
		case static_cast <uint8_t> (error_t::FIELD_COUNT_MISMATCH):
			return "field count differs from expected";
		// Если разделитель не удалось определить по содержимому
		case static_cast <uint8_t> (error_t::SEPARATOR_UNDETECTED):
			return "field separator could not be detected";
		// Если разделитель не задан вовсе либо совпадает с занятым разбором знаком
		case static_cast <uint8_t> (error_t::SEPARATOR_CONFLICT):
			return "field separator is unset or conflicts with quote or newline";
		// Если заголовок объявлен пустым именем поля
		case static_cast <uint8_t> (error_t::EMPTY_HEADER):
			return "empty field name in header";
		// Если имя поля в заголовке объявлено повторно
		case static_cast <uint8_t> (error_t::DUPLICATE_HEADER):
			return "duplicate field name in header";
		// Если заголовок затребован, а текст пуст
		case static_cast <uint8_t> (error_t::NO_HEADER):
			return "header requested but input is empty";
		// Если превышен предел, заданный настройками разбора
		case static_cast <uint8_t> (error_t::OVERFLOW_LIMIT):
			return "parser limit exceeded";
		// Если подача продолжена после объявленного конца текста
		case static_cast <uint8_t> (error_t::TEXT_ALREADY_ENDED):
			// Выводим описание кода ошибки
			return "feeding continued after the text was declared complete";
		// Если файл таблицы открыть не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_OPENED):
			// Выводим описание кода ошибки
			return "cannot open the table file";
		// Если текст таблицы записать в файл не удалось
		case static_cast <uint8_t> (error_t::FILE_NOT_WRITTEN):
			// Выводим описание кода ошибки
			return "cannot write the table file";
	}
	// Выводим сообщение о неизвестной ошибке
	return "unknown error";
}
/**
 * @brief Метод получения названия кодировки
 *
 * @param encoding кодировка исходного текста
 * @return         название кодировки
 *
 */
const char * awh::codec::csv::name(const encoding_t encoding) noexcept {
	/**
	 * Определяем кодировку исходного текста
	 */
	switch(static_cast <uint8_t> (encoding)){
		// Если кодировка не определена
		case static_cast <uint8_t> (encoding_t::NONE):
			return "none";
		// Если кодировкой является UTF-8
		case static_cast <uint8_t> (encoding_t::UTF8):
			return "UTF-8";
		// Если кодировкой является UTF-16 с обратным порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF16LE):
			return "UTF-16LE";
		// Если кодировкой является UTF-16 с прямым порядком байтов
		case static_cast <uint8_t> (encoding_t::UTF16BE):
			return "UTF-16BE";
		// Если кодировкой является ISO-8859-1
		case static_cast <uint8_t> (encoding_t::LATIN1):
			return "ISO-8859-1";
		// Если кодировкой является US-ASCII
		case static_cast <uint8_t> (encoding_t::ASCII):
			return "US-ASCII";
		// Если кодировкой является Windows-1252
		case static_cast <uint8_t> (encoding_t::CP1252):
			return "windows-1252";
	}
	// Выводим название неизвестной кодировки
	return "unknown";
}
/**
 * @brief Метод определения кодировки по метке порядка байтов
 *
 * @details Метки сличаются от длинной к короткой намеренно: метка UTF-32 с обратным
 * порядком байтов начинается теми же двумя байтами, что и метка UTF-16 с обратным
 * порядком, и сличение от короткой опознало бы её неверно
 *
 * @warning Метка UTF-32 выводится отсутствием кодировки, а не кодировкой UTF-16:
 * поддержки UTF-32 разбор не имеет, и назвать такой текст разбираемым значило бы
 * прочесть его знаками вперемешку с пустыми. Отказ выдаётся разбором отдельно
 *
 * @param text начало исходного текста
 * @return     определённая кодировка, UTF-8 при отсутствии метки либо отсутствие
 *             кодировки у неподдерживаемой
 *
 */
awh::codec::csv::encoding_t awh::codec::csv::encoding(const string_view text) noexcept {
	/**
	 * Если длины текста довольно для метки UTF-32
	 */
	if(text.length() >= 4){
		/**
		 * Если обнаружена метка порядка байтов кодировки UTF-32
		 */
		if(((static_cast <uint8_t> (text[0]) == 0xFF) && (static_cast <uint8_t> (text[1]) == 0xFE) &&
		    (static_cast <uint8_t> (text[2]) == 0x00) && (static_cast <uint8_t> (text[3]) == 0x00)) ||
		   ((static_cast <uint8_t> (text[0]) == 0x00) && (static_cast <uint8_t> (text[1]) == 0x00) &&
		    (static_cast <uint8_t> (text[2]) == 0xFE) && (static_cast <uint8_t> (text[3]) == 0xFF)))
			// Выводим отсутствие кодировки, поскольку UTF-32 разбором не поддерживается
			return encoding_t::NONE;
	}
	/**
	 * Если длины текста довольно для метки UTF-8
	 */
	if(text.length() >= 3){
		/**
		 * Если обнаружена метка UTF-8
		 */
		if((static_cast <uint8_t> (text[0]) == 0xEF) && (static_cast <uint8_t> (text[1]) == 0xBB) &&
		   (static_cast <uint8_t> (text[2]) == 0xBF))
			// Выводим кодировку UTF-8
			return encoding_t::UTF8;
	}
	/**
	 * Если длины текста довольно для метки UTF-16
	 */
	if(text.length() >= 2){
		/**
		 * Если обнаружена метка UTF-16 с обратным порядком байтов
		 */
		if((static_cast <uint8_t> (text[0]) == 0xFF) && (static_cast <uint8_t> (text[1]) == 0xFE))
			// Выводим кодировку UTF-16 с обратным порядком байтов
			return encoding_t::UTF16LE;
		/**
		 * Если обнаружена метка UTF-16 с прямым порядком байтов
		 */
		if((static_cast <uint8_t> (text[0]) == 0xFE) && (static_cast <uint8_t> (text[1]) == 0xFF))
			// Выводим кодировку UTF-16 с прямым порядком байтов
			return encoding_t::UTF16BE;
	}
	// Выводим кодировку, принимаемую при отсутствии метки
	return encoding_t::UTF8;
}
/**
 * @brief Метод получения знака конца строки
 *
 * @param newline вид знака конца строки
 * @return        последовательность знаков конца строки
 *
 */
string_view awh::codec::csv::newline(const newline_t newline) noexcept {
	/**
	 * Определяем вид знака конца строки
	 */
	switch(static_cast <uint8_t> (newline)){
		// Если знаком конца строки является возврат каретки с переводом строки
		case static_cast <uint8_t> (newline_t::CRLF):
			return "\r\n";
		// Если знаком конца строки является перевод строки
		case static_cast <uint8_t> (newline_t::LF):
			return "\n";
		// Если знаком конца строки является одиночный возврат каретки
		case static_cast <uint8_t> (newline_t::CR):
			return "\r";
	}
	// Выводим знак конца строки, принятый договором
	return "\r\n";
}
/**
 * @brief Метод проверки пригодности знака в разделители полей
 *
 * @param separator знак-разделитель полей
 * @param quote     знак кавычек
 * @return          результат проверки
 *
 */
bool awh::codec::csv::suitable(const char separator, const char quote) noexcept {
	/**
	 * Если разделитель не задан вовсе
	 */
	if(separator == '\0')
		// Выводим признак непригодности
		return false;
	/**
	 * Если разделитель совпадает со знаком кавычек либо со знаком конца строки
	 *
	 * @note Совпадение это делает текст неразбираемым: знак не может означать
	 *       одновременно и границу поля, и границу записи
	 */
	if((separator == quote) || (separator == '\r') || (separator == '\n'))
		// Выводим признак непригодности
		return false;
	// Выводим признак пригодности
	return true;
}
/**
 * @brief Метод проверки необходимости заключить поле в кавычки
 *
 * @param text      содержимое поля
 * @param separator знак-разделитель полей
 * @param quote     знак кавычек
 * @param quoting   правило заключения поля в кавычки
 * @return          результат проверки
 *
 */
bool awh::codec::csv::quotable(const string_view text, const char separator, const char quote, const quoting_t quoting) noexcept {
	/**
	 * Если кавычки не ставятся вовсе
	 */
	if(quoting == quoting_t::NONE)
		// Выводим признак отсутствия необходимости
		return false;
	/**
	 * Если в кавычки берутся все поля без разбора
	 */
	if(quoting == quoting_t::ALL)
		// Выводим признак необходимости кавычек
		return true;
	/**
	 * Признак необходимости кавычек, вызванной содержимым поля
	 *
	 * @note Необходимость эта считается прежде правила заключения в кавычки и правилу
	 *       не подчиняется: поле, содержащее разделитель либо знак конца строки, без
	 *       кавычек разрывает запись, и никакое правило записи этого не отменяет.
	 *       Прежде правило «все, кроме чисел» отвечало прежде такой проверки, и поле
	 *       вида «\t\r\n0», признанное числом, записывалось без кавычек - разрывая
	 *       запись надвое
	 */
	bool necessary = false;
	/**
	 * Если поле не пусто
	 *
	 * @note Пустое поле кавычек не требует: договор отличает поле пустое от поля,
	 *       записанного пустыми кавычками, лишь при особом уговоре, и умолчанием
	 *       такого отличия нет
	 */
	if(!text.empty()){
		/**
		 * Если поле начинается либо оканчивается пробельным знаком
		 *
		 * @note Обвязка без кавычек теряется у тех читающих, что снимают её сами, -
		 *       кавычки здесь сохраняют значащие пробелы
		 */
		if(ascii::isSpace(text.front()) || ascii::isSpace(text.back()))
			// Запоминаем признак необходимости кавычек
			necessary = true;
		/**
		 * Выполняем перебор всех знаков поля
		 */
		else for(const char letter : text){
			/**
			 * Если знак является разделителем, кавычкой либо знаком конца строки
			 */
			if((letter == separator) || (letter == quote) || (letter == '\r') || (letter == '\n')){
				// Запоминаем признак необходимости кавычек
				necessary = true;
				// Выходим из перебора знаков поля
				break;
			}
		}
	}
	/**
	 * Если в кавычки берутся все поля, кроме записанных числом
	 */
	if(quoting == quoting_t::NONNUMERIC){
		// Значение поля, приведённое к числу
		double result = 0.;
		// Выводим признак необходимости кавычек, если поле числом не записано
		return (necessary || !awh::codec::csv::real(text, result));
	}
	// Выводим признак необходимости кавычек по содержимому поля
	return necessary;
}
/**
 * @brief Метод приведения содержимого поля к целому числу со знаком
 *
 * @param text   содержимое поля
 * @param result полученное значение
 * @return       результат приведения
 *
 */
bool awh::codec::csv::integer(const string_view text, int64_t & result) noexcept {
	// Получаем содержимое поля без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если приводить нечего
	 */
	if(value.empty())
		// Выводим признак неудачного приведения
		return false;
	// Выполняем разбор целого числа со знаком
	const lexical_t::result_t <char> res = lexical_t::fromChars(value.data(), value.data() + value.length(), result);
	/**
	 * Выводим признак успешного приведения, если число разобрано целиком
	 *
	 * @note Остаток за числом отвергается намеренно: «52abc» числом не является,
	 *       и приведение такого поля к 52 скрыло бы ошибку в содержимом
	 */
	return (static_cast <bool> (res) && (res.ptr == (value.data() + value.length())));
}
/**
 * @brief Метод приведения содержимого поля к целому числу без знака
 *
 * @param text   содержимое поля
 * @param result полученное значение
 * @return       результат приведения
 *
 */
bool awh::codec::csv::integer(const string_view text, uint64_t & result) noexcept {
	// Получаем содержимое поля без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если приводить нечего
	 */
	if(value.empty())
		// Выводим признак неудачного приведения
		return false;
	/**
	 * Если содержимое записано со знаком числа
	 *
	 * @note Число со знаком в тип без знака не приводится даже тогда, когда знак
	 *       положительный: запрошенный тип и есть указание на ожидаемую запись
	 */
	if((value.front() == '-') || (value.front() == '+'))
		// Выводим признак неудачного приведения
		return false;
	// Выполняем разбор целого числа без знака
	const lexical_t::result_t <char> res = lexical_t::fromChars(value.data(), value.data() + value.length(), result);
	// Выводим признак успешного приведения, если число разобрано целиком
	return (static_cast <bool> (res) && (res.ptr == (value.data() + value.length())));
}
/**
 * @brief Метод приведения содержимого поля к числу с плавающей точкой
 *
 * @param text   содержимое поля
 * @param result полученное значение
 * @return       результат приведения
 *
 */
bool awh::codec::csv::real(const string_view text, double & result) noexcept {
	// Получаем содержимое поля без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если приводить нечего
	 */
	if(value.empty())
		// Выводим признак неудачного приведения
		return false;
	// Выполняем разбор числа с плавающей точкой
	const lexical_t::result_t <char> res = lexical_t::fromChars(value.data(), value.data() + value.length(), result);
	// Выводим признак успешного приведения, если число разобрано целиком
	return (static_cast <bool> (res) && (res.ptr == (value.data() + value.length())));
}
/**
 * @brief Метод приведения содержимого поля к логическому значению
 *
 * @details Признаются записи, сложившиеся на деле: «true» и «false», «yes» и «no»,
 * «on» и «off», единица и ноль - в любом регистре. Наречия здесь нет: у CSV нет
 * договора о записи логического значения вовсе, и признавать приходится всё
 * употребимое
 *
 * @param text   содержимое поля
 * @param result полученное значение
 * @return       результат приведения
 *
 */
bool awh::codec::csv::boolean(const string_view text, bool & result) noexcept {
	// Получаем содержимое поля без пробельной обвязки
	const string_view value = ::trim(text);
	/**
	 * Если приводить нечего
	 */
	if(value.empty())
		// Выводим признак неудачного приведения
		return false;
	/**
	 * Если содержимое отвечает записи истины
	 */
	if(::compare(value, "true") || ::compare(value, "yes") || ::compare(value, "on") || (value == "1")){
		// Устанавливаем полученное значение
		result = true;
		// Выводим признак успешного приведения
		return true;
	}
	/**
	 * Если содержимое отвечает записи лжи
	 */
	if(::compare(value, "false") || ::compare(value, "no") || ::compare(value, "off") || (value == "0")){
		// Устанавливаем полученное значение
		result = false;
		// Выводим признак успешного приведения
		return true;
	}
	// Выводим признак неудачного приведения
	return false;
}
