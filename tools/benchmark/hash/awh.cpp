/**
 * @file: awh.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения хэширования на модуле AWH — те же сценарии нагрузки,
 *        что и у стенда CityHash, выполненные средствами модуля src/sys/hash
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/hash.hpp>

/**
 * Подключаем общее окружение эталонных стендов
 */
#include "common.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён эталонных стендов
 */
using namespace rival;

/**
 * @brief Внутреннее окружение стенда
 *
 * @details Модуль формирует хэш любой разрядности как поток октетов, поэтому
 *          128-битный результат снимается в массив октетов - ровно так же, как
 *          библиотека CityHash отдаёт его парой чисел. Вывод в длинное число
 *          модуля BigNum вынесен в отдельный сценарий: он показывает цену
 *          самого длинного числа, которой у сравниваемой библиотеки нет
 *
 */
namespace {
	// Контрольная сумма прогона
	static uint64_t gChecksum = 0;
	/**
	 * @brief Функция прогона сценариев одноразового хэширования
	 *
	 */
	static void oneshot() noexcept {
		// Создаём объект хэширования данных
		const awh::hash_t hash;
		// Получаем эталонный буфер данных
		const uint8_t * data = buffer().data();
		/**
		 * Выполняем перебор всех размеров данных сценариев
		 */
		for(size_t i = 0; i < SIZES_COUNT; i++){
			// Получаем размер данных очередного сценария
			const size_t size = SIZES[i];
			// Определяем количество операций замера
			const size_t count = rounds(size);
			// Выполняем замер сценария
			const double value = measure(count, size, [&]() noexcept {
				// Выполняем хэширование буфера данных
				gChecksum += hash.hash <uint64_t> (data, size);
			});
			// Выводим результат сценария
			report("hash64/" + to_string(size), size, value);
		}
	}
	/**
	 * @brief Функция прогона сценариев хэширования в 128-битный результат
	 *
	 */
	static void wide() noexcept {
		// Создаём объект хэширования данных
		const awh::hash_t hash;
		// Получаем эталонный буфер данных
		const uint8_t * data = buffer().data();
		/**
		 * Выполняем перебор всех размеров данных сценариев
		 */
		for(size_t i = 0; i < SIZES_COUNT; i++){
			// Получаем размер данных очередного сценария
			const size_t size = SIZES[i];
			// Определяем количество операций замера
			const size_t count = rounds(size);
			// Выполняем замер сценария
			const double value = measure(count, size, [&]() noexcept {
				// Выполняем хэширование буфера данных
				const array <uint8_t, 16> result = hash.hash <array <uint8_t, 16>> (data, size);
				// Первая половина результата хэширования
				uint64_t low = 0;
				// Вторая половина результата хэширования
				uint64_t high = 0;
				// Копируем первую половину результата хэширования
				::memcpy(&low, result.data(), sizeof(low));
				// Копируем вторую половину результата хэширования
				::memcpy(&high, result.data() + sizeof(low), sizeof(high));
				// Накапливаем контрольную сумму прогона
				gChecksum += (low + high);
			});
			// Выводим результат сценария
			report("hash128/" + to_string(size), size, value);
		}
	}
	/**
	 * @brief Функция прогона сценариев хэширования в длинное число
	 *
	 * @details Сценарий выполняется только этим стендом: у сравниваемой
	 *          библиотеки результата шире 128 бит нет вовсе
	 *
	 */
	static void bignum() noexcept {
		// Создаём объект хэширования данных
		const awh::hash_t hash;
		// Получаем эталонный буфер данных
		const uint8_t * data = buffer().data();
		/**
		 * Выполняем перебор всех размеров данных сценариев
		 */
		for(size_t i = 0; i < SIZES_COUNT; i++){
			// Получаем размер данных очередного сценария
			const size_t size = SIZES[i];
			// Определяем количество операций замера
			const size_t count = rounds(size);
			// Выполняем замер сценария
			const double value = measure(count, size, [&]() noexcept {
				// Выполняем хэширование буфера данных
				const awh::uint256_t result = hash.hash <awh::uint256_t> (data, size);
				// Накапливаем контрольную сумму прогона
				gChecksum += result.data()[0];
			});
			// Выводим результат сценария
			report("bignum256/" + to_string(size), size, value);
		}
	}
	/**
	 * @brief Функция прогона сценария потокового хэширования
	 *
	 * @details У CityHash потокового режима нет вовсе, поэтому сценарий
	 *          выполняется только этим стендом и в сравнение не входит
	 *
	 */
	static void streaming() noexcept {
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Определяем количество операций замера
		const size_t count = rounds(data.size());
		// Выполняем замер сценария
		const double value = measure(count, data.size(), [&]() noexcept {
			// Создаём объект потокового хэширования
			awh::hash_t hash;
			/**
			 * Выполняем передачу данных в потоковое хэширование порциями
			 */
			for(size_t offset = 0; offset < data.size(); offset += STREAM_CHUNK)
				// Выполняем добавление очередной порции данных
				hash.update(data.data() + offset, ((data.size() - offset) < STREAM_CHUNK ? (data.size() - offset) : STREAM_CHUNK));

			// Накапливаем контрольную сумму прогона
			gChecksum += hash.digest <uint64_t> ();
		});
		// Выводим результат сценария
		report("stream/" + to_string(STREAM_CHUNK), data.size(), value);
	}
};

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Выводим заголовок отчёта стенда
	header("AWH");
	// Выполняем прогон сценариев одноразового хэширования
	::oneshot();
	// Выполняем прогон сценариев хэширования в 128-битный результат
	::wide();
	// Выполняем прогон сценариев хэширования в длинное число
	::bignum();
	// Выполняем прогон сценария потокового хэширования
	::streaming();
	/**
	 * Выполняем перебор всех параметров запуска
	 */
	for(int32_t i = 1; i < argc; i++){
		/**
		 * Если запрошен вывод контрольной суммы прогона
		 */
		if(::strcmp(argv[i], "--checksum") == 0)
			// Выводим контрольную сумму прогона
			checksum(gChecksum);
	}
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
