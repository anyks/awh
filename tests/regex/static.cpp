/**
 * @file static.cpp
 * @date 2026-07-31
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
 * @brief Автоматические тесты синтаксического разбора регулярных выражений — сличение вердиктов
 *        разбора с эталонной реализацией PCRE2 на наборе шаблонов, покрывающем конструкции
 *        синтаксиса PCRE, и на шаблонах, порождаемых псевдослучайным образом
 *
 * @copyright Copyright © 2026
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
#include <regex/prefilter.hpp>

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

/**
 * @brief Проверка поиска последовательности в тексте по якорному байту
 *
 * @details Поиск отыскивает в тексте не первый байт искомого, а байт, пробой
 *          текста признанный редчайшим: байт первый разборчивостью не отличается
 *          и обрывает поиск непрестанно. Проверка сличает итог с поиском
 *          средствами стандартными - совпадать они обязаны в точности,
 *          и совпадать при всяком тексте, ибо якорь выбирается самим текстом.
 *
 *          Путь этот включается лишь при остатке текста от четырёх килобайтов,
 *          отчего тексты проверки заведомо длиннее: на текстах коротких проверка
 *          мерила бы поиск обычный и правки не касалась бы вовсе.
 *
 */
TEST(Regex, StaticSeekAnchor) {
	/**
	 * @brief Набор текстов, поиск в каких выполняется
	 *
	 */
	vector <string> texts;
	// Создаём связный текст со знаками препинания, редкими в нём
	{
		// Создаём наполняемый текст сопоставления
		string text;
		// Выполняем размещение текста сопоставления
		text.reserve(0x10000);
		/**
		 * Выполняем наполнение текста сопоставления
		 */
		while(text.size() < 0xC000)
			// Выполняем добавление очередного участка текста
			text.append("the quick brown fox jumps over the lazy dog 1234 ");
		// Выполняем добавление искомых последовательностей у конца текста
		text.append("needle-in-haystack foxtrot forman@anyks.com needle 4096 ");
		// Выполняем добавление текста в набор
		texts.push_back(std::move(text));
	}
	// Создаём текст, знаками препинания насыщенный
	{
		// Создаём наполняемый текст сопоставления
		string text;
		// Выполняем размещение текста сопоставления
		text.reserve(0x10000);
		/**
		 * Выполняем наполнение текста сопоставления
		 */
		while(text.size() < 0xC000)
			// Выполняем добавление очередного участка текста
			text.append("{\"a-b\":[1,2],\"c@d\":\"e.f\"}\n");
		// Выполняем добавление искомых последовательностей у конца текста
		text.append("needle-in-haystack forman@anyks.com zzz");
		// Выполняем добавление текста в набор
		texts.push_back(std::move(text));
	}
	// Создаём текст из одного повторяющегося байта
	texts.push_back(string(0xC000, 'a') + "aab");
	// Создаём источник псевдослучайных значений
	mt19937 gen(20260816);
	// Создаём текст случайных байтов
	{
		// Создаём наполняемый текст сопоставления
		string text;
		// Выполняем размещение текста сопоставления
		text.reserve(0xC000);
		/**
		 * Выполняем наполнение текста сопоставления
		 */
		while(text.size() < 0xC000)
			// Выполняем добавление очередного байта текста
			text.append(1, static_cast <char> (gen() & 0xFF));
		// Выполняем добавление текста в набор
		texts.push_back(std::move(text));
	}
	/**
	 * @brief Набор искомых последовательностей
	 *
	 */
	const char * const NEEDLES[] = {
		"needle-in-haystack", "forman@anyks.com", "foxtrot", "needle",
		"the quick", "no-such-sequence-here", "zzz", "aab", "a", "ab",
		"dog 1234", "\"c@d\"", "}\n{", "", "over the lazy"
	};
	/**
	 * Выполняем перебор набора текстов поиска
	 */
	for(const auto & text : texts){
		// Получаем размер текста поиска
		const size_t size = text.size();
		/**
		 * Выполняем перебор набора искомых последовательностей
		 */
		for(const char * item : NEEDLES){
			// Получаем искомую последовательность
			const string_view what(item);
			/**
			 * Выполняем перебор позиций начала поиска
			 *
			 * @details Позиции берутся и нулевая, и произвольные, и предельные:
			 *          выбор якоря ведётся пробой остатка текста, а не текста
			 *          целиком, отчего от позиции начала поиска он и зависит.
			 *
			 */
			for(const size_t pos : {size_t(0), size_t(1), size_t(7), (size / 3), (size - 0x1000),
			 (size - 0x0800), (size - 1), size, (size + 1), string_view::npos}){
				/**
				 * Если позиция начала поиска за пределы текста выходит
				 */
				if(pos > size){
					// Выполняем проверку отказа поиска за пределами текста
					EXPECT_EQ(regex::seek(text, what, pos), string_view::npos) << item;
					// Переходим к следующей позиции начала поиска
					continue;
				}
				// Выполняем проверку совпадения итога поиска с поиском обычным
				EXPECT_EQ(regex::seek(text, what, pos), string_view(text).find(what, pos))
				 << "искомое «" << item << "» с позиции " << pos;
			}
		}
	}
}
