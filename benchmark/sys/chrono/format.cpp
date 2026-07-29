/**
 * @file: format.cpp
 * @date: 2026-07-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения формирования записи даты — форматы ISO 8601 и журнала
 *        веб-сервера, перевод записи из формата в формат и обозначение временной зоны
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
 * @brief Внутренние параметры и сценарии бенчмарков формирования записи даты
 *
 */
namespace {
	/**
	 * @brief Пороги количества формирований записи в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double FORMAT_ISO_THRESHOLD = 750000.0;
	static constexpr double FORMAT_CLF_THRESHOLD = 800000.0;
	static constexpr double FORMAT_STRIP_THRESHOLD = 120000.0;
	static constexpr double FORMAT_ZONE_THRESHOLD = 10000000.0;
	/**
	 * @brief Пороги количества выделений памяти на одно формирование записи
	 *
	 * @details Показатель от машины не зависит, поэтому пороги заданы вплотную к
	 *          измеренным значениям отладочной сборки. Оптимизированная сборка
	 *          устраняет часть промежуточных объектов и выделяет не больше, поэтому
	 *          пороги, снятые с отладочной, годятся обеим. Одно выделение здесь
	 *          неустранимо по подписи метода: он отдаёт наружу объект строки, а
	 *          запись даты длиннее порога размещения строки внутри самого объекта.
	 *          Всё, что сверх этого одного, - промежуточные строки пути
	 *          формирования, и они устранимы правкой модуля.
	 *
	 *          Формирование записи в этот один и укладывается, а перевод записи из
	 *          формата в формат - в десять: девять из них приходятся на разбор
	 *          исходной записи, и понижать их следует там же, где и пороги разбора.
	 *          Обозначение временной зоны не выделяет памяти вовсе, но не по
	 *          устройству, а по случайности: запись "UTC-3:28" короче порога
	 *          размещения строки внутри объекта
	 *
	 */
	static constexpr double FORMAT_ISO_ALLOCATIONS = 1.0;
	static constexpr double FORMAT_CLF_ALLOCATIONS = 1.0;
	static constexpr double FORMAT_STRIP_ALLOCATIONS = 10.0;
	static constexpr double FORMAT_ZONE_ALLOCATIONS = 0.5;

	/**
	 * @brief Формат формирования записи даты ISO 8601
	 *
	 * @details Запись содержит все составляющие даты, включая миллисекунды и
	 *          смещение временной зоны: сокращённый формат измерял бы длину
	 *          обрабатываемой строки, а не работу метода
	 *
	 */
	static constexpr const char * FORMAT_ISO = "%Y-%m-%dT%H:%M:%S.%s%o";
	/**
	 * @brief Формат формирования записи даты журнала веб-сервера
	 *
	 * @details Месяц выводится названием, а смещение зоны - числом: работа с
	 *          таблицей названий в общем показателе присутствует
	 *
	 */
	static constexpr const char * FORMAT_CLF = "%d/%h/%Y:%H:%M:%S %z";
	/**
	 * @brief Исходная запись даты сценария перевода записи из формата в формат
	 *
	 */
	static constexpr const char * SAMPLE_STRIP = "Sun Apr 6 2025 15:37:01 +0300";
	/**
	 * @brief Исходный формат сценария перевода записи из формата в формат
	 *
	 */
	static constexpr const char * FORMAT_STRIP_FROM = "%a %h %e %Y %H:%M:%S %z";
	/**
	 * @brief Конечный формат сценария перевода записи из формата в формат
	 *
	 */
	static constexpr const char * FORMAT_STRIP_TO = "%d/%h/%Y:%H:%M:%S %z";
	/**
	 * @brief Смещение временной зоны сценария обозначения временной зоны в секундах
	 *
	 * @details Смещение выбрано некратным часу нарочно: кратное часу обозначение
	 *          формируется коротким путём без вывода минут
	 *
	 */
	static constexpr int32_t SAMPLE_ZONE = -12480;

