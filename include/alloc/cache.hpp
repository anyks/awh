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
 *          <b>Отдача излишка опускает кэш НИЖЕ предела, а не ровно под него.</b>
 *          Предел кэша при этом соблюдается по-прежнему: отдача заводится ровно на нём.
 *          Прекращалась же она прежде на нём же, - и следующее освобождение возвращало
 *          лежащее за предел и звало отдачу снова. Кэш, дошедший до предела, снова́л к
 *          центральным спискам на КАЖДОМ освобождении, беря общий замок раз в действие:
 *          ровно то, ради избавления от чего кэш и заведён. Запас в четверть предела
 *          даёт кэшу пачку действий до следующей отдачи, и стоит он лишь того, что кэш
 *          держит на четверть меньше памяти. Глубина запаса на меру не влияет вовсе -
 *          половина, четверть и восьмая доли дали одно и то же, - значимо само наличие
 *          запаса. Замеры и разбор записаны у `Cache::relieve`.
 *
 *          <b>Рабочий предел служит рычагом просьбы опустошиться.</b> Просьбу читает
 *          хозяин на холодных путях, а не на каждом освобождении: то стоило бы около пяти
 *          сотых на трёх сценариях замера. Поток, который только освобождает, на
 *          пополнение разряда не заходит вовсе, - оттого ставящий просьбу опускает
 *          рабочий предел до нуля, и отдача излишка заводится на первом же освобождении.
 *          Заданный приложением предел хранится отдельно и возвращается на место, едва
 *          просьба исполнена. Порядок двух записей значим: рычаг опускается ПРЕЖДЕ
 *          просьбы, - и то же окно закрыто вторично самолечением в `Cache::relieve`.
 *          Цена промаха доказана подменой: кэш с опущенным рычагом опустошается на каждом
 *          освобождении и не держит ничего - 92.57 нс/оп и НУЛЬ байт в кэше против 9.15
 *          нс/оп и 32768 байт. Закреплено проверкой `CacheWorksAfterPurge`, и та доказала
 *          способность отказать на той же подмене.
 *
 *          <b>Предел кэша задаётся в байтах, а не в блоках.</b> Предел в блоках
 *          означал бы у мелких разрядов килобайты, а у крупных - мегабайты, то есть
 *          не означал бы ничего. Приложению же нужен предел расхода памяти.
 *
 * \~english
 * @brief Header file of the thread-local block caches
 *
 * @section cache_decisions Deliberate decisions
 *
 * @details <b>The thread cache takes no lock at all.</b> That is its whole point:
 *          allocation and free on the hot path are done by taking a block off a list,
 *          and no other thread touches that list. A lock is taken only when exchanging
 *          batches with the central lists, that is, once per batch.
 *
 *          <b>Memory for the cache itself is taken from the page heap, not from
 *          malloc.</b> The cache is created from under the intercepted malloc, and
 *          asking it for memory would go into infinite recursion.
 *
 *          <b>The thread-local pointer is plain, and destruction hangs on a key.</b>
 *          A thread-local with a destructor is written via __cxa_thread_atexit, and
 *          that is free to ask for memory - from the same malloc. Hence the thread
 *          holds a bare pointer, and returning blocks on thread exit hangs on a
 *          pthread_key_create key (FlsAlloc on Windows), whose callback allocates
 *          nothing.
 *
 *          <b>Caches of finished threads are reused, not returned.</b> The number of
 *          simultaneously living threads is usually stable, and returning cache memory
 *          to the heap with new creation per thread would be wasted work.
 *
 *          <b>The list takes no lock, but counters are read by a foreign thread.</b>
 *          Querying consumption sums the accumulated occupied and the cache-held
 *          across ALL caches, hence these fields are atomic, though only the owner
 *          writes them. The conclusion nobody accesses the cache so plain fields
 *          suffice is wrong, and plain they once were - ThreadSanitizer reported both
 *          races. Counters are fixed by separate load and store, not an atomic RMW:
 *          only the owner may write them, and a prohibition on the fast path would
 *          cost for nothing.
 *
 *          <b>Returning the excess drops the cache BELOW the limit, not exactly to
 *          it.</b> The cache limit is still respected: returning starts exactly at it.
 *          But it used to stop exactly at it too - and the next free would return
 *          what was beyond the limit and trigger returning again. A cache that
 *          reached the limit went to the central lists on EVERY free, taking the
 *          shared lock once per action: exactly what the cache was built to avoid. A
 *          reserve of a quarter limit gives the cache a batch of actions before the
 *          next return, and it costs only that the cache holds a quarter less memory.
 *          The depth of the reserve does not affect the measurement at all - half,
 *          quarter and eighth all gave the same - what matters is the presence of
 *          the reserve. Measurements and analysis are recorded at Cache::relieve.
 *
 *          <b>The working limit serves as a lever for the yield request.</b> The
 *          request is read by the owner on cold paths, not on every free: that would
 *          cost about five hundredths on three benchmark scenarios. A thread that
 *          only frees never goes to class refill - hence whoever sets the request
 *          drops the working limit to zero, and excess returning starts on the very
 *          first free. The limit set by the application is stored separately and
 *          restored once the request is fulfilled. The order of the two writes
 *          matters: the lever is dropped BEFORE the request - and the same window is
 *          closed a second time by self-healing in Cache::relieve. The cost of a miss
 *          is proven by substitution: a cache with the dropped lever empties on every
 *          free and holds nothing - 92.57 ns/op and ZERO bytes in cache versus 9.15
 *          ns/op and 32768 bytes. Confirmed by the CacheWorksAfterPurge check, which
 *          also proved the ability to refuse on the same substitution.
 *
 *          <b>The cache limit is set in bytes, not in blocks.</b> A limit in blocks
 *          would mean kilobytes for small classes and megabytes for large ones, that
 *          is, nothing at all. What the application needs is a memory consumption
 *          limit.
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
#include <atomic>

