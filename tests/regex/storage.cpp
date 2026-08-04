/**
 * @file: storage.cpp
 * @date: 2026-08-04
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты хранилища собранных регулярных выражений —
 *        запись собранных выражений последовательностью байтов, восстановление
 *        их из записи и отказ восстановления записи испорченной
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <random>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>
#include <regex/storage.hpp>

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
 * @brief Функция порождения набора выражений для проверки хранилища
 *
 * @param regexp объект работы с регулярными выражениями
 * @param count  количество порождаемых выражений
 * @return       набор текстов порождённых выражений
 *
 */
static vector <string> samples(const size_t count) noexcept {
	// Набор текстов порождённых выражений
	vector <string> result;
	/**
	 * @brief Набор составляющих порождаемого выражения
	 *
	 */
	const char * atoms[] = {
		"a", "[a-z]", "\\d", "\\w", ".", "x", "(?:ab|cd)", "[^q]", "\\s",
		"(?<name>z)", "(?=a)", "(?<!b)", "\\bq", "(?>a|b)", "[[:alpha:]]"
	};
	/**
	 * @brief Набор кванторов повторения порождаемого выражения
	 *
	 */
	const char * repeats[] = {"", "*", "+", "?", "{2,4}", "*?", "+?", "{1,3}?", "*+"};
	// Создаём порождатель случайных значений с постоянным зерном
	mt19937 generator(20260804);
	// Выполняем размещение набора текстов порождённых выражений
	result.reserve(count);
	/**
	 * Выполняем порождение набора выражений
	 */
	for(size_t i = 0; i < count; i++) {
		// Текст порождаемого выражения
		string pattern;
		// Получаем количество составляющих порождаемого выражения
		const size_t length = (1 + (generator() % 5));
		/**
		 * Выполняем порождение составляющих выражения
		 */
		for(size_t j = 0; j < length; j++) {
			// Выполняем добавление составляющей выражения
			pattern.append(atoms[generator() % 15]);
			// Выполняем добавление квантора повторения
			pattern.append(repeats[generator() % 9]);
		}
		/**
		 * Если выражение получает захватывающую группу со ссылкой
		 */
		if((generator() % 4) == 0)
			// Выполняем оборачивание выражения захватывающей группой
			pattern = ("(" + pattern + ")\\1?");
		// Выполняем добавление текста выражения в набор
		result.push_back(::move(pattern));
	}
	// Выводим набор текстов порождённых выражений
	return result;
}
/**
 * @brief Тест записи и восстановления собранных выражений
 *
 * @details Восстановленное выражение сличается с собранным начисто по
 *          границам совпадения на наборе текстов: совпадение устройства
 *          записи проверяется поведением, а не сличением полей.
 *
 */
TEST(Regex, StorageRoundtrip) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Получаем набор текстов порождённых выражений
	const auto patterns = samples(500);
	// Набор собранных начисто выражений
	vector <regex::storage_t::exp_t> fresh;
	// Выполняем размещение набора собранных выражений
	fresh.reserve(patterns.size());
	/**
	 * Выполняем сборку набора выражений
	 */
	for(const auto & pattern : patterns) {
		// Выполняем сборку регулярного выражения
		const auto exp = regexp.build(pattern, {regexp_t::flag_t::DUPNAMES});
		/**
		 * Если сборка регулярного выражения выполнена
		 */
		if(exp)
			// Выполняем добавление собранного выражения в набор
			fresh.push_back(exp);
	}
	// Выполняем проверку сборки набора выражений
	ASSERT_FALSE(fresh.empty());
	// Запись хранилища собранных выражений
	string record;
	// Выполняем запись собранных выражений
	ASSERT_TRUE(storage.save(fresh, record)) << "код " << static_cast <uint32_t> (storage.error());
	// Выполняем проверку непустоты записи хранилища
	ASSERT_FALSE(record.empty());
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	// Выполняем восстановление собранных выражений
	ASSERT_TRUE(storage.load(record, restored)) << "код " << static_cast <uint32_t> (storage.error());
	// Выполняем проверку количества восстановленных выражений
	ASSERT_EQ(restored.size(), fresh.size());
	/**
	 * @brief Набор текстов сличения поведения выражений
	 *
	 */
	const vector <string> texts = {
		"", "a", "abc", "aaa 123 zzz", "x b00_\nb0", "qqqqqqqqqq", "abababab",
		"z", "-42.5e3", "\tмного текста\n", "AZaz09", "  ", "b", "cdcdcd"
	};
	/**
	 * Выполняем перебор набора восстановленных выражений
	 */
	for(size_t i = 0; i < fresh.size(); i++) {
		/**
		 * Выполняем перебор набора текстов сличения
		 */
		for(const auto & text : texts)
			// Выполняем сличение границ совпадения восстановленного выражения
			EXPECT_EQ(regexp.match(text, fresh.at(i)), regexp.match(text, restored.at(i)))
			 << "выражение \"" << patterns.at(i) << "\" на тексте \"" << text << "\"";
		// Выполняем сличение соответствия имён именованных групп
		EXPECT_EQ(regexp.groups(fresh.at(i)), regexp.groups(restored.at(i)));
		// Выполняем сличение количества захватывающих групп
		EXPECT_EQ(regexp.captures(fresh.at(i)), regexp.captures(restored.at(i)));
	}
}
/**
 * @brief Тест восстановления выражения с порождением машинного кода
 *
 * @details Порождённый машинный код в записи не хранится и порождается заново,
 *          поэтому восстановленное выражение обязано сопоставлять так же.
 *
 */
