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
 * @brief Проверки общих определений контейнера CSV — описания кодов ошибок, названия
 *        кодировок и их определение, пригодность разделителя, необходимость кавычек и
 *        приведение содержимого поля числом
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/csv/csv.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Проверка описаний кодов ошибок
 *
 * @details Описание обязано быть у всякого кода: без него отказ разбора доходит до
 * потребителя одним числом
 *
 */
TEST(CodecCsvCommon, Messages) {
	// Выполняем проверку описания отсутствия ошибок
	ASSERT_STREQ(csv::message(csv::error_t::NONE), "no error");
	/**
	 * Выполняем перебор всех кодов ошибок разбора
	 */
	for(uint32_t code = 0; code <= static_cast <uint32_t> (csv::error_t::OVERFLOW_LIMIT); code++){
		// Получаем описание очередного кода ошибки
		const char * message = csv::message(static_cast <csv::error_t> (code));
		// Выполняем проверку того, что описание кода ошибки выдано
		ASSERT_NE(message, nullptr) << code;
		// Выполняем проверку того, что описание кода ошибки не пусто
		ASSERT_GT(::strlen(message), 0u) << code;
		// Выполняем проверку того, что код ошибки описанием опознан
		ASSERT_STRNE(message, "unknown error") << code;
	}
	// Выполняем проверку описания кода ошибки за пределами перечисления
	ASSERT_STREQ(csv::message(static_cast <csv::error_t> (0xFF)), "unknown error");
}
/**
 * @brief Проверка названий кодировок исходного текста
 *
 */
TEST(CodecCsvCommon, Encodings) {
	// Выполняем проверку названия кодировки UTF-8
	ASSERT_STREQ(csv::name(csv::encoding_t::UTF8), "UTF-8");
	// Выполняем проверку названия кодировки UTF-16 с обратным порядком байтов
	ASSERT_STREQ(csv::name(csv::encoding_t::UTF16LE), "UTF-16LE");
	// Выполняем проверку названия кодировки UTF-16 с прямым порядком байтов
	ASSERT_STREQ(csv::name(csv::encoding_t::UTF16BE), "UTF-16BE");
	// Выполняем проверку названия кодировки Windows-1252
	ASSERT_STREQ(csv::name(csv::encoding_t::CP1252), "windows-1252");
	// Выполняем проверку названия кодировки за пределами перечисления
	ASSERT_STREQ(csv::name(static_cast <csv::encoding_t> (0xFF)), "unknown");
}
/**
 * @brief Проверка определения кодировки по метке порядка байтов
 *
 * @details Текст без метки считается записанным в UTF-8: кодировку свою запись CSV не
 * объявляет никак
 *
 */
TEST(CodecCsvCommon, Signature) {
	// Выполняем проверку определения кодировки по метке UTF-8
	ASSERT_EQ(csv::encoding("\xEF\xBB\xBF" "a,b"), csv::encoding_t::UTF8);
	// Выполняем проверку определения кодировки по метке UTF-16 с обратным порядком байтов
	ASSERT_EQ(csv::encoding("\xFF\xFE" "a\0"), csv::encoding_t::UTF16LE);
	// Выполняем проверку определения кодировки по метке UTF-16 с прямым порядком байтов
	ASSERT_EQ(csv::encoding("\xFE\xFF\0" "a"), csv::encoding_t::UTF16BE);
	// Выполняем проверку определения кодировки текста без метки
	ASSERT_EQ(csv::encoding("a,b\r\n"), csv::encoding_t::UTF8);
	// Выполняем проверку определения кодировки пустого текста
	ASSERT_EQ(csv::encoding(""), csv::encoding_t::UTF8);
}
/**
 * @brief Проверка опознания метки порядка байтов кодировки UTF-32
 *
 * @details Метка UTF-32 с обратным порядком байтов начинается теми же двумя байтами,
 * что и метка UTF-16 с обратным порядком. Кодировка эта разбором не поддерживается, и
 * опознать её нужно затем, чтобы не прочесть такой текст как UTF-16 со вставленными
 * пустыми знаками - молча и без единой жалобы
 *
 */
TEST(CodecCsvCommon, SignatureUtf32) {
	// Выполняем проверку опознания метки UTF-32 с обратным порядком байтов
	ASSERT_EQ(csv::encoding(string_view("\xFF\xFE\x00\x00", 4)), csv::encoding_t::NONE);
	// Выполняем проверку опознания метки UTF-32 с прямым порядком байтов
	ASSERT_EQ(csv::encoding(string_view("\x00\x00\xFE\xFF", 4)), csv::encoding_t::NONE);
	/**
	 * Выполняем проверку того, что метка UTF-16 короче четырёх байтов опознаётся верно
	 *
	 * @note Метка UTF-16, за которой стоят настоящие знаки, а не нули, кодировкой
	 *       UTF-32 не является: сличение обязано смотреть на все четыре байта
	 */
	ASSERT_EQ(csv::encoding(string_view("\xFF\xFE\x61\x00", 4)), csv::encoding_t::UTF16LE);
}
/**
 * @brief Проверка знаков конца строки
 *
 */
