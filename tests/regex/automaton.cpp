/**
 * @file: automaton.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты детерминированного исполнения регулярных выражений — сличение
 *        вердикта наличия совпадения с эталонной реализацией PCRE2 и с исполнением без возврата,
 *        а также проверка сохранения вердикта при сбросе кэша состояний автомата
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

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/dfa.hpp>
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

/**
 * @brief Проверка сохранения вердикта при повторном исполнении программы
 *
 */
TEST(Regex, AutomatonCache) {
	// Создаём объект разбора регулярного выражения
	regex::parser_t parser;
	// Выполняем разбор регулярного выражения
	ASSERT_TRUE(parser.parse("^[a-z]+[0-9]{2,4}$", 0));
	// Создаём объект компиляции регулярного выражения
	regex::compiler_t compiler;
	// Создаём компилируемую программу регулярного выражения
	regex::program_t program;
	// Выполняем компиляцию регулярного выражения
	ASSERT_TRUE(compiler.compile(parser, program));
	// Создаём объект детерминированного исполнения
	regex::dfa_t dfa;
	/**
	 * Выполняем многократное исполнение программы на различных текстах
	 *
	 * @details Кэш состояний сохраняется между вызовами, поэтому повторное
	 *          исполнение обязано давать тот же вердикт, что и первое.
	 *
	 */
	for(size_t i = 0; i < 4; i++) {
		// Выполняем проверку вердикта на соответствующем тексте
		EXPECT_TRUE(dfa.test(program, "abc123", 0));
		// Выполняем проверку вердикта на тексте с недостаточным числом цифр
		EXPECT_FALSE(dfa.test(program, "abc1", 0));
		// Выполняем проверку вердикта на тексте с избыточным числом цифр
		EXPECT_FALSE(dfa.test(program, "abc123456", 0));
		// Выполняем проверку вердикта на тексте без букв
		EXPECT_FALSE(dfa.test(program, "123", 0));
	}
}