TEST(Regex, StorageMachine) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Выполняем сборку регулярного выражения с порождением машинного кода
	const auto exp = regexp.build("([a-z]+)@([a-z]+)\\.[a-z]{2,4}", {regexp_t::flag_t::JIT});
	// Выполняем проверку сборки регулярного выражения
	ASSERT_TRUE(!!exp);
	// Запись хранилища собранных выражений
	string record;
	// Выполняем запись собранных выражений
	ASSERT_TRUE(storage.save({exp}, record));
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	// Выполняем восстановление собранных выражений
	ASSERT_TRUE(storage.load(record, restored));
	// Выполняем проверку количества восстановленных выражений
	ASSERT_EQ(restored.size(), 1);
	// Выполняем сличение границ совпадения восстановленного выражения
	EXPECT_EQ(regexp.match("пишите на forman@anyks.com сегодня", exp),
	 regexp.match("пишите на forman@anyks.com сегодня", restored.front()));
	// Выполняем проверку извлечения захваченных групп
	EXPECT_EQ(regexp.exec("forman@anyks.com", restored.front()).size(), 3);
}
/**
 * @brief Тест отказа восстановления записи испорченной
 *
 * @details Запись приходит из источника, доверия не заслуживающего, поэтому
 *          порча её обязана оборачиваться отказом, а не блужданием по памяти.
 *
 */
TEST(Regex, StorageCorrupted) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Выполняем сборку регулярного выражения
	const auto exp = regexp.build("(?<year>\\d{4})-(\\d{2})-(\\d{2})[T ](?>[0-9:]+)");
	// Выполняем проверку сборки регулярного выражения
	ASSERT_TRUE(!!exp);
	// Запись хранилища собранных выражений
	string record;
	// Выполняем запись собранных выражений
	ASSERT_TRUE(storage.save({exp}, record));
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	// Выполняем проверку отказа восстановления пустой записи
	EXPECT_FALSE(storage.load("", restored));
	EXPECT_EQ(storage.error(), regex::storage_error_t::EMPTY);
	/**
	 * Выполняем проверку отказа восстановления записи с чужим опознанием
	 */
	{
		// Получаем запись с испорченным опознанием
		string damaged = record;
		// Выполняем порчу опознания записи
		damaged[0] = static_cast <char> (damaged[0] ^ 0xFF);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(storage.load(damaged, restored));
		EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_MAGIC);
	}
	/**
	 * Выполняем проверку отказа восстановления записи иной версии
	 */
	{
		// Получаем запись с испорченной версией устройства
		string damaged = record;
		// Выполняем порчу версии устройства записи
		damaged[8] = static_cast <char> (damaged[8] + 1);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(storage.load(damaged, restored));
		EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_VERSION);
	}
	/**
	 * Выполняем проверку отказа восстановления записи оборванной
	 */
	for(size_t length = 1; length < record.size(); length += 7) {
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(storage.load(string_view(record).substr(0, length), restored))
		 << "запись длиной " << length;
		// Выполняем проверку очистки набора восстановленных выражений
		EXPECT_TRUE(restored.empty());
	}
	/**
	 * Выполняем проверку отказа восстановления записи с испорченным содержимым
	 *
	 * @details Порча проверяется в каждом байте содержимого: отказ обязан быть
	 *          при всякой, а падения быть не обязано ни при какой.
	 */
	for(size_t i = 26; i < record.size(); i++) {
		// Получаем запись с испорченным содержимым
		string damaged = record;
		// Выполняем порчу очередного байта содержимого записи
		damaged[i] = static_cast <char> (damaged[i] ^ 0x5A);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(storage.load(damaged, restored)) << "испорчен байт " << i;
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_NE(storage.error(), regex::storage_error_t::NONE);
	}
}
/**
 * @brief Тест записи набора выражений
 *
 */
TEST(Regex, StorageMultiple) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Набор собранных выражений
	vector <regex::storage_t::exp_t> fresh;
	/**
	 * Выполняем сборку набора выражений
	 */
	for(const char * pattern : {"^a+$", "[0-9]{3}-[0-9]{2}", "(?i)ПрИвЕт", "\\p{Cyrillic}+"}) {
		// Выполняем сборку регулярного выражения
		const auto exp = regexp.build(pattern, {regexp_t::flag_t::UTF, regexp_t::flag_t::UCP});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(!!exp) << pattern;
		// Выполняем добавление собранного выражения в набор
		fresh.push_back(exp);
	}
	// Запись хранилища собранных выражений
	string record;
	// Выполняем запись собранных выражений
	ASSERT_TRUE(storage.save(fresh, record));
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	// Выполняем восстановление собранных выражений
	ASSERT_TRUE(storage.load(record, restored));
	// Выполняем проверку количества восстановленных выражений
	ASSERT_EQ(restored.size(), fresh.size());
	// Выполняем проверку сопоставления восстановленных выражений
	EXPECT_TRUE(regexp.test("aaa", restored.at(0)));
	EXPECT_FALSE(regexp.test("aab", restored.at(0)));
	EXPECT_TRUE(regexp.test("123-45", restored.at(1)));
	EXPECT_TRUE(regexp.test("ПРИВЕТ", restored.at(2)));
	EXPECT_TRUE(regexp.test("узел", restored.at(3)));
	EXPECT_FALSE(regexp.test("node", restored.at(3)));
	// Выполняем проверку отказа записи набора с пустой ссылкой
	EXPECT_FALSE(storage.save({nullptr}, record));
	EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_CONTENT);
}
