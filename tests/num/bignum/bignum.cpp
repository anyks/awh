/**
 * @file: bignum.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля работы с длинными числами —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "bignum.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void BigNumFixture::SetUp(){
	// Создаём объект знакового длинного целого числа
	this->_integer = std::make_unique <awh::int128_t> ();
	// Создаём объект беззнакового длинного целого числа
	this->_natural = std::make_unique <awh::uint128_t> ();
	// Создаём объект вещественного длинного числа
	this->_real = std::make_unique <awh::real64_t> ();
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void BigNumFixture::TearDown() {}
