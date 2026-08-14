/**
 * @file bignum.cpp
 * @date 2026-07-30
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
 * @brief Тесты вывода результата хэширования в длинные числа — проверка заполнения всей разрядности
 *        длинного числа, пригодности вещественного результата в качестве ключа, хэширования самих
 *        длинных чисел и работы специализации хэширования стандартной библиотеки
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
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
 * @brief Тест вывода результата хэширования в длинные целые числа
 *
 */
TEST_F(HashFixture, IntegerBigNumHashTest){
	// Выполняем формирование 128-битного результата хэширования
	const awh::uint128_t result128 = this->_hash->hash <awh::uint128_t> ("проверка хэширования");
	// Выполняем формирование 256-битного результата хэширования
	const awh::uint256_t result256 = this->_hash->hash <awh::uint256_t> ("проверка хэширования");
	// Выполняем формирование 512-битного результата хэширования
	const awh::uint512_t result512 = this->_hash->hash <awh::uint512_t> ("проверка хэширования");
	// Проверяем заполнение результата хэширования
	EXPECT_FALSE(result128.zero());
	// Проверяем заполнение результата хэширования
	EXPECT_FALSE(result256.zero());
	// Проверяем заполнение результата хэширования
	EXPECT_FALSE(result512.zero());
	// Проверяем префиксное свойство результата хэширования
	EXPECT_EQ(::memcmp(result128.data(), result256.data(), awh::uint128_t::size()), 0);
	// Проверяем префиксное свойство результата хэширования
	EXPECT_EQ(::memcmp(result256.data(), result512.data(), awh::uint256_t::size()), 0);
	// Проверяем повторяемость результата хэширования
	EXPECT_TRUE(this->_hash->hash <awh::uint256_t> ("проверка хэширования") == result256);
	// Проверяем отличие результата хэширования других данных
	EXPECT_FALSE(this->_hash->hash <awh::uint256_t> ("проверка хэширования!") == result256);
	// Выполняем формирование знакового результата хэширования
	const awh::int128_t result = this->_hash->hash <awh::int128_t> ("проверка хэширования");
	// Проверяем совпадение битового образа знакового и беззнакового результата
	EXPECT_EQ(::memcmp(result.data(), result128.data(), awh::int128_t::size()), 0);
}

/**
 * @brief Тест заполнения всех разрядов результата хэширования
 *
 */
TEST_F(HashFixture, FilledBigNumHashTest){
	// Набор незаполненных разрядов результата хэширования
	std::vector <size_t> zero(awh::uint512_t::size(), 0);
	/**
	 * Выполняем перебор наборов данных для хэширования
	 */
	for(uint32_t i = 0; i < 256; i++){
		// Выполняем формирование результата хэширования
		const awh::uint512_t result = this->_hash->hash <awh::uint512_t> (&i, sizeof(i));
		/**
		 * Выполняем перебор всех байтов результата хэширования
		 */
		for(uint16_t j = 0; j < awh::uint512_t::size(); j++){
			/**
			 * Если очередной байт результата хэширования нулевой
			 */
			if(result[j] == 0)
				// Увеличиваем количество нулевых значений очередного байта
				zero[j]++;
		}
	}
	/**
	 * Выполняем перебор всех байтов результата хэширования
	 */
	for(uint16_t i = 0; i < awh::uint512_t::size(); i++)
		// Проверяем заполнение очередного байта результата хэширования, у случайных
		// данных нулевым оказывается примерно каждый двести пятьдесят шестой байт
		EXPECT_LT(zero[i], static_cast <size_t> (16));
}

/**
 * @brief Тест вывода результата хэширования в длинные вещественные числа
 *
 */
