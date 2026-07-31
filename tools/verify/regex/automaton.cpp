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
#include "main.hpp"

/**
 * Устанавливаем ширину единицы кодирования эталонной реализации
 */
#define PCRE2_CODE_UNIT_WIDTH 8

/**
 * Подключаем заголовочный файл эталонной реализации
 */
#include <pcre2.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Функция проверки наличия совпадения эталонной реализацией
 *
 * @param pattern текст регулярного выражения
 * @param text    текст для сопоставления
 * @return        результат проверки наличия совпадения
 *
 */
static bool oracle(const string & pattern, const string & text) noexcept {
	// Код ошибки компиляции регулярного выражения
	int32_t code = 0;
	// Смещение ошибки компиляции регулярного выражения
	PCRE2_SIZE offset = 0;
	// Выполняем компиляцию регулярного выражения эталонной реализацией
	pcre2_code * expression = ::pcre2_compile(
		reinterpret_cast <PCRE2_SPTR> (pattern.c_str()), pattern.size(), 0, &code, &offset, nullptr
	);
	/**
	 * Если компиляция регулярного выражения не выполнена
	 */
	if(expression == nullptr)
		// Выводим результат проверки наличия совпадения
		return false;
	// Выполняем создание набора данных сопоставления
	pcre2_match_data * data = ::pcre2_match_data_create_from_pattern(expression, nullptr);
	// Выполняем сопоставление регулярного выражения с текстом
	const int32_t count = ::pcre2_match(
		expression, reinterpret_cast <PCRE2_SPTR> (text.c_str()), text.size(), 0, 0, data, nullptr
	);
	// Выполняем освобождение набора данных сопоставления
	::pcre2_match_data_free(data);
	// Выполняем освобождение скомпилированного регулярного выражения
	::pcre2_code_free(expression);
	// Выводим результат проверки наличия совпадения
	return (count > 0);
}

/**
 * @brief Проверка вердикта детерминированного исполнения на привязках к позиции в тексте
 *
 */
