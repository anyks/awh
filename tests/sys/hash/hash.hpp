/**
 * @file: hash.hpp
 * @date: 2026-07-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля хэширования — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HASH_TESTS__
#define __AWH_HASH_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/hash.hpp"

/**
 * @brief Класс фикстуры для тестов хэширования
 *
 * @details Фикстура создаёт объект хэширования и подготавливает буфер данных
 *          заведомо большего размера, чем блок вычислительного движка, чтобы
 *          тесты могли брать от него отрезки произвольной длины.
 *
 */
class HashFixture : public testing::Test {
	protected:
		// Объект хэширования данных
		std::unique_ptr <awh::hash_t> _hash;
		// Буфер данных для хэширования
		std::vector <uint8_t> _buffer;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
};

#endif // __AWH_HASH_TESTS__