TEST(CodecCsvCommon, Newline) {
	// Выполняем проверку знака конца строки, названного договором
	ASSERT_EQ(csv::newline(csv::newline_t::CRLF), "\r\n");
	// Выполняем проверку знака конца строки систем семейства UNIX
	ASSERT_EQ(csv::newline(csv::newline_t::LF), "\n");
	// Выполняем проверку одиночного возврата каретки
	ASSERT_EQ(csv::newline(csv::newline_t::CR), "\r");
}
/**
 * @brief Проверка пригодности знака в разделители полей
 *
 * @details Разделителем не бывает знак, уже занятый разбором: совпадение с ним делает
 * текст неразбираемым, ибо знак не может означать одновременно и границу поля, и
 * границу записи
 *
 */
TEST(CodecCsvCommon, Suitable) {
	// Выполняем проверку пригодности запятой
	ASSERT_TRUE(csv::suitable(',', '"'));
	// Выполняем проверку пригодности точки с запятой
	ASSERT_TRUE(csv::suitable(';', '"'));
	// Выполняем проверку пригодности знака табуляции
	ASSERT_TRUE(csv::suitable('\t', '"'));
	// Выполняем проверку непригодности знака кавычек
	ASSERT_FALSE(csv::suitable('"', '"'));
	// Выполняем проверку непригодности возврата каретки
	ASSERT_FALSE(csv::suitable('\r', '"'));
	// Выполняем проверку непригодности перевода строки
	ASSERT_FALSE(csv::suitable('\n', '"'));
	// Выполняем проверку непригодности незаданного разделителя
	ASSERT_FALSE(csv::suitable('\0', '"'));
	/**
	 * Выполняем проверку пригодности запятой при кавычках, заданных запятой
	 *
	 * @note Пригодность считается парой: знак, годный при одних кавычках, при других
	 *       негоден
	 */
	ASSERT_FALSE(csv::suitable(',', ','));
}
/**
 * @brief Проверка необходимости заключить поле в кавычки
 *
 */
