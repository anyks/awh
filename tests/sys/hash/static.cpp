/**
 * @file: static.cpp
 * @date: 2026-07-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты модуля хэширования — проверка одноразового и потокового хэширования, совпадения их
 *        результатов, префиксного свойства результата разной разрядности и вывода результата
 *        в длинные числа произвольной разрядности
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <set>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <cstring>
#include <cstdint>
#include <unordered_set>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "hash.hpp"

/**
 * @brief Тест создания объекта хэширования
 *
 */
TEST_F(HashFixture, CreateHashTest){
	// Проверяем создание объекта хэширования данных
	ASSERT_TRUE(this->_hash != nullptr);
	// Проверяем начальное значение хэширования
	EXPECT_EQ(this->_hash->seed(), static_cast <uint64_t> (0));
	// Проверяем размер обработанных данных
	EXPECT_EQ(this->_hash->length(), static_cast <uint64_t> (0));

	// Сбрасываем объект хэширования данных
	this->_hash.reset();

	// Проверяем сброс объекта хэширования данных
	ASSERT_TRUE(this->_hash == nullptr);
}

/**
 * @brief Тест повторяемости результата хэширования
 *
 */
TEST_F(HashFixture, RepeatableHashTest){
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 0; size < 512; size++){
		// Выполняем хэширование буфера данных
		const uint64_t result = this->_hash->hash <uint64_t> (this->_buffer.data(), size);
		// Проверяем повторяемость результата хэширования
		EXPECT_EQ(this->_hash->hash <uint64_t> (this->_buffer.data(), size), result);
	}
}

/**
 * @brief Тест хэширования пустых данных
 *
 */
TEST_F(HashFixture, EmptyHashTest){
	// Проверяем хэширование пустого буфера данных
	EXPECT_EQ(this->_hash->hash <uint64_t> (this->_buffer.data(), 0), this->_hash->hash <uint64_t> (""));
	// Проверяем хэширование отсутствующего буфера данных
	EXPECT_EQ(this->_hash->hash <uint64_t> (nullptr, 128), this->_hash->hash <uint64_t> (this->_buffer.data(), 0));
	// Проверяем отличие результата хэширования пустых данных от нуля
	EXPECT_NE(this->_hash->hash <uint64_t> (""), static_cast <uint64_t> (0));
}

/**
 * @brief Тест зависимости результата хэширования от начального значения
 *
 */
TEST_F(HashFixture, SeedHashTest){
	// Создаём объект хэширования с начальным значением
	awh::hash_t hash(0x5A17ULL);
	// Проверяем установленное начальное значение хэширования
	EXPECT_EQ(hash.seed(), static_cast <uint64_t> (0x5A17ULL));
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 0; size < 256; size++)
		// Проверяем отличие результата хэширования с начальным значением
		EXPECT_NE(hash.hash <uint64_t> (this->_buffer.data(), size), this->_hash->hash <uint64_t> (this->_buffer.data(), size));

	// Устанавливаем начальное значение хэширования по умолчанию
	hash.seed(0);
	// Проверяем совпадение результата хэширования с объектом по умолчанию
	EXPECT_EQ(hash.hash <uint64_t> (this->_buffer.data(), 128), this->_hash->hash <uint64_t> (this->_buffer.data(), 128));
}

/**
 * @brief Тест совпадения потокового и одноразового хэширования
 *
 */
TEST_F(HashFixture, StreamHashTest){
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 0; size < 300; size++){
		// Выполняем одноразовое хэширование буфера данных
		const uint64_t result = this->_hash->hash <uint64_t> (this->_buffer.data(), size);
		/**
		 * Выполняем перебор размеров порций данных потокового хэширования
		 */
		for(size_t chunk = 1; chunk < 70; chunk++){
			// Создаём объект потокового хэширования
			awh::hash_t hash;
			/**
			 * Выполняем передачу данных в потоковое хэширование порциями
			 */
			for(size_t offset = 0; offset < size; offset += chunk)
				// Выполняем добавление очередной порции данных
				hash.update(this->_buffer.data() + offset, ((size - offset) < chunk ? (size - offset) : chunk));

			// Проверяем размер обработанных потоковым хэшированием данных
			EXPECT_EQ(hash.length(), static_cast <uint64_t> (size));
			// Проверяем совпадение потокового и одноразового результата хэширования
			EXPECT_EQ(hash.digest <uint64_t> (), result);
		}
	}
}

