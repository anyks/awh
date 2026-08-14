/**
 * @file common.cpp
 * @date 2026-08-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки общих объявлений контейнера JSON — описания кодов отказов, названия
 *        видов узлов, проверка записи числа на соответствие стандарту и разбор
 *        необходимости экранирования
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/json/json.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Проверка описаний кодов отказов разбора
 *
 * @details Всякий объявленный код обязан нести своё описание: описание неизвестного
 * кода у объявленного означало бы, что отказ выдаётся, а объяснить его нечем
 *
 */
TEST(CodecJsonCommon, Messages) {
	/**
	 * Коды отказов разбора текста документа
	 */
	const vector <json::error_t> errors = {
		json::error_t::NONE, json::error_t::INTERNAL, json::error_t::UNEXPECTED_EOF,
		json::error_t::INVALID_CHARACTER, json::error_t::INVALID_ENCODING,
		json::error_t::UNSUPPORTED_ENCODING, json::error_t::UNTERMINATED_STRING,
		json::error_t::INVALID_ESCAPE, json::error_t::INVALID_UNICODE,
		json::error_t::UNPAIRED_SURROGATE, json::error_t::CONTROL_IN_STRING,
		json::error_t::INVALID_NUMBER, json::error_t::NUMBER_OUT_OF_RANGE,
		json::error_t::INVALID_LITERAL, json::error_t::TRAILING_CHARACTERS,
		json::error_t::EXPECTED_VALUE, json::error_t::EXPECTED_KEY,
		json::error_t::EXPECTED_COLON, json::error_t::EXPECTED_COMMA,
		json::error_t::TRAILING_COMMA, json::error_t::DUPLICATE_KEY,
		json::error_t::DEPTH_EXCEEDED, json::error_t::STRING_TOO_LONG,
		json::error_t::NUMBER_TOO_LONG, json::error_t::TOO_MANY_NODES,
		json::error_t::COMMENT_NOT_ALLOWED, json::error_t::UNTERMINATED_COMMENT,
		json::error_t::EMPTY_TEXT, json::error_t::OVERFLOW_LIMIT
	};
	// Набор уже встреченных описаний кодов отказов
	unordered_set <string> seen;
	/**
	 * Выполняем перебор всех кодов отказов разбора
	 */
	for(const json::error_t error : errors){
		// Получаем описание очередного кода отказа разбора
		const char * text = json::message(error);
		// Выполняем проверку наличия описания кода отказа
		ASSERT_TRUE((text != nullptr) && (::strlen(text) > 0));
		// Выполняем проверку того, что описание кода отказа не является заглушкой
		ASSERT_STRNE(text, "unknown error");
		// Выполняем проверку неповторимости описания кода отказа
		ASSERT_TRUE(seen.emplace(text).second) << "описание «" << text << "» повторяется";
	}
	// Выполняем проверку выдачи заглушки на неизвестный код отказа
	ASSERT_STREQ(json::message(static_cast <json::error_t> (0xFF)), "unknown error");
}
/**
 * @brief Проверка названий видов узлов документа
 *
 */
TEST(CodecJsonCommon, Names) {
	// Выполняем проверку названия неопределённого узла
	ASSERT_STREQ(json::name(json::kind_t::NONE), "none");
	// Выполняем проверку названия пустого значения
	ASSERT_STREQ(json::name(json::kind_t::NUL), "null");
	// Выполняем проверку названия логического значения
	ASSERT_STREQ(json::name(json::kind_t::BOOL), "boolean");
	// Выполняем проверку названия числа
	ASSERT_STREQ(json::name(json::kind_t::NUMBER), "number");
	// Выполняем проверку названия строки
	ASSERT_STREQ(json::name(json::kind_t::STRING), "string");
	// Выполняем проверку названия массива
	ASSERT_STREQ(json::name(json::kind_t::ARRAY), "array");
	// Выполняем проверку названия объекта
	ASSERT_STREQ(json::name(json::kind_t::OBJECT), "object");
	// Выполняем проверку выдачи заглушки на неизвестный вид узла
	ASSERT_STREQ(json::name(static_cast <json::kind_t> (0xFF)), "unknown");
}
/**
 * @brief Проверка признания годных записей числа
 *
 */
TEST(CodecJsonCommon, NumericValid) {
	/**
	 * Годные записи чисел
	 */
	const vector <string> values = {
		"0", "-0", "1", "-1", "42", "1234567890",
		"0.0", "-0.5", "3.14159", "0.000001",
		"1e10", "1E10", "1e+10", "1E-10", "-1.5e-30",
		"9007199254740993", "1e308"
	};
	/**
	 * Выполняем перебор всех годных записей чисел
	 */
	for(const string & value : values)
		// Выполняем проверку признания годной записи числа
		ASSERT_TRUE(json::numeric(value)) << "запись «" << value << "»";
}
/**
 * @brief Проверка отклонения негодных записей числа
 *
 */
TEST(CodecJsonCommon, NumericInvalid) {
	/**
	 * Негодные записи чисел
	 */
	const vector <string> values = {
		"", "-", "+", "+1", "01", "-01", "00", ".5", "-.5",
		"1.", "1.e5", "1e", "1e+", "1e-", "1.2.3", "1e1e1",
		" 1", "1 ", "0x10", "NaN", "Infinity", "abc", "1,5"
	};
	/**
	 * Выполняем перебор всех негодных записей чисел
	 */
	for(const string & value : values)
		// Выполняем проверку отклонения негодной записи числа
		ASSERT_FALSE(json::numeric(value)) << "запись «" << value << "»";
}
/**
 * @brief Проверка разбора необходимости экранирования
 *
 */
TEST(CodecJsonCommon, Escapable) {
	// Выполняем проверку отсутствия необходимости экранирования простого содержимого
	ASSERT_FALSE(json::escapable("простой текст", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования кавычки
	ASSERT_TRUE(json::escapable("a\"b", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования знака отмены
	ASSERT_TRUE(json::escapable("a\\b", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования управляющего знака
	ASSERT_TRUE(json::escapable(string("a\nb"), json::escape_t::MINIMAL));
	// Выполняем проверку отсутствия необходимости экранирования косой черты по умолчанию
	ASSERT_FALSE(json::escapable("a/b", json::escape_t::MINIMAL));
	// Выполняем проверку необходимости экранирования косой черты при затребовании
	ASSERT_TRUE(json::escapable("a/b", json::escape_t::SOLIDUS));
	// Выполняем проверку отсутствия необходимости экранирования знаков вне US-ASCII
	ASSERT_FALSE(json::escapable("привет", json::escape_t::SOLIDUS));
	// Выполняем проверку необходимости экранирования знаков вне US-ASCII при затребовании
	ASSERT_TRUE(json::escapable("привет", json::escape_t::ASCII));
	// Выполняем проверку отсутствия необходимости экранирования пустого содержимого
	ASSERT_FALSE(json::escapable("", json::escape_t::ASCII));
}
