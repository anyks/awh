/**
 * @file huge.hpp
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
 * @brief Заголовочный файл слоя крупных выдач — обслуживание запросов, не
 *        помещающихся в кусок страничной кучи
 *
 * @section huge_decisions Намеренные решения
 *
 * @details <b>Крупная выдача берётся прямо у источника, минуя страничную кучу.</b>
 *          Куча нарезает адресное пространство кусками по четыре мегабайта и области
 *          свои через границу куска не ведёт: область там описывается местом в
 *          пределах одного куска, и запрос сверх куска ей не выразить вовсе. Оттого
 *          такие запросы обслуживает слой над кучей, обращаясь к источнику напрямую -
 *          ровно как о том сказано у @c Central::alloc.
 *
 *          <b>Учёт ведётся таблицей в стороне, а не заголовком перед блоком.</b>
 *          Заголовок сдвинул бы выданный адрес с границы страницы, а крупная выдача
 *          затем и берётся у источника, чтобы лечь на страницу: того требует и
 *          выравнивающая выдача, и отдача части области системе. Таблица же лежит
 *          отдельно и выданного блока не трогает.
 *
 *          <b>Освобождённая область придерживается под потолком, а не отдаётся
 *          немедленно.</b> Прежде она отдавалась системе тут же: держать свободными
 *          куски по многу мегабайт впрок значило бы копить у себя ровно то, ради чего
 *          к нам и приходят с жалобой на расход. Довод этот верен по расходу, но он
 *          считал ценой отдачи ВЫЗОВ СИСТЕМЫ, тогда как настоящая цена лежит в
 *          страницах: область, взятая заново, приходит чистой, и за каждую её страницу
 *          платится отказом страницы при первом обращении. Замер разделил цену натрое
 *          и показал на пяти мегабайтах - выдача 1.2 мкс, касание страниц 306 мкс
 *          против 9.4 у системного распределителя. Обрыв выходил ровно на границе
 *          слоя: до четырёх мегабайт мы шли вровень с системным, за нею - в семь раз
 *          медленнее. Замечен он был не нами, а соседней сессией на кодеках, где
 *          изображал просадку кодека впятеро.
 *
 *          Оттого область теперь придерживается готовой к повторной выдаче, а
 *          отдаётся - по просьбе `Allocator::purge()` либо сверх потолка
 *          `options.hugeCache`. Потолок этот и есть плата за скорость, заданная
 *          числом: нуль возвращает прежнее поведение дословно. Укрытая область не
 *          придерживается никогда - договор её обещает отдачу системе.
 *
 *          <b>Своё место в разрядах у слоя нет, но номер он имеет.</b> Освобождение
 *          узнаёт слой по номеру разряда, отданному разбором размера: спрашивать слой
 *          на каждом освобождении сверх разрядов значило бы брать его замок там, где
 *          крупных выдач нет вовсе.
 *
 * \~english
 * @brief Header file of the huge allocation layer — serving requests that do not fit
 *        into a page heap chunk
 *
 * @section huge_decisions Deliberate decisions
 *
 * @details <b>A huge allocation is taken straight from the source, bypassing the page
 *          heap.</b> The heap carves address space into four-megabyte chunks and does
 *          not carry its regions across a chunk boundary: a region there is described
 *          by a place within a single chunk, and a request beyond a chunk cannot be
 *          expressed at all. Hence such requests are served by a layer above the heap,
 *          addressing the source directly — exactly as stated at @c Central::alloc.
 *
 *          <b>Accounting is kept in a side table, not in a header before the block.</b>
 *          A header would shift the allocated address off the page boundary, and a huge
 *          allocation is taken from the source precisely to land on a page: that is
 *          required both by aligned allocation and by returning part of a region to the
 *          system. The table lies separately and does not touch the allocated block.
 *
 *          <b>A released region is retained under a ceiling, not returned
 *          immediately.</b> Previously it was returned to the system at once: holding
 *          free chunks of many megabytes in reserve would mean accumulating exactly
 *          what people come to us complaining about in terms of consumption. The
 *          argument was valid for consumption, but it counted the price of returning as
 *          a SYSTEM CALL, whereas the real price lies in pages: a region taken anew
 *          comes clean, and for each of its pages a page fault is paid on first access.
 *          The measurement split the price threefold and showed at five megabytes —
 *          allocation 1.2 μs, page touching 306 μs versus 9.4 for the system
 *          allocator. The cliff was exactly at the layer boundary: up to four megabytes
 *          we matched the system allocator, beyond it we were seven times slower. It
 *          was noticed not by us but by a neighbouring session on codecs, where it
 *          looked like a fivefold codec slowdown.
 *
 *          Hence the region is now retained ready for reuse, and returned — on request
 *          of `Allocator::purge()` or beyond the `options.hugeCache` ceiling. That
 *          ceiling is the price for speed, given as a number: zero restores the
 *          previous behaviour verbatim. A concealed region is never retained — the
 *          contract promises its return to the system.
 *
 *          <b>The layer has no place among the size classes, but it has a number.</b>
 *          Free recognises the layer by the class number handed over by size parsing:
 *          asking the layer on every free beyond the classes would mean taking its lock
 *          where there are no huge allocations at all.
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
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "source.hpp"
#include "classes.hpp"
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
		 * @brief Класс слоя крупных выдач
		 *
		 * \~english
		 * @brief Huge allocation layer class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Huge {
			public:
				// Начальная длина таблицы поиска крупной выдачи в местах
				static constexpr size_t TABLE = 64;
				/**
				 * Номер разряда, которым отмечается крупная выдача
				 *
				 * Разрядов у распределителя не больше @c Classes::LIMIT, и номер этот с
				 * настоящим разрядом не сходится ни при какой настройке. Заслоны
				 * отмечаются самим @c Classes::LIMIT, оттого здесь взято следующее число
				 */
				static constexpr size_t INDEX = (awh::alloc::Classes::LIMIT + 1u);
			public:
				/**
				 * \~russian
				 * @brief Состояние слоя крупных выдач
				 *
				 * \~english
				 * @brief State of the huge allocation layer
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ State {
					// Число живых крупных выдач
					size_t live;
					// Взято у источника под крупные выдачи в байтах
					size_t taken;
					// Выдано прикладному коду крупными выдачами в байтах
					size_t given;
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
				 * @brief Учётная запись крупной выдачи
				 *
				 * \~english
				 * @brief Huge allocation record
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Record {
					/**
					 * \~russian
					 * @brief Признак закрытого блока
					 *
					 * @details Счёта одного мало: выданный блок ДОСТУПЕН, а держателей раскрытия
					 *          у него нет, и по одному лишь счёту первое закрытие читалось бы как
					 *          лишнее. Признак отвечает «каков блок сейчас», а счёт - «сколько рук его держат».
					 *
					 * \~english
					 * @brief Sign of a sealed block
					 *
					 * @details A count alone is not enough: an issued block is ACCESSIBLE,
					 *          and there are no holders of an unsealing at it, and by one
					 *          only count a first sealing would be read as an extra one.
					 *          A sign answers `what is a block now`, and a count - `how many hands hold it`.
					 *
					 * \~
					 */
					bool sealed;
					/**
					 * \~russian
					 * @brief Признак блока, укрытого от снимков памяти
					 *
					 * @details Такой блок обязан затираться при освобождении: договор укрытой
					 *          выдачи обещает это наравне с самим укрытием.
					 *
					 * \~english
					 * @brief Sign of a block hidden from the memory snapshots
					 *
					 * @details Such a block is obliged to be wiped on a release: a contract of a hidden
					 *          issue promises this on a par with the hiding itself.
					 *
					 * \~
					 */
					bool hidden;
					// Размер взятой у источника области в байтах
					size_t span;
					// Затребованный прикладным кодом размер в байтах
					size_t size;
					/**
					 * \~russian
					 * @brief Счёт раскрытий закрытого блока
					 *
					 * @details Не признак, а именно СЧЁТ: при общем хранилище на процесс два
					 *          потока раскрывают одну область, и тот, кто закончит первым,
					 *          закрыл бы память под ногами второго - отказ доступа посреди
					 *          чужой работы, то есть падение, и притом плавающее. Область
					 *          запирается на нуле, и только на нуле.
					 *
					 * \~english
					 * @brief A count of the unsealings of a sealed block
					 *
					 * @details Not a sign, but exactly a COUNT: at a common storage per process two
					 *          threads unseal one region, and the one that finishes first
					 *          would seal the memory under the feet of the second - an access failure in the middle
					 *          of an alien work, that is a crash, and a floating one. A region
					 *          is sealed at a zero, and only at a zero.
					 *
					 * \~
					 */
					uint32_t opened;
					// Адрес начала взятой у источника области
					uint8_t * base;
					// Адрес выданного прикладному коду блока
					uint8_t * block;
					// Следующая повторно используемая запись
					struct Record * spare;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Record() noexcept;
				} record_t;
			private:
				// Источник страниц
				source_t * _source;
				// Замок слоя крупных выдач
				spin_t _lock;
				// Таблица поиска крупной выдачи по адресу
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
				/**
				 * \~russian
				 * @brief Общий список кусков памяти под учётные записи
				 *
				 * @details Связь лежит в первых восьми байтах самого куска, оттого списку не нужно
				 *          собственной памяти. Устроен он так же, как у страничной кучи, и по той
				 *          же причине: без него куски эти отдать нечем, и снятие захвата оставляло
				 *          бы их занятыми у системы.
				 *
				 * \~english
				 * @brief A common list of the memory chunks under the account records
				 *
				 * @details A linkage lies in the first eight bytes of a chunk itself, therefore a list needs no
				 *          own memory. It is arranged the same as at a page heap, and by the
				 *          same reason: without it there is nothing to give these chunks back, and a removal of a capture would leave
				 *          them occupied at the system.
				 *
				 * \~
				 */
				void * _metaChunks;
				// Список повторно используемых учётных записей
				record_t * _spare;
				/**
				 * \~russian
				 * @brief Список придержанных областей, готовых к повторной выдаче
				 *
				 * @details Записи в нём несут `base` и `span`, а `block` и `size` не значат
				 *          ничего: область снята с учёта и прикладному коду не принадлежит.
				 *
				 * \~english
				 * @brief A list of the held regions, ready for a repeated issue
				 *
				 * @details The records in it carry `base` and `span`, while `block` and `size` mean
				 *          nothing: a region is taken off the account and does not belong to an application code.
				 *
				 * \~
				 */
				record_t * _cached;
				// Придержано областей в байтах
				size_t _cachedBytes;
				// Потолок придержанного в байтах: нуль - не придерживать вовсе
				size_t _ceiling;
				// Состояние слоя крупных выдач
				state_t _state;
			private:
				// Метка снесённого места таблицы поиска
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
				 * @brief Метод повторной выдачи придержанной области
				 *
				 * @param size      требуемый размер в байтах
				 * @param alignment требуемое выравнивание в байтах
				 * @param actual    действительный размер выданной области
				 * @return          адрес области либо nullptr
				 *
				 * \~english
				 * @brief Method of reusing a retained region
				 *
				 * @param size      required size in bytes
				 * @param alignment required alignment in bytes
				 * @param actual    actual size of the allocated region
				 * @return          address of the region or nullptr
				 *
				 * \~
				 */
				uint8_t * recycle(const size_t size, const size_t alignment, size_t & actual) noexcept;
				/**
				 * \~russian
				 * @brief Метод придержки освобождённой области
				 *
				 * @param base адрес начала области
				 * @param span размер области в байтах
				 * @return     признак придержки области
				 *
				 * \~english
				 * @brief Method of retaining a released region
				 *
				 * @param base start address of the region
				 * @param span region size in bytes
				 * @return     retention flag
				 *
				 * \~
				 */
				bool retain(uint8_t * base, const size_t span) noexcept;
				/**
				 * \~russian
				 * @brief Метод перестроения таблицы поиска
				 *
				 * @param length требуемая длина таблицы в местах
				 * @return       признак перестроения таблицы
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
				 * @brief Метод заведения слоя крупных выдач
				 *
				 * @param source источник страниц
				 * @return       признак заведения слоя
				 *
				 * \~english
				 * @brief Method of initializing the huge allocation layer
				 *
				 * @param source page source
				 * @return       layer initialization flag
				 *
				 * \~
				 */
				bool init(source_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия слоя крупных выдач
				 *
				 * \~english
				 * @brief Method of tearing down the huge allocation layer
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания потолка придержанного
				 *
				 * @note Придержка нужна ради цены СТРАНИЦ, а не ради цены выдачи: сама
				 *       выдача крупной области стоит около микросекунды, тогда как
				 *       первое обращение к её страницам после свежего отображения -
				 *       три сотни. Область, взятая у системы заново, приходит чистой, и
				 *       за каждую её страницу платится отказом страницы
				 *
				 * @param bytes потолок придержанного в байтах: нуль - не придерживать
				 *
				 * \~english
				 * @brief Method of setting the retention ceiling
				 *
				 * @note Retention is needed for the price of PAGES, not the price of allocation: a
				 *       huge region allocation costs about a microsecond, while first access to
				 *       its pages after fresh mapping costs three hundred. A region taken from
				 *       the system anew comes clean, and a page fault is paid for each of its
				 *       pages
				 *
				 * @param bytes retention ceiling in bytes: zero means no retention
				 *
				 * \~
				 */
				void ceiling(const size_t bytes) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи придержанных областей системе
				 *
				 * @return объём отданного системе в байтах
				 *
				 * \~english
				 * @brief Method of releasing the retained regions to the system
				 *
				 * @return volume returned to the system in bytes
				 *
				 * \~
				 */
				size_t drain() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения обслуживаемости размера слоем
				 *
				 * @param size требуемый размер в байтах
				 * @return     признак обслуживаемости размера слоем
				 *
				 * \~english
				 * @brief Method of determining whether the size is served by the layer
				 *
				 * @param size required size in bytes
				 * @return     flag of the size being served by the layer
				 *
				 * \~
				 */
				static bool wanted(const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод крупной выдачи памяти
				 *
				 * @param size      требуемый размер в байтах
				 * @param alignment требуемое выравнивание в байтах, нуль - страничное
				 * @return          адрес выданной памяти либо nullptr
				 *
				 * \~english
				 * @brief Method of huge memory allocation
				 *
				 * @param size      required size in bytes
				 * @param alignment required alignment in bytes, zero means page alignment
				 * @return          address of the allocated memory or nullptr
				 *
				 * \~
				 */
				void * alloc(const size_t size, const size_t alignment) noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи блока, укрытого от снимков памяти
				 *
				 * @note Блок этот затирается при освобождении и не попадает в снимок
				 *       памяти при падении - там, где система такое умеет. Ответ
				 *       `hidden` говорит, состоялось ли укрытие: молчаливое понижение
				 *       обещания хуже честного отказа
				 *
				 * @param size   требуемый размер в байтах
				 * @param hidden признак состоявшегося укрытия
				 * @param wire   признак необходимости запрета уходить в подкачку
				 * @param wired  признак состоявшегося запрета либо nullptr
				 * @return       адрес выданного блока либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a block concealed from memory dumps
				 *
				 * @note This block is wiped on free and does not appear in a crash memory dump
				 *       where the system supports it. The `hidden` answer tells whether the
				 *       concealment happened: a silent downgrade of the promise is worse than
				 *       an honest refusal
				 *
				 * @param size   required size in bytes
				 * @param hidden flag of the concealment having taken place
				 * @param wire   flag of needing to prevent swapping out
				 * @param wired  flag of the swap prevention having taken place or nullptr
				 * @return       address of the allocated block or nullptr
				 *
				 * \~
				 */
				void * conceal(const size_t size, bool & hidden, const bool wire = false, bool * wired = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения крупной выдачи
				 *
				 * @param ptr адрес освобождаемой памяти
				 * @return    затребованный прикладным кодом размер блока либо нуль
				 *
				 * \~english
				 * @brief Method of freeing a huge allocation
				 *
				 * @param ptr address of the memory being freed
				 * @return    size requested by the application code or zero
				 *
				 * \~
				 */
				size_t free(void * ptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод опознания крупной выдачи по адресу её начала
				 *
				 * @param ptr  разбираемый адрес
				 * @param size размер блока в байтах
				 * @return     признак принадлежности адреса слою
				 *
				 * \~english
				 * @brief Method of identifying a huge allocation by its start address
				 *
				 * @param ptr  address being resolved
				 * @param size block size in bytes
				 * @return     flag of the address belonging to the layer
				 *
				 * \~
				 */
				bool owner(const void * ptr, size_t * size) const noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора адреса, лежащего внутри крупной выдачи
				 *
				 * @param addr   разбираемый адрес
				 * @param begin  адрес начала блока
				 * @param size   размер блока в байтах
				 * @param offset смещение разбираемого адреса от начала блока
				 * @return       признак принадлежности адреса слою
				 *
				 * \~english
				 * @brief Method of resolving an address lying inside a huge allocation
				 *
				 * @param addr   address being resolved
				 * @param begin  block start address
				 * @param size   block size in bytes
				 * @param offset offset of the address being resolved from the block start
				 * @return       flag of the address belonging to the layer
				 *
				 * \~
				 */
				bool resolve(const void * addr, const void ** begin, size_t * size, ptrdiff_t * offset) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод смены доступности выданного блока
				 *
				 * @note Раскрытия СЧИТАЮТСЯ: область запирается на нуле, и только на нуле.
				 *       Иначе поток, закончивший первым, закрыл бы память под ногами
				 *       второго — отказ доступа посреди чужой работы
				 *
				 * @note Границы блока правятся до страниц: смена доступности идёт
				 *       страницами, и невыровненное начало система вправе отвергнуть
				 *
				 * @param addr   адрес выданного блока
				 * @param opened признак открываемого блока
				 * @return       признак выполнения операции
				 *
				 * \~english
				 * @brief Method of changing the accessibility of an allocated block
				 *
				 * @note Reveals are COUNTED: the area is locked at zero, and only at zero.
				 *       Otherwise the thread that finished first would close the memory from
				 *       under the second — a denial of access amid someone else's work
				 *
				 * @note Block boundaries are rounded to pages: accessibility changes go by
				 *       pages, and the system may reject an unaligned start
				 *
				 * @param addr   address of the allocated block
				 * @param opened flag of the block being opened
				 * @return       sign of successful operation
				 *
				 * \~
				 */
				bool shield(const void * addr, const bool opened) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения состояния слоя крупных выдач
				 *
				 * @return состояние слоя крупных выдач
				 *
				 * \~english
				 * @brief Method of getting the state of the huge allocation layer
				 *
				 * @return state of the huge allocation layer
				 *
				 * \~
				 */
				state_t state() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод подготовки слоя к ветвлению процесса
				 *
				 * \~english
				 * @brief Method of preparing the layer for a process fork
				 *
				 * \~
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления работы слоя после ветвления
				 *
				 * \~english
				 * @brief Method of resuming the layer after a fork
				 *
				 * \~
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения слоя в порядок в потомке ветвления
				 *
				 * \~english
				 * @brief Method of bringing the layer in order in a fork child
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
				explicit Huge() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Huge() noexcept = default;
		} huge_t;
	};
};