/**
 * @brief Тест неизменности состояния потокового хэширования при получении результата
 *
 */
TEST_F(HashFixture, DigestHashTest){
	// Создаём объект потокового хэширования
	awh::hash_t hash;
	// Выполняем добавление первой половины данных
	hash.update(this->_buffer.data(), 100);
	// Получаем промежуточный результат хэширования
	const uint64_t result = hash.digest <uint64_t> ();
	// Проверяем повторяемость промежуточного результата хэширования
	EXPECT_EQ(hash.digest <uint64_t> (), result);
	// Проверяем совпадение промежуточного результата с одноразовым хэшированием
	EXPECT_EQ(result, this->_hash->hash <uint64_t> (this->_buffer.data(), 100));
	// Выполняем добавление второй половины данных
	hash.update(this->_buffer.data() + 100, 100);
	// Проверяем совпадение итогового результата с одноразовым хэшированием
	EXPECT_EQ(hash.digest <uint64_t> (), this->_hash->hash <uint64_t> (this->_buffer.data(), 200));

	// Выполняем сброс состояния потокового хэширования
	hash.clear();
	// Проверяем сброс размера обработанных данных
	EXPECT_EQ(hash.length(), static_cast <uint64_t> (0));
	// Проверяем совпадение результата хэширования после сброса с хэшированием пустых данных
	EXPECT_EQ(hash.digest <uint64_t> (), this->_hash->hash <uint64_t> (""));
}

/**
 * @brief Тест префиксного свойства результата хэширования
 *
 */
TEST_F(HashFixture, PrefixHashTest){
	// Буфер результата хэширования
	uint8_t result[32];
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 0; size < 200; size++){
		// Выполняем формирование результата хэширования наибольшей разрядности
		this->_hash->hash(this->_buffer.data(), size, result, sizeof(result));
		// Выполняем формирование 8-битного результата хэширования
		const uint8_t result8 = this->_hash->hash <uint8_t> (this->_buffer.data(), size);
		// Выполняем формирование 16-битного результата хэширования
		const uint16_t result16 = this->_hash->hash <uint16_t> (this->_buffer.data(), size);
		// Выполняем формирование 32-битного результата хэширования
		const uint32_t result32 = this->_hash->hash <uint32_t> (this->_buffer.data(), size);
		// Выполняем формирование 64-битного результата хэширования
		const uint64_t result64 = this->_hash->hash <uint64_t> (this->_buffer.data(), size);
		// Проверяем совпадение 8-битного результата с началом потока байтов
		EXPECT_EQ(result8, result[0]);
		// Проверяем совпадение 16-битного результата с началом потока байтов
		EXPECT_EQ(result16, static_cast <uint16_t> (result[0] | (result[1] << 8)));
		// Проверяем совпадение 32-битного результата с началом потока байтов
		EXPECT_EQ(result32, static_cast <uint32_t> (result64));
		// Проверяем совпадение 64-битного результата с началом потока байтов
		EXPECT_EQ(::memcmp(&result64, result, sizeof(result64)), 0);
	}
}

/**
 * @brief Тест хэширования текста и контейнеров данных
 *
 */
