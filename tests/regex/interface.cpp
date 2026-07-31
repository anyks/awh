/**
 * @file: interface.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Автоматические тесты открытого интерфейса модуля регулярных выражений —
 *        сборка выражений с кэшем собранного, сопоставление выражения с текстом,
 *        извлечение захваченных групп по номеру и по имени и разделение выражения
 *        несколькими потоками исполнения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <thread>
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>

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
 * @brief Проверка сборки и сопоставления регулярного выражения
 *
 */
TEST(Regex, Interface) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp;
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(\\w+)@(\\w+)\\.([a-z]{2,})");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Выполняем проверку отсутствия ошибки сборки регулярного выражения
	EXPECT_EQ(regexp.error(), regex::error_t::NONE);
	// Выполняем проверку количества захватывающих групп выражения
	EXPECT_EQ(regexp.captures(expression), static_cast <uint32_t> (3));
	// Выполняем проверку наличия совпадения в тексте
	EXPECT_TRUE(regexp.test("почта forman@anyks.com здесь", expression));
	// Выполняем проверку отсутствия совпадения в тексте
	EXPECT_FALSE(regexp.test("почты здесь нет", expression));
	// Получаем текст совпадения и захваченных групп
	const auto & result = regexp.exec("почта forman@anyks.com здесь", expression);
	// Выполняем проверку количества извлечённых значений
	ASSERT_EQ(result.size(), static_cast <size_t> (4));
	// Выполняем проверку текста совпадения целиком
	EXPECT_EQ(result.at(0), "forman@anyks.com");
	// Выполняем проверку текста первой захватывающей группы
	EXPECT_EQ(result.at(1), "forman");
	// Выполняем проверку текста второй захватывающей группы
	EXPECT_EQ(result.at(2), "anyks");
	// Выполняем проверку текста третьей захватывающей группы
	EXPECT_EQ(result.at(3), "com");
}

/**
 * @brief Проверка извлечения границ совпадения и захваченных групп
 *
 */
TEST(Regex, InterfaceBounds) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp;
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("a(b)?(c)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Получаем границы совпадения и захваченных групп
	const auto & bounds = regexp.match("xxac", expression);
	// Выполняем проверку количества извлечённых границ
	ASSERT_EQ(bounds.size(), static_cast <size_t> (3));
	// Выполняем проверку начальной границы совпадения
	EXPECT_EQ(bounds.at(0).first, static_cast <size_t> (2));
	// Выполняем проверку конечной границы совпадения
	EXPECT_EQ(bounds.at(0).second, static_cast <size_t> (4));
	/**
	 * Выполняем проверку невыполненного захвата первой группой
	 *
	 * @details Границы невыполненного захвата равны позиции отсутствующего символа,
	 *          что отличает невыполненный захват от захвата пустого текста.
	 *
	 */
	EXPECT_EQ(bounds.at(1).first, string_view::npos);
	// Выполняем проверку конечной границы невыполненного захвата
	EXPECT_EQ(bounds.at(1).second, string_view::npos);
	// Выполняем проверку начальной границы захвата второй группой
	EXPECT_EQ(bounds.at(2).first, static_cast <size_t> (3));
	// Выполняем проверку конечной границы захвата второй группой
	EXPECT_EQ(bounds.at(2).second, static_cast <size_t> (4));
	// Получаем текст совпадения и захваченных групп
	const auto & result = regexp.exec("xxac", expression);
	// Выполняем проверку количества извлечённых значений
	ASSERT_EQ(result.size(), static_cast <size_t> (3));
	// Выполняем проверку пустого текста невыполненного захвата
	EXPECT_TRUE(result.at(1).empty());
	// Выполняем проверку текста захвата второй группой
	EXPECT_EQ(result.at(2), "c");
}

/**
 * @brief Проверка извлечения номера именованной группы
 *
 */
TEST(Regex, InterfaceNames) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp;
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(?<year>\\d{4})-(?<month>\\d{2})-(?<day>\\d{2})");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Выполняем проверку номера именованной группы года
	EXPECT_EQ(regexp.group(expression, "year"), static_cast <uint32_t> (1));
	// Выполняем проверку номера именованной группы месяца
	EXPECT_EQ(regexp.group(expression, "month"), static_cast <uint32_t> (2));
	// Выполняем проверку номера именованной группы дня
	EXPECT_EQ(regexp.group(expression, "day"), static_cast <uint32_t> (3));
	// Выполняем проверку отсутствия неизвестной именованной группы
	EXPECT_EQ(regexp.group(expression, "hour"), static_cast <uint32_t> (0));
	// Получаем текст совпадения и захваченных групп
	const auto & result = regexp.exec("дата 2026-07-31 здесь", expression);
	// Выполняем проверку количества извлечённых значений
	ASSERT_EQ(result.size(), static_cast <size_t> (4));
	// Выполняем проверку текста именованной группы года
	EXPECT_EQ(result.at(regexp.group(expression, "year")), "2026");
	// Выполняем проверку текста именованной группы месяца
	EXPECT_EQ(result.at(regexp.group(expression, "month")), "07");
	// Выполняем проверку текста именованной группы дня
	EXPECT_EQ(result.at(regexp.group(expression, "day")), "31");
}

/**
 * @brief Проверка установки режимов сборки регулярного выражения
 *
 */
