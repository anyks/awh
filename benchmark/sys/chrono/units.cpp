/**
 * @file: units.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения календарной арифметики — получение штампа времени, извлечение
 *        составляющих даты, границы суток, актуализация остатка, смещение даты и аббревиатура
 *
 * @copyright: Copyright © 2026
 *
 */

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
 * @brief Внутренние параметры и сценарии бенчмарков календарной арифметики
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
	static constexpr double TIMESTAMP_THRESHOLD = 13000000.0;
	static constexpr double UNIT_THRESHOLD = 4500000.0;
	static constexpr double BEGIN_THRESHOLD = 11000000.0;
	static constexpr double ACTUAL_THRESHOLD = 14500000.0;
	static constexpr double OFFSET_THRESHOLD = 3700000.0;
	static constexpr double ABBREVIATION_THRESHOLD = 47000000.0;
	static constexpr double SET_THRESHOLD = 900000.0;
	/**
	 * @brief Количество оборотов прогона сценария установки календарной единицы
	 *
	 * @details Установка на порядок дороже прочих единиц календарной арифметики, и
	 *          общее число оборотов уменьшено, чтобы прогон набора укладывался в
	 *          прежнее время
	 *
	 */
	static constexpr size_t SET_ROUNDS = 100000;
	/**
	 * @brief Пороги количества выделений памяти на одну операцию
	 *
	 * @details Ограничение сверху: календарная арифметика работает с полями объекта
	 *          даты и целыми числами, поэтому выделений быть не должно вовсе, и
	 *          измеренное это подтверждает - ноль в обеих сборках. В отличие от
	 *          пропускной способности показатель от машины не зависит, поэтому порог
	 *          задан вплотную к измеренному значению
	 *
	 */
	static constexpr double CALENDAR_ALLOCATIONS = 0.01;

	/**
	 * @brief Функция прогона сценария получения штампа времени
	 *
	 * @details Получение штампа времени - самая частая операция модуля: к ней
	 *          обращается журналирование на каждой записи, а сетевой движок - на
	 *          каждом обороте цикла событий. Обращение ко времени обслуживается
	 *          через страницу общего доступа ядра и системным вызовом на части
	 *          платформ не является, поэтому показатель отражает стоимость самого
	 *          обращения, а не перехода в ядро
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t stamping() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель полученных штампов времени
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CALENDAR_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем получение текущего штампа времени с накоплением результата
			summary += chrono.timestamp(awh::chrono_t::type_t::MILLISECONDS);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария извлечения составляющей даты
	 *
	 * @details Извлечение любой составляющей требует полного разложения штампа
	 *          времени на календарные поля, поэтому показатель отражает стоимость
	 *          разложения целиком, а не выборки одного поля
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t extraction() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель извлечённых составляющих даты
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CALENDAR_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем извлечение количества часов из даты с накоплением результата
			summary += chrono.get <uint8_t> ((CHRONO_DATE + index), awh::chrono_t::unit_t::HOUR);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария получения границы месяца
	 *
	 * @details Граница вычисляется всюду, где данные группируются по календарным
	 *          отрезкам: от суточных счётчиков до сроков действия. Замер идёт по
	 *          месяцу нарочно: границы суток, часа, минуты и секунды получаются
	 *          остатком от деления штампа времени, и замер на них измерял бы одну
	 *          операцию деления. Граница месяца требует определения года, начала
	 *          года, признака високосности и перебора длительностей месяцев -
	 *          то есть всей календарной части метода
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t bounding() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель полученных границ месяца
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CALENDAR_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем получение начала месяца указанной даты с накоплением результата
			summary += chrono.begin((CHRONO_DATE + index), awh::chrono_t::type_t::MONTH);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария актуализации оставшегося времени
	 *
	 * @details Актуализация вычисляет границу единицы измерения и разницу до неё в
	 *          запрошенных единицах, поэтому стоит дороже получения самой границы.
	 *          Замер идёт по остатку дней в году: год - наибольшая единица
	 *          измерения, и разница до его границы считается через ту же
	 *          календарную часть, что и сама граница
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t actualization() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель результатов актуализации
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CALENDAR_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем расчёт оставшегося количества дней в году с накоплением результата
			summary += chrono.actual(
				(CHRONO_DATE + index), awh::chrono_t::type_t::DAY,
				awh::chrono_t::type_t::YEAR, awh::chrono_t::actual_t::LEFT
			);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария смещения даты
	 *
	 * @details Смещение выполняется на месяцы нарочно: месяц - единственная единица
	 *          непостоянной длительности, и смещение на неё требует разложения даты
	 *          на календарные поля и обратной сборки. Смещение на секунды или сутки
	 *          сводится к сложению и работу метода не показывает
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t shifting() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель смещённых дат
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CALENDAR_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем смещение даты на три месяца вперёд с накоплением результата
			summary += chrono.offset(
				(CHRONO_DATE + index), 3, awh::chrono_t::type_t::MONTH,
				awh::chrono_t::offset_t::INCREMENT
			);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария получения аббревиатуры даты
	 *
	 * @details Аббревиатура подбирает наибольшую единицу измерения, в которой
	 *          продолжительность выражается значащим числом, поэтому выполняет
	 *          перебор единиц от года к миллисекундам
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t abbreviating() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель полученных аббревиатур даты
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CALENDAR_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем получение аббревиатуры даты с накоплением результата
			summary += static_cast <uint64_t> (chrono.abbreviation(CHRONO_DATE + index).second);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция прогона сценария установки календарной единицы в сводной зоне
	 *
	 * @details Установка номера месяца - самый тяжёлый из путей записи: единица
	 *          меняет саму дату, а значит требует согласования полей объекта
	 *          пересборкой его из штампа времени и разрешения смещения зоны по новой
	 *          дате. Замер идёт в сводной зоне нарочно: смещение прочих зон от
	 *          момента не зависит, и разрешение на них вырождается в согласование
	 *          выводных признаков, тогда как сводная зона проходит весь путь -
	 *          опорный штамп по стандартному времени, определение летнего времени и
	 *          сличение разрешённого смещения со стандартным
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t setting() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Устанавливаем сводную временную зону Северной Америки
		chrono.setTimeZone(awh::chrono_t::zone_t::ET);
		// Накопитель установленных номеров месяца
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(SET_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем установку номера месяца, перебирая весь их промежуток
			chrono.set <uint8_t> (static_cast <uint8_t> ((index % 12) + 1), awh::chrono_t::unit_t::MONTH);
			// Накапливаем установленный номер месяца
			summary += chrono.get <uint8_t> (awh::chrono_t::unit_t::MONTH, awh::chrono_t::storage_t::LOCAL);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария получения штампа времени
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & stamped() noexcept {
		// Итоги прогона сценария получения штампа времени
		static const outcome_t result = ::stamping();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария извлечения составляющей даты
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & extracted() noexcept {
		// Итоги прогона сценария извлечения составляющей даты
		static const outcome_t result = ::extraction();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария получения границы суток
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & bounded() noexcept {
		// Итоги прогона сценария получения границы суток
		static const outcome_t result = ::bounding();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария актуализации оставшегося времени
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & actualized() noexcept {
		// Итоги прогона сценария актуализации оставшегося времени
		static const outcome_t result = ::actualization();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария смещения даты
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & shifted() noexcept {
		// Итоги прогона сценария смещения даты
		static const outcome_t result = ::shifting();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария получения аббревиатуры даты
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & abbreviated() noexcept {
		// Итоги прогона сценария получения аббревиатуры даты
		static const outcome_t result = ::abbreviating();
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария установки календарной единицы
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & settled() noexcept {
		// Итоги прогона сценария установки календарной единицы
		static const outcome_t result = ::setting();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии получения штампа времени
	AWH_CHRONO_SCENARIO(Timestamp, ::stamped)
	// Объявляем сценарии установки календарной единицы
	AWH_CHRONO_SCENARIO(Set, ::settled)
	// Объявляем сценарии извлечения составляющей даты
	AWH_CHRONO_SCENARIO(Unit, ::extracted)
	// Объявляем сценарии получения границы суток
	AWH_CHRONO_SCENARIO(Begin, ::bounded)
	// Объявляем сценарии актуализации оставшегося времени
	AWH_CHRONO_SCENARIO(Actual, ::actualized)
	// Объявляем сценарии смещения даты
	AWH_CHRONO_SCENARIO(Offset, ::shifted)
	// Объявляем сценарии получения аббревиатуры даты
	AWH_CHRONO_SCENARIO(Abbreviation, ::abbreviated)

	// Регистрируем сценарий скорости получения штампа времени
	static const bool gTimestamp = awh::benchmark::add(
		"chrono/units/timestamp", "штампов/с", TIMESTAMP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedTimestamp
	);
	// Регистрируем сценарий выделений памяти на получение штампа времени
	static const bool gMemoryTimestamp = awh::benchmark::add(
		"chrono/units/timestamp/allocations", "выделений", CALENDAR_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryTimestamp
	);
	// Регистрируем сценарий скорости установки календарной единицы
	static const bool gSet = awh::benchmark::add(
		"chrono/units/set", "установок/с", SET_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSet
	);
	// Регистрируем сценарий выделений памяти на установку календарной единицы
	static const bool gMemorySet = awh::benchmark::add(
		"chrono/units/set/allocations", "выделений", CALENDAR_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memorySet
	);
	// Регистрируем сценарий скорости извлечения составляющей даты
	static const bool gUnit = awh::benchmark::add(
		"chrono/units/get", "извлечений/с", UNIT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedUnit
	);
	// Регистрируем сценарий выделений памяти на извлечение составляющей даты
	static const bool gMemoryUnit = awh::benchmark::add(
		"chrono/units/get/allocations", "выделений", CALENDAR_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryUnit
	);
	// Регистрируем сценарий скорости получения границы месяца
	static const bool gBegin = awh::benchmark::add(
		"chrono/units/begin", "границ/с", BEGIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedBegin
	);
	// Регистрируем сценарий выделений памяти на получение границы месяца
	static const bool gMemoryBegin = awh::benchmark::add(
		"chrono/units/begin/allocations", "выделений", CALENDAR_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryBegin
	);
	// Регистрируем сценарий скорости актуализации оставшегося времени
	static const bool gActual = awh::benchmark::add(
		"chrono/units/actual", "расчётов/с", ACTUAL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedActual
	);
	// Регистрируем сценарий выделений памяти на актуализацию оставшегося времени
	static const bool gMemoryActual = awh::benchmark::add(
		"chrono/units/actual/allocations", "выделений", CALENDAR_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryActual
	);
	// Регистрируем сценарий скорости смещения даты
	static const bool gOffset = awh::benchmark::add(
		"chrono/units/offset", "смещений/с", OFFSET_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedOffset
	);
	// Регистрируем сценарий выделений памяти на смещение даты
	static const bool gMemoryOffset = awh::benchmark::add(
		"chrono/units/offset/allocations", "выделений", CALENDAR_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryOffset
	);
	// Регистрируем сценарий скорости получения аббревиатуры даты
	static const bool gAbbreviation = awh::benchmark::add(
		"chrono/units/abbreviation", "аббревиатур/с", ABBREVIATION_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedAbbreviation
	);
	// Регистрируем сценарий выделений памяти на получение аббревиатуры даты
	static const bool gMemoryAbbreviation = awh::benchmark::add(
		"chrono/units/abbreviation/allocations", "выделений", CALENDAR_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryAbbreviation
	);
};
