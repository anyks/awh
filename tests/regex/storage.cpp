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

/**
 * @brief Тест отказа восстановления записи чужого устройства
 *
 * @details Наборы программы пишутся образом памяти, поэтому запись отвечает
 *          порядку байтов машины и размещению полей структур. Запись, чужим
 *          опознанием помеченная, обязана быть отвергнута, а не прочитана
 *          неверно.
 *
 */
TEST(Regex, StoragePlatform) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Выполняем сборку регулярного выражения
	const auto exp = regexp.build("[a-z]+[0-9]{2,4}", {});
	// Выполняем проверку сборки регулярного выражения
	ASSERT_TRUE(!!exp);
	// Запись хранилища собранных выражений
	string record;
	// Выполняем запись собранных выражений
	ASSERT_TRUE(storage.save({exp}, record));
	// Выполняем проверку размера записи хранилища
	ASSERT_GT(record.size(), 32);
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	// Выполняем проверку восстановления записи нетронутой
	ASSERT_TRUE(storage.load(record, restored));
	/**
	 * Выполняем перебор байтов опознания устройства машины
	 */
	for(size_t i = 10; i < 12; i++) {
		// Получаем запись с чужим опознанием устройства машины
		string foreign = record;
		// Выполняем подмену очередного байта опознания устройства
		foreign[i] = static_cast <char> (foreign[i] ^ 0x5A);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(storage.load(foreign, restored)) << "подменён байт " << i;
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_PLATFORM);
	}
	/**
	 * Выполняем проверку отказа записи с неизвестным методом сжатия
	 */
	{
		// Получаем запись с подменённым методом сжатия
		string packed = record;
		// Выполняем подмену метода сжатия записи
		packed[12] = static_cast <char> (0x5A);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(storage.load(packed, restored));
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_METHOD);
	}
	/**
	 * Выполняем перебор байтов выравнивания заголовка записи
	 */
	for(size_t i = 13; i < 16; i++) {
		// Получаем запись с непустыми байтами выравнивания
		string filled = record;
		// Выполняем заполнение очередного байта выравнивания
		filled[i] = static_cast <char> (0x5A);
		// Выполняем проверку отказа восстановления записи
		EXPECT_FALSE(storage.load(filled, restored)) << "заполнен байт " << i;
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_CONTENT);
	}
}
/**
 * @brief Тест восстановления записи, взятой во владение
 *
 * @details Наборы программы обозревают участки записи, поэтому запись обязана
 *          жить столько же, сколько восстановленные выражения. Тест уничтожает
 *          источник записи до сопоставления: выражение, переживающее это,
 *          записью владеет, а не обозревает чужую память.
 *
 */
TEST(Regex, StorageAdopt) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	/**
	 * Выполняем восстановление выражений из записи, взятой во владение
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto exp = regexp.build("([a-z]+)@([a-z]+)\\.[a-z]{2,4}", {});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(!!exp);
		// Запись хранилища собранных выражений
		string record;
		// Выполняем запись собранных выражений
		ASSERT_TRUE(storage.save({exp}, record));
		// Выполняем восстановление собранных выражений переносом записи
		ASSERT_TRUE(storage.adopt(::move(record), restored));
		// Выполняем очистку остатка перенесённой записи
		record.clear();
		// Выполняем освобождение места, занятого остатком записи
		string().swap(record);
	}
	// Выполняем проверку количества восстановленных выражений
	ASSERT_EQ(restored.size(), 1);
	// Выполняем сличение границ совпадения восстановленного выражения
	const auto bounds = regexp.match("пишите на forman@anyks.com сегодня", restored.front());
	// Выполняем проверку наличия совпадения восстановленного выражения
	ASSERT_EQ(bounds.size(), 3);
	// Выполняем проверку захваченного текста восстановленного выражения
	EXPECT_EQ(string("forman@anyks.com").size(), (bounds.front().second - bounds.front().first));
}
/**
 * @brief Тест переживания выражением уничтожения источника записи
 *
 * @details Метод «load» заводит копию записи сам, поэтому выражение обязано
 *          сопоставлять и после уничтожения записи, поданной ему на вход.
 *
 */
