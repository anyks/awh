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
	 * @details Мера: длина эталонной записи, помноженная на число кругов, делённая на
	 *          время прогона. Замерено 04.09.2026 на macOS ARM64 в Release: 150-152
	 *          МБ/с. Порог выставлен ОЖИДАНИЕМ на порядок ниже, а не дном по стендам:
	 *          показатель этот от машины ЗАВИСИТ, а прочие машины кодеком CEF ещё не
	 *          проходились
	 *
	 */
	constexpr double WRITE_DETECTION_THRESHOLD = 10.0;

	/**
	 * @brief Порог пропускной способности сборки записи заслона сети из дерева
	 *
	 * @details Мера: длина эталонной записи, помноженная на число кругов, делённая на
	 *          время прогона. Замерено 04.09.2026 на macOS ARM64 в Release: 159 МБ/с.
	 *          Порог выставлен ОЖИДАНИЕМ на порядок ниже, а не дном по стендам
	 *
	 */
	constexpr double WRITE_FIREWALL_THRESHOLD = 10.0;

	/**
	 * @brief Порог расхода выделений памяти на сборку одной записи
	 *
	 * @details Мера: число выделений памяти за прогон, делённое на число собранных
	 *          записей. Замерено 04.09.2026 на macOS ARM64 в Release: 3 выделения на
	 *          запись. Запас держится широким, покуда стенд на libstdc++ не пройден:
	 *          запас короткой строки там иной, и число может вырасти кратно, поломкой
	 *          не будучи
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
		/**
		 * Признак того, что сборка записи отказом хотя бы раз завершилась
		 *
		 * @note Сторож этот обязателен: числитель меры берётся длиной ЭТАЛОННОЙ записи,
		 *       помноженной на число кругов, а не длиной собранного. Откажи писатель -
		 *       и сценарий отчитается о работе, какой не было, тем БЫСТРЕЕ, чем раньше
		 *       отказ наступил. Замерено 04.09.2026: сборка записи auditd отдавала
		 *       3248 МБ/с, собирая 45 октетов вместо 387 и отказывая всякий круг
		 *
		 */
		bool refused = false;
		// Выполняем замер сборки записи из дерева события
		const awh::benchmark::event::outcome_t output = awh::benchmark::event::measure(
			text.size(), rounds, [&writer, &doc, &result, &refused]() noexcept -> size_t {
				// Если сборка записи отказом завершилась
				if(!writer.write(doc.root(), result)){
					// Запоминаем отказ сборки записи
					refused = true;
					// Выводим отсутствие собранной записи
					return 0;
				}
				// Выводим длину собранной записи
				return result.size();
			}
		);
		// Если сборка записи отказом завершилась
		if(refused)
			// Выводим пустые итоги прогона сценария
			return awh::benchmark::event::outcome_t();
		// Выводим итоги прогона сценария
		return output;
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
			result.reason = "разбор либо сборка эталонной записи отказом завершились";
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
	 * @brief Функция замера сборки записи заслона сети
	 *
	 * @details Записью надзора за системой сценарий этот мериться НЕ может: эталон
	 *          auditd взят из живого журнала и несёт в заголовке одиннадцать полей
	 *          вместо семи, оттого остаток заголовка уезжает в расширение первым
	 *          ключом - с чертами и пробелами внутри, - а такой ключ записи CEF
	 *          не поддаётся, и писатель отвечает отказом законно. Для чтения эталон
	 *          этот годен и ценен, для сборки - негоден вовсе
	 *
	 * @return результат измерения
	 *
	 */
	awh::benchmark::result_t writeFirewall() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем замер сборки эталонной записи
		const auto output = ::assemble(awh::benchmark::event::firewall(), 5000);
		// Если операций прогоном не выполнено
		if(output.operations == 0){
			// Помечаем измерение недействительным
			result.invalid = true;
			// Устанавливаем причину недействительности измерения
			result.reason = "разбор либо сборка эталонной записи отказом завершились";
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
	 * Выполняем регистрацию сценария сборки записи заслона сети из дерева
	 */
	static const bool WRITE_FIREWALL_REGISTERED = awh::benchmark::add(
		"codec/cef: сборка записи заслона", "МБ/с", WRITE_FIREWALL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, writeFirewall
	);
	/**
	 * Выполняем регистрацию сценария расхода выделений памяти на сборку
	 */
	static const bool WRITE_ALLOCATIONS_REGISTERED = awh::benchmark::add(
		"codec/cef: выделения на сборку", "выд./запись", WRITE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, writeAllocations
	);
};
