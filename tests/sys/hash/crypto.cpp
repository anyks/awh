/**
 * @file: crypto.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты хэширования в числа модуля криптографии — проверка совпадения результата с модулем
 *        хэширования, работы хэширования с ключами и вывода результата в длинные числа произвольной
 *        разрядности, недоступного прежней реализации
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdint>
#include <unordered_set>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "hash.hpp"
#include "../../../include/sys/crypto.hpp"

/**
 * @brief Класс фикстуры для тестов хэширования модуля криптографии
 *
 * @details Фикстура создаёт объект криптографии вместе с обязательным для него
 *          окружением фреймворка и объект хэширования, результаты которых тесты
 *          между собой сличают.
 *
 */
class HashCryptoFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект работы с логами
		std::unique_ptr <awh::log_t> _log;
		// Объект криптографии
		std::unique_ptr <awh::crypto_t> _crypto;
		// Объект хэширования данных
		std::unique_ptr <awh::hash_t> _hash;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp() override {
			// Создаём объект фреймворка
			this->_fmk = std::make_unique <awh::fmk_t> ();
			// Создаём объект работы с логами
			this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
			// Создаём объект криптографии
			this->_crypto = std::make_unique <awh::crypto_t> (this->_fmk.get(), this->_log.get());
			// Создаём объект хэширования данных
			this->_hash = std::make_unique <awh::hash_t> ();
		}
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown() override {}
};

/**
 * @brief Тест совпадения хэширования модуля криптографии с модулем хэширования
 *
 */
TEST_F(HashCryptoFixture, EqualCryptoHashTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Формируем буфер данных из текста
	const std::vector <char> buffer(text.begin(), text.end());
	// Формируем бинарный буфер данных из текста
	const std::vector <uint8_t> binary(text.begin(), text.end());
	// Проверяем совпадение хэширования текста
	EXPECT_EQ(this->_crypto->hash <uint64_t> (text), this->_hash->hash <uint64_t> (text));
	// Проверяем совпадение хэширования строкового литерала
	EXPECT_EQ(this->_crypto->hash <uint32_t> ("Anyks Framework, Hello World!!!"), this->_hash->hash <uint32_t> (text));
	// Проверяем совпадение хэширования буфера данных
	EXPECT_EQ(this->_crypto->hash <uint64_t> (buffer), this->_hash->hash <uint64_t> (text));
	// Проверяем совпадение хэширования бинарного буфера данных
	EXPECT_EQ(this->_crypto->hash <uint64_t> (binary), this->_hash->hash <uint64_t> (text));
	// Проверяем совпадение хэширования сырых данных
	EXPECT_EQ(this->_crypto->hash <uint64_t> (text.data(), text.size()), this->_hash->hash <uint64_t> (text));
	// Проверяем хэширование по умолчанию
	EXPECT_EQ(this->_crypto->hash(text), this->_hash->hash <uint64_t> (text));
}

/**
 * @brief Тест хэширования модуля криптографии в длинные числа
 *
 */
TEST_F(HashCryptoFixture, BigNumCryptoHashTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Выполняем хэширование текста в 128-битный массив байтов
	const awh::crypto_t::uint128_t result1 = this->_crypto->hash <awh::crypto_t::uint128_t> (text);
	// Выполняем хэширование текста в 128-битное длинное число
	const awh::uint128_t result2 = this->_crypto->hash <awh::uint128_t> (text);
	// Выполняем хэширование текста в 256-битное длинное число
	const awh::uint256_t result3 = this->_crypto->hash <awh::uint256_t> (text);
	// Выполняем хэширование текста в 1024-битное длинное число
	const awh::uint1024_t result4 = this->_crypto->hash <awh::uint1024_t> (text);
	// Проверяем совпадение результата в массиве байтов и длинном числе
	EXPECT_EQ(::memcmp(result1.data(), result2.data(), result1.size()), 0);
	// Проверяем префиксное свойство результата хэширования
	EXPECT_EQ(::memcmp(result2.data(), result3.data(), awh::uint128_t::size()), 0);
	// Проверяем префиксное свойство результата хэширования
	EXPECT_EQ(::memcmp(result3.data(), result4.data(), awh::uint256_t::size()), 0);
	// Проверяем заполнение результата хэширования наибольшей разрядности
	EXPECT_FALSE(result4.zero());
	// Проверяем совпадение результата с модулем хэширования
	EXPECT_TRUE(this->_hash->hash <awh::uint1024_t> (text) == result4);
	// Выполняем хэширование текста в вещественное длинное число
	const awh::real128_t result5 = this->_crypto->hash <awh::real128_t> (text);
	// Проверяем пригодность вещественного результата в качестве ключа
	EXPECT_TRUE(result5 == result5);
}

