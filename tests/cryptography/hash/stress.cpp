/**
 * @file stress.cpp
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
 * @brief Нагрузочные тесты модуля хэширования — массовая сверка потокового хэширования со случайным
 *        дроблением данных, проверка равномерности распределения результата по корзинам и по каждому
 *        его разряду, а также вывод результата во все объявленные разрядности длинных чисел
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <random>
#include <cstring>
#include <cstdint>
#include <unordered_set>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "hash.hpp"

/**
 * @brief Тест потокового хэширования со случайным дроблением данных
 *
 */
TEST_F(HashFixture, RandomStreamStressHashTest){
	// Создаём генератор случайных чисел с постоянным зерном для воспроизводимости теста
	std::mt19937 engine(0x5A17ULL);
	// Создаём распределение размеров данных для хэширования
	std::uniform_int_distribution <size_t> sizes(0, (this->_buffer.size() - 1));
	// Создаём распределение размеров порций данных потокового хэширования
	std::uniform_int_distribution <size_t> chunks(1, 200);
	/**
	 * Выполняем перебор наборов данных для хэширования
	 */
	for(uint32_t i = 0; i < 3000; i++){
		// Определяем размер данных для хэширования
		const size_t size = sizes(engine);
		// Создаём объект потокового хэширования
		awh::hash_t hash;
		// Смещение в буфере данных для хэширования
		size_t offset = 0;
		/**
		 * Выполняем передачу данных в потоковое хэширование случайными порциями
		 */
		while(offset < size){
			// Определяем размер очередной порции данных
			const size_t chunk = chunks(engine);
			// Выполняем добавление очередной порции данных
			hash.update(this->_buffer.data() + offset, ((size - offset) < chunk ? (size - offset) : chunk));
			// Выполняем смещение в буфере данных для хэширования
			offset += chunk;
		}
		// Проверяем размер обработанных потоковым хэшированием данных
		ASSERT_EQ(hash.length(), static_cast <uint64_t> (size));
		// Проверяем совпадение потокового и одноразового результата хэширования
		ASSERT_EQ(hash.digest <uint64_t> (), this->_hash->hash <uint64_t> (this->_buffer.data(), size));
	}
}

/**
 * @brief Тест повторного использования объекта потокового хэширования
 *
 */
TEST_F(HashFixture, ReuseStressHashTest){
	// Создаём объект потокового хэширования
	awh::hash_t hash;
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 0; size < 400; size++){
		// Выполняем сброс состояния потокового хэширования
		hash.clear();
		// Выполняем добавление данных в потоковое хэширование
		hash.update(this->_buffer.data(), size);
		// Проверяем совпадение результата хэширования после сброса состояния
		ASSERT_EQ(hash.digest <uint64_t> (), this->_hash->hash <uint64_t> (this->_buffer.data(), size));
	}
	/**
	 * Выполняем перебор начальных значений хэширования
	 */
	for(uint64_t seed = 0; seed < 200; seed++){
		// Устанавливаем очередное начальное значение хэширования
		hash.seed(seed);
		// Выполняем добавление данных в потоковое хэширование
		hash.update(this->_buffer.data(), 300);
		// Создаём объект одноразового хэширования с тем же начальным значением
		const awh::hash_t sample(seed);
		// Проверяем совпадение результата хэширования после установки начального значения
		ASSERT_EQ(hash.digest <uint64_t> (), sample.hash <uint64_t> (this->_buffer.data(), 300));
	}
}

/**
 * @brief Тест отсутствия коллизий хэширования на массовых наборах данных
 *
 */
