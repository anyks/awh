/**
 * @file cache.hpp
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
 * @brief Заголовочный файл поток-локальных кэшей блоков
 *
 * @section cache_decisions Намеренные решения
 *
 * @details <b>Кэш потока не берёт замка вовсе.</b> В этом весь его смысл: выделение и
 *          освобождение на горячем пути обходятся снятием блока со списка, и никакой
 *          иной поток к этому списку не обращается. Замок берётся лишь при обмене
 *          пачками с центральными списками, то есть раз на пачку.
 *
 *          <b>Память под сам кэш берётся у страничной кучи, не у malloc.</b> Кэш
 *          заводится из-под перехваченного malloc, и обращение за памятью к нему же
 *          ушло бы в бесконечную возвратность.
 *
 *          <b>Поток-локальный указатель прост, а разрушение висит на ключе.</b>
 *          Поток-локальный вид с деструктором записывается через `__cxa_thread_atexit`,
 *          а тот волен обратиться за памятью - к тому же malloc. Оттого поток хранит
 *          голый указатель, а возврат блоков при завершении потока висит на ключе
 *          `pthread_key_create` (у Windows - `FlsAlloc`), чей отклик ничего не выделяет.
 *
 *          <b>Кэши завершившихся потоков не отдаются, а переиспользуются.</b> Число
 *          живущих разом потоков обыкновенно устойчиво, и возврат кучей памяти кэша с
 *          новым заведением на каждом потоке был бы работой впустую.
 *
 *          <b>Предел кэша задаётся в байтах, а не в блоках.</b> Предел в блоках
 *          означал бы у мелких разрядов килобайты, а у крупных - мегабайты, то есть
 *          не означал бы ничего. Приложению же нужен предел расхода памяти.
 *
 * \~english
 * @brief Header file of the thread-local block caches
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_CACHE__
#define __AWH_ALLOC_CACHE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "central.hpp"
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
		 * @brief Класс поток-локального кэша блоков
		 *
		 * \~english
		 * @brief Thread-local block cache class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Cache {
			public:
				/**
				 * Предел кэша потока по умолчанию в байтах
				 *
				 * Четыре мегабайта - колено измеренной кривой, а не круглое число.
				 * Замеры выдачи блоков от 512 байт до 8 килобайт при разных пределах:
				 * два мегабайта дают 127 наносекунд на действие, четыре - 43, восемь -
				 * 41, шестнадцать - 40. Работа рабочего набора вылезает за малый предел,
				 * и кэш начинает без толку сновать к центральным спискам. Платится за
				 * это ареной: каждая ступень стоит примерно своей же величины
				 */
				// Предел кэша потока по умолчанию в байтах
				static constexpr size_t LIMIT = (4u * 1024u * 1024u);
			private:
				/**
				 * @brief Список свободных блоков одного разряда
				 *
				 */
				typedef struct List {
					// Голова списка свободных блоков
					void * free;
					// Число свободных блоков в списке
					size_t count;
				} list_t;
			private:
				/**
				 * Управляющий, заведший кэш
				 *
				 * Хранится в самом кэше, а не поток-локально: отклик завершения потока
				 * зовётся системой уже ПОСЛЕ разрушения поток-локальных мест, и
				 * поток-локальный указатель на управляющего к тому времени обнулён.
				 * Проверено опытом на macOS
				 */
				// Управляющий, заведший кэш
				class Caches * _owner;
				// Центральные списки
				central_t * _central;
				// Разряды размеров
				classes_t * _classes;
			private:
				// Лежит в кэше в байтах
				size_t _bytes;
				// Предел кэша в байтах
				size_t _limit;
			private:
				/**
				 * Учёт занятого прикладным кодом, накопленный этим потоком
				 *
				 * Общий счётчик, правимый на КАЖДОЙ выдаче и КАЖДОМ освобождении, обращает
				 * восемь потоков в очередь за одной строкой памяти: замеры дали 87
				 * наносекунд на действие против 5 без него. Оттого счёт идёт здесь, у
				 * потока, а общему достаётся лишь накопленное пачкой
				 */
				// Занятое прикладным кодом, накопленное этим потоком
				int64_t _tally;
				// Освобождено этим потоком с последней проверки порога отдачи
				size_t _freed;
			private:
				/**
				 * Подсказка поиска куска по адресу
				 *
				 * Разбор адреса стоит поиска по таблице, и стоит он его на КАЖДОМ
				 * освобождении: съём образцов стека показал, что на работе контейнеров
				 * туда уходит львиная доля пути освобождения. Поток же освобождает блоки
				 * из тех же немногих кусков, что и выдавал, - оттого последний найденный
				 * кусок помнится здесь, и поиск сводится к сличению границ
				 *
				 * Вид её непрозрачен НАМЕРЕННО: устройство куска знает страничная куча, и
				 * кэшу знать его незачем
				 */
				// Подсказка поиска куска по адресу
				void * _hint;
			private:
				// Списки свободных блоков по разрядам
				list_t _lists[Classes::LIMIT];
			private:
				// Следующий кэш в общем списке
				Cache * _next;
				// Признак занятости кэша живым потоком
				bool _busy;
			private:
				/**
				 * \~russian
				 * @brief Метод возврата пачки блоков разряда центральным спискам
				 *
				 * @param index номер разряда
				 * @param count требуемое число возвращаемых блоков
				 * @return      действительно возвращённое число блоков
				 *
				 * \~english
				 * @brief Method of returning a batch of blocks to the central lists
				 *
				 */
				size_t drain(const size_t index, const size_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения кэша
				 *
				 * @param central центральные списки
				 * @param classes разряды размеров
				 * @param limit   предел кэша в байтах
				 * @return        признак заведения кэша
				 *
				 * \~english
				 * @brief Method of initializing the cache
				 *
				 */
				bool init(central_t * central, classes_t * classes, const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи всех блоков кэша центральным спискам
				 *
				 * \~english
				 * @brief Method of returning all cached blocks to the central lists
				 *
				 */
				void flush() noexcept;
				/**
				 * \~russian
				 * @brief Метод отвязки кэша от завершившегося потока
				 *
				 * @note Зовётся откликом системы, а не приложением: отклик получает лишь
				 *       сам кэш, оттого управляющий и хранится в нём
				 *
				 * \~english
				 * @brief Method of detaching the cache from a finished thread
				 *
				 */
				void release() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи блока разряда
				 *
				 * @param index номер разряда
				 * @return      адрес выданного блока либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a block of a class
				 *
				 */
				void * alloc(const size_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата блока разряда
				 *
				 * @param index номер разряда
				 * @param addr  адрес возвращаемого блока
				 *
				 * \~english
				 * @brief Method of returning a block of a class
				 *
				 */
				void free(const size_t index, void * addr) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод накопления занятого прикладным кодом
				 *
				 * @note Замка не берёт вовсе: кэш принадлежит одному потоку
				 *
				 * @param delta изменение занятого в байтах
				 * @param batch величина, по достижении которой накопленное отдаётся наружу
				 * @return      отдаваемое наружу накопленное, либо нуль
				 *
				 * \~english
				 * @brief Method of accumulating the memory occupied by the application
				 *
				 */
				int64_t tally(const int64_t delta, const int64_t batch) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения накопленного, но не отданного наружу
				 *
				 * @return накопленное этим потоком в байтах
				 *
				 * \~english
				 * @brief Method of getting what is accumulated but not yet handed out
				 *
				 */
				int64_t pending() const noexcept;
				/**
				 * \~russian
				 * @brief Метод накопления освобождённого до шага проверки
				 *
				 * @note Замка не берёт вовсе: кэш принадлежит одному потоку. Затем и
				 *       заведён: проверка порога отдачи идёт к куче ПОД ОБЩИЙ ЗАМОК, и
				 *       спроси мы её на каждом освобождении - восемь потоков дали бы 194
				 *       наносекунды на действие вместо четырёх (замерено). Со счётчиком
				 *       общий замок берётся раз в шаг, а не раз в освобождение
				 *
				 * @param delta изменение освобождённого в байтах
				 * @param step  шаг, по достижении которого пора проверять порог
				 * @return      признак того, что шаг пройден и порог пора проверить
				 *
				 * \~english
				 * @brief Method of accumulating freed memory up to a check step
				 *
				 */
				bool stepped(const size_t delta, const size_t step) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения места под подсказку поиска куска
				 *
				 * @return место под подсказку поиска
				 *
				 * \~english
				 * @brief Method of getting the slot for the chunk lookup hint
				 *
				 */
				void ** hint() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания предела кэша
				 *
				 * @param limit предел кэша в байтах
				 *
				 * \~english
				 * @brief Method of setting the cache limit
				 *
				 */
				void limit(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения объёма лежащего в кэше
				 *
				 * @return объём лежащего в кэше в байтах
				 *
				 * \~english
				 * @brief Method of getting the amount held in the cache
				 *
				 */
				size_t bytes() const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Cache() noexcept;
			/**
			 * Управляющий кэшами имеет доступ к связи и занятости
			 */
			friend class Caches;
		} cache_t;
		/**
		 * \~russian
		 * @brief Класс управления поток-локальными кэшами
		 *
		 * \~english
		 * @brief Thread-local caches manager class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Caches {
			private:
				// Центральные списки
				central_t * _central;
				// Разряды размеров
				classes_t * _classes;
			private:
				// Замок общего списка кэшей
				spin_t _lock;
				// Общий список заведённых кэшей
				cache_t * _caches;
				// Число заведённых кэшей
				size_t _count;
				// Предел кэша потока в байтах
				size_t _limit;
			private:
				// Признак заведённости ключа завершения потока
				bool _keyed;
			private:
				/**
				 * \~russian
				 * @brief Метод заведения кэша текущему потоку
				 *
				 * @return заведённый кэш либо nullptr
				 *
				 * \~english
				 * @brief Method of creating a cache for the current thread
				 *
				 */
				cache_t * create() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения управляющего кэшами
				 *
				 * @param central центральные списки
				 * @param classes разряды размеров
				 * @return        признак заведения
				 *
				 * \~english
				 * @brief Method of initializing the caches manager
				 *
				 */
				bool init(central_t * central, classes_t * classes) noexcept;
				/**
				 * \~russian
				 * @brief Метод разрушения управляющего кэшами
				 *
				 * @note Отдаёт центральным спискам блоки всех кэшей
				 *
				 * \~english
				 * @brief Method of destroying the caches manager
				 *
				 */
				void destroy() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения кэша текущего потока
				 *
				 * @note Заводит кэш при первом обращении потока
				 *
				 * @return кэш текущего потока либо nullptr
				 *
				 * \~english
				 * @brief Method of getting the current thread cache
				 *
				 */
				cache_t * local() noexcept;
				/**
				 * \~russian
				 * @brief Метод отвязки кэша от завершившегося потока
				 *
				 * @param cache отвязываемый кэш
				 *
				 * \~english
				 * @brief Method of detaching a cache from a finished thread
				 *
				 */
				void retire(cache_t * cache) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания предела кэша потока
				 *
				 * @note Действует и на уже заведённые кэши
				 *
				 * @param limit предел кэша в байтах
				 *
				 * \~english
				 * @brief Method of setting the per-thread cache limit
				 *
				 */
				void limit(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения объёма, лежащего во всех кэшах
				 *
				 * @param count число заведённых кэшей
				 * @return      объём лежащего в кэшах в байтах
				 *
				 * \~english
				 * @brief Method of getting the amount held in all caches
				 *
				 */
				size_t cached(size_t * count) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения накопленного кэшами, но не отданного наружу
				 *
				 * @note Складывает накопленное ВСЕМИ кэшами, включая кэши завершившихся
				 *       потоков: те не разрушаются, а переиспользуются, и накопленное в
				 *       них остаётся верным
				 *
				 * @return накопленное всеми кэшами в байтах
				 *
				 * \~english
				 * @brief Method of getting what the caches accumulated but have not handed out
				 *
				 */
				int64_t pending() noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи центральным спискам блоков всех кэшей
				 *
				 * @return объём отданного в байтах
				 *
				 * \~english
				 * @brief Method of returning all cached blocks to the central lists
				 *
				 */
				size_t flush() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата замка перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring the lock before forking
				 *
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения замка после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing the lock after forking
				 *
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения кэшей в порядок у потомка ветвления
				 *
				 * @note Зовётся СТРОГО ПОСЛЕ `Central::adopt`: отдача блоков идёт в
				 *       центральные списки, а те до своего отпускания держат замки,
				 *       захваченные не пережившими ветвление потоками
				 *
				 * @note Потомку достаётся один поток, а кэши прочих потоков родителя
				 *       держат блоки, вернуть которые больше некому
				 *
				 * \~english
				 * @brief Method of reconciling the caches in the forked child
				 *
				 */
				void adopt() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Caches() noexcept;
		} caches_t;
	};
};

#endif // __AWH_ALLOC_CACHE__
