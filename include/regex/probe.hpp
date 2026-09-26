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

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/macro/global.hpp"

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
			/**
			 * Сопоставление порождённым машинным кодом
			 *
			 * @warning Путь этот НЕЛЬЗЯ звать «MACHINE»: FreeBSD объявляет это имя
			 *          макросом в `<machine/param.h>`, подставляя вместо него строку с
			 *          названием обработчика. Область видимости перечисления от такого
			 *          не защищает - препроцессор про неё не знает и правит текст до
			 *          разбора, - и сборка на FreeBSD обрывалась отказом «expected
			 *          identifier». Тот же капкан однажды уже сработал у режима сборки
			 *          выражения, о чём сказано у `flag_t::JIT`, и повторился здесь.
			 *          Заводя новые пути, имя следует сличать с системными макросами
			 */
			JITTED    = 0x00,
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
			TABULATING = 0x0E, // Построение таблицы принадлежности байтов классу у исполнения с возвратом
			PROBING   = 0x0F, // Проба исполнением с возвратом, проход автомата снявшая
			LINING    = 0x10, // Пропуск позиций до начала строки у исполнения с возвратом
			SOLIDING  = 0x11, // Пропуск точек возврата ряда повторения у исполнения с возвратом
			BARRING   = 0x12, // Отказ по набору начальных байтов у выражения, к позиции поиска привязанного
			SLIDING   = 0x13, // Продвижение ленивого ряда к ближайшему байту, продолжению пригодному
			CHAINING  = 0x14, // Проход цепочки ограниченного повторения у исполнения с возвратом
			YIELDING  = 0x15, // Проба исполнением с возвратом, пределом оборванная и автомату уступившая
			RECALLING = 0x16, // Размещение класса символов номером, построением запомненным, без отсева повторов
			WAIVING   = 0x17, // Отказ построения от обхода дерева в поисках глагола, вида которого разбор не заводил
			COUNT     = 0x18  // Количество учитываемых путей исполнения
		};
		/**
		 * \~russian
		 * @brief Мера работы сопоставления, учёту подлежащая
		 *
		 * @details Мера работы от пути исполнения отличается родом, а не величиной,
		 *          и оттого заведена отдельным перечислением. Путь - это ускоритель,
		 *          вердикта не меняющий: погасить его можно, и итог останется верен.
		 *          Мера работы гашению не подлежит вовсе - шаг цикла либо запись
		 *          ячейки захвата есть само сопоставление, а не ускорение его.
		 *
		 *          Заведена мера ради довода о причине. Время правки в горячей
		 *          единице трансляции недоказательно: всякая правка двигает
		 *          выравнивание тесных циклов в ней же, и сдвиг этот доходил
		 *          до полутора десятков сотых - больше самой правки. Число
		 *          же операций от выравнивания и от загрузки машины не зависит
		 *          вовсе и меняется скачком, отчего и годится доводом.
		 *
		 * \~english
		 * @brief Measure of the work of matching subject to accounting
		 * @details A measure of work differs from an execution path in kind, not in size,
		 *          and is therefore defined as a separate enumeration. A path is an
		 *          accelerator that does not change the verdict: it can be switched off
		 *          and the outcome stays correct. A measure of work cannot be switched off
		 *          at all — a step of the loop or a save of a capture cell is the matching
		 *          itself, not an acceleration of it.
		 *
		 *          The measure is introduced for the sake of an argument about causation.
		 *          The time of an edit in a hot translation unit proves nothing: every edit
		 *          shifts the alignment of the tight loops in that same unit, and that shift
		 *          reached fifteen hundredths — more than the edit being measured. The count
		 *          of operations, on the contrary, does not depend on alignment or on the
		 *          load of the machine at all and changes stepwise, which is what makes it
		 *          an argument.
		 *
		 * \~
		 */
		enum class work_t : uint8_t {
			STEPS  = 0x00, // Шаги цикла исполнения с возвратом
			SAVES  = 0x01, // Записи границы ячейки захвата текста
			CHECKS = 0x02, // Проверки принадлежности байта классу символов
			POINTS = 0x03, // Размещения точки возврата
			FRAMES = 0x04, // Заведения кадра вызова подвыражения
			/**
			 * Обходы цикла исполнения с возвратом
			 *
			 * @details Мера эта от «STEPS» отличается тем, что шаг считает
			 *          работу, а обход - заход в разбор кода операции. Ряд
			 *          одинаковых инструкций подряд проходится одним заходом,
			 *          поглощая столько байтов, какова длина ряда: работа
			 *          остаётся прежней, а обходов становится меньше, и мера
			 *          эта выигрыш прохода рядом единственная и показывает
			 */
			ROUNDS = 0x05,
			/**
			 * Попытки сопоставления исполнением с возвратом
			 *
			 * @details Мера эта считает заходы в попытку: сброс ячеек захвата,
			 *          очистку наборов, заход в цикл исполнения. Плата за заход
			 *          от работы попытки не зависит, и у выражения, делающего
			 *          много попыток с малой работой каждая, она и составляет
			 *          большую долю времени - мера эта долю ту и раскладывает
			 *          по сценариям
			 */
			ATTEMPTS = 0x06,
			COUNT  = 0x07  // Количество учитываемых мер работы
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
				/**
				 * \~russian
				 * @brief Метод извлечения счётчика меры работы
				 *
				 * @param work учитываемая мера работы сопоставления
				 * @return     количество операций учитываемой меры
				 *
				 * \~english
				 * @brief Method of getting the counter of a measure of work
				 * @param work measure of the work of matching being accounted for
				 * @return     number of operations of the measure being accounted for
				 *
				 * \~
				 */
				static uint64_t amount(const work_t work) noexcept;
				/**
				 * \~russian
				 * @brief Метод учёта выполненных операций меры работы
				 *
				 * @param work  выполненная мера работы сопоставления
				 * @param count количество выполненных операций
				 *
				 * @details Количество принимается величиной, а не единицей, ибо цикл
				 *          исполнения ведёт счётчик шагов местной переменной и вносит
				 *          его разом по завершении: приращение разделяемого счётчика
				 *          на каждом шаге обошлось бы дороже самого шага и исказило бы
				 *          образец стека, по какому и ведётся разыскание.
				 *
				 * \~english
				 * @brief Method of accounting for the operations performed of a measure of work
				 * @param work  measure of the work of matching that was performed
				 * @param count number of operations performed
				 *
				 * @details The count is taken as a quantity rather than as a unit, because the
				 *          execution loop keeps its step counter in a local variable and adds it
				 *          in one go upon completion: incrementing a shared counter at every step
				 *          would cost more than the step itself and would distort the stack
				 *          sample by which the investigation is carried out.
				 *
				 * \~
				 */
				static void spend(const work_t work, const uint64_t count) noexcept;
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
	 * Учёт выполненных операций меры работы сопоставления
	 */
	#define AWH_REGEX_SPEND(WORK, COUNT) awh::regex::Probe::spend(WORK, COUNT)
	/**
	 * Приращение местного счётчика меры работы, полем исполнителя ведомого
	 */
	#define AWH_REGEX_COUNTED(FIELD) ((FIELD)++)
/**
 * Если учёт путей исполнения сопоставления не заведён
 */
#else
	/**
	 * Учёт не ведётся: приращение счётчика расхода стоит дороже пути учитываемого
	 */
	#define AWH_REGEX_TICK(PATH) ((void) 0)
	/**
	 * Учёт не ведётся: приращение счётчика стоит дороже операции учитываемой
	 */
	#define AWH_REGEX_SPEND(WORK, COUNT) ((void) 0)
	/**
	 * Учёт не ведётся: приращение поля стоит дороже операции учитываемой
	 */
	#define AWH_REGEX_COUNTED(FIELD) ((void) 0)
#endif