TEST_F(HashFixture, CollisionStressHashTest){
	// Набор сформированных результатов хэширования
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор наборов данных отличающихся четырьмя байтами
	 */
	for(uint32_t i = 0; i < 100000; i++){
		// Копируем буфер данных для хэширования
		std::vector <uint8_t> buffer(this->_buffer.begin(), (this->_buffer.begin() + 200));
		// Записываем номер набора данных в начало буфера, что делает каждый набор неповторимым
		::memcpy(buffer.data(), &i, sizeof(i));
		// Добавляем результат хэширования изменённого буфера данных
		results.emplace(this->_hash->hash <uint64_t> (buffer.data(), buffer.size()));
	}
	// Проверяем отсутствие коллизий хэширования
	EXPECT_EQ(results.size(), static_cast <size_t> (100000));

	// Выполняем очистку набора сформированных результатов хэширования
	results.clear();
	/**
	 * Выполняем перебор текстовых ключей
	 */
	for(uint32_t i = 0; i < 100000; i++)
		// Добавляем результат хэширования очередного текстового ключа
		results.emplace(this->_hash->hash <uint64_t> ("Anyks Framework key #" + std::to_string(i)));

	// Проверяем отсутствие коллизий хэширования текстовых ключей
	EXPECT_EQ(results.size(), static_cast <size_t> (100000));
}

/**
 * @brief Тест равномерности распределения результата хэширования по корзинам
 *
 */
TEST_F(HashFixture, DistributionStressHashTest){
	// Количество корзин распределения
	const size_t count = 256;
	// Количество наборов данных для хэширования
	const size_t total = 262144;
	// Набор корзин распределения
	std::vector <size_t> buckets(count, 0);
	/**
	 * Выполняем перебор наборов данных для хэширования
	 */
	for(uint32_t i = 0; i < total; i++)
		// Увеличиваем счётчик корзины результата хэширования
		buckets[this->_hash->hash <uint64_t> (&i, sizeof(i)) % count]++;

	// Определяем ожидаемое количество попаданий в корзину
	const double expected = (static_cast <double> (total) / static_cast <double> (count));
	// Значение критерия согласия Пирсона
	double criterion = 0.0;
	/**
	 * Выполняем перебор всех корзин распределения
	 */
	for(size_t i = 0; i < count; i++){
		// Определяем отклонение количества попаданий от ожидаемого
		const double deviation = (static_cast <double> (buckets[i]) - expected);
		// Увеличиваем значение критерия согласия
		criterion += ((deviation * deviation) / expected);
	}
	// Проверяем равномерность распределения, для 255 степеней свободы значение
	// критерия согласия при уровне значимости 0.001 не превышает 331
	EXPECT_LT(criterion, 331.0);
}

/**
 * @brief Тест равномерности каждого разряда результата хэширования
 *
 */
TEST_F(HashFixture, BitsStressHashTest){
	// Количество наборов данных для хэширования
	const size_t total = 100000;
	// Набор счётчиков единичных значений разрядов результата хэширования
	std::vector <size_t> bits(64, 0);
	/**
	 * Выполняем перебор наборов данных для хэширования
	 */
	for(uint32_t i = 0; i < total; i++){
		// Выполняем хэширование очередного набора данных
		const uint64_t result = this->_hash->hash <uint64_t> (&i, sizeof(i));
		/**
		 * Выполняем перебор всех разрядов результата хэширования
		 */
		for(uint8_t bit = 0; bit < 64; bit++){
			/**
			 * Если очередной разряд результата хэширования единичный
			 */
			if((result >> bit) & 1)
				// Увеличиваем счётчик единичных значений очередного разряда
				bits[bit]++;
		}
	}
	/**
	 * Выполняем перебор всех разрядов результата хэширования
	 */
	for(uint8_t bit = 0; bit < 64; bit++){
		// Определяем долю единичных значений очередного разряда
		const double ratio = (static_cast <double> (bits[bit]) / static_cast <double> (total));
		// Проверяем равномерность очередного разряда результата хэширования
		EXPECT_GT(ratio, 0.49);
		// Проверяем равномерность очередного разряда результата хэширования
		EXPECT_LT(ratio, 0.51);
	}
}

/**
 * @brief Тест вывода результата хэширования во все объявленные разрядности длинных чисел
 *
 */
