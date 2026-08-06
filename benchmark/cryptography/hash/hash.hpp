/**
 * @file: hash.hpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл бенчмарков модуля хэширования — общее окружение сценариев,
 *        эталонные буферы данных разного размера и средства проведения замера
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_HASH__
#define __AWH_BENCHMARK_HASH__

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/cryptography/hash.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён бенчмарков
	 *
	 */
	namespace benchmark {
		/**
		 * @brief Пространство имён бенчмарков модуля хэширования
		 *
		 */
		namespace hash {
			/**
			 * @brief Размер данных сценариев ключа ассоциативного контейнера в октетах
			 *
			 * @details Восемь октетов - это машинное слово, самый частый ключ
			 *          хэш-таблицы. На таком размере измеряется не пропускная
			 *          способность, а стоимость самого вызова: работы с данными
			 *          там меньше, чем работы по её запуску
			 *
			 */
			static constexpr size_t TINY_SIZE = 8;
			/**
			 * @brief Размер данных сценариев короткого ключа в октетах
			 *
			 * @details Шестнадцать октетов - граница первой ветви обработки:
			 *          данные до этого размера читаются двумя числами внахлёст
			 *          и сворачиваются без обращения к состоянию движка
			 *
			 */
			static constexpr size_t SHORT_SIZE = 16;
			/**
			 * @brief Размер данных сценариев текстового ключа в октетах
			 *
			 */
			static constexpr size_t TEXT_SIZE = 32;
			/**
			 * @brief Размер данных сценариев размером с блок в октетах
			 *
			 * @details Размер блока движка. Данные до этой границы включительно
			 *          обрабатываются коротким путём, свыше - через состояние из
			 *          четырёх независимых разрядов
			 *
			 */
			static constexpr size_t BLOCK_SIZE = 64;
			/**
			 * @brief Размер данных сценариев среднего размера в октетах
			 *
			 */
			static constexpr size_t MEDIUM_SIZE = 256;
			/**
			 * @brief Размер данных сценариев размером со страницу памяти в октетах
			 *
			 */
			static constexpr size_t PAGE_SIZE = 4096;
			/**
			 * @brief Размер данных сценариев потоковой обработки в октетах
			 *
			 */
			static constexpr size_t LARGE_SIZE = (1024 * 1024);
			/**
			 * @brief Структура итогов прогона сценария
			 *
			 */
			typedef struct Outcome {
				// Количество выполненных операций
				size_t operations;
				// Размер обработанных данных одной операции в октетах
				size_t size;
				// Затраченное время в секундах
				double seconds;
				// Количество выполненных выделений памяти
				size_t allocations;
				// Суммарный объём выделенной памяти в октетах
				size_t allocated;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Outcome() noexcept :
				 operations(0), size(0), seconds(0.0), allocations(0), allocated(0) {}
			} outcome_t;
			/**
			 * @brief Функция формирования сведений о прогоне сценария
			 *
			 * @param output итоги прогона сценария
			 * @return       сведения о прогоне для вывода
			 *
			 */
			std::string details(const outcome_t & output) noexcept;
			/**
			 * @brief Функция извлечения количества операций в секунду
			 *
			 * @param output итоги прогона сценария
			 * @return       количество операций в секунду
			 *
			 */
			double perSecond(const outcome_t & output) noexcept;
			/**
			 * @brief Функция извлечения пропускной способности хэширования
			 *
			 * @param output итоги прогона сценария
			 * @return       количество обработанных октетов в секунду
			 *
			 */
			double perBytes(const outcome_t & output) noexcept;
			/**
			 * @brief Функция извлечения количества выделений памяти на одну операцию
			 *
			 * @param output итоги прогона сценария
			 * @return       количество выделений памяти на одну операцию
			 *
			 */
			double perOperation(const outcome_t & output) noexcept;
			/**
			 * @brief Функция получения контрольной суммы прогонов
			 *
			 * @note Накопление результатов операций в контрольную сумму обязательно:
			 *       без потребителя результата компилятор вправе устранить измеряемое
			 *       хэширование целиком, и замерялась бы стоимость пустого цикла
			 *
			 * @return ссылка на контрольную сумму прогонов
			 *
			 */
			volatile uint64_t & checksum() noexcept;
			/**
			 * @brief Функция получения эталонного буфера данных
			 *
			 * @details Буфер заполнен значащими данными во всех октетах и общий
			 *          для всех сценариев: отрезок нужного размера берётся от его
			 *          начала, поэтому сравнение сценариев между собой идёт на
			 *          одних и тех же данных
			 *
			 * @return эталонный буфер данных
			 *
			 */
			const std::vector <uint8_t> & buffer() noexcept;
			/**
			 * @brief Функция получения эталонного объекта хэширования
			 *
			 * @return эталонный объект хэширования
			 *
			 */
			const awh::hash_t & engine() noexcept;
			/**
			 * @brief Шаблон типа измеряемого сценария
			 *
			 * @tparam CALLBACK тип функции выполнения одной операции
			 *
			 */
			template <typename CALLBACK>
			/**
			 * @brief Функция проведения замера сценария
			 *
			 * @details Замеру предшествует прогрев: он выводит данные в кэш процессора
			 *          и разогревает предсказатель переходов, поэтому первые операции
			 *          к установившемуся режиму отношения не имеют
			 *
			 * @param rounds   количество выполняемых операций
			 * @param size     размер обрабатываемых одной операцией данных в октетах
			 * @param callback функция выполнения одной операции
			 * @return         итоги прогона сценария
			 *
			 */
			inline outcome_t measure(const size_t rounds, const size_t size, CALLBACK callback) noexcept {
				/**
				 * Выполняем прогрев измеряемой операции
				 */
				for(size_t i = 0; i < 64; i++)
					// Выполняем очередную операцию прогрева
					callback();
				// Включаем учёт выделений памяти
				awh::benchmark::counting(true);
				// Запоминаем момент начала измерения
				const auto start = std::chrono::steady_clock::now();
				/**
				 * Выполняем измеряемую операцию требуемое количество раз
				 */
				for(size_t i = 0; i < rounds; i++)
					// Выполняем очередную измеряемую операцию
					callback();
				// Запоминаем момент окончания измерения
				const auto finish = std::chrono::steady_clock::now();
				// Отключаем учёт выделений памяти
				awh::benchmark::counting(false);
				// Итоги прогона сценария
				outcome_t result;
				// Устанавливаем количество выполненных операций
				result.operations = rounds;
				// Устанавливаем размер обработанных данных одной операции
				result.size = size;
				// Устанавливаем затраченное время
				result.seconds = std::chrono::duration <double> (finish - start).count();
				// Получаем статистику выделений памяти
				awh::benchmark::allocations(result.allocations, result.allocated);
				// Выводим итоги прогона сценария
				return result;
			}
		};
	};
};

#endif // __AWH_BENCHMARK_HASH__
