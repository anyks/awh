/**
 * @file common.hpp
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
 * @brief Общее окружение эталонных стендов сравнения транспортного протокола QUIC —
 *        параметры нагрузки, учёт выделений памяти, разбор параметров запуска и
 *        вывод результатов в формате набора бенчмарков `benchmark/proto/quic`
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_QUIC__
#define __AWH_BENCHMARK_RIVAL_QUIC__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>

/**
 * @brief Пространство имён эталонных стендов сравнения транспортного протокола QUIC
 *
 * @details Объём передачи, размер блока постановки в очередь и число потоков
 *          обязаны совпадать со сценариями `benchmark/proto/quic` библиотеки AWH:
 *          сравниваются реализации транспорта, а не разные объёмы работы, поэтому
 *          любое расхождение здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Объём передаваемых данных сценария в октетах
	 *
	 */
	static constexpr size_t PAYLOAD_SIZE = (64 * 1024 * 1024);
	/**
	 * @brief Размер блока, ставящегося приложением в очередь отправки
	 *
	 */
	static constexpr size_t BLOCK_SIZE = (64 * 1024);
	/**
	 * @brief Количество потоков сценария передачи по множеству потоков
	 *
	 */
	static constexpr size_t STREAM_COUNT = 64;
	/**
	 * @brief Структура итогов прогона передачи данных
	 *
	 * @details Повторяет структуру итогов сценария `benchmark/proto/quic`, чтобы
	 *          сведения о прогоне печатались тем же форматом и сводились в таблицу
	 *          без пересчёта
	 *
	 */
	typedef struct Transfer {
		// Количество переданных датаграмм
		size_t datagrams;
		// Количество принятых октетов
		size_t received;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти в октетах
		size_t bytes;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Transfer() noexcept :
		 datagrams(0), received(0), seconds(0.0), allocations(0), bytes(0) {}
	} transfer_t;
	/**
	 * @brief Внутреннее состояние учёта выделений памяти
	 *
	 * @details Учёт ведётся ровно вокруг измеряемого участка передачи. Для стенда
	 *          на языке C++ выделения считают перегруженные операторы выделения
	 *          памяти, для стенда на языке C - собственный распределитель, переданный
	 *          движку транспорта. И тот и другой охватывают только выделения самого
	 *          транспорта: криптографический слой BoringSSL общий у обоих стендов и
	 *          в учёт не попадает ни у того, ни у другого
	 *
	 */
	namespace counter {
		// Количество выполненных выделений памяти
		static size_t count = 0;
		// Суммарный объём выделенной памяти в октетах
		static size_t bytes = 0;
		// Флаг активности учёта выделений памяти
		static bool enabled = false;
	};
	/**
	 * @brief Функция управления учётом выделений памяти
	 *
	 * @param mode режим учёта выделений памяти
	 *
	 */
	static inline void counting(const bool mode) noexcept {
		// Если учёт выделений памяти включается
		if(mode){
			// Сбрасываем количество выполненных выделений памяти
			counter::count = 0;
			// Сбрасываем объём выделенной памяти
			counter::bytes = 0;
		}
		// Устанавливаем режим учёта выделений памяти
		counter::enabled = mode;
	}
	/**
	 * @brief Функция учёта одного выполненного выделения памяти
	 *
	 * @param size размер выделенной памяти в октетах
	 *
	 */
	static inline void account(const size_t size) noexcept {
		// Если учёт выделений памяти активен
		if(counter::enabled){
			// Считаем выполненное выделение памяти
			counter::count++;
			// Суммируем объём выделенной памяти
			counter::bytes += size;
		}
	}
	/**
	 * @brief Функция проверки соответствия сценария фильтру запуска
	 *
	 * @param name   название сценария
	 * @param filter фильтр названий сценариев
	 * @return       результат проверки соответствия
	 *
	 */
	static inline bool selected(const char * name, const char * filter) noexcept {
		// Если фильтр не задан, выполняются все сценарии
		return ((filter == nullptr) || (::strstr(name, filter) != nullptr));
	}
	/**
	 * @brief Функция получения фильтра названий сценариев из параметров запуска
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     фильтр названий сценариев
	 *
	 */
	static inline const char * filter(const int32_t argc, char ** argv) noexcept {
		/**
		 * Перебираем параметры запуска стенда
		 */
		for(int32_t i = 1; i < argc; i++){
			// Если задан фильтр названий выполняемых сценариев
			if(::strncmp(argv[i], "--filter=", 9) == 0)
				// Выводим фильтр названий сценариев
				return (argv[i] + 9);
		}
		// Выводим отсутствие фильтра названий сценариев
		return nullptr;
	}
	/**
	 * @brief Функция вывода результата прогона сценария пропускной способности
	 *
	 * @note Формат вывода повторяет набор бенчмарков AWH октет в октет, поэтому
	 *       результаты стендов и библиотеки сводятся в одну таблицу без пересчёта
	 *
	 * @param name     название сценария
	 * @param transfer итоги прогона передачи данных
	 *
	 */
	static inline void throughput(const char * name, const transfer_t & transfer) noexcept {
		// Объём переданных данных в мебибайтах
		const double megabytes = (static_cast <double> (transfer.received) / (1024.0 * 1024.0));
		// Вычисляем пропускную способность в мебибайтах в секунду
		const double value = ((transfer.seconds > 0.0) ? (megabytes / transfer.seconds) : 0.0);
		// Выводим измеренное значение характеристики
		::printf("%-34s %14.2f   (%s)\n", name, value, "МБ/с");
		// Выводим сведения о прогоне сценария
		::printf(
			"%36sдатаграмм: %zu, выделений: %zu (%.2f на датаграмму), выделено: %.1f МБ (%.2f× от переданного)\n",
			"", transfer.datagrams, transfer.allocations,
			(transfer.datagrams > 0 ? (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.datagrams)) : 0.0),
			(static_cast <double> (transfer.bytes) / (1024.0 * 1024.0)),
			(transfer.received > 0 ? (static_cast <double> (transfer.bytes) / static_cast <double> (transfer.received)) : 0.0)
		);
	}
	/**
	 * @brief Функция вывода результата прогона сценария выделений памяти
	 *
	 * @param name     название сценария
	 * @param transfer итоги прогона передачи данных
	 *
	 */
	static inline void allocations(const char * name, const transfer_t & transfer) noexcept {
		// Вычисляем количество выделений памяти на одну датаграмму
		const double value = ((transfer.datagrams > 0) ? (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.datagrams)) : 0.0);
		// Выводим измеренное значение характеристики
		::printf("%-34s %14.2f   (%s)\n", name, value, "выделений");
		// Выводим сведения о прогоне сценария
		::printf(
			"%36sвыделено: %.1f МБ (%.2f× от переданного)\n",
			"", (static_cast <double> (transfer.bytes) / (1024.0 * 1024.0)),
			(transfer.received > 0 ? (static_cast <double> (transfer.bytes) / static_cast <double> (transfer.received)) : 0.0)
		);
	}
	/**
	 * @brief Функция вывода сообщения о неудачном прогоне сценария
	 *
	 * @param name   название сценария
	 * @param reason причина отказа от прогона
	 *
	 */
	static inline void skip(const char * name, const char * reason) noexcept {
		// Выводим сообщение о неудачном прогоне сценария
		::printf("%-34s %14s   (%s)\n", name, "—", reason);
	}
};

#endif // __AWH_BENCHMARK_RIVAL_QUIC__
