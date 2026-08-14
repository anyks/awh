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
 * @brief Тесты вычислительного движка хэширования — проверка функций смешивания и перемешивания чисел,
 *        формирования результата произвольной длины, приведения вещественного результата к конечному
 *        значению и записи результата в типы данных разной природы
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <unordered_set>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "hash.hpp"

/**
 * @brief Тест функции смешивания двух чисел
 *
 */
TEST_F(HashFixture, MixEngineHashTest){
	// Проверяем повторяемость результата смешивания
	EXPECT_EQ(awh::hashing::mix(12345, 67890), awh::hashing::mix(12345, 67890));
	// Проверяем коммутативность результата смешивания, произведение от перестановки не меняется
	EXPECT_EQ(awh::hashing::mix(12345, 67890), awh::hashing::mix(67890, 12345));
	// Проверяем обнуление результата смешивания с нулём
	EXPECT_EQ(awh::hashing::mix(0, 0xFFFFFFFFFFFFFFFFULL), static_cast <uint64_t> (0));
	// Проверяем результат смешивания с единицей
	EXPECT_EQ(awh::hashing::mix(1, 0x0123456789ABCDEFULL), static_cast <uint64_t> (0x0123456789ABCDEFULL));
	// Проверяем результат смешивания предельных значений, произведение двух
	// максимальных чисел равно 0xFFFFFFFFFFFFFFFE0000000000000001
	EXPECT_EQ(awh::hashing::mix(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL), static_cast <uint64_t> (0xFFFFFFFFFFFFFFFFULL));
	// Набор сформированных результатов смешивания
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор наборов смешиваемых чисел
	 */
	for(uint64_t i = 1; i < 20000; i++)
		// Добавляем результат смешивания очередной пары чисел
		results.emplace(awh::hashing::mix((i * 0x9E3779B97F4A7C15ULL), (i ^ 0xC2B2AE3D27D4EB4FULL)));

	// Проверяем отсутствие коллизий смешивания
	EXPECT_EQ(results.size(), static_cast <size_t> (19999));
}

/**
 * @brief Тест функции сведения пары начальных значений хэширования
 *
 * @details Свести пару ключей смешиванием нельзя: смешивание мультипликативно,
 *          и нулевой ключ обнуляет его целиком. Сведение обязано оставаться
 *          значимым по обоим ключам при любом их значении
 *
 */
TEST_F(HashFixture, MergeEngineHashTest){
	// Проверяем повторяемость результата сведения
	EXPECT_EQ(awh::hashing::merge(12345, 67890), awh::hashing::merge(12345, 67890));
	// Проверяем значимость порядка ключей в паре
	EXPECT_NE(awh::hashing::merge(12345, 67890), awh::hashing::merge(67890, 12345));
	// Проверяем отличие результата сведения пары нулей от нуля
	EXPECT_NE(awh::hashing::merge(0, 0), static_cast <uint64_t> (0));
	// Проверяем отличие результата сведения пары равных ключей от нуля
	EXPECT_NE(awh::hashing::merge(0x0123456789ABCDEFULL, 0x0123456789ABCDEFULL), static_cast <uint64_t> (0));
	// Проверяем значимость первого ключа при нулевом втором
	EXPECT_NE(awh::hashing::merge(7, 0), awh::hashing::merge(999, 0));
	// Проверяем значимость второго ключа при нулевом первом
	EXPECT_NE(awh::hashing::merge(0, 7), awh::hashing::merge(0, 999));
	// Набор сформированных результатов сведения
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор значений первого ключа хэширования
	 */
	for(uint64_t seed1 = 0; seed1 < 150; seed1++){
		/**
		 * Выполняем перебор значений второго ключа хэширования
		 */
		for(uint64_t seed2 = 0; seed2 < 150; seed2++)
			// Добавляем результат сведения очередной пары ключей
			results.emplace(awh::hashing::merge(seed1, seed2));
	}
	// Проверяем отсутствие коллизий сведения
	EXPECT_EQ(results.size(), static_cast <size_t> (150 * 150));
}

/**
 * @brief Тест функции окончательного перемешивания числа
 *
 */
