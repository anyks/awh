/**
 * @file: zones.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения работы с временными зонами — сопоставление обозначения
 *        зоны с известной и перевод обозначения со смещением в секунды
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdlib>

/**
 * Подключаем заголовочный файл бенчмарков модуля работы с датой и временем
 */
#include "chrono.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля работы с датой и временем
 */
using namespace awh::benchmark::chrono;

/**
 * @brief Внутренние параметры и сценарии бенчмарков работы с временными зонами
 *
 */
namespace {
	/**
	 * @brief Пороги количества операций в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double MATCH_THRESHOLD = 950000.0;
	static constexpr double SHIFT_THRESHOLD = 320000.0;
	/**
	 * @brief Пороги количества выделений памяти на одну операцию
	 *
	 * @details Реестр временных зон - это ассоциативный массив, построенный при
	 *          создании объекта, поэтому поиск в нём выделять память не обязан:
	 *          обозначение приходит представлением строки и копироваться под ключ
	 *          поиска не должно. Измеренное - два выделения на сопоставление и
	 *          четыре на перевод со смещением, то есть копия обозначения делается,
	 *          и не по одному разу. Пороги заданы вплотную к измеренному отладочной
	 *          сборкой: оптимизированная устраняет по одному из них и выделяет не
	 *          больше. Понижать их следует правкой модуля, а не правкой порога
	 *
	 */
	static constexpr double MATCH_ALLOCATIONS = 2.0;
	static constexpr double SHIFT_ALLOCATIONS = 3.0;

	/**
	 * @brief Сопоставляемое обозначение временной зоны
	 *
	 * @details Обозначение выбрано из середины реестра: поиск идёт по
	 *          ассоциативному массиву, но перед ним обозначение проверяется на
	 *          известные сокращения, и обозначение из начала проверки измеряло бы
	 *          длину пути до него, а не работу метода
	 *
	 */
	static constexpr const char * SAMPLE_ZONE = "YEKT";
	/**
	 * @brief Переводимое обозначение временной зоны со смещением
	 *
	 * @details Обозначение содержит и название зоны, и смещение от неё, причём
	 *          смещение некратно часу: перевод требует разбора обеих составляющих
	 *
	 */
	static constexpr const char * SAMPLE_SHIFT = "GMT-0328";

	/**
	 * @brief Функция прогона сценария сопоставления обозначения временной зоны
	 *
	 * @details Сопоставление выполняется на каждой разобранной записи даты,
	 *          размеченной названием зоны, поэтому его стоимость входит в стоимость
	 *          разбора такой записи
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t matching() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель сопоставленных временных зон
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем сопоставление обозначения временной зоны с накоплением результата
			summary += static_cast <uint64_t> (chrono.matchTimeZone(SAMPLE_ZONE));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария перевода обозначения зоны в смещение
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t shifting() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель полученных смещений временной зоны
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем перевод обозначения временной зоны в смещение с накоплением результата
			summary += static_cast <uint64_t> (::llabs(chrono.getTimeZone(SAMPLE_SHIFT)));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария сопоставления обозначения зоны
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & matched() noexcept {
		// Итоги прогона сценария сопоставления обозначения зоны
		static const outcome_t result = ::matching();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария перевода обозначения в смещение
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & shifted() noexcept {
		// Итоги прогона сценария перевода обозначения в смещение
		static const outcome_t result = ::shifting();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии сопоставления обозначения временной зоны
	AWH_CHRONO_SCENARIO(Match, ::matched)
	// Объявляем сценарии перевода обозначения временной зоны в смещение
	AWH_CHRONO_SCENARIO(Shift, ::shifted)

	// Регистрируем сценарий скорости сопоставления обозначения временной зоны
	static const bool gMatch = awh::benchmark::add(
		"chrono/zones/match", "сопоставлений/с", MATCH_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedMatch
	);
	// Регистрируем сценарий выделений памяти на сопоставление обозначения зоны
	static const bool gMemoryMatch = awh::benchmark::add(
		"chrono/zones/match/allocations", "выделений", MATCH_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryMatch
	);
	// Регистрируем сценарий скорости перевода обозначения зоны в смещение
	static const bool gShift = awh::benchmark::add(
		"chrono/zones/offset", "переводов/с", SHIFT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedShift
	);
	// Регистрируем сценарий выделений памяти на перевод обозначения зоны в смещение
	static const bool gMemoryShift = awh::benchmark::add(
		"chrono/zones/offset/allocations", "выделений", SHIFT_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryShift
	);
};
