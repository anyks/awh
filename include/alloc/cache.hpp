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
 *          <b>Замка не берёт СПИСОК, но счётчики читает чужой поток.</b> Опрос расхода
 *          складывает накопленное занятое и лежащее в кэше по ВСЕМ кэшам, оттого поля
 *          эти неделимые, хотя пишет в них один хозяин. Вывод «к кэшу не обращается
 *          никто, значит обычные поля годятся» неверен, и обычными они однажды были -
 *          ThreadSanitizer доложил обе гонки. Правятся счётчики загрузкой и записью
 *          порознь, а не неделимой правкой: писать в них вправе лишь хозяин, и запрет
 *          на быстром пути стоил бы ни за что.
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
#include <atomic>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "link.hpp"
#include "central.hpp"
#include "classes.hpp"
#include "../sys/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_CACHE_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_CACHE_INLINE inline __attribute__((always_inline))
#endif

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
				 * \~russian
				 * @brief Метод пополнения разряда пачкой блоков у центральных списков
				 *
				 * @note Холодный хвост быстрого пути выдачи: вынесен в файл кода, чтобы
				 *       подстановка тащила за собою лишь снятие блока с головы списка
				 *
				 * @param index номер разряда
				 * @return      адрес выданного блока либо nullptr
				 *
				 * \~english
				 * @brief Method of refilling a class with a batch from the central lists
				 *
				 */
				void * refill(const size_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи излишка центральным спискам
				 *
				 * @note Холодный хвост быстрого пути возврата: перебор предела - случай
				 *       редкий, и держать его в подставляемом теле незачем
				 *
				 * \~english
				 * @brief Method of giving the excess back to the central lists
				 *
				 */
				void relieve() noexcept;
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
				/**
				 * Лежащее в кэше НЕДЕЛИМО по той же причине, что и накопленное
				 *
				 * Складывает его чужой поток - опрос лежащего в кэшах и отдача излишка
				 * центральным спискам, - а пишет хозяин на каждой выдаче и каждом
				 * освобождении: доложено ThreadSanitizer'ом в `Caches::cached`. Правится
				 * загрузкой и записью порознь: пишет один поток, и неделимая правка со
				 * своим запретом стоила бы на быстром пути ни за что
				 */
				std::atomic <size_t> _bytes;
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
				/**
				 * Накопитель НЕДЕЛИМЫЙ, хотя хозяин у него один
				 *
				 * Читает его чужой поток - опрос расхода складывает накопленное всеми
				 * кэшами, - и обычное поле давало бы состязание по стандарту: доложено
				 * ThreadSanitizer'ом, чтение в `Caches::pending` против записи хозяина.
				 * Правится он ЗАГРУЗКОЙ И ЗАПИСЬЮ порознь, а не чтением-правкой-записью:
				 * писать в него вправе один поток, и неделимая правка со своим запретом
				 * стоила бы на быстром пути ни за что
				 */
				// Занятое прикладным кодом, накопленное этим потоком
				std::atomic <int64_t> _tally;
				// Освобождено этим потоком с последней проверки порога отдачи
				size_t _freed;
			private:
				/**
				 * \~russian
				 * @brief Просьба опустошиться, поставленная чужим потоком
				 *
				 * @details Кэш потока НЕ ЗАЩИЩЁН замком, и в том весь его смысл: обращается
				 *          к нему лишь свой поток, и обращение стоит считанных команд.
				 *          Оттого трогать чужой кэш нельзя ВОВСЕ - а прежде это делалось:
				 *          отдача памяти системе (`Allocator::purge`) опустошала кэши всех
				 *          потоков подряд, включая работающие прямо сейчас
				 *
				 * @details Порча от этого выглядела так: живой блок, которым владеет
				 *          приложение, оказывался в списке свободных, и разбор списка
				 *          утыкался в данные приложения вместо указателя. Найдено
				 *          ворошителем `tools/fuzz/alloc.cpp` 23.08.2026, доказано
				 *          ThreadSanitizer: `Caches::flush` правила списки чужого кэша
				 *          (cache.cpp:255-265), пока хозяин писал их в `Cache::refill`
				 *
				 * @details Взамен чужому кэшу ставится просьба, и хозяин опустошает его САМ,
				 *          когда в следующий раз пойдёт холодным путём
				 *
				 * @warning Кэш потока, заснувшего надолго, по чужой просьбе теперь НЕ
				 *          опустошится: снять с него память некому, пока он не проснётся.
				 *          Иначе и быть не может - забрать память у работающего потока без
				 *          его участия нельзя, - но отдача памяти системе стала оттого менее
				 *          полной. Кэш завершающегося потока отдаётся целиком, и в этом
				 *          отдача не изменилась
				 *
				 * @warning Счёт обвалов ворошителя мерой тут НЕ ГОДИТСЯ: он зависит от
				 *          занятости машины, и одна и та же сборка давала то 16 отказов из
				 *          20, то ни одного. Судить следует по числу гонок, какие называет
				 *          ThreadSanitizer
				 *
				 * \~english
				 * @brief Request to yield the cache, set by a foreign thread
				 *
				 */
				std::atomic <bool> _yield;
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
				AWH_CACHE_INLINE void * alloc(const size_t index) noexcept {
					// Если кэш не заведён либо разряд неведом
					if((this->_classes == nullptr) || (index >= this->_classes->count()))
						// Выдавать нечего
						return nullptr;
					// Если свободных блоков разряда в кэше не осталось
					if(this->_lists[index].free == nullptr)
						// Забираем у центральных списков пачку блоков
						return this->refill(index);
					// Снимаем блок с головы списка
					void * result = this->_lists[index].free;
					// Головой списка становится следующий блок
					this->_lists[index].free = awh::alloc::Link::next(result);
					// Уменьшаем число блоков в кэше
					this->_lists[index].count--;
					// Уменьшаем лежащее в кэше
					this->_bytes.store((this->_bytes.load(std::memory_order_relaxed) - this->_classes->size(index)), std::memory_order_relaxed);
					// Выводим выданный блок
					return result;
				}
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
				AWH_CACHE_INLINE void free(const size_t index, void * addr) noexcept {
					// Если кэш не заведён, разряд неведом либо блок не задан
					if((this->_classes == nullptr) || (index >= this->_classes->count()) || (addr == nullptr))
						// Возвращать нечего
						return;
					// Связываем возвращаемый блок с прежней головой списка
					awh::alloc::Link::next(addr, this->_lists[index].free);
					// Головой списка становится возвращаемый блок
					this->_lists[index].free = addr;
					// Увеличиваем число блоков в кэше
					this->_lists[index].count++;
					// Увеличиваем лежащее в кэше
					this->_bytes.store((this->_bytes.load(std::memory_order_relaxed) + this->_classes->size(index)), std::memory_order_relaxed);
					/**
					 * Просьба опустошиться здесь НЕ читается намеренно
					 *
					 * Читать её на каждом освобождении стоит около пяти сотых на трёх
					 * сценариях замера разом (81 780 → 77 760 тыс.оп/с на одном разряде,
					 * 801 095 → 753 877 на потоках, 84 864 → 80 840 на освобождении
					 * чужим) - и не даёт ничего: поток, который только освобождает, копит
					 * блоки в своём кэше, кэш перебирает предел, и просьбу видит
					 * `relieve`. Мгновенности здесь не требуется вовсе
					 */
					// Если предел кэша перебран
					if(this->_bytes.load(std::memory_order_relaxed) > this->_limit)
						// Отдаём излишек центральным спискам
						this->relieve();
				}
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
				AWH_CACHE_INLINE int64_t tally(const int64_t delta, const int64_t batch) noexcept {
					// Накапливаем изменение занятого
					const int64_t result = (this->_tally.load(std::memory_order_relaxed) + delta);
					// Если накопленное не дотянуло до величины отдачи
					if((result < batch) && (result > -batch)){
						// Запоминаем накопленное
						this->_tally.store(result, std::memory_order_relaxed);
						// Отдавать нечего
						return 0;
					}
					// Обнуляем накопленное: оно уходит наружу
					this->_tally.store(0, std::memory_order_relaxed);
					// Выводим отдаваемое наружу накопленное
					return result;
				}
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
				AWH_CACHE_INLINE void ** hint() noexcept {
					// Выводим место под подсказку поиска
					return &this->_hint;
				}
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
		 * Зеркало кэша текущего потока
		 *
		 * Объявлено ЗДЕСЬ, а не только в файле кода, ради быстрого пути `local`: тот
		 * зовётся на каждую выдачу и каждое освобождение, и вызов ради чтения одного
		 * указателя стоил бы дороже самого чтения. Определение лежит в `cache.cpp`,
		 * там же записаны доводы модели `initial-exec` и перечень систем, каким
		 * зеркало не заводится вовсе
		 */
		#if !defined(_WIN32) && !defined(_WIN64) && !defined(__APPLE__) && !defined(__MACH__) && !defined(__OpenBSD__)
			#define AWH_ALLOC_MIRROR 1
			extern __thread cache_t * __awh_alloc_mirror__ __attribute__((tls_model("initial-exec")));
		#endif
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
				 * @brief Метод заведения ключа хранения кэшей потоков
				 *
				 * @note Зовётся ОТДЕЛЬНО от заведения и ПОЗЖЕ него: заведение приходится
				 *       на первую выдачу памяти в процессе, когда средства потоков могут
				 *       быть ещё не готовы сами. До этого вызова кэшей потоков нет вовсе,
				 *       и выдача идёт общим путём через центральные списки
				 *
				 * @return признак заведённого ключа
				 *
				 * \~english
				 * @brief Method of creating the thread cache storage key
				 *
				 * @return flag of the key having been created
				 *
				 */
				bool arm() noexcept;
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
			private:
				/**
				 * \~russian
				 * @brief Метод заведения кэша текущему потоку
				 *
				 * @note Холодный хвост `local`: сюда приходят лишь первым обращением
				 *       потока да у систем, каким зеркало не заводится
				 *
				 * @return кэш текущего потока либо nullptr
				 *
				 * \~english
				 * @brief Method of creating a cache for the current thread
				 *
				 */
				cache_t * enter() noexcept;
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
				AWH_CACHE_INLINE cache_t * local() noexcept {
					/**
					 * Отвечаем из зеркала, не заходя в файл кода
					 *
					 * Заполненное зеркало значит, что кэш потоку уже заведён и записан в
					 * ключ хранения: прочие проверки холодного пути отвечали бы то же
					 * самое. Профиль `perf` на Debian отдавал `Caches::local` 9.9 %
					 * времени сценария одного разряда - и то была цена вызова, а не работы
					 */
					#if defined(AWH_ALLOC_MIRROR)
						// Если зеркало заполнено
						if(__awh_alloc_mirror__ != nullptr)
							// Выводим кэш из зеркала
							return __awh_alloc_mirror__;
					#endif
					// Уходим холодным путём: кэша потоку ещё нет либо зеркала нет вовсе
					return this->enter();
				}
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
