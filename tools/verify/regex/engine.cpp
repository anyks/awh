/**
 * @file engine.cpp
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
 * @brief Автоматические тесты движка регулярных выражений — сличение границ совпадения
 *        и захваченных групп с эталонной реализацией PCRE2 на текстах различной длины,
 *        покрывающее выбор способа сопоставления и поиск позиции начала совпадения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/engine.hpp>

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

using bounds_t = vector <pair <size_t, size_t>>;

/**
 * @brief Функция сопоставления регулярного выражения эталонной реализацией
 *
 * @param pattern текст регулярного выражения
 * @param text    текст для сопоставления
 * @param result  набор границ совпадения и захваченных групп
 * @return        результат поиска совпадения
 *
 */
static bool sample(const string & pattern, const string & text, bounds_t & result) noexcept {
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
		const PCRE2_SIZE * bounds = ::pcre2_get_ovector_pointer(data);
		/**
		 * Выполняем перенос границ совпадения и захваченных групп
		 */
		for(uint32_t i = 0; i < ::pcre2_get_ovector_count(data); i++)
			// Выполняем добавление границ очередного захвата
			result.emplace_back(bounds[i * 2], bounds[(i * 2) + 1]);
	}
	// Выполняем освобождение набора данных сопоставления
	::pcre2_match_data_free(data);
	// Выполняем освобождение скомпилированного регулярного выражения
	::pcre2_code_free(expression);
	// Выводим результат поиска совпадения
	return found;
}
/**
 * @brief Функция сличения наборов границ совпадения
 *
 * @param first  первый сличаемый набор границ совпадения
 * @param second второй сличаемый набор границ совпадения
 * @return       результат сличения наборов границ совпадения
 *
 */
static bool equal(const bounds_t & first, const bounds_t & second) noexcept {
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
	const bounds_t & rest = ((first.size() > second.size()) ? first : second);
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
 * @brief Проверка сличения границ совпадения на текстах различной длины
 *
 */
TEST(Regex, Engine) {
	/**
	 * @brief Проверяемое регулярное выражение и искомая последовательность
	 *
	 */
	struct Item {
		// Текст проверяемого регулярного выражения
		const char * pattern;
		// Последовательность, размещаемая в тексте сопоставления
		const char * needle;
	};
	// Создаём набор проверяемых регулярных выражений
	const vector <Item> items = {
		{"(\\w+)@(\\w+)\\.(\\w+)", "user@example.com"},
		{"^.*?(ERROR|WARN) ([0-9]{3,5}):", "ERROR 4041:"},
		{"([a-z]+)ing\\b", "running"},
		{"(?i)(HTTP)/(\\d)\\.(\\d)", "http/1.1"},
		{"\\b([A-Z][a-z]+) ([A-Z][a-z]+)\\b", "John Smith"},
		{"(a+)(b+)(c+)", "aaabbbccc"},
		{"([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})", "192.168.001.100"},
		{"(?m)^([A-Za-z-]+): (.+)$", "Content-Length: 42"}
	};
	/**
	 * Создаём набор длин текста сопоставления
	 *
	 * @details Длины подобраны так, чтобы покрыть оба способа установки границ
	 *          совпадения: исполнение без возврата на коротких текстах и поиск
	 *          позиции начала совпадения проходом в обратном направлении на длинных.
	 *
	 */
	const vector <size_t> sizes = {0, 1, 40, 600, 5000, 60000};
	// Создаём основу текста сопоставления
	const string filler = "the quick brown fox jumps over the lazy dog 12345 ";
	/**
	 * Выполняем сличение границ совпадения для каждого выражения
	 */
	for(const auto & item : items) {
		/**
		 * Выполняем сличение границ совпадения для каждой длины текста
		 */
		for(const size_t target : sizes) {
			// Создаём основу текста сопоставления заданной длины
			string base;
			/**
			 * Выполняем наполнение основы текста сопоставления
			 */
			while(base.size() < target)
				// Выполняем добавление основы текста сопоставления
				base.append(filler);
			// Выполняем усечение основы текста до заданной длины
			base.resize(target);
			/**
			 * Выполняем сличение границ совпадения для каждого положения искомого
			 */
			for(size_t place = 0; place < 3; place++) {
				// Создаём текст сопоставления
				string text = base;
				// Определяем положение размещения искомой последовательности
				const size_t at = ((place == 0) ? 0 : ((place == 1) ? (text.size() / 2) : text.size()));
				// Выполняем размещение искомой последовательности в тексте
				text.insert(at, item.needle);
				// Создаём объект движка регулярных выражений
				regex::engine_t engine;
				// Выполняем сборку регулярного выражения
				ASSERT_TRUE(engine.build(item.pattern, 0)) << "Шаблон: " << item.pattern;
				// Набор границ совпадения движка регулярных выражений
				bounds_t ours;
				// Набор границ совпадения эталонной реализации
				bounds_t theirs;
				// Получаем результат сопоставления движком регулярных выражений
				const bool actual = engine.exec(text, 0, ours);
				// Получаем результат сопоставления эталонной реализацией
				const bool expected = sample(item.pattern, text, theirs);
				// Выполняем сличение результатов сопоставления
				ASSERT_EQ(actual, expected)
					<< "Шаблон: " << item.pattern << ", длина текста: " << text.size()
					<< ", положение искомого: " << place;
				/**
				 * Если совпадение обнаружено обеими реализациями
				 */
				if(actual)
					// Выполняем сличение границ совпадения и захваченных групп
					EXPECT_TRUE(equal(ours, theirs))
						<< "Шаблон: " << item.pattern << ", длина текста: " << text.size()
						<< ", положение искомого: " << place
						<< ", границы движка: [" << ours.front().first << "," << ours.front().second << "]"
						<< ", границы эталона: [" << theirs.front().first << "," << theirs.front().second << "]";
			}
		}
	}
}