/**
 * @brief Тест хэширования модуля криптографии с ключом
 *
 */
TEST_F(HashCryptoFixture, SeedCryptoHashTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Формируем буфер данных из текста
	const std::vector <uint8_t> buffer(text.begin(), text.end());
	// Ключ хэширования
	const uint64_t seed = 0x1234567890ABCDEFULL;
	// Выполняем хэширование текста с ключом
	const uint64_t result = this->_crypto->hashWithSeed <uint64_t> (text, seed);
	// Проверяем отличие результата хэширования с ключом от хэширования без ключа
	EXPECT_NE(result, this->_crypto->hash <uint64_t> (text));
	// Проверяем совпадение хэширования буфера данных с ключом
	EXPECT_EQ(this->_crypto->hashWithSeed <uint64_t> (buffer, seed), result);
	// Проверяем совпадение хэширования сырых данных с ключом
	EXPECT_EQ(this->_crypto->hashWithSeed <uint64_t> (text.data(), text.size(), seed), result);
	// Проверяем совпадение хэширования с ключом с модулем хэширования
	{
		// Создаём объект хэширования с ключом
		const awh::hash_t hash(seed);
		// Проверяем совпадение результата хэширования
		EXPECT_EQ(hash.hash <uint64_t> (text), result);
	}
	// Проверяем хэширование с ключом в длинное число
	EXPECT_FALSE(this->_crypto->hashWithSeed <awh::uint256_t> (text, seed).zero());
	// Набор сформированных результатов хэширования
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор ключей хэширования
	 */
	for(uint64_t i = 0; i < 10000; i++)
		// Добавляем результат хэширования текста с очередным ключом
		results.emplace(this->_crypto->hashWithSeed <uint64_t> (text, i));

	// Проверяем отсутствие коллизий хэширования с разными ключами
	EXPECT_EQ(results.size(), static_cast <size_t> (10000));
}

/**
 * @brief Тест хэширования модуля криптографии с несколькими ключами
 *
 */
TEST_F(HashCryptoFixture, SeedsCryptoHashTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Формируем буфер данных из текста
	const std::vector <uint8_t> buffer(text.begin(), text.end());
	// Первый ключ хэширования
	const uint64_t seed1 = 0x1234567890ABCDEFULL;
	// Второй ключ хэширования
	const uint64_t seed2 = 0x0FEDCBA098765432ULL;
	// Выполняем хэширование текста с ключами
	const uint64_t result = this->_crypto->hashWithSeeds <uint64_t> (text, seed1, seed2);
	// Проверяем отличие результата хэширования с ключами от хэширования с одним ключом
	EXPECT_NE(result, this->_crypto->hashWithSeed <uint64_t> (text, seed1));
	// Проверяем совпадение хэширования буфера данных с ключами
	EXPECT_EQ(this->_crypto->hashWithSeeds <uint64_t> (buffer, seed1, seed2), result);
	// Проверяем совпадение хэширования сырых данных с ключами
	EXPECT_EQ(this->_crypto->hashWithSeeds <uint64_t> (text.data(), text.size(), seed1, seed2), result);
	// Проверяем совпадение хэширования с ключами со сведением ключей
	{
		// Создаём объект хэширования со сведённым начальным значением
		const awh::hash_t hash(awh::hashing::merge(seed1, seed2));
		// Проверяем совпадение результата хэширования
		EXPECT_EQ(hash.hash <uint64_t> (text), result);
	}
	// Проверяем хэширование с ключами в длинное число
	EXPECT_FALSE(this->_crypto->hashWithSeeds <awh::uint512_t> (text, seed1, seed2).zero());
}

