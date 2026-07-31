/**
 * @file: static.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты синтаксического разбора регулярных выражений — сличение вердиктов
 *        разбора с эталонной реализацией PCRE2 на наборе шаблонов, покрывающем конструкции
 *        синтаксиса PCRE, и на шаблонах, порождаемых псевдослучайным образом
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
#include <regex/parser.hpp>

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
 * @brief Проверка разрешения ссылок и определения количества захватывающих групп
 *
 */
TEST(Regex, Captures) {
	// Создаём объект разбора регулярного выражения
	regex::parser_t parser;
	// Выполняем разбор регулярного выражения с захватывающими группами
	ASSERT_TRUE(parser.parse("(a)(?:b)(?<name>c)(d)", 0));
	// Выполняем проверку количества захватывающих групп
	EXPECT_EQ(parser.captures(), 3u);
	// Выполняем разбор регулярного выражения со сбросом нумерации ветвей
	ASSERT_TRUE(parser.parse("(?|(a)|(b)(c))", 0));
	// Выполняем проверку количества захватывающих групп
	EXPECT_EQ(parser.captures(), 2u);
	// Выполняем разбор регулярного выражения с опережающей ссылкой на группу
	ASSERT_TRUE(parser.parse("(?&name)(?<name>a)", 0));
	// Выполняем проверку разбора регулярного выражения с неразрешимой ссылкой
	EXPECT_FALSE(parser.parse("(?&missing)(?<name>a)", 0));
	// Выполняем проверку кода ошибки разбора
	EXPECT_EQ(parser.error(), regex::error_t::BAD_BACKREFERENCE);
}