TEST_F(HashFixture, AvalancheEngineHashTest){
	// Количество изменившихся разрядов результата перемешивания
	size_t total = 0;
	// Количество выполненных проверок
	size_t count = 0;
	// Набор сформированных результатов перемешивания
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор перемешиваемых чисел
	 */
	for(uint64_t i = 0; i < 5000; i++){
		// Выполняем перемешивание очередного числа
		const uint64_t result = awh::hashing::avalanche(i);
		// Добавляем результат перемешивания очередного числа
		results.emplace(result);
		// Проверяем повторяемость результата перемешивания
		EXPECT_EQ(awh::hashing::avalanche(i), result);
		/**
		 * Выполняем перебор всех разрядов перемешиваемого числа
		 */
		for(uint8_t bit = 0; bit < 64; bit++){
			// Выполняем перемешивание числа с инвертированным разрядом
			const uint64_t value = awh::hashing::avalanche(i ^ (static_cast <uint64_t> (1) << bit));
			// Увеличиваем количество изменившихся разрядов результата перемешивания
			total += static_cast <size_t> (__builtin_popcountll(value ^ result));
			// Увеличиваем количество выполненных проверок
			count++;
		}
	}
	// Проверяем отсутствие коллизий перемешивания
	EXPECT_EQ(results.size(), static_cast <size_t> (5000));
	// Определяем среднее количество изменившихся разрядов результата перемешивания
	const double average = (static_cast <double> (total) / static_cast <double> (count));
	// Проверяем лавинный эффект перемешивания
	EXPECT_GT(average, 31.0);
	// Проверяем отсутствие смещения лавинного эффекта перемешивания
	EXPECT_LT(average, 33.0);
}

/**
 * @brief Тест функции формирования хэша буфера данных
 *
 */
TEST_F(HashFixture, GenerateEngineHashTest){
	// Буфер результата хэширования
	uint8_t result[64];
	/**
	 * Выполняем перебор размеров результата хэширования
	 */
	for(size_t length = 1; length <= sizeof(result); length++){
		// Заполняем буфер результата хэширования
		::memset(result, 0xFF, sizeof(result));
		// Выполняем формирование результата хэширования
		awh::hashing::generate(this->_buffer.data(), 128, 0, result, length);
		// Проверяем совпадение результата хэширования с 64-битным хэшем
		EXPECT_EQ(awh::hashing::generate(this->_buffer.data(), 128, 0), this->_hash->hash <uint64_t> (this->_buffer.data(), 128));
		/**
		 * Выполняем перебор байтов результата хэширования за пределами запрошенной длины
		 */
		for(size_t i = length; i < sizeof(result); i++)
			// Проверяем сохранность байтов буфера за пределами запрошенной длины
			EXPECT_EQ(result[i], static_cast <uint8_t> (0xFF));
	}
	// Выполняем формирование результата хэширования в отсутствующий буфер
	awh::hashing::generate(this->_buffer.data(), 128, 0, nullptr, sizeof(result));
	// Выполняем формирование результата хэширования нулевой длины
	awh::hashing::generate(this->_buffer.data(), 128, 0, result, 0);
	// Проверяем формирование результата хэширования отсутствующих данных
	EXPECT_EQ(awh::hashing::generate(nullptr, 128, 0), awh::hashing::generate(this->_buffer.data(), 0, 0));
	// Проверяем зависимость результата хэширования от начального значения
	EXPECT_NE(awh::hashing::generate(this->_buffer.data(), 128, 0), awh::hashing::generate(this->_buffer.data(), 128, 1));
}

/**
 * @brief Тест функции формирования хэша в произвольном типе данных
 *
 */