TEST_F(HashFixture, RealBigNumHashTest){
	/**
	 * Выполняем перебор наборов данных для хэширования
	 */
	for(uint32_t i = 0; i < 4096; i++){
		// Выполняем формирование вещественного результата хэширования
		const awh::real128_t result = this->_hash->hash <awh::real128_t> (&i, sizeof(i));
		// Проверяем пригодность результата хэширования в качестве ключа
		EXPECT_TRUE(result == result);
		// Проверяем конечность результата хэширования
		EXPECT_NE(static_cast <uint8_t> (result.category()), static_cast <uint8_t> (awh::bignum::class_t::UNDEFINED));
		// Проверяем конечность результата хэширования
		EXPECT_NE(static_cast <uint8_t> (result.category()), static_cast <uint8_t> (awh::bignum::class_t::UNLIMITED));
	}
	// Выполняем формирование вещественного результата хэширования половинной точности
	const awh::real16_t result16 = this->_hash->hash <awh::real16_t> ("проверка хэширования");
	// Проверяем пригодность результата хэширования в качестве ключа
	EXPECT_TRUE(result16 == result16);
	// Выполняем формирование вещественного результата хэширования восьмерной точности
	const awh::real256_t result256 = this->_hash->hash <awh::real256_t> ("проверка хэширования");
	// Проверяем пригодность результата хэширования в качестве ключа
	EXPECT_TRUE(result256 == result256);
}

/**
 * @brief Тест хэширования длинных чисел
 *
 */
TEST_F(HashFixture, HashOfBigNumTest){
	// Создаём длинное число
	awh::uint256_t num = 1;
	// Набор сформированных результатов хэширования
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор степеней двойки
	 */
	for(uint16_t i = 0; i < 200; i++){
		// Добавляем результат хэширования очередного длинного числа
		results.emplace(this->_hash->hash <uint64_t> (num));
		// Выполняем сдвиг длинного числа
		num <<= 1;
	}
	// Проверяем отсутствие коллизий хэширования длинных чисел
	EXPECT_EQ(results.size(), static_cast <size_t> (200));
	// Проверяем совпадение хэширования длинного числа с хэшированием его буфера
	EXPECT_EQ(this->_hash->hash <uint64_t> (num), this->_hash->hash <uint64_t> (num.data(), awh::uint256_t::size()));
	// Проверяем совпадение потокового хэширования длинного числа
	{
		// Создаём объект потокового хэширования
		awh::hash_t hash;
		// Выполняем добавление длинного числа в потоковое хэширование
		hash.update(num);
		// Проверяем совпадение результата потокового хэширования
		EXPECT_EQ(hash.digest <uint64_t> (), this->_hash->hash <uint64_t> (num));
	}
}

/**
 * @brief Тест специализации хэширования длинных чисел стандартной библиотеки
 *
 */
TEST_F(HashFixture, StandardBigNumHashTest){
	// Набор длинных чисел
	std::unordered_set <awh::uint256_t> numbers;
	/**
	 * Выполняем перебор набора длинных чисел
	 */
	for(uint32_t i = 0; i < 10000; i++)
		// Добавляем очередное длинное число в набор
		numbers.emplace(awh::uint256_t(i) * awh::uint256_t(0xFFFFFFFFULL));

	// Проверяем размер набора длинных чисел
	EXPECT_EQ(numbers.size(), static_cast <size_t> (10000));
	// Проверяем поиск длинного числа в наборе
	EXPECT_TRUE(numbers.find(awh::uint256_t(5000) * awh::uint256_t(0xFFFFFFFFULL)) != numbers.end());
	// Проверяем отсутствие постороннего длинного числа в наборе
	EXPECT_TRUE(numbers.find(awh::uint256_t(3)) == numbers.end());
	// Проверяем совпадение результата специализации с хэшированием буфера числа
	{
		// Создаём длинное число
		const awh::int512_t num = -12345678;
		// Проверяем совпадение результата специализации хэширования
		EXPECT_EQ(std::hash <awh::int512_t> {}(num), this->_hash->hash <size_t> (num.data(), awh::int512_t::size()));
	}
}