	/**
	 * @brief Функция прогона сценария формирования записи даты
	 *
	 * @param format формат формирования записи даты
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t formatting(const char * format) noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель длин сформированных записей даты
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&](const size_t index) noexcept {
			// Выполняем формирование записи даты с накоплением её длины
			summary += chrono.format((CHRONO_DATE + index), format).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария перевода записи даты из формата в формат
	 *
	 * @details Перевод выполняет разбор исходной записи и формирование конечной за
	 *          один вызов, поэтому его стоимость складывается из стоимости обеих
	 *          операций. Показатель ловит появление лишней работы на стыке: если
	 *          перевод станет дороже суммы разбора и формирования, между ними
	 *          завелось промежуточное представление
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t stripping() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель длин переведённых записей даты
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем перевод записи даты из формата в формат с накоплением её длины
			summary += chrono.strip(SAMPLE_STRIP, FORMAT_STRIP_FROM, FORMAT_STRIP_TO).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария обозначения временной зоны
	 *
	 * @details Обозначение временной зоны формируется на каждой записи журнала,
	 *          размеченной временной зоной, и работы в нём немного: перевод
	 *          смещения в часы и минуты с выводом знака
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t zoning() noexcept {
		// Объект работы с датой и временем
		awh::chrono_t chrono(framework(), logger());
		// Накопитель длин сформированных обозначений временной зоны
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHRONO_ROUNDS, [&]([[maybe_unused]] const size_t index) noexcept {
			// Выполняем формирование обозначения временной зоны с накоплением его длины
			summary += chrono.format(SAMPLE_ZONE).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария формирования записи ISO 8601
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & formattedISO() noexcept {
		// Итоги прогона сценария формирования записи ISO 8601
		static const outcome_t result = ::formatting(FORMAT_ISO);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария формирования записи журнала
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & formattedCLF() noexcept {
		// Итоги прогона сценария формирования записи журнала
		static const outcome_t result = ::formatting(FORMAT_CLF);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария перевода записи даты
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & stripped() noexcept {
		// Итоги прогона сценария перевода записи даты
		static const outcome_t result = ::stripping();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария обозначения временной зоны
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & zoned() noexcept {
		// Итоги прогона сценария обозначения временной зоны
		static const outcome_t result = ::zoning();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии формирования записи ISO 8601
	AWH_CHRONO_SCENARIO(ISO, ::formattedISO)
	// Объявляем сценарии формирования записи журнала веб-сервера
	AWH_CHRONO_SCENARIO(CLF, ::formattedCLF)
	// Объявляем сценарии перевода записи даты из формата в формат
	AWH_CHRONO_SCENARIO(Strip, ::stripped)
	// Объявляем сценарии обозначения временной зоны
	AWH_CHRONO_SCENARIO(Zone, ::zoned)

	// Регистрируем сценарий скорости формирования записи ISO 8601
	static const bool gFormatISO = awh::benchmark::add(
		"chrono/format/iso8601", "записей/с", FORMAT_ISO_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedISO
	);
	// Регистрируем сценарий выделений памяти на формирование записи ISO 8601
	static const bool gMemoryFormatISO = awh::benchmark::add(
		"chrono/format/iso8601/allocations", "выделений", FORMAT_ISO_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryISO
	);
	// Регистрируем сценарий скорости формирования записи журнала веб-сервера
	static const bool gFormatCLF = awh::benchmark::add(
		"chrono/format/clf", "записей/с", FORMAT_CLF_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedCLF
	);
	// Регистрируем сценарий выделений памяти на формирование записи журнала
	static const bool gMemoryFormatCLF = awh::benchmark::add(
		"chrono/format/clf/allocations", "выделений", FORMAT_CLF_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryCLF
	);
	// Регистрируем сценарий скорости перевода записи даты из формата в формат
	static const bool gFormatStrip = awh::benchmark::add(
		"chrono/format/strip", "переводов/с", FORMAT_STRIP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedStrip
	);
	// Регистрируем сценарий выделений памяти на перевод записи даты
	static const bool gMemoryFormatStrip = awh::benchmark::add(
		"chrono/format/strip/allocations", "выделений", FORMAT_STRIP_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryStrip
	);
	// Регистрируем сценарий скорости формирования обозначения временной зоны
	static const bool gFormatZone = awh::benchmark::add(
		"chrono/format/zone", "обозначений/с", FORMAT_ZONE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedZone
	);
	// Регистрируем сценарий выделений памяти на обозначение временной зоны
	static const bool gMemoryFormatZone = awh::benchmark::add(
		"chrono/format/zone/allocations", "выделений", FORMAT_ZONE_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryZone
	);
};