TEST(CodecCsvCommon, Quotable) {
	// Выполняем проверку поля, кавычек не требующего
	ASSERT_FALSE(csv::quotable("abc", ',', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку поля с разделителем внутри
	ASSERT_TRUE(csv::quotable("a,b", ',', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку поля с кавычкой внутри
	ASSERT_TRUE(csv::quotable("a\"b", ',', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку поля с переводом строки внутри
	ASSERT_TRUE(csv::quotable("a\nb", ',', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку поля с возвратом каретки внутри
	ASSERT_TRUE(csv::quotable("a\rb", ',', '"', csv::quoting_t::MINIMAL));
	/**
	 * Выполняем проверку поля с пробельной обвязкой
	 *
	 * @note Обвязка без кавычек теряется у тех читающих, что снимают её сами: кавычки
	 *       здесь сохраняют значащие пробелы
	 */
	ASSERT_TRUE(csv::quotable(" a", ',', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку поля с пробелом в конце
	ASSERT_TRUE(csv::quotable("a ", ',', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку пустого поля
	ASSERT_FALSE(csv::quotable("", ',', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку правила заключения в кавычки всех полей
	ASSERT_TRUE(csv::quotable("abc", ',', '"', csv::quoting_t::ALL));
	// Выполняем проверку правила отказа от кавычек вовсе
	ASSERT_FALSE(csv::quotable("a,b", ',', '"', csv::quoting_t::NONE));
	// Выполняем проверку правила заключения в кавычки всего, кроме чисел
	ASSERT_FALSE(csv::quotable("42", ',', '"', csv::quoting_t::NONNUMERIC));
	// Выполняем проверку поля, числом не являющегося, при том же правиле
	ASSERT_TRUE(csv::quotable("abc", ',', '"', csv::quoting_t::NONNUMERIC));
	/**
	 * Выполняем проверку поля с иным разделителем
	 *
	 * @note Кавычки ставятся по заданному разделителю, а не по запятой: текст с
	 *       точкой с запятой иначе получал бы кавычки не там, где нужно
	 */
	ASSERT_FALSE(csv::quotable("a,b", ';', '"', csv::quoting_t::MINIMAL));
	// Выполняем проверку поля с заданным разделителем внутри
	ASSERT_TRUE(csv::quotable("a;b", ';', '"', csv::quoting_t::MINIMAL));
}
/**
 * @brief Проверка приведения содержимого поля к целому числу со знаком
 *
 */
TEST(CodecCsvCommon, Integer) {
	// Полученное значение
	int64_t result = 0;
	// Выполняем проверку приведения положительного числа
	ASSERT_TRUE(csv::integer("42", result));
	// Выполняем проверку полученного значения
	ASSERT_EQ(result, 42);
	// Выполняем проверку приведения отрицательного числа
	ASSERT_TRUE(csv::integer("-17", result));
	// Выполняем проверку полученного значения
	ASSERT_EQ(result, -17);
	// Выполняем проверку приведения числа с пробельной обвязкой
	ASSERT_TRUE(csv::integer("  8  ", result));
	// Выполняем проверку полученного значения
	ASSERT_EQ(result, 8);
	/**
	 * Выполняем проверку отказа приведения числа с остатком
	 *
	 * @note Остаток за числом отвергается намеренно: приведение «52abc» к 52 скрыло бы
	 *       ошибку в содержимом
	 */
	ASSERT_FALSE(csv::integer("52abc", result));
	// Выполняем проверку отказа приведения пустого содержимого
	ASSERT_FALSE(csv::integer("", result));
	// Выполняем проверку отказа приведения содержимого, числом не являющегося
	ASSERT_FALSE(csv::integer("abc", result));
}
/**
 * @brief Проверка приведения содержимого поля к целому числу без знака
 *
 */
TEST(CodecCsvCommon, Unsigned) {
	// Полученное значение
	uint64_t result = 0;
	// Выполняем проверку приведения числа
	ASSERT_TRUE(csv::integer("42", result));
	// Выполняем проверку полученного значения
	ASSERT_EQ(result, 42u);
	/**
	 * Выполняем проверку отказа приведения числа со знаком
	 *
	 * @note Число со знаком в тип без знака не приводится даже тогда, когда знак
	 *       положительный: запрошенный тип и есть указание на ожидаемую запись
	 */
	ASSERT_FALSE(csv::integer("+42", result));
	// Выполняем проверку отказа приведения отрицательного числа
	ASSERT_FALSE(csv::integer("-42", result));
}
/**
 * @brief Проверка приведения содержимого поля к числу с плавающей точкой
 *
 */
TEST(CodecCsvCommon, Real) {
	// Полученное значение
	double result = 0.;
	// Выполняем проверку приведения дробного числа
	ASSERT_TRUE(csv::real("3.14", result));
	// Выполняем проверку полученного значения
	ASSERT_DOUBLE_EQ(result, 3.14);
	// Выполняем проверку приведения числа в показательной записи
	ASSERT_TRUE(csv::real("1e3", result));
	// Выполняем проверку полученного значения
	ASSERT_DOUBLE_EQ(result, 1000.);
	// Выполняем проверку отказа приведения содержимого с остатком
	ASSERT_FALSE(csv::real("3.14abc", result));
	// Выполняем проверку отказа приведения пустого содержимого
	ASSERT_FALSE(csv::real("", result));
}
/**
 * @brief Проверка приведения содержимого поля к логическому значению
 *
 * @details Договора о записи логического значения у CSV нет вовсе, потому признаются
 * все записи, сложившиеся на деле
 *
 */
TEST(CodecCsvCommon, Boolean) {
	// Полученное значение
	bool result = false;
	// Выполняем проверку приведения записи истины
	ASSERT_TRUE(csv::boolean("true", result));
	// Выполняем проверку полученного значения
	ASSERT_TRUE(result);
	// Выполняем проверку приведения записи истины в верхнем регистре
	ASSERT_TRUE(csv::boolean("TRUE", result));
	// Выполняем проверку полученного значения
	ASSERT_TRUE(result);
	// Выполняем проверку приведения записи истины словом «yes»
	ASSERT_TRUE(csv::boolean("yes", result));
	// Выполняем проверку полученного значения
	ASSERT_TRUE(result);
	// Выполняем проверку приведения записи истины единицей
	ASSERT_TRUE(csv::boolean("1", result));
	// Выполняем проверку полученного значения
	ASSERT_TRUE(result);
	// Выполняем проверку приведения записи лжи
	ASSERT_TRUE(csv::boolean("false", result));
	// Выполняем проверку полученного значения
	ASSERT_FALSE(result);
	// Выполняем проверку приведения записи лжи словом «off»
	ASSERT_TRUE(csv::boolean("off", result));
	// Выполняем проверку полученного значения
	ASSERT_FALSE(result);
	// Выполняем проверку отказа приведения содержимого, логическим значением не являющегося
	ASSERT_FALSE(csv::boolean("может быть", result));
	// Выполняем проверку отказа приведения пустого содержимого
	ASSERT_FALSE(csv::boolean("", result));
}