TEST_F(HashFixture, ContainerHashTest){
	// Текст для хэширования
	const std::string text = "Приветствуем вас в очень длинном тексте для проверки хэширования";
	// Формируем буфер данных из текста
	const std::vector <char> buffer(text.begin(), text.end());
	// Формируем бинарный буфер данных из текста
	const std::vector <uint8_t> binary(text.begin(), text.end());
	// Выполняем хэширование текста
	const uint64_t result = this->_hash->hash <uint64_t> (text.data(), text.size());
	// Проверяем хэширование строкового литерала
	EXPECT_EQ(this->_hash->hash <uint64_t> ("Приветствуем вас в очень длинном тексте для проверки хэширования"), result);
	// Проверяем хэширование строки
	EXPECT_EQ(this->_hash->hash <uint64_t> (text), result);
	// Проверяем хэширование буфера данных
	EXPECT_EQ(this->_hash->hash <uint64_t> (buffer), result);
	// Проверяем хэширование бинарного буфера данных
	EXPECT_EQ(this->_hash->hash <uint64_t> (binary), result);
	// Проверяем оператор хэширования текста
	EXPECT_EQ((* this->_hash)(text), result);
	// Проверяем хэширование представления текста
	EXPECT_EQ(this->_hash->hash <uint64_t> (std::string_view(text)), result);
	// Проверяем хэширование части представления текста
	EXPECT_EQ(this->_hash->hash <uint64_t> (std::string_view(text).substr(0, 10)), this->_hash->hash <uint64_t> (text.data(), 10));
}

/**
 * @brief Тест лавинного эффекта хэширования
 *
 */
TEST_F(HashFixture, AvalancheHashTest){
	// Количество совпавших разрядов результата хэширования
	size_t total = 0;
	// Количество выполненных проверок
	size_t count = 0;
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 1; size <= 128; size++){
		// Копируем буфер данных для хэширования
		std::vector <uint8_t> buffer(this->_buffer.begin(), this->_buffer.begin() + size);
		// Выполняем хэширование исходного буфера данных
		const uint64_t result = this->_hash->hash <uint64_t> (buffer.data(), size);
		/**
		 * Выполняем перебор всех байтов буфера данных
		 */
		for(size_t i = 0; i < size; i++){
			/**
			 * Выполняем перебор всех разрядов очередного байта
			 */
			for(uint8_t bit = 0; bit < 8; bit++){
				// Выполняем инверсию очередного разряда буфера данных
				buffer[i] ^= static_cast <uint8_t> (1 << bit);
				// Выполняем хэширование изменённого буфера данных
				const uint64_t value = this->_hash->hash <uint64_t> (buffer.data(), size);
				// Выполняем возврат изменённого разряда буфера данных
				buffer[i] ^= static_cast <uint8_t> (1 << bit);
				// Проверяем изменение результата хэширования
				ASSERT_NE(value, result);
				// Увеличиваем количество изменившихся разрядов результата хэширования
				total += static_cast <size_t> (__builtin_popcountll(value ^ result));
				// Увеличиваем количество выполненных проверок
				count++;
			}
		}
	}
	// Определяем среднее количество изменившихся разрядов результата хэширования
	const double average = (static_cast <double> (total) / static_cast <double> (count));
	// Проверяем лавинный эффект хэширования, при котором изменение одного разряда
	// входных данных изменяет примерно половину разрядов результата
	EXPECT_GT(average, 31.0);
	// Проверяем отсутствие смещения лавинного эффекта хэширования
	EXPECT_LT(average, 33.0);
}

/**
 * @brief Тест отсутствия коллизий хэширования
 *
 */
TEST_F(HashFixture, CollisionHashTest){
	// Набор сформированных результатов хэширования
	std::unordered_set <uint64_t> results;
	/**
	 * Выполняем перебор последовательных ключей
	 */
	for(uint32_t i = 0; i < 500000; i++)
		// Добавляем результат хэширования очередного ключа
		results.emplace(this->_hash->hash <uint64_t> (&i, sizeof(i)));

	// Проверяем отсутствие коллизий хэширования
	EXPECT_EQ(results.size(), static_cast <size_t> (500000));

	// Выполняем очистку набора сформированных результатов хэширования
	results.clear();
	/**
	 * Выполняем перебор ключей отличающихся одним разрядом
	 */
	for(uint8_t i = 0; i < 64; i++){
		// Формируем очередной ключ хэширования
		const uint64_t key = (static_cast <uint64_t> (1) << i);
		// Добавляем результат хэширования очередного ключа
		results.emplace(this->_hash->hash <uint64_t> (&key, sizeof(key)));
	}
	// Проверяем отсутствие коллизий хэширования
	EXPECT_EQ(results.size(), static_cast <size_t> (64));
}