TEST_F(HashFixture, WidthStressHashTest){
	/**
	 * @brief Макрос проверки вывода результата хэширования в длинное число
	 *
	 * @param TYPE тип длинного числа для проверки
	 *
	 */
	#define AWH_HASH_CHECK_WIDTH(TYPE) \
		{ \
			/* Выполняем формирование результата хэширования */ \
			const TYPE result = this->_hash->hash <TYPE> (this->_buffer.data(), 333); \
			/* Проверяем повторяемость результата хэширования */ \
			ASSERT_TRUE(this->_hash->hash <TYPE> (this->_buffer.data(), 333) == result); \
			/* Проверяем пригодность результата хэширования в качестве ключа */ \
			ASSERT_TRUE(result == result); \
			/* Проверяем префиксное свойство результата хэширования */ \
			ASSERT_EQ(::memcmp(result.data(), reference, (TYPE::size() < sizeof(reference) ? TYPE::size() : sizeof(reference))), 0); \
			/* Проверяем работу специализации хэширования стандартной библиотеки */ \
			ASSERT_EQ(std::hash <TYPE> {}(result), this->_hash->hash <size_t> (result.data(), TYPE::size())); \
		}

	// Буфер эталонного результата хэширования
	uint8_t reference[16];
	// Выполняем формирование эталонного результата хэширования
	this->_hash->hash(this->_buffer.data(), 333, reference, sizeof(reference));

	// Проверяем вывод результата хэширования в знаковые длинные целые числа
	AWH_HASH_CHECK_WIDTH(awh::int24_t)
	AWH_HASH_CHECK_WIDTH(awh::int40_t)
	AWH_HASH_CHECK_WIDTH(awh::int48_t)
	AWH_HASH_CHECK_WIDTH(awh::int56_t)
	AWH_HASH_CHECK_WIDTH(awh::int72_t)
	AWH_HASH_CHECK_WIDTH(awh::int80_t)
	AWH_HASH_CHECK_WIDTH(awh::int96_t)
	AWH_HASH_CHECK_WIDTH(awh::int128_t)
	AWH_HASH_CHECK_WIDTH(awh::int160_t)
	AWH_HASH_CHECK_WIDTH(awh::int192_t)
	AWH_HASH_CHECK_WIDTH(awh::int224_t)
	AWH_HASH_CHECK_WIDTH(awh::int256_t)
	AWH_HASH_CHECK_WIDTH(awh::int320_t)
	AWH_HASH_CHECK_WIDTH(awh::int384_t)
	AWH_HASH_CHECK_WIDTH(awh::int512_t)
	AWH_HASH_CHECK_WIDTH(awh::int768_t)
	AWH_HASH_CHECK_WIDTH(awh::int1024_t)
	AWH_HASH_CHECK_WIDTH(awh::int1536_t)
	AWH_HASH_CHECK_WIDTH(awh::int2048_t)
	AWH_HASH_CHECK_WIDTH(awh::int3072_t)
	AWH_HASH_CHECK_WIDTH(awh::int4096_t)
	AWH_HASH_CHECK_WIDTH(awh::int6144_t)
	AWH_HASH_CHECK_WIDTH(awh::int8192_t)

	// Проверяем вывод результата хэширования в беззнаковые длинные целые числа
	AWH_HASH_CHECK_WIDTH(awh::uint24_t)
	AWH_HASH_CHECK_WIDTH(awh::uint40_t)
	AWH_HASH_CHECK_WIDTH(awh::uint48_t)
	AWH_HASH_CHECK_WIDTH(awh::uint56_t)
	AWH_HASH_CHECK_WIDTH(awh::uint72_t)
	AWH_HASH_CHECK_WIDTH(awh::uint80_t)
	AWH_HASH_CHECK_WIDTH(awh::uint96_t)
	AWH_HASH_CHECK_WIDTH(awh::uint128_t)
	AWH_HASH_CHECK_WIDTH(awh::uint160_t)
	AWH_HASH_CHECK_WIDTH(awh::uint192_t)
	AWH_HASH_CHECK_WIDTH(awh::uint224_t)
	AWH_HASH_CHECK_WIDTH(awh::uint256_t)
	AWH_HASH_CHECK_WIDTH(awh::uint320_t)
	AWH_HASH_CHECK_WIDTH(awh::uint384_t)
	AWH_HASH_CHECK_WIDTH(awh::uint512_t)
	AWH_HASH_CHECK_WIDTH(awh::uint768_t)
	AWH_HASH_CHECK_WIDTH(awh::uint1024_t)
	AWH_HASH_CHECK_WIDTH(awh::uint1536_t)
	AWH_HASH_CHECK_WIDTH(awh::uint2048_t)
	AWH_HASH_CHECK_WIDTH(awh::uint3072_t)
	AWH_HASH_CHECK_WIDTH(awh::uint4096_t)
	AWH_HASH_CHECK_WIDTH(awh::uint6144_t)
	AWH_HASH_CHECK_WIDTH(awh::uint8192_t)

	/**
	 * @brief Макрос проверки вывода результата хэширования в вещественное длинное число
	 *
	 * @param TYPE тип вещественного длинного числа для проверки
	 *
	 */
	#define AWH_HASH_CHECK_REAL(TYPE) \
		{ \
			/* Выполняем перебор наборов данных для хэширования */ \
			for(uint32_t i = 0; i < 512; i++){ \
				/* Выполняем формирование результата хэширования */ \
				const TYPE result = this->_hash->hash <TYPE> (&i, sizeof(i)); \
				/* Проверяем пригодность результата хэширования в качестве ключа */ \
				ASSERT_TRUE(result == result); \
				/* Проверяем конечность результата хэширования */ \
				ASSERT_NE(static_cast <uint8_t> (result.category()), static_cast <uint8_t> (awh::bignum::class_t::UNLIMITED)); \
				/* Проверяем конечность результата хэширования */ \
				ASSERT_NE(static_cast <uint8_t> (result.category()), static_cast <uint8_t> (awh::bignum::class_t::UNDEFINED)); \
			} \
		}

	// Проверяем вывод результата хэширования в вещественные длинные числа
	AWH_HASH_CHECK_REAL(awh::real16_t)
	AWH_HASH_CHECK_REAL(awh::real24_t)
	AWH_HASH_CHECK_REAL(awh::real32_t)
	AWH_HASH_CHECK_REAL(awh::real48_t)
	AWH_HASH_CHECK_REAL(awh::real64_t)
	AWH_HASH_CHECK_REAL(awh::real80_t)
	AWH_HASH_CHECK_REAL(awh::real96_t)
	AWH_HASH_CHECK_REAL(awh::real128_t)
	AWH_HASH_CHECK_REAL(awh::real192_t)
	AWH_HASH_CHECK_REAL(awh::real256_t)
	AWH_HASH_CHECK_REAL(awh::real384_t)
	AWH_HASH_CHECK_REAL(awh::real512_t)
	AWH_HASH_CHECK_REAL(awh::real768_t)
	AWH_HASH_CHECK_REAL(awh::real1024_t)

	// Отменяем макрос проверки вывода результата хэширования в длинное число
	#undef AWH_HASH_CHECK_WIDTH
	// Отменяем макрос проверки вывода результата хэширования в вещественное длинное число
	#undef AWH_HASH_CHECK_REAL
}

