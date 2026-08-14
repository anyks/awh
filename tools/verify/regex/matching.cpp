/**
 * @file matching.cpp
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
 * @brief Автоматические тесты сопоставления регулярных выражений — сличение границ совпадения
 *        и захваченных групп с эталонной реализацией PCRE2 на наборе шаблонов регулярного
 *        подмножества и на шаблонах, порождаемых псевдослучайным образом
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

using result_t = vector <pair <size_t, size_t>>;

/**
 * @brief Функция сопоставления регулярного выражения эталонной реализацией
 *
 * @param pattern текст регулярного выражения
 * @param text    текст для сопоставления
 * @param result  набор границ совпадения и захваченных групп
 * @return        результат поиска совпадения
 *
 */
static bool reference(const string & pattern, const string & text, result_t & result) noexcept {
	// Код ошибки компиляции регулярного выражения
	int32_t code = 0;
	// Смещение ошибки компиляции регулярного выражения
	PCRE2_SIZE offset = 0;
	// Выполняем очистку набора границ совпадения
	result.clear();
	// Выполняем компиляцию регулярного выражения эталонной реализацией
	pcre2_code * expression = ::pcre2_compile(
		reinterpret_cast <PCRE2_SPTR> (pattern.c_str()), pattern.size(), 0, &code, &offset, nullptr
	);
	/**
	 * Если компиляция регулярного выражения не выполнена
	 */
	if(expression == nullptr)
		// Выводим результат поиска совпадения
		return false;
	// Выполняем создание набора данных сопоставления
	pcre2_match_data * data = ::pcre2_match_data_create_from_pattern(expression, nullptr);
	// Выполняем сопоставление регулярного выражения с текстом
	const int32_t count = ::pcre2_match(
		expression, reinterpret_cast <PCRE2_SPTR> (text.c_str()), text.size(), 0, 0, data, nullptr
	);
	// Флаг обнаружения совпадения
	bool found = false;
	/**
	 * Если совпадение обнаружено
	 */
	if(count > 0) {
		// Выполняем установку флага обнаружения совпадения
		found = true;
		// Получаем набор границ совпадения эталонной реализации
		const PCRE2_SIZE * vector = ::pcre2_get_ovector_pointer(data);
		// Получаем количество границ совпадения эталонной реализации
		const uint32_t total = ::pcre2_get_ovector_count(data);
		/**
		 * Выполняем перенос границ совпадения и захваченных групп
		 */
		for(uint32_t i = 0; i < total; i++)
			// Выполняем добавление границ очередного захвата
			result.emplace_back(vector[i * 2], vector[(i * 2) + 1]);
	}
	// Выполняем освобождение набора данных сопоставления
	::pcre2_match_data_free(data);
	// Выполняем освобождение скомпилированного регулярного выражения
	::pcre2_code_free(expression);
	// Выводим результат поиска совпадения
	return found;
}
/**
 * @brief Функция сопоставления регулярного выражения модулем регулярных выражений
 *
 * @param pattern   текст регулярного выражения
 * @param text      текст для сопоставления
 * @param result    набор границ совпадения и захваченных групп
 * @param supported флаг принадлежности выражения регулярному подмножеству
 * @return          результат поиска совпадения
 *
 */
static bool actual(const string & pattern, const string & text, result_t & result, bool & supported) noexcept {
	// Выполняем очистку набора границ совпадения
	result.clear();
	// Выполняем установку флага принадлежности регулярному подмножеству
	supported = true;
	// Создаём объект разбора регулярного выражения
	regex::parser_t parser;
	/**
	 * Если разбор регулярного выражения не выполнен
	 */
	if(!parser.parse(pattern, 0)) {
		// Выполняем сброс флага принадлежности регулярному подмножеству
		supported = false;
		// Выводим результат поиска совпадения
		return false;
	}
	// Создаём объект компиляции регулярного выражения
	regex::compiler_t compiler;
	// Создаём компилируемую программу регулярного выражения
	regex::program_t program;
	/**
	 * Если компиляция регулярного выражения не выполнена
	 */
	if(!compiler.compile(parser, program)) {
		// Выполняем сброс флага принадлежности регулярному подмножеству
		supported = false;
		// Выводим результат поиска совпадения
		return false;
	}
	// Создаём объект исполнения регулярного выражения
	regex::pike_t pike;
	// Выводим результат сопоставления регулярного выражения с текстом
	return pike.exec(program, text, 0, result);
}
/**
 * @brief Функция сличения наборов границ совпадения
 *
 * @details Количество границ, сообщаемых реализациями, может различаться:
 *          отсутствующие границы считаются незахваченными.
 *
 * @param first  первый сличаемый набор границ совпадения
 * @param second второй сличаемый набор границ совпадения
 * @return       результат сличения наборов границ совпадения
 *
 */
