/**
 * @file: common.hpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение эталонных стендов сравнения хэширования — эталонный буфер данных,
 *        параметры нагрузки, драйвер прогона, разбор параметров запуска и вывод результатов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_HASH__
#define __AWH_BENCHMARK_RIVAL_HASH__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

/**
 * @brief Пространство имён эталонных стендов сравнения хэширования
 *
 * @details Параметры нагрузки обязаны совпадать со сценариями
 *          `benchmark/sys/hash` библиотеки AWH: сравниваются реализации
 *          хэширования, а не разные объёмы работы, поэтому любое расхождение
 *          здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Размер эталонного буфера данных в октетах
	 *
	 */
	static constexpr size_t BUFFER_SIZE = (1024 * 1024);
	/**
	 * @brief Набор размеров данных сценариев одноразового хэширования в октетах
	 *
	 * @details Размеры совпадают с границами ветвей обработки обеих реализаций:
	 *          короткие данные и та, и другая обрабатывают отдельными путями по
	 *          длине, поэтому сравнение только на крупных буферах скрыло бы
	 *          самый частый случай — ключ ассоциативного контейнера
	 *
	 */
	static constexpr size_t SIZES[] = {4, 8, 16, 24, 32, 48, 64, 96, 128, 256, 1024, 4096, 65536, 1048576};
	/**
	 * @brief Количество размеров данных сценариев одноразового хэширования
	 *
	 */
	static constexpr size_t SIZES_COUNT = (sizeof(SIZES) / sizeof(SIZES[0]));
	/**
	 * @brief Объём данных, обрабатываемый одним замером, в октетах
	 *
	 * @details Количество операций замера выводится из этого объёма делением на
	 *          размер данных: так короткие и длинные сценарии занимают сравнимое
	 *          время, а показатель остаётся устойчивым на обоих концах набора
	 *
	 */
	static constexpr size_t VOLUME = (256 * 1024 * 1024);
	/**
	 * @brief Наименьшее количество операций замера
	 *
	 */
	static constexpr size_t MINIMUM_ROUNDS = 1000;
	/**
	 * @brief Количество повторов замера
	 *
	 * @details Из повторов берётся наилучший: замер конкурирует за процессор с
	 *          остальной системой, и медленные повторы отражают её работу, а не
	 *          работу измеряемой реализации
	 *
	 */
	static constexpr uint8_t ATTEMPTS = 5;
	/**
	 * @brief Количество операций прогрева перед замером
	 *
	 */
	static constexpr size_t WARMUP_ROUNDS = 64;
	/**
	 * @brief Размер порции данных сценария потоковой подачи в октетах
	 *
	 */
	static constexpr size_t STREAM_CHUNK = 1500;
	/**
	 * @brief Функция получения эталонного буфера данных
	 *
	 * @details Буфер заполнен значащими октетами во всех позициях и совпадает с
	 *          буфером сценариев `benchmark/sys/hash`
	 *
	 * @return эталонный буфер данных
	 *
	 */
	inline const std::vector <uint8_t> & buffer() noexcept {
		/**
		 * @brief Функция формирования эталонного буфера данных
		 *
		 * @return эталонный буфер данных
		 *
		 */
		static const std::vector <uint8_t> result = []() noexcept -> std::vector <uint8_t> {
			// Эталонный буфер данных
			std::vector <uint8_t> data(BUFFER_SIZE, 0);
			/**
			 * Выполняем заполнение эталонного буфера данных значащими октетами
			 */
			for(size_t i = 0; i < data.size(); i++)
				// Заполняем очередной октет эталонного буфера данных
				data[i] = static_cast <uint8_t> ((i * 131) ^ (i >> 3));
			// Выводим эталонный буфер данных
			return data;
		}();
		// Выводим эталонный буфер данных
		return result;
	}
	/**
	 * @brief Функция извлечения количества операций замера для размера данных
	 *
	 * @param size размер данных одной операции в октетах
	 * @return     количество операций замера
	 *
	 */
	inline size_t rounds(const size_t size) noexcept {
		// Определяем количество операций по объёму данных замера
		const size_t result = ((size > 0) ? (VOLUME / size) : MINIMUM_ROUNDS);
		// Выводим количество операций замера
		return ((result < MINIMUM_ROUNDS) ? MINIMUM_ROUNDS : result);
	}
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
	 * @details Драйвер общий для всех стендов: разница в обвязке замера иначе
	 *          оказалась бы неотделима от разницы в самих реализациях. Шаблонный,
	 *          а не виртуальный - косвенный вызов на каждую операцию исказил бы
	 *          сценарии коротких данных кратно
	 *
	 * @param count    количество операций замера
	 * @param size     размер данных одной операции в октетах
	 * @param callback функция выполнения одной операции
	 * @return         наилучшая пропускная способность в октетах в секунду
	 *
	 */
	inline double measure(const size_t count, const size_t size, CALLBACK callback) noexcept {
		// Наилучшая пропускная способность
		double result = 0.0;
		/**
		 * Выполняем прогрев измеряемой операции
		 */
		for(size_t i = 0; i < WARMUP_ROUNDS; i++)
			// Выполняем очередную операцию прогрева
			callback();
		/**
		 * Выполняем требуемое количество повторов замера
		 */
		for(uint8_t attempt = 0; attempt < ATTEMPTS; attempt++){
			// Запоминаем момент начала измерения
			const auto start = std::chrono::steady_clock::now();
			/**
			 * Выполняем измеряемую операцию требуемое количество раз
			 */
			for(size_t i = 0; i < count; i++)
				// Выполняем очередную измеряемую операцию
				callback();
			// Запоминаем момент окончания измерения
			const auto finish = std::chrono::steady_clock::now();
			// Определяем затраченное на замер время
			const double seconds = std::chrono::duration <double> (finish - start).count();
			/**
			 * Если время замера измерено
			 */
			if(seconds > 0.0){
				// Определяем пропускную способность замера
				const double value = ((static_cast <double> (count) * static_cast <double> (size)) / seconds);
				/**
				 * Если пропускная способность замера превышает наилучшую
				 */
				if(value > result)
					// Запоминаем наилучшую пропускную способность
					result = value;
			}
		}
		// Выводим наилучшую пропускную способность
		return result;
	}
	/**
	 * @brief Функция вывода заголовка отчёта стенда
	 *
	 * @param name название измеряемой реализации
	 *
	 */
	inline void header(const char * name) noexcept {
		// Выводим название измеряемой реализации
		::printf("# %s\n", name);
		// Выводим заголовок таблицы результатов
		::printf("%-24s %16s %16s\n", "Сценарий", "октетов/с", "нс/операцию");
	}
	/**
	 * @brief Функция вывода результата сценария
	 *
	 * @param name  название сценария
	 * @param size  размер данных одной операции в октетах
	 * @param value пропускная способность в октетах в секунду
	 *
	 */
	inline void report(const std::string & name, const size_t size, const double value) noexcept {
		// Определяем среднее время выполнения одной операции в наносекундах
		const double nanoseconds = ((value > 0.0) ? ((static_cast <double> (size) * 1e9) / value) : 0.0);
		// Выводим результат сценария
		::printf("%-24s %16.0f %16.2f\n", name.c_str(), value, nanoseconds);
	}
	/**
	 * @brief Функция вывода контрольной суммы прогона
	 *
	 * @details Контрольная сумма выводится по требованию и служит доказательством
	 *          того, что измеряемое хэширование действительно выполнялось: без
	 *          потребителя результата компилятор вправе устранить его целиком
	 *
	 * @param checksum контрольная сумма прогона
	 *
	 */
	inline void checksum(const uint64_t checksum) noexcept {
		// Выводим контрольную сумму прогона
		::printf("checksum: %llu\n", static_cast <unsigned long long> (checksum));
	}
};

#endif // __AWH_BENCHMARK_RIVAL_HASH__
