/**
 * @file: parameterized.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты модуля хэширования — прогон подготовленных наборов входных данных
 *        через вычислительный движок с проверкой результата по эталонным значениям, закрывающим
 *        все ветви обработки данных, и сверкой результата меньшей разрядности с ним же
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "hash.hpp"

/**
 * @brief Параметры теста хэширования данных
 *
 */
struct HashVectorTestParameter {
	// Текст для хэширования
	std::string text = "";
	// Начальное значение хэширования
	uint64_t seed = 0;
	// Эталонный результат хэширования в шестнадцатеричном виде
	std::string result = "";
};

/**
 * @brief Параметризованный тестовый класс хэширования данных
 *
 */
class HashVectorParameterizedFixture : public HashFixture, public ::testing::WithParamInterface <HashVectorTestParameter> {
	public:
		// Параметры теста хэширования данных
		HashVectorTestParameter _parameter = GetParam();
	public:
		/**
		 * @brief Метод формирования шестнадцатеричного представления буфера данных
		 *
		 * @param buffer буфер данных для представления
		 * @param size   размер буфера данных для представления
		 * @return       шестнадцатеричное представление буфера данных
		 *
		 */
		static std::string encode(const uint8_t * buffer, const size_t size) noexcept {
			// Результат работы функции
			std::string result(size * 2, '0');
			/**
			 * Выполняем перебор всех байтов буфера данных
			 */
			for(size_t i = 0; i < size; i++){
				// Формируем старший символ очередного байта
				result[i * 2] = "0123456789abcdef"[buffer[i] >> 4];
				// Формируем младший символ очередного байта
				result[(i * 2) + 1] = "0123456789abcdef"[buffer[i] & 0x0F];
			}
			// Выводим результат работы функции
			return result;
		}
};

/**
 * @brief Тест хэширования данных с параметрами
 *
 */
TEST_P(HashVectorParameterizedFixture, VectorHashTest){
	// Буфер результата хэширования
	uint8_t result[32];
	// Создаём объект хэширования с указанным начальным значением
	const awh::hash_t hash(this->_parameter.seed);
	// Выполняем формирование результата хэширования
	hash.hash(this->_parameter.text.data(), this->_parameter.text.size(), result, sizeof(result));
	// Проверяем совпадение результата хэширования с эталонным значением
	ASSERT_EQ(HashVectorParameterizedFixture::encode(result, sizeof(result)), this->_parameter.result);
	// Выполняем формирование результата хэширования текста
	const uint64_t value = hash.hash <uint64_t> (this->_parameter.text);
	// Проверяем совпадение результата хэширования текста с результатом хэширования буфера данных
	ASSERT_EQ(value, hash.hash <uint64_t> (this->_parameter.text.data(), this->_parameter.text.size()));
}

/**
 * @brief Тест потокового хэширования данных с параметрами
 *
 */
TEST_P(HashVectorParameterizedFixture, VectorStreamHashTest){
	// Буфер результата хэширования
	uint8_t result[32];
	/**
	 * Выполняем перебор размеров порций данных потокового хэширования
	 */
	for(size_t chunk = 1; chunk <= 40; chunk++){
		// Создаём объект потокового хэширования с указанным начальным значением
		awh::hash_t hash(this->_parameter.seed);
		/**
		 * Выполняем передачу данных в потоковое хэширование порциями
		 */
		for(size_t offset = 0; offset < this->_parameter.text.size(); offset += chunk)
			// Выполняем добавление очередной порции данных
			hash.update(this->_parameter.text.data() + offset, ((this->_parameter.text.size() - offset) < chunk ? (this->_parameter.text.size() - offset) : chunk));

		// Выполняем формирование результата потокового хэширования
		hash.digest(result, sizeof(result));
		// Проверяем совпадение результата потокового хэширования с эталонным значением
		ASSERT_EQ(HashVectorParameterizedFixture::encode(result, sizeof(result)), this->_parameter.result);
	}
}

/**
 * @brief Тест префиксного свойства результата хэширования с параметрами
 *
 */
