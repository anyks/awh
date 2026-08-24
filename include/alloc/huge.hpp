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
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_HUGE__
#define __AWH_ALLOC_HUGE__

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
#include "../sys/global.hpp"

/**
 * @brief Пространство имён фреймворка
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён распределителя памяти
	 *
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс слоя крупных выдач
		 *
		 * \~english
		 * @brief Huge allocation layer class
		 *
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
				// Номер разряда, которым отмечается крупная выдача
				static constexpr size_t INDEX = (awh::alloc::Classes::LIMIT + 1u);
			public:
				/**
				 * \~russian
				 * @brief Состояние слоя крупных выдач
				 *
				 * \~english
				 * @brief State of the huge allocation layer
				 *
				 */
				typedef struct State {
					// Число живых крупных выдач
					size_t live;
					// Взято у источника под крупные выдачи в байтах
					size_t taken;
					// Выдано прикладному коду крупными выдачами в байтах
					size_t given;
					/**
					 * @brief Конструктор
					 *
					 */
					State() noexcept : live(0), taken(0), given(0) {}
				} state_t;
			private:
				/**
				 * \~russian
				 * @brief Учётная запись крупной выдачи
				 *
				 * \~english
				 * @brief Huge allocation record
				 *
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
					// Следующая повторно используемая запись
					struct Record * spare;
					/**
					 * Признак блока, укрытого от снимков памяти
					 *
					 * Такой блок обязан затираться при освобождении: договор укрытой
					 * выдачи обещает это наравне с самим укрытием
					 */
					bool hidden;
					/**
					 * @brief Конструктор
					 *
					 */
					Record() noexcept :
					 block(nullptr), base(nullptr), span(0), size(0), spare(nullptr), hidden(false) {}
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
				 * Общий список кусков памяти под учётные записи
				 *
				 * Связь лежит в первых восьми байтах самого куска, оттого списку не нужно
				 * собственной памяти. Устроен он так же, как у страничной кучи, и по той
				 * же причине: без него куски эти отдать нечем, и снятие захвата оставляло
				 * бы их занятыми у системы
				 */
				// Общий список кусков памяти под учётные записи
				void * _metaChunks;
				// Список повторно используемых учётных записей
				record_t * _spare;
				/**
				 * Список придержанных областей, готовых к повторной выдаче
				 *
				 * Записи в нём несут `base` и `span`, а `block` и `size` не значат
				 * ничего: область снята с учёта и прикладному коду не принадлежит
				 */
				// Список придержанных областей
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
				 * @brief Метод выдачи памяти под учётную запись
				 *
				 * @return адрес выданной памяти либо nullptr
				 *
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
				 */
				bool retain(uint8_t * base, const size_t span) noexcept;
				/**
				 * @brief Метод перестроения таблицы поиска
				 *
				 * @param length требуемая длина таблицы в местах
				 * @return       признак перестроения таблицы
				 *
				 */
				bool rehash(const size_t length) noexcept;
				/**
				 * @brief Метод внесения записи в таблицу поиска
				 *
				 * @param record вносимая запись
				 * @return       признак внесения записи
				 *
				 */
				bool enroll(record_t * record) noexcept;
				/**
				 * @brief Метод поиска записи по адресу блока
				 *
				 * @param addr  разбираемый адрес
				 * @param exact признак поиска ровно по началу блока
				 * @return      найденная запись либо nullptr
				 *
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
				 */
				bool init(source_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия слоя крупных выдач
				 *
				 * \~english
				 * @brief Method of tearing down the huge allocation layer
				 *
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
				 * @param size   required size in bytes
				 * @param hidden flag of the concealment having taken place
				 * @return       address of the allocated block or nullptr
				 *
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
				 * @return   size requested by the application code or zero
				 *
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
				 */
				bool resolve(const void * addr, const void ** begin, size_t * size, ptrdiff_t * offset) const noexcept;
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
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления работы слоя после ветвления
				 *
				 * \~english
				 * @brief Method of resuming the layer after a fork
				 *
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения слоя в порядок в потомке ветвления
				 *
				 * \~english
				 * @brief Method of bringing the layer in order in a fork child
				 *
				 */
				void adopt() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Huge() noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Huge() noexcept {}
		} huge_t;
	};
};

#endif // __AWH_ALLOC_HUGE__
