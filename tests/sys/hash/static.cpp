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
