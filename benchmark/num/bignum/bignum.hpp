/**
 * @file bignum.hpp
 * @date 2026-07-26
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
 * @brief Заголовочный файл бенчмарков модуля длинных чисел — общее окружение сценариев,
 *        эталонные операнды разной разрядности и средства проведения замера
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_BIGNUM__
#define __AWH_BENCHMARK_BIGNUM__

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <string>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/num/bignum.hpp"

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
		 * @brief Пространство имён бенчмарков модуля длинных чисел
		 *
		 */
		namespace bignum {
			/**
			 * @brief Структура итогов прогона сценария
			 *
			 */
			typedef struct Outcome {
				// Количество выполненных операций
				size_t operations;
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
				 operations(0), seconds(0.0), allocations(0), allocated(0) {}
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
			 *       вычисление целиком, и замерялась бы стоимость пустого цикла
			 *
			 * @return ссылка на контрольную сумму прогонов
			 *
			 */
			volatile uint64_t & checksum() noexcept;
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
			 * @details Замеру предшествует прогрев: первые операции выводят внутренние
			 *          буферы модуля на рабочий объём, и их выделения памяти к
			 *          установившемуся режиму отношения не имеют
			 *
			 * @param rounds   количество выполняемых операций
			 * @param callback функция выполнения одной операции
			 * @return         итоги прогона сценария
			 *
			 */
			inline outcome_t measure(const size_t rounds, CALLBACK callback) noexcept {
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
				// Устанавливаем затраченное время
				result.seconds = std::chrono::duration <double> (finish - start).count();
				// Получаем статистику выделений памяти
				awh::benchmark::allocations(result.allocations, result.allocated);
				// Выводим итоги прогона сценария
				return result;
			}
			/**
			 * @brief Функция получения эталонного 128-битного беззнакового операнда
			 *
			 * @details Все разряды операндов значащие: обнуление старших разрядов
			 *          сократило бы объём работы деления и умножения и дало бы
			 *          завышенные показатели
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::uint128_t & first128() noexcept;
			/**
			 * @brief Функция получения второго эталонного 128-битного беззнакового операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::uint128_t & second128() noexcept;
			/**
			 * @brief Функция получения эталонного 512-битного беззнакового операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::uint512_t & first512() noexcept;
			/**
			 * @brief Функция получения эталонного 2048-битного беззнакового операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::uint2048_t & first2048() noexcept;
			/**
			 * @brief Функция получения второго эталонного 2048-битного беззнакового операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::uint2048_t & second2048() noexcept;
			/**
			 * @brief Функция получения эталонного 64-битного вещественного операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::real64_t & first64r() noexcept;
			/**
			 * @brief Функция получения второго эталонного 64-битного вещественного операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::real64_t & second64r() noexcept;
			/**
			 * @brief Функция получения эталонного 128-битного вещественного операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::real128_t & first128r() noexcept;
			/**
			 * @brief Функция получения второго эталонного 128-битного вещественного операнда
			 *
			 * @return эталонный операнд
			 *
			 */
			const awh::real128_t & second128r() noexcept;
			/**
			 * @brief Функция получения эталонной десятичной записи 128-битного целого числа
			 *
			 * @return эталонная десятичная запись
			 *
			 */
			const std::string & decimal128() noexcept;
			/**
			 * @brief Функция получения эталонной десятичной записи вещественного числа
			 *
			 * @return эталонная десятичная запись
			 *
			 */
			const std::string & decimalReal() noexcept;
		};
	};
};

#endif // __AWH_BENCHMARK_BIGNUM__