/**
 * @brief Тест хэширования данных большого объёма
 *
 */
TEST_F(HashFixture, VolumeStressHashTest){
	// Создаём буфер данных большого объёма
	std::vector <uint8_t> buffer(1048576, 0);
	/**
	 * Выполняем заполнение буфера данных
	 */
	for(size_t i = 0; i < buffer.size(); i++)
		// Заполняем очередной байт буфера данных
		buffer[i] = static_cast <uint8_t> ((i * 97) ^ (i >> 5));

	// Выполняем одноразовое хэширование буфера данных
	const uint64_t result = this->_hash->hash <uint64_t> (buffer.data(), buffer.size());
	// Создаём объект потокового хэширования
	awh::hash_t hash;
	/**
	 * Выполняем передачу данных в потоковое хэширование порциями простого размера
	 */
	for(size_t offset = 0; offset < buffer.size(); offset += 4093)
		// Выполняем добавление очередной порции данных
		hash.update(buffer.data() + offset, ((buffer.size() - offset) < 4093 ? (buffer.size() - offset) : 4093));

	// Проверяем размер обработанных потоковым хэшированием данных
	EXPECT_EQ(hash.length(), static_cast <uint64_t> (buffer.size()));
	// Проверяем совпадение потокового и одноразового результата хэширования
	EXPECT_EQ(hash.digest <uint64_t> (), result);
	// Проверяем отличие результата хэширования усечённого буфера данных
	EXPECT_NE(this->_hash->hash <uint64_t> (buffer.data(), (buffer.size() - 1)), result);
}
