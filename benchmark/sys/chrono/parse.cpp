/**
 * @file: parse.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения разбора записи даты — форматы ISO 8601, журнала веб-сервера
 *        и сплошной записи без разделителей
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
 * @brief Внутренние параметры и сценарии бенчмарков разбора записи даты
 *
 */
namespace {
	/**
	 * @brief Пороги количества разборов в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double PARSE_ISO_THRESHOLD = 340000.0;
	static constexpr double PARSE_CLF_THRESHOLD = 350000.0;
	static constexpr double PARSE_COMPACT_THRESHOLD = 440000.0;
	/**
	 * @brief Пороги количества выделений памяти на один разбор
	 *
	 * @details Показатель от машины не зависит, поэтому пороги заданы вплотную к
	 *          измеренным значениям отладочной сборки. Оптимизированная сборка
	 *          устраняет часть промежуточных объектов и выделяет не больше, поэтому
	 *          пороги, снятые с отладочной, годятся обеим. Они и есть главный
	 *          показатель набора: разбор записи даты в поля объекта известного
	 *          размера выделять память не обязан вовсе, и всякое выделение здесь -
	 *          работа, которой можно не делать.
	 *
	 *          Измеренное - от восьми до десяти выделений на разбор одной записи,
	 *          то есть по выделению на каждую переменную формата: разбор каждой из
	 *          них заводит список найденных групп совпадения, а он размещается в
	 *          динамической памяти. Пороги эти поэтому не столько стражи от
	 *          регрессии, сколько список целей: каждый из них надлежит понизить
	 *          правкой модуля, а не правкой порога. Разбор поля даты - это чтение
	 *          нескольких цифр из уже готовой строки, и списка групп ему не нужно
	 *
	 */
	static constexpr double PARSE_ISO_ALLOCATIONS = 0.01;
	static constexpr double PARSE_CLF_ALLOCATIONS = 0.01;
	static constexpr double PARSE_COMPACT_ALLOCATIONS = 0.01;

	/**
	 * @brief Разбираемая запись даты формата ISO 8601
	 *
	 * @details Самый распространённый формат обмена датами: им размечены заголовки
	 *          протоколов, поля документов JSON и штампы времени журналов
	 *
	 */
	static constexpr const char * SAMPLE_ISO = "2024-08-06T11:08:55.101Z";
	/**
	 * @brief Формат разбора записи даты ISO 8601
	 *
	 */
	static constexpr const char * FORMAT_ISO = "%Y-%m-%dT%H:%M:%S.%s%Z";
	/**
	 * @brief Разбираемая запись даты формата журнала веб-сервера
	 *
	 * @details Формат журнала нагружает разбор иначе: месяц записан названием и
	 *          определяется поиском по таблице, а смещение зоны - числом
	 *
	 */
	static constexpr const char * SAMPLE_CLF = "[18/Jul/2024:13:34:00 +0300]";
	/**
	 * @brief Формат разбора записи даты журнала веб-сервера
	 *
	 */
	static constexpr const char * FORMAT_CLF = "%d/%h/%Y:%H:%M:%S %z";
	/**
	 * @brief Разбираемая сплошная запись даты без разделителей
	 *
	 * @details Разделителей между полями нет вовсе, поэтому границы полей задаёт
	 *          один только формат. Сценарий ловит регрессию, при которой разбор
	 *          начинает опираться на разделители вместо разрядности поля
	 *
	 */
	static constexpr const char * SAMPLE_COMPACT = "20050809T183142+0330";
	/**
	 * @brief Формат разбора сплошной записи даты
	 *
	 */
	static constexpr const char * FORMAT_COMPACT = "%Y%m%dT%H%M%S%z";

	/**
	 * @brief Функция прогона сценария разбора записи даты
	 *
	 * @param sample разбираемая запись даты
	 * @param format формат разбора записи даты
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t parsing(const char * sample, const char * format) noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель результатов разбора записи даты
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем разбор записи даты с накоплением результата
			summary += chrono.parse(sample, format);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора записи ISO 8601
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedISO() noexcept {
		// Итоги прогона сценария разбора записи ISO 8601
		static const outcome_t result = ::parsing(SAMPLE_ISO, FORMAT_ISO);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора записи журнала
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedCLF() noexcept {
		// Итоги прогона сценария разбора записи журнала
		static const outcome_t result = ::parsing(SAMPLE_CLF, FORMAT_CLF);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария разбора сплошной записи
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & parsedCompact() noexcept {
		// Итоги прогона сценария разбора сплошной записи
		static const outcome_t result = ::parsing(SAMPLE_COMPACT, FORMAT_COMPACT);
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии разбора записи ISO 8601
	AWH_CHRONO_SCENARIO(ISO, ::parsedISO)
	// Объявляем сценарии разбора записи журнала веб-сервера
	AWH_CHRONO_SCENARIO(CLF, ::parsedCLF)
	// Объявляем сценарии разбора сплошной записи
	AWH_CHRONO_SCENARIO(Compact, ::parsedCompact)

	// Регистрируем сценарий скорости разбора записи ISO 8601
	static const bool gParseISO = awh::benchmark::add(
		"chrono/parse/iso8601", "разборов/с", PARSE_ISO_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedISO
	);
	// Регистрируем сценарий выделений памяти на разбор записи ISO 8601
	static const bool gMemoryParseISO = awh::benchmark::add(
		"chrono/parse/iso8601/allocations", "выделений", PARSE_ISO_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryISO
	);
	// Регистрируем сценарий скорости разбора записи журнала веб-сервера
	static const bool gParseCLF = awh::benchmark::add(
		"chrono/parse/clf", "разборов/с", PARSE_CLF_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedCLF
	);
	// Регистрируем сценарий выделений памяти на разбор записи журнала веб-сервера
	static const bool gMemoryParseCLF = awh::benchmark::add(
		"chrono/parse/clf/allocations", "выделений", PARSE_CLF_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryCLF
	);
	// Регистрируем сценарий скорости разбора сплошной записи
	static const bool gParseCompact = awh::benchmark::add(
		"chrono/parse/compact", "разборов/с", PARSE_COMPACT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedCompact
	);
	// Регистрируем сценарий выделений памяти на разбор сплошной записи
	static const bool gMemoryParseCompact = awh::benchmark::add(
		"chrono/parse/compact/allocations", "выделений", PARSE_COMPACT_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryCompact
	);
};
