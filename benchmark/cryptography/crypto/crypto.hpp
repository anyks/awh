/**
 * @file crypto.hpp
 * @date 2026-08-01
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
 * @brief Заголовочный файл бенчмарков модуля криптографии — общее окружение сценариев,
 *        эталонные буферы данных разного размера и средства проведения замера
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_SYS_CRYPTO__
#define __AWH_BENCHMARK_SYS_CRYPTO__

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
#include "../../../include/sys/fmk.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/cryptography/crypto.hpp"

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
		 * @brief Пространство имён бенчмарков модуля криптографии
		 *
		 */
		namespace crypto {
			/**
			 * @brief Размер данных сценариев короткого сообщения в октетах
			 *
			 * @details Размер порядка сетевого кадра: на нём измеряется не пропускная
			 *          способность шифра, а стоимость самого вызова - отведение буфера
			 *          результата, выработка вектора инициализации и снятие имитовставки
			 *
			 */
			static constexpr size_t SHORT_SIZE = 64;
			/**
			 * @brief Размер данных сценариев порции потока в октетах
			 *
			 * @details Порция такого размера - обычная единица подачи в сетевых работах:
			 *          на ней стоимость вызова и стоимость шифрования сравнимы между собой
			 *
			 */
			static constexpr size_t CHUNK_SIZE = 1024;
			/**
			 * @brief Размер данных сценариев крупной порции потока в октетах
			 *
			 * @details На порции такого размера стоимость вызова уже пренебрежима, и
			 *          показатель отражает работу одного лишь шифра. Сличение с порцией
			 *          меньшего размера показывает, сколько стоит сама подача
			 *
			 */
			static constexpr size_t BULK_SIZE = (64 * 1024);
			/**
			 * @brief Размер данных сценариев крупного сообщения в октетах
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
			 * @brief Функция извлечения пропускной способности шифрования
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
			 *       шифрование целиком, и замерялась бы стоимость пустого цикла
			 *
			 * @return ссылка на контрольную сумму прогонов
			 *
			 */
			volatile uint64_t & checksum() noexcept;
			/**
			 * @brief Функция получения эталонного буфера данных
			 *
			 * @details Буфер заполнен значащими данными во всех октетах и общий для всех
			 *          сценариев: отрезок нужного размера берётся от его начала, поэтому
			 *          сличение сценариев между собой идёт на одних и тех же данных
			 *
			 * @return эталонный буфер данных
			 *
			 */
			const std::vector <uint8_t> & buffer() noexcept;
			/**
			 * @brief Функция получения объекта фреймворка
			 *
			 * @return объект фреймворка
			 *
			 */
			const awh::fmk_t * framework() noexcept;
			/**
			 * @brief Функция получения объекта работы с логами
			 *
			 * @return объект работы с логами
			 *
			 */
			const awh::log_t * logger() noexcept;
			/**
			 * @brief Функция получения эталонного объекта криптографии
			 *
			 * @details Объект заведён с паролем и солью и служит всем сценариям, шифрования
			 *          требующим. Ключ выводится единожды - при первом обращении, - и цену
			 *          вывода замеры сценариев шифрования не несут: она измеряется отдельным
			 *          сценарием вывода ключа
			 *
			 * @return эталонный объект криптографии
			 *
			 */
			awh::crypto_t & engine() noexcept;
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
			 * @details Замеру предшествует прогрев: он выводит данные в кэш процессора и
			 *          разогревает предсказатель переходов, поэтому первые операции к
			 *          установившемуся режиму отношения не имеют
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
				for(size_t i = 0; i < 16; i++)
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
			/**
			 * @brief Макрос объявления сценариев замера
			 *
			 * @details Всякий сценарий даёт два показателя - пропускную способность и
			 *          количество выделений памяти на операцию, - и оба берутся из одного
			 *          прогона: прогонять сценарий дважды значит мерить два разных прогона
			 *
			 * @param SUFFIX  окончание имён объявляемых функций
			 * @param OUTCOME функция получения итогов прогона сценария
			 *
			 */
			#define AWH_CRYPTO_SCENARIO(SUFFIX, OUTCOME)                            \
				static awh::benchmark::result_t speed ## SUFFIX() noexcept {        \
					awh::benchmark::result_t result;                                \
					const outcome_t & outcome = OUTCOME();                          \
					result.value = perSecond(outcome);                              \
					result.details = details(outcome);                              \
					return result;                                                  \
				}                                                                   \
				static awh::benchmark::result_t bytes ## SUFFIX() noexcept {        \
					awh::benchmark::result_t result;                                \
					const outcome_t & outcome = OUTCOME();                          \
					result.value = perBytes(outcome);                               \
					result.details = details(outcome);                              \
					return result;                                                  \
				}                                                                   \
				static awh::benchmark::result_t memory ## SUFFIX() noexcept {       \
					awh::benchmark::result_t result;                                \
					const outcome_t & outcome = OUTCOME();                          \
					result.value = perOperation(outcome);                           \
					result.details = details(outcome);                              \
					return result;                                                  \
				}
		};
	};
};

#endif // __AWH_BENCHMARK_SYS_CRYPTO__
