/**
 * @file queue.hpp
 * @date 2026-02-07
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
 * @brief Заголовочный файл сетевой очереди — класс Network_Queue с выравниванием на границу кэш-линии, обеспечивающий
 *        буферизацию исходящих и входящих сетевых данных фиксированной ёмкости без ложного разделения кэша между
 *        потоками
 *
 * \~english
 * @brief Header file of the network queue — the Network_Queue class with the alignment on the boundary of a cache line, providing
 *        the buffering of the outgoing and of the incoming network data of a fixed capacity without a false sharing of the cache between
 *        the threads
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NETWORK_QUEUE__
#define __AWH_NETWORK_QUEUE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstring>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/log.hpp"

/**
 * Если размер буфера в байтах не определён
 */
#ifndef AWH_NETWORK_QUEUE_BUFFER_SIZE
	/**
	 * \~russian
	 * Устанавливаем размер буфера для сетевой очереди (по умолчанию 64 КБ)
	 *
	 * @note Это ёмкость одного блока буфера очереди и одновременно верхняя граница объёма данных,
	 *       который очередь способна удержать.
	 *       Может быть переопределён на этапе сборки через -D.
	 *
	 * \~english
	 * Set the size of the buffer for the network queue (64 KB by default)
	 * @note This is the capacity of one block of the buffer of the queue and at the same time the upper boundary of the volume of the data,
	 *       which the queue is capable of holding.
	 *       May be redefined at the stage of the build through -D.
	 *
	 * \~
	 */
	#define AWH_NETWORK_QUEUE_BUFFER_SIZE 0x10000
#endif

/**
 * \~russian
 * Выравнивание отдельной очереди на границу кэш-линии для предотвращения ложного разделения
 *
 * @note Выравнивание ставится на месте объявления очереди, а не на самом классе.
 *       Очередь почти всегда лежит внутри узла события и обслуживается тем же
 *       потоком, что и соседние поля узла: делить строку кэша там не с кем, а
 *       выравнивание добивало очередь с 72 октетов до 128 и заставляло весь узел
 *       выравниваться на 64 - клиентский узел терял на этом 144 октета, узел
 *       принятого подключения 176. Ставить его следует там, где очередь
 *       действительно наполняется одним потоком, а разбирается другим
 *
 * \~english
 * The alignment of a separate queue on the boundary of a cache line for the prevention of the false sharing
 * @note The alignment is placed at the place of the declaration of the queue, and not on the class itself.
 *       The queue almost always lies inside a node of an event and is served by the same
 *       thread as the neighbouring fields of the node: there is nobody to share a cache line with there, and
 *       the alignment padded the queue from 72 octets up to 128 and forced the whole node
 *       to be aligned on 64 — a client node lost 144 octets on this,
 *       a node of an accepted connection 176. It should be placed where the queue
 *       is really filled by one thread, and disassembled by another
 *
 * \~
 */
