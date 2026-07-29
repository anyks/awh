/**
 * @file: duration.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения работы с продолжительностью — разбор обозначения размерности
 *        времени и формирование текстового обозначения продолжительности
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
 * @brief Внутренние параметры и сценарии бенчмарков работы с продолжительностью
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
	static constexpr double PARSE_THRESHOLD = 2650000.0;
	static constexpr double PRINT_THRESHOLD = 590000.0;
	/**
	 * @brief Пороги количества выделений памяти на одну операцию
	 *
	 * @details Разбор обозначения выделяет память единожды - на список найденных
	 *          групп совпадения, что заложено подписью разбирающей функции.
	 *          Формирование обозначения не выделяет памяти вовсе: запись вида
	 *          "1.03h" короче порога размещения строки внутри самого объекта
	 *
	 */
	static constexpr double PARSE_ALLOCATIONS = 0.01;
	static constexpr double PRINT_ALLOCATIONS = 0.01;

	/**
	 * @brief Разбираемое обозначение размерности времени
	 *
	 * @details Обозначением размерности задаются сроки жизни соединений, таймауты и
	 *          сроки хранения: каждое такое значение приходит из настроек текстом и
	 *          разбирается этим методом
	 *
	 */
	static constexpr const char * SAMPLE_DURATION = "90m";
	/**
	 * @brief Продолжительность формирования текстового обозначения в секундах
	 *
	 * @details Значение выбрано с дробной частью в выбранной размерности: целое
	 *          количество единиц выводится коротким путём без дробной части
	 *
	 */
	static constexpr double SAMPLE_SECONDS = 3725.0;

	/**
	 * @brief Функция прогона сценария разбора обозначения размерности времени
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t parsing() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель полученного количества секунд
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем разбор обозначения размерности времени с накоплением результата
			summary += static_cast <uint64_t> (chrono.seconds(SAMPLE_DURATION));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария формирования обозначения продолжительности
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t printing() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель длин сформированных обозначений продолжительности
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем формирование обозначения продолжительности с накоплением его длины
			summary += chrono.seconds(SAMPLE_SECONDS).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария разбора обозначения размерности
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsed() noexcept {
		// Итоги прогона сценария разбора обозначения размерности
		static const outcome_t result = ::parsing();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария формирования обозначения
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printed() noexcept {
		// Итоги прогона сценария формирования обозначения
		static const outcome_t result = ::printing();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии разбора обозначения размерности времени
	AWH_CHRONO_SCENARIO(Parse, ::parsed)
	// Объявляем сценарии формирования обозначения продолжительности
	AWH_CHRONO_SCENARIO(Print, ::printed)

	// Регистрируем сценарий скорости разбора обозначения размерности времени
	static const bool gParse = awh::benchmark::add(
		"chrono/duration/parse", "разборов/с", PARSE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedParse
	);
	// Регистрируем сценарий выделений памяти на разбор обозначения размерности
	static const bool gMemoryParse = awh::benchmark::add(
		"chrono/duration/parse/allocations", "выделений", PARSE_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryParse
	);
	// Регистрируем сценарий скорости формирования обозначения продолжительности
	static const bool gPrint = awh::benchmark::add(
		"chrono/duration/print", "обозначений/с", PRINT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrint
	);
	// Регистрируем сценарий выделений памяти на формирование обозначения
	static const bool gMemoryPrint = awh::benchmark::add(
		"chrono/duration/print/allocations", "выделений", PRINT_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryPrint
	);
};