/**
 * @brief Тест хэширования данных большого размера
 *
 */
TEST_F(HashFixture, LargeHashTest){
	// Набор сформированных результатов хэширования
	std::set <uint64_t> results;
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 1024; size <= this->_buffer.size(); size += 64)
		// Добавляем результат хэширования очередного размера данных
		results.emplace(this->_hash->hash <uint64_t> (this->_buffer.data(), size));

	// Проверяем отличие результатов хэширования данных разного размера
	EXPECT_EQ(results.size(), static_cast <size_t> (((this->_buffer.size() - 1024) / 64) + 1));
}

/**
 * @brief Тест конструкторов объекта хэширования
 *
 */
TEST_F(HashFixture, ConstructorHashTest){
	// Создаём объект хэширования с начальным значением по умолчанию
	const awh::hash_t hash1;
	// Создаём объект хэширования с указанным начальным значением
	const awh::hash_t hash2(0x1234567890ABCDEFULL);
	// Проверяем начальное значение хэширования по умолчанию
	EXPECT_EQ(hash1.seed(), static_cast <uint64_t> (0));
	// Проверяем указанное начальное значение хэширования
	EXPECT_EQ(hash2.seed(), static_cast <uint64_t> (0x1234567890ABCDEFULL));
	// Проверяем размер обработанных данных объекта хэширования
	EXPECT_EQ(hash2.length(), static_cast <uint64_t> (0));
	// Проверяем отличие результатов хэширования объектов с разными начальными значениями
	EXPECT_NE(hash1.hash <uint64_t> ("Hello World!!!"), hash2.hash <uint64_t> ("Hello World!!!"));
	// Проверяем размер блока данных хэширования
	EXPECT_EQ(awh::hash_t::BLOCK, static_cast <size_t> (64));
}

/**
 * @brief Тест формирования результата хэширования в буфер
 *
 */
TEST_F(HashFixture, BufferHashTest){
	// Буфер результата одноразового хэширования
	uint8_t result1[48];
	// Буфер результата потокового хэширования
	uint8_t result2[48];
	// Создаём объект потокового хэширования
	awh::hash_t hash;
	// Выполняем добавление данных в потоковое хэширование
	hash.update(this->_buffer.data(), 150);
	/**
	 * Выполняем перебор размеров результата хэширования
	 */
	for(size_t length = 1; length <= sizeof(result1); length++){
		// Заполняем буфер результата одноразового хэширования
		::memset(result1, 0x00, sizeof(result1));
		// Заполняем буфер результата потокового хэширования
		::memset(result2, 0x00, sizeof(result2));
		// Выполняем формирование результата одноразового хэширования
		this->_hash->hash(this->_buffer.data(), 150, result1, length);
		// Выполняем формирование результата потокового хэширования
		hash.digest(result2, length);
		// Проверяем совпадение результатов одноразового и потокового хэширования
		EXPECT_EQ(::memcmp(result1, result2, length), 0);
	}
	// Выполняем формирование результата хэширования в отсутствующий буфер
	this->_hash->hash(this->_buffer.data(), 150, nullptr, sizeof(result1));
	// Выполняем формирование результата потокового хэширования в отсутствующий буфер
	hash.digest(nullptr, sizeof(result2));
	// Выполняем формирование результата хэширования нулевой длины
	this->_hash->hash(this->_buffer.data(), 150, result1, 0);
	// Выполняем формирование результата потокового хэширования нулевой длины
	hash.digest(result2, 0);
}

/**
 * @brief Тест добавления данных в потоковое хэширование
 *
 */
