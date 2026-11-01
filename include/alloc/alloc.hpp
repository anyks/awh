/**
 * @file alloc.hpp
 * @date 2026-08-20
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
 * @brief Заголовочный файл распределителя памяти — захват выделения памяти процесса,
 *        управление возвратом памяти системе, опрос состояния расхода и разбор
 *        адреса сбоя обращения
 *
 * @section alloc_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Захват идёт по всему процессу, а не по одному фреймворку.</b>
 *          Контейнеры стандартной библиотеки ходят в malloc мимо всякого нашего
 *          договора, и распределитель, принадлежащий фреймворку, управления памятью
 *          процесса не даёт. Оттого модуль заслоняет собою выделение памяти целиком.
 *          Способ заслона разнится по системам и вынесен в capture.hpp.
 *
 *          <b>Настройки задаются вызовами, а не переменными окружения.</b> Эталоны -
 *          gperftools и mimalloc - настраиваются переменными окружения, и управление
 *          памятью оказывается вне приложения. Здесь наоборот: приложение задаёт
 *          режим само и меняет его в работе. Переменные окружения не читаются вовсе.
 *
 *          <b>Возврат памяти системе выключаем, а не ускоряем.</b> Настройка сроком
 *          и размером блока покрывает оба края: узел под нагрузкой занимает арену
 *          при заведении и не отдаёт её вовсе, слабое устройство отдаёт сразу. Оба
 *          края - один и тот же рычаг, а не два разных режима.
 *
 *          <b>Заслоны и карантин настраиваются выборкой, а не сборкой.</b> У
 *          gperftools они живут в отдельной библиотеке tcmalloc_minimal_debug, и
 *          включить их на работающем узле нельзя. Здесь они - настройка с долей
 *          выборки, отчего годны и в бою: одна выдача из тысячи стоит недорого, а
 *          повреждение памяти ловится в точке дефекта, а не всплывает фантомом.
 *
 *          <b>Разбор адреса сбоя живёт здесь, а не в модуле сигналов.</b> Сказать,
 *          чем был адрес - живым блоком, освобождённым, выходом за границу или чужой
 *          памятью, - может лишь тот, кто эту память выдавал. Модуль сигналов адрес
 *          снимает (si_addr) и приносит сюда; обратной зависимости нет.
 *
 *          <b>Источник страниц подменяем.</b> Страничная куча не знает, откуда
 *          берутся страницы: mmap, VirtualAlloc, крупные страницы или заранее
 *          отведённая область. Устройство взято у gperftools (SysAllocator) - оно
 *          там выбрано верно.
 *
 * \~english
 * @brief Header file of the memory allocator — capturing process memory allocation,
 *        controlling memory release back to the system, querying consumption state
 *        and resolving the address of an access fault
 *
 * @section alloc_decisions Deliberate decisions
 *
 * @details What is listed below looks odd but was chosen deliberately and is not
 *          subject to change. The section exists so that code analysis does not start
 *          each time from the same conclusions.
 *
 *          <b>Capture covers the whole process, not just one framework.</b> Standard
 *          library containers call malloc bypassing any of our contracts, and an
 *          allocator belonging to a framework does not give control over process
 *          memory. Hence the module shadows memory allocation entirely. The shadowing
 *          method differs by system and lives in capture.hpp.
 *
 *          <b>Settings are given by calls, not environment variables.</b> The
 *          references - gperftools and mimalloc - are configured by environment
 *          variables, and memory management ends up outside the application. Here it
 *          is the other way around: the application sets the mode itself and changes it
 *          at runtime. Environment variables are not read at all.
 *
 *          <b>Memory return to the system is disabled, not accelerated.</b> The setting
 *          by time and block size covers both ends: a node under load takes the arena
 *          at establishment and never returns it, a weak device returns at once. Both
 *          ends are one and the same lever, not two different modes.
 *
 *          <b>Guards and quarantine are configured by sampling, not by build.</b> In
 *          gperftools they live in a separate library tcmalloc_minimal_debug, and they
 *          cannot be enabled on a running node. Here they are a setting with a sampling
 *          rate, hence they work in production too: one allocation in a thousand costs
 *          little, and memory corruption is caught at the defect point rather than
 *          surfacing as a phantom.
 *
 *          <b>Fault address resolution lives here, not in the signal module.</b> Saying
 *          what the address was - a live block, a freed one, an out-of-bounds access or
 *          foreign memory - can only whoever allocated that memory. The signal module
 *          captures the address (si_addr) and brings it here; there is no reverse
 *          dependency.
 *
 *          <b>The page source is substitutable.</b> The page heap does not know where
 *          pages come from: mmap, VirtualAlloc, large pages, or a pre-reserved area.
 *          The design is taken from gperftools (SysAllocator) - it was chosen correctly
 *          there.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>
#include <functional>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "trace.hpp"
#include "source.hpp"
#include "profile.hpp"

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/macro/global.hpp"

/**
 * \~russian
 * @brief Пространство имён фреймворка
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён распределителя памяти
	 *
	 * \~english
	 * @brief Namespace of the memory allocator
	 *
	 * \~
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Способ заполнения выдаваемой и освобождаемой памяти
		 *
		 * \~english
		 * @brief The way of filling allocated and released memory
		 *
		 * \~
		 */
		enum class fill_t : uint8_t {
			NONE = 0x00, // Не трогать содержимое вовсе
			ZERO = 0x01, // Обнулять выдаваемое: незаданный указатель читается нулём
			JUNK = 0x02  // Засевать освобождаемое заметным образом: висячий указатель даёт явную чушь
		};
		/**
		 * \~russian
		 * @brief Способ отдачи свободной памяти системе
		 *
		 * @note Способ выбирается устройством машины, а не вкусом: на узле с избытком
		 *       памяти отдача стоит вызовов системы впустую, а на слабой машине
		 *       удержанное впрок и есть та самая нехватка, ради какой к распределителю
		 *       и приходят
		 *
		 * \~english
		 * @brief The way of returning free memory to the system
		 *
		 * @note The method is chosen by the device, not by taste: on a node with memory to
		 *       spare, returning costs system calls for nothing, while on a weak machine
		 *       what is held in reserve is exactly the shortage people come to the
		 *       allocator about
		 *
		 * \~
		 */
		enum class purge_t : uint8_t {
			NEVER  = 0x00, // Не отдавать вовсе, даже по просьбе: память занята навсегда
			MANUAL = 0x01, // Отдавать лишь по просьбе: Allocator::purge()
			ONFREE = 0x02, // Смотреть при освобождении: порог решает, отдавать ли
			TIMED  = 0x03  // Смотреть потоком обслуживания по отсрочке purgeDelay
		};
		/**
		 * \~russian
		 * @brief Свойство расхода памяти, доступное опросу
		 *
		 * \~english
		 * @brief Memory consumption property available for querying
		 *
		 * \~
		 */
		enum class property_t : uint8_t {
			ALLOCATED = 0x00, // Занято прикладным кодом прямо сейчас
			PEAK      = 0x01, // Наибольшее занятое за время работы
			HEAP      = 0x02, // Взято у системы ВСЕГО: куча, крупные выдачи, заслоны, свой учёт
			CACHED    = 0x03, // Свободно в поток-локальных кэшах
			PAGEFREE  = 0x04, // Свободно в страничной куче, но системе не отдано
			UNMAPPED  = 0x05  // Отдано системе обратно
		};
		/**
		 * \~russian
		 * @brief Чем оказался разбираемый адрес
		 *
		 * \~english
		 * @brief What the resolved address turned out to be
		 *
		 * \~
		 */
		enum class origin_t : uint8_t {
			UNKNOWN  = 0x00, // Разобрать не удалось
			FOREIGN  = 0x01, // Память нам не принадлежит вовсе
			NULLPAGE = 0x02, // Нулевая страница: разыменование нулевого указателя
			LIVE     = 0x03, // Живой выданный блок
			FREED    = 0x04, // Блок освобождён, но ещё под карантином
			OVERRUN  = 0x05, // Адрес за концом блока
			UNDERRUN = 0x06  // Адрес перед началом блока
		};

		/**
		 * \~russian
		 * @brief Структура сведения о защите выданного блока
		 *
		 * @note Оба признака отвечают о том, что состоялось НА ДЕЛЕ, а не о том, о чём
		 *       просили: умеет такое не всякая система, а право на запрет подкачки к
		 *       тому же ограничено пределом. Молчаливое понижение защиты хуже честного
		 *       отказа - решать, годится ли она, обязан звавший
		 *
		 * \~english
		 * @brief Information about the protection of an allocated block
		 *
		 * @note Both flags report what happened IN FACT, not what was asked for: not every
		 *       system can do it, and the right to prevent swapping is in any case limited.
		 *       A silent downgrade of protection is worse than an honest refusal - the
		 *       caller must decide whether it suffices
		 *
		 * \~
		 */
		typedef struct Shelter {
			// Признак блока, которому запрещено уходить в подкачку
			bool wired;
			// Признак блока, укрытого от снимков памяти
			bool hidden;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			constexpr Shelter() noexcept :
			 wired(false), hidden(false) {}
		} shelter_t;
		/**
		 * \~russian
		 * @brief Структура сведения о разобранном адресе
		 *
		 * \~english
		 * @brief Information about the resolved address
		 *
		 * \~
		 */
		typedef struct Region {
			// Размер блока, если он определён
			size_t size;
			// Чем оказался разбираемый адрес
			origin_t origin;
			// Смещение разбираемого адреса от начала блока
			ptrdiff_t offset;
			// Адрес начала блока, если он определён
			const void * begin;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			constexpr Region() noexcept :
			 size(0), origin(origin_t::UNKNOWN),
			 offset(0), begin(nullptr) {}
		} region_t;
		/**
		 * \~russian
		 * @brief Класс настройки распределителя памяти
		 *
		 * @note Значения по умолчанию отвечают обычному узлу: арена не занимается
		 *       заранее, возврат отложен, заслоны выключены. Крайние режимы -
		 *       нагруженный узел и слабое устройство - задаются потребителем
		 *
		 * \~english
		 * @brief Memory allocator configuration class
		 *
		 * @note Default values suit an ordinary node: the arena is not taken in advance,
		 *       return is deferred, guards are off. Extreme modes - a loaded node and a
		 *       weak device - are set by the consumer
		 *
		 * \~
		 */
		typedef class Options {
			public:
				// Запрет обращаться к системе сверх занятой области
				bool confined;
				// Просить у системы крупные страницы
				bool hugePages;
			public:
				// Занимаемая при заведении процесса область, в байтах: нуль - не занимать
				size_t arena;
				/**
				 * \~russian
				 * @brief Потолок придержанных крупных областей, в байтах
				 *
				 * @details Крупная выдача идёт у системы напрямую, и отданная ей область при
				 *          следующей выдаче приходит ЧИСТОЙ: за каждую её страницу платится отказом
				 *          страницы при первом обращении. Замером на macOS это три сотни микросекунд
				 *          против девяти на пяти мегабайтах - разница в тридцать раз, и берётся она
				 *          не выдачей, а страницами.
				 *
				 * @details Придержка держит освобождённые области у себя, готовыми к повторной
				 *          выдаче. Нуль - не придерживать вовсе, отдавать системе тут же; так вёл
				 *          себя слой прежде.
				 *
				 * @note Умолчание - 256 МБ, и выбрано оно ОСТОРОЖНЫМ, а не наибольшим
				 *
				 * @details Прежде здесь стояли шестнадцать мегабайт, и это противоречило умолчанию
				 *          `purgeMode`: тот гласит `MANUAL` - «отдавать лишь по просьбе», - а слой
				 *          крупных выдач отдавал системе всё сверх потолка, никого не спрашивая. Два
				 *          слоя одного распределителя жили по противоположным укладам, и обещание
				 *          настройки нарушалось молча. Просьба же опустошает придержку и сегодня -
				 *          `Allocator::purge()` зовёт `Huge::drain()` первым делом.
				 *
				 * @details Цена того расхождения измерена на наборе замеров ABC (macOS ARM64): работа
				 *          брала у системы 44 897 МБ крупными областями и отдавала их обратно, платя
				 *          2 422 763 отказами страниц против 119 841 у системного распределителя - в
				 *          двадцать раз больше, а времени в ядре 2.24 с против 0.19. Разбор шёл при
				 *          этом в полтора-два раза медленнее системного.
				 *
				 * @details Три величины замерены на одном и том же наборе:
				 *
				 *          - шестнадцать мегабайт: расход 489 МБ, отставание от 1.24x до 1.75x;
				 *          - 256 МБ: расход 630 МБ, около половины выигрыша;
				 *          - без предела: расход 1071 МБ, отставание закрыто целиком (1.60x → 1.00x
				 *          на чтении крупной записи; из 3240 крупных выдач 3224 достаются готовыми,
				 *          отказов остаётся 66 578).
				 *
				 * @details Умолчанием взята середина, и довод у неё один: системный распределитель
				 *          берёт на том же наборе 717 МБ. Умолчание, расходующее БОЛЬШЕ системного,
				 *          для встраиваемой библиотеки негодно - тот, кто нас встроил и ничего не
				 *          настраивал, платил бы за решение, какого не принимал. Скорость же остаётся
				 *          доступной одной строкой настройки, а память, отданную под придержку, вернуть
				 *          после упора приложения нельзя.
				 *
				 * @details Приложению, какому скорость дороже памяти, потолок задаётся числом - вплоть
				 *          до `static_cast <size_t> (-1)`, - а вместе с ним стоит выбирать и
				 *          `purgeMode`: уклады эти парны.
				 *
				 * @note Совет ядру забыть содержимое придержанной области (`MADV_FREE`) взамен
				 *       отдачи проверен и ОТВЕРГНУТ замером: расход не убавился ни на октет,
				 *       а время в ядре выросло с 0.18 до 5.99 секунды
				 *
				 *
				 * \~english
				 * @brief Ceiling of retained huge regions, in bytes
				 *
				 * @details A huge allocation goes to the system directly, and a region returned to it
				 *          comes back CLEAN on the next allocation: a page fault is paid for each of
				 *          its pages on first access. Measured on macOS this is three hundred
				 *          microseconds versus nine for five megabytes - a thirtyfold difference, and
				 *          it is taken not by the allocation but by the pages.
				 *
				 * @details Retention keeps released regions at hand, ready for reuse. Zero means no
				 *          retention at all, return to the system at once; that was the previous
				 *          behaviour of the layer.
				 *
				 * @note The default is 256 MB, and it was chosen as CAUTIOUS, not maximal
				 *
				 * @details Previously sixteen megabytes stood here, and that contradicted the
				 *          purgeMode default: the latter says MANUAL - return only on request - while
				 *          the huge allocation layer returned everything beyond the ceiling to the
				 *          system without asking anyone. Two layers of one allocator lived by opposite
				 *          rules, and the setting promise was violated silently. The request empties
				 *          the retention to this day - Allocator::purge() calls Huge::drain() first.
				 *
				 * @details The price of that discrepancy was measured on the ABC benchmark set
				 *          (macOS ARM64): the work took 44,897 MB from the system in huge regions and
				 *          returned them, paying 2,422,763 page faults against 119,841 for the system
				 *          allocator - twenty times more, and kernel time 2.24 s versus 0.19. Parsing
				 *          was meanwhile 1.5-2 times slower than the system allocator.
				 *
				 * @details Three values were measured on the same set:
				 *
				 *          - sixteen megabytes: consumption 489 MB, lag from 1.24x to 1.75x;
				 *          - 256 MB: consumption 630 MB, about half the win;
				 *          - no limit: consumption 1071 MB, the lag closed entirely (1.60x to 1.00x
				 *          on large-record reading; of 3240 huge allocations 3224 are served from
				 *          cache, 66,578 faults remain).
				 *
				 * @details The default is the middle, and it has one argument: the system allocator
				 *          takes 717 MB on the same set. A default consuming MORE than the system is
				 *          unsuitable for an embedded library - whoever embedded us and configured
				 *          nothing would pay for a decision they did not make. Speed remains available
				 *          with a single setting line, while memory given to retention cannot be
				 *          reclaimed after the application hits a wall.
				 *
				 * @details For an application where speed matters more than memory, the ceiling is
				 *          set to a number - up to static_cast <size_t> (-1) - and together with it
				 *          purgeMode should be chosen: these rules are paired.
				 *
				 * @note The advice to the kernel to forget the contents of a retained region
				 *       (MADV_FREE) instead of returning it was tested and REJECTED by measurement:
				 *       consumption did not decrease by a single octet, and kernel time grew from 0.18
				 *       to 5.99 seconds
				 *
				 * \~
				 */
				size_t hugeCache;
				// Потолок кучи, в байтах: нуль - без потолка
				size_t heapLimit;
				/**
				 * \~russian
				 * @brief Потолок придержки областей сверх разрядов, в байтах: нуль - не придерживать
				 *
				 * @details Область сверх разрядов (от границы разрядов до размера куска кучи) после
				 *          освобождения не возвращается куче, а ждёт следующего запроса того же
				 *          размера. Круг «выдал - освободил» ускоряется этим в полтора раза.
				 *
				 * @warning Придержка ВРАЖДЕБНА росту области на месте: растёт та за счёт
				 *          СВОБОДНЫХ соседей, а придержанная для кучи занята. Замерено -
				 *          рост перевыдачей падает вчетверо. Оттого умолчание нулевое:
				 *          обмен этот выбирает приложение, а не мы за него
				 *
				 *
				 * \~english
				 * @brief Ceiling of retained regions beyond the size classes, in bytes: zero means no
				 *        retention
				 *
				 * @details A region beyond the size classes (from the class boundary to the heap chunk
				 *          size) is not returned to the heap after free but waits for the next request
				 *          of the same size. The allocate-free cycle is accelerated by this 1.5 times.
				 *
				 * @warning Retention is HOSTILE to in-place region growth: a region grows at the
				 *          expense of FREE neighbours, while a retained one is occupied for the heap.
				 *          Measured - reallocation-based growth drops fourfold. Hence the default is
				 *          zero: this trade-off is chosen by the application, not by us
				 *
				 * \~
				 */
				size_t spanCache;
				/**
				 * \~russian
				 * @brief Порог свободного в куче, ниже какого отдачу не затевать, в байтах
				 *
				 * @details Отдача стоит вызовов системы и правки таблиц страниц: затевать её ради
				 *          пары освободившихся страниц значило бы платить за неё чаще, чем она того
				 *          стоит. Нуль - порога нет, отдавать всё, что подошло по отсрочке.
				 *
				 *
				 * \~english
				 * @brief Free heap threshold below which returning is not started, in bytes
				 *
				 * @details Returning costs system calls and page table edits: starting it for a couple
				 *          of freed pages would mean paying for it more often than it is worth. Zero
				 *          means no threshold, return everything that came due by the delay.
				 *
				 * \~
				 */
				size_t purgeFree;
				// Доля выборки заслонов: одна выдача из скольких: нуль - заслоны выключены
				size_t guardRate;
				// Объём карантина на освобождённое, в байтах: нуль - карантин выключен
				size_t quarantine;
				/**
				 * \~russian
				 * @brief Наименьший возвращаемый системе кусок, в байтах
				 *
				 * @note Отдача мелочью не окупается: вызов системы стоит одинаково и для страницы,
				 *       и для мегабайта. Кусок этот НЕ МОЖЕТ быть крупнее куска страничной кучи
				 *       (четыре мегабайта): области её через границу куска не идут, и заказанное
				 *       сверх того не наберётся никогда - отдача просто перестанет происходить.
				 *       Проверено щупом: при заказе в восемь мегабайт не отдалось ни байта.
				 *
				 *
				 * \~english
				 * @brief Smallest piece returned to the system, in bytes
				 *
				 * @note Returning small pieces does not pay off: a system call costs the same for a
				 *       page and for a megabyte. This piece CANNOT be larger than the page heap chunk
				 *       (four megabytes): its regions do not cross a chunk boundary, and anything
				 *       ordered beyond that will never accumulate - returning will simply stop
				 *       happening. Verified by a probe: ordering eight megabytes returned not a byte.
				 *
				 * \~
				 */
				size_t purgeBlock;
				// Потолок поток-локальных кэшей, в байтах: нуль - по усмотрению модуля
				size_t cacheLimit;
				/**
				 * \~russian
				 * @brief Граница разрядов, в байтах: нуль - по усмотрению модуля
				 *
				 * @details Блок ниже границы обслуживается разрядом с кэшем потока, выше - областью
				 *          страниц. Размен обеих сторон отмерен и записан у `Classes::DEFAULT`:
				 *          высокая граница выигрывает втрое на выдаче крупных блоков постоянного
				 *          размера и проигрывает вчетверо на перевыдаче с ростом, ибо область страниц
				 *          умеет расти на месте, а блок разряда - нет.
				 *
				 * @note Поднимать стоит приложению, какое берёт много крупных блоков постоянного
				 *       размера и не растит их перевыдачей: буфера сетевого обмена, страницы
				 *       хранилищ, снимки. Выше `Classes::MAXIMUM` граница не поднимается: таблицы
				 *       поиска построены на нём
				 *
				 *
				 * \~english
				 * @brief Size class boundary, in bytes: zero means at the module's discretion
				 *
				 * @details A block below the boundary is served by a class with a thread cache, above
				 *          it by a page region. The trade-off on both sides is measured and recorded
				 *          at Classes::DEFAULT: a high boundary wins threefold on allocating large
				 *          blocks of constant size and loses fourfold on reallocation with growth,
				 *          because a page region can grow in place while a class block cannot.
				 *
				 * @note Worth raising for an application that takes many large blocks of constant
				 *       size and does not grow them by reallocation: network exchange buffers, storage
				 *       pages, snapshots. Above Classes::MAXIMUM the boundary is not raised: the
				 *       lookup tables are built on it
				 *
				 * \~
				 */
				size_t classLimit;
				// Порог доклада о крупном выделении, в байтах: нуль - не докладывать
				size_t reportLarge;
				/**
				 * \~russian
				 * @brief Доля выборки учёта мест выдачи: одна выдача из скольких
				 *
				 * @details Нуль - учёт выключен, и выключен он по умолчанию: место выдачи стоит съёма
				 *          стека при выдаче и поиска по таблице при КАЖДОМ освобождении, в том числе
				 *          у блоков, под учёт не попавших. Плата эта для разбора, а не для боя.
				 *
				 *
				 * \~english
				 * @brief Sampling rate for allocation site accounting: one allocation out of how many
				 *
				 * @details Zero means accounting is off, and it is off by default: an allocation site
				 *          costs a stack capture on allocation and a table lookup on EVERY free,
				 *          including blocks not taken into account. This price is for analysis, not
				 *          for production.
				 *
				 * \~
				 */
				size_t profileRate;
				// Отсрочка возврата памяти системе, в миллисекундах: -1 - не возвращать вовсе
				int64_t purgeDelay;
			public:
				// Способ заполнения выдаваемой и освобождаемой памяти
				fill_t fill;
				/**
				 * \~russian
				 * @brief Способ отдачи свободной памяти системе
				 *
				 * @details Способ отвечает за то, КОГДА распределитель смотрит, не пора ли отдавать;
				 *          сколько при этом отдаётся, решают `purgeDelay`, `purgeFree` и `purgeBlock`.
				 *          Разделение это не выдумано здесь: так устроены и glibc с её порогом
				 *          усечения, и jemalloc с отсрочкой увядания, и tcmalloc со скоростью отдачи
				 *
				 *   - `NEVER`  - не отдавать вовсе, даже по просьбе. Для машин с постоянной
				 *                нагрузкой и для работы в реальном времени, где вызов системы
				 *                посреди работы дороже любой сэкономленной страницы
				 *   - `MANUAL` - отдавать лишь по просьбе `Allocator::purge()`. Приложение
				 *                само знает свои затишья: конец запроса, выгрузка набора
				 *   - `ONFREE` - смотреть при каждом освобождении. С нулевой отсрочкой и без
				 *                порога отдаёт сразу, ничего не накапливая - для слабых машин,
				 *                где память дороже тактов; с порогом `purgeFree` копит до него
				 *                и отдаёт кусками не меньше `purgeBlock`
				 *   - `TIMED`  - смотреть потоком обслуживания раз в полсотни миллисекунд.
				 *                Путь освобождения не дорожает ни на такт, но заводится поток
				 *
				 * @note По умолчанию взят `MANUAL`: заводить поток обслуживания в каждом процессе,
				 *       позвавшем захват, приложение не просило. Отсрочка при этом задана - она
				 *       действует у тех способов, где обход есть
				 *
				 *
				 * \~english
				 * @brief The way of returning free memory to the system
				 *
				 * @details The method answers WHEN the allocator looks at whether it is time to
				 *          return; how much is returned is decided by purgeDelay, purgeFree and
				 *          purgeBlock. This division is not invented here: it is how glibc with its
				 *          trim threshold, jemalloc with its decay delay, and tcmalloc with its return
				 *          speed are built
				 *
				 *    - NEVER  - never return, not even on request. For machines with constant load
				 *               and for real-time work, where a system call amid work costs more than
				 *               any saved page
				 *    - MANUAL - return only on request of Allocator::purge(). The application knows
				 *               its own quiet times: end of request, dataset unload
				 *    - ONFREE - look at every free. With zero delay and no threshold it returns at
				 *               once, accumulating nothing - for weak machines where memory costs
				 *               more than cycles; with the purgeFree threshold it accumulates up to it
				 *               and returns in pieces not smaller than purgeBlock
				 *    - TIMED  - look by the service thread every fifty milliseconds. The free path
				 *               costs not a cycle more, but a thread is started
				 *
				 * @note The default is MANUAL: starting a service thread in every process that called
				 *       capture was not requested by the application. The delay is nonetheless set -
				 *       it acts in those methods where a sweep exists
				 *
				 * \~
				 */
				purge_t purgeMode;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @details Помечен `constexpr` НЕ ради скорости, а ради порядка построения, и
				 *          снимать пометку нельзя. Действующие настройки лежат переменной
				 *          модуля, а захват выдачи памяти случается ДО построения переменных
				 *          модуля: приложение вправе завести объект фреймворка переменной
				 *          уровня файла, и тогда его конструктор идёт первым же обходом
				 *          построения. Без `constexpr` собиратель заводит для переменной
				 *          обычное построение, и то ЗАТИРАЕТ заданные захватом настройки
				 *          умолчаниями - молча и целиком: ни потолка придержки, ни порядка
				 *          отдачи, ни карантина, ни заслонов.
				 *
				 * @note Довод общий с набором функций захвата; там же записано, чем это доказано
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @details Marked constexpr NOT for speed but for construction order, and the mark
				 *          cannot be removed. The effective settings live in a module variable, and
				 *          memory allocation capture happens BEFORE module variables are constructed:
				 *          the application may create a framework object at file scope, and then its
				 *          constructor runs in the very first construction pass. Without constexpr the
				 *          compiler creates ordinary initialization for the variable, and that
				 *          OVERWRITES the settings set by capture with defaults - silently and
				 *          entirely: no retention ceiling, no return order, no quarantine, no guards.
				 *
				 * @note The argument is shared with the set of capture functions; the proof is
				 *       recorded there as well
				 *
				 * \~
				 */
				constexpr Options() noexcept :
				 confined(false), hugePages(false),
				 arena(0), hugeCache(256u * 1024u * 1024u),
				 heapLimit(0), spanCache(0), purgeFree(0),
				 guardRate(0), quarantine(0), purgeBlock(0),
				 cacheLimit(0), classLimit(0), reportLarge(0),
				 profileRate(0), purgeDelay(10),
				 fill(fill_t::NONE), purgeMode(purge_t::MANUAL) {}
		} options_t;
		/**
		 * \~russian
		 * @brief Сторож постоянного заведения настроек
		 *
		 * @details Сличение это вычисляется на этапе сборки и требует, чтобы конструктор годился
		 *          в постоянное выражение. Снимут `constexpr` - сборка встанет здесь, а не
		 *          обернётся молчаливой потерей настроек у потребителя.
		 *
		 *
		 * \~english
		 * @brief Guard of constant settings initialization
		 *
		 * @details This check is computed at build time and requires the constructor to be a
		 *          constant expression. Remove constexpr - the build stops here rather than
		 *          wrapping into a silent loss of settings for the consumer.
		 *
		 * \~
		 */
		static_assert((Options().purgeMode == purge_t::MANUAL), "Settings must be initialized with a constant value");
		/**
		 * \~russian
		 * @brief Класс распределителя памяти
		 *
		 * @note Методы объявлены статическими намеренно: распределитель заслоняет собою
		 *       выделение памяти всего процесса, и второго такого в процессе быть не
		 *       может. Заводить объект значило бы обещать возможность, какой нет
		 *
		 * \~english
		 * @brief Memory allocator class
		 *
		 * @note The methods are deliberately declared static: the allocator shadows memory
		 *       allocation for the whole process, and a second one cannot exist in a
		 *       process. Creating an object would promise a possibility that does not exist
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Allocator {
			public:
				/**
				 * \~russian
				 * @brief Метод захвата выделения памяти процесса
				 *
				 * @note Звать до порождения потоков. Захват переписыванием переписывает
				 *       начала функций уже загруженной библиотеки времени исполнения, и
				 *       поток, исполняющий их в этот миг, получил бы половину прежнего
				 *       кода и половину нового
				 *
				 * @note У систем ELF захват НЕ выборочен: там подмена идёт именами и
				 *       случается САМИМ СВЯЗЫВАНИЕМ - распределитель встаёт на место
				 *       системного с первой выдачи, звал кто этот метод или нет. Здесь
				 *       он лишь применяет настройки к уже заведённой куче, а release
				 *       снимает только отметку. Выборочность есть у macOS и MS Windows:
				 *       там имена наших функций частные, и до захвата они не зовутся
				 *
				 * @note Отключается ключом сборки: cmake -DCMAKE_BUILD_ALLOCATOR=OFF ..
				 *       Признак AWH_ALLOC_DISABLED обращает наши функции в частные, и
				 *       метод этот отвечает отказом на всех системах. Ключ - подпорка на
				 *       время доводки модуля
				 *
				 * @param options настройки распределителя
				 * @return        признак состоявшегося захвата
				 *
				 * \~english
				 * @brief Method of capturing process memory allocation
				 *
				 * @note Call before spawning threads. Rewriting capture rewrites the entries of
				 *       functions of an already loaded runtime library, and a thread executing
				 *       them at that instant would get half of the old code and half of the new
				 *
				 * @note On ELF systems capture is NOT selective: there substitution goes by names
				 *       and happens BY THE LINKER ITSELF - the allocator takes the place of the
				 *       system one from the first allocation, whether anyone called this method
				 *       or not. Here it merely applies settings to an already established heap,
				 *       and release only removes the mark. Selectivity exists on macOS and MS
				 *       Windows: there our function names are private and are not called before
				 *       capture
				 *
				 * @note Disabled by the build key: cmake -DCMAKE_BUILD_ALLOCATOR=OFF .. The flag
				 *       AWH_ALLOC_DISABLED makes our functions private, and this method answers
				 *       with refusal on all systems. The key is a crutch during module tuning
				 *
				 * @param options allocator settings
				 * @return        flag of the capture having taken place
				 *
				 * \~
				 */
				static bool capture(const options_t & options) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия захвата выделения памяти процесса
				 *
				 * @note Выданная память остаётся годной: снятие возвращает прежние
				 *       функции, а освобождение выданного нами по-прежнему идёт к нам
				 *
				 * \~english
				 * @brief Method of releasing the capture of process memory allocation
				 *
				 * @note Allocated memory remains valid: release returns the previous functions,
				 *       and freeing what we allocated still comes to us
				 *
				 * \~
				 */
				static void surrender() noexcept;
				/**
				 * \~russian
				 * @brief Метод определения захваченности выделения памяти процесса
				 *
				 * @return признак захвата
				 *
				 * \~english
				 * @brief Method of determining whether process memory allocation is captured
				 *
				 * @return capture flag
				 *
				 * \~
				 */
				static bool captured() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения действующих настроек
				 *
				 * @return действующие настройки
				 *
				 * \~english
				 * @brief Method of getting the effective settings
				 *
				 * @return effective settings
				 *
				 * \~
				 */
				static const options_t & options() noexcept;
				/**
				 * \~russian
				 * @brief Метод изменения настроек в работе
				 *
				 * @note Меняются лишь те настройки, какие в работе изменимы: отсрочка
				 *       возврата, потолки, выборка заслонов, карантин и способ
				 *       заполнения. Размер занимаемой при заведении области значение
				 *       имеет лишь при захвате и здесь не действует
				 *
				 * @param options требуемые настройки
				 *
				 * \~english
				 * @brief Method of changing settings at runtime
				 *
				 * @note Only those settings that are changeable at runtime are changed: return
				 *       delay, ceilings, guard sampling, quarantine and fill method. The size of
				 *       the area taken at establishment matters only at capture and does not act
				 *       here
				 *
				 * @param options required settings
				 *
				 * \~
				 */
				static void options(const options_t & options) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод опроса расхода памяти
				 *
				 * @param property требуемое свойство
				 * @return         величина свойства в байтах
				 *
				 * \~english
				 * @brief Method of querying memory consumption
				 *
				 * @param property required property
				 * @return         property value in bytes
				 *
				 * \~
				 */
				static size_t property(const property_t property) noexcept;
				/**
				 * Счётчик выдач языка C++ заводится только ключом стенда замера
				 */
				#if defined(AWH_ALLOC_COUNTING)
					/**
					 * \~russian
					 * @brief Метод управления счётчиком выдач языка C++
					 *
					 * @note Ключ сборки `AWH_ALLOC_COUNTING` заводит счётчик лишь для стендов
					 *       замера: в обычной сборке ни счётчика, ни метода нет
					 *
					 * @param mode режим счёта: истина включает счёт и обнуляет накопленное
					 *
					 * \~english
					 * @brief Method of controlling the C++ allocation counter
					 *
					 * @note The `AWH_ALLOC_COUNTING` build key creates the counter for benchmark
					 *       stands only: a regular build has neither the counter nor the method
					 *
					 * @param mode counting mode: true enables counting and zeroes the accumulated values
					 *
					 * \~
					 */
					static void counting(const bool mode) noexcept;
					/**
					 * \~russian
					 * @brief Метод опроса счётчика выдач языка C++
					 *
					 * @param count количество выполненных выдач
					 * @param bytes суммарный объём выданного в байтах
					 *
					 * \~english
					 * @brief Method of querying the C++ allocation counter
					 *
					 * @param count number of allocations performed
					 * @param bytes total volume allocated in bytes
					 *
					 * \~
					 */
					static void allocations(size_t & count, size_t & bytes) noexcept;
				#endif
				/**
				 * \~russian
				 * @brief Метод возврата системе свободной памяти
				 *
				 * @note Отсрочка возврата при этом не отменяется: метод возвращает то,
				 *       что свободно сейчас, а установленный порядок остаётся прежним
				 *
				 * @note Кэш опустошается лишь у ЗВАВШЕГО потока: кэш потока замком не
				 *       защищён, и править его чужими руками нельзя - прочим потокам
				 *       ставится просьба, какую они исполняют сами на ближайшем
				 *       освобождении. Оттого у многопоточного приложения ответ бывает
				 *       заметно меньше свободного: остальное лежит в кэшах, какие
				 *       опустеют следом
				 *
				 * @note При запрете отдачи (`purge_t::NEVER`) ответом идёт нуль, но кэши
				 *       всё равно опустошаются: запрет касается ОТДАЧИ СИСТЕМЕ, а не
				 *       возврата блоков центральным спискам
				 *
				 * @return объём возвращённой системе памяти в байтах
				 *
				 * \~english
				 * @brief Method of returning free memory to the system
				 *
				 * @note The return delay is not cancelled: the method returns what is free now,
				 *       and the established order remains unchanged
				 *
				 * @note The cache is emptied only for the CALLING thread: the thread cache is not
				 *       protected by a lock, and it cannot be edited by foreign hands - other
				 *       threads are sent a request they execute themselves on the nearest free.
				 *       Hence in a multithreaded application the answer may be noticeably less
				 *       than free: the rest lies in caches that will empty shortly
				 *
				 * @note With returning forbidden (purge_t::NEVER) the answer is zero, but caches
				 *       are still emptied: the prohibition concerns RETURNING TO THE SYSTEM, not
				 *       the return of blocks to central lists
				 *
				 * @return amount of memory returned to the system in bytes
				 *
				 * \~
				 */
				static size_t purge() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод подмены источника страниц
				 *
				 * @note Звать до захвата: заменять источник у кучи, уже раздавшей страницы,
				 *       значило бы отдавать их потом не тому, кто выдавал
				 *
				 * @note Подмена состоится лишь тогда, когда до неё НИКТО не просил памяти
				 *       через нас, и это зависит не от системы, а от того, что успело
				 *       случиться в процессе. У систем ELF и у macOS подмена именами
				 *       работает с самого запуска, и куча там обыкновенно заводится ещё при
				 *       заведении переменных процесса - прежде, чем прикладной код получит
				 *       управление; тогда подмена отвечает отказом. Но зависит это от
				 *       библиотеки времени исполнения и от самой программы: у OpenBSD,
				 *       скажем, до `main` не выделяется ничего, и подмена там проходит.
				 *       Оттого ответ метода обязан быть ПРОВЕРЕН, а не предположен: отказ -
				 *       обычный исход
				 *
				 * @param source требуемый источник страниц
				 * @return       признак состоявшейся подмены
				 *
				 * \~english
				 * @brief Method of substituting the page source
				 *
				 * @note Call before capture: replacing the source in a heap that has already
				 *       handed out pages would mean returning them later to the wrong allocator
				 *
				 * @note The substitution happens only when NOBODY has asked for memory through us
				 *       before it, and that depends not on the system but on what has managed to
				 *       happen in the process. On ELF systems and macOS name substitution works
				 *       from startup, and the heap there is usually established during process
				 *       variable initialization - before the application code gets control; then
				 *       substitution answers with refusal. But it depends on the runtime library
				 *       and the program itself: on OpenBSD, for instance, nothing is allocated
				 *       before main, and substitution passes there. Hence the method answer must
				 *       be CHECKED, not assumed: refusal is a normal outcome
				 *
				 * @param source required page source
				 * @return       flag of the substitution having taken place
				 *
				 * \~
				 */
				static bool source(source_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения действующего источника страниц
				 *
				 * @return действующий источник страниц
				 *
				 * \~english
				 * @brief Method of getting the effective page source
				 *
				 * @return effective page source
				 *
				 * \~
				 */
				static source_t * source() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора адреса обращения
				 *
				 * @note Метод зовётся из разбора сбоя обращения, а не из обработчика
				 *       сигнала: внутри берутся замки, а обработчику того нельзя
				 *
				 * @param addr разбираемый адрес
				 * @return     сведения о разобранном адресе
				 *
				 * \~english
				 * @brief Method of resolving an access address
				 *
				 * @note The method is called from fault address resolution, not from the signal
				 *       handler: locks are taken inside, and the handler must not do that
				 *
				 * @param addr address being resolved
				 * @return     information about the resolved address
				 *
				 * \~
				 */
				static region_t resolve(const void * addr) noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи защищённой памяти
				 *
				 * @note Защита состоит из трёх обещаний: область не попадает в снимок
				 *       памяти при падении, ей запрещено уходить в подкачку, а содержимое
				 *       затирается при освобождении. Первые два умеет не всякая система, и
				 *       ответ `shelter` говорит, что состоялось НА ДЕЛЕ. Затирание же
				 *       обещано всегда
				 *
				 * @note Защита эта - НЕ шифрование: живую память шифровать нечем, к ней
				 *       обращается сам процессор. Она защищает от снимков - подкачки,
				 *       снимка при падении, спящего режима, - а не от того, кто читает
				 *       память процесса: такой прочтёт и ключ
				 *
				 * @note Блок берётся у системы отдельной областью, а не из кучи: цена его
				 *       - целая страница даже под малый запрос. Место таких блоков -
				 *       ключи и пароли, а не рабочие буферы
				 *
				 * @note Освобождается методом `release`, а НЕ обычным `free`. Совпадают
				 *       они лишь там, где захват выдачи памяти процесса состоялся: у
				 *       систем ELF наш `free` стоит в двоичном файле с самого
				 *       связывания, а у macOS и MS Windows без захвата `free` -
				 *       СИСТЕМНЫЙ, и наша область валит его. Проверено тестом склада
				 *       тайн: `pointer being freed was not allocated`
				 *
				 * @param size    требуемый размер в байтах
				 * @param shelter сведения о состоявшейся защите либо nullptr
				 * @return        адрес выданной памяти либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating protected memory
				 *
				 * @note Protection consists of three promises: the area does not appear in a
				 *       crash memory dump, it is forbidden to swap out, and the content is wiped
				 *       on free. Not every system can do the first two, and the shelter answer
				 *       tells what happened IN FACT. Wiping, however, is always promised
				 *
				 * @note This protection is NOT encryption: live memory cannot be encrypted, the
				 *       processor itself accesses it. It protects from snapshots - swapping,
				 *       crash dump, hibernation - not from one who reads process memory: such a
				 *       one reads the key as well
				 *
				 * @note The block is taken from the system as a separate area, not from the heap:
				 *       its cost is a whole page even for a small request. The place for such
				 *       blocks is keys and passwords, not working buffers
				 *
				 * @note Freed by the release method, NOT by ordinary free. They coincide only
				 *       where process memory allocation capture succeeded: on ELF systems our
				 *       free is in the binary from linking onward, while on macOS and MS Windows
				 *       without capture free is the SYSTEM one, and our area crashes it. Verified
				 *       by the secret storage test: pointer being freed was not allocated
				 *
				 * @param size    required size in bytes
				 * @param shelter information about the protection achieved or nullptr
				 * @return        address of the allocated memory or nullptr
				 *
				 * \~
				 */
				static void * secure(const size_t size, shelter_t * shelter = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод смены доступности защищённого блока
				 *
				 * @details Закрытый блок отвечает на чтение ОТКАЗОМ, а не байтами:
				 *          отладчик, читающий чужую память, спотыкается, а переполнение
				 *          соседнего буфера упирается в недоступную страницу вместо того,
				 *          чтобы вычитать содержимое. Место такого блока - ключи и пароли
				 *          между обращениями к ним.
				 *
				 * @note Границы блока правятся САМИ: начало опускается к границе
				 *       страницы, длина поднимается к ней же. Смена доступности идёт
				 *       страницами, и невыровненное начало система вправе отвергнуть -
				 *       отказ этот читался бы как «не умею»
				 *
				 * @note Отказ означает «не умею»: источник, страницами системы не
				 *       владеющий, доступности не меняет вовсе. Отличать его от «не
				 *       вышло» обязан звавший
				 *
				 * @note Закрытый блок обязан быть открыт ПРЕЖДЕ освобождения: затирание
				 *       при возврате пишет в него, а запись в недоступную страницу валит
				 *       программу
				 *
				 * @param addr   адрес защищённого блока
				 * @param opened признак открываемого блока
				 * @return       признак выполнения операции
				 *
				 * \~english
				 * @brief Method of changing the accessibility of a protected block
				 *
				 * @details A closed block answers reads with a REFUSAL, not bytes: a debugger reading
				 *          foreign memory stumbles, and a neighbouring buffer overflow hits an
				 *          inaccessible page instead of reading out the content. The place for such a
				 *          block is keys and passwords between accesses to them.
				 *
				 * @note Block boundaries are adjusted THEMSELVES: the start is lowered to the page
				 *       boundary, the length is raised to it. Accessibility changes go by pages, and
				 *       the system may reject an unaligned start - that rejection would read as
				 *       cannot do
				 *
				 * @note Refusal means cannot do: a source not owning system pages does not
				 *       change accessibility at all. The caller must distinguish it from did not
				 *       work out
				 *
				 * @note A closed block must be opened BEFORE freeing: wiping on return writes to it,
				 *       and writing to an inaccessible page crashes the program
				 *
				 * @param addr   address of the protected block
				 * @param opened flag of the block being opened
				 * @return       flag of the operation having been performed
				 *
				 * \~
				 */
				static bool shield(const void * addr, const bool opened) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата выданной памяти
				 *
				 * @note Возврат этот идёт к НАМ всегда, состоялся захват выдачи памяти
				 *       процесса или нет. Обычный `free` там, где захвата не было,
				 *       принадлежит системе и нашу область не узнаёт
				 *
				 * @param addr адрес возвращаемой памяти
				 *
				 * \~english
				 * @brief Method of releasing the allocated memory
				 *
				 * @note This return goes to US always, whether process memory allocation capture
				 *       succeeded or not. The ordinary free where there was no capture belongs to
				 *       the system and does not recognise our area
				 *
				 * @param addr address of the memory being released
				 *
				 * \~
				 */
				static void release(void * addr) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перебора удерживаемой прикладным кодом памяти
				 *
				 * @note Отвечает он о том, что выдано и до сих пор не освобождено, а НЕ
				 *       об утечках: намерений прикладного кода распределитель не знает, и
				 *       блок, живущий до конца процесса, бывает и утечкой, и намеренным
				 *       кэшем. Называть найденное утечкой волен лишь тот, кто писал код
				 *
				 * @note Перебираются лишь блоки, попавшие под выборку `profileRate`: без
				 *       неё перебор отвечает нулём, а не пустотой знания
				 *
				 * @note Перебор идёт с взятым замком учёта: отклику нельзя ни выделять
				 *       память, ни звать распределитель. Возврат лжи прекращает перебор
				 *
				 * @param callback отклик перебора
				 * @return         число перебранных блоков
				 *
				 * \~english
				 * @brief Method of enumerating the memory held by the application
				 *
				 * @note It reports what has been allocated and not yet freed, NOT about leaks: the
				 *       allocator does not know the application's intentions, and a block living
				 *       until the end of the process may be both a leak and a deliberate cache.
				 *       Calling a find a leak is the prerogative of whoever wrote the code
				 *
				 * @note Only blocks that fell under the profileRate sampling are enumerated: without
				 *       it the enumeration answers with zero, not with empty knowledge
				 *
				 * @note Enumeration runs with the accounting lock held: the callback must neither
				 *       allocate memory nor call the allocator. Returning false stops the enumeration
				 *
				 * @param callback enumeration callback
				 * @return         number of blocks enumerated
				 *
				 * \~
				 */
				static size_t holdings(function <bool (const holding_t &)> callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод опроса записей по освобождённым блокам
				 *
				 * @details Карантин держит освобождённое засеянным и сличает засев при
				 *          изъятии блока: не сохранившийся засев означает запись по
				 *          указателю, освобождённому прежде, - тот самый дефект, ради
				 *          какого карантин и заведён
				 *
				 * @note Находка эта прежде уходила ТОЛЬКО в журнал, и то потоком
				 *       обслуживания: приложение, журнала не заведшее либо потока не
				 *       имеющее, о дефекте своего кода не узнавало вовсе. Опрос
				 *       заведён затем, чтобы находку можно было спросить
				 *
				 * @warning Ловится запись лишь в блок, ЛЕЖАЩИЙ в карантине, и лишь при
				 *          засеве `fill_t::JUNK`: без засева сличать нечего, а блок,
				 *          карантин уже покинувший, вправе быть выдан вновь
				 *
				 * @param culprit адрес первого испорченного блока, либо nullptr
				 * @param offset  смещение первой порчи от начала блока, либо nullptr
				 * @return        число блоков, испорченных записью после освобождения
				 *
				 * \~english
				 * @brief Method of querying writes to freed blocks
				 *
				 * @details Quarantine keeps freed memory seeded and compares the seed when a block is
				 *          taken: a seed that did not survive means a write through a pointer freed
				 *          earlier - the very defect the quarantine was built for
				 *
				 * @note This find previously went ONLY to the log, and then by the service thread:
				 *       an application without a log or without a thread did not learn about its own
				 *       code defect at all. The query was built so that the find could be asked for
				 *
				 * @warning Only a write into a block LYING in quarantine is caught, and only with
				 *          fill_t::JUNK seeding: without seeding there is nothing to compare, and a
				 *          block that has already left quarantine may be allocated anew
				 *
				 * @param culprit address of the first corrupted block, or nullptr
				 * @param offset  offset of the first corruption from the block start, or nullptr
				 * @return        number of blocks corrupted by writes after free
				 *
				 * \~
				 */
				static size_t spoiled(const void ** culprit = nullptr, size_t * offset = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод обращения адреса стека вызовов в имя
				 *
				 * @note Имена функций самой ПРОГРАММЫ видны лишь тогда, когда та связана
				 *       с выносом имён в динамическую таблицу (`-rdynamic` у систем ELF):
				 *       иначе разбор назовёт образ и смещение, но не функцию. Имена
				 *       разделяемых библиотек видны всегда
				 *
				 * @param frame  разбираемый адрес
				 * @param symbol сведения о разобранном адресе
				 * @return       признак разбора адреса
				 *
				 * \~english
				 * @brief Method of resolving a call stack address into a name
				 *
				 * @note Function names of the PROGRAM itself are visible only when it is linked
				 *       with names exported to the dynamic table (-rdynamic on ELF systems):
				 *       otherwise the resolution names the image and offset but not the function.
				 *       Shared library names are always visible
				 *
				 * @param frame  address being resolved
				 * @param symbol information about the resolved address
				 * @return       resolution flag
				 *
				 * \~
				 */
				static bool symbol(const void * frame, symbol_t & symbol) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки отклика на крупное выделение
				 *
				 * @note Отклик зовётся вне выдачи памяти, отдельным потоком: звать его
				 *       изнутри выдачи значило бы уйти в возвратность через ту же выдачу
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback for a large allocation
				 *
				 * @note The callback is called outside memory allocation, by a separate thread:
				 *       calling it from inside allocation would mean going into recursion through
				 *       the same allocation
				 *
				 * @param callback callback function
				 *
				 * \~
				 */
				static void onLarge(function <void (const void *, const size_t)> callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки отклика на достижение потолка кучи
				 *
				 * @param callback функция обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback for reaching the heap limit
				 *
				 * @param callback callback function
				 *
				 * \~
				 */
				static void onLimit(function <void (const size_t)> callback) noexcept;
		} allocator_t;
	};
};
