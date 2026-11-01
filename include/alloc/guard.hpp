/**
 * @file guard.hpp
 * @date 2026-08-21
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
 * @brief Заголовочный файл заслонов и карантина — выдача части блоков между
 *        закрытыми страницами и удержание освобождённого от повторной выдачи
 *
 * @section guard_decisions Намеренные решения
 *
 * @details <b>Заслонённые блоки берутся прямо у источника, а не у страничной кучи.</b>
 *          Куча нарезает куски по четыре мегабайта и раздаёт из них логические
 *          страницы по восемь килобайт; заслон же ставится страницей системы, и
 *          запрет посреди куска накрыл бы соседние выдачи, к дефекту касательства не
 *          имеющие.
 *
 *          <b>Заслона два, а не один.</b> Один задний заслон ловит переполнение, но
 *          недобор перед началом блока оставляет незамеченным - его пришлось бы ловить
 *          сличением образца при освобождении, то есть много позже точки дефекта.
 *          Два заслона ловят оба края немедленно, ценою двух страниц на блок; выдача
 *          эта и без того выборочная.
 *
 *          <b>Блок прижимается к заднему заслону.</b> Переполнение на единственный
 *          байт обязано валить обращение, а не попадать в пропуск выравнивания.
 *          Пропуск неизбежен, но лежит он перед блоком, где недобор ловится передним
 *          заслоном.
 *
 *          <b>Освобождённый заслонённый блок закрывается целиком, а не отдаётся.</b>
 *          Отдай мы его системе сразу - адрес достался бы следующей выдаче, и
 *          обращение по висячему указателю прошло бы молча. Закрытая же область валит
 *          обращение в точке дефекта и разбором адреса опознаётся освобождённой.
 *
 *          <b>Удержание закрытых областей ограничено ДВУМЯ границами.</b> Числом - ради
 *          места под адреса у мелких блоков, и объёмом - ради самой памяти у крупных.
 *          Одной лишь границы по числу мало: под заслонами выдаётся до мегабайта, и
 *          четыре тысячи областей держали бы до четырёх гигабайт. Замерено на кругах
 *          нагрузки: 201 мегабайт удержанного при НУЛЕВОМ занятом, причём рост встал не
 *          оттого, что память вернулась, а оттого, что счёт областей упёрся в предел.
 *
 *          <b>Карантин ведётся кольцом записей, а не связью внутри блоков.</b> Связь
 *          внутри блока заняла бы первые байты, и засев освобождённого проверить было
 *          бы нечем: как раз затёртые связью байты и портит запись по висячему
 *          указателю чаще всего. Кольцо же лежит в стороне и блок не трогает вовсе.
 *
 *          <b>Живость блока разряда известна лишь через карантин.</b> Свободный блок
 *          лежит в списках без всякой отметки, а перебор списков под замками у самого
 *          сбоя ненадёжен вдвойне. Оттого разбор отвечает FREED, когда блок найден в
 *          карантине, и LIVE, когда область жива, а в карантине блока нет. С
 *          выключенным карантином второе не значит ничего - о чём и сказано настройкой.
 *
 * \~english
 * @brief Header file of guard pages and quarantine — issuing part of the blocks
 *        between protected pages and holding freed memory back from reuse
 *
 * @section guard_decisions Deliberate decisions
 *
 * @details <b>Guarded blocks are taken straight from the source, not from the page
 *          heap.</b> The heap carves chunks of four megabytes and hands out logical
 *          pages of eight kilobytes from them; a guard is a system page, and a
 *          prohibition in the middle of a chunk would cover neighbouring allocations
 *          unrelated to the defect.
 *
 *          <b>There are two guards, not one.</b> A single trailing guard catches
 *          overflow but leaves underrun before the block start unnoticed - it would
 *          have to be caught by comparing a pattern on free, that is, much later than
 *          the defect point. Two guards catch both edges immediately, at the cost of
 *          two pages per block; this allocation is selective anyway.
 *
 *          <b>The block is pressed against the trailing guard.</b> Overflow by a
 *          single byte must crash the access, not land in an alignment gap. The gap is
 *          inevitable but lies before the block, where underrun is caught by the
 *          leading guard.
 *
 *          <b>A freed guarded block is closed entirely, not returned.</b> Were we to
 *          return it to the system at once - the address would go to the next
 *          allocation, and access through a dangling pointer would pass silently. A
 *          closed region crashes the access at the defect point and is identified as
 *          freed by address resolution.
 *
 *          <b>Holding closed regions is limited by TWO bounds.</b> By count - for the
 *          sake of address space for small blocks, and by volume - for memory itself
 *          with large ones. A count bound alone is not enough: under guards up to a
 *          megabyte is allocated, and four thousand regions would hold up to four
 *          gigabytes. Measured on load cycles: 201 megabytes held with ZERO occupied,
 *          and the growth stopped not because memory was returned but because the
 *          region count hit its limit.
 *
 *          <b>Quarantine is a ring of records, not linkage inside blocks.</b> Linkage
 *          inside a block would take the first bytes, and the seed of a freed block
 *          could not be checked: it is precisely the bytes overwritten by linkage that
 *          a write through a dangling pointer corrupts most often. The ring lies aside
 *          and does not touch the block at all.
 *
 *          <b>Liveness of a class block is known only through quarantine.</b> A free
 *          block lies in the lists without any mark, and enumerating lists under locks
 *          at the fault itself is doubly unreliable. Hence resolution answers FREED
 *          when the block is found in quarantine, and LIVE when the region is alive
 *          and the block is not in quarantine. With quarantine disabled the latter
 *          means nothing - as stated in the setting.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 * \~
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "source.hpp"
#include "../sys/macro/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_GUARD_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_GUARD_INLINE inline __attribute__((always_inline))
#endif

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
		 * @brief Класс заслонов
		 *
		 * \~english
		 * @brief Guard pages class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Guard {
			public:
				// Начальная длина таблицы поиска заслонённого блока в местах
				static constexpr size_t TABLE = 256;
				// Наибольший размер, выдаваемый под заслонами, в байтах
				static constexpr size_t MAXIMUM = (1u * 1024u * 1024u);
				/**
				 * Наибольшее число закрытых областей, удерживаемых от отдачи
				 *
				 * Закрытая область не отдаётся системе затем, чтобы обращение по
				 * освобождённому адресу валило, а не проходило молча. Удерживать их без
				 * счёта нельзя: у долгой работы кончится не память, так место под адреса
				 */
				static constexpr size_t SEALED = 4096;
				/**
				 * Наибольший ОБЪЁМ закрытых областей, удерживаемых от отдачи
				 *
				 * Счёт областей объёма не ограничивает: под заслонами выдаётся до
				 * мегабайта, и четыре тысячи закрытых областей держали бы до четырёх
				 * гигабайт - закрытых, но взятых у системы и однажды тронутых. Замерено
				 * щупом на кругах настройка-нагрузка-выключение: на блоках до
				 * шестнадцати килобайт заслоны держали 201 мегабайт при НУЛЕВОМ занятом,
				 * и рост встал не оттого, что память вернулась, а оттого, что счёт
				 * областей упёрся в предел
				 *
				 * Границы нужны обе: счёт бережёт место под адреса у мелких блоков,
				 * объём - саму память у крупных
				 */
				static constexpr size_t RETAINED = (64u * 1024u * 1024u);
			public:
				/**
				 * \~russian
				 * @brief Состояние заслонов
				 *
				 * \~english
				 * @brief Guard pages state
				 *
				 * \~
				 */
				typedef struct State {
					// Число живых заслонённых блоков
					size_t live;
					// Число закрытых заслонённых блоков, ожидающих отдачи
					size_t sealed;
					// Взято у источника под заслонённые блоки в байтах
					size_t taken;
					// Выдано прикладному коду под заслонами в байтах
					size_t given;
					/**
					 * \~russian
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					State() noexcept;
				} state_t;
			private:
				/**
				 * \~russian
				 * @brief Учётная запись заслонённого блока
				 *
				 *
				 * \~english
				 * @brief Accounting record of a guarded block
				 *
				 * \~
				 */
				typedef struct Record {
					// Адрес выданного прикладному коду блока
					uint8_t * block;
					// Адрес начала взятой у источника области
					uint8_t * base;
					// Размер взятой у источника области в байтах
					size_t span;
					// Затребованный прикладным кодом размер в байтах
					size_t size;
					// Признак закрытой области, ожидающей отдачи
					bool sealed;
					// Следующая закрытая область в очереди на отдачу
					struct Record * queue;
					/**
					 * \~russian
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Record() noexcept;
				} record_t;
			private:
				// Источник страниц
				source_t * _source;
				// Замок заслонов
				spin_t _lock;
				/**
				 * Таблица поиска заслонённого блока
				 *
				 * Открытая адресация с удвоением: заслонённых блоков немного по самому
				 * устройству выборки, а искать их приходится на каждом освобождении -
				 * перебор списка для того негоден
				 */
				// Таблица поиска заслонённого блока по адресу
				record_t ** _table;
				// Длина таблицы поиска в местах
				size_t _length;
				// Размер взятой под таблицу поиска области в байтах
				size_t _region;
				// Число записей, внесённых в таблицу
				size_t _enrolled;
				// Число мест таблицы, помеченных снесёнными
				size_t _buried;
				// Текущий кусок памяти под учётные записи
				uint8_t * _meta;
				// Остаток текущего куска памяти под учётные записи
				size_t _metaLeft;
				// Общий список кусков памяти под учётные записи
				void * _metaChunks;
				// Список повторно используемых учётных записей
				record_t * _spare;
				// Начало очереди закрытых областей, старейшая из них
				record_t * _oldest;
				// Конец очереди закрытых областей, младшая из них
				record_t * _newest;
				// Объём закрытых областей, удерживаемых от отдачи, в байтах
				size_t _sealedBytes;
				// Доля выборки: одна выдача из скольких
				std::atomic <size_t> _rate;
				/**
				 * Признак источника, не умеющего закрывать страницы
				 *
				 * Заслоны стоят на закрытых страницах, и с таким источником они
				 * невозможны ВОВСЕ: выборка выключается насовсем, иначе каждая выдача
				 * брала бы у источника область впустую.
				 *
				 * Признак отдельный, а не выключенная доля выборки, и в этом весь смысл.
				 * Прежде выключение писало нуль прямо в долю - а правку настроек долю
				 * ЗАДАЁТ, и та оживляла выборку заново. Насовсем выключение при этом не
				 * было ни разу: каждый вызов настроек стоил ещё одной впустую взятой
				 * области. Источник же за жизнь распределителя не меняется, и приговор
				 * ему выносится один раз
				 */
				// Признак источника, не умеющего закрывать страницы
				std::atomic <bool> _unable;
				// Счётчик выдач для выборки
				std::atomic <size_t> _counter;
				// Состояние заслонов
				state_t _state;
			private:
				/**
				 * Метка снесённого места таблицы поиска
				 *
				 * Открытая адресация не терпит пустоты на месте снесённой записи: перебор
				 * мест идёт подряд и остановился бы на ней, не дойдя до записи, попавшей
				 * дальше по перебору. Оттого снесённое место метится особым знаком -
				 * занятым для перебора и свободным для записи
				 */
				static record_t * const _tomb;
			private:
				/**
				 * \~russian
				 * @brief Метод выдачи памяти под учётную запись
				 *
				 * @return адрес выданной памяти либо nullptr
				 *
				 *
				 * \~english
				 * @brief Method of allocating memory for an accounting record
				 *
				 * @return address of the allocated memory or nullptr
				 *
				 * \~
				 */
				void * meta() noexcept;
				/**
				 * \~russian
				 * @brief Метод перестроения таблицы поиска
				 *
				 * @param length требуемая длина таблицы в местах
				 * @return       признак перестроения таблицы
				 *
				 *
				 * \~english
				 * @brief Method of rebuilding the lookup table
				 *
				 * @param length required table length in slots
				 * @return       table rebuild flag
				 *
				 * \~
				 */
				bool rehash(const size_t length) noexcept;
				/**
				 * \~russian
				 * @brief Метод внесения записи в таблицу поиска
				 *
				 * @param record вносимая запись
				 * @return       признак внесения записи
				 *
				 *
				 * \~english
				 * @brief Method of enrolling a record into the lookup table
				 *
				 * @param record the record being enrolled
				 * @return       enrollment flag
				 *
				 * \~
				 */
				bool enroll(record_t * record) noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска записи по адресу блока
				 *
				 * @param addr  разбираемый адрес
				 * @param exact признак поиска ровно по началу блока
				 * @return      найденная запись либо nullptr
				 *
				 *
				 * \~english
				 * @brief Method of finding a record by block address
				 *
				 * @param addr  address being resolved
				 * @param exact flag of searching exactly by block start
				 * @return      found record or nullptr
				 *
				 * \~
				 */
				record_t * lookup(const void * addr, const bool exact) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения заслонов
				 *
				 * @param source источник страниц
				 * @return       признак заведения заслонов
				 *
				 * \~english
				 * @brief Method of initializing the guard pages
				 *
				 * @param source page source
				 * @return       sign of successful initialization
				 *
				 * \~
				 */
				bool init(source_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия заслонов
				 *
				 * \~english
				 * @brief Method of shutting down the guard pages
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения надобности заслона очередной выдаче
				 *
				 * @note Метод меняет счётчик выборки и оттого зовётся ровно однажды на
				 *       выдачу; повторный вопрос о том же запросе ответит иначе
				 *
				 * @param size требуемый размер в байтах
				 * @return     признак надобности заслона
				 *
				 * \~english
				 * @brief Method of determining whether the next allocation needs a guard
				 *
				 * @note The method changes the sampling counter and is therefore called exactly once
				 *       per allocation; asking again about the same request will answer differently
				 *
				 * @param size required size in bytes
				 * @return     sign of a guard being needed
				 *
				 * \~
				 */
				/**
				 * \~russian
				 * @brief Метод выборки выдачи под заслоны
				 *
				 * @note Холодный хвост `wanted`: сюда приходят лишь при включённых заслонах
				 *
				 * @param size требуемый размер в байтах
				 * @return     признак того, что выборка взяла эту выдачу
				 *
				 * \~english
				 * @brief Method of sampling an allocation for guarding
				 *
				 * @note Cold tail of wanted: reached only when guards are enabled
				 *
				 * @param size required size in bytes
				 * @return     sign of the sampling having taken this allocation
				 *
				 * \~
				 */
				bool sampled(const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения надобности заслона очередной выдаче
				 *
				 * @details Заслоны выключены у подавляющего большинства приложений, а вопрос
				 *          стоит на пути каждой выдачи: работы здесь одно нестрогое чтение,
				 *          а вызов стоил дороже работы. При включённых заслонах решение
				 *          принимает выборка холодного пути.
				 *
				 * @param size требуемый размер в байтах
				 * @return     признак надобности заслона
				 *
				 * \~english
				 * @brief Method of determining whether the next allocation needs a guard
				 *
				 * @details Guards are off in the vast majority of applications, and the
				 *          question stands on the path of every allocation: the work here
				 *          is a single relaxed read, while the call cost more than the work.
				 *          With guards enabled, the decision is made by the cold path sampling.
				 *
				 * @param size required size in bytes
				 * @return     sign of the guard being needed
				 *
				 * \~
				 */
				AWH_GUARD_INLINE bool wanted(const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи заслонённого блока
				 *
				 * @param size требуемый размер в байтах
				 * @return     адрес выданного блока либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a guarded block
				 *
				 * @param size required size in bytes
				 * @return     address of the allocated block or nullptr
				 *
				 * \~
				 */
				void * alloc(const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения заслонённого блока
				 *
				 * @note Область при этом закрывается целиком и системе не отдаётся:
				 *       обращение по освобождённому адресу обязано валить, а не проходить
				 *
				 * @note Признак принадлежности отдаётся ОТДЕЛЬНО от размера: повторное
				 *       освобождение заслонённого блока даёт нулевой размер при вполне
				 *       нашем адресе, и не различай мы этих случаев - такой указатель ушёл
				 *       бы прежнему распределителю, то есть чужой куче
				 *
				 * @param ptr  адрес освобождаемого блока
				 * @param mine признак принадлежности адреса заслонам
				 * @return     затребованный размер блока в байтах, либо нуль
				 *
				 * \~english
				 * @brief Method of freeing a guarded block
				 *
				 * @note The region is closed wholly and is not returned to the system: an access
				 *       to the freed address is bound to crash, not to pass
				 *
				 * @note The ownership flag is returned SEPARATELY from the size: a repeated free
				 *       of a guarded block yields a zero size with a perfectly our address, and
				 *       without telling these cases apart such a pointer would go to the previous
				 *       allocator, that is, to a foreign heap
				 *
				 * @param ptr  address of the block being freed
				 * @param mine sign of the address belonging to the guards
				 * @return     requested block size in bytes, or zero
				 *
				 * \~
				 */
				size_t free(void * ptr, bool * mine) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения принадлежности адреса заслонам
				 *
				 * @param ptr  разбираемый адрес
				 * @param size размер блока, если он определён
				 * @return     признак принадлежности адреса заслонам
				 *
				 * \~english
				 * @brief Method of determining whether the address belongs to the guards
				 *
				 * @param ptr  address being resolved
				 * @param size block size in bytes, if it has been determined
				 * @return     sign of the address belonging to the guards
				 *
				 * \~
				 */
				bool owner(const void * ptr, size_t * size) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора адреса обращения
				 *
				 * @param addr   разбираемый адрес
				 * @param begin  адрес начала блока, если он определён
				 * @param size   размер блока, если он определён
				 * @param offset смещение разбираемого адреса от начала блока
				 * @param sealed признак освобождённого блока
				 * @return       признак принадлежности адреса заслонённой области
				 *
				 * \~english
				 * @brief Method of resolving an access address
				 *
				 * @param addr   address being resolved
				 * @param begin  block start address, if it has been determined
				 * @param size   block size in bytes, if it has been determined
				 * @param offset offset of the address from the block start
				 * @param sealed sign of the block having been freed
				 * @return       sign of the address belonging to a guarded region
				 *
				 * \~
				 */
				bool resolve(const void * addr, const void ** begin, size_t * size, ptrdiff_t * offset, bool * sealed) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания доли выборки
				 *
				 * @param rate одна выдача из скольких: нуль - заслоны выключены
				 *
				 * \~english
				 * @brief Method of setting the sampling rate
				 *
				 * @param rate one allocation out of how many: zero means guards off
				 *
				 * \~
				 */
				void rate(const size_t rate) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния заслонов
				 *
				 * @return состояние заслонов
				 *
				 * \~english
				 * @brief Method of getting the guard pages state
				 *
				 * @return guard pages state
				 *
				 * \~
				 */
				state_t state() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата замка перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring the lock before process forking
				 *
				 * \~
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод отпускания замка после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing the lock after process forking
				 *
				 * \~
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения замка в порядок у потомка ветвления
				 *
				 * \~english
				 * @brief Method of resetting the lock in the forked child
				 *
				 * \~
				 */
				void adopt() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Guard() noexcept;
		} guard_t;
		/**
		 * \~russian
		 * @brief Класс карантина освобождённой памяти
		 *
		 * \~english
		 * @brief Freed memory quarantine class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Quarantine {
			public:
				// Знак засева освобождаемой памяти
				static constexpr uint8_t JUNK = 0xDE;
				// Наименьшее число мест в кольце карантина
				static constexpr size_t MINIMUM = 256;
				// Наибольшее число мест в кольце карантина
				static constexpr size_t LIMIT = 65536;
				// Доля объёма карантина, приходящаяся на одно место кольца, в байтах
				static constexpr size_t SHARE = 512;
			public:
				/**
				 * \~russian
				 * @brief Состояние карантина
				 *
				 * \~english
				 * @brief Quarantine state
				 *
				 * \~
				 */
				typedef struct State {
					// Число блоков, удерживаемых карантином
					size_t held;
					// Объём удерживаемой карантином памяти в байтах
					size_t bytes;
					// Число блоков, прошедших карантин за время работы
					size_t passed;
					// Число блоков, испорченных записью после освобождения
					size_t spoiled;
					// Адрес блока, испорченного записью после освобождения, первого по счёту
					const void * culprit;
					// Смещение первой порчи от начала испорченного блока
					size_t offset;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					State() noexcept;
				} state_t;
			private:
				/**
				 * \~russian
				 * @brief Место кольца карантина
				 *
				 *
				 * \~english
				 * @brief Quarantine ring slot
				 *
				 * \~
				 */
				typedef struct Slot {
					// Адрес удерживаемого блока
					void * block;
					// Размер удерживаемого блока в байтах
					size_t size;
					// Номер разряда, которому принадлежит блок
					size_t index;
				} slot_t;
			private:
				// Источник страниц
				source_t * _source;
				// Замок карантина
				spin_t _lock;
				// Кольцо удерживаемых блоков
				slot_t * _ring;
				// Размер кольца в местах
				size_t _length;
				// Размер взятой под кольцо области в байтах
				size_t _region;
				// Место записи очередного блока
				size_t _head;
				// Место изъятия старейшего блока
				size_t _tail;
				// Число удерживаемых блоков
				size_t _held;
				// Объём удерживаемой памяти в байтах
				size_t _bytes;
				// Потолок объёма карантина в байтах
				size_t _limit;
				// Признак засева удерживаемой памяти
				bool _junk;
				// Состояние карантина
				state_t _state;
			private:
				/**
				 * \~russian
				 * @brief Метод сличения засева удерживаемого блока
				 *
				 * @param block разбираемый блок
				 * @param size  размер блока в байтах
				 * @return      признак сохранности засева
				 *
				 *
				 * \~english
				 * @brief Method of comparing the seed of a held block
				 *
				 * @param block block being resolved
				 * @param size  block size in bytes
				 * @return      sign of the seed having survived
				 *
				 * \~
				 */
				bool intact(const void * block, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения карантина
				 *
				 * @param source источник страниц
				 * @param limit  потолок объёма карантина в байтах: нуль - карантин выключен
				 * @return       признак заведения карантина
				 *
				 * \~english
				 * @brief Method of initializing the quarantine
				 *
				 * @param source page source
				 * @param limit  quarantine volume ceiling in bytes: zero means no quarantine
				 * @return       sign of successful initialization
				 *
				 * \~
				 */
				bool init(source_t * source, const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия карантина
				 *
				 * @note Удерживаемые блоки при этом НЕ возвращаются: возвращать их
				 *       некуда - слои, которым они принадлежат, к этому мигу уже сняты
				 *
				 * \~english
				 * @brief Method of shutting down the quarantine
				 *
				 * @note Held blocks are NOT returned: there is nowhere to return them - the layers
				 *       they belong to have already been torn down by this instant
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удержания освобождённого блока
				 *
				 * @note Вытеснением карантин НЕ занимается: принявший блок волен
				 *       переполниться, и звать `release` до опустошения обязан тот, кто
				 *       знает, куда возвращать вытесненное
				 *
				 * @param ptr   адрес удерживаемого блока
				 * @param size  размер удерживаемого блока в байтах
				 * @param index номер разряда, которому принадлежит блок
				 * @return      признак принятия блока карантином
				 *
				 * \~english
				 * @brief Method of holding a freed block
				 *
				 * @note The quarantine does NOT evict: the receiver is free to overflow, and
				 *       `release` must be called before filling up by whoever knows where to
				 *       return the evicted block
				 *
				 * @param ptr   address of the block being held
				 * @param size  size of the held block in bytes
				 * @param index class index the block belongs to
				 * @return      sign of the block having been taken into quarantine
				 *
				 * \~
				 */
				bool hold(void * ptr, const size_t size, const size_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод изъятия старейшего удерживаемого блока
				 *
				 * @note Блок изымается лишь когда карантин переполнен объёмом либо
				 *       числом мест; пока он вмещается - метод отвечает пустотой
				 *
				 * @param size  размер изъятого блока в байтах
				 * @param index номер разряда изъятого блока
				 * @return      адрес изъятого блока либо nullptr
				 *
				 * \~english
				 * @brief Method of taking out the oldest held block
				 *
				 * @note A block is taken out only when the quarantine overflows by volume or by
				 *       the number of places; while it fits, the method answers with emptiness
				 *
				 * @param size  size of the taken-out block in bytes
				 * @param index class index of the taken-out block
				 * @return      address of the taken-out block or nullptr
				 *
				 * \~
				 */
				void * release(size_t * size, size_t * index) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения удержания адреса карантином
				 *
				 * @param addr  разбираемый адрес
				 * @param begin адрес начала блока, если он определён
				 * @param size  размер блока, если он определён
				 * @return      признак удержания адреса карантином
				 *
				 * \~english
				 * @brief Method of determining whether the address is held by the quarantine
				 *
				 * @param addr  address being resolved
				 * @param begin block start address, if defined
				 * @param size  block size in bytes, if defined
				 * @return      sign of the address being held by the quarantine
				 *
				 * \~
				 */
				bool held(const void * addr, const void ** begin, size_t * size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания потолка объёма карантина
				 *
				 * @param limit потолок объёма в байтах: нуль - карантин выключен
				 *
				 * \~english
				 * @brief Method of setting the quarantine volume ceiling
				 *
				 * @param limit volume ceiling in bytes: zero disables the quarantine
				 *
				 * \~
				 */
				void limit(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания засева удерживаемой памяти
				 *
				 * @param junk признак засева
				 *
				 * \~english
				 * @brief Method of setting the junk filling of held memory
				 *
				 * @param junk sign of junk filling
				 *
				 * \~
				 */
				void junk(const bool junk) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния карантина
				 *
				 * @return состояние карантина
				 *
				 * \~english
				 * @brief Method of getting the quarantine state
				 *
				 * @return quarantine state
				 *
				 * \~
				 */
				state_t state() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата замка перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring the lock before process forking
				 *
				 * \~
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод отпускания замка после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing the lock after process forking
				 *
				 * \~
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения замка в порядок у потомка ветвления
				 *
				 * \~english
				 * @brief Method of resetting the lock in the forked child
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
				Quarantine() noexcept;
		} quarantine_t;
	};
};