TEST_P(HashVectorParameterizedFixture, VectorPrefixHashTest){
	// Создаём объект хэширования с указанным начальным значением
	const awh::hash_t hash(this->_parameter.seed);
	// Буфер младших разрядов результата хэширования
	uint8_t buffer[8];
	// Выполняем формирование 32-битного результата хэширования
	const uint32_t result32 = hash.hash <uint32_t> (this->_parameter.text);
	// Выполняем формирование 64-битного результата хэширования
	const uint64_t result64 = hash.hash <uint64_t> (this->_parameter.text);
	// Выполняем формирование 128-битного результата хэширования
	const awh::uint128_t result128 = hash.hash <awh::uint128_t> (this->_parameter.text);
	/**
	 * Выполняем перебор всех байтов 32-битного результата хэширования
	 */
	for(size_t i = 0; i < sizeof(result32); i++)
		// Извлекаем очередной байт 32-битного результата хэширования
		buffer[i] = static_cast <uint8_t> (result32 >> (i * 8));

	// Проверяем совпадение 32-битного результата хэширования с эталонным значением
	ASSERT_EQ(HashVectorParameterizedFixture::encode(buffer, sizeof(result32)), this->_parameter.result.substr(0, (sizeof(result32) * 2)));
	/**
	 * Выполняем перебор всех байтов 64-битного результата хэширования
	 */
	for(size_t i = 0; i < sizeof(result64); i++)
		// Извлекаем очередной байт 64-битного результата хэширования
		buffer[i] = static_cast <uint8_t> (result64 >> (i * 8));

	// Проверяем совпадение 64-битного результата хэширования с эталонным значением
	ASSERT_EQ(HashVectorParameterizedFixture::encode(buffer, sizeof(result64)), this->_parameter.result.substr(0, (sizeof(result64) * 2)));
	// Проверяем совпадение 128-битного результата хэширования с эталонным значением
	ASSERT_EQ(HashVectorParameterizedFixture::encode(result128.data(), awh::uint128_t::size()), this->_parameter.result.substr(0, (awh::uint128_t::size() * 2)));
}

/**
 * @brief Инициализация параметров теста хэширования данных
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, HashVectorParameterizedFixture,
	::testing::Values(
		HashVectorTestParameter({
			"",
			0ULL,
			"9d3f66d2672792176606f5800d69382e95e9da94e532057f951fe727474116bd"
		}),
		HashVectorTestParameter({
			"a",
			0ULL,
			"f60a472feef6cbb87c8911948085ee4f035e77321fac7f9e7b4fc566c93dd8dd"
		}),
		HashVectorTestParameter({
			"abc",
			0ULL,
			"4c63b1f6c30fe9af526436a2b1599e59a11c18b08514491a2fea1f82a4ae1434"
		}),
		HashVectorTestParameter({
			"Hello World!!!",
			0ULL,
			"baabb57795745d4702123d5d132e2bb6628fef41764be278d7ae786f9b8fab3f"
		}),
		HashVectorTestParameter({
			"Anyks Framework",
			0ULL,
			"b1fa8ff945dc8527ec8c5e81f4e0e0023d455de245736b86b43cf462ceaa175d"
		}),
		HashVectorTestParameter({
			"0123456789ABCDEF",
			0ULL,
			"f6bbcdac93cb0af837b702439fe989acd5a5b8fbe9ae8ce34d027f78f3e13b03"
		}),
		HashVectorTestParameter({
			"0123456789ABCDEF0",
			0ULL,
			"3b335683fc9e4879845998e04f488e64f4812415bba8ebe7708557efc722520f"
		}),
		HashVectorTestParameter({
			"The quick brown fox jumps over the lazy dog",
			0ULL,
			"8a447fa1138afd6bb2db468295e44eab75bf530f9a3a74782a68b86badf87e5a"
		}),
		HashVectorTestParameter({
			"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF",
			0ULL,
			"5c6bea5e8682c7354332f729ab4e90afd3bd25a3b519ca641da18d77cc72ac50"
		}),
		HashVectorTestParameter({
			"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF!",
			0ULL,
			"1aa0826ec2a5bcce3e9b1193db5c29081313d2731d98f04b8cf73be5f7138268"
		}),
		HashVectorTestParameter({
			"Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework!!!",
			0ULL,
			"fb803fd469c254799cdff7538b0423b59275490e29f91fd642aa0c1581606b61"
		}),
		HashVectorTestParameter({
			"",
			81985529216486895ULL,
			"b0bd934e742ff9c602850d54548c4fee9f164b786d0eacbbdb0f4a5ea04a7d09"
		}),
		HashVectorTestParameter({
			"Hello World!!!",
			81985529216486895ULL,
			"d9bf90460aaaeba59a615b540bd9e20d3eaeb3091c9acfbff610cd21e6ebb353"
		}),
		HashVectorTestParameter({
			"Anyks Framework",
			1ULL,
			"57622d516c3ad867a678bc8672b8560140b52db8855b002819bbbce45e3d1ed3"
		}),
		HashVectorTestParameter({
			"The quick brown fox jumps over the lazy dog",
			18446744073709551615ULL,
			"a9c9a2b5bd0706fe0520f4eb3ba0608dab76a7937fdf587576ea2a6678dc5ba9"
		})
	)
);
