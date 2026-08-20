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
					// Отметка времени освобождения в миллисекундах
					uint64_t stamp;
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
