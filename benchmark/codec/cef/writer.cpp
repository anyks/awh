/**
 * @file writer.cpp
 * @date 2026-09-04
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
 * @brief Бенчмарки записи событий в запись CEF — пропускной способности сборки записи из
 *        дерева контейнера ABC и расхода выделений памяти на сборку
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы бенчмарков
 */
#include "cef.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>

/**
 * @brief Пространство имён сценариев этого файла
 *
 * @note Держится оно безымянным намеренно: сценарии кодеков собираются одной программою
 *
 */
namespace {
	/**
	 * @brief Объект окружения сценариев с отключённым выводом журнала
	 *
	 */
	struct SilentCefWriter {
		// Объект фреймворка сценариев
		awh::fmk_t fmk;
		// Объект журнала сценариев
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		SilentCefWriter() noexcept : log(&this->fmk) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта окружения сценариев
	 *
	 * @return объект окружения сценариев
	 *
	 */
	SilentCefWriter & writerEnvironment() noexcept {
		// Объект окружения сценариев
		static SilentCefWriter env;
		// Выводим объект окружения сценариев
		return env;
	}

	/**
	 * @brief Порог пропускной способности сборки записи IDS из дерева
	 *
	 * @details Порог взят по дну сценария с запасом: показатель этот от машины ЗАВИСИТ
	 *
	 */
	constexpr double WRITE_DETECTION_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности сборки записи auditd из дерева
	 *
	 */
	constexpr double WRITE_AUDIT_THRESHOLD = 10.0;

	/**
	 * @brief Порог расхода выделений памяти на сборку одной записи
	 *
	 */
	constexpr double WRITE_ALLOCATIONS_THRESHOLD = 200.0;

	/**
	 * @brief Функция замера сборки записи из дерева контейнера ABC
	 *
	 * @param text      эталонная запись, деревом укладываемая
	 * @param rounds    количество собираемых записей
	 * @param threshold заполняемый результат измерения
	 * @return          итоги прогона сценария
	 *
	 */
	awh::benchmark::event::outcome_t assemble(const std::string & text, const size_t rounds) noexcept {
		// Объект события CEF
		awh::codec::cef::document_t doc(&::writerEnvironment().fmk, &::writerEnvironment().log);
		// Выполняем разбор эталонной записи в дерево события
		if(!doc.parse(text))
			// Выводим пустые итоги прогона сценария
			return awh::benchmark::event::outcome_t();
		// Объект записи событий
		awh::codec::cef::writer_t writer(&::writerEnvironment().fmk, &::writerEnvironment().log);
		// Собираемая запись CEF
		std::string result;
		// Выводим итоги замера сборки записи из дерева события
		return awh::benchmark::event::measure(text.size(), rounds, [&writer, &doc, &result]() noexcept -> size_t {
			// Если сборка записи отказом завершилась
			if(!writer.write(doc.root(), result))
				// Выводим отсутствие собранной записи
				return 0;
			// Выводим длину собранной записи
			return result.size();
		});
	}

	/**
	 * @brief Функция замера сборки записи обнаружения вторжений
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeDetection() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем замер сборки эталонной записи
		const auto output = ::assemble(awh::benchmark::event::detection(), 20000);
		// Если операций прогоном не выполнено
		if(output.operations == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "разбор эталонной записи отказом завершился";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную пропускную способность сборки
		result.value = awh::benchmark::event::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера сборки записи надзора за системой
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeAudit() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем замер сборки эталонной записи
		const auto output = ::assemble(awh::benchmark::event::audit(), 5000);
		// Если операций прогоном не выполнено
		if(output.operations == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "разбор эталонной записи отказом завершился";
			// Выводим результат измерения
			return result;
		}
		// Устанавливаем измеренную пропускную способность сборки
		result.value = awh::benchmark::event::perSecond(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * @brief Функция замера расхода выделений памяти на сборку записи
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем замер сборки эталонной записи
		const auto output = ::assemble(awh::benchmark::event::detection(), 2000);
		// Если учёт выделений памяти неработоспособен
		if(!awh::benchmark::event::counted(output, result))
			// Выводим результат измерения
			return result;
		// Устанавливаем измеренный расход выделений памяти на запись
		result.value = awh::benchmark::event::perRecord(output);
		// Устанавливаем сведения о прогоне сценария
		result.details = awh::benchmark::event::details(output);
		// Выводим результат измерения
		return result;
	}

	/**
	 * Выполняем регистрацию сценария сборки записи IDS из дерева
	 */
	static const bool WRITE_DETECTION_REGISTERED = awh::benchmark::add(
		"codec/cef: сборка записи IDS", "МБ/с", WRITE_DETECTION_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeDetection
	);
	/**
	 * Выполняем регистрацию сценария сборки записи auditd из дерева
	 */
	static const bool WRITE_AUDIT_REGISTERED = awh::benchmark::add(
		"codec/cef: сборка записи auditd", "МБ/с", WRITE_AUDIT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeAudit
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку
	 */
	static const bool WRITE_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/cef: выделения на сборку", "выд./запись", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
};
