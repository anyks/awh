/**
 * @file profile.hpp
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
 * @brief Заголовочный файл учёта мест выдачи памяти — запоминание стека вызовов у
 *        части выданных блоков и перебор того, что прикладной код удерживает
 *
 * @section profile_decisions Намеренные решения
 *
 * @details <b>Речь об УДЕРЖИВАЕМОМ, а не об утечках.</b> Распределитель не знает
 *          намерений прикладного кода: блок, живущий до конца процесса, может быть и
 *          утечкой, и намеренным кэшем. Оттого модуль отвечает на вопрос «что и откуда
 *          выдано и до сих пор не освобождено», а называть это утечкой волен лишь тот,
 *          кто писал код.
 *
 *          <b>Учёт выборочный, и по умолчанию выключен вовсе.</b> Место выдачи стоит
 *          съёма стека при выдаче и поиска по таблице при КАЖДОМ освобождении - в том
 *          числе у блоков, под учёт не попавших. Плата эта оправдана при разборе, а не
 *          в бою, и потому берётся лишь по требованию.
 *
 *          <b>Стек хранится в самой записи, а не ссылкой.</b> Ссылка означала бы
 *          второе хранилище со своим сроком жизни и своим замком. Запись же с местом
 *          под стек внутри выдаётся и возвращается одним движением, а стоит она сотни
 *          байт на блок - при выборке одного блока из тысячи это ничто.
 *
 *          <b>Отклик перебора зовётся БЕЗ замка, записи снимаются пачками под ним.</b>
 *          Отклик пишет приложение, а перебор затевается ради поиска течи - ради
 *          доклада, - и доклад просит памяти; выдача же при взведённой выборке идёт во
 *          внесение записи, а то берёт этот самый замок, и держи мы его на весь перебор
 *          - поток встал бы намертво на своём же замке (проверено `sample/alloc/debug.cpp`).
 *          Оттого пачка записей снимается под замком и КОПИРУЕТ стек в себя, а отклик
 *          читает её уже вне замка: отпустив замок, права на запись мы теряем - сосед
 *          волен снести её и выдать заново. Подробности - в теле `walk`.
 *
 * \~english
 * @brief Header file of allocation site accounting — remembering the call stack of a
 *        fraction of allocated blocks and enumerating what the application holds
 *
 * @section profile_decisions Deliberate decisions
 *
 * @details <b>This is about what is HELD, not about leaks.</b> The allocator does not
 *          know the application's intentions: a block living until the end of the
 *          process may be both a leak and a deliberate cache. Hence the module answers
 *          the question "what has been allocated from where and not yet freed", and
 *          calling it a leak is the prerogative of whoever wrote the code.
 *
 *          <b>Accounting is selective and disabled by default.</b> An allocation site
 *          costs a stack capture on allocation and a table lookup on EVERY free —
 *          including blocks not taken into account. This price is justified during
 *          analysis, not in production, and is therefore taken only on demand.
 *
 *          <b>The stack is stored in the record itself, not by reference.</b> A
 *          reference would mean a second storage with its own lifetime and its own
 *          lock. A record with room for the stack inside is allocated and returned in
 *          one motion, and it costs hundreds of bytes per block — when sampling one
 *          block in a thousand this is nothing.
 *
 *          <b>The enumeration callback is called WITHOUT the lock; records are taken
 *          in batches under it.</b> The callback is written by the application, and
 *          enumeration is started to find a leak — for reporting — and the report asks
 *          for memory; while allocation with sampling enabled goes to record insertion,
 *          and that takes this very lock, and were we to hold it for the entire
 *          enumeration the thread would deadlock on its own lock (verified in
 *          `sample/alloc/debug.cpp`). Hence a batch of records is taken under the lock
 *          and COPIES the stack into itself, while the callback reads it already
 *          outside the lock: having released the lock we lose the right to the record —
 *          a neighbour is free to remove it and allocate anew. Details are in the body
 *          of `walk`.
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
#include "trace.hpp"
#include "source.hpp"
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
		 * @brief Сведения об удерживаемом прикладным кодом блоке
		 *
		 * @note Массив стека указывает на память самой записи: годен он лишь внутри
		 *       отклика перебора, а нужен дольше - копировать
		 *
		 * \~english
		 * @brief Information about a block held by the application
		 *
		 *
		 * @note The frames array points to the record's own memory: it is valid only
		 *       inside the enumeration callback; copy it if needed longer
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Holding {
			// Адрес удерживаемого блока
			const void * block;
			// Затребованный размер блока в байтах
			size_t size;
			// Отметка времени выдачи в миллисекундах
			uint64_t stamp;
			// Стек вызовов места выдачи
			const void * const * frames;
			// Глубина снятого стека вызовов
			size_t depth;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Holding() noexcept;
		} holding_t;
		/**
		 * \~russian
		 * @brief Класс учёта мест выдачи памяти
		 *
		 * \~english
		 * @brief Allocation site accounting class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Profile {
			public:
				// Начальная длина таблицы учёта в местах
				static constexpr size_t TABLE = 1024;
				// Наибольшее число учитываемых блоков
				static constexpr size_t LIMIT = (1024u * 1024u);
			public:
				/**
				 * \~russian
				 * @brief Состояние учёта мест выдачи
				 *
				 * \~english
				 * @brief Allocation site accounting state
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ State {
					// Число учитываемых живых блоков
					size_t live;
					// Объём учитываемых живых блоков в байтах
					size_t bytes;
					// Число блоков, взятых под учёт за время работы
					size_t enrolled;
					// Число блоков, не взятых под учёт из-за нехватки места
					size_t dropped;
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
				/**
				 * \~russian
				 * @brief Отклик перебора удерживаемых блоков
				 *
				 * @note Возврат лжи прекращает перебор: он идёт с взятым замком, и дать
				 *       ему уйти раньше времени важнее удобства
				 *
				 * \~english
				 * @brief Callback for enumerating held blocks
				 *
				 *
				 * @note Returning false stops the enumeration: it runs with the lock held,
				 *       and letting it finish early matters more than convenience
				 *
				 * \~
				 */
				typedef bool (* walker_t)(const holding_t & holding, void * context);
			private:
				/**
				 * \~russian
				 * @brief Учётная запись выданного блока
				 *
				 * \~english
				 * @brief Accounting record of an allocated block
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Record {
					// Адрес выданного блока
					const void * block;
					// Затребованный размер блока в байтах
					size_t size;
					// Отметка времени выдачи в миллисекундах
					uint64_t stamp;
					// Глубина снятого стека вызовов
					size_t depth;
					// Следующая запись в списке повторно используемых
					struct Record * spare;
					// Стек вызовов места выдачи
					const void * frames[Trace::DEPTH];
				} record_t;
			private:
				// Источник страниц
				source_t * _source;
				// Съём стека вызовов
				trace_t * _trace;
				// Замок учёта
				spin_t _lock;
				// Таблица учёта выданных блоков
				record_t ** _table;
				// Длина таблицы учёта в местах
				size_t _length;
				// Размер взятой под таблицу учёта области в байтах
				size_t _region;
				// Число записей, внесённых в таблицу
				size_t _enrolled;
				// Число мест таблицы, помеченных снесёнными
				size_t _buried;
				// Признак идущего перебора: при нём таблица не перестраивается (курсор перебора неподвижен)
				bool _walking;
				// Текущий кусок памяти под учётные записи
				uint8_t * _meta;
				// Остаток текущего куска памяти под учётные записи
				size_t _metaLeft;
				// Общий список кусков памяти под учётные записи
				void * _metaChunks;
				// Список повторно используемых учётных записей
				record_t * _spare;
				// Доля выборки: одна выдача из скольких
				std::atomic <size_t> _rate;
				// Счётчик выдач для выборки
				std::atomic <size_t> _counter;
				// Число учитываемых живых блоков
				std::atomic <size_t> _live;
				// Состояние учёта мест выдачи
				state_t _state;
			private:
				/**
				 * \~russian
				 * @brief Метка снесённого места таблицы учёта
				 *
				 * \~english
				 * @brief Tombstone marker of a removed accounting table slot
				 *
				 * \~
				 */
				static record_t * const _tomb;
			private:
				/**
				 * \~russian
				 * @brief Метод выдачи памяти под учётную запись
				 *
				 * @return адрес выданной памяти либо nullptr
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
				 * @brief Метод перестроения таблицы учёта
				 *
				 * @param length требуемая длина таблицы в местах
				 * @return       признак перестроения таблицы
				 *
				 *
				 * \~english
				 * @brief Method of rebuilding the accounting table
				 *
				 * @param length required table length in slots
				 * @return       table rebuild flag
				 *
				 * \~
				 */
				bool rehash(const size_t length) noexcept;
				/**
				 * \~russian
				 * @brief Метод внесения записи в таблицу учёта
				 *
				 * @param record вносимая запись
				 * @return       признак внесения записи
				 *
				 *
				 * \~english
				 * @brief Method of inserting a record into the accounting table
				 *
				 * @param record the record being inserted
				 * @return       insertion flag
				 *
				 * \~
				 */
				bool insert(record_t * record) noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска места записи в таблице учёта
				 *
				 * @param block разбираемый адрес блока
				 * @return      место записи в таблице либо длина таблицы
				 *
				 *
				 * \~english
				 * @brief Method of finding a record slot in the accounting table
				 *
				 * @param block address of the block being resolved
				 * @return      record slot in the table or the table length
				 *
				 * \~
				 */
				size_t seek(const void * block) const noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия записи с учёта по месту в таблице
				 *
				 * @note Зовётся ПОД замком учёта: правит таблицу, счётчики и состояние
				 *
				 * @note Заведён общим куском для снятия блока с учёта и для вытеснения
				 *       прежней записи того же адреса: два места правят одни и те же
				 *       шесть величин, и разойдись они хоть в одной - счёт живых блоков
				 *       поехал бы молча
				 *
				 * @param index место снимаемой записи в таблице
				 *
				 * \~english
				 * @brief Method of removing a record from the table by its slot
				 *
				 *
				 * @note Called UNDER the accounting lock: edits the table, counters and state
				 *
				 * @note Built as a common piece for removing a block from the accounting and for
				 *       evicting a previous record of the same address: two places edit the same six
				 *       values, and had they diverged in even one the live block count would drift
				 *       silently
				 *
				 * @param index slot of the record being removed in the table
				 *
				 * \~
				 */
				void discard(const size_t index) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения учёта мест выдачи
				 *
				 * @param source источник страниц
				 * @param trace  съём стека вызовов
				 * @return       признак заведения учёта
				 *
				 * \~english
				 * @brief Method of initializing the allocation site accounting
				 *
				 *
				 * @param source page source
				 * @param trace  call stack capture
				 * @return       accounting initialization flag
				 *
				 * \~
				 */
				bool init(source_t * source, trace_t * trace) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия учёта мест выдачи
				 *
				 * \~english
				 * @brief Method of shutting down the allocation site accounting
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выборки выдачи под учёт места
				 *
				 * @note Холодный хвост `wanted`: сюда приходят лишь при включённом учёте
				 *
				 * @return признак того, что выборка взяла эту выдачу
				 *
				 * \~english
				 * @brief Method of sampling an allocation for tracking
				 *
				 *
				 * @note Cold tail of `wanted`: reached only when accounting is enabled
				 *
				 * @return sign of the sampling having taken this allocation
				 *
				 * \~
				 */
				bool sampled() noexcept;
				/**
				 * \~russian
				 * @brief Метод определения надобности учёта очередной выдачи
				 *
				 * @details Учёт мест выдачи выключен у подавляющего большинства приложений,
				 *          а вопрос стоит на пути каждой выдачи: работы здесь одно нестрогое
				 *          чтение, а вызов стоил дороже работы. При включённом учёте решение
				 *          принимает выборка холодного пути.
				 *
				 * @return признак надобности учёта
				 *
				 * \~english
				 * @brief Method of determining whether the next allocation needs tracking
				 *
				 * @details Allocation site tracking is off in the vast majority of applications,
				 *          and the question stands on the path of every allocation: the work
				 *          here is a single relaxed read, while the call cost more than the
				 *          work. With tracking enabled, the decision is made by the cold path
				 *          sampling.
				 *
				 * @return sign of tracking being needed
				 *
				 * \~
				 */
				__AWH_INLINE__ bool wanted() noexcept;
				/**
				 * \~russian
				 * @brief Метод определения ведения учёта хоть каких-то блоков
				 *
				 * @note Нужен затем, чтобы освобождение не искало в таблице, когда учёта
				 *       нет вовсе: поиск этот стоит замка на каждом освобождении
				 *
				 * @return признак наличия учитываемых блоков
				 *
				 * \~english
				 * @brief Method of determining whether any blocks are accounted for
				 *
				 *
				 * @note Needed so that free does not search the table when there is no accounting at
				 *       all: that search costs a lock on every free
				 *
				 * @return sign of tracked blocks being present
				 *
				 * \~
				 */
				__AWH_INLINE__ bool tracking() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод взятия выданного блока под учёт
				 *
				 * @note Стек снимается здесь же: место выдачи известно лишь в этот миг
				 *
				 * @param block адрес выданного блока
				 * @param size  затребованный размер блока в байтах
				 * @param stamp отметка времени выдачи в миллисекундах
				 * @param skip  число ближних уровней стека, какие пропустить
				 * @return      признак взятия блока под учёт
				 *
				 * \~english
				 * @brief Method of taking an allocated block into account
				 *
				 *
				 * @note The stack is captured right here: the allocation site is known only at this
				 *       instant
				 *
				 * @param block address of the allocated block
				 * @param size  requested block size in bytes
				 * @param stamp allocation timestamp in milliseconds
				 * @param skip  number of nearest stack levels to skip
				 * @return      sign of the block being taken into account
				 *
				 * \~
				 */
				bool enroll(const void * block, const size_t size, const uint64_t stamp, const size_t skip) noexcept;
				/**
				 * \~russian
				 * @brief Метод правки размера блока, состоящего под учётом
				 *
				 * @note Нужен там, где блок вырос НА МЕСТЕ: выдачи при этом не было, и
				 *       запись учёта осталась бы с прежним размером. Поиск утечек шёл
				 *       бы тогда по числам меньше настоящих - как раз у тех блоков, что
				 *       растут, то есть у самых подозрительных
				 *
				 * @param block адрес блока
				 * @param size  новый размер блока в байтах
				 * @return      признак состоявшейся правки
				 *
				 * \~english
				 * @brief Method of amending the size of a tracked block
				 *
				 *
				 * @note Needed where a block grew IN PLACE: there was no allocation, and the
				 *       accounting record would keep the previous size. Leak hunting would then run
				 *       on numbers smaller than the real ones — exactly for those blocks that grow,
				 *       that is, the most suspicious ones
				 *
				 * @param block block address
				 * @param size  new block size in bytes
				 * @return      sign of the amendment having taken place
				 *
				 * \~
				 */
				bool amend(const void * block, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия блока с учёта
				 *
				 * @param block адрес освобождаемого блока
				 * @return      признак снятия блока с учёта
				 *
				 * \~english
				 * @brief Method of removing a block from the accounting
				 *
				 * @param block address of the block being freed
				 * @return      sign of the block being removed from the accounting
				 *
				 * \~
				 */
				bool expel(const void * block) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перебора удерживаемых прикладным кодом блоков
				 *
				 * @param callback отклик перебора
				 * @param context  предмет, отдаваемый отклику
				 * @return         число перебранных блоков
				 *
				 * \~english
				 * @brief Method of enumerating the blocks held by the application
				 *
				 * @param callback enumeration callback
				 * @param context  item passed to the callback
				 * @return         number of enumerated blocks
				 *
				 * \~
				 */
				size_t walk(walker_t callback, void * context) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания доли выборки
				 *
				 * @param rate одна выдача из скольких: нуль - учёт выключен
				 *
				 * \~english
				 * @brief Method of setting the sampling rate
				 *
				 * @param rate one allocation out of how many: zero means accounting off
				 *
				 * \~
				 */
				void rate(const size_t rate) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния учёта мест выдачи
				 *
				 * @return состояние учёта мест выдачи
				 *
				 * \~english
				 * @brief Method of getting the allocation site accounting state
				 *
				 * @return state of the allocation site accounting
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
				explicit Profile() noexcept;
		} profile_t;
	};
};