#define __AWH_NETWORK_QUEUE_CACHELINE_ALIGN__ alignas(64)

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс для работы с сетевыми очередями (буфер исходящих данных соединения)
	 *
	 * @details
	 * Очередь используется как буфер обратного давления (backpressure) на стороне отправки: пока
	 * сокет/ограничитель скорости не готовы принять данные, они хранятся здесь, а приложение получает
	 * сигналы доступности места (см. QUEUE_OVERFLOW / QUEUE_AVAILABLE и метод available()).
	 *
	 * Внутри объединены две стратегии хранения, выбираемые по типу очереди (type_t):
	 *  - TCP (поток): bip-buffer (bipartite buffer) - два непрерывных региона A/B без сдвигов памяти
	 *    (memmove). front() отдаёт непрерывный прогон (потребитель дочитывает остаток следующим вызовом
	 *    после pop). available() возвращает наибольший непрерывный свободный регион - гарантированный
	 *    максимум одной записи. push() помещает СКОЛЬКО ВЛЕЗАЕТ и возвращает принятое: поток делится где
	 *    угодно, поэтому отказ целиком заставлял бы отправителя терять всю порцию вместо её остатка.
	 *  - UDP (границы сообщений): линейный буфер с заголовком размера (size_t) перед каждой записью и
	 *    дефрагментацией compact() при нехватке места в хвосте. Запись неделима (всё-или-ничего).
	 *
	 * Буфер выделяется лениво из потоко-локального пула блоков при первой записи и возвращается в пул
	 * при опустошении очереди (модель «один поток на воркер-процесс», пул без блокировок). Благодаря
	 * этому простаивающие соединения не удерживают память под буфер.
	 *
	 * @par Замеры (Apple Silicon, буфер 64 КБ, блок 1024 Б)
	 * Экономия памяти (RSS), 100 000 соединений, 5% под backpressure:
	 * Латентность push() под фрагментацией, 500 000 итераций (наносекунды):
	 * Воспроизведение бенчмарков:
	 *
	 * \~english
	 * @brief Class for working with the network queues (buffer of the outgoing data of a connection)
	 * @details
	 * The queue is used as a buffer of the backpressure at the side of the sending: while the
	 * socket/the limiter of the speed are not ready to accept the data, it is stored here, and the application receives
	 * the signals of the availability of the room (see QUEUE_OVERFLOW / QUEUE_AVAILABLE and the available() method).
	 * Inside two strategies of the storage are united, chosen by the type of the queue (type_t):
	 *  - TCP (a stream): a bip-buffer (bipartite buffer) — two continuous regions A/B without the shifts of the memory
	 *    (memmove). front() gives back a continuous run (the consumer reads the remainder by the next call
	 *    after pop). available() returns the largest continuous free region — the guaranteed
	 *    maximum of one record. push() places AS MUCH AS FITS and returns the accepted amount: a stream is divisible
	 *    anywhere, so a refusal in whole would force the sender to lose the entire portion instead of its remainder.
	 *  - UDP (the boundaries of the messages): a linear buffer with a header of the size (size_t) before every record and
	 *    a defragmentation compact() at a shortage of the room in the tail. A record is atomic (all-or-nothing).
	 * The buffer is allocated lazily from a thread-local pool of the blocks at the first writing and is returned into the pool
	 * at the emptying of the queue (the model «one thread per worker process», a pool without locks). Thanks to
	 * this the idling connections do not hold the memory for a buffer.
	 * @par Measurements (Apple Silicon, a buffer of 64 KB, a block of 1024 B)
	 * The economy of the memory (RSS), 100 000 connections, 5% under the backpressure:
	 * The latency of push() under the fragmentation, 500 000 iterations (nanoseconds):
	 * The reproduction of the benchmarks:
	 *
	 * \~
	 *
	 * @code
	 *   sizeof(Network_Queue):             128 байт (буфер вынесен в кучу)
	 *   100 000 пустых очередей:           +13.0 МБ  (буферы не выделены)
	 *   5 000 активных буферов:            +79.4 МБ
	 *   Эквивалент старой реализации:    6250.0 МБ  (inline 64 КБ x N)
	 *   Экономия:                            ~68x
	 * @endcode
	 * @code
	 *                 p50    p90    p99   p99.9    mean
	 *   TCP (bip):     41     42     42      84    28.8   (плоский хвост, без memmove)
	 *   UDP (linear):  41    500    583     709   103.9   (хвост от compact/memmove)
	 * @endcode
	 * @code
	 *   ./unit-tests/net --gtest_also_run_disabled_tests --gtest_filter='*RssFootprintBenchmark*'
	 *   ./unit-tests/net --gtest_also_run_disabled_tests --gtest_filter='*LatencyBenchmark*'
	 * @endcode
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Network_Queue {
		public:
			/**
			 * \~russian
			 * @brief Типы сетевых очередей
			 *
			 * \~english
			 * @brief Types of the network queues
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE = 0x00, // Тип очереди не установлен
				TCP  = 0x01, // Тип очереди для потоков данных
				UDP  = 0x02  // Тип очереди для границ сообщений
			};
		private:
			// Тип очереди
			type_t _type;
		private:
			// Позиция чтения (начало региона A); для bip-режима TCP - начало основного региона
			size_t _read;
			// Позиция записи (конец региона A); для bip-режима TCP - конец основного региона
			size_t _write;
			// Кэшированный размер полезных данных (без метаданных)
			size_t _total;
			// Количество записей в очереди (для TCP отслеживает количество байт)
			size_t _count;
			// Конец вторичного региона B (только bip-режим TCP, всегда начинается с 0); 0 = регион B неактивен
			size_t _bwrite;
		private:
			// Смещение участка, выданного методом tail() и ещё не подтверждённого методом commit(); отрицательного значения не бывает, признаком отсутствия выдачи служит _reserved
			size_t _offset;
			// Ёмкость выданного участка прямой записи (служит проверкой при подтверждении)
			size_t _capacity;
			// Признак выданного, но ещё не подтверждённого участка прямой записи
			bool _reserved;
			// Признак того, что выданный участок открывает вторичный регион B (только bip-режим TCP)
			bool _opening;
		private:
			// Буфер для хранения данных очереди (выделяется лениво из пула при первой записи)
			uint8_t * _buffer;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * \~russian
			 * @brief Метод сдвига всех данных к началу буфера при фрагментации (используется только для UDP)
			 *
			 * \~english
			 * @brief Method of shifting all the data to the beginning of the buffer at a fragmentation (is used only for UDP)
			 *
			 * \~
			 */
			void compact() noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод ленивого выделения буфера очереди из пула (если ещё не выделен)
			 *
			 * @return результат выделения буфера (false при нехватке памяти)
			 *
			 * \~english
			 * @brief Method of the lazy allocation of the buffer of the queue from the pool (if it is not allocated yet)
			 * @return result of the allocation of the buffer (false at a shortage of the memory)
			 *
			 * \~
			 */
			bool reserve() noexcept;
			/**
			 * \~russian
			 * @brief Метод возврата буфера очереди в пул (вызывается при опустошении очереди)
			 *
			 * \~english
			 * @brief Method of the return of the buffer of the queue into the pool (is called at the emptying of the queue)
			 *
			 * \~
			 */
			void release() noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод быстрого получения размера записи (без проверок - вызывается только для валидных позиций)
			 *
			 * @return размер данных в очереди
			 *
			 * \~english
			 * @brief Method of the fast getting of the size of a record (without the checks — is called only for the valid positions)
			 * @return size of the data in the queue
			 *
			 * \~
			 */
			size_t recordSize(const size_t pos) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки размера записи (прямой доступ)
			 *
			 * @param pos  позиция записи для обновления размера
			 * @param size новый размер данных в очереди
			 *
			 * \~english
			 * @brief Method of setting the size of a record (a direct access)
			 * @param pos  position of the record to update the size of
			 * @param size new size of the data in the queue
			 *
			 * \~
			 */
			void recordSize(const size_t pos, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки очереди от всех данных
			 *
			 * \~english
			 * @brief Method of clearing the queue of all the data
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки на пустоту очереди
			 *
			 * @details Определён в заголовке намеренно: проверка сводится к сравнению
			 *          поля с нулём, а вызов её из другой единицы трансляции встроить
			 *          нельзя. Отправка спрашивает очередь на каждом обращении, и
			 *          обмен короткими сообщениями платил за это обращением к чужому
			 *          модулю там, где хватает одного сравнения
			 *
			 * @return результат проверки на пустоту очереди
			 *
			 * \~english
			 * @brief Method of checking the queue for emptiness
			 * @details Is defined in the header deliberately: the check comes down to a comparison of
			 *          a field with zero, and a call of it from another translation unit cannot be
			 *          inlined. The sending asks the queue at every address, and
			 *          the exchange of short messages paid for this by an address to a foreign
			 *          module where one comparison is enough
			 * @return result of the check of the queue for emptiness
			 *
			 * \~
			 */
			bool empty() const noexcept {
				// Очередь пуста, когда в ней нет полезных данных (учитывает оба региона bip-режима)
				return (this->_total == 0);
			}
		public:
			/**
			 * \~russian
			 * @brief Метод получения общего размера полезных данных в очереди (без учёта метаданных)
			 *
			 * @return размер данных в очереди
			 *
			 * \~english
			 * @brief Method of getting the total size of the useful data in the queue (without the metadata taken into account)
			 * @return size of the data in the queue
			 *
			 * \~
			 */
			size_t size() const noexcept {
				// Возвращаем кэшированный размер полезных данных в очереди
				return this->_total;
			}
		public:
			/**
			 * \~russian
			 * @brief Метод получения количества записей в очереди
			 *
			 * @return количество записей в очереди
			 *
			 * \~english
			 * @brief Method of getting the number of the records in the queue
			 * @return number of the records in the queue
			 *
			 * \~
			 */
			size_t count() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определения доступного пространства для новых данных (в байтах полезной нагрузки)
			 *
			 * @note   Возвращает размер наибольшего непрерывного свободного региона: для TCP (bip) это гарантированный максимум одной записи, для UDP - свободное место за вычетом заголовка.
			 * @return доступное пространство для новых данных в очереди
			 *
			 * \~english
			 * @brief Method of the determination of the available room for the new data (in the bytes of the payload)
			 * @note   Returns the size of the largest continuous free region: for TCP (bip) this is the guaranteed maximum of one record, for UDP — the free room minus the header.
			 * @return available room for the new data in the queue
			 *
			 * \~
			 */
			size_t available() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения типа сетевой очереди
			 *
			 * @return тип сетевой очереди
			 *
			 * \~english
			 * @brief Method of getting the type of the network queue
			 * @return type of the network queue
			 *
			 * \~
			 */
			type_t type() const noexcept;
			/**
			 * \~russian
			 * @brief Метод Установки типа сетевой очереди
			 *
			 * \~english
			 * @brief Method of setting the type of the network queue
			 *
			 * \~
			 */
			void type(const type_t type) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод удаления верхней записи из очереди
			 *
			 * @param size размер данных для удаления из очереди
			 * @return     результат удаления верхней записи из очереди (true при успехе, false если очередь пуста)
			 *
			 * \~english
			 * @brief Method of removing the top record from the queue
			 * @param size size of the data to remove from the queue
			 * @return     result of the removal of the top record from the queue (true at a success, false if the queue is empty)
			 *
			 * \~
			 */
			bool pop(const size_t size = 0) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод добавления данных в очередь
			 *
			 * @param data данные для добавления в очередь
			 * @param size размер данных для добавления в очередь
			 * @return     количество данных, успешно добавленных в очередь (0 при неудаче, когда недостаточно места)
			 *
			 * \~english
			 * @brief Method of adding the data into the queue
			 * @param data data to add into the queue
			 * @param size size of the data to add into the queue
			 * @return     amount of the data successfully added into the queue (0 at a failure, when there is not enough room)
			 *
			 * \~
			 */
			size_t push(const void * data, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения хвостового участка очереди для прямой записи (без промежуточного копирования)
			 *
			 * @details Служит вытягивающей модели отправки: источник данных пишет прямо в очередь,
			 *          а не в свой буфер с последующим копированием методом push(). Выданный участок
			 *          обязан быть подтверждён методом commit() либо отменён передачей нуля в него;
			 *          до подтверждения очередь считает участок занятым и повторной выдачи не делает
			 *
			 * @param data указатель на начало участка для записи (устанавливается методом)
			 * @param size ёмкость выданного участка в байтах (устанавливается методом)
			 * @return     результат выдачи участка (false при отсутствии места либо при неподтверждённой прежней выдаче)
			 *
			 * \~english
			 * @brief Method of getting the tail region of the queue for the direct writing (without an intermediate copying)
			 * @param data pointer to the beginning of the region for the writing (is set by the method)
			 * @param size capacity of the given region in the bytes (is set by the method)
			 * @return     result of the giving of the region (false at an absence of the room or at an unconfirmed previous giving)
			 *
			 * \~
			 */
			bool tail(void ** data, size_t & size) noexcept;
			/**
			 * \~russian
			 * @brief Метод подтверждения данных, записанных прямо в хвостовой участок очереди
			 *
			 * @note Передача нуля отменяет выдачу участка, не изменяя содержимого очереди
			 *
			 * @param size количество действительно записанных байт (не более ёмкости выданного участка)
			 * @return     результат подтверждения (false при отсутствии выдачи либо при превышении ёмкости)
			 *
			 * \~english
			 * @brief Method of the confirmation of the data written directly into the tail region of the queue
			 * @param size amount of the really written bytes (not more than the capacity of the given region)
			 * @return     result of the confirmation (false at an absence of the giving or at an excess of the capacity)
			 *
			 * \~
			 */
			bool commit(const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения данных из очереди (без удаления - для чтения)
			 *
			 * @param data данные для получения из очереди (устанавливается указатель на данные в очереди)
			 * @param size размер данных для получения из очереди
			 * @return     результат (true при успехе, false если очередь пуста)
			 *
			 * \~english
			 * @brief Method of getting the data from the queue (without the removal — for the reading)
			 * @param data data to get from the queue (a pointer to the data in the queue is set)
			 * @param size size of the data to get from the queue
			 * @return     result (true at a success, false if the queue is empty)
			 *
			 * \~
			 */
			bool front(const void ** data, size_t & size) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Запрещаем оператор перемещения очереди (объект владеет буфером, выделенным из пула)
			 *
			 * \~english
			 * @brief We forbid the move operator of the queue (the object owns a buffer allocated from the pool)
			 *
			 * \~
			 */
			Network_Queue & operator = (Network_Queue &&) = delete;
			/**
			 * \~russian
			 * @brief Запрещаем оператор копирования очереди (объект владеет буфером, выделенным из пула)
			 *
			 * \~english
			 * @brief We forbid the copy operator of the queue (the object owns a buffer allocated from the pool)
			 *
			 * \~
			 */
			Network_Queue & operator = (const Network_Queue &) = delete;
		public:
			/**
			 * \~russian
			 * @brief Запрещаем перемещение очереди (объект владеет буфером, выделенным из пула)
			 *
			 * \~english
			 * @brief We forbid the moving of the queue (the object owns a buffer allocated from the pool)
			 *
			 * \~
			 */
			Network_Queue(Network_Queue &&) = delete;
			/**
			 * \~russian
			 * @brief Запрещаем копирование очереди (объект владеет буфером, выделенным из пула)
			 *
			 * \~english
			 * @brief We forbid the copying of the queue (the object owns a buffer allocated from the pool)
			 *
			 * \~
			 */
			Network_Queue(const Network_Queue &) = delete;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Network_Queue(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Network_Queue() noexcept;
	} net_queue_t;
};

#endif // __AWH_NETWORK_QUEUE__