/**
 * Стандартные заголовочные файлы систем Unix
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <pthread.h>
#endif

/**
 * Наши модули
 */
#include "spin.hpp"
#include "link.hpp"
#include "central.hpp"
#include "classes.hpp"
#include "../sys/macro/global.hpp"

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
		 * @brief Класс поток-локального кэша блоков
		 *
		 * \~english
		 * @brief Thread-local block cache class
		 *
		 * \~
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
				/**
				 * Порог мёртвого запаса разряда в кэше
				 *
				 * Освобождение чужим потоком ложится в кэш потока-освободителя, и разряд,
				 * который тот сам не выдаёт, копится без пользы: центральные списки пусты,
				 * и выдающий поток нарезает всё новые области. Замерено суточной выдержкой
				 * движка 27.10: по одному блоку 656 Б на оборот копилось сутки - +33,8 МБ,
				 * страница каждые ~24 с, - при пределе кэша в 64 МБ отдача излишка не
				 * достигалась бы и за день. Список разряда, существенно выросший за окно
				 * наблюдения IDLE_WINDOW, отдаётся центральным спискам ЦЕЛИКОМ;
				 * горячий запас, из которого идут выдачи, не трогается вовсе
				 */
				// Порог мёртвого запаса разряда в кэше в байтах
				static constexpr size_t IDLE = (1u * 1024u * 1024u);
				/**
				 * Окно наблюдения за ростом списка в миллисекундах
				 *
				 * Список сливается центру, если он вырос за окно наблюдения: выброс
				 * разрушения дерева укладывается в окно МИЛЛИСЕКУНДАМИ и не сливается
				 * (его единственный слив за круг стоит ничтожно), а список, копимый
				 * часами, сливается каждое окно и держится в пределах IDLE
				 */
				// Окно наблюдения за ростом списка в миллисекундах
				static constexpr uint64_t IDLE_WINDOW = 60000;
			private:
				/**
				 * \~russian
				 * @brief Метод пополнения разряда пачкой блоков у центральных списков
				 *
				 * @warning Список разряда обязан быть ПУСТ: взятая пачка кладётся головой
				 *          напрямую, и лежавшее в списке прежде потерялось бы молча -
				 *          блоки остались бы выданными кэшу навсегда. Единственный
				 *          звавший, `alloc`, заходит сюда лишь при исчерпанном списке, а
				 *          просьба опустошиться, исполняемая здесь же, опустошает все
				 *          разряды прежде набора
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
				 * @warning The class list MUST be empty: the batch taken is placed at the head
				 *          directly, and whatever lay in the list before would be lost silently -
				 *          the blocks would remain issued to the cache forever. The only caller,
				 *          `alloc`, enters here only with the list exhausted, and the flush
				 *          request executed right here empties all classes before taking
				 *
				 * @note Cold tail of the fast allocation path: moved into the code file so that
				 *       the substitution carries only taking a block off the list head
				 *
				 * @param index class index
				 * @return      address of the issued block or nullptr
				 *
				 * \~
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
				 * @note Only the excess beyond the cache limit is returned: the cache keeps a
				 *       working set at hand
				 *
				 * \~
				 */
				void relieve() noexcept;
				/**
				 * \~russian
				 * @brief Список свободных блоков одного разряда
				 *
				 *
				 * \~english
				 * @brief Free block list of one size class
				 *
				 * \~
				 */
				typedef struct List {
					// Голова списка свободных блоков
					void * free;
					// Число свободных блоков в списке
					size_t count;
				} list_t;
			private:
				/**
				 * Наблюдение за ростом списка разряда
				 *
				 * Наблюдается ОДИН разряд за раз: слив мёртвого запаса - случай
				 * редкий, и держать по метке на каждый разряд незачем. Метка
				 * снимается пробуждением кэша - освобождением в наблюдаемый список
				 */
				// Наблюдаемый разряд плюс единица, нуль если наблюдения нет
				size_t _watch;
				// Миг начала наблюдения в миллисекундах
				uint64_t _watched;
				// Занятое наблюдаемого списка при начале наблюдения в байтах
				size_t _stocked;
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
				/**
				 * Предел кэша НЕДЕЛИМ по той же причине, что и лежащее в кэше
				 *
				 * Пишет его ЧУЖОЙ поток - установщик настроек обходит все заведённые кэши
				 * и правит предел каждому, - а читает хозяин на каждом освобождении, и
				 * читает БЕЗ замка. Замок установщика хозяина не сдерживает, состязание
				 * налицо. Прежде предел лежал обычным числом: на наших системах чтение и
				 * запись выровненного слова и так неделимы, оттого беда наружу не выходила,
				 * - но по стандарту это состязание, и ThreadSanitizer его назовёт. Разбор
				 * лежащего в кэше двумя строками выше говорит ровно то же, а предел под
				 * него не подвели
				 *
				 * Быстрый путь от этого не дорожает: чтение без порядка складывается в ту
				 * же одну команду, что и чтение обычного числа
				 */
				std::atomic <size_t> _limit;
				/**
				 * Предел, заданный приложением
				 *
				 * Рабочий предел двумя строками выше служит ещё и рычагом просьбы
				 * опустошиться: ставя её, чужой поток обнуляет рабочий предел, и хозяин
				 * заходит в отдачу излишка на первом же освобождении - там просьбу и
				 * видит. Заданный приложением предел на это время хранится здесь и
				 * возвращается на место, едва просьба исполнена
				 *
				 * Заведено взамен чтения просьбы на быстром пути: то стоило около пяти
				 * сотых на трёх сценариях замера, а это не стоит ничего
				 */
				// Предел, заданный приложением
				std::atomic <size_t> _kept;
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
				 * @details The thread cache is NOT protected by a lock, and that is its whole
				 *          point: only the owning thread accesses it, and access costs a few
				 *          instructions. Hence touching a foreign cache is forbidden AT ALL -
				 *          and it was done before: returning memory to the system
				 *          (Allocator::purge) emptied all thread caches in a row, including
				 *          those working right now
				 *
				 * @details The corruption looked like this: a live block owned by the
				 *          application ended up in the free list, and list traversal hit
				 *          application data instead of a pointer. Found by the fuzzer
				 *          tools/fuzz/alloc.cpp on 23.08.2026, proven by ThreadSanitizer:
				 *          Caches::flush edited the lists of a foreign cache (cache.cpp:255-265)
				 *          while the owner was writing them in Cache::refill
				 *
				 * @details Instead a request is set on the foreign cache, and the owner
				 *          empties it THEMSELVES next time they go the cold path
				 *
				 * @warning A thread cache that has slept for long will NOT be emptied by a
				 *          foreign request: nobody can take memory from it until it wakes up.
				 *          It cannot be otherwise - taking memory from a working thread
				 *          without its participation is impossible - but returning memory to
				 *          the system became less complete because of it. A finishing thread
				 *          cache is returned entirely, and that did not change
				 *
				 * @warning Counting fuzzer crashes as a measure does NOT work here: it depends
				 *          on machine load, and the same build gave either 16 failures out of
				 *          20 or none at all. Judgement should go by the number of races that
				 *          ThreadSanitizer names
				 *
				 * \~
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
				 * @param index class index
				 * @param count number of blocks being returned
				 * @return      number of blocks actually returned
				 *
				 * \~
				 */
				size_t drain(const size_t index, const size_t count) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения мига наблюдения за ростом
				 *
				 * @details Часы стоят лишь на наблюдении мёртвого запаса: зовутся по
				 *          разу на освобождении в переросший порог список, а не на каждом
				 *          освобождении
				 *
				 * @return миг в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the growth observation instant
				 *
				 * @details The clock is used only for dead stock observation: called once
				 *          per free into a list past the threshold, not per every free
				 *
				 * @return instant in milliseconds
				 *
				 * \~
				 */
				static uint64_t ticking() noexcept;
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
				 * @param central central lists
				 * @param classes size classes
				 * @param limit   cache limit in bytes
				 * @return        sign of the cache having been initialized
				 *
				 * \~
				 */
				bool init(central_t * central, classes_t * classes, const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи всех блоков кэша центральным спискам
				 *
				 * \~english
				 * @brief Method of returning all cached blocks to the central lists
				 *
				 * \~
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
				 * @note The cache is returned to the pool for reuse by a new thread:
				 *       its blocks go back to the central lists
				 *
				 * \~
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
				 * @param index class index
				 * @return      address of the allocated block or nullptr
				 *
				 * \~
				 */
				AWH_CACHE_INLINE void * alloc(const size_t index) noexcept;
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
				 * @param index class index
				 * @param addr  address of the block being returned
				 *
				 * \~
				 */
				AWH_CACHE_INLINE void free(const size_t index, void * addr) noexcept;
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
				 * @note Takes no lock at all: the cache belongs to a single thread
				 *
				 * @param delta change of the occupied memory in bytes
				 * @param batch value upon reaching which the accumulated amount is handed out
				 * @return      the accumulated amount handed out, or zero
				 *
				 * \~
				 */
				AWH_CACHE_INLINE int64_t tally(const int64_t delta, const int64_t batch) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения накопленного, но не отданного наружу
				 *
				 * @return накопленное этим потоком в байтах
				 *
				 * \~english
				 * @brief Method of getting what is accumulated but not yet handed out
				 *
				 * @return accumulated value in bytes
				 *
				 * \~
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
				 * @note Takes no lock at all: the cache belongs to a single thread. That is why it
				 *       was built: checking the return threshold goes to the heap UNDER THE SHARED
				 *       LOCK, and asking it on every free would give 194 nanoseconds per action
				 *       instead of four (measured). With a counter the shared lock is taken once
				 *       per step, not once per free
				 *
				 * @param delta freed memory change in bytes
				 * @param step  step upon reaching which the threshold is due to be checked
				 * @return      sign that the step has been passed and the threshold is due
				 *
				 * \~
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
				 * @return hint slot address
				 *
				 * \~
				 */
				AWH_CACHE_INLINE void ** hint() noexcept;
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
				 * @param limit cache limit in bytes
				 *
				 * \~
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
				 * @return amount held in bytes
				 *
				 * \~
				 */
				size_t bytes() const noexcept;
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
				Cache() noexcept;
			/**
			 * Управляющий кэшами имеет доступ к связи и занятости
			 */
			friend class Caches;
		} cache_t;
		/**
		 * Ключ хранения кэша для быстрого пути macOS
		 *
		 * Зеркала средствами собирателя у macOS нет и быть не может: место потока
		 * заводится там лениво через `_tlv_get_addr`, а тот просит памяти, то есть зовёт
		 * нас изнутри выдачи. Доводы и опыт записаны в `cache.cpp`. Оттого быстрый путь
		 * `local` уходил там ВЫЗОВОМ в `enter`, и вызов этот приходился на каждую выдачу
		 * и каждое освобождение.
		 *
		 * Читаем ключ прямо здесь. Обращение к ключу у macOS памяти не просит вовсе -
		 * место под значения ключей заводится потоку целиком, вместе с самим потоком, и
		 * щуп отмерил ноль выделений на первую отметку при любом номере ключа (разобрано
		 * там же, у сторожа возвратности). Оттого и сторожить здесь нечего: у macOS он
		 * пуст по тому же доводу.
		 *
		 * Ключ системы остаётся хранением, как и был: только он даёт отклик завершения
		 * потока. Здесь лишь снят вызов с пути чтения
		 */
		#if defined(__APPLE__) || defined(__MACH__)
			/**
			 * Место потока хранится ключом системы
			 */
			#define AWH_ALLOC_TSD 1
			// Ключ места потока
			extern pthread_key_t __awh_alloc_tsd__;
			// Признак готовности захвата
			extern std::atomic <bool> __awh_alloc_armed__;
			/**
			 * Прямое чтение места потока, минуя вызов
			 *
			 * Чтение ключа вызовом стоит 1.502 наносекунды, прямое обращение к месту
			 * потока - 0.351 (замерено на этой машине, 20 млн заходов). Но дороже самой
			 * разницы обходится ВЫЗОВ как таковой: живые значения приходится держать в
			 * сохраняемых регистрах, и пролог выдачи сохранял их ВОСЕМЬ на каждом
			 * обращении - девять команд пролога и пять эпилога на всякую выдачу
			 *
			 * Устройство места потока у macOS договором не объявлено, и полагаться на
			 * него вслепую нельзя. Оттого оно ПРОВЕРЯЕТСЯ при заведении ключа: кладём
			 * метку обычным средством, читаем прямым обращением и сличаем. Разошлось -
			 * прямой путь не заводится вовсе, и чтение идёт вызовом, как и шло
			 *
			 * Хранится здесь номер места, а не признак: сличение с `NOSLOT` разом
			 * отвечает и на вопрос «проверено ли», и на вопрос «где лежит»
			 */
			static constexpr size_t AWH_ALLOC_NOSLOT = static_cast <size_t> (-1);
			// Номер места потока для прямого чтения, либо AWH_ALLOC_NOSLOT
			extern std::atomic <size_t> __awh_alloc_direct__;
			/**
			 * \~russian
			 * @brief Функция получения основания места потока
			 *
			 * @return основание места потока, либо nullptr у неведомого набора команд
			 *
			 * \~english
			 * @brief Function of getting the thread-specific storage base
			 *
			 * @return thread-specific storage base address
			 *
			 * \~
			 */
			AWH_CACHE_INLINE void ** __awh_alloc_slots__() noexcept;
		#endif
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
			/**
			 * Зеркало места потока хранится переменной TLS
			 */
			#define AWH_ALLOC_MIRROR 1
			// Зеркало места потока
			extern __thread cache_t * __awh_alloc_mirror__ __attribute__((tls_model("initial-exec")));
		#endif
		/**
		 * \~russian
		 * @brief Класс управления поток-локальными кэшами
		 *
		 * \~english
		 * @brief Thread-local caches manager class
		 *
		 * \~
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
				/**
				 * Общий на процесс запас кэшей потоков в байтах
				 *
				 * Предел кэша ЗАДАННЫЙ приложением - предел каждого кэша порознь, и
				 * расход при нём растёт числом потоков без всякой границы: сто потоков
				 * при пределе в четыре мегабайта держат четыреста. Запас же общий: доля
				 * потока выводится из него делением на число заведённых кэшей, и расход
				 * ограничен запасом, сколько бы потоков ни было
				 *
				 * Оттого же он и БЫСТРЕЕ постоянного предела. Замер удержания шестидесяти
				 * пяти тысяч блоков вразбивку - рабочий набор в девять мегабайт - при
				 * разных пределах: 1 МБ дал 18.32 нс/оп, 2 МБ - 17.45, 4 МБ - 16.06,
				 * 8 МБ - 12.68, 16 МБ - 9.97, 64 МБ - 9.66. Постоянные четыре мегабайта
				 * оказывались вдвое меньше рабочего набора, и кэш сновал к центральным
				 * спискам всю дорогу. Однопоточному приложению теперь достаётся весь
				 * запас, а шестнадцати потокам - те же четыре мегабайта на каждый
				 *
				 * Величина запаса взята у tcmalloc: там общий запас кэшей потоков тоже
				 * задан на процесс и по умолчанию того же порядка
				 *
				 * Прежние 64 МБ сняты на девятимегабайтном рабочем наборе и для него же
				 * запасны. Кодеки строят и рушат дерево в сотни мегабайт: при 64 МБ кэш
				 * перебирает запас всю дорогу и снуёт к центральным спискам. Поднятие до
				 * 256 МБ вернуло на освобождении крупной кучи подряд более чем вдвое
				 * (50 против 21 нс на действие)
				 */
				static constexpr size_t BUDGET = (256u * 1024u * 1024u);
				/**
				 * Наименьшая доля запаса, отводимая кэшу потока
				 *
				 * Доля, делённая на многие сотни потоков, обратилась бы в ничто, и кэш
				 * перестал бы работать кэшем вовсе: расход упёрся бы в запас, а скорость
				 * - в общий замок центральных списков. Ниже этой границы доля не идёт, и
				 * запас при многих потоках честно перебирается
				 */
				static constexpr size_t SHARE = (1u * 1024u * 1024u);
			private:
				// Общий на процесс запас кэшей потоков в байтах
				std::atomic <size_t> _budget;
				// Предел кэша потока в байтах
				/**
				 * Предел кэша потока НЕДЕЛИМ: читается он не только под замком
				 *
				 * Пишет его установщик настроек под замком общего списка, а `Caches::create`
				 * читает ПОСЛЕ выхода из-под замка - заведение кэша идёт вне него намеренно.
				 * Названо ThreadSanitizer'ом на щупе, где чужой поток правит предел, пока
				 * работники заводят себе кэши; разбор мой прежде говорил, будто поле это
				 * всюду под замком, и был неверен
				 */
				std::atomic <size_t> _limit;
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
				 * @return sign of successful creation
				 *
				 * \~
				 */
				cache_t * create() noexcept;
				/**
				 * \~russian
				 * @brief Метод раздачи доли общего запаса заведённым кэшам
				 *
				 * @note Зовётся ПОД ЗАМКОМ общего списка кэшей: перебирает его весь
				 *
				 * @note Ничего не делает, когда предел задан приложением: заданный предел
				 *       - предел каждого кэша порознь, и делить его не на что
				 *
				 * \~english
				 * @brief Method of handing out the shares of the common budget
				 *
				 * @note The budget is shared between all caches: each gets a fair share
				 *
				 * @note The budget is recalculated when caches are created or destroyed:
				 *       the share depends on the number of active caches and the total
				 *       limit set by the application. A single cache gets the whole
				 *       budget, and two caches get half each
				 *
				 * \~
				 */
				void reshare() noexcept;
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
				 * @param central central lists
				 * @param classes size classes
				 * @return        sign of successful initialization
				 *
				 * \~
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
				 * @note The key is created once per process and destroyed on exit:
				 *       no per-thread cleanup is needed
				 *
				 * @return sign of successful creation
				 *
				 * \~
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
				 * @note All caches are emptied and the pool is released
				 *
				 * \~
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
				 * @note The cache is taken from the pool or created fresh:
				 *       the pool is checked first to avoid allocation
				 *
				 * @return sign of successful creation
				 *
				 * \~
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
				 * @note The cache is created on first access for the thread:
				 *       no upfront allocation is needed, and threads that never allocate
				 *       never get a cache. The cache is destroyed on thread exit
				 *
				 * @return current thread cache or nullptr
				 *
				 * \~
				 */
				AWH_CACHE_INLINE cache_t * local() noexcept;
				/**
				 * \~russian
				 * @brief Метод отвязки кэша от завершившегося потока
				 *
				 * @param cache отвязываемый кэш
				 *
				 * \~english
				 * @brief Method of detaching a cache from a finished thread
				 *
				 * @param cache cache being detached
				 *
				 * \~
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
				 * @note The limit applies to new caches: existing ones keep their limit
				 *
				 * @param limit cache limit in bytes
				 *
				 * \~
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
				 * @param count number of initialized caches
				 * @return      amount held in the caches in bytes
				 *
				 * \~
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
				 * @note The value is approximate: it is read without locks, and other
				 *       threads may be adding or removing blocks at the same time.
				 *       The exact value is not needed for the purpose of this method
				 *
				 * @return accumulated value in bytes
				 *
				 * \~
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
				 * @return number of blocks actually returned
				 *
				 * \~
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
				 * \~
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения замка после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing the lock after forking
				 *
				 * \~
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
				 * @note The child gets a copy of the parent's caches with locks in an undefined
				 *       state: they must be reset before any use
				 *
				 * @note The cache pool is cleared: the child starts with fresh caches,
				 *       and any cache pointers held by the parent are invalid
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
				Caches() noexcept;
		} caches_t;
	};
};
