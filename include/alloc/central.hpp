/**
 * @file central.hpp
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
 * @brief Заголовочный файл центральных списков свободных блоков
 *
 * @section central_decisions Намеренные решения
 *
 * @details <b>Связь свободных блоков лежит в них самих.</b> Отдельная таблица связей
 *          стоила бы памяти на каждый блок и обращения к ней на каждом выделении.
 *          Свободный же блок никем не читается, и первые восемь его байт вольны
 *          держать указатель на следующий: наименьший разряд равен шестнадцати байтам,
 *          места хватает всегда.
 *
 *          <b>Замок у каждого разряда свой, а у кучи общий.</b> Один общий замок
 *          свёл бы весь распределитель к очереди из потоков. Порядок захвата всегда
 *          один - сперва разряд, затем куча, - и обратного порядка нет нигде, оттого
 *          взаимной блокировки не возникает.
 *
 *          <b>Блоки передаются пачками, а не поштучно.</b> Захват замка стоит дороже
 *          самой передачи, оттого поток-локальный кэш забирает и возвращает сразу
 *          связанную цепочку блоков, платя за замок однажды на всю пачку.
 *
 *          <b>Разряд области помечается в самой куче.</b> Освобождение обязано узнать
 *          разряд по одному лишь адресу, и отдельная таблица «адрес - разряд» была бы
 *          вторым учётом того же. Куча же и без того хранит область по каждой её
 *          странице, - метка кладётся туда же.
 *
 * \~english
 * @brief Header file of the central free lists
 *
 * @section central_decisions Deliberate decisions
 *
 * @details <b>The linkage of free blocks lives in the blocks themselves.</b> A separate
 *          link table would cost memory per block and an access on every allocation. A
 *          free block is read by nobody, and its first eight bytes are free to hold a
 *          pointer to the next one: the smallest class is sixteen bytes, there is
 *          always room.
 *
 *          <b>Each class has its own lock, while the heap has a shared one.</b> A single
 *          shared lock would reduce the whole allocator to a queue of threads. The
 *          acquisition order is always the same - first the class, then the heap - and
 *          the reverse order exists nowhere, hence no deadlock arises.
 *
 *          <b>Blocks are transferred in batches, not one by one.</b> Acquiring a lock
 *          costs more than the transfer itself, hence the thread-local cache takes and
 *          returns a linked chain of blocks at once, paying for the lock once per
 *          batch.
 *
 *          <b>The region class is tagged in the heap itself.</b> Free must determine the
 *          class by the address alone, and a separate address-to-class table would be a
 *          second accounting of the same. The heap already stores the region by each of
 *          its pages - the tag goes there.
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
#include <atomic>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "spin.hpp"
#include "pages.hpp"
#include "classes.hpp"

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
	 * Используем стандартное пространство имён
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
		 * @brief Класс центральных списков свободных блоков
		 *
		 * \~english
		 * @brief Central free lists class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Central {
			public:
				/**
				 * \~russian
				 * @brief Наибольшее число блоков в одной передаваемой пачке
				 *
				 * \~english
				 * @brief Maximum number of blocks in a single transmitted burst
				 *
				 * \~
				 */
				static constexpr size_t BATCH = 64;
				/**
				 * \~russian
				 * @brief Окно покоя области перед возвратом куче в миллисекундах
				 *
				 * @details Область, чьи блоки вернулись все, покоится в цепочке разряда:
				 *          работа, строящая и разрушающая одно вместилище раунд за раундом,
				 *          берёт те же области вновь, и немедленная отдача их куче обращала
				 *          бы выборку в перешагивание тысяч пустых областей - замером
				 *          полного набора YAML выборка платила по шесть тысяч переходов
				 *          на пачку, и снятие значения с дерева падало вдвое
				 *
				 * \~english
				 * @brief Region idle window before returning to the heap in milliseconds
				 *
				 * @details A region whose blocks have all returned idles in the class chain:
				 *          work building and destroying the same vessel round after round
				 *          takes the same regions again, and returning them to the heap
				 *          immediately turned fetching into stepping over thousands of empty
				 *          regions - by a full YAML set measurement fetching paid six
				 *          thousand hops per burst, and taking a value from the tree halved
				 *
				 * \~
				 */
				static constexpr uint64_t SPAN_WINDOW = 60000;
				/**
				 * \~russian
				 * @brief Наибольший размер придерживаемой области в страницах кучи
				 *
				 * @details Придержка и рост области НА МЕСТЕ враждебны друг другу: растёт область
				 *          за счёт свободных соседей, а придержанная для кучи занята. Придержка
				 *          без предела роняла «рост перевыдачей» вчетверо, отбирая единственный
				 *          сценарий, где порог держался на всех системах.
				 *
				 *
				 * \~english
				 * @brief Largest retained region size in heap pages
				 *
				 * @details Retention and in-place region growth are hostile to each other: a region
				 *          grows at the expense of free neighbours, while a retained one is occupied
				 *          for the heap. Boundless retention dropped reallocation growth fourfold,
				 *          taking away the one scenario where the threshold held on all systems.
				 *
				 * \~
				 */
				static constexpr size_t KEPT_PAGES = 8;
				/**
				 * \~russian
				 * @brief Потолок придержки областей сверх разрядов по умолчанию в байтах
				 *
				 * @details <b>Нуль: придержка ВЫКЛЮЧЕНА по умолчанию, и это решение замера.</b>
				 *          Она ускоряет круг «выдал - освободил» на областях сверх разрядов в
				 *          полтора раза (73 563 → 113 074 тысяч действий в секунду), но роняет
				 *          рост области перевыдачей вчетверо (18 050 → 3 913): растёт область за
				 *          счёт СВОБОДНЫХ соседей, а придержанная для кучи занята. По порогу
				 *          скорости обмен убыточен - «крупные блоки» не выигрываются и с нею
				 *          (у tcmalloc 115 315), а «рост перевыдачей» был единственным
				 *          сценарием, где порог держался на всех системах.
				 *
				 * @note Приложению, не растящему области перевыдачей, придержка даётся
				 *       настройкой: обмен этот - его выбор, а не наш
				 *
				 *
				 * \~english
				 * @brief Default ceiling of retained regions beyond the size classes in bytes
				 *
				 * @details ZERO: retention is DISABLED by default, and this is a measured decision.
				 *          It accelerates the allocate-free cycle on regions beyond the size classes
				 *          1.5 times (73,563 to 113,074 thousand actions per second) but drops
				 *          in-place region growth fourfold (18,050 to 3,913): a region grows at the
				 *          expense of FREE neighbours, while a retained one is occupied for the heap.
				 *          By the speed threshold the trade-off is unprofitable - large blocks do not
				 *          win even with it (tcmalloc has 115,315), and in-place growth was the one
				 *          scenario where the threshold held on all systems.
				 *
				 * @note For an application not growing regions by reallocation, retention is
				 *       given by a setting: this trade-off is its choice, not ours
				 *
				 * \~
				 */
				static constexpr size_t DEFAULT_KEPT = 0;
			public:
				/**
				 * \~russian
				 * @brief Структура состояния центральных списков
				 *
				 * \~english
				 * @brief Structure of the status of central lists
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ State {
					// Выдано кэшам потоков в байтах
					size_t live;
					// Лежит в центральных списках в байтах
					size_t cached;
					// Нарезано областей под разряды
					size_t regions;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit State() noexcept;
				} state_t;
			private:
				/**
				 * \~russian
				 * @brief Структура центрального списка одного разряда
				 *
				 * \~english
				 * @brief Structure of a central free list of one size class
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ List {
					// Замок разряда
					spin_t lock;
					// Число блоков, выданных кэшам потоков
					size_t live;
					// Число свободных блоков в списке
					size_t count;
					// Число нарезанных под разряд областей
					size_t regions;
					// Голова цепочки областей разряда
					Pages::span_t * top;
					// Хвост цепочки областей разряда
					Pages::span_t * tail;
					// Число возвратов пачек в разряд
					size_t returns;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit List() noexcept;
				} list_t;
				/**
				 * \~russian
				 * @brief Структура записи придержанной области сверх разрядов
				 *
				 * @details Кладётся в САМУ придержанную область: та свободна, а держать под
				 *          неё отдельную память значило бы просить память у распределителя
				 *          изнутри его же освобождени.
				 *
				 * \~english
				 * @brief Structure of a retained region record beyond the size classes
				 *
				 * @details Placed into the RETAINED region itself: the region is free, and holding
				 *          separate memory for it would mean asking the allocator for memory from
				 *          inside its own free.
				 *
				 * \~
				 */
				typedef struct Kept {
					// Размер области в страницах кучи
					size_t pages;
					// Следующая придержанная область того же размера
					struct Kept * next;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param pages размер области в страницах кучи
					 * @param next  следующая придержанная область того же размера
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param pages size of the region in heap pages
					 * @param next  next held region of the same size
					 *
					 * \~
					 */
					constexpr Kept(const size_t pages, struct Kept * next) noexcept :
					 pages(pages), next(next) {}
				} kept_t;
			private:
				// Замок страничной кучи
				spin_t _heap;
				// Замок придержки, отдельный от замка кучи
				spin_t _reserve;
				// Центральные списки по разрядам
				list_t _lists[classes_t::LIMIT];
			private:
				// Объём придержанного в байтах (читается без замка на быстрых путях, оттого атомарен)
				atomic_size_t _keptBytes;
				// Потолок придержки в байтах (читается без замка на быстрых путях, оттого атомарен)
				atomic_size_t _keptCeiling;
			private:
				/**
				 * \~russian
				 * @brief Придержанные области по числу страниц
				 *
				 * @details Придержка освобождённых областей сверх разрядов.
				 *
				 * @note Ниже границы разрядов выдачу держит кэш потока, выше размера куска -
				 *       придержка слоя крупных выдач, а МЕЖДУ ними всякое действие ходило в
				 *       страничную кучу: поиск области, изъятие из списка, дробление, пометка
				 *       всех её страниц - и всё это под замком кучи. Счётчик показывал ровно один
				 *       вызов `Pages::alloc` и один `Pages::mark` на каждую выдачу
				 *
				 * @note Придержанная область куче НЕ возвращается: она остаётся выданной и ждёт
				 *       следующего запроса того же размера. Отдаётся - по просьбе отдачи памяти
				 *       либо сверх потолка, заданного настройкой; нулевой потолок возвращает
				 *       прежнее поведение дословно
				 *
				 * \~english
				 * @brief Retained regions by page count
				 *
				 * @details Retention of released regions beyond the size classes.
				 *
				 * @note Below the class boundary allocation is held by the thread cache, above the
				 *       chunk size by the huge allocation layer retention, and BETWEEN them every
				 *       action went to the page heap: finding a region, taking it from the list,
				 *       splitting, marking all its pages - and all under the heap lock. The counter
				 *       showed exactly one Pages::alloc and one Pages::mark call per allocation
				 *
				 * @note A retained region is NOT returned to the heap: it stays allocated and waits
				 *       for the next request of the same size. It is returned on request of memory
				 *
				 * \~
				 */
				kept_t * _kept[Pages::PAGES + 1];
			private:
				// Страничная куча
				pages_t * _pages;
				// Разряды размеров
				classes_t * _classes;
			private:
				/**
				 * \~russian
				 * @brief Метод нарезки новой области под разряд
				 *
				 * @note Зовётся из-под замка разряда и захватывает замок кучи сам
				 *
				 * @param index номер разряда
				 * @return      признак нарезки области
				 *
				 * \~english
				 * @brief Method of carving a new region for a class
				 *
				 * @note Called under the heap lock: the region is taken from the page heap and split
				 *       into blocks of the class size
				 *
				 * @param index class index
				 * @return      sign of successful operation
				 *
				 * \~
				 */
				bool carve(const size_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи куче покоящихся областей разряда
				 *
				 * \~english
				 * @brief Method of returning idling regions of a class to the heap
				 *
				 * \~
				 */
				size_t shed(const size_t index, const uint64_t now, const bool all) noexcept;
				/**
				 * \~russian
				 * @brief Метод переноса области в хвост цепочки разряда
				 *
				 * \~english
				 * @brief Method of moving a region to the tail of the class chain
				 *
				 * \~
				 */
				void tail(const size_t index, Pages::span_t * span) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения центральных списков
				 *
				 * @param pages   страничная куча
				 * @param classes разряды размеров
				 * @return        признак заведения списков
				 *
				 * \~english
				 * @brief Method of initializing the central free lists
				 *
				 * @param pages   page heap
				 * @param classes size classes
				 * @return        sign of successful initialization
				 *
				 * \~
				 */
				bool init(pages_t * pages, classes_t * classes) noexcept;
				/**
				 * \~russian
				 * @brief Метод сброса центральных списков
				 *
				 * @note Память кучи не отдаёт: её отдаёт сама куча при разрушении
				 *
				 * \~english
				 * @brief Method of resetting the central free lists
				 *
				 * @note Returns all blocks to the page heap and clears the lists
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод изъятия пачки блоков разряда
				 *
				 * @param index номер разряда
				 * @param head  голова изъятой цепочки блоков
				 * @param tail  хвост изъятой цепочки блоков
				 * @param count требуемое число блоков
				 * @return      действительно изъятое число блоков
				 *
				 * \~english
				 * @brief Method of fetching a batch of blocks of a class
				 *
				 * @param index class index
				 * @param head  head of the fetched block chain
				 * @param tail  tail of the fetched block chain
				 * @param count requested number of blocks
				 * @return      number of blocks actually fetched
				 *
				 * \~
				 */
				size_t fetch(const size_t index, void ** head, void ** tail, const size_t count) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата пачки блоков разряда
				 *
				 * @param index номер разряда
				 * @param head  голова возвращаемой цепочки блоков
				 * @param tail  хвост возвращаемой цепочки блоков
				 * @param count число возвращаемых блоков
				 *
				 * \~english
				 * @brief Method of returning a batch of blocks of a class
				 *
				 * @param index class index
				 * @param head  head of the returned block chain
				 * @param tail  tail of the returned block chain
				 * @param count number of blocks being returned
				 *
				 * \~
				 */
				void back(const size_t index, void * head, void * tail, const size_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи памяти сверх разрядов
				 *
				 * @param size   требуемый размер в байтах
				 * @param served действительно выданный размер в байтах, либо nullptr
				 * @return       адрес выданной памяти либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating memory beyond the size classes
				 *
				 * @param size   required size in bytes
				 * @param served actually allocated size in bytes, or nullptr
				 * @return       address of the allocated memory or nullptr
				 *
				 * \~
				 */
				void * alloc(const size_t size, size_t * served = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод расширения выданной сверх разрядов области на месте
				 *
				 * @note Удаётся не всегда: соседняя область вправе быть занята либо мала.
				 *       Отказ здесь не ошибка - звавший переносит содержимое обычным путём
				 *
				 * @param addr адрес начала расширяемой области
				 * @param size требуемый размер в байтах
				 * @return     признак состоявшегося расширения
				 *
				 * \~english
				 * @brief Method of expanding an allocated region in place
				 *
				 * @note Growth goes at the expense of FREE neighbours: a retained region is
				 *       occupied for the heap and cannot be grown into. Refusal here is not
				 *       an error - the caller moves the content by ordinary means
				 *
				 * @param addr address of the start of the region being expanded
				 * @param size required size in bytes
				 * @return     sign of the expansion having taken place
				 *
				 * \~
				 */
				bool expand(void * addr, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата памяти, выданной сверх разрядов
				 *
				 * @param addr адрес возвращаемой памяти
				 * @param now  текущее время в миллисекундах
				 * @return     признак возврата памяти
				 *
				 * \~english
				 * @brief Method of returning memory allocated beyond the size classes
				 *
				 * @param addr address of the memory being returned
				 * @param now  current time in milliseconds
				 * @return     sign of the memory having been returned
				 *
				 * \~
				 */
				bool free(void * addr, const uint64_t now, const bool keeping = true) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод быстрого опознания блока разряда
				 *
				 * @details Отвечает номером разряда одним чтением метки страницы, не
				 *          трогая ни указателей страниц, ни учётной записи области.
				 *
				 * @note Отказ означает лишь «быстрым путём не вышло»: разбор идёт полным
				 *       путём через `owner`
				 *
				 * @param addr  разбираемый адрес
				 * @param index номер разряда, которому принадлежит адрес
				 * @param hint  место хранения подсказки поиска
				 * @return      признак опознанного блока разряда
				 *
				 * \~english
				 * @brief Method of quickly identifying a size class block
				 *
				 * @details Answers with the class number by a single read of the page tag,
				 *          touching neither page pointers nor the region record.
				 *
				 * @note Refusal means only that the quick path did not work: resolution
				 *       goes the full way through `owner`
				 *
				 * @param addr  address being resolved
				 * @param index class number the address belongs to
				 * @param hint  hint storage location for the lookup
				 * @return      sign of an identified class block
				 *
				 * \~
				 */
				__AWH_INLINE__ bool sorted(const void * addr, size_t * index, void ** hint = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения разряда, которому принадлежит адрес
				 *
				 * @param addr  разбираемый адрес
				 * @param index номер разряда, либо LIMIT если память выдана сверх разрядов
				 * @param begin адрес начала области, которой принадлежит адрес
				 * @param size  размер области в байтах
				 * @return      признак того, что адрес выдан нами
				 *
				 * \~english
				 * @brief Method of determining the class an address belongs to
				 *
				 * @param addr  address being resolved
				 * @param index class number, or LIMIT if the memory was allocated beyond the
				 *              size classes
				 * @param begin start address of the region the address belongs to
				 * @param size  region size in bytes
				 * @return      sign of the address having been allocated by us
				 *
				 * \~
				 */
				__AWH_INLINE__ bool owner(const void * addr, size_t * index, void ** begin, size_t * size, void ** hint = nullptr) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отдачи системе свободной памяти
				 *
				 * @param now текущее время в миллисекундах
				 * @param all отдавать всё, не глядя на отсрочку
				 * @return    объём отданной системе памяти в байтах
				 *
				 * \~english
				 * @brief Method of returning free memory to the system
				 *
				 * @param now current time in milliseconds
				 * @param all return everything regardless of the delay
				 * @return    amount of memory returned to the system in bytes
				 *
				 * \~
				 */
				size_t purge(const uint64_t now, const bool all) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания порядка отдачи памяти системе
				 *
				 * @param delay отсрочка в миллисекундах: -1 - не отдавать вовсе
				 * @param block наименьший отдаваемый кусок в байтах
				 *
				 * \~english
				 * @brief Method of setting the memory release policy
				 *
				 * @param delay delay in milliseconds: -1 means never return
				 * @param block smallest returned piece in bytes
				 *
				 * \~
				 */
				void policy(const int64_t delay, const size_t block) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания потолка взятого у источника
				 *
				 * @param limit потолок в байтах: нуль - без потолка
				 *
				 * \~english
				 * @brief Method of setting the ceiling of memory taken from the source
				 *
				 * @param limit ceiling in bytes: zero means no ceiling
				 *
				 * \~
				 */
				void ceiling(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания потолка придержки областей сверх разрядов
				 *
				 * @note Нуль выключает придержку и отдаёт придержанное куче немедленно
				 *
				 * @param limit потолок придержки в байтах
				 *
				 * \~english
				 * @brief Method of setting the ceiling of the span reserve
				 *
				 * @note Retention accelerates the allocate-free cycle 1.5 times but drops in-place
				 *       growth fourfold. Zero disables retention entirely
				 *
				 * @param limit ceiling in bytes: zero means no retention
				 *
				 * \~
				 */
				void keeper(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод занятия области у кучи
				 *
				 * @note Нужен затем, что у систем ELF куча заводится задолго до захвата -
				 *       с первой выдачи стандартной библиотеки; заказанное приложением
				 *       при заведении там применить неоткуда
				 *
				 * @param arena    занимаемая область в байтах
				 * @param confined запрет обращаться к источнику сверх занятого
				 * @return         признак занятия требуемой области
				 *
				 * \~english
				 * @brief Method of occupying an area of the heap
				 *
				 * @note Needed because on ELF systems the heap is established long before capture -
				 *       from the first standard library allocation; what the application ordered at
				 *       establishment has nowhere to be applied there
				 *
				 * @param arena    area to occupy in bytes
				 * @param confined ban on going to the source beyond what is occupied
				 * @return         flag of the required area having been occupied
				 *
				 * \~
				 */
				bool occupy(const size_t arena, const bool confined) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод взятия у кучи области с ожиданием изымаемых
				 *
				 * @note Обход отдачи изымает свободные области из списков на время
				 *       обращения к системе, и под заданным потолком кучи выдача упирается
				 *       в пустоту там, где память есть - она изъята и вот-вот вернётся.
				 *       Замерено на OpenBSD: восемь нехваток на восьми потоках, и ВСЕ
				 *       восемь пришлись ровно на изъятые области. Оттого отказ при
				 *       ненулевом числе изъятых - повод подождать, а не отвечать отказом
				 *
				 * @param pages требуемое число страниц кучи
				 * @return      адрес выданной области либо nullptr
				 *
				 * \~english
				 * @brief Method of taking a span from the heap, awaiting withheld ones
				 *
				 * @note The return sweep withdraws free regions from the lists for the duration of
				 *       the system call, and under the set heap ceiling allocation hits emptiness
				 *       where memory exists - it is withdrawn and about to return. Measured on
				 *       OpenBSD: eight shortages on eight threads, and ALL eight fell exactly on
				 *       withdrawn regions. Hence refusal with a non-zero number of withdrawn is
				 *       reason to wait, not to answer with refusal
				 *
				 * @param pages required number of heap pages
				 * @return      address of the taken region or nullptr
				 *
				 * \~
				 */
				void * take(const size_t pages, size_t * served = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод взятия области у придержки
				 *
				 * @param pages требуемое число страниц кучи
				 * @return      адрес придержанной области либо nullptr
				 *
				 * \~english
				 * @brief Method of taking a span from the reserve
				 *
				 * @param pages required number of heap pages
				 * @return      address of the retained region or nullptr
				 *
				 * \~
				 */
				void * recall(const size_t pages) noexcept;
				/**
				 * \~russian
				 * @brief Метод придержки освобождённой области
				 *
				 * @note Отвечает отказом, когда придерживать не следует: потолок исчерпан,
				 *       область слишком велика либо придержка выключена вовсе. Отказ не
				 *       ошибка - область идёт куче обычным путём
				 *
				 * @param addr  адрес освобождаемой области
				 * @param pages размер области в страницах кучи
				 * @return      признак придержки области
				 *
				 * \~english
				 * @brief Method of reserving a freed span
				 *
				 * @note Answers with refusal when retention is not advisable: the ceiling is
				 *       exhausted, the region is too large, or retention is disabled entirely.
				 *       Refusal is not an error - the region goes to the heap by ordinary means
				 *
				 * @param addr  address of the region being freed
				 * @param pages region size in heap pages
				 * @return      sign of the region having been retained
				 *
				 * \~
				 */
				bool keep(void * addr, const size_t pages) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи придержанных областей куче
				 *
				 * @param all отдавать всё, не глядя на потолок
				 * @return    объём отданного куче в байтах
				 *
				 * \~english
				 * @brief Method of returning the reserved spans to the heap
				 *
				 * @param all return everything, ignoring the ceiling
				 * @return    volume returned to the heap in bytes
				 *
				 * \~
				 */
				size_t unkeep(const bool all) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения упёртости кучи в потолок
				 *
				 * @return признак упёртости кучи в потолок
				 *
				 * \~english
				 * @brief Method of determining whether the heap has hit its ceiling
				 *
				 * @return sign of the heap having hit its ceiling
				 *
				 * \~
				 */
				bool jammed() noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска области кучи, которой принадлежит адрес
				 *
				 * @note Ходит к куче ПОД ЗАМКОМ, и в том вся суть метода. Прежде разбор
				 *       адреса (`Allocator::resolve`) обращался к куче напрямую, минуя
				 *       замок, - а куча в это же время правила список кусков в `Pages::grow`.
				 *       ThreadSanitizer называл гонку прямо: запись в pages.cpp:457 против
				 *       чтения в pages.cpp:972
				 *
				 * @note Метод зовётся из разбора сбоя обращения, а не из обработчика
				 *       сигнала: замок обработчику брать нельзя
				 *
				 * @param addr  разбираемый адрес
				 * @param begin адрес начала найденной области
				 * @param pages размер найденной области в страницах кучи
				 * @param live  признак выданной наружу области
				 * @return      признак того, что адрес принадлежит куче
				 *
				 * \~english
				 * @brief Method of locating the heap region an address belongs to
				 *
				 * @note Goes to the heap UNDER LOCK, and that is the whole point of the method.
				 *       Previously address resolution (Allocator::resolve) went to the heap directly,
				 *       bypassing the lock, while the heap was editing the chunk list in Pages::grow.
				 *       ThreadSanitizer named the race directly: write in pages.cpp:457 against read
				 *       in pages.cpp:972
				 *
				 * @note The method is called from address resolution on a fault, not from a
				 *       signal handler: a signal handler must not take the lock
				 *
				 * @param addr  address being resolved
				 * @param begin start address of the found region
				 * @param pages size of the found region in heap pages
				 * @param live  sign of the region being issued to the outside
				 * @return      sign of the address belonging to the heap
				 *
				 * \~
				 */
				bool locate(const void * addr, const void ** begin, size_t * pages, bool * live) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения состояния центральных списков
				 *
				 * @return состояние списков
				 *
				 * \~english
				 * @brief Method of getting the central free lists state
				 *
				 * @return lists state
				 *
				 * \~
				 */
				state_t state() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния страничной кучи
				 *
				 * @return состояние кучи
				 *
				 * \~english
				 * @brief Method of getting the page heap state
				 *
				 * @return page heap state
				 *
				 * \~
				 */
				Pages::state_t heap() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата всех замков перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring all locks before forking
				 *
				 * \~
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения всех замков после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing all locks after forking
				 *
				 * \~
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод принудительного освобождения всех замков у потомка
				 *
				 * @note Потомку ветвления достаётся один поток, а замки, захваченные
				 *       прочими потоками родителя, остаются захваченными навсегда
				 *
				 * \~english
				 * @brief Method of forcibly releasing all locks in the forked child
				 *
				 * @note The fork child gets a single thread, while locks held by the parent's
				 *       other threads remain held forever
				 *
				 * \~
				 */
				void adopt() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Central() noexcept;
		} central_t;
	};
};