static bool same(const result_t & first, const result_t & second) noexcept {
	// Получаем количество сличаемых границ совпадения
	const size_t count = ((first.size() < second.size()) ? first.size() : second.size());
	/**
	 * Выполняем сличение границ совпадения
	 */
	for(size_t i = 0; i < count; i++) {
		/**
		 * Если границы очередного захвата различаются
		 */
		if((first.at(i).first != second.at(i).first) || (first.at(i).second != second.at(i).second))
			// Выводим результат сличения наборов границ совпадения
			return false;
	}
	// Получаем набор границ совпадения, содержащий несличённые границы
	const result_t & rest = ((first.size() > second.size()) ? first : second);
	/**
	 * Выполняем проверку несличённых границ совпадения
	 */
	for(size_t i = count; i < rest.size(); i++) {
		/**
		 * Если несличённая граница совпадения содержит захват
		 */
		if(rest.at(i).first != string_view::npos)
			// Выводим результат сличения наборов границ совпадения
			return false;
	}
	// Выводим результат сличения наборов границ совпадения
	return true;
}

/**
 * @brief Проверка сличения границ совпадения на шаблонах регулярного подмножества
 *
 */
TEST(Regex, Matching) {
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
	// Создаём набор проверяемых пар шаблона и текста
	const vector <Item> items = {
		// Литералы и последовательности
		{"abc", "xxabcyy"}, {"abc", "xxabyy"}, {"a", ""}, {"", "abc"},
		// Кванторы повторения и их жадность
		{"a*", "aaa"}, {"a*?", "aaa"}, {"a+", "baaa"}, {"a+?", "baaa"},
		{"a?", "b"}, {"a{2}", "aaa"}, {"a{2,}", "aaaa"}, {"a{1,3}", "aaaaa"},
		{"a{2,3}?", "aaaa"}, {".*", "abc"}, {".*?", "abc"}, {".+b", "aabab"},
		// Классы символов
		{"[abc]+", "xxabcabxx"}, {"[^abc]+", "abxyzab"}, {"[a-z0-9]+", "__abc123__"},
		{"\\d+", "abc12345def"}, {"\\w+", "  hello  "}, {"\\s+", "ab   cd"},
		{"\\D+", "123abc456"}, {"\\W+", "ab..cd"}, {"\\S+", "  word  "},
		// Выбор одной из ветвей и приоритет
		{"a|ab", "ab"}, {"ab|a", "ab"}, {"(a|ab)c", "abc"}, {"x|y|z", "aaz"},
		// Группы и захваты
		{"(a)(b)(c)", "abc"}, {"((a)(b))", "ab"}, {"(a)*", "aaa"}, {"(a|b)+", "abab"},
		{"(?:ab)+", "ababab"}, {"(a)(?:b)(c)", "abc"}, {"(a)?b", "b"},
		{"^([A-Za-z0-9-]+):\\s*(.+)$", "Content-Type: text/html"},
		{"(\\w+)@(\\w+)\\.(\\w+)", "mail: user@example.com"},
		// Привязки к позиции в тексте
		{"^abc", "abcdef"}, {"^abc", "xabcdef"}, {"abc$", "xxabc"}, {"abc$", "xxabcx"},
		{"\\Aabc", "abc"}, {"abc\\z", "abc"}, {"abc\\Z", "abc\n"}, {"\\bword\\b", "a word here"},
		{"\\bword\\b", "awordhere"}, {"\\Bord", "word"},
		// Режимы компиляции
		{"(?i)ABC", "xxabcxx"}, {"(?i)[a-z]+", "ABC"}, {"(?s).+", "a\nb"}, {"(?m)^b", "a\nb"},
		{"(?m)b$", "b\nc"}, {"(?i)(?:AbC)+", "abcABC"},
		// Совпадения нулевой длины
		{"b*", "aaa"}, {"()", "abc"}, {"(a*)", "bbb"}, {"x?", "yyy"}
	};
	/**
	 * Выполняем сличение границ совпадения для каждой пары
	 */
	for(const auto & item : items) {
		// Набор границ совпадения модуля регулярных выражений
		result_t ours;
		// Набор границ совпадения эталонной реализации
		result_t theirs;
		// Флаг принадлежности выражения регулярному подмножеству
		bool supported = true;
		// Получаем результат сопоставления модулем регулярных выражений
		const bool first = actual(item.pattern, item.text, ours, supported);
		// Выполняем проверку принадлежности выражения регулярному подмножеству
		ASSERT_TRUE(supported) << "Шаблон: " << item.pattern;
		// Получаем результат сопоставления эталонной реализацией
		const bool second = reference(item.pattern, item.text, theirs);
		// Выполняем сличение результатов сопоставления
		EXPECT_EQ(first, second) << "Шаблон: " << item.pattern << ", текст: " << item.text;
		/**
		 * Если совпадение обнаружено обеими реализациями
		 */
		if(first && second)
			// Выполняем сличение границ совпадения и захваченных групп
			EXPECT_TRUE(same(ours, theirs))
				<< "Шаблон: " << item.pattern << ", текст: " << item.text
				<< ", границы модуля: [" << ours.front().first << "," << ours.front().second << "]"
				<< ", границы эталона: [" << theirs.front().first << "," << theirs.front().second << "]";
	}
}