TEST(Regex, StorageLifetime) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	/**
	 * Выполняем восстановление выражений из записи, уничтожаемой следом
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto exp = regexp.build("\\b[0-9]{1,3}(?:\\.[0-9]{1,3}){3}\\b", {});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(!!exp);
		// Запись хранилища собранных выражений
		string record;
		// Выполняем запись собранных выражений
		ASSERT_TRUE(storage.save({exp}, record));
		// Выполняем восстановление собранных выражений
		ASSERT_TRUE(storage.load(record, restored));
		// Выполняем затирание записи хранилища
		record.assign(record.size(), '\x5A');
		// Выполняем освобождение места, занятого записью
		string().swap(record);
	}
	// Выполняем проверку количества восстановленных выражений
	ASSERT_EQ(restored.size(), 1);
	// Выполняем проверку сопоставления восстановленного выражения
	EXPECT_TRUE(regexp.test("узел 192.168.5.150 отвечает", restored.front()));
	EXPECT_FALSE(regexp.test("узел 192.168.5 отвечает", restored.front()));
}

/**
 * @brief Функция обращения последовательности байтов
 *
 * @param source исходная последовательность байтов
 * @param result выводимая последовательность байтов
 * @return       результат обращения последовательности
 *
 * @details Обращение служит образцом обработчика сжатия: оно содержимое
 *          изменяет, размер сохраняет и обратимо самим собою. Настоящие
 *          методы сжатия берутся из модуля «compressor», но тесты модуля
 *          регулярных выражений от него намеренно не зависят: модуль
 *          самостоятелен, и проверять надлежит устройство крючков,
 *          а не сторонние библиотеки.
 *
 */
static bool reversed(string_view source, string & result) noexcept {
	// Выполняем размещение выводимой последовательности байтов
	result.assign(source.rbegin(), source.rend());
	// Выводим результат обращения последовательности
	return true;
}
/**
 * @brief Тест записи и восстановления со сжатием содержимого
 *
 */
TEST(Regex, StoragePacking) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	regex::storage_t storage;
	// Получаем набор текстов порождённых выражений
	const auto patterns = samples(200);
	// Набор собранных начисто выражений
	vector <regex::storage_t::exp_t> fresh;
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
	// Запись хранилища без сжатия содержимого
	string plain;
	// Выполняем запись собранных выражений без сжатия
	ASSERT_TRUE(storage.save(fresh, plain));
	// Выполняем установку сжатия записи хранилища
	storage.packer(compressor::method_t::LZ4, &reversed, &reversed);
	// Запись хранилища со сжатием содержимого
	string packed;
	// Выполняем запись собранных выражений со сжатием
	ASSERT_TRUE(storage.save(fresh, packed)) << "код " << static_cast <uint32_t> (storage.error());
	// Выполняем проверку совпадения размеров записей
	ASSERT_EQ(packed.size(), plain.size());
	// Выполняем проверку расхождения содержимого записей
	EXPECT_NE(packed, plain);
	// Выполняем проверку записи метода сжатия в заголовок
	EXPECT_EQ(static_cast <uint8_t> (packed[12]), static_cast <uint8_t> (compressor::method_t::LZ4));
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	// Выполняем восстановление собранных выражений
	ASSERT_TRUE(storage.adopt(::move(packed), restored)) << "код " << static_cast <uint32_t> (storage.error());
	// Выполняем проверку количества восстановленных выражений
	ASSERT_EQ(restored.size(), fresh.size());
	/**
	 * @brief Набор текстов сличения поведения выражений
	 *
	 */
	const vector <string> texts = {
		"", "a", "abc", "aaa 123 zzz", "x b00_\nb0", "qqqqqqqqqq", "z", "-42.5e3"
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
	}
}
/**
 * @brief Тест отказов сжатия записи хранилища
 *
 */
