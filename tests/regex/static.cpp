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
#include <regex/text.hpp>
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

/**
 * @brief Проверка отказа привязки к позиции за пределами текста
 *
 * @details Проверка закрепляет намеренное решение: позиция за размером текста
 *          привязке не отвечает ни при каком её виде, и отказ выносится единым
 *          образом. Часть видов привязки обращается к тексту напрямую, часть -
 *          разбором символа, и без единой проверки первые бросили бы исключение
 *          из функции, объявленной noexcept, тогда как вторые ответили бы
 *          отказом молча. Путь этот вызывающими ныне не достигается, однако
 *          держится он соглашением вызывающих, а не проверкой.
 *
 */
TEST(Regex, StaticAssertionBounds) {
	/**
	 * @brief Набор проверяемых видов привязки к позиции в тексте
	 *
	 */
	const vector <regex::anchor_t> anchors = {
		regex::anchor_t::TEXT_BEGIN,
		regex::anchor_t::TEXT_END,
		regex::anchor_t::TEXT_FINISH,
		regex::anchor_t::LINE_BEGIN,
		regex::anchor_t::LINE_END,
		regex::anchor_t::WORD_EDGE,
		regex::anchor_t::WORD_INNER,
		regex::anchor_t::SEARCH_HEAD,
		regex::anchor_t::KEEP_OUT
	};
	/**
	 * @brief Набор проверяемых наборов режимов компиляции
	 *
	 */
	const vector <uint32_t> modes = {
		0,
		static_cast <uint32_t> (regex::flag_t::MULTILINE),
		static_cast <uint32_t> (regex::flag_t::UTF),
		(static_cast <uint32_t> (regex::flag_t::UTF) | static_cast <uint32_t> (regex::flag_t::UCP)),
		(static_cast <uint32_t> (regex::flag_t::UTF) | static_cast <uint32_t> (regex::flag_t::MULTILINE))
	};
	/**
	 * @brief Набор проверяемых текстов сопоставления
	 *
	 */
	const vector <string> texts = {"", "a\n", "узел", "слово\nстрока"};
	/**
	 * Выполняем перебор набора текстов сопоставления
	 */
	for(const auto & text : texts){
		/**
		 * Выполняем перебор набора видов привязки к позиции в тексте
		 */
		for(const auto anchor : anchors){
			/**
			 * Выполняем перебор набора наборов режимов компиляции
			 */
			for(const auto flags : modes){
				/**
				 * Выполняем перебор позиций за пределами текста
				 */
				for(size_t offset = 1; offset < 5; offset++)
					// Выполняем проверку отказа привязки к позиции за пределами текста
					EXPECT_FALSE(regex::assertion(text, 0, anchor, flags, text.size() + offset));
				// Выполняем проверку отказа привязки к предельной позиции
				EXPECT_FALSE(regex::assertion(text, 0, anchor, flags, string::npos));
			}
		}
	}
}