/**
 * @brief Проверка отнесения выражений к нерегулярному подмножеству
 *
 */
/**
 * @brief Проверка сличения границ совпадения на порождаемых шаблонах
 *
 */
TEST(Regex, MatchingFuzzing) {
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
	// Счётчик расхождений результатов сопоставления
	size_t diverged = 0;
	/**
	 * Выполняем порождение и сличение результатов сопоставления
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
		// Набор границ совпадения модуля регулярных выражений
		result_t ours;
		// Набор границ совпадения эталонной реализации
		result_t theirs;
		// Флаг принадлежности выражения регулярному подмножеству
		bool supported = true;
		// Получаем результат сопоставления модулем регулярных выражений
		const bool first = actual(pattern, text, ours, supported);
		/**
		 * Если выражение регулярному подмножеству не принадлежит
		 */
		if(!supported)
			// Переходим к следующему порождаемому шаблону
			continue;
		// Получаем результат сопоставления эталонной реализацией
		const bool second = reference(pattern, text, theirs);
		// Увеличиваем счётчик сличённых пар шаблона и текста
		compared++;
		/**
		 * Если результаты сопоставления совпали
		 */
		if((first == second) && (!first || same(ours, theirs)))
			// Переходим к следующему порождаемому шаблону
			continue;
		// Увеличиваем счётчик расхождений результатов сопоставления
		diverged++;
		// Выводим сообщение о расхождении результатов сопоставления
		ADD_FAILURE()
			<< "Шаблон: " << pattern << ", текст: " << text
			<< ", вердикт модуля: " << (first ? "да" : "нет")
			<< ", вердикт эталона: " << (second ? "да" : "нет");
		/**
		 * Если количество выведенных расхождений достигло предела
		 */
		if(diverged >= 10)
			// Выходим из цикла сличения результатов сопоставления
			break;
	}
	// Выполняем проверку выполнения сличения результатов сопоставления
	EXPECT_GT(compared, 0u);
	// Выполняем проверку отсутствия расхождений результатов сопоставления
	EXPECT_EQ(diverged, 0u);
}