TEST_F(HashFixture, UpdateHashTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework";
	// Формируем буфер данных из текста
	const std::vector <char> buffer(text.begin(), text.end());
	// Создаём объект потокового хэширования для сырых данных
	awh::hash_t hash1;
	// Создаём объект потокового хэширования для текста
	awh::hash_t hash2;
	// Создаём объект потокового хэширования для буфера данных
	awh::hash_t hash3;
	// Выполняем добавление сырых данных в потоковое хэширование
	hash1.update(text.data(), text.size());
	// Выполняем добавление текста в потоковое хэширование
	hash2.update(text);
	// Выполняем добавление буфера данных в потоковое хэширование
	hash3.update(buffer);
	// Проверяем совпадение результата добавления текста
	EXPECT_EQ(hash2.digest <uint64_t> (), hash1.digest <uint64_t> ());
	// Проверяем совпадение результата добавления буфера данных
	EXPECT_EQ(hash3.digest <uint64_t> (), hash1.digest <uint64_t> ());
	// Проверяем размер обработанных данных
	EXPECT_EQ(hash3.length(), static_cast <uint64_t> (text.size()));

	// Выполняем добавление отсутствующих данных в потоковое хэширование
	hash1.update(nullptr, 128);
	// Выполняем добавление данных нулевого размера в потоковое хэширование
	hash1.update(text.data(), 0);
	// Проверяем сохранность размера обработанных данных
	EXPECT_EQ(hash1.length(), static_cast <uint64_t> (text.size()));
	// Проверяем сохранность результата потокового хэширования
	EXPECT_EQ(hash1.digest <uint64_t> (), hash2.digest <uint64_t> ());
}

/**
 * @brief Тест потокового хэширования данных размером с блок
 *
 */
TEST_F(HashFixture, BlockStreamHashTest){
	/**
	 * Выполняем перебор размеров данных кратных размеру блока хэширования
	 */
	for(size_t size = awh::hash_t::BLOCK; size <= (awh::hash_t::BLOCK * 8); size += awh::hash_t::BLOCK){
		// Создаём объект потокового хэширования блоками
		awh::hash_t hash1;
		// Создаём объект потокового хэширования половинами блока
		awh::hash_t hash2;
		/**
		 * Выполняем передачу данных блоками
		 */
		for(size_t offset = 0; offset < size; offset += awh::hash_t::BLOCK)
			// Выполняем добавление очередного блока данных
			hash1.update(this->_buffer.data() + offset, awh::hash_t::BLOCK);
		/**
		 * Выполняем передачу данных половинами блока
		 */
		for(size_t offset = 0; offset < size; offset += (awh::hash_t::BLOCK / 2))
			// Выполняем добавление очередной половины блока данных
			hash2.update(this->_buffer.data() + offset, (awh::hash_t::BLOCK / 2));

		// Проверяем совпадение результата хэширования блоками с одноразовым хэшированием
		EXPECT_EQ(hash1.digest <uint64_t> (), this->_hash->hash <uint64_t> (this->_buffer.data(), size));
		// Проверяем совпадение результата хэширования половинами блока с одноразовым хэшированием
		EXPECT_EQ(hash2.digest <uint64_t> (), this->_hash->hash <uint64_t> (this->_buffer.data(), size));
	}
}

/**
 * @brief Тест хэширования данных отличающихся размером и порядком байтов
 *
 */
TEST_F(HashFixture, DistinctHashTest){
	// Проверяем отличие результата хэширования данных разного размера
	EXPECT_NE(this->_hash->hash <uint64_t> ("Anyks"), this->_hash->hash <uint64_t> ("Anyks "));
	// Проверяем отличие результата хэширования данных с переставленными байтами
	EXPECT_NE(this->_hash->hash <uint64_t> ("Anyks Framework"), this->_hash->hash <uint64_t> ("Framework Anyks"));
	// Проверяем отличие результата хэширования данных отличающихся одним байтом
	EXPECT_NE(this->_hash->hash <uint64_t> ("Anyks Framework"), this->_hash->hash <uint64_t> ("Anyks Framewark"));
	// Набор сформированных результатов хэширования
	std::set <uint64_t> results;
	/**
	 * Выполняем перебор размеров данных состоящих из нулевых байтов
	 */
	for(size_t size = 0; size <= 256; size++){
		// Формируем буфер нулевых данных
		const std::vector <uint8_t> buffer(size, 0);
		// Добавляем результат хэширования нулевых данных
		results.emplace(this->_hash->hash <uint64_t> (buffer.data(), size));
	}
	// Проверяем отличие результатов хэширования нулевых данных разного размера
	EXPECT_EQ(results.size(), static_cast <size_t> (257));
}