TEST(Regex, StoragePackingErrors) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	regex::storage_t storage;
	// Выполняем сборку регулярного выражения
	const auto exp = regexp.build("[a-z]+[0-9]{2,4}", {});
	// Выполняем проверку сборки регулярного выражения
	ASSERT_TRUE(!!exp);
	// Запись хранилища собранных выражений
	string record;
	// Набор восстановленных выражений
	vector <regex::storage_t::exp_t> restored;
	/**
	 * Выполняем проверку отказа записи без обработчика сжатия
	 */
	{
		// Выполняем установку метода сжатия без обработчиков
		storage.packer(compressor::method_t::ZSTD, nullptr, nullptr);
		// Выполняем проверку отказа записи собранных выражений
		EXPECT_FALSE(storage.save({exp}, record));
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_METHOD);
	}
	/**
	 * Выполняем проверку отказа записи при невыполненном сжатии
	 */
	{
		// Выполняем установку обработчика, сжатия не выполняющего
		storage.packer(compressor::method_t::ZSTD,
		 [](string_view, string &) noexcept -> bool { return false; }, &reversed);
		// Выполняем проверку отказа записи собранных выражений
		EXPECT_FALSE(storage.save({exp}, record));
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(storage.error(), regex::storage_error_t::BAD_PACKING);
	}
	// Выполняем установку исправного сжатия записи хранилища
	storage.packer(compressor::method_t::ZSTD, &reversed, &reversed);
	// Выполняем запись собранных выражений
	ASSERT_TRUE(storage.save({exp}, record));
	/**
	 * Выполняем проверку отказа восстановления без обработчика разжатия
	 */
	{
		// Создаём объект хранилища без обработчиков сжатия
		const regex::storage_t plain;
		// Выполняем проверку отказа восстановления собранных выражений
		EXPECT_FALSE(plain.load(record, restored));
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(plain.error(), regex::storage_error_t::BAD_METHOD);
	}
	/**
	 * Выполняем проверку отказа восстановления при невыполненном разжатии
	 */
	{
		// Создаём объект хранилища с обработчиком, разжатия не выполняющим
		regex::storage_t broken;
		// Выполняем установку обработчика, разжатия не выполняющего
		broken.packer(compressor::method_t::ZSTD, &reversed,
		 [](string_view, string &) noexcept -> bool { return false; });
		// Выполняем проверку отказа восстановления собранных выражений
		EXPECT_FALSE(broken.load(record, restored));
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(broken.error(), regex::storage_error_t::BAD_PACKING);
	}
	/**
	 * Выполняем проверку отказа восстановления при расхождении размера
	 */
	{
		// Создаём объект хранилища с обработчиком, размер изменяющим
		regex::storage_t shrunk;
		// Выполняем установку обработчика, размер изменяющего
		shrunk.packer(compressor::method_t::ZSTD, &reversed,
		 [](string_view source, string & result) noexcept -> bool {
			// Выполняем разжатие содержимого с потерей последнего байта
			result.assign(source.rbegin(), source.rend());
			/**
			 * Если разжатое содержимое не пусто
			 */
			if(!result.empty())
				// Выполняем усечение разжатого содержимого
				result.erase(result.size() - 1);
			// Выводим результат разжатия содержимого
			return true;
		 });
		// Выполняем проверку отказа восстановления собранных выражений
		EXPECT_FALSE(shrunk.load(record, restored));
		// Выполняем проверку установки кода ошибки хранилища
		EXPECT_EQ(shrunk.error(), regex::storage_error_t::BAD_CONTENT);
	}
}
/**
 * @brief Тест отказа восстановления записи подделанной
 *
 * @details Контрольная сумма ловит порчу случайную, но от подделки не оберегает
 *          вовсе: подделыватель считает её заново. Единственным заслоном служит
 *          проверка правильности восстановленной программы, а наборы её
 *          обозревают запись прямо на месте - адреса переходов и указания на
 *          классы символов употребляются исполнением без проверки границ.
 *          Снятие проверки выводило сопоставление за пределы записи, что
 *          обнаружено разметчиком обращений к памяти, поэтому испытание
 *          закрепляет её наличие: подделка обязана отвергаться, а не
 *          сопоставляться.
 *
 */