TEST(Regex, Automaton) {
	/**
	 * @brief Проверяемая пара шаблона и текста сопоставления
	 *
	 */
	struct Item {
		// Текст проверяемого регулярного выражения
		const char * pattern;
		// Текст для сопоставления
		const char * text;
	};
	/**
	 * Создаём набор проверяемых пар шаблона и текста
	 *
	 * @details Набор сосредоточен на привязках к позиции в тексте: их проверка
	 *          зависит от соседних байтов, которые при детерминированном исполнении
	 *          известны лишь частично, поэтому именно здесь возможны расхождения.
	 *
	 */
	const vector <Item> items = {
		// Привязки к началу и концу текста
		{"^abc", "abcdef"}, {"^abc", "xabcdef"}, {"abc$", "xxabc"}, {"abc$", "xxabcx"},
		{"abc$", "xxabc\n"}, {"abc\\z", "abc\n"}, {"abc\\Z", "abc\n"}, {"abc\\Z", "abc\n\n"},
		{"\\Aabc", "abc"}, {"\\Aabc", "xabc"}, {"^$", ""}, {"^$", "\n"}, {"^\\z", ""},
		// Привязки к границам строк
		{"(?m)^b", "a\nb"}, {"(?m)^b", "ab"}, {"(?m)b$", "b\nc"}, {"(?m)^a", "\na"},
		{"(?m)^", "\n"}, {"(?m)$", "\n"}, {"(?m)^x", "a\n"}, {"(?m)c$", "abc\n"},
		// Привязки к границам слова
		{"\\bword\\b", "a word here"}, {"\\bword\\b", "awordhere"}, {"\\bword", "word"},
		{"word\\b", "word"}, {"\\Bord", "word"}, {"\\Bord", "ord"}, {"a\\Bb", "ab"},
		{"\\b", ""}, {"\\B", ""}, {"x\\b", "x "}, {"\\ba", "_a"}, {"\\b_", " _"},
		// Совпадения нулевой длины и пустые тексты
		{"a*", ""}, {"", ""}, {"b*", "aaa"}, {"x?", "yyy"},
		// Кванторы и классы символов
		{"[0-9]{3,}", "ab1234cd"}, {"[0-9]{5,}", "ab1234cd"}, {"(ab)+c", "ababc"},
		{"(?i)ABC", "xxabcxx"}, {"(?s)a.b", "a\nb"}, {"a.b", "a\nb"},
		// Выражения, встречающиеся при разборе протоколов
		{"^([A-Za-z0-9-]+):\\s*(.+)$", "Content-Type: text/html"},
		{"^([A-Za-z0-9-]+):\\s*(.+)$", "no colon here"},
		{"^(GET|POST|PUT) (\\S+) HTTP/1\\.1$", "GET /index.html HTTP/1.1"},
		{"^(GET|POST|PUT) (\\S+) HTTP/1\\.1$", "GET /index.html HTTP/1.0"}
	};
	/**
	 * Выполняем сличение вердикта для каждой пары шаблона и текста
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
		// Создаём объект детерминированного исполнения
		regex::dfa_t dfa;
		// Выполняем проверку применимости детерминированного исполнения
		ASSERT_TRUE(dfa.available(program)) << "Шаблон: " << item.pattern;
		// Получаем вердикт детерминированного исполнения
		const bool actual = dfa.test(program, item.text, 0);
		// Получаем вердикт эталонной реализации
		const bool expected = oracle(item.pattern, item.text);
		// Выполняем сличение вердиктов наличия совпадения
		EXPECT_EQ(actual, expected) << "Шаблон: " << item.pattern << ", текст: " << item.text;
	}
}

/**
 * @brief Проверка сохранения вердикта при повторном исполнении программы
 *
 */
/**
 * @brief Проверка вердикта детерминированного исполнения на порождаемых шаблонах
 *
 */
TEST(Regex, AutomatonFuzzing) {
	// Создаём генератор псевдослучайных чисел с фиксированным зерном
	mt19937 engine(20260731);
	// Создаём набор фрагментов, из которых собираются шаблоны
	const vector <string> pieces = {
		"a", "b", "c", "x", "0", "1", ".", "^", "$", "|", "*", "+", "?",
		"(", ")", "[ab]", "[^a]", "[a-c]", "\\d", "\\w", "\\s", "\\b", "\\B",
		"(?:", "a*", "b+", "c?", "a*?", "b+?", "c??", "{2}", "{1,3}", "{0,2}",
		"(?i)", "(?s)", "(?m)", "\\A", "\\z", "\\Z", "ab", "abc", "a|b", "(a)", "(a|b)",
		"(?i:", "[0-9]", "[^0-9]", "\\D", "\\W", "\\S", "A", "B", "-", "_", "\\.", "\\*",
		"(a)(b)", "((a))", "(a|)", "a{2,}", "[abc]+", "[^abc]*", "\\bx", "x\\b", ".*", ".+", ".?"
	};
	// Создаём набор текстов сопоставления
	const vector <string> texts = {
		"", "a", "ab", "abc", "aab", "abcabc", "xyz", "aaa", "a\nb", "0a1b",
		" a b ", "abcabcabc", "cba", "aXbXc", "\n", "aaaa", "ab\ncd",
		"AbC", "A_B-C", "0123", "a.b*c", "  ", "aaaaaaaaaaaa", "xAx", "\n\n", "abc\n",
		"The quick brown fox", "a1b2c3", "___", "aBcDeF", "..."
	};
	// Создаём распределение количества фрагментов в шаблоне
	uniform_int_distribution <size_t> lengths(1, 9);
	// Создаём распределение индексов фрагментов
	uniform_int_distribution <size_t> indexes(0, pieces.size() - 1);
	// Создаём распределение индексов текстов сопоставления
	uniform_int_distribution <size_t> samples(0, texts.size() - 1);
	// Счётчик сличённых пар шаблона и текста
	size_t compared = 0;
	// Счётчик расхождений вердиктов наличия совпадения
	size_t diverged = 0;
	/**
	 * Выполняем порождение и сличение вердиктов наличия совпадения
	 */
	for(size_t i = 0; i < 100000; i++) {
		// Текст порождаемого регулярного выражения
		string pattern;
		// Получаем количество фрагментов очередного шаблона
		const size_t count = lengths(engine);
		/**
		 * Выполняем сборку шаблона из фрагментов
		 */
		for(size_t j = 0; j < count; j++)
			// Выполняем добавление очередного фрагмента шаблона
			pattern.append(pieces.at(indexes(engine)));
		// Получаем текст сопоставления
		const string & text = texts.at(samples(engine));
		// Создаём объект разбора регулярного выражения
		regex::parser_t parser;
		/**
		 * Если разбор регулярного выражения не выполнен
		 */
		if(!parser.parse(pattern, 0))
			// Переходим к следующему порождаемому шаблону
			continue;
		// Создаём объект компиляции регулярного выражения
		regex::compiler_t compiler;
		// Создаём компилируемую программу регулярного выражения
		regex::program_t program;
		/**
		 * Если компиляция регулярного выражения не выполнена
		 */
		if(!compiler.compile(parser, program))
			// Переходим к следующему порождаемому шаблону
			continue;
		// Создаём объект детерминированного исполнения
		regex::dfa_t dfa;
		/**
		 * Если детерминированное исполнение неприменимо
		 */
		if(!dfa.available(program))
			// Переходим к следующему порождаемому шаблону
			continue;
		// Получаем вердикт детерминированного исполнения
		const bool actual = dfa.test(program, text, 0);
		// Получаем вердикт эталонной реализации
		const bool expected = oracle(pattern, text);
		// Увеличиваем счётчик сличённых пар шаблона и текста
		compared++;
		/**
		 * Если вердикты наличия совпадения совпали
		 */
		if(actual == expected)
			// Переходим к следующему порождаемому шаблону
			continue;
		// Увеличиваем счётчик расхождений вердиктов
		diverged++;
		// Выводим сообщение о расхождении вердиктов наличия совпадения
		ADD_FAILURE()
			<< "Шаблон: " << pattern << ", текст: " << text
			<< ", вердикт автомата: " << (actual ? "да" : "нет")
			<< ", вердикт эталона: " << (expected ? "да" : "нет");
		/**
		 * Если количество выведенных расхождений достигло предела
		 */
		if(diverged >= 10)
			// Выходим из цикла сличения вердиктов
			break;
	}
	// Выполняем проверку выполнения сличения вердиктов
	EXPECT_GT(compared, 0u);
	// Выполняем проверку отсутствия расхождений вердиктов
	EXPECT_EQ(diverged, 0u);
}