TEST(Regex, InterfaceFlags) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp;
	// Выполняем сборку регулярного выражения с учётом регистра символов
	const auto sensitive = regexp.build("ПРИВЕТ", static_cast <uint32_t> (regex::flag_t::UTF));
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (sensitive));
	// Выполняем проверку отсутствия совпадения при учёте регистра символов
	EXPECT_FALSE(regexp.test("привет", sensitive));
	// Выполняем сборку регулярного выражения без учёта регистра символов
	const auto insensitive = regexp.build("ПРИВЕТ", {regex::flag_t::UTF, regex::flag_t::CASELESS});
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (insensitive));
	// Выполняем проверку наличия совпадения без учёта регистра символов
	EXPECT_TRUE(regexp.test("привет", insensitive));
	// Выполняем сборку регулярного выражения с привязкой к границам строк
	const auto multiline = regexp.build("^b", {regex::flag_t::MULTILINE});
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (multiline));
	// Выполняем проверку наличия совпадения с привязкой к границам строк
	EXPECT_TRUE(regexp.test("a\nb", multiline));
}

/**
 * @brief Проверка отказа сборки ошибочного регулярного выражения
 *
 */
TEST(Regex, InterfaceFailure) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp;
	// Выполняем сборку ошибочного регулярного выражения
	const auto expression = regexp.build("a(b");
	// Выполняем проверку отказа сборки регулярного выражения
	EXPECT_FALSE(static_cast <bool> (expression));
	// Выполняем проверку установки кода ошибки сборки выражения
	EXPECT_NE(regexp.error(), regex::error_t::NONE);
	// Выполняем проверку установки текста ошибки сборки выражения
	EXPECT_FALSE(regexp.message().empty());
	// Выполняем проверку отсутствия совпадения несобранного выражения
	EXPECT_FALSE(regexp.test("ab", expression));
	// Выполняем проверку отсутствия захватывающих групп несобранного выражения
	EXPECT_EQ(regexp.captures(expression), static_cast <uint32_t> (0));
}

/**
 * @brief Проверка кэша собранных регулярных выражений
 *
 */
TEST(Regex, InterfaceCache) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp;
	// Выполняем сборку регулярного выражения
	const auto first = regexp.build("[a-z]+\\d+");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (first));
	// Выполняем повторную сборку того же регулярного выражения
	const auto second = regexp.build("[a-z]+\\d+");
	/**
	 * Выполняем проверку вывода собранного ранее выражения
	 *
	 * @details Кэш избавляет от повторной сборки того же выражения, пока собранное
	 *          ранее выражение удерживается вызывающей стороной.
	 *
	 */
	EXPECT_EQ(first.get(), second.get());
	// Выполняем сборку того же выражения с иным набором режимов
	const auto third = regexp.build("[a-z]+\\d+", {regex::flag_t::CASELESS});
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (third));
	/**
	 * Выполняем проверку сборки отдельного выражения для иного набора режимов
	 *
	 * @details Ключом кэша является пара из набора режимов и текста выражения,
	 *          поэтому режимы разделяют собранные выражения между собой.
	 *
	 */
	EXPECT_NE(first.get(), third.get());
	// Выполняем проверку сопоставления собранного ранее выражения
	EXPECT_TRUE(regexp.test("abc123", second));
	// Выполняем очистку кэша собранных выражений
	regexp.clear();
	// Выполняем проверку сопоставления выражения после очистки кэша
	EXPECT_TRUE(regexp.test("abc123", first));
}

/**
 * @brief Проверка сопоставления выражения несколькими потоками исполнения
 *
 */
TEST(Regex, InterfaceThreads) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp;
	// Выполняем установку согласования доступа к кэшу собранных выражений
	regexp.threadSafety(true);
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(\\w+)=(\\d+)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Количество потоков исполнения сопоставления
	constexpr size_t count = 8;
	// Количество сопоставлений в каждом потоке исполнения
	constexpr size_t rounds = 1000;
	// Набор потоков исполнения сопоставления
	vector <thread> threads;
	// Набор количества совпадений каждого потока исполнения
	vector <size_t> matched(count, 0);
	// Выполняем размещение набора потоков исполнения
	threads.reserve(count);
	/**
	 * Выполняем запуск потоков исполнения сопоставления
	 *
	 * @details Собранное выражение после сборки не изменяется и разделяется
	 *          потоками исполнения, тогда как рабочее состояние сопоставления
	 *          хранится отдельно для каждого потока исполнения.
	 *
	 */
	for(size_t i = 0; i < count; i++) {
		// Выполняем запуск потока исполнения сопоставления
		threads.emplace_back([&regexp, &expression, &matched, i]() noexcept {
			/**
			 * Выполняем сопоставление выражения с текстом
			 */
			for(size_t j = 0; j < rounds; j++) {
				// Получаем текст совпадения и захваченных групп
				const auto & result = regexp.exec("параметр width=1024 задан", expression);
				/**
				 * Если текст совпадения и захваченных групп извлечён
				 */
				if((result.size() == 3) && (result.at(1).compare("width") == 0) && (result.at(2).compare("1024") == 0))
					// Увеличиваем количество совпадений потока исполнения
					matched.at(i)++;
			}
		});
	}
	/**
	 * Выполняем ожидание завершения потоков исполнения
	 */
	for(auto & item : threads)
		// Выполняем ожидание завершения потока исполнения
		item.join();
	/**
	 * Выполняем проверку количества совпадений каждого потока исполнения
	 */
	for(size_t i = 0; i < count; i++)
		// Выполняем проверку количества совпадений потока исполнения
		EXPECT_EQ(matched.at(i), rounds) << "Поток: " << i;
}
