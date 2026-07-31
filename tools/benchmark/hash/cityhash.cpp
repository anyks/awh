/**
 * @file: cityhash.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Эталонный стенд сравнения хэширования на библиотеке CityHash — те же сценарии
 *        нагрузки, что и у стенда AWH, выполненные средствами исходных текстов
 *        подмодуля submodules/cityhash
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы библиотеки CityHash
 */
#include <city.h>

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
 * @details Библиотека предоставляет только одноразовое хэширование: состояния,
 *          которое можно было бы дополнять частями данных, у неё нет вовсе,
 *          поэтому сценарий потоковой подачи здесь не выполняется. 128-битный
 *          результат возвращается парой 64-битных чисел, поскольку целого типа
 *          такой разрядности в языке нет
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
		// Получаем эталонный буфер данных
		const char * data = reinterpret_cast <const char *> (buffer().data());
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
				gChecksum += ::CityHash64(data, size);
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
		// Получаем эталонный буфер данных
		const char * data = reinterpret_cast <const char *> (buffer().data());
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
				const uint128 result = ::CityHash128(data, size);
				// Накапливаем контрольную сумму прогона
				gChecksum += (Uint128Low64(result) + Uint128High64(result));
			});
			// Выводим результат сценария
			report("hash128/" + to_string(size), size, value);
		}
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
	header("CityHash");
	// Выполняем прогон сценариев одноразового хэширования
	::oneshot();
	// Выполняем прогон сценариев хэширования в 128-битный результат
	::wide();
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
