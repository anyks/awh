/**
 * @file driver.hpp
 * @date 2026-07-31
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
 * @brief Общий драйвер стендов сравнения модуля регулярных выражений
 *
 * @details Границы прогрева и замера обязаны совпадать у обеих сравниваемых
 *          реализаций: стенд, заводящий их у себя, рано или поздно разойдётся
 *          с соседним, и сравнение начнёт мерить разницу драйверов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_REGEX_DRIVER__
#define __AWH_BENCHMARK_REGEX_DRIVER__

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
	 * @details Итог каждого сопоставления накапливается и выводится в конце прогона:
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
	 * @tparam Matcher тип функции сопоставления прогоняемого сценария
	 *
	 */
	template <typename Matcher>
	/**
	 * @brief Функция прогона одного сценария
	 *
	 * @details Сборка выражения замером не охватывается: она выполняется однажды,
	 *          тогда как сопоставление - на каждом обмене, и смешение их
	 *          показало бы скорость сборки, а не скорость сопоставления
	 *
	 * @param scenario прогоняемый сценарий сравнения
	 * @param mask     фильтр названий выполняемых сценариев
	 * @param matcher  функция сопоставления прогоняемого сценария
	 *
	 */
	static inline void execute(const scenarios::scenario_t & scenario, const char * mask, Matcher matcher) noexcept {
		// Если сценарий фильтром не выбран - прогон не выполняется
		if(!rival::selected(scenario.name, mask))
			// Выходим из функции
			return;
		// Получаем количество повторений замера сценария
		const size_t rounds = scenarios::rounds(scenario.kind);
		/**
		 * Выполняем прогрев: первые повторения выходят на установившийся режим,
		 * и замер с холодного старта мерил бы разгон реализации, а не её скорость
		 */
		for(size_t i = 0; i < scenarios::WARMUP; i++)
			// Выполняем очередное повторение прогрева
			checksum() += matcher();
		// Запоминаем момент начала замера
		const auto start = rival::now();
		/**
		 * Выполняем замер
		 */
		for(size_t i = 0; i < rounds; i++)
			// Выполняем очередное повторение замера
			checksum() += matcher();
		// Запоминаем момент окончания замера
		const auto finish = rival::now();
		// Собираем итоги прогона сценария
		rival::outcome_t outcome;
		// Записываем количество выполненных операций
		outcome.operations = rounds;
		// Записываем затраченное время
		outcome.seconds = rival::elapsed(start, finish);
		// Выводим итоги прогона сценария
		rival::report(scenario.name, "match/s", rival::perSecond(outcome), outcome);
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

#endif // __AWH_BENCHMARK_REGEX_DRIVER__
