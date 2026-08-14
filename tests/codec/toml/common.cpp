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
 * @brief Проверки общих определений контейнера TOML — описания кодов ошибок, названия
 *        кодировок и типов значений, знаки конца строки и набор знаков имени ключа
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/toml/toml.hpp>

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
TEST(CodecTomlCommon, Messages) {
	// Выполняем проверку описания отсутствия ошибок
	ASSERT_STREQ(toml::message(toml::error_t::NONE), "no error");
	/**
	 * Выполняем перебор всех кодов ошибок разбора и записи
	 */
	for(uint32_t code = 0; code <= static_cast <uint32_t> (toml::error_t::CONFLICTING_SETTINGS); code++){
		// Получаем описание очередного кода ошибки
		const char * message = toml::message(static_cast <toml::error_t> (code));
		// Выполняем проверку того, что описание кода ошибки выдано
		ASSERT_NE(message, nullptr) << code;
		// Выполняем проверку того, что описание кода ошибки не пусто
		ASSERT_GT(::strlen(message), 0u) << code;
		// Выполняем проверку того, что код ошибки описанием опознан
		ASSERT_STRNE(message, "unknown error") << code;
	}
	// Выполняем проверку описания кода ошибки за пределами перечисления
	ASSERT_STREQ(toml::message(static_cast <toml::error_t> (0xFF)), "unknown error");
}
/**
 * @brief Проверка названий кодировок исходного текста
 *
 */
TEST(CodecTomlCommon, Encodings) {
	// Выполняем проверку названия кодировки UTF-8
	ASSERT_STREQ(toml::name(toml::encoding_t::UTF8), "UTF-8");
	// Выполняем проверку названия кодировки UTF-16 с обратным порядком байтов
	ASSERT_STREQ(toml::name(toml::encoding_t::UTF16LE), "UTF-16LE");
	// Выполняем проверку названия кодировки UTF-16 с прямым порядком байтов
	ASSERT_STREQ(toml::name(toml::encoding_t::UTF16BE), "UTF-16BE");
	// Выполняем проверку названия неопределённой кодировки
	ASSERT_STREQ(toml::name(toml::encoding_t::NONE), "unknown");
}
/**
 * @brief Проверка названий типов значений
 *
 * @details Названия эти уходят в сообщения об ошибках несовпадения типа, и опознан
 * обязан быть всякий тип: описание отводит отметке времени четыре вида, и путать их
 * между собою нельзя
 *
 */
TEST(CodecTomlCommon, Types) {
	// Выполняем проверку названия типа последовательности знаков
	ASSERT_STREQ(toml::name(toml::type_t::STRING), "string");
	// Выполняем проверку названия типа целого числа
	ASSERT_STREQ(toml::name(toml::type_t::INTEGER), "integer");
	// Выполняем проверку названия типа числа с плавающей точкой
	ASSERT_STREQ(toml::name(toml::type_t::FLOAT), "float");
	// Выполняем проверку названия логического типа
	ASSERT_STREQ(toml::name(toml::type_t::BOOLEAN), "boolean");
	// Выполняем проверку названия типа отметки времени со смещением
	ASSERT_STREQ(toml::name(toml::type_t::OFFSET_DATETIME), "offset date-time");
	// Выполняем проверку названия типа отметки времени без смещения
	ASSERT_STREQ(toml::name(toml::type_t::LOCAL_DATETIME), "local date-time");
	// Выполняем проверку названия типа местной даты
	ASSERT_STREQ(toml::name(toml::type_t::LOCAL_DATE), "local date");
	// Выполняем проверку названия типа местного времени
	ASSERT_STREQ(toml::name(toml::type_t::LOCAL_TIME), "local time");
	// Выполняем проверку названия типа перечня
	ASSERT_STREQ(toml::name(toml::type_t::ARRAY), "array");
	// Выполняем проверку названия типа таблицы
	ASSERT_STREQ(toml::name(toml::type_t::TABLE), "table");
	// Выполняем проверку названия неопределённого типа
	ASSERT_STREQ(toml::name(toml::type_t::NONE), "none");
}
/**
 * @brief Проверка знаков конца строки собираемого текста
 *
 */
TEST(CodecTomlCommon, Newlines) {
	// Выполняем проверку знака конца строки систем семейства UNIX
	ASSERT_EQ(toml::newline(toml::newline_t::LF), "\n");
	// Выполняем проверку знака конца строки системы MS Windows
	ASSERT_EQ(toml::newline(toml::newline_t::CRLF), "\r\n");
}
/**
 * @brief Проверка набора знаков имени ключа без кавычек
 *
 * @details Набор задан описанием и знаками US-ASCII ограничен: всё прочее в имени без
 * кавычек недопустимо и требует ограждения
 *
 */
TEST(CodecTomlCommon, BareKeys) {
	/**
	 * Выполняем перебор всех значений байта
	 */
	for(uint32_t code = 0; code < 0x100; code++){
		// Получаем очередной проверяемый знак
		const char letter = static_cast <char> (code);
		// Получаем ожидаемый признак допустимости знака в имени без кавычек
		const bool expected = (((code >= 'a') && (code <= 'z')) || ((code >= 'A') && (code <= 'Z')) ||
		                       ((code >= '0') && (code <= '9')) || (code == '_') || (code == '-'));
		// Выполняем проверку признака допустимости знака в имени без кавычек
		ASSERT_EQ(toml::bare(letter), expected) << code;
	}
	// Выполняем проверку недопустимости точки в имени без кавычек
	ASSERT_FALSE(toml::bare('.'));
	// Выполняем проверку недопустимости пробела в имени без кавычек
	ASSERT_FALSE(toml::bare(' '));
	// Выполняем проверку недопустимости кавычки в имени без кавычек
	ASSERT_FALSE(toml::bare('"'));
}
/**
 * @brief Проверка умолчаний структур модуля
 *
 * @details Умолчания эти значащи: отметка времени без смещения отличается от отметки со
 * смещением в ноль, и различить их можно лишь по обозначению отсутствия смещения
 *
 */
TEST(CodecTomlCommon, Defaults) {
	// Объект положения в исходном тексте
	const toml::location_t location;
	// Выполняем проверку того, что смещение неизвестно
	ASSERT_EQ(location.offset, toml::NO_OFFSET);
	// Выполняем проверку того, что номер строки не установлен
	ASSERT_EQ(location.line, 0u);
	// Выполняем проверку того, что положение в строке не установлено
	ASSERT_EQ(location.column, 0u);
	// Объект отметки времени
	const toml::stamp_t stamp;
	/**
	 * Выполняем проверку отсутствия смещения часового пояса
	 *
	 * @note Ноль обозначением отсутствия быть не может: он означает часовой пояс UTC
	 */
	ASSERT_EQ(stamp.offset, toml::NO_TIMEZONE);
	// Выполняем проверку того, что обозначение отсутствия смещения нулём не является
	ASSERT_NE(toml::NO_TIMEZONE, 0);
	// Объект значения текста настроек
	const toml::value_t value;
	// Выполняем проверку того, что тип значения не определён
	ASSERT_EQ(value.type, toml::type_t::NONE);
	// Объект составной части имени ключа
	const toml::part_t part;
	// Выполняем проверку того, что запись имени части взята без кавычек
	ASSERT_EQ(part.naming, toml::naming_t::BARE);
}
