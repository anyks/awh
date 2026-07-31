/**
 * @file: matching.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты сопоставления регулярных выражений — сличение границ совпадения
 *        и захваченных групп с эталонной реализацией PCRE2 на наборе шаблонов регулярного
 *        подмножества и на шаблонах, порождаемых псевдослучайным образом
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <random>
#include <string>
#include <vector>
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/pike.hpp>
#include <regex/parser.hpp>
#include <regex/compiler.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../main.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

using result_t = vector <pair <size_t, size_t>>;

/**
 * @brief Проверка отнесения выражений к нерегулярному подмножеству
 *
 */
TEST(Regex, Unsupported) {
	// Создаём набор выражений, требующих исполнения с возвратом
	const vector <const char *> items = {
		"(a)\\1", "(?=a)b", "(?<=a)b", "(?>a)b", "a++", "(?R)", "(a)(?1)",
		"(a)?(?(1)b|c)", "a\\Kb", "\\X", "(a*)*", "(|a)*"
	};
	/**
	 * Выполняем проверку отнесения выражений к нерегулярному подмножеству
	 */
	for(const auto & item : items) {
		// Создаём объект разбора регулярного выражения
		regex::parser_t parser;
		// Выполняем разбор регулярного выражения
		ASSERT_TRUE(parser.parse(item, 0)) << "Шаблон: " << item;
		// Создаём объект компиляции регулярного выражения
		regex::compiler_t compiler;
		// Создаём компилируемую программу регулярного выражения
		regex::program_t program;
		// Выполняем проверку отказа компиляции выражения
		EXPECT_FALSE(compiler.compile(parser, program)) << "Шаблон: " << item;
		// Выполняем проверку кода ошибки компиляции
		EXPECT_EQ(compiler.error(), regex::error_t::UNSUPPORTED) << "Шаблон: " << item;
	}
}

/**
 * @brief Проверка формирования предварительного отбора позиций
 *
 */
TEST(Regex, Prefilter) {
	/**
	 * @brief Проверяемый шаблон и ожидаемый предварительный отбор
	 *
	 */
	struct Item {
		// Текст проверяемого регулярного выражения
		const char * pattern;
		// Ожидаемый обязательный литерал совпадения
		const char * literal;
		// Ожидаемый флаг применимости набора допустимых байтов
		bool active;
		// Байты, обязанные присутствовать в наборе допустимых
		const char * allowed;
		// Байты, обязанные отсутствовать в наборе допустимых
		const char * denied;
	};
	// Создаём набор проверяемых шаблонов
	const vector <Item> items = {
		// Литерал извлекается из последовательности символов
		{"abc", "abc", true, "a", "bcz"},
		// Литерал извлекается из середины выражения
		{"\\w+@\\w+", "@", true, "az09_", "@ ."},
		// Литерал извлекается из обязательного повторения
		{"[0-9]+end", "end", true, "0459", "aez "},
		// Литерал не извлекается из необязательного повторения
		{"(abc)?xyz", "xyz", true, "ax", "bcyz"},
		// Литерал не извлекается из выбора одной из ветвей
		{"cat|dog", "", true, "cd", "atog"},
		// Сопоставление без учёта регистра исключает литерал
		{"(?i)abc", "", true, "aA", "bBcC"},
		// Класс символов задаёт набор допустимых байтов
		{"[xyz]+", "", true, "xyz", "abw0"},
		// Выражение с совпадением нулевой длины отбор не допускает
		{"a*", "", false, "", ""},
		// Выражение с необязательным началом отбор не допускает
		{"(abc)?", "", false, "", ""}
	};
	/**
	 * Выполняем проверку предварительного отбора для каждого шаблона
	 */
	for(const auto & item : items) {
		// Создаём объект разбора регулярного выражения
		regex::parser_t parser;
		// Выполняем разбор регулярного выражения
		ASSERT_TRUE(parser.parse(item.pattern, 0)) << "Шаблон: " << item.pattern;
		// Создаём объект компиляции регулярного выражения
		regex::compiler_t compiler;
		// Создаём компилируемую программу регулярного выражения
		regex::program_t program;
		// Выполняем компиляцию регулярного выражения
		ASSERT_TRUE(compiler.compile(parser, program)) << "Шаблон: " << item.pattern;
		// Выполняем проверку обязательного литерала совпадения
		EXPECT_EQ(program.prefilter.literal, string(item.literal)) << "Шаблон: " << item.pattern;
		// Выполняем проверку применимости набора допустимых байтов
		EXPECT_EQ(program.prefilter.active, item.active) << "Шаблон: " << item.pattern;
		/**
		 * Выполняем проверку присутствия байтов в наборе допустимых
		 */
		for(const char * letter = item.allowed; *letter != '\0'; letter++)
			// Выполняем проверку допустимости очередного байта
			EXPECT_TRUE(program.prefilter.bytes[static_cast <uint8_t> (*letter)])
				<< "Шаблон: " << item.pattern << ", байт: " << *letter;
		/**
		 * Выполняем проверку отсутствия байтов в наборе допустимых
		 */
		for(const char * letter = item.denied; *letter != '\0'; letter++)
			// Выполняем проверку недопустимости очередного байта
			EXPECT_FALSE(program.prefilter.bytes[static_cast <uint8_t> (*letter)])
				<< "Шаблон: " << item.pattern << ", байт: " << *letter;
	}
}