/**
 * @brief Тест преобразования 128-битного хэша в 64-битный
 *
 */
TEST_F(HashCryptoFixture, Hash128to64CryptoHashTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Выполняем хэширование текста в 128-битный хэш
	const awh::crypto_t::uint128_t hash = this->_crypto->hash <awh::crypto_t::uint128_t> (text);
	// Выполняем преобразование 128-битного хэша в 64-битный
	const uint64_t result = this->_crypto->hash128to64(hash);
	// Проверяем повторяемость результата преобразования
	EXPECT_EQ(this->_crypto->hash128to64(hash), result);
	// Проверяем отличие результата преобразования от нуля
	EXPECT_NE(result, static_cast <uint64_t> (0));
	// Набор сформированных результатов преобразования
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор наборов данных для хэширования
	 */
	for(uint32_t i = 0; i < 20000; i++)
		// Добавляем результат преобразования 128-битного хэша очередного набора данных
		results.emplace(this->_crypto->hash128to64(this->_crypto->hash <awh::crypto_t::uint128_t> (&i, sizeof(i))));

	// Проверяем отсутствие коллизий преобразования
	EXPECT_EQ(results.size(), static_cast <size_t> (20000));
}

/**
 * @brief Тест значимости каждого из пары ключей хэширования
 *
 * @details Пара ключей сводится в одно начальное значение, и свести её
 *          перемешиванием нельзя: перемешивание мультипликативно, и нулевой
 *          ключ обнулял его целиком - пары (999, 0) и (7, 0) давали одно и то
 *          же значение, совпадающее к тому же с хэшированием по нулевому ключу.
 *          Тест закрепляет значимость обоих ключей при любом их значении
 *
 */
TEST_F(HashCryptoFixture, ZeroSeedsCryptoHashTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Проверяем значимость первого ключа при нулевом втором
	EXPECT_NE(this->_crypto->hashWithSeeds <uint64_t> (text, 7, 0), this->_crypto->hashWithSeeds <uint64_t> (text, 999, 0));
	// Проверяем значимость второго ключа при нулевом первом
	EXPECT_NE(this->_crypto->hashWithSeeds <uint64_t> (text, 0, 7), this->_crypto->hashWithSeeds <uint64_t> (text, 0, 999));
	// Проверяем отличие хэширования по паре нулевых ключей от хэширования по одному нулевому
	EXPECT_NE(this->_crypto->hashWithSeeds <uint64_t> (text, 0, 0), this->_crypto->hashWithSeed <uint64_t> (text, 0));
	// Проверяем отличие хэширования по паре равных ключей от хэширования по паре нулевых
	EXPECT_NE(this->_crypto->hashWithSeeds <uint64_t> (text, 42, 42), this->_crypto->hashWithSeeds <uint64_t> (text, 0, 0));
	// Проверяем значимость порядка ключей в паре
	EXPECT_NE(this->_crypto->hashWithSeeds <uint64_t> (text, 7, 999), this->_crypto->hashWithSeeds <uint64_t> (text, 999, 7));
	// Набор сформированных результатов хэширования
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор значений первого ключа хэширования
	 */
	for(uint64_t seed1 = 0; seed1 < 150; seed1++){
		/**
		 * Выполняем перебор значений второго ключа хэширования
		 */
		for(uint64_t seed2 = 0; seed2 < 150; seed2++)
			// Добавляем результат хэширования по очередной паре ключей
			results.emplace(this->_crypto->hashWithSeeds <uint64_t> (text, seed1, seed2));
	}
	// Проверяем отсутствие совпадений результата хэширования по разным парам ключей
	EXPECT_EQ(results.size(), static_cast <size_t> (150 * 150));
}