TEST_F(HashFixture, CreateEngineHashTest){
	// Выполняем формирование результата хэширования в массиве байтов
	const std::array <uint8_t, 16> result1 = awh::hashing::create <std::array <uint8_t, 16>> (this->_buffer.data(), 64);
	// Выполняем формирование результата хэширования в длинном числе
	const awh::uint128_t result2 = awh::hashing::create <awh::uint128_t> (this->_buffer.data(), 64);
	// Проверяем совпадение результата хэширования в массиве байтов и длинном числе
	EXPECT_EQ(::memcmp(result1.data(), result2.data(), result1.size()), 0);
	// Проверяем совпадение результата хэширования с результатом объекта хэширования
	EXPECT_TRUE(this->_hash->hash <awh::uint128_t> (this->_buffer.data(), 64) == result2);
	// Проверяем формирование результата хэширования с начальным значением
	EXPECT_NE(awh::hashing::create <uint64_t> (this->_buffer.data(), 64, 12345), awh::hashing::create <uint64_t> (this->_buffer.data(), 64));
	// Проверяем формирование результата хэширования по умолчанию
	EXPECT_EQ(awh::hashing::create(this->_buffer.data(), 64), awh::hashing::create <uint64_t> (this->_buffer.data(), 64));
}

/**
 * @brief Тест записи результата хэширования в типы данных разной природы
 *
 */
TEST_F(HashFixture, AssignEngineHashTest){
	// Буфер сформированного хэша
	uint8_t buffer[16] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
	};
	// Результат хэширования во встроенном числовом типе
	uint32_t result1 = 0;
	// Результат хэширования в массиве байтов
	std::array <uint8_t, 16> result2;
	// Результат хэширования в длинном числе
	awh::uint128_t result3;
	// Выполняем запись сформированного хэша во встроенный числовой тип
	awh::hashing::assign(result1, buffer);
	// Выполняем запись сформированного хэша в массив байтов
	awh::hashing::assign(result2, buffer);
	// Выполняем запись сформированного хэша в длинное число
	awh::hashing::assign(result3, buffer);
	/**
	 * Проверяем запись сформированного хэша во встроенный числовой тип
	 *
	 * @details Ожидаемое значение записано числом, а не сличением представлений
	 *          в памяти: сличение полагалось бы на порядок байтов процессора, а
	 *          поток октетов записывается от младшего октета к старшему всегда
	 */
	EXPECT_EQ(result1, static_cast <uint32_t> (0x04030201));
	// Проверяем запись сформированного хэша в массив байтов
	EXPECT_EQ(::memcmp(result2.data(), buffer, result2.size()), 0);
	// Проверяем запись сформированного хэша в длинное число
	EXPECT_EQ(::memcmp(result3.data(), buffer, awh::uint128_t::size()), 0);
}

/**
 * @brief Тест приведения вещественного результата хэширования к конечному значению
 *
 */
TEST_F(HashFixture, FiniteEngineHashTest){
	// Создаём вещественное длинное число
	awh::real64_t result = awh::real64_t::unlimited();
	// Выполняем приведение бесконечности к конечному значению
	awh::hashing::finite(result.data(), awh::real64_t::size());
	// Проверяем приведение бесконечности к конечному значению
	EXPECT_NE(static_cast <uint8_t> (result.category()), static_cast <uint8_t> (awh::bignum::class_t::UNLIMITED));

	// Создаём значение не являющееся числом
	result = awh::real64_t::undefined();
	// Выполняем приведение значения не являющегося числом к конечному значению
	awh::hashing::finite(result.data(), awh::real64_t::size());
	// Проверяем приведение значения не являющегося числом к конечному значению
	EXPECT_NE(static_cast <uint8_t> (result.category()), static_cast <uint8_t> (awh::bignum::class_t::UNDEFINED));
	// Проверяем пригодность приведённого значения в качестве ключа
	EXPECT_TRUE(result == result);

	// Создаём конечное вещественное длинное число
	const awh::real64_t value = 3.1415926;
	// Копируем конечное вещественное длинное число
	result = value;
	// Выполняем приведение конечного значения
	awh::hashing::finite(result.data(), awh::real64_t::size());
	// Проверяем сохранность конечного значения
	EXPECT_TRUE(result == value);

	// Выполняем приведение отсутствующего буфера числа
	awh::hashing::finite(nullptr, awh::real64_t::size());
	// Выполняем приведение буфера числа недостаточного размера
	awh::hashing::finite(result.data(), 1);
	// Проверяем сохранность конечного значения
	EXPECT_TRUE(result == value);
}
