/**
 * @file probe.hpp
 * @date 2026-08-29
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
 * \~russian
 * @brief Заголовочный файл учёта путей исполнения сопоставления — счётчики,
 *        показывающие, каким путём прошло сопоставление, и позволяющие проверкам
 *        удостоверять применение ускорителей, вердикта не меняющих
 *
 * \~english
 * @brief Header file of the accounting of the execution paths of matching — the counters
 *        showing which path the matching took and allowing the tests to certify the use
 *        of the accelerators that do not change the verdict
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_REGEX_PROBE__
#define __AWH_REGEX_PROBE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён модуля регулярных выражений
	 *
	 * \~english
	 * @brief Namespace of the regular expression module
	 *
	 * \~
	 */
	namespace regex {
		/**
		 * \~russian
		 * @brief Путь исполнения сопоставления, учёту подлежащий
		 *
		 * @details Учитываются пути, вердикта не меняющие: всякий из них лишь
		 *          ускоряет поиск, а отключение его оставляет итог верным.
		 *          Сличение вердиктов такого отключения не замечает вовсе,
		 *          и проба гашения показала, что семь таких путей гасились,
		 *          не сорвав ни одной проверки набора, - порождённый машинный
		 *          код в их числе.
		 *
		 * \~english
		 * @brief Execution path of matching subject to accounting
		 * @details The paths accounted for are those that do not change the verdict: each of
		 *          them only speeds up the search, while turning it off leaves the outcome
		 *          correct. Comparing verdicts does not notice such a switch-off at all,
		 *          and a probe of switching them off showed that seven such paths could be
		 *          switched off without failing a single test of the suite, the generated
		 *          machine code among them.
		 *
		 * \~
		 */
		enum class path_t : uint8_t {
			MACHINE   = 0x00, // Сопоставление порождённым машинным кодом
			PLAIN     = 0x01, // Поиск последовательности выражения, литералом сопоставляемого
			SEEKING   = 0x02, // Отбор позиций у детерминированного исполнения
			CACHING   = 0x03, // Обращение к кэшу состояний детерминированного исполнения
			PIKING    = 0x04, // Отбор позиций у исполнения без возврата
			TRACKING  = 0x05, // Отбор позиций у исполнения с возвратом
			BOUNDING  = 0x06, // Отодвигание начала поиска у исполнения с возвратом
			PRESUMING = 0x07, // Отказ по проверке возможности у исполнения без возврата
			DENYING   = 0x08, // Отказ по проверке возможности у исполнения с возвратом
			VERIFYING = 0x09, // Отказ по пробе детерминированным исполнением
			SWEEPING  = 0x0A, // Проход текста единственной попыткой
			HALTING   = 0x0B, // Остановка автомата привязкой к позиции начала поиска
			REUSING   = 0x0C, // Переиспользование итога автомата, вызывающей стороной снятого
			SUBSETTING = 0x0D, // Отказ построения по разбору дерева до самого построения
			COUNT     = 0x0E  // Количество учитываемых путей исполнения
		};
		/**
		 * \~russian
		 * @brief Класс учёта путей исполнения сопоставления
		 *
		 * @details Учёт ведётся лишь у библиотеки, признаком сборки
		 *          «AWH_REGEX_PROBING» собранной: приращение счётчика
		 *          обходится в десятки наносекунд у сопоставления, какое
		 *          само укладывается в двадцать, и месту его в выпускаемой
		 *          сборке нет. Признак этот ставится набору проверок всегда,
		 *          а метод «enabled» позволяет проверке отказать, если учёт
		 *          не заведён: молчаливый пропуск проверки равен молчаливому
		 *          отключению того, что она стережёт.
		 *
		 * \~english
		 * @brief Class of the accounting of the execution paths of matching
		 * @details The accounting is kept only by a library built with the build flag
		 *          "AWH_REGEX_PROBING": incrementing a counter costs tens of nanoseconds
		 *          for a match that itself fits into twenty, and it has no place in a
		 *          release build. The flag is always set for the test suite, while the
		 *          "enabled" method allows a test to fail if the accounting is not
		 *          compiled in: silently skipping a test equals silently switching off
		 *          what it guards.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Probe {
			public:
				/**
				 * \~russian
				 * @brief Метод проверки заведения учёта путей исполнения
				 *
				 * @return признак заведения учёта путей исполнения
				 *
				 * \~english
				 * @brief Method of checking whether the accounting of the execution paths is compiled in
				 * @return indication that the accounting of the execution paths is compiled in
				 *
				 * \~
				 */
				static bool enabled() noexcept;
				/**
				 * \~russian
				 * @brief Метод сброса счётчиков путей исполнения
				 *
				 * \~english
				 * @brief Method of resetting the counters of the execution paths
				 *
				 * \~
				 */
				static void reset() noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения счётчика пути исполнения
				 *
				 * @param path учитываемый путь исполнения сопоставления
				 * @return     количество прохождений пути исполнения
				 *
				 * \~english
				 * @brief Method of getting the counter of an execution path
				 * @param path execution path of matching being accounted for
				 * @return     number of passes of the execution path
				 *
				 * \~
				 */
				static uint64_t count(const path_t path) noexcept;
				/**
				 * \~russian
				 * @brief Метод учёта прохождения пути исполнения
				 *
				 * @param path пройденный путь исполнения сопоставления
				 *
				 * \~english
				 * @brief Method of accounting for a pass of an execution path
				 * @param path execution path of matching that was passed
				 *
				 * \~
				 */
				static void tick(const path_t path) noexcept;
		} probe_t;
	};
};

/**
 * Если учёт путей исполнения сопоставления заведён
 */
#if defined(AWH_REGEX_PROBING)
	/**
	 * Учёт прохождения пути исполнения сопоставления
	 */
	#define AWH_REGEX_TICK(PATH) awh::regex::Probe::tick(PATH)
/**
 * Если учёт путей исполнения сопоставления не заведён
 */
#else
	/**
	 * Учёт не ведётся: приращение счётчика расхода стоит дороже пути учитываемого
	 */
	#define AWH_REGEX_TICK(PATH) ((void) 0)
#endif

#endif // __AWH_REGEX_PROBE__
