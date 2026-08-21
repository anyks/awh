/**
 * @file pages.hpp
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
 * @brief Заголовочный файл страничной кучи — выдача и возврат областей, кратных
 *        странице, слияние соседних свободных областей и отложенная отдача памяти
 *        системе
 *
 * @section pages_decisions Намеренные решения
 *
 * @details <b>Своя память под учётные записи, а не контейнеры стандартной библиотеки.</b>
 *          Куча эта заслоняет собою malloc всего процесса, и всякий контейнер внутри
 *          неё ушёл бы за памятью в неё же - то есть в бесконечную возвратность на
 *          первом же выделении. Оттого учётные записи областей берутся у своего
 *          нарезающего распределителя, а тот - прямо у источника страниц. По той же
 *          причине здесь нет ни new, ни исключений.
 *
 *          <b>Куски у источника берутся крупные и выровненные.</b> Область в четыре
 *          мегабайта, выровненная по своему размеру, позволяет по любому адресу внутри
 *          неё найти её начало одной маской, без поиска. Из этого же следует, что
 *          страницы кучи - величина своя (8 КБ), а не то, что зовёт страницей система:
 *          у MS Windows зернистость выдачи 64 КБ, и вести учёт ею значило бы дробить
 *          память вчетверо грубее нужного.
 *
 *          <b>Кусок отдаётся системе целиком либо не отдаётся вовсе.</b> MS Windows
 *          не даёт освободить часть отведённой области: MEM_RELEASE принимает лишь
 *          адрес, полученный от VirtualAlloc. Оттого частями отдаётся только
 *          содержимое (purge), а адреса освобождаются лишь вместе со всем куском.
 *
 *          <b>Слияние идёт только внутри куска.</b> Два соседних по адресу куска могут
 *          оказаться отданы системе порознь, и область, слитая через их границу, при
 *          отдаче распалась бы надвое. Оттого граница куска для слияния непроницаема.
 *
 * \~english
 * @brief Header file of the page heap — allocating and returning page-multiple regions,
 *        coalescing adjacent free regions and deferred return of memory to the system
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_PAGES__
#define __AWH_ALLOC_PAGES__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "source.hpp"
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
		 * @brief Класс страничной кучи
		 *
		 * @note Замков внутри нет: их берёт вызывающая сторона. Куча эта - основание
		 *       распределителя, и место замка выбирается им, а не ею
		 *
		 * \~english
		 * @brief Page heap class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Pages {
			public:
				// Размер страницы кучи в байтах
				static constexpr size_t PAGE = 8192;
				// Размер куска, берущегося у источника, в байтах
				static constexpr size_t CHUNK = (4u * 1024u * 1024u);
				// Число страниц в куске
				static constexpr size_t PAGES = (CHUNK / PAGE);
				// Наибольшее число страниц, учитываемое отдельным списком
				static constexpr size_t LISTS = 128;
				// Начальная длина таблицы поиска куска по адресу
				static constexpr size_t TABLE = 1024;
				// Наибольшее число областей, отдаваемых системе за один заход
				static constexpr size_t BATCH = 32;
			public:
				/**
				 * \~russian
				 * @brief Состояние страничной кучи
				 *
				 * \~english
				 * @brief Page heap state
				 *
				 */
				typedef struct State {
					// Взято у источника всего
					size_t total;
					// Выдано наружу прямо сейчас
					size_t used;
					// Свободно и системе не отдано
					size_t free;
					// Свободно и системе отдано
					size_t purged;
					// Число взятых у источника кусков
					size_t chunks;
					/**
					 * @brief Конструктор
					 *
					 */
					State() noexcept :
					 total(0), used(0), free(0), purged(0), chunks(0) {}
				} state_t;
			private:
				/**
				 * Предварительное объявление вложенных видов
				 *
				 * Объявление вида прямо в поле («struct Chunk * chunk;») завело бы вид в
				 * объемлющем пространстве имён, а не в классе, и обращение к его полям
				 * оказалось бы обращением к незавершённому виду
				 */
				struct Chunk;
				struct Span;
				/**
				 * @brief Учётная запись области страниц
				 *
				 */
				typedef struct Span {
					// Адрес начала области
					uint8_t * base;
					// Размер области в страницах кучи
					size_t pages;
					// Признак свободной области
					bool released;
					// Признак отданного системе содержимого
					bool purged;
					/**
					 * Признак области, изъятой на время отдачи системе
					 *
					 * Область эта не лежит ни в одном списке свободных и выдана быть не
					 * может, но освобождённой числится по-прежнему - оттого разбор адреса
					 * сбоя отвечает о ней верно. Слияние же обязано её обходить: изъятую
					 * область нельзя изъять вторично
					 */
					// Признак изъятости области на время отдачи
					bool pending;
					// Отметка времени освобождения в миллисекундах
					uint64_t stamp;
					// Метка владельца области, проставляемая слоем выше
					uint32_t tag;
					// Кусок, которому принадлежит область
					Chunk * chunk;
					// Предыдущая область в списке свободных
					Span * prev;
					// Следующая область в списке свободных
					Span * next;
				} span_t;
				/**
				 * @brief Учётная запись куска, взятого у источника
				 *
				 */
				typedef struct Chunk {
					// Адрес начала куска
					uint8_t * base;
					// Размер куска в байтах
					size_t size;
					// Число выданных наружу страниц куска
					size_t used;
					// Следующий кусок в общем списке
					Chunk * next;
					// Указатели областей по номеру страницы куска
					span_t * index[PAGES];
				} chunk_t;
			private:
				// Источник страниц
				source_t * _source;
				// Общий список взятых у источника кусков
				chunk_t * _chunks;
				/**
				 * Таблица поиска куска по адресу
				 *
				 * Куски выровнены по своему размеру, оттого начало куска берётся у любого
				 * адреса одной маской, а таблица отвечает лишь на вопрос, наш ли это кусок.
				 * Перебор списка кусков для того негоден: он зовётся на каждом
				 * освобождении, а не однажды при разборе сбоя
				 */
				// Таблица поиска куска по адресу
				chunk_t ** _table;
				// Длина таблицы поиска в местах
				size_t _length;
				// Число кусков, внесённых в таблицу
				size_t _enrolled;
				// Списки свободных областей по числу страниц
				span_t * _lists[LISTS + 1];
				// Список свободных областей, не помещающихся в списки по числу страниц
				span_t * _large;
				// Список повторно используемых учётных записей областей
				span_t * _spare;
				// Текущий кусок памяти под учётные записи
				uint8_t * _meta;
				// Остаток текущего куска памяти под учётные записи
				size_t _metaLeft;
				// Общий список кусков памяти под учётные записи
				void * _metaChunks;
				// Запрет обращаться к источнику сверх уже взятого
				bool _confined;
				// Отсрочка отдачи памяти системе в миллисекундах
				int64_t _delay;
				// Наименьший отдаваемый системе кусок в байтах
				size_t _block;
				// Потолок взятого у источника в байтах
				size_t _limit;
				// Признак отказа кучи в росте из-за потолка
				bool _jammed;
				// Состояние кучи
				state_t _state;
			private:
				/**
				 * @brief Метод выдачи памяти под учётную запись
				 *
				 * @return адрес выданной памяти либо nullptr
				 *
				 */
				void * meta() noexcept;
				/**
				 * @brief Метод взятия у источника нового куска
				 *
				 * @return взятый кусок либо nullptr
				 *
				 */
				chunk_t * grow() noexcept;
				/**
				 * @brief Метод внесения области в список свободных
				 *
				 * @param span вносимая область
				 *
				 */
				void push(span_t * span) noexcept;
				/**
				 * @brief Метод изъятия области из списка свободных
				 *
				 * @param span изымаемая область
				 *
				 */
				void pull(span_t * span) noexcept;
				/**
				 * @brief Метод поиска свободной области требуемого размера
				 *
				 * @param pages требуемое число страниц
				 * @return      найденная область либо nullptr
				 *
				 */
				span_t * search(const size_t pages) noexcept;
				/**
				 * @brief Метод слияния области с соседями по куску
				 *
				 * @param span сливаемая область
				 * @return     область после слияния
				 *
				 */
				span_t * merge(span_t * span) noexcept;
				/**
				 * @brief Метод записи области в указатели куска
				 *
				 * @param span записываемая область
				 *
				 */
				void mark(span_t * span) noexcept;
				/**
				 * \~russian
				 * @brief Метод перестроения таблицы поиска куска по адресу
				 *
				 * @param length требуемая длина таблицы в местах
				 * @return       признак перестроения таблицы
				 *
				 * \~english
				 * @brief Method of rebuilding the chunk lookup table
				 *
				 */
				bool rehash(const size_t length) noexcept;
				/**
				 * \~russian
				 * @brief Метод внесения куска в таблицу поиска
				 *
				 * @param chunk вносимый кусок
				 * @return      признак внесения куска
				 *
				 * \~english
				 * @brief Method of enrolling a chunk into the lookup table
				 *
				 */
				bool enroll(chunk_t * chunk) noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска куска, которому принадлежит адрес
				 *
				 * @param addr разбираемый адрес
				 * @return     найденный кусок либо nullptr
				 *
				 * \~english
				 * @brief Method of looking up the chunk an address belongs to
				 *
				 */
				chunk_t * lookup(const void * addr) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения кучи
				 *
				 * @param source   источник страниц
				 * @param arena    занимаемая при заведении область в байтах
				 * @param confined запрет обращаться к источнику сверх занятого
				 * @return         признак заведения кучи
				 *
				 * \~english
				 * @brief Method of initializing the heap
				 *
				 */
				bool init(source_t * source, const size_t arena, const bool confined) noexcept;
				/**
				 * \~russian
				 * @brief Метод разрушения кучи
				 *
				 * @note Отдаёт источнику всё взятое, включая память учётных записей
				 *
				 * \~english
				 * @brief Method of destroying the heap
				 *
				 */
				void destroy() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи области страниц
				 *
				 * @param pages требуемое число страниц кучи
				 * @return      адрес выданной области либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a region of pages
				 *
				 */
				void * alloc(const size_t pages) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата области страниц
				 *
				 * @note Время передаётся снаружи, а не берётся у системы: куча зовётся
				 *       из-под замка распределителя, и обращение к часам оттуда стоило бы
				 *       перехода в ядро на каждом освобождении
				 *
				 * @param addr адрес возвращаемой области
				 * @param now  текущее время в миллисекундах
				 * @return     признак возврата области
				 *
				 * \~english
				 * @brief Method of returning a region of pages
				 *
				 */
				bool free(void * addr, const uint64_t now) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения размера выданной области
				 *
				 * @param addr адрес выданной области
				 * @return     размер области в страницах кучи, либо нуль
				 *
				 * \~english
				 * @brief Method of determining the size of an allocated region
				 *
				 */
				size_t size(const void * addr) const noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска области, которой принадлежит адрес
				 *
				 * @param addr  разбираемый адрес
				 * @param begin адрес начала найденной области
				 * @param pages размер найденной области в страницах кучи
				 * @param live  признак выданной наружу области
				 * @return      признак того, что адрес принадлежит куче
				 *
				 * \~english
				 * @brief Method of finding the region an address belongs to
				 *
				 */
				bool locate(const void * addr, const void ** begin, size_t * pages, bool * live) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки принадлежности адреса куче
				 *
				 * @note В отличие от locate работает за постоянное время и годится для
				 *       вызова на каждом освобождении
				 *
				 * @param addr проверяемый адрес
				 * @return     признак того, что адрес лежит внутри взятого у источника куска
				 *
				 * \~english
				 * @brief Method of checking whether an address belongs to the heap
				 *
				 */
				bool owns(const void * addr) const noexcept;
				/**
				 * \~russian
				 * @brief Метод описания области, которой принадлежит адрес
				 *
				 * @param addr  разбираемый адрес
				 * @param begin адрес начала найденной области
				 * @param pages размер найденной области в страницах кучи
				 * @param tag   метка владельца найденной области
				 * @return      признак того, что адрес принадлежит выданной наружу области
				 *
				 * \~english
				 * @brief Method of describing the region an address belongs to
				 *
				 */
				bool describe(const void * addr, void ** begin, size_t * pages, uint32_t * tag) const noexcept;
				/**
				 * \~russian
				 * @brief Метод пометки выданной области
				 *
				 * @param addr адрес начала выданной области
				 * @param tag  проставляемая метка владельца
				 * @return     признак пометки области
				 *
				 * \~english
				 * @brief Method of tagging an allocated region
				 *
				 */
				bool tag(void * addr, const uint32_t tag) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отдачи системе свободной памяти
				 *
				 * @param now  текущее время в миллисекундах
				 * @param all  отдавать всё, не глядя на отсрочку
				 * @return     объём отданной системе памяти в байтах
				 *
				 * \~english
				 * @brief Method of returning free memory to the system
				 *
				 */
				size_t purge(const uint64_t now, const bool all) noexcept;
				/**
				 * \~russian
				 * @brief Метод изъятия областей, подлежащих отдаче системе
				 *
				 * @note Половина отдачи, идущая ПОД ЗАМКОМ. Вторая половина - обращение к
				 *       источнику - обязана идти снаружи: то системные вызовы, и держать
				 *       на них замок кучи значило бы заставить прочие потоки кружить всё
				 *       время работы ядра
				 *
				 * @param now    текущее время в миллисекундах
				 * @param all    отдавать всё, не глядя на отсрочку
				 * @param spans  место под изъятые области
				 * @param limit  наибольшее число изымаемых областей
				 * @param cursor место перебираемого списка, откуда продолжать
				 * @return       число изъятых областей
				 *
				 * \~english
				 * @brief Method of detaching the regions to be returned to the system
				 *
				 */
				size_t detach(const uint64_t now, const bool all, void ** spans, const size_t limit, size_t * cursor) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата изъятых областей в списки свободных
				 *
				 * @note Половина отдачи, идущая ПОД ЗАМКОМ, вслед за обращением к источнику
				 *
				 * @param spans  изъятые области
				 * @param count  число изъятых областей
				 * @param given  признаки состоявшейся отдачи по каждой области
				 * @return       объём отданной системе памяти в байтах
				 *
				 * \~english
				 * @brief Method of returning the detached regions to the free lists
				 *
				 */
				size_t attach(void ** spans, const size_t count, const bool * given) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи содержимого изъятой области источнику
				 *
				 * @note Половина отдачи, идущая БЕЗ ЗАМКА. Изъятая область никому не
				 *       выдаётся и никем не сливается, а источник состояния не хранит, -
				 *       оттого обращение это замка не требует
				 *
				 * @param span изъятая область
				 * @return     признак состоявшейся отдачи
				 *
				 * \~english
				 * @brief Method of discharging a detached region to the source
				 *
				 */
				bool discharge(void * span) const noexcept;
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
				 */
				void policy(const int64_t delay, const size_t block) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания потолка взятого у источника
				 *
				 * @note Потолок считается по ВЗЯТОМУ у источника, а не по выданному
				 *       прикладному коду: обращаться к системе сверх него куча не станет,
				 *       но уже взятое раздаёт до последней страницы
				 *
				 * @param limit потолок в байтах: нуль - без потолка
				 *
				 * \~english
				 * @brief Method of setting the ceiling of memory taken from the source
				 *
				 */
				void ceiling(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения упёртости кучи в потолок
				 *
				 * @note Признак снимается опросом: он взводится, когда куче отказано в
				 *       росте, и держится до следующего опроса
				 *
				 * @return признак упёртости кучи в потолок
				 *
				 * \~english
				 * @brief Method of determining whether the heap has hit its ceiling
				 *
				 */
				bool jammed() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения состояния кучи
				 *
				 * @return состояние кучи
				 *
				 * \~english
				 * @brief Method of getting the heap state
				 *
				 */
				const state_t & state() const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Pages() noexcept;
		} pages_t;
	};
};

#endif // __AWH_ALLOC_PAGES__
