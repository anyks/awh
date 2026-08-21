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
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_CENTRAL__
#define __AWH_ALLOC_CENTRAL__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "pages.hpp"
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
		 * @brief Класс центральных списков свободных блоков
		 *
		 * \~english
		 * @brief Central free lists class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Central {
			public:
				// Наибольшее число блоков в одной передаваемой пачке
				static constexpr size_t BATCH = 64;
			public:
				/**
				 * \~russian
				 * @brief Состояние центральных списков
				 *
				 * \~english
				 * @brief Central free lists state
				 *
				 */
				typedef struct State {
					// Лежит в центральных списках в байтах
					size_t cached;
					// Выдано кэшам потоков в байтах
					size_t live;
					// Нарезано областей под разряды
					size_t regions;
					/**
					 * @brief Конструктор
					 *
					 */
					State() noexcept : cached(0), live(0), regions(0) {}
				} state_t;
			private:
				/**
				 * @brief Центральный список одного разряда
				 *
				 */
				typedef struct List {
					// Замок разряда
					spin_t lock;
					// Голова списка свободных блоков
					void * free;
					// Число свободных блоков в списке
					size_t count;
					// Число блоков, выданных кэшам потоков
					size_t live;
					// Число нарезанных под разряд областей
					size_t regions;
					/**
					 * @brief Конструктор
					 *
					 */
					List() noexcept : lock(), free(nullptr), count(0), live(0), regions(0) {}
				} list_t;
			private:
				// Страничная куча
				pages_t * _pages;
				// Разряды размеров
				classes_t * _classes;
			private:
				// Замок страничной кучи
				spin_t _heap;
				// Центральные списки по разрядам
				list_t _lists[Classes::LIMIT];
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
				 */
				bool carve(const size_t index) noexcept;
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
				 */
				void back(const size_t index, void * head, void * tail, const size_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи памяти сверх разрядов
				 *
				 * @param size требуемый размер в байтах
				 * @return     адрес выданной памяти либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating memory beyond the size classes
				 *
				 */
				void * alloc(const size_t size) noexcept;
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
				 */
				bool free(void * addr, const uint64_t now) noexcept;
			public:
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
				 */
				bool owner(const void * addr, size_t * index, void ** begin, size_t * size, void ** hint = nullptr) noexcept;
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
				/**
				 * \~russian
				 * @brief Метод задания потолка взятого у источника
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
				 * @brief Метод получения состояния центральных списков
				 *
				 * @return состояние списков
				 *
				 * \~english
				 * @brief Method of getting the central free lists state
				 *
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
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения всех замков после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing all locks after forking
				 *
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
				 */
				void adopt() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Central() noexcept;
		} central_t;
	};
};

#endif // __AWH_ALLOC_CENTRAL__
