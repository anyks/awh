/**
 * @file abc.hpp
 * @date 2026-08-19
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
 * @brief Заголовочный файл замеров бинарного контейнера ABC — общее окружение
 *        сценариев, эталонные записи всех путей разбора и средства проведения замера
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_CODEC_ABC__
#define __AWH_BENCHMARK_CODEC_ABC__

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/codec/abc/abc.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён замеров
	 *
	 */
	namespace benchmark {
		/**
		 * @brief Пространство имён замеров бинарного контейнера ABC
		 *
		 * @note Пространство имён названо по содержимому, а не по модулю: имя codec
		 *       внутри пространства имён замеров перекрыло бы пространство имён самого
		 *       модуля awh::codec, а имя abc - пространство имён контейнера
		 *
		 */
		namespace binary {
			/**
			 * @brief Структура итогов прогона сценария
			 *
			 */
			typedef struct Outcome {
				// Количество обработанных октетов записи
				size_t bytes;
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
				 bytes(0), operations(0), seconds(0.0), allocations(0), allocated(0) {}
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
			 * @brief Функция извлечения пропускной способности обработки
			 *
			 * @param output итоги прогона сценария
			 * @return       пропускная способность в мегабайтах в секунду
			 *
			 */
			double perSecond(const outcome_t & output) noexcept;
			/**
			 * @brief Функция извлечения количества выделений памяти на одну запись
			 *
			 * @param output итоги прогона сценария
			 * @return       количество выделений памяти на одну запись
			 *
			 */
			double perDocument(const outcome_t & output) noexcept;
			/**
			 * @brief Функция извлечения задержки обработки одной записи
			 *
			 * @param output итоги прогона сценария
			 * @return       задержка обработки одной записи в микросекундах
			 *
			 */
			double perLatency(const outcome_t & output) noexcept;
			/**
			 * @brief Функция проверки работоспособности учёта выделений памяти
			 *
			 * @details Показатель «не более» при молчащем счётчике отчитывается «уложился»
			 *          всегда: нулевой расход меньше всякого порога. Проверка эта отделяет
			 *          исправный модуль от неработающего учёта
			 *
			 * @param output итоги прогона сценария
			 * @param result заполняемый результат измерения
			 * @return       признак работоспособности учёта
			 *
			 */
			bool counted(const outcome_t & output, awh::benchmark::result_t & result) noexcept;
			/**
			 * @brief Функция проверки того, что сценарий выполнил работу
			 *
			 * @details Показатель «на одну операцию» при нуле операций выдал бы ноль, а
			 *          ноль укладывается во всякий порог с верхней границей: молчание
			 *          сценария отчиталось бы успехом
			 *
			 * @param output итоги прогона сценария
			 * @param result заполняемый результат измерения
			 * @return       признак выполненной работы
			 *
			 */
			bool worked(const outcome_t & output, awh::benchmark::result_t & result) noexcept;
			/**
			 * @brief Функция получения контрольной суммы прогонов
			 *
			 * @note Накопление итогов в контрольную сумму обязательно: без потребителя
			 *       результата компилятор вправе устранить измеряемую работу целиком, и
			 *       замерялась бы стоимость пустого цикла
			 *
			 * @return ссылка на контрольную сумму прогонов
			 *
			 */
			volatile uint64_t & checksum() noexcept;
			/**
			 * @brief Шаблон типа измеряемого сценария
			 *
			 * @tparam CALLBACK тип функции обработки одной записи
			 *
			 */
			template <typename CALLBACK>
			/**
			 * @brief Функция проведения замера сценария
			 *
			 * @details Замеру предшествует прогрев: первая обработка выводит на рабочий
			 *          режим кэш процессора и распределитель памяти, и её стоимость к
			 *          установившемуся режиму отношения не имеет
			 *
			 * @param bytes    размер обрабатываемой записи в октетах
			 * @param rounds   количество обрабатываемых записей
			 * @param callback функция обработки одной записи
			 * @return         итоги прогона сценария
			 *
			 */
			inline outcome_t measure(const size_t bytes, const size_t rounds, CALLBACK callback) noexcept {
				// Выполняем прогрев измеряемой работы
				checksum() += static_cast <uint64_t> (callback());
				// Включаем учёт выделений памяти
				awh::benchmark::counting(true);
				// Запоминаем момент начала измерения
				const auto start = std::chrono::steady_clock::now();
				// Накопитель итогов работы
				uint64_t accumulator = 0;
				/**
				 * Выполняем обработку требуемого количества записей
				 */
				for(size_t i = 0; i < rounds; i++)
					// Выполняем обработку очередной записи
					accumulator += static_cast <uint64_t> (callback());
				// Запоминаем момент окончания измерения
				const auto finish = std::chrono::steady_clock::now();
				// Отключаем учёт выделений памяти
				awh::benchmark::counting(false);
				// Накапливаем контрольную сумму прогона
				checksum() += accumulator;
				// Итоги прогона сценария
				outcome_t result;
				// Устанавливаем количество обработанных октетов
				result.bytes = (bytes * rounds);
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
			 * @brief Функция получения эталонной записи ответа службы
			 *
			 * @details Все эталонные записи собираются однократно до замера и разбираются
			 *          по указателям на их октеты: сборка записи внутри измеряемого цикла
			 *          вносила бы в замер стоимость сборки вместо стоимости разбора
			 *
			 * @return эталонная запись ответа службы
			 *
			 */
			const std::vector <uint8_t> & service() noexcept;
			/**
			 * @brief Функция получения эталонной крупной записи
			 *
			 * @details Размер выбран заведомо превосходящим кэш последнего уровня: разбор
			 *          записи, целиком укладывающейся в кэш, показывает скорость работы с
			 *          кэшем, а не установившуюся пропускную способность
			 *
			 * @return эталонная крупная запись
			 *
			 */
			const std::vector <uint8_t> & large() noexcept;
			/**
			 * @brief Функция получения эталонной записи с преобладанием чисел
			 *
			 * @return эталонная запись с преобладанием чисел
			 *
			 */
			const std::vector <uint8_t> & numbers() noexcept;
			/**
			 * @brief Функция получения эталонной записи с преобладанием строк
			 *
			 * @return эталонная запись с преобладанием строк
			 *
			 */
			const std::vector <uint8_t> & strings() noexcept;
			/**
			 * @brief Функция получения эталонной записи с преобладанием двоичных значений
			 *
			 * @details Двоичное значение есть то, ради чего двоичный контейнер и берут: в
			 *          текстовом виде оно потребовало бы перекодировки, а здесь ложится
			 *          как есть
			 *
			 * @return эталонная запись с преобладанием двоичных значений
			 *
			 */
			const std::vector <uint8_t> & blobs() noexcept;
			/**
			 * @brief Функция получения эталонной записи с глубокой вложенностью
			 *
			 * @return эталонная запись с глубокой вложенностью
			 *
			 */
			const std::vector <uint8_t> & nested() noexcept;
		};
	};
};

#endif // __AWH_BENCHMARK_CODEC_ABC__