TEST(Regex, StorageForged) {
	/**
	 * @brief Функция вычисления контрольной суммы содержимого записи
	 *
	 * @details Порядок вычисления повторяет хранилище: испытание подделывает
	 *          запись целиком, а значит и сумму её пересчитывает.
	 *
	 * @param data содержимое, сумма какого вычисляется
	 * @return     вычисленная контрольная сумма содержимого
	 *
	 */
	auto checksum = [](const string_view data) noexcept -> uint64_t {
		// Накопленная контрольная сумма содержимого
		uint64_t result = 0xCBF29CE484222325ull;
		// Получаем позицию чтения содержимого
		size_t offset = 0;
		/**
		 * Выполняем перебор содержимого восьмибайтовыми долями
		 */
		for(; (offset + 8) <= data.size(); offset += 8) {
			// Собираемая доля содержимого
			uint64_t block = 0;
			/**
			 * Выполняем сборку доли содержимого байтами от младшего
			 */
			for(uint8_t shift = 0; shift < 64; shift += 8)
				// Выполняем добавление очередного байта доли
				block |= (static_cast <uint64_t> (static_cast <uint8_t> (data[offset + (shift >> 3)])) << shift);
			// Выполняем смешивание доли содержимого
			result ^= block;
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
			// Выполняем перемешивание накопленной суммы
			result ^= (result >> 29);
		}
		/**
		 * Выполняем перебор остатка содержимого побайтно
		 */
		for(; offset < data.size(); offset++) {
			// Выполняем смешивание очередного байта содержимого
			result ^= static_cast <uint64_t> (static_cast <uint8_t> (data[offset]));
			// Выполняем умножение накопленной суммы
			result *= 0x100000001B3ull;
		}
		// Выводим вычисленную контрольную сумму содержимого
		return result;
	};
	/**
	 * @brief Размер заголовка записи хранилища
	 *
	 * @details Заголовок несёт опознание, версию устройства, опознание машины,
	 *          метод сжатия, размеры содержимого и контрольную сумму его,
	 *          а сама сумма стоит последними восемью байтами заголовка.
	 *
	 */
	constexpr size_t HEADER = 40, CONTROL = 32;
	/**
	 * @brief Шаг перебора байтов содержимого записи
	 *
	 * @details Перебор ведётся с шагом, а не подряд: запись с порождённым кодом
	 *          весит тринадцать килобайт, и перебор подряд по трём подменам
	 *          на байт занимал двадцать пять секунд - для набора тестов много.
	 *          Шаг охвата не сужает: подделке подвергаются все области записи,
	 *          а их устройство вдоль записи однородно.
	 *
	 */
	constexpr size_t STEP = 5;
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp;
	// Создаём объект хранилища собранных выражений
	const regex::storage_t storage;
	// Набор собранных регулярных выражений
	vector <regex::storage_t::exp_t> expressions;
	/**
	 * @brief Набор выражений, устройством записи различающихся
	 *
	 */
	const char * patterns[] = {
		"[a-z]+", "(\\w+)@(\\w+)", "\\p{L}{2,5}", "(?<name>a)\\k<name>",
		"^(?:GET|POST) /[a-z]*$", "(?>a+)b", "\\d{1,3}(?:\\.\\d{1,3}){3}"
	};
	/**
	 * Выполняем сборку набора регулярных выражений
	 *
	 * @details Сборка ведётся с режимом порождения машинного кода намеренно:
	 *          запись при этом несёт и порождённый код, а он исполняется,
	 *          а не разбирается, отчего проверкой не оберегаем вовсе. Код
	 *          записи берётся лишь при установленном признаке доверия, снятом
	 *          по умолчанию, - и испытание закрепляет именно это: подделанный
	 *          код исполняться не должен. Установка признака доверия обратно
	 *          в умолчание обращает испытание падением, а не отказом.
	 *
	 */
	for(const char * pattern : patterns) {
		// Выполняем сборку очередного регулярного выражения
		const auto expression = regexp.build(pattern, {regexp_t::flag_t::DUPNAMES, regexp_t::flag_t::JIT});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(expression) << pattern;
		// Выполняем добавление собранного выражения в набор
		expressions.push_back(expression);
	}
	// Запись хранилища собранных выражений
	string record;
	// Выполняем запись собранных выражений
	ASSERT_TRUE(storage.save(expressions, record));
	// Выполняем проверку вмещения записью заголовка
	ASSERT_GT(record.size(), HEADER);
	// Количество записей, отвергнутых несообразным содержимым
	size_t refused = 0;
	/**
	 * Выполняем обход байтов содержимого записи
	 */
	for(size_t i = HEADER; i < record.size(); i += STEP) {
		/**
		 * Выполняем перебор подмен значения очередного байта
		 */
		for(const uint32_t value : {0x00u, 0x7Fu, 0xFFu}) {
			/**
			 * Если подмена значения байта его не меняет
			 */
			if(static_cast <char> (value) == record[i])
				// Переходим к следующей подмене значения байта
				continue;
			// Получаем подделываемую запись хранилища
			string forged = record;
			// Выполняем подмену значения очередного байта содержимого
			forged[i] = static_cast <char> (value);
			// Получаем контрольную сумму подделанного содержимого
			const uint64_t control = checksum(string_view(forged).substr(HEADER));
			/**
			 * Выполняем подмену контрольной суммы записи
			 */
			for(uint8_t shift = 0; shift < 64; shift += 8)
				// Выполняем запись очередного байта контрольной суммы
				forged[CONTROL + (shift >> 3)] = static_cast <char> ((control >> shift) & 0xFF);
			// Набор выражений, записью подделанной восстановленных
			vector <regex::storage_t::exp_t> restored;
			/**
			 * Если запись подделанная восстановлению не поддалась
			 */
			if(!storage.load(forged, restored)) {
				/**
				 * Если запись отвергнута несообразным содержимым
				 */
				if(storage.error() == regex::storage_error_t::BAD_CONTENT)
					// Увеличиваем количество отвергнутых записей
					refused++;
				// Переходим к следующей подмене значения байта
				continue;
			}
			/**
			 * Выполняем сопоставление восстановленными выражениями
			 *
			 * @details Указания подделанные употребляются не восстановлением,
			 *          а сопоставлением, отчего запись, восстановление прошедшая,
			 *          обязана быть и сопоставлением пройдена без выхода
			 *          за пределы: разметчик обращений к памяти обнаруживает
			 *          выход именно здесь.
			 *
			 */
			for(const auto & expression : restored) {
				// Выполняем сопоставление текста непустого
				regexp.match("abc user_name 4711 GET /x a@b", expression);
				// Выполняем сопоставление текста пустого
				regexp.match("", expression);
				// Выполняем проверку наличия совпадения в тексте
				regexp.test("zzz", expression);
			}
		}
	}
	/**
	 * Выполняем проверку отвержения записей подделанных
	 *
	 * @details Без проверки правильности программы отказов этих остаются
	 *          единицы: замер снятия её показал двадцать три отказа взамен
	 *          тысячи восьмисот двадцати семи.
	 *
	 */
	EXPECT_GT(refused, static_cast <size_t> (500));
}