/**
 * @brief Тест вывода результата хэширования во встроенные вещественные типы
 *
 */
TEST_F(HashFixture, RealHashTest){
	/**
	 * Выполняем перебор наборов данных для хэширования
	 */
	for(uint32_t i = 0; i < 100000; i++){
		// Выполняем формирование результата хэширования одинарной точности
		const float result1 = this->_hash->hash <float> (&i, sizeof(i));
		// Выполняем формирование результата хэширования двойной точности
		const double result2 = this->_hash->hash <double> (&i, sizeof(i));
		// Проверяем пригодность результата хэширования в качестве ключа
		ASSERT_TRUE(result1 == result1);
		// Проверяем пригодность результата хэширования в качестве ключа
		ASSERT_TRUE(result2 == result2);
		// Проверяем конечность результата хэширования
		ASSERT_FALSE(std::isinf(result1));
		// Проверяем конечность результата хэширования
		ASSERT_FALSE(std::isinf(result2));
	}
	// Проверяем повторяемость результата хэширования
	EXPECT_EQ(this->_hash->hash <double> ("Anyks Framework"), this->_hash->hash <double> ("Anyks Framework"));
	// Проверяем отличие результата хэширования других данных
	EXPECT_NE(this->_hash->hash <double> ("Anyks Framework"), this->_hash->hash <double> ("Anyks Framewark"));
}

/**
 * @brief Тест совпадения числового результата с потоком октетов
 *
 * @details Проверка не зависит от порядка байтов процессора: число собирается
 *          из потока октетов явными сдвигами, поэтому на процессоре с обратным
 *          порядком байтов тест поймает расхождение путей вывода результата.
 *
 */
TEST_F(HashFixture, NumericHashTest){
	// Буфер результата хэширования
	uint8_t result[8];
	/**
	 * Выполняем перебор размеров данных для хэширования
	 */
	for(size_t size = 0; size < 200; size++){
		// Выполняем формирование результата хэширования потоком октетов
		this->_hash->hash(this->_buffer.data(), size, result, sizeof(result));
		// Собираемое из потока октетов число
		uint64_t value = 0;
		/**
		 * Выполняем перебор всех октетов результата хэширования
		 */
		for(size_t i = 0; i < sizeof(result); i++)
			// Добавляем очередной октет результата хэширования в число
			value |= (static_cast <uint64_t> (result[i]) << (i * 8));

		// Создаём объект потокового хэширования
		awh::hash_t hash;
		// Выполняем добавление данных в потоковое хэширование
		hash.update(this->_buffer.data(), size);
		// Проверяем совпадение числового результата одноразового хэширования с потоком октетов
		ASSERT_EQ(this->_hash->hash <uint64_t> (this->_buffer.data(), size), value);
		// Проверяем совпадение числового результата потокового хэширования с потоком октетов
		ASSERT_EQ(hash.digest <uint64_t> (), value);
		// Проверяем совпадение быстрого пути потокового хэширования с потоком октетов
		ASSERT_EQ(hash.digest(), value);
		// Проверяем совпадение усечённого числового результата с потоком октетов
		ASSERT_EQ(this->_hash->hash <uint32_t> (this->_buffer.data(), size), static_cast <uint32_t> (value));
		// Проверяем совпадение усечённого результата потокового хэширования с потоком октетов
		ASSERT_EQ(hash.digest <uint16_t> (), static_cast <uint16_t> (value));
		// Проверяем совпадение знакового результата хэширования с потоком октетов
		ASSERT_EQ(this->_hash->hash <int64_t> (this->_buffer.data(), size), static_cast <int64_t> (value));
	}
}
