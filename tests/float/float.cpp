/**
 * @file: float.cpp
 * @date: 2026-07-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файл
 */
#include "float.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void FloatFixture::SetUp() {}
/**
 * @brief Метод очистки тестового окружения
 *
 */
void FloatFixture::TearDown() {}
/**
 * @brief Сравнивает биты двух double
 *
 * @param a первое значение
 * @param b второе значение
 * @return  true при побитовом равенстве
 */
bool FloatFixture::sameBits(const double a, const double b) noexcept {
	// Переменная для хранения побитового представления первого числа
	uint64_t ua = 0;
	// Переменная для хранения побитового представления второго числа
	uint64_t ub = 0;
	// Копируем побитовое представление первого числа в ua
	std::memcpy(&ua, &a, sizeof(ua));
	// Копируем побитовое представление второго числа в ub
	std::memcpy(&ub, &b, sizeof(ub));
	// Проверяем побитовое равенство
	return (ua == ub);
}
/**
 * @brief Сравнивает биты двух float
 *
 * @param a первое значение
 * @param b второе значение
 * @return  true при побитовом равенстве
 */
bool FloatFixture::sameBits(const float a, const float b) noexcept {
	// Переменная для хранения побитового представления первого числа
	uint32_t ua = 0;
	// Переменная для хранения побитового представления второго числа
	uint32_t ub = 0;
	// Копируем побитовое представление первого числа в ua
	std::memcpy(&ua, &a, sizeof(ua));
	// Копируем побитовое представление второго числа в ub
	std::memcpy(&ub, &b, sizeof(ub));
	// Проверяем побитовое равенство
	return (ua == ub);
}
/**
 * @brief Разбирает double через floating_t и сверяет со strtod
 *
 * @param text исходная строка
 */
void FloatFixture::expectDoubleMatchesStrtod(const char * text) noexcept {
	// Проверяем, что текст не пустой
	ASSERT_NE(text, nullptr);
	// Переменная для хранения фактического значения double
	double actual = 0;
	// Указатель на конец разбора для strtod
	char * endRef = nullptr;
	// Разбираем строку с помощью strtod для получения ожидаемого значения
	const double expected = std::strtod(text, &endRef);
	// Разбираем строку с помощью floating_t::fromChars
	const auto answer = awh::floating_t::fromChars(text, text + std::strlen(text), actual);
	// Проверяем, что разбор прошёл успешно
	ASSERT_TRUE(static_cast <bool> (answer)) << "parse failed: " << text;
	// Проверяем, что указатель на конец разбора совпадает с ожидаемым
	ASSERT_EQ(answer.ptr, endRef) << "ptr mismatch: " << text;
	// Если ожидаемое значение — NaN, то проверяем, что и фактическое значение — NaN
	if(std::isnan(expected))
		// Проверяем, что фактическое значение также является NaN
		ASSERT_TRUE(std::isnan(actual)) << "nan mismatch: " << text;
	// Иначе проверяем, что биты фактического и ожидаемого значения совпадают
	else ASSERT_TRUE(sameBits(actual, expected)) << "value mismatch: " << text << " got=" << actual << " expected=" << expected;
}
/**
 * @brief Разбирает float через floating_t и сверяет со strtof
 *
 * @param text исходная строка
 */
void FloatFixture::expectFloatMatchesStrtof(const char * text) noexcept {
	// Проверяем, что текст не пустой
	ASSERT_NE(text, nullptr);
	// Переменная для хранения фактического значения float
	float actual = 0;
	// Указатель на конец разбора для strtof
	char * endRef = nullptr;
	// Разбираем строку с помощью strtof для получения ожидаемого значения
	const float expected = std::strtof(text, &endRef);
	// Разбираем строку с помощью floating_t::fromChars
	const auto answer = awh::floating_t::fromChars(text, text + std::strlen(text), actual);
	// Проверяем, что разбор прошёл успешно
	ASSERT_TRUE(static_cast <bool> (answer)) << "parse failed: " << text;
	// Проверяем, что указатель на конец разбора совпадает с ожидаемым
	ASSERT_EQ(answer.ptr, endRef) << "ptr mismatch: " << text;
	// Если ожидаемое значение — NaN, то проверяем, что и фактическое значение — NaN
	if(std::isnan(expected))
		// Проверяем, что фактическое значение также является NaN
		ASSERT_TRUE(std::isnan(actual)) << "nan mismatch: " << text;
	// Иначе проверяем, что биты фактического и ожидаемого значения совпадают
	else ASSERT_TRUE(sameBits(actual, expected)) << "value mismatch: " << text << " got=" << actual << " expected=" << expected;
}
