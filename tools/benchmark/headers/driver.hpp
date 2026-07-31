/**
 * @file: driver.hpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общий драйвер стендов сравнения контейнера HTTP-заголовков
 *
 * @details Границы прогрева и замера обязаны совпадать у всех сравниваемых
 *          реализаций: стенд, заводящий их у себя, рано или поздно разойдётся
 *          с соседним, и сравнение начнёт мерить разницу драйверов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_HEADERS_DRIVER__
#define __AWH_BENCHMARK_HEADERS_DRIVER__

/**
 * Подключаем заголовочный файлы стендов
 */
#include "../common.hpp"
#include "scenarios.hpp"

/**
 * @brief Инкапсулируем драйвер стендов в пространство имён
 *
 */
namespace driver {
	/**
	 * @brief Накопитель итогов работы сценариев
	 *
	 * @details Итог каждой операции накапливается и выводится в конце прогона:
	 *          без этого оптимизатор вправе убрать измеряемую работу целиком,
	 *          и стенд мерил бы пустой цикл
	 *
	 * @return ссылка на накопитель итогов
	 *
	 */
	static inline uint64_t & checksum() noexcept {
		// Накопитель итогов работы сценариев
		static uint64_t result = 0;
		// Выводим ссылку на накопитель итогов
		return result;
	}
	/**
	 * @brief Шаблон функции прогона одного сценария
	 *
	 * @tparam Scenario тип функции прогоняемого сценария
	 *
	 */
	template <typename Scenario>
	/**
	 * @brief Функция прогона одного сценария
	 *
	 * @param name     название сценария
	 * @param units    единица измерения характеристики
	 * @param rounds   количество повторений замера
	 * @param mask     фильтр названий выполняемых сценариев
	 * @param scenario прогоняемый сценарий
	 *
	 */
	static inline void execute(const char * name, const char * units, const size_t rounds, const char * mask, Scenario scenario) noexcept {
		// Если сценарий фильтром не выбран - прогон не выполняется
		if(!rival::selected(name, mask))
			// Выходим из функции
			return;
		/**
		 * Выполняем прогрев: первые повторения выходят на установившийся режим,
		 * и замер с холодного старта мерил бы разгон реализации, а не её скорость
		 */
		for(size_t i = 0; i < scenarios::WARMUP; i++)
			// Выполняем очередное повторение прогрева
			checksum() += scenario(i);
		// Запоминаем момент начала замера
		const auto start = rival::now();
		/**
		 * Выполняем замер
		 */
		for(size_t i = 0; i < rounds; i++)
			// Выполняем очередное повторение замера
			checksum() += scenario(i);
		// Запоминаем момент окончания замера
		const auto finish = rival::now();
		// Собираем итоги прогона сценария
		rival::outcome_t outcome;
		// Записываем количество выполненных операций
		outcome.operations = rounds;
		// Записываем затраченное время
		outcome.seconds = rival::elapsed(start, finish);
		// Выводим итоги прогона сценария
		rival::report(name, units, rival::perSecond(outcome), outcome);
	}
	/**
	 * @brief Функция вывода накопленных итогов работы сценариев
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 *
	 */
	static inline void digest(const int32_t argc, char ** argv) noexcept {
		/**
		 * Выводим накопленные итоги, если стенд запрошен с признаком их вывода:
		 * накопитель существует, чтобы оптимизатор не убрал измеряемую работу,
		 * и в обычном прогоне он выводу не подлежит
		 */
		for(int32_t i = 1; i < argc; i++){
			// Если признак вывода накопленных итогов передан
			if(::strcmp(argv[i], "--digest") == 0)
				// Выводим накопленные итоги работы сценариев
				::printf("checksum: %llu\n", static_cast <unsigned long long> (checksum()));
		}
	}
};

#endif // __AWH_BENCHMARK_HEADERS_DRIVER__
