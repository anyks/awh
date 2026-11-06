/**
 * @file connection.hpp
 * @date 2026-07-21
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
 * @brief Заголовочный файл конечного автомата соединения QUIC — класс quic::Connection,
 *        управляющий пространствами номеров пакетов, потоками приложения, контролем перегрузки и потока, оценкой RTT,
 *        обнаружением потерь, миграцией пути и завершением соединения (RFC 9000, RFC 9002)
 *
 * \~english
 * @brief Header file of the finite automaton of a QUIC connection — the class quic::Connection
 *        controlling the spaces of the numbers of the packets, the streams of the application, the congestion and the flow control, the estimation of the RTT,
 *        the detection of the losses, the migration of the path and the completion of the connection (RFC 9000, RFC 9002)
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <new>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <functional>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "quic.hpp"
#include "frame.hpp"
#include "crypto.hpp"
#include "smallvec.hpp"
#include "handshake.hpp"
#include "../../cryptography/tls/coder.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 * \~english
	 * @brief QUIC transport protocol namespace
	 *
	 * \~
	 */
	namespace quic {
		/**
		 * \~russian
		 * @brief Сегментированный FIFO-буфер собранных данных потока на переиспользуемых блоках
		 *
		 * @details Собранные данные потока копятся до выдачи приложению. Непрерывная строка
		 *          наращивается удвоением ёмкости - чередой перевыделений с копированием,
		 *          дающей многократный кумулятивный расход памяти на длинных передачах.
		 *          Здесь данные хранятся цепочкой блоков фиксированного размера, взятых из
		 *          общего пула соединения и возвращаемых в него при сливе: блоки выделяются
		 *          один раз и переиспользуются между потоками, поэтому рост буфера
		 *          перевыделений не порождает. Семантика чисто FIFO: дозапись в хвост и слив
		 *          целиком с начала - частичного среза с головы у выдачи данных потока нет.
		 *
		 * \~english
		 * @brief Segmented FIFO buffer of the assembled data of a stream on the reused blocks
		 *
		 * @details The assembled data of a stream accumulates until the issue to the application. A continuous string
		 *          is grown by a doubling of the capacity - a succession of the reallocations with a copying
		 *          giving a multiple cumulative expenditure of the memory on the long transmissions.
		 *          Here the data is stored by a chain of the blocks of a fixed size taken from
		 *          a common pool of the connection and returned into it at a draining: the blocks are allotted
		 *          once and are reused between the streams, therefore a growth of the buffer
		 *          generates no reallocations. The semantics is purely FIFO: an appending into the tail and a draining
		 *          entirely from the beginning - there is no partial slice from the head at the issue of the data of a stream.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Chunked_FIFO {
			public:
				// Размер блока данных в октетах
				static constexpr size_t BLOCK = 16384;
				// Предельное число блоков, удерживаемых пулом (сверх предела блоки освобождаются)
				static constexpr size_t POOL_LIMIT = 1024;
			private:
				// Суммарный логический размер накопленных данных в октетах
				size_t _bytes;
				// Цепочка блоков данных (последний - текущий хвостовой, возможно неполный)
				vector <string> _blocks;
			private:
				/**
				 * \~russian
				 * @brief Метод получения блока из пула либо выделения нового
				 *
				 * @param pool пул переиспользуемых блоков
				 * @return     блок с сохранённой либо зарезервированной ёмкостью
				 *
				 * \~english
				 * @brief Method of getting a block from the pool or of allotting a new one
				 *
				 * @param pool pool of the reused blocks
				 * @return     block with a preserved or a reserved capacity
				 *
				 * \~
				 */
				static string acquire(vector <string> & pool) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата блока в пул либо его освобождения сверх предела
				 *
				 * @param pool  пул переиспользуемых блоков
				 * @param block возвращаемый блок
				 *
				 * \~english
				 * @brief Method of the return of a block into the pool or of its release above the limit
				 *
				 * @param pool  pool of the reused blocks
				 * @param block block being returned
				 *
				 * \~
				 */
				static void release(vector <string> & pool, string && block) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки отсутствия накопленных данных
				 *
				 * @return признак отсутствия данных
				 *
				 * \~english
				 * @brief Method of checking the absence of the accumulated data
				 *
				 * @return flag of the absence of the data
				 *
				 * \~
				 */
				bool empty() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения суммарного размера накопленных данных
				 *
				 * @return суммарный размер данных в октетах
				 *
				 * \~english
				 * @brief Method of getting the total size of the accumulated data
				 *
				 * @return total size of the data in octets
				 *
				 * \~
				 */
				size_t size() const noexcept;
				/**
				 * \~russian
				 * @brief Метод дозаписи данных в хвост буфера
				 *
				 * @param pool пул переиспользуемых блоков
				 * @param data дописываемые данные
				 *
				 * \~english
				 * @brief Method of the appending of the data into the tail of the buffer
				 *
				 * @param pool pool of the reused blocks
				 * @param data data being appended
				 *
				 * \~
				 */
				void append(vector <string> & pool, string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Метод слива всех накопленных данных в выходной буфер
				 *
				 * @param pool   пул переиспользуемых блоков (блоки возвращаются в него)
				 * @param output выходной буфер (данные дописываются)
				 *
				 * \~english
				 * @brief Method of the draining of all the accumulated data into the output buffer
				 *
				 * @param pool   pool of the reused blocks (the blocks are returned into it)
				 * @param output output buffer (the data is appended)
				 *
				 * \~
				 */
				void drain(vector <string> & pool, string & output) noexcept;
				/**
				 * \~russian
				 * @brief Метод отбрасывания всех накопленных данных
				 *
				 * @param pool пул переиспользуемых блоков (блоки возвращаются в него)
				 *
				 * \~english
				 * @brief Method of the discarding of all the accumulated data
				 *
				 * @param pool pool of the reused blocks (the blocks are returned into it)
				 *
				 * \~
				 */
				void clear(vector <string> & pool) noexcept;
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
				explicit Chunked_FIFO() noexcept;
		} chunked_fifo_t;
		/**
		 * \~russian
		 * @brief Класс соединения QUIC (RFC 9000 §5)
		 *
		 * @details Слой соединения над хендшейк-машиной: приём и сборка UDP-датаграмм
		 *          целиком - разбор коалесцированных пакетов, снятие/установка защиты,
		 *          диспетчеризация фреймов, пространства номеров пакетов, генерация
		 *          подтверждений (ACK), CRYPTO-потоки со сборкой по смещениям и
		 *          завершение соединения (CONNECTION_CLOSE), восстановление потерь
		 *          по RFC 9002 (оценка RTT, детект потерь по порогам номера и
		 *          времени, таймер PTO с ретрансмиссией), а также потоки приложения
		 *          (RFC 9000 §2-§4): открытие потоков, отправка и сборка данных
		 *          STREAM по смещениям, flow control соединения и потоков
		 *          (MAX_DATA/MAX_STREAM_DATA), лимиты потоков (MAX_STREAMS),
		 *          аварийное завершение (RESET_STREAM/STOP_SENDING). Работает
		 *          без ввода-вывода и собственных таймеров (sans-IO): датаграммы
		 *          передаются через read(), исходящие извлекаются через write(),
		 *          текущее время передаётся параметром, дедлайн ближайшего события
		 *          отдаёт timeout(), просроченные таймеры обрабатывает tick().
		 *
		 * \~english
		 * @brief Class of a QUIC connection (RFC 9000 §5)
		 *
		 * @details A layer of the connection over the handshake machine: the acceptance and the assembly of the UDP datagrams
		 *          entirely - the parsing of the coalesced packets, the removal/the installation of the protection,
		 *          the dispatching of the frames, the spaces of the numbers of the packets, the generation
		 *          of the acknowledgements (ACK), the CRYPTO streams with an assembly by the displacements and
		 *          the completion of the connection (CONNECTION_CLOSE), the recovery of the losses
		 *          by RFC 9002 (the estimation of the RTT, the detection of the losses by the thresholds of the number and
		 *          of the time, the timer PTO with a retransmission), and also the streams of the application
		 *          (RFC 9000 §2-§4): the opening of the streams, the sending and the assembly of the data
		 *          of a STREAM by the displacements, the flow control of the connection and of the streams
		 *          (MAX_DATA/MAX_STREAM_DATA), the limits of the streams (MAX_STREAMS),
		 *          the emergency completion (RESET_STREAM/STOP_SENDING). It works
		 *          without an input-output and without the timers of its own (sans-IO): the datagrams
		 *          are transmitted through read(), the outgoing ones are extracted through write(),
		 *          the current time is transmitted by a parameter, the deadline of the nearest event
		 *          is issued by timeout(), the expired timers are processed by tick().
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Connection {
			public:
				/**
				 * \~russian
				 * @brief Состояния соединения
				 *
				 * \~english
				 * @brief States of the connection
				 *
				 * \~
				 */
				enum class state_t : uint8_t {
					NONE        = 0x00, // Соединение не начато
					HANDSHAKING = 0x01, // Выполняется хендшейк
					CONNECTED   = 0x02, // Соединение установлено
					CLOSING     = 0x03, // Локальный эндпоинт завершил соединение и выдерживает период завершения
					DRAINING    = 0x04  // Соединение завершено: удалённым эндпоинтом либо по истечении периода завершения
				};
			public:
				/**
				 * \~russian
				 * @brief Тип pull-источника данных потока (для больших тел без лишней копии)
				 *
				 * @details Альтернатива send(): движок сам запрашивает у источника данные ровно
				 *          тогда, когда открыто окно и есть место в буфере отправки потока ниже
				 *          верхней водяной метки. Источник пишет данные напрямую в переданный
				 *          буфер (не более cap байт), выставляет eof = true по достижении конца
				 *          тела и возвращает число записанных байт, либо -1 при ошибке (поток
				 *          будет аварийно завершён RESET_STREAM). Приложение не держит копию всего
				 *          тела - генерирует данные по мере ухода в сеть
				 *
				 * @param sid    идентификатор потока
				 * @param buffer буфер для заполнения (хвост буфера отправки потока)
				 * @param cap    ёмкость буфера
				 * @param eof    флаг достижения конца тела
				 * @return       число записанных байт либо -1 при ошибке
				 *
				 * \~english
				 * @brief Type of the pull source of the data of a stream (for the big bodies without a superfluous copy)
				 *
				 * @details An alternative to send(): the engine itself requests the data from the source exactly
				 *          then when the window is open and there is a place in the buffer of the sending of the stream below
				 *          the upper water mark. The source writes the data directly into the transmitted
				 *          buffer (not more than cap octets), sets eof = true at the reaching of the end
				 *          of the body and returns the number of the written octets, or -1 at an error (the stream
				 *          will be emergently completed by a RESET_STREAM). The application does not hold a copy of the whole
				 *          body - it generates the data as it goes away into the network
				 *
				 * @param sid    identifier of the stream
				 * @param buffer buffer for the filling (the tail of the buffer of the sending of the stream)
				 * @param cap    capacity of the buffer
				 * @param eof    flag of the reaching of the end of the body
				 * @return       number of the written octets or -1 at an error
				 *
				 * \~
				 */
				using data_source_callback_t = function <int64_t (const uint64_t, uint8_t *, const size_t, bool &)>;
			public:
				/**
				 * \~russian
				 * @brief Максимальный размер исходящей UDP-датаграммы (RFC 9000 §14.1)
				 *
				 * \~english
				 * @brief Largest size of an outgoing UDP datagram (RFC 9000 §14.1)
				 *
				 * \~
				 */
				static constexpr size_t MAX_DATAGRAM_SIZE = 1200;
				/**
				 * \~russian
				 * @brief Лимит буфера сборки CRYPTO-потока одного уровня (RFC 9000 §7.5)
				 *
				 * \~english
				 * @brief Limit of the buffer of the assembly of the CRYPTO stream of a single level (RFC 9000 §7.5)
				 *
				 * \~
				 */
				static constexpr size_t MAX_CRYPTO_BUFFER = 65536;
				/**
				 * \~russian
				 * @brief Длина генерируемых идентификаторов соединения (RFC 9000 §7.2 требует не менее 8 для DCID клиента)
				 *
				 * @note Длина одинакова у всех выдаваемых идентификаторов: пакеты
				 *       с коротким заголовком поля длины не несут, и разобрать
				 *       идентификатор получателя можно только зная её заранее
				 *
				 * \~english
				 * @brief Length of the generated identifiers of the connection (RFC 9000 §7.2 requires not less than 8 for the DCID of a client)
				 *
				 * @note The length is the same at all the issued identifiers: the packets
				 *       with a short header do not carry a field of the length, and to parse
				 *       the identifier of the receiver is possible only knowing it beforehand
				 *
				 * \~
				 */
				static constexpr size_t LOCAL_CID_SIZE = 8;
				/**
				 * \~russian
				 * @brief Предельный размер исходящей датаграммы при поиске размера пути (RFC 8899)
				 *
				 * @note Выбран по типовому размеру кадра Ethernet за вычетом заголовков
				 *       IPv6 и UDP: на пути с IPv4-заголовком запас лишь увеличивается.
				 *       Датаграмма сверх этого размера не отправляется никогда,
				 *       включая зонды размера пути
				 *
				 * \~english
				 * @brief Limiting size of an outgoing datagram at the search of the size of the path (RFC 8899)
				 *
				 * @note It is chosen by the typical size of an Ethernet frame minus the headers
				 *       of IPv6 and UDP: on a path with an IPv4 header the reserve only increases.
				 *       A datagram above this size is never sent,
				 *       including the probes of the size of the path
				 *
				 * \~
				 */
				static constexpr size_t MAX_PROBE_SIZE = 1452;
			private:
				/**
				 * \~russian
				 * @brief Количество пространств номеров пакетов (RFC 9000 §12.3)
				 *
				 * \~english
				 * @brief Number of the spaces of the numbers of the packets (RFC 9000 §12.3)
				 *
				 * \~
				 */
				static constexpr size_t SPACES = 3;
				/**
				 * \~russian
				 * @brief Пространство номеров пакетов (RFC 9000 §12.3)
				 *
				 * \~english
				 * @brief Space of the numbers of the packets (RFC 9000 §12.3)
				 *
				 * \~
				 */
				enum class space_t : uint8_t {
					INITIAL     = 0x00, // Пространство пакетов Initial
					HANDSHAKE   = 0x01, // Пространство пакетов Handshake
					APPLICATION = 0x02  // Пространство пакетов приложения (0-RTT и 1-RTT)
				};
			private:
				/**
				 * \~russian
				 * @brief Структура отправленного блока данных потока приложения (для ретрансмиссии)
				 *
				 * \~english
				 * @brief Structure of a sent block of the data of a stream of the application (for a retransmission)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Chunk {
					// Идентификатор потока
					uint64_t sid;
					// Смещение данных в потоке
					uint64_t offset;
					// Длина отправленных данных в октетах
					uint64_t size;
					// Флаг завершения потока (FIN)
					bool fin;
					/**
					 * \~russian
					 * @brief Данные потока приложения. На горячем пути свежей отправки не
					 *        заполняются: учётная запись пакета ссылается на буфер отправки
					 *        потока (sid+offset+size), а сам буфер удерживается до подтверждения.
					 *        Заполняются лишь при постановке в очередь ретрансмиссии после
					 *        потери - перечитыванием из буфера отправки потока (RFC 9000 §13.3).
					 *
					 * \~english
					 * @brief Data of a stream of the application. On a hot path of a fresh sending they are not
					 *        filled: an account record of a packet refers to the send buffer of the
					 *        stream (sid+offset+size), and the buffer itself is held up to the acknowledgment.
					 *        Filled only when putting into the queue of the retransmission after a loss -
					 *        by rereading from the send buffer of the stream (RFC 9000 §13.3).
					 *
					 * \~
					 */
					string data;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Chunk() noexcept;
				} chunk_t;
				/**
				 * \~russian
				 * @brief Структура учёта отправленного ack-eliciting пакета (RFC 9002 §A.1)
				 *
				 * \~english
				 * @brief Structure of the account of a sent ack-eliciting packet (RFC 9002 §A.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Sent {
					// Номер отправленного пакета
					uint64_t pn;
					// Время отправки пакета в миллисекундах
					uint64_t time;
					// Размер пакета в октетах (для congestion control)
					size_t size;
					// Флаг наличия фрейма HANDSHAKE_DONE в пакете
					bool handshakeDone;
					/**
					 * \~russian
					 * @brief Флаг отправки пакета на уровне ранних данных. Уровни ранних данных
					 *        и приложения делят одно пространство номеров пакетов, а различать
					 *        их необходимо: при отказе удалённого узла в ранних данных
					 *        содержимое ранних пакетов подлежит повторной отправке
					 *        защитой уровня приложения (RFC 9001 §4.6.2)
					 *
					 * \~english
					 * @brief Flag of sending of a packet at the level of the early data. The levels of the early
					 *        data and of the application share one space of the numbers of the packets, and to
					 *        distinguish them is necessary: at a refusal of the remote node in the early data
					 *        the contents of the early packets is subject to the repeated sending by
					 *        the protection of the level of the application (RFC 9001 §4.6.2)
					 *
					 * \~
					 */
					bool early;
					/**
					 * \~russian
					 * @brief Флаг отправки пакета с маркировкой поддержки ECN. Проверка пути
					 *        сверяет счётчики маркировок удалённого узла с числом подтверждённых
					 *        помеченных пакетов, поэтому маркировка запоминается пакетно
					 *        (RFC 9000 §13.4.2.1)
					 *
					 * \~english
					 * @brief Flag of sending of a packet with a marking of the ECN support. The path validation
					 *        compares the counters of the markings of the remote node with the number of the acknowledged
					 *        marked packets, therefore a marking is remembered per packet
					 *        (RFC 9000 §13.4.2.1)
					 *
					 * \~
					 */
					bool ecn;
					/**
					 * \~russian
					 * @brief Флаг отправки пакета зондом размера пути. Потеря зонда признаком
					 *        перегрузки не является: пакет отброшен как не помещающийся
					 *        в путь, а не из-за затора (RFC 9000 §14.4)
					 *
					 * \~english
					 * @brief Flag of sending of a packet by a probe of a path size. A loss of a probe is not
					 *        a sign of an overload: the packet is dropped as not fitting
					 *        into the path, and not because of a jam (RFC 9000 §14.4)
					 *
					 * \~
					 */
					bool pmtu;
					/**
					 * \~russian
					 * @brief Флаг отправки пакета по прежнему пути соединения. Окно перегрузки и
					 *        оценка задержки характеризуют конкретный путь, поэтому подтверждения
					 *        и потери пакетов прежнего пути на состояние нового не влияют: их
					 *        содержимое переотправляется, но контроль перегрузки они не трогают
					 *        (RFC 9000 §9.4)
					 *
					 * \~english
					 * @brief Flag of sending of a packet by the previous path of the connection. The congestion window and
					 *        the estimation of the delay characterise a concrete path, therefore the acknowledgments
					 *        and the losses of the packets of the previous path do not affect the state of the new one: their
					 *        contents is retransmitted, but the congestion control they do not touch
					 *        (RFC 9000 §9.4)
					 *
					 * \~
					 */
					bool stale;
					// Отправленные блоки данных потоков приложения (для ретрансмиссии): инлайн-хранилище на типовой пакет
					small_vector_t <chunk_t, 2> stream;
					// Отправленные CRYPTO-данные пакета со смещениями (для ретрансмиссии)
					vector <std::pair <uint64_t, string>> crypto;
					// Отправленные управляющие фреймы: тип и идентификатор потока (для повтора при потере)
					vector <std::pair <frame_t, uint64_t>> control;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Sent() noexcept;
				} sent_t;
				/**
				 * \~russian
				 * @brief Структура состояния одного потока приложения (RFC 9000 §3)
				 *
				 * \~english
				 * @brief Structure of the state of a single stream of the application (RFC 9000 §3)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Stream {
					// Смещение начала буфера исходящих данных в потоке
					uint64_t txOffset;
					// Буфер исходящих данных потока (ожидают упаковки в пакеты)
					string txBuffer;
					/**
					 * \~russian
					 * @brief Pull-источник данных тела потока (пусто - не задан). Если задан, движок сам
					 *        дозаполняет txBuffer из источника при упаковке, пока не открыт eof; ставится
					 *        методом dataSource(). Альтернатива постановке данных через send()
					 *
					 * \~english
					 * @brief Pull-source of the data of the stream body (empty - not set). If set, the engine itself
					 *        refills the txBuffer from the source on packing, while the eof is not opened; set by
					 *        the dataSource() method. An alternative to putting the data through send()
					 *
					 * \~
					 */
					data_source_callback_t txSource;
					/**
					 * \~russian
					 * @brief Количество уже упакованных октетов в начале буфера. Упакованные
					 *        данные не вырезаются сразу: вырезание сдвигает весь остаток буфера,
					 *        что даёт квадратичную стоимость на длинных передачах. Освобождение
					 *        выполняется уплотнением, когда потреблённая часть занимает
					 *        существенную долю буфера
					 *
					 * \~english
					 * @brief Number of the already packed octets at the beginning of the buffer. The packed
					 *        data are not cut out at once: a cutting shifts the whole remainder of the buffer,
					 *        which gives a quadratic cost on long transfers. The release
					 *        is performed by a compaction when the consumed part occupies
					 *        a significant share of the buffer
					 *
					 * \~
					 */
					size_t txCursor;
					/**
					 * \~russian
					 * @brief Смещение непрерывно подтверждённого префикса отправленных данных.
					 *        Буфер отправки удерживает отправленные, но ещё не подтверждённые
					 *        данные для их ретрансмиссии по ссылке из учётной записи пакета,
					 *        а уплотнением освобождается лишь подтверждённая часть (RFC 9000 §13.3)
					 *
					 * \~english
					 * @brief Offset of the continuously acknowledged prefix of the sent data.
					 *        The send buffer holds the sent but not yet acknowledged
					 *        data for their retransmission by a reference from an account record of a packet,
					 *        and a compaction releases only the acknowledged part (RFC 9000 §13.3)
					 *
					 * \~
					 */
					uint64_t txAcked;
					/**
					 * \~russian
					 * @brief Подтверждённые блоки отправленных данных, идущие не по порядку.
					 *        Отправленные блоки примыкают вплотную и не пересекаются, поэтому
					 *        ключ - смещение начала блока, значение - смещение его конца:
					 *        подтверждённый префикс продвигается по цепочке примыкающих блоков
					 *
					 * \~english
					 * @brief The acknowledged blocks of the sent data going not in order.
					 *        The sent blocks adjoin tightly and do not intersect, therefore the
					 *        key is the offset of the beginning of a block, the value is the offset of its end:
					 *        the acknowledged prefix moves along the chain of the adjoining blocks
					 *
					 * \~
					 */
					map <uint64_t, uint64_t> txPending;
					// Лимит отправки потока от удалённого эндпоинта (MAX_STREAM_DATA)
					uint64_t txMax;
					// Флаг постановки завершения потока приложением (FIN)
					bool txFin;
					// Флаг выполненной отправки завершения потока (FIN)
					bool txFinSent;
					// Флаг необходимости отправки фрейма RESET_STREAM
					bool txReset;
					// Флаг выполненной отправки фрейма RESET_STREAM
					bool txResetSent;
					// Код ошибки приложения фрейма RESET_STREAM
					uint64_t txResetCode;
					// Флаг необходимости отправки фрейма STREAM_DATA_BLOCKED
					bool txBlocked;
					// Лимит, о блокировке которым уже уведомлён удалённый эндпоинт
					uint64_t txBlockedAt;
					/**
					 * \~russian
					 * @brief Флаг поданного сигнала writable для текущего заполнения буфера отправки.
					 *        Взводится (false) при частичном приёме в send() - буфер достиг верхней
					 *        водяной метки; сбрасывается подачей сигнала при падении буфера ниже нижней
					 *        метки, чтобы на одно заполнение приходился ровно один сигнал возобновления
					 *
					 * \~english
					 * @brief Flag of the already given signal writable for the current filling of the send buffer.
					 *        Raised (false) at a partial acceptance in send() - the buffer reached the upper
					 *        high-water mark; dropped by giving the signal when the buffer falls below the lower
					 *        mark, so that one signal of the resumption falls on one filling
					 *
					 * \~
					 */
					bool writableNotified;
					// Смещение непрерывно собранных входящих данных потока
					uint64_t rxOffset;
					// Наибольшее принятое смещение данных потока (для flow control)
					uint64_t rxHigh;
					// Буфер сборки входящих данных потока по смещениям
					map <uint64_t, string> rxBuffer;
					// Собранные непрерывные данные потока на переиспользуемых блоках (ожидают выдачи приложению)
					chunked_fifo_t rxReady;
					// Анонсированный лимит приёма потока (MAX_STREAM_DATA)
					uint64_t rxMax;
					// Флаг необходимости отправки обновлённого лимита MAX_STREAM_DATA
					bool rxMaxQueued;
					// Флаг наличия финального размера потока (принят FIN или RESET_STREAM)
					bool rxFin;
					// Финальный размер потока в октетах
					uint64_t rxFinal;
					// Флаг выданного приложению завершения потока (FIN)
					bool rxFinDelivered;
					// Количество данных потока, учтённых потреблёнными в flow control соединения
					uint64_t rxCounted;
					// Флаг аварийного завершения потока удалённым эндпоинтом (RESET_STREAM)
					bool rxReset;
					// Код ошибки приложения принятого фрейма RESET_STREAM
					uint64_t rxResetCode;
					// Флаг необходимости отправки фрейма STOP_SENDING
					bool stopQueued;
					// Флаг выполненной отправки фрейма STOP_SENDING
					bool stopSent;
					// Код ошибки приложения фрейма STOP_SENDING
					uint64_t stopCode;
					// Флаг учтённого завершения потока в лимите MAX_STREAMS
					bool credited;
					/**
					 * \~russian
					 * @brief Флаг присутствия потока в списке готовых к выдаче. Означает
					 *        именно членство в списке, а не готовность: готовность
					 *        перевычисляется при чтении списка, поэтому поток попадает
					 *        в него не более одного раза
					 *
					 * \~english
					 * @brief Flag of the presence of a stream in the list of those ready for issuing. Means
					 *        exactly a membership in the list, and not a readiness: the readiness
					 *        is recomputed on reading the list, therefore a stream gets into
					 *        it not more than once
					 *
					 * \~
					 */
					bool queued;
					/**
					 * \~russian
					 * @brief Флаг присутствия потока в списке ожидающих управляющих фреймов.
					 *        По аналогии с `queued` означает членство в списке, а не наличие
					 *        фрейма: наличие перевычисляется при обходе списка, поэтому поток
					 *        попадает в него не более одного раза, а обход управляющих фреймов
					 *        идёт по нему вместо всего списка потоков
					 *
					 * \~english
					 * @brief Flag of the presence of a stream in the list of those waiting for the control frames.
					 *        By analogy with `queued` means a membership in the list, and not the presence
					 *        of a frame: the presence is recomputed on walking the list, therefore a stream
					 *        gets into it not more than once, and the walk over the control frames
					 *        goes by it instead of the whole list of the streams
					 *
					 * \~
					 */
					bool controlPending;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Stream() noexcept;
				} stream_data_t;
				/**
				 * \~russian
				 * @brief Структура состояния одного пространства номеров пакетов
				 *
				 * \~english
				 * @brief Structure of the state of a single space of the numbers of the packets
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Space {
					// Номер следующего отправляемого пакета
					uint64_t txPn;
					// Наибольший номер пакета, подтверждённый пиром
					uint64_t largestAcked;
					// Флаг наличия подтверждений от пира
					bool hasAcked;
					// Наибольший принятый номер пакета
					uint64_t largestRx;
					// Флаг приёма хотя бы одного пакета
					bool hasRx;
					/**
					 * \~russian
					 * @brief Битовая маска приёма 64 номеров пакетов ниже наибольшего принятого:
					 *        бит i соответствует номеру largestRx - (i + 1). Служит защитой от
					 *        повторной обработки пакета и не зависит от прореживания списка
					 *        диапазонов ranges, который хранится только для сборки фреймов ACK
					 *
					 * \~english
					 * @brief Bit mask of the acceptance of 64 numbers of the packets below the largest received:
					 *        bit i corresponds to the number largestRx - (i + 1). Serves as a protection
					 *        against a repeated processing of a packet and does not depend on the thinning of the list
					 *        of the ranges, which is stored only for the assembling of the ACK frames
					 *
					 * \~
					 */
					uint64_t dedup;
					// Флаг необходимости отправки подтверждения (принят ack-eliciting пакет)
					bool ackElicited;
					// Флаг необходимости немедленного подтверждения без задержки (принят пакет вне очереди, RFC 9000 §13.2.1)
					bool ackImmediate;
					/**
					 * \~russian
					 * @brief Время приёма ack-eliciting пакета, породившего необходимость
					 *        подтверждения. Разница с временем отправки подтверждения
					 *        кодируется полем Ack Delay фрейма ACK (RFC 9000 §19.3)
					 *
					 * \~english
					 * @brief Time of the acceptance of an ack-eliciting packet that gave rise to the necessity
					 *        of the acknowledgment. The difference with the time of sending the acknowledgment
					 *        is encoded by the Ack Delay field of the ACK frame (RFC 9000 §19.3)
					 *
					 * \~
					 */
					uint64_t ackTime;
					// Диапазоны принятых номеров пакетов в порядке убывания
					vector <frame::range_t> ranges;
					/**
					 * \~russian
					 * @brief Счётчики принятых пакетов по маркировке ECN заголовка IP-пакета.
					 *        Эхом возвращаются пиру во фрейме ACK_ECN, по их приросту он судит
					 *        о перегрузке пути, не дожидаясь потерь (RFC 9000 §13.4)
					 *
					 * \~english
					 * @brief Counters of the accepted packets by the ECN marking of the header of an IP packet.
					 *        Returned by an echo to the peer in the ACK_ECN frame, by their growth he judges
					 *        about the overload of the path without waiting for the losses (RFC 9000 §13.4)
					 *
					 * \~
					 */
					uint64_t ect0;
					// Счётчик принятых пакетов с маркировкой ECT(1)
					uint64_t ect1;
					// Счётчик принятых пакетов с маркировкой CE
					uint64_t ce;
					// Наибольший принятый от пира счётчик пакетов с маркировкой CE
					uint64_t peerCe;
					// Наибольший принятый от пира счётчик пакетов с маркировкой ECT(0)
					uint64_t peerEct0;
					// Наибольший принятый от пира счётчик пакетов с маркировкой ECT(1)
					uint64_t peerEct1;
					// Флаг приёма счётчиков ECN от пира
					bool hasPeerEcn;
					/**
					 * \~russian
					 * @brief Количество пакетов пространства, отправленных с маркировкой поддержки
					 *        ECN. Счётчики маркировок удалённого узла не вправе его превышать -
					 *        иначе счётчики недостоверны (RFC 9000 §13.4.2.1)
					 *
					 * \~english
					 * @brief Number of the packets of the space sent with a marking of the support
					 *        of the ECN. The counters of the markings of the remote node have no right to exceed it -
					 *        otherwise the counters are unreliable (RFC 9000 §13.4.2.1)
					 *
					 * \~
					 */
					uint64_t ecnSent;
					// Смещение начала буфера исходящих CRYPTO-данных в потоке уровня
					uint64_t txOffset;
					// Буфер исходящих CRYPTO-данных уровня (ожидают упаковки в пакеты)
					string txBuffer;
					// Смещение непрерывно собранных входящих CRYPTO-данных уровня
					uint64_t rxOffset;
					// Буфер сборки входящих CRYPTO-данных по смещениям
					map <uint64_t, string> rxBuffer;
					// Суммарный размер буферизированных CRYPTO-данных (учитывается инкрементально)
					size_t rxBuffered;
					// Список отправленных и ещё не подтверждённых ack-eliciting пакетов
					vector <sent_t> sent;
					// Очередь ретрансмиссии CRYPTO-данных со смещениями (RFC 9002 §6.3)
					deque <std::pair <uint64_t, string>> rtxQueue;
					// Время детекта потерь пакетов пространства (RFC 9002 §6.1.2)
					uint64_t lossTime;
					// Флаг взведённого таймера детекта потерь
					bool hasLossTime;
					// Время отправки последнего ack-eliciting пакета (RFC 9002 §6.2.1)
					uint64_t lastElicited;
					// Флаг наличия отправленных ack-eliciting пакетов
					bool hasElicited;
					// Флаг необходимости отправки зондирующего фрейма PING (RFC 9002 §6.2.4)
					bool pingQueued;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Space() noexcept;
				} space_data_t;
				/**
				 * \~russian
				 * @brief Структура состояния пути соединения (RFC 9000 §9)
				 *
				 * \~english
				 * @brief Structure of the state of the path of the connection (RFC 9000 §9)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Path {
					// Флаг ожидания ответа на отправленную проверку пути (RFC 9000 §8.2)
					bool pending;
					// Флаг необходимости отправки фрейма PATH_CHALLENGE
					bool queued;
					// Флаг подтверждённой достижимости пути
					bool validated;
					/**
					 * \~russian
					 * @brief Очередь данных принятых проверок пути: на каждый принятый фрейм
					 *        PATH_CHALLENGE отправляется свой фрейм PATH_RESPONSE с его данными,
					 *        поэтому одного слота недостаточно - вторая проверка, принятая до
					 *        сборки ответа, затёрла бы первую (RFC 9000 §8.2.2)
					 *
					 * \~english
					 * @brief Queue of the data of the accepted path validations: for every accepted
					 *        PATH_CHALLENGE frame its own PATH_RESPONSE frame is sent with its data,
					 *        therefore one slot is not enough - a second validation, accepted before
					 *        the assembling of the answer, would overwrite the first (RFC 9000 §8.2.2)
					 *
					 * \~
					 */
					deque <string> responses;
					// Отправленные данные проверки достижимости пути
					uint8_t probe[proto::PATH_DATA_SIZE];
					/**
					 * \~russian
					 * @brief Флаг выполняемой проверки предпочтительного адреса сервера. Переезд
					 *        на него допустим лишь после подтверждения его достижимости, поэтому
					 *        проверка выполняется отдельной пробирующей датаграммой на сам
					 *        предпочтительный адрес, пока соединение работает по текущему пути
					 *        (RFC 9000 §9.6.2)
					 *
					 * \~english
					 * @brief Flag of the performing validation of the preferred address of the server. A move
					 *        onto it is allowed only after the confirmation of its reachability, therefore
					 *        the validation is performed by a separate probing datagram to the preferred
					 *        address itself, while the connection works by the current path
					 *        (RFC 9000 §9.6.2)
					 *
					 * \~
					 */
					bool relocating;
					/**
					 * \~russian
					 * @brief Флаг адресации собранной датаграммы предпочтительному адресу. Пока
					 *        проверка не пройдена, соединение отправляет по двум адресам сразу,
					 *        поэтому вызывающему коду сообщается адресат каждой датаграммы
					 *
					 * \~english
					 * @brief Flag of the addressing of the assembled datagram to the preferred address. While
					 *        the validation is not passed, the connection sends to two addresses at once,
					 *        therefore the calling code is informed of the addressee of each datagram
					 *
					 * \~
					 */
					bool alternate;
					// Идентификатор соединения предпочтительного адреса сервера (RFC 9000 §5.1.1)
					cid_t relocation;
					/**
					 * \~russian
					 * @brief Дедлайн отказа от проверки достижимости пути в миллисекундах. Ответа
					 *        на проверку можно не дождаться вовсе, и переотправлять её бесконечно
					 *        незачем: по истечении дедлайна путь признаётся непригодным
					 *        (RFC 9000 §8.2.4)
					 *
					 * \~english
					 * @brief Deadline of the refusal from the path reachability validation in milliseconds. An answer
					 *        to the validation can be not waited for at all, and to retransmit it endlessly
					 *        there is no reason: on the expiration of the deadline the path is recognised as unfit
					 *        (RFC 9000 §8.2.4)
					 *
					 * \~
					 */
					uint64_t deadline;
					/**
					 * \~russian
					 * @brief Адрес удалённого эндпоинта подтверждённого пути. Смена адреса при
					 *        установленном соединении означает миграцию: путь считается новым
					 *        и требует проверки достижимости (RFC 9000 §9)
					 *
					 * \~english
					 * @brief Address of the remote endpoint of the confirmed path. A change of the address at an
					 *        established connection means a migration: the path is counted new
					 *        and requires a reachability validation (RFC 9000 §9)
					 *
					 * \~
					 */
					string address;
					/**
					 * \~russian
					 * @brief Последний адрес удалённого эндпоинта, достижимость которого подтверждена.
					 *        Смену адреса способен подделать находящийся на пути посторонний, и
					 *        непройденная проверка нового адреса обязана возвращать соединение на
					 *        этот - иначе одна поддельная датаграмма разрывала бы соединение
					 *        (RFC 9000 §9.3.2)
					 *
					 * \~english
					 * @brief Last address of the remote endpoint, the reachability of which is confirmed.
					 *        A change of the address is able to be forged by a stranger standing on the path, and
					 *        a not passed validation of a new address is obliged to return the connection to
					 *        this one - otherwise one forged datagram would break the connection
					 *        (RFC 9000 §9.3.2)
					 *
					 * \~
					 */
					string previous;
					// Количество выполненных смен пути соединения
					uint64_t migrations;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Path() noexcept;
				} path_t;
				/**
				 * \~russian
				 * @brief Структура состояния завершения соединения (RFC 9000 §10.2)
				 *
				 * \~english
				 * @brief Structure of the state of the completion of the connection (RFC 9000 §10.2)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Close {
					// Код ошибки завершения соединения
					uint64_t code;
					// Флаг ошибки приложения при завершении соединения
					bool app;
					// Флаг постановки завершения соединения в очередь отправки
					bool queued;
					// Флаг выполненной отправки фрейма CONNECTION_CLOSE
					bool sent;
					// Количество принятых пакетов после отправки фрейма CONNECTION_CLOSE
					uint64_t received;
					// Порог принятых пакетов для повторной отправки CONNECTION_CLOSE (RFC 9000 §10.2.1)
					uint64_t threshold;
					/**
					 * \~russian
					 * @brief Дедлайн периода завершения соединения в миллисекундах: по его истечении
					 *        состояние соединения подлежит освобождению, а до него локальный эндпоинт
					 *        повторяет фрейм CONNECTION_CLOSE в ответ на приходящие пакеты
					 *        (RFC 9000 §10.2). Нулевое значение - дедлайн ещё не взведён
					 *
					 * \~english
					 * @brief Deadline of the period of the connection completion in milliseconds: on its expiration
					 *        the state of the connection is subject to release, and before it the local endpoint
					 *        repeats the CONNECTION_CLOSE frame in answer to the arriving packets
					 *        (RFC 9000 §10.2). A zero value - the deadline is not yet armed
					 *
					 * \~
					 */
					uint64_t deadline;
					// Причина завершения соединения
					string reason;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Close() noexcept;
				} close_t;
				/**
				 * \~russian
				 * @brief Структура состояния контроля перегрузки (RFC 9002 §7)
				 *
				 * \~english
				 * @brief Structure of the state of the congestion control (RFC 9002 §7)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Congestion {
					// Окно перегрузки в октетах
					uint64_t window;
					// Порог замедленного старта (RFC 9002 §7.3.1)
					uint64_t threshold;
					// Количество неподтверждённых октетов в полёте (RFC 9002 §B.2)
					uint64_t inflight;
					// Время начала периода восстановления (RFC 9002 §7.3.2)
					uint64_t recovery;
					// Флаг активного периода восстановления
					bool inRecovery;
					// Количество зондирующих пакетов, разрешённых сверх окна перегрузки (RFC 9002 §7.5)
					uint8_t probes;
					/**
					 * \~russian
					 * @brief Время отправки наиболее позднего подтверждённого пакета по всем
					 *        пространствам номеров. Пакеты, отправленные после него и признанные
					 *        потерянными, образуют период устойчивой перегрузки (RFC 9002 §7.6)
					 *
					 * \~english
					 * @brief Time of sending of the most lately acknowledged packet over all
					 *        spaces of the numbers. The packets sent after it and recognised as lost
					 *        form the period of the persistent congestion (RFC 9002 §7.6)
					 *
					 * \~
					 */
					uint64_t acked;
					// Флаг наличия хотя бы одного подтверждённого пакета
					bool hasAcked;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Congestion() noexcept;
				} congestion_t;
				/**
				 * \~russian
				 * @brief Структура оценки задержки приёма-передачи (RFC 9002 §5)
				 *
				 * \~english
				 * @brief Structure of the estimation of the delay of the reception-transmission (RFC 9002 §5)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Rtt {
					// Последняя измеренная задержка приёма-передачи
					uint64_t latest;
					// Минимальная измеренная задержка приёма-передачи
					uint64_t minimum;
					// Сглаженная задержка приёма-передачи
					uint64_t smoothed;
					// Вариативность задержки приёма-передачи
					uint64_t variation;
					// Флаг наличия первого измерения задержки приёма-передачи
					bool sampled;
					// Счётчик срабатываний таймера PTO без подтверждений (RFC 9002 §6.2.1)
					uint8_t ptoCount;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Rtt() noexcept;
				} rtt_t;
				/**
				 * \~russian
				 * @brief Структура маркировки перегрузки пути (RFC 9000 §13.4)
				 *
				 * \~english
				 * @brief Structure of the marking of the congestion of the path (RFC 9000 §13.4)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Marking {
					/**
					 * \~russian
					 * @brief Маркировка ECN обрабатываемой датаграммы. Действует на время разбора
					 *        одной датаграммы: сетевого уровня соединение не имеет, и маркировку
					 *        ему сообщает вызывающий код
					 *
					 * \~english
					 * @brief ECN marking of the datagram being processed. Acts for the time of the parsing of
					 *        one datagram: the connection has no network level, and the marking
					 *        is reported to it by the calling code
					 *
					 * \~
					 */
					event::ecn_t received;
					/**
					 * \~russian
					 * @brief Флаг маркировки исходящих датаграмм поддержкой ECN. Устанавливается
					 *        вызывающим кодом: маркировка накладывается на заголовок IP-пакета,
					 *        до которого соединение не достаёт (RFC 9000 §13.4.1)
					 *
					 * \~english
					 * @brief Flag of the marking of the outgoing datagrams with the ECN support. Set by
					 *        the calling code: the marking is laid onto the header of an IP packet,
					 *        which the connection does not reach (RFC 9000 §13.4.1)
					 *
					 * \~
					 */
					bool enabled;
					/**
					 * \~russian
					 * @brief Флаг непройденной проверки пути на поддержку ECN. Путь может стирать
					 *        маркировку либо удалённый узел может её не возвращать: маркировать
					 *        далее бессмысленно, а счётчикам перегрузки доверять нельзя
					 *        (RFC 9000 §13.4.2)
					 *
					 * \~english
					 * @brief Flag of the not passed path validation for the ECN support. A path can erase
					 *        the marking or the remote node can not return it: to mark
					 *        further is senseless, and the counters of the overload can not be trusted
					 *        (RFC 9000 §13.4.2)
					 *
					 * \~
					 */
					bool failed;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Marking() noexcept;
				} marking_t;
				/**
				 * \~russian
				 * @brief Структура лимитов потоков приложения (RFC 9000 §4)
				 *
				 * \~english
				 * @brief Structure of the limits of the streams of the application (RFC 9000 §4)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Limits {
					// Количество открытых локально двунаправленных потоков
					uint64_t openedBidi;
					// Количество открытых локально однонаправленных потоков
					uint64_t openedUni;
					// Лимит удалённого эндпоинта на локально открываемые двунаправленные потоки
					uint64_t maxBidiRemote;
					// Лимит удалённого эндпоинта на локально открываемые однонаправленные потоки
					uint64_t maxUniRemote;
					// Количество потоков, открытых удалённым эндпоинтом (двунаправленных)
					uint64_t acceptedBidi;
					// Количество потоков, открытых удалённым эндпоинтом (однонаправленных)
					uint64_t acceptedUni;
					// Анонсированный лимит на двунаправленные потоки удалённого эндпоинта
					uint64_t maxBidiLocal;
					// Анонсированный лимит на однонаправленные потоки удалённого эндпоинта
					uint64_t maxUniLocal;
					// Флаг необходимости отправки обновлённого лимита MAX_STREAMS (двунаправленные)
					bool bidiQueued;
					// Флаг необходимости отправки обновлённого лимита MAX_STREAMS (однонаправленные)
					bool uniQueued;
					// Флаг необходимости отправки STREAMS_BLOCKED при упоре в лимит (двунаправленные)
					bool bidiBlocked;
					// Флаг необходимости отправки STREAMS_BLOCKED при упоре в лимит (однонаправленные)
					bool uniBlocked;
					// Лимит, при котором STREAMS_BLOCKED уже отправлен (двунаправленные)
					uint64_t bidiBlockedAt;
					// Лимит, при котором STREAMS_BLOCKED уже отправлен (однонаправленные)
					uint64_t uniBlockedAt;
					// Санитарная верхняя граница анонсируемого начального лимита потоков одного направления
					uint64_t cap;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Limits() noexcept;
				} limits_t;
				/**
				 * \~russian
				 * @brief Структура контроля потока данных соединения (RFC 9000 §4.1)
				 *
				 * \~english
				 * @brief Structure of the flow control of the data of the connection (RFC 9000 §4.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Flow {
					// Количество отправленных данных потоков приложения
					uint64_t txData;
					// Лимит отправки данных соединения от удалённого эндпоинта (MAX_DATA)
					uint64_t txMax;
					// Флаг необходимости отправки фрейма DATA_BLOCKED
					bool txBlocked;
					// Лимит, о блокировке которым уже уведомлён удалённый эндпоинт
					uint64_t txBlockedAt;
					// Сумма наибольших принятых смещений потоков
					uint64_t rxData;
					// Количество выданных приложению данных потоков
					uint64_t rxConsumed;
					// Анонсированный лимит приёма данных соединения (MAX_DATA)
					uint64_t rxMax;
					// Флаг необходимости отправки обновлённого лимита MAX_DATA
					bool rxQueued;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Flow() noexcept;
				} flow_t;
				/**
				 * \~russian
				 * @brief Структура переиспользуемых буферов сборки и разбора пакетов
				 *
				 * @details Буферы удерживают ёмкость между вызовами: выделение памяти
				 *          на каждую датаграмму обходилось бы дороже самой обработки
				 *
				 * \~english
				 * @brief Structure of the reused buffers of the assembly and of the parsing of the packets
				 *
				 * @details The buffers hold the capacity between the calls: an allocation of the memory
				 *          per every datagram would come dearer than the processing itself
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Buffers {
					// Буфер копии принимаемой датаграммы для снятия защиты заголовков
					string datagram;
					// Буфер расшифрованной нагрузки принятого пакета
					string plain;
					// Буфер собираемого заголовка исходящего пакета
					string header;
					/**
					 * \~russian
					 * @brief Буферы собираемой нагрузки по уровням шифрования. В одной датаграмме
					 *        пакет каждого уровня встречается не более одного раза, поэтому
					 *        индексация уровнем однозначна
					 *
					 * \~english
					 * @brief Buffers of the assembling payload by the levels of encryption. In one datagram
					 *        a packet of every level occurs not more than once, therefore the
					 *        indexing by a level is unambiguous
					 *
					 * \~
					 */
					string payload[handshake_t::LEVELS];
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Buffers() noexcept;
				} buffers_t;
				/**
				 * \~russian
				 * @brief Структура очередей датаграмм приложения (RFC 9221)
				 *
				 * \~english
				 * @brief Structure of the queues of the datagrams of the application (RFC 9221)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Dgram {
					/**
					 * \~russian
					 * @brief Очередь исходящих датаграмм приложения. Доставка датаграмм ненадёжна:
					 *        потерянные не ретранслируются, поэтому очередь только на отправку
					 *        и учёта отправленного не ведёт (RFC 9221 §5.2)
					 *
					 * \~english
					 * @brief Queue of the outgoing datagrams of the application. The delivery of the datagrams is unreliable:
					 *        the lost ones are not retransmitted, therefore the queue is only for sending
					 *        and the account of the sent is not kept (RFC 9221 §5.2)
					 *
					 * \~
					 */
					deque <string> tx;
					// Очередь принятых датаграмм приложения (ожидают выдачи приложению)
					deque <string> rx;
					// Готовая датаграмма без состояния (Version Negotiation либо Retry)
					string stateless;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Dgram() noexcept;
				} dgram_t;
				/**
				 * \~russian
				 * @brief Структура идентификаторов соединения локального и удалённого эндпоинтов (RFC 9000 §5.1)
				 *
				 * \~english
				 * @brief Structure of the identifiers of the connection of the local and of the remote endpoints (RFC 9000 §5.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Identity {
					// Идентификатор соединения локального эндпоинта
					cid_t source;
					// Идентификатор соединения удалённого эндпоинта
					cid_t destination;
					// Исходный DCID первого пакета Initial клиента (RFC 9000 §7.3)
					cid_t original;
					// SCID пакета Retry (RFC 9000 §17.2.5)
					cid_t retry;
					// Флаг обновления DCID по первому ответу сервера (RFC 9000 §7.2)
					bool updated;
					// Флаг прохождения соединения через пакет Retry
					bool retried;
					// Флаг выполненного переезда на предпочтительный адрес сервера (RFC 9000 §9.6)
					bool relocated;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Identity() noexcept;
				} identity_t;
				/**
				 * \~russian
				 * @brief Структура контроля анти-амплификации (RFC 9000 §8.1)
				 *
				 * \~english
				 * @brief Structure of the control of the anti-amplification (RFC 9000 §8.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Amplify {
					// Количество октетов, принятых от удалённого эндпоинта
					uint64_t received;
					// Количество октетов, отправленных удалённому эндпоинту
					uint64_t sent;
					/**
					 * \~russian
					 * @brief Флаг подтверждённого адреса удалённого эндпоинта. До подтверждения
					 *        сервер не вправе отправить более трёхкратного объёма принятого
					 *
					 * \~english
					 * @brief Flag of the confirmed address of the remote endpoint. Before the confirmation
					 *        the server has no right to send more than the threefold volume of the accepted
					 *
					 * \~
					 */
					bool validated;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param endpoint роль локального эндпоинта на соединении
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param endpoint role of the local endpoint on the connection
					 *
					 * \~
					 */
					explicit Amplify(const endpoint_t endpoint) noexcept;
				} amplify_t;
				/**
				 * \~russian
				 * @brief Структура состояния и лимитов защиты AEAD (RFC 9001 §6.6)
				 *
				 * \~english
				 * @brief Structure of the state and of the limits of the protection AEAD (RFC 9001 §6.6)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ AEAD {
					// Количество неудачных снятий защиты пакетов (счётчик лимита целостности)
					uint64_t failures;
					// Предельное число пакетов на одном ключе (лимит конфиденциальности)
					uint64_t confidentiality;
					// Предельное число неудачных снятий защиты (лимит целостности)
					uint64_t integrity;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit AEAD() noexcept;
				} aead_t;
				/**
				 * \~russian
				 * @brief Структура основных скаляров состояния соединения
				 *
				 * \~english
				 * @brief Structure of the main scalars of the state of the connection
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Core {
					// Роль локального эндпоинта на соединении
					endpoint_t endpoint;
					// Состояние соединения
					state_t state;
					// Код ошибки транспорта соединения (локальной или полученной от пира)
					error_t error;
					// Битовые флаги состояния соединения (пространство имён flags в connection.cpp)
					uint8_t flags;
					// Битовые флаги настраиваемых опций соединения (пространство имён options в connection.cpp)
					uint8_t options;
					/**
					 * \~russian
					 * @brief Опаковое представление адреса удалённого эндпоинта. Слой соединения
					 *        работает без ввода-вывода и адреса не знает, поэтому его сообщает
					 *        вызывающий код: к нему привязывается токен проверки адреса (RFC 9000 §8.1.4)
					 *
					 * \~english
					 * @brief Packable representation of the address of the remote endpoint. The connection layer
					 *        works without input/output and does not know the address, therefore it is reported
					 *        by the calling code: the address validation token is tied to it (RFC 9000 §8.1.4)
					 *
					 * \~
					 */
					string address;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param endpoint роль локального эндпоинта на соединении
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param endpoint role of the local endpoint on the connection
					 *
					 * \~
					 */
					explicit Core(const endpoint_t endpoint) noexcept;
				} core_t;
				/**
				 * \~russian
				 * @brief Структура временных меток соединения в миллисекундах
				 *
				 * \~english
				 * @brief Structure of the time stamps of the connection in milliseconds
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Times {
					// Текущее время последнего вызова
					uint64_t now;
					// Время последнего принятого и обработанного пакета (RFC 9000 §10.1)
					uint64_t idle;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Times() noexcept;
				} times_t;
				/**
				 * \~russian
				 * @brief Структура транспортных параметров соединения (RFC 9000 §18)
				 *
				 * \~english
				 * @brief Structure of the transport parameters of the connection (RFC 9000 §18)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transport {
					// Локальные транспортные параметры (дополняются идентификаторами при старте)
					quic::params::params_t local;
					// Транспортные параметры удалённого эндпоинта (после завершения хендшейка)
					quic::params::params_t remote;
					// Запомненные параметры удалённого эндпоинта прошлого соединения (для проверки 0-RTT)
					quic::params::params_t remembered;
					// Флаг установки возобновляемой сессии с запомненными параметрами (RFC 9001 §4.6.1)
					bool resumed;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Transport() noexcept;
				} transport_t;
				/**
				 * \~russian
				 * @brief Структура состояния потоков приложения (RFC 9000 §2)
				 *
				 * \~english
				 * @brief Structure of the state of the streams of the application (RFC 9000 §2)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Streams {
					// Список потоков приложения по идентификаторам
					map <uint64_t, stream_data_t> list;
					// Очередь ретрансмиссии блоков данных потоков приложения
					deque <chunk_t> retransmit;
					/**
					 * \~russian
					 * @brief Идентификаторы потоков, готовых к выдаче данных приложению. Список
					 *        пополняется по месту изменения состояния приёма, а устаревшие записи
					 *        отсеиваются при чтении: обход всего списка потоков на каждый запрос
					 *        не окупается, а хранение набором стоило бы выделения памяти
					 *        на каждое изменение готовности.
					 *
					 * \~english
					 * @brief Identifiers of the streams ready for issuing the data to the application. The list
					 *        is refilled by the place of a change of the receiving state, and the outdated records
					 *        are screened off on reading: a walk over the whole list of the streams for every
					 *        request does not pay off, and a storage by a set would cost a memory allocation
					 *        for every change of the readiness
					 *
					 * \~
					 */
					vector <uint64_t> readable;
					/**
					 * \~russian
					 * @brief Идентификаторы потоков с ожидающими управляющими фреймами
					 *        (MAX_STREAM_DATA, RESET_STREAM, STOP_SENDING, STREAM_DATA_BLOCKED).
					 *        Список пополняется по месту постановки фрейма в очередь, а устаревшие
					 *        записи отсеиваются при обходе: обход всего списка потоков на каждый
					 *        пакет не окупается на множестве потоков - у отправителя данных
					 *        управляющих фреймов потоков нет вовсе, и список остаётся пустым
					 *
					 * \~english
					 * @brief Identifiers of the streams with the waiting control frames
					 *        (MAX_STREAM_DATA, RESET_STREAM, STOP_SENDING, STREAM_DATA_BLOCKED).
					 *        The list is refilled by the place of putting a frame into the queue, and the outdated
					 *        records are screened off on a walk: a walk over the whole list of the streams for every
					 *        packet does not pay off on a multitude of the streams - at a sender of the data
					 *        the streams have no control frames at all, and the list stays empty
					 *
					 * \~
					 */
					vector <uint64_t> control;
					/**
					 * \~russian
					 * @brief Идентификаторы потоков с данными к отправке, хранятся отсортированными.
					 *        Порядок значим - обход идёт круговым `lower_bound` по курсору, поэтому
					 *        набор упорядочен; вектор, а не set, чтобы вставка и удаление шли по
					 *        переиспользуемой ёмкости, не выделяя узел на каждую постановку потока
					 *        (методы `pending`/`dequeue`). Пополняется при постановке данных (send) и
					 *        разблокировке лимитом (MAX_STREAM_DATA), а устаревшие, слитые и
					 *        заблокированные записи отсеиваются по месту при обходе. Обход всего
					 *        списка потоков на каждый пакет не окупается: у соединения без передач пуст
					 *
					 * \~english
					 * @brief Identifiers of the streams with the data for sending, stored sorted.
					 *        The order matters - a walk goes by a circular `lower_bound` over the cursor, therefore the
					 *        set is ordered; a vector, and not a set, so that an insertion and a removal went by
					 *        a reusable capacity, not allocating a node for every putting of a stream
					 *        (the `pending`/`dequeue` methods). Refilled when putting the data (send) and
					 *        unblocking by a limit (MAX_STREAM_DATA), and the outdated, merged and
					 *        blocked records are screened off by the place on a walk. A walk over the whole
					 *        list of the streams for every packet does not pay off: at a connection without transfers it is empty
					 *
					 * \~
					 */
					vector <uint64_t> writable;
					/**
					 * \~russian
					 * @brief Идентификаторы потоков, буфер отправки которых опустился ниже нижней
					 *        водяной метки после заполнения до верхней (сигнал writable). Список
					 *        пополняется по месту дренирования буфера и вычитывается методом
					 *        `drained()`: приложение, получившее частичный приём в send(), по этому
					 *        сигналу возобновляет выдачу данных потока. Пуст, пока backpressure
					 *        не срабатывал
					 *
					 * \~english
					 * @brief Identifiers of the streams, the send buffer of which fell below the lower
					 *        high-water mark after a filling up to the upper (the writable signal). The list
					 *        is refilled by the place of the draining of a buffer and read out by the
					 *        `drained()` method: an application, having received a partial acceptance in send(), by this
					 *        signal resumes the issuing of the stream data. Empty while the backpressure
					 *        has not tripped
					 *
					 * \~
					 */
					vector <uint64_t> drained;
					/**
					 * \~russian
					 * @brief Идентификатор потока, с которого начинается обход при упаковке данных.
					 *        Обеспечивает круговое обслуживание потоков: без него потоки с меньшими
					 *        идентификаторами занимали бы датаграмму целиком, а остальные простаивали
					 *
					 * \~english
					 * @brief Identifier of the stream from which a walk begins on packing the data.
					 *        It provides a circular servicing of the streams: without it the streams with smaller
					 *        identifiers would occupy a datagram entirely, and the rest would stand idle
					 *
					 * \~
					 */
					uint64_t cursor;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Streams() noexcept;
				} streams_t;
				/**
				 * \~russian
				 * @brief Структура токенов проверки адреса (RFC 9000 §8.1)
				 *
				 * \~english
				 * @brief Structure of the tokens of the checking of the address (RFC 9000 §8.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Token {
					// Флаг проверки адреса клиента сервером через пакет Retry
					bool retry;
					// Флаг необходимости отправки фрейма NEW_TOKEN (только сервер)
					bool queued;
					// Токен пакетов Initial (полученный клиентом из пакета Retry)
					string initial;
					// Токен проверки адреса, выданный сервером в пакете Retry
					string retried;
					/**
					 * \~russian
					 * @brief Токен проверки адреса фрейма NEW_TOKEN: у сервера - выданный на этом
					 *        соединении, у клиента - принятый от сервера и пригодный к предъявлению
					 *        в первом пакете следующего соединения (RFC 9000 §8.1.3)
					 *
					 * \~english
					 * @brief Token of the address validation of a NEW_TOKEN frame: at a server - the issued one on this
					 *        connection, at a client - the accepted one from the server and fit for presenting
					 *        in the first packet of the next connection (RFC 9000 §8.1.3)
					 *
					 * \~
					 */
					string address;
					/**
					 * \~russian
					 * @brief Общий ключ вывода токенов сброса без сохранения состояния. Пока ключ
					 *        не задан, токены генерируются случайно: сброс без сохранения состояния
					 *        такому соединению недоступен, поскольку воспроизвести токен по одному
					 *        идентификатору невозможно (RFC 9000 §10.3.2)
					 *
					 * \~english
					 * @brief Shared key of the derivation of the stateless reset tokens. While the key
					 *        is not set, the tokens are generated randomly: a stateless reset
					 *        is unavailable for such a connection, since to reproduce a token by one
					 *        identifier is impossible (RFC 9000 §10.3.2)
					 *
					 * \~
					 */
					string reset;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Token() noexcept;
				} token_t;
				/**
				 * \~russian
				 * @brief Структура состояния фазы ключей уровня приложения (RFC 9001 §6)
				 *
				 * \~english
				 * @brief Structure of the state of the phase of the keys of the level of the application (RFC 9001 §6)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Phase {
					// Текущая фаза ключей уровня приложения
					bool current;
					// Флаг готовности ключей следующей фазы
					bool ready;
					// Флаг наличия ключей предыдущей фазы для приёма запоздавших пакетов
					bool hasPrevious;
					// Ключи снятия защиты следующей фазы
					crypto::keys_t nextRead;
					// Ключи защиты следующей фазы
					crypto::keys_t nextWrite;
					// Ключи снятия защиты предыдущей фазы
					crypto::keys_t prevRead;
					// Номер первого пакета, отправленного в текущей фазе ключей
					uint64_t sent;
					// Количество пакетов, защищённых ключами текущей фазы (RFC 9001 §6.6)
					uint64_t packets;
					// Дедлайн сброса ключей предыдущей фазы (0 - сброс не запланирован, RFC 9001 §6.3)
					uint64_t discard;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Phase() noexcept;
				} phase_t;
				/**
				 * \~russian
				 * @brief Структура состояния поиска размера пути (RFC 8899)
				 *
				 * \~english
				 * @brief Structure of the state of the search of the size of the path (RFC 8899)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Pmtu {
					/**
					 * \~russian
					 * @brief Подтверждённый размер исходящей датаграммы. Начинается с размера,
					 *        который обязан пропускать любой путь, и наращивается зондированием
					 *        по мере подтверждения больших размеров
					 *
					 * \~english
					 * @brief Confirmed size of an outgoing datagram. Begins with a size
					 *        which any path is obliged to let through, and grows by probing
					 *        as the larger sizes are acknowledged
					 *
					 * \~
					 */
					size_t size;
					// Верхняя граница поиска размера пути
					size_t high;
					/**
					 * \~russian
					 * @brief Верхняя граница поиска, заданная вызывающим кодом. Хранится отдельно
					 *        от рабочей границы: смена пути начинает поиск заново, и заданное
					 *        приложением ограничение обязано пережить её
					 *
					 * \~english
					 * @brief Upper bound of the search, set by the calling code. Stored separately
					 *        from the working bound: a change of the path begins the search anew, and the set
					 *        by the application limit is obliged to outlive it
					 *
					 * \~
					 */
					size_t limit;
					// Размер отправляемого зонда размера пути (0 - зонд не собирается)
					size_t probe;
					// Количество отправленных попыток текущего зонда размера пути
					uint8_t count;
					// Количество подряд потерянных датаграмм сверх базового размера (детекция чёрной дыры пути, RFC 8899 §5.4)
					uint8_t blackhole;
					// Флаг необходимости отправки зонда размера пути
					bool queued;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Pmtu() noexcept;
				} pmtu_t;
				/**
				 * \~russian
				 * @brief Структура идентификатора соединения удалённого эндпоинта (RFC 9000 §5.1.1)
				 *
				 * \~english
				 * @brief Structure of an identifier of the connection of the remote endpoint (RFC 9000 §5.1.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ RemoteCid {
					// Порядковый номер идентификатора соединения
					uint64_t seq;
					// Флаг использования идентификатора в качестве DCID
					bool used;
					/**
					 * \~russian
					 * @brief Флаг наличия токена сброса без сохранения состояния. Идентификатор
					 *        хендшейка токена не имеет: он приходит только фреймом
					 *        NEW_CONNECTION_ID либо транспортным параметром (RFC 9000 §10.3.1)
					 *
					 * \~english
					 * @brief Flag of the presence of a stateless reset token. A handshake identifier
					 *        has no token: it comes only by a NEW_CONNECTION_ID frame
					 *        or a transport parameter (RFC 9000 §10.3.1)
					 *
					 * \~
					 */
					bool hasToken;
					// Идентификатор соединения
					cid_t cid;
					// Токен сброса без сохранения состояния (RFC 9000 §10.3)
					uint8_t resetToken[proto::RESET_TOKEN_SIZE];
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit RemoteCid() noexcept;
				} remote_cid_t;
				/**
				 * \~russian
				 * @brief Структура набора идентификаторов соединения (RFC 9000 §5.1.1)
				 *
				 * \~english
				 * @brief Structure of the collection of the identifiers of the connection (RFC 9000 §5.1.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Routing {
					// Порядковый номер текущего идентификатора соединения удалённого эндпоинта
					uint64_t sequence;
					// Порядковый номер, до которого идентификаторы удалённого эндпоинта выведены из обращения
					uint64_t retirePrior;
					// Порядковый номер следующего выдаваемого идентификатора соединения
					uint64_t issuedSeq;
					// Флаг вывода идентификатора хендшейка локального эндпоинта из обращения
					bool retired;
					// Список идентификаторов соединения удалённого эндпоинта
					vector <remote_cid_t> remote;
					// Очередь порядковых номеров для отправки фреймов RETIRE_CONNECTION_ID
					deque <uint64_t> retireQueue;
					// Выданные локальные идентификаторы соединения по порядковым номерам
					map <uint64_t, frame::new_connection_id_t> issued;
					// Очередь порядковых номеров для отправки фреймов NEW_CONNECTION_ID
					vector <uint64_t> issueQueue;
					// Идентификаторы, введённые в обращение и ещё не выданные вызывающему коду
					vector <cid_t> added;
					// Идентификаторы, выведенные из обращения и ещё не выданные вызывающему коду
					vector <cid_t> removed;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Routing() noexcept;
				} routing_t;
				/**
				 * \~russian
				 * @brief Структура идентификаторов соединения (RFC 9000 §5.1)
				 *
				 * \~english
				 * @brief Structure of the identifiers of the connection (RFC 9000 §5.1)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Cids {
					// Активная пара идентификаторов соединения эндпоинтов (§5.1)
					identity_t identity;
					// Набор выданных и выводимых из обращения идентификаторов (§5.1.1)
					routing_t routing;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Cids() noexcept;
				} cids_t;
				/**
				 * \~russian
				 * @brief Структура криптографического состояния соединения (RFC 9001)
				 *
				 * \~english
				 * @brief Structure of the cryptographic state of the connection (RFC 9001)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Crypto {
					// Машина криптографического хендшейка
					handshake_t handshake;
					// Бит фазы ключей уровня приложения (§6)
					phase_t phase;
					// Состояние и лимиты защиты AEAD (§6.6)
					aead_t aead;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param endpoint роль локального эндпоинта на соединении
					 * @param ctx      идентификатор шаблона контекста безопасности
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param endpoint role of the local endpoint on the connection
					 * @param ctx      identifier of the template of the context of the safety
					 *
					 * \~
					 */
					explicit Crypto(const endpoint_t endpoint, const tls::coder::id_t ctx) noexcept;
				} crypto_t;
			private:
				// Основные скаляры состояния соединения
				core_t _core;
			private:
				// Идентификаторы соединения эндпоинтов (RFC 9000 §5.1)
				cids_t _cids;
			private:
				// Состояние поиска размера пути (RFC 8899)
				pmtu_t _pmtu;
				// Токены проверки адреса соединения (RFC 9000 §8.1)
				token_t _token;
			private:
				// Состояние контроля перегрузки (RFC 9002 §7)
				congestion_t _congestion;
				/**
				 * \~russian
				 * @brief Переиспользуемый буфер времён отправки пакетов, подтверждённых текущим
				 *        фреймом ACK: передаётся в детект потерь для разрыва периода устойчивой
				 *        перегрузки. Хранится членом, а не локальной переменной, чтобы обработка
				 *        каждого фрейма ACK в горячем пути не выделяла память заново (RFC 9002 §7.6)
				 *
				 * \~english
				 * @brief Reusable buffer of the sending times of the packets acknowledged by the current
				 *        ACK frame: passed into the loss detection for the breaking of the period of the persistent
				 *        congestion. Stored as a member, and not a local variable, so that the processing
				 *        of every ACK frame in a hot path did not allocate memory anew (RFC 9002 §7.6)
				 *
				 * \~
				 */
				vector <uint64_t> _ackedTimes;
			private:
				// Состояние пути соединения (RFC 9000 §9)
				path_t _path;
			private:
				// Состояние завершения соединения (RFC 9000 §10.2)
				close_t _close;
			private:
				// Контроль анти-амплификации (RFC 9000 §8.1)
				amplify_t _amplify;
			private:
				// Временные метки соединения в миллисекундах
				times_t _times;
			private:
				// Маркировка перегрузки пути (RFC 9000 §13.4)
				marking_t _marking;
			private:
				// Оценка задержки приёма-передачи (RFC 9002 §5)
				rtt_t _rtt;
			private:
				// Идентификатор шаблона контекста безопасности (для кэша билетов возобновления)
				tls::coder::id_t _ctx;
				// Флаг сохранения полученного билета возобновления в кэш кодера (сохраняется однократно)
				bool _persisted;
			private:
				// Криптографическое состояние соединения (RFC 9001)
				crypto_t _crypto;
				// Локальные и удалённые транспортные параметры (RFC 9000 §18)
				transport_t _transport;
			private:
				// Переиспользуемые буферы сборки и разбора пакетов
				buffers_t _buffer;
			private:
				// Состояния пространств номеров пакетов
				space_data_t _spaces[SPACES];
			private:
				// Список потоков приложения по идентификаторам
				streams_t _stream;
			private:
				// Очереди датаграмм приложения (RFC 9221)
				dgram_t _dgram;
			private:
				// Лимиты потоков приложения (RFC 9000 §4)
				limits_t _limits;
			private:
				// Контроль потока данных соединения (RFC 9000 §4.1)
				flow_t _flow;
			private:
				/**
				 * \~russian
				 * @brief Пул переиспользуемых учётных записей отправленных пакетов. На каждую
				 *        датаграмму запись создаётся и на подтверждении уничтожается; хранение
				 *        освобождённых записей с сохранённой ёмкостью их векторов снимает
				 *        выделение буфера блоков на каждый пакет (по образцу objpool ngtcp2)
				 *
				 * \~english
				 * @brief Pool of the reusable account records of the sent packets. For every
				 *        datagram a record is created and on the acknowledgment destroyed; a storage of
				 *        the released records with the kept capacity of their vectors removes
				 *        an allocation of the blocks buffer for every packet (by the objpool ngtcp2 pattern)
				 *
				 * \~
				 */
				vector <sent_t> _sentPool;
			private:
				/**
				 * \~russian
				 * @brief Пул переиспользуемых блоков сегментированных приёмных буферов потоков.
				 *        Собранные данные каждого потока копятся до выдачи приложению цепочкой
				 *        блоков фиксированного размера; при сливе блоки возвращаются сюда и
				 *        переиспользуются другими потоками, поэтому рост приёмных буферов не
				 *        порождает перевыделений с копированием (churn роста непрерывной строки)
				 *
				 * \~english
				 * @brief Pool of the reusable blocks of the segmented receiving buffers of the streams.
				 *        The assembled data of every stream accumulates before issuing to the application by a chain
				 *        of blocks of a fixed size; on a merge the blocks return here and
				 *        are reused by other streams, therefore a growth of the receiving buffers does not
				 *        produce reallocations with copying (a churn of the growth of a continuous string)
				 *
				 * \~
				 */
				vector <string> _blockPool;
			private:
				/**
				 * \~russian
				 * @brief Верхняя водяная метка буфера отправки одного потока в октетах (backpressure).
				 *        send() принимает данные лишь до этого объёма несобранного буфера, возвращая
				 *        число принятых байт (частичный приём); остаток дописывает приложение. По
				 *        умолчанию ноль - ограничение снято (буфер растёт по мере поступления), что
				 *        повторяет прежнее поведение движка. Ненулевая метка включает backpressure и
				 *        ограничивает удержание памяти; на синтетических стендах с RTT≈0 меньшая метка
				 *        резко снижает удержание без потери пропускной способности. Ставится sendWaterMarks()
				 *
				 * \~english
				 * @brief High-water mark of the send buffer of one stream in octets (backpressure).
				 *        send() accepts the data only up to this volume of an unpicked buffer, returning
				 *        the number of the accepted bytes (a partial acceptance); the remainder appends the application. By
				 *        default zero - the limit is removed (the buffer grows as it arrives), which
				 *        repeats the former behaviour of the engine. A non-zero mark turns on the backpressure and
				 *        limits the memory holding; on the synthetic stands with RTT≈0 a smaller mark
				 *        sharply reduces the holding without a loss of the throughput. Set by sendWaterMarks()
				 *
				 * \~
				 */
				size_t _sendHigh = 0;
				/**
				 * \~russian
				 * @brief Нижняя водяная метка буфера отправки одного потока в октетах. Когда после
				 *        заполнения до верхней метки буфер дренируется ниже нижней, поток попадает в
				 *        список `drained` (сигнал writable приложению - можно досылать). По умолчанию
				 *        ноль (как и верхняя метка - backpressure выключен); при включении разумно брать
				 *        около половины верхней метки, чтобы приложение возобновляло выдачу, не дожидаясь
				 *        полного опустошения буфера. Ставится sendWaterMarks()
				 *
				 * \~english
				 * @brief Low-water mark of the send buffer of one stream in octets. When after
				 *        a filling up to the upper mark the buffer is drained below the lower one, the stream gets into
				 *        the `drained` list (the writable signal to the application - one can send more). By default
				 *        zero (as well as the upper mark - the backpressure is off); on turning on it is reasonable to take
				 *        about a half of the upper mark, so that the application resumed the issuing
				 *        without waiting for a complete emptying of the buffer. Set by sendWaterMarks()
				 *
				 * \~
				 */
				size_t _sendLow = 0;
			private:
				/**
				 * \~russian
				 * @brief Метод определения пространства номеров пакетов по уровню шифрования
				 *
				 * @param level уровень шифрования
				 * @return      пространство номеров пакетов
				 *
				 * \~english
				 * @brief Method of the determination of the space of the numbers of the packets by the level of the encryption
				 *
				 * @param level level of the encryption
				 * @return      space of the numbers of the packets
				 *
				 * \~
				 */
				space_t space(const level_t level) const noexcept;
				/**
				 * \~russian
				 * @brief Метод регистрации принятого номера пакета в диапазонах пространства
				 *
				 * @param space пространство номеров пакетов
				 * @param pn    принятый номер пакета
				 *
				 * \~english
				 * @brief Method of the registration of an accepted number of a packet in the ranges of the space
				 *
				 * @param space space of the numbers of the packets
				 * @param pn    accepted number of the packet
				 *
				 * \~
				 */
				void record(const space_t space, const uint64_t pn) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки повторного приёма номера пакета
				 *
				 * @param space пространство номеров пакетов
				 * @param pn    принятый номер пакета
				 * @return      результат проверки (true - пакет уже был принят)
				 *
				 * \~english
				 * @brief Method of checking a repeated acceptance of a number of a packet
				 *
				 * @param space space of the numbers of the packets
				 * @param pn    accepted number of the packet
				 * @return      result of the checking (true - the packet has already been accepted)
				 *
				 * \~
				 */
				bool duplicate(const space_t space, const uint64_t pn) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод перекладывания исходящих CRYPTO-данных хендшейка в буферы пространств
				 *
				 * \~english
				 * @brief Method of the shifting of the outgoing CRYPTO data of the handshake into the buffers of the spaces
				 *
				 * \~
				 */
				void pull() noexcept;
				/**
				 * \~russian
				 * @brief Метод постановки завершения соединения с ошибкой транспорта в очередь
				 *
				 * @param error код ошибки транспорта
				 *
				 * \~english
				 * @brief Method of the putting of a completion of the connection with an error of the transport into the queue
				 *
				 * @param error error code of the transport
				 *
				 * \~
				 */
				void fail(const error_t error) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сброса ключей уровня вместе с состоянием восстановления потерь (RFC 9001 §4.9)
				 *
				 * @param level уровень шифрования
				 *
				 * \~english
				 * @brief Method of the reset of the keys of a level together with the state of the recovery of the losses (RFC 9001 §4.9)
				 *
				 * @param level level of the encryption
				 *
				 * \~
				 */
				void discard(const level_t level) noexcept;
				/**
				 * \~russian
				 * @brief Метод обновления оценки задержки приёма-передачи (RFC 9002 §5.3)
				 *
				 * @param sample измеренная задержка приёма-передачи
				 * @param delay  задержка подтверждения удалённого эндпоинта
				 *
				 * \~english
				 * @brief Method of the updating of the estimation of the delay of the reception-transmission (RFC 9002 §5.3)
				 *
				 * @param sample measured delay of the reception-transmission
				 * @param delay  delay of the acknowledgement of the remote endpoint
				 *
				 * \~
				 */
				void rtt(const uint64_t sample, const uint64_t delay) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения переиспользуемой учётной записи пакета из пула
				 *
				 * @return учётная запись с очищенными полями и сохранённой ёмкостью векторов
				 *
				 * \~english
				 * @brief Method of getting a reused account record of a packet from the pool
				 *
				 * @return account record with the cleared fields and the preserved capacity of the vectors
				 *
				 * \~
				 */
				sent_t acquire() noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата учётной записи пакета в пул для переиспользования
				 *
				 * @param record освобождаемая учётная запись отправленного пакета
				 *
				 * \~english
				 * @brief Method of the return of an account record of a packet into the pool for a reuse
				 *
				 * @param record account record of the sent packet being released
				 *
				 * \~
				 */
				void recycle(sent_t & record) noexcept;
				/**
				 * \~russian
				 * @brief Метод повторной постановки содержимого пакета в очереди отправки (RFC 9002 §6.3)
				 *
				 * @param space  пространство номеров пакетов
				 * @param packet учётная запись потерянного либо зондируемого пакета
				 *
				 * \~english
				 * @brief Method of a repeated putting of the content of a packet into the queues of the sending (RFC 9002 §6.3)
				 *
				 * @param space  space of the numbers of the packets
				 * @param packet account record of the lost or of the probed packet
				 *
				 * \~
				 */
				void requeue(const space_t space, const sent_t & packet) noexcept;
				/**
				 * \~russian
				 * @brief Метод учёта подтверждения отправленного блока данных потока (RFC 9000 §13.3)
				 *
				 * @note Продвигает непрерывно подтверждённый префикс буфера отправки потока,
				 *       позволяя уплотнению освободить подтверждённые данные. Удерживаемые
				 *       до подтверждения данные перечитываются из буфера при ретрансмиссии
				 *
				 * @param chunk отправленный блок данных потока из учётной записи пакета
				 *
				 * \~english
				 * @brief Method of the account of the acknowledgement of a sent block of the data of a stream (RFC 9000 §13.3)
				 *
				 * @note It advances the continuously acknowledged prefix of the buffer of the sending of the stream,
				 *       allowing the compaction to free the acknowledged data. The data held
				 *       until the acknowledgement is reread from the buffer at a retransmission
				 *
				 * @param chunk sent block of the data of the stream from the account record of the packet
				 *
				 * \~
				 */
				void settle(const chunk_t & chunk) noexcept;
				/**
				 * \~russian
				 * @brief Метод постановки потока в список ожидающих управляющих фреймов
				 *
				 * @note Поток добавляется в список не более одного раза (флаг `controlPending`),
				 *       чтобы обход управляющих фреймов шёл по короткому списку, а не по всему
				 *       списку потоков. Вызывается по месту постановки управляющего фрейма в очередь
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the putting of a stream into the list of those awaiting the control frames
				 *
				 * @note A stream is added into the list not more than once (the flag `controlPending`),
				 *       so that the traversal of the control frames would go by a short list rather than by the whole
				 *       list of the streams. It is called at the place of the putting of a control frame into the queue
				 *
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void schedule(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод постановки потока в набор потоков с данными к отправке
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the putting of a stream into the collection of the streams with the data for the sending
				 *
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void pending(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления потока из набора потоков с данными к отправке
				 *
				 * @param sid идентификатор потока
				 *
				 * \~english
				 * @brief Method of the removal of a stream from the collection of the streams with the data for the sending
				 *
				 * @param sid identifier of the stream
				 *
				 * \~
				 */
				void dequeue(const uint64_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки пути на поддержку ECN по счётчикам подтверждения (RFC 9000 §13.4.2.1)
				 *
				 * @note Непройденная проверка снимает маркировку с исходящих датаграмм
				 *       до смены пути: счётчики перегрузки стали недостоверны, и
				 *       реагировать на них сокращением окна нельзя
				 *
				 * @param space  пространство номеров пакетов
				 * @param frame  разобранный фрейм подтверждения со счётчиками маркировок
				 * @param marked количество впервые подтверждённых помеченных пакетов
				 * @return       результат обнаружения прироста счётчика перегрузки
				 *
				 * \~english
				 * @brief Method of checking the path for the support of ECN by the counters of the acknowledgement (RFC 9000 §13.4.2.1)
				 *
				 * @note A not passed check removes the marking from the outgoing datagrams
				 *       until a change of the path: the counters of the congestion have become unreliable, and
				 *       to react to them by a reduction of the window is impossible
				 *
				 * @param space  space of the numbers of the packets
				 * @param frame  parsed frame of the acknowledgement with the counters of the markings
				 * @param marked number of the first-time acknowledged marked packets
				 * @return       result of the detection of an increment of the counter of the congestion
				 *
				 * \~
				 */
				bool validate(const space_t space, const frame::ack_t & frame, const uint64_t marked) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата содержимого отправленных ранних данных в очереди отправки (RFC 9001 §4.6.2)
				 *
				 * @note Вызывается при отказе удалённого узла в ранних данных: отправленные
				 *       ранние пакеты удалённым узлом не разбирались и подтверждены никогда
				 *       не будут, поэтому их содержимое возвращается в очереди отправки
				 *       немедленно, не дожидаясь детекта потерь. Событием перегрузки
				 *       отказ не является - пакеты пути не теряли
				 *
				 * \~english
				 * @brief Method of the return of the content of the sent early data into the queues of the sending (RFC 9001 §4.6.2)
				 *
				 * @note It is called at a refusal of the remote node in the early data: the sent
				 *       early packets have not been parsed by the remote node and will never be
				 *       acknowledged, therefore their content is returned into the queues of the sending
				 *       immediately without waiting for the detection of the losses. The refusal is not
				 *       an event of a congestion - the packets have not been lost by the path
				 *
				 * \~
				 */
				void restore() noexcept;
				/**
				 * \~russian
				 * @brief Метод продвижения поиска размера пути (RFC 8899 §5.3)
				 *
				 * @note Поиск ведётся делением интервала между подтверждённым размером
				 *       и верхней границей: подтверждённый зонд поднимает нижнюю
				 *       границу, потерянный - опускает верхнюю
				 *
				 * \~english
				 * @brief Method of the advancement of the search of the size of the path (RFC 8899 §5.3)
				 *
				 * @note The search is conducted by a division of the interval between the confirmed size
				 *       and the upper boundary: a confirmed probe raises the lower
				 *       boundary, a lost one lowers the upper
				 *
				 * \~
				 */
				void discover() noexcept;
				/**
				 * \~russian
				 * @brief Метод учёта потери полноразмерной датаграммы для детекции чёрной дыры пути (RFC 8899 §5.4)
				 *
				 * @note Наращивает счётчик подряд потерянных датаграмм сверх базового
				 *       размера. По исчерпании предела проб подтверждённый размер пути
				 *       опускается к обязательному минимуму, а поиск начинается заново:
				 *       иначе передача навсегда встаёт на переставшем проходить размере
				 *
				 * \~english
				 * @brief Method of the account of a loss of a full-size datagram for the detection of a black hole of the path (RFC 8899 §5.4)
				 *
				 * @note It grows the counter of the consecutively lost datagrams above the base
				 *       size. At the exhaustion of the limit of the trials the confirmed size of the path
				 *       is lowered to the obligatory minimum, while the search begins anew:
				 *       otherwise the transmission stands forever at a size which has ceased to pass
				 *
				 * \~
				 */
				void deflate() noexcept;
				/**
				 * \~russian
				 * @brief Метод детекта потерянных пакетов пространства (RFC 9002 §6.1)
				 *
				 * @note Потерянные CRYPTO-данные и фрейм HANDSHAKE_DONE ставятся
				 *       в очередь ретрансмиссии
				 *
				 * @param space      пространство номеров пакетов
				 * @param ackedTimes времена отправки пакетов, подтверждённых текущим фреймом
				 *                   ACK (по возрастанию), для разрыва периода устойчивой
				 *                   перегрузки на подтверждении внутри серии потерь
				 *
				 * \~english
				 * @brief Method of the detection of the lost packets of a space (RFC 9002 §6.1)
				 *
				 * @note The lost CRYPTO data and the frame HANDSHAKE_DONE are put
				 *       into the queue of the retransmission
				 *
				 * @param space      space of the numbers of the packets
				 * @param ackedTimes times of the sending of the packets acknowledged by the current frame
				 *                   ACK (in an ascending order), for a break of the period of a persistent
				 *                   congestion at an acknowledgement inside a series of the losses
				 *
				 * \~
				 */
				void detect(const space_t space, const vector <uint64_t> & ackedTimes = {}) noexcept;
				/**
				 * \~russian
				 * @brief Метод постановки зондирующих данных пространства в очередь (RFC 9002 §6.2.4)
				 *
				 * @param space пространство номеров пакетов
				 *
				 * \~english
				 * @brief Method of the putting of the probing data of a space into the queue (RFC 9002 §6.2.4)
				 *
				 * @param space space of the numbers of the packets
				 *
				 * \~
				 */
				void probe(const space_t space) noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления интервала таймера PTO (RFC 9002 §6.2.1)
				 *
				 * @param space пространство номеров пакетов
				 * @return      интервал таймера PTO в миллисекундах
				 *
				 * \~english
				 * @brief Method of the calculation of the interval of the timer PTO (RFC 9002 §6.2.1)
				 *
				 * @param space space of the numbers of the packets
				 * @return      interval of the timer PTO in milliseconds
				 *
				 * \~
				 */
				uint64_t interval(const space_t space) const noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления дедлайна таймера PTO пространства (RFC 9002 §6.2.1)
				 *
				 * @param space пространство номеров пакетов
				 * @return      дедлайн таймера PTO в миллисекундах (0 - таймер не взведён)
				 *
				 * \~english
				 * @brief Method of the calculation of the deadline of the timer PTO of a space (RFC 9002 §6.2.1)
				 *
				 * @param space space of the numbers of the packets
				 * @return      deadline of the timer PTO in milliseconds (0 - the timer is not raised)
				 *
				 * \~
				 */
				uint64_t deadline(const space_t space) const noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления длительности периода устойчивой перегрузки (RFC 9002 §7.6.1)
				 *
				 * @return длительность периода устойчивой перегрузки в миллисекундах
				 *
				 * \~english
				 * @brief Method of the calculation of the duration of the period of a persistent congestion (RFC 9002 §7.6.1)
				 *
				 * @return duration of the period of the persistent congestion in milliseconds
				 *
				 * \~
				 */
				uint64_t persistence() const noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления дедлайна таймаута простоя соединения (RFC 9000 §10.1)
				 *
				 * @return дедлайн таймаута простоя в миллисекундах (0 - таймаут не согласован)
				 *
				 * \~english
				 * @brief Method of the calculation of the deadline of the timeout of the idleness of the connection (RFC 9000 §10.1)
				 *
				 * @return deadline of the timeout of the idleness in milliseconds (0 - the timeout is not agreed)
				 *
				 * \~
				 */
				uint64_t idle() const noexcept;
				/**
				 * \~russian
				 * @brief Метод учёта данных потока потреблёнными в flow control соединения
				 *
				 * @param stream состояние потока
				 * @param target учтённое смещение данных потока в октетах
				 *
				 * \~english
				 * @brief Method of the account of the data of a stream as consumed in the flow control of the connection
				 *
				 * @param stream state of the stream
				 * @param target accounted displacement of the data of the stream in octets
				 *
				 * \~
				 */
				void consume(stream_data_t & stream, const uint64_t target) noexcept;
				/**
				 * \~russian
				 * @brief Метод обнаружения сброса без сохранения состояния (RFC 9000 §10.3.1)
				 *
				 * @note Сброс опознаётся по совпадению последних 16 октетов датаграммы
				 *       с одним из токенов, полученных от удалённого эндпоинта для
				 *       использованных идентификаторов соединения
				 *
				 * @param data буфер принятой датаграммы
				 * @param size размер принятой датаграммы
				 * @return     результат обнаружения (true - датаграмма является сбросом)
				 *
				 * \~english
				 * @brief Method of the detection of a stateless reset (RFC 9000 §10.3.1)
				 *
				 * @note A reset is recognized by a coincidence of the last 16 octets of a datagram
				 *       with one of the tokens obtained from the remote endpoint for
				 *       the used identifiers of the connection
				 *
				 * @param data buffer of the accepted datagram
				 * @param size size of the accepted datagram
				 * @return     result of the detection (true - the datagram is a reset)
				 *
				 * \~
				 */
				bool stateless(const uint8_t * data, const size_t size) const noexcept;
				/**
				 * \~russian
				 * @brief Метод формирования ключа сервера для кэша билетов возобновления (RFC 9001 §4.6)
				 *
				 * @note Ключом выступает доменное имя сервера (SNI) шаблона контекста
				 *       безопасности, а при его отсутствии - опаковое представление
				 *       адреса удалённого эндпоинта
				 *
				 * @return ключ сервера для кэша билетов возобновления
				 *
				 * \~english
				 * @brief Method of the forming of the key of the server for the cache of the tickets of the resumption (RFC 9001 §4.6)
				 *
				 * @note The key is the domain name of the server (SNI) of the template of the context
				 *       of the safety, while at its absence - an opaque representation
				 *       of the address of the remote endpoint
				 *
				 * @return key of the server for the cache of the tickets of the resumption
				 *
				 * \~
				 */
				string sessionKey() const noexcept;
				/**
				 * \~russian
				 * @brief Метод сохранения полученного билета возобновления в кэш кодера (RFC 9001 §4.6)
				 *
				 * @note Билет присылается сервером уже после установления соединения;
				 *       сохранение выполняется однократно и делает возобновление сессии
				 *       прозрачным для вызывающего кода
				 *
				 * \~english
				 * @brief Method of the preservation of an obtained ticket of the resumption into the cache of the coder (RFC 9001 §4.6)
				 *
				 * @note The ticket is sent by the server already after the establishment of the connection;
				 *       the preservation is performed once and makes the resumption of the session
				 *       transparent for the calling code
				 *
				 * \~
				 */
				void persist() noexcept;
				/**
				 * \~russian
				 * @brief Метод формирования токена проверки адреса клиента (RFC 9000 §8.1.4)
				 *
				 * @note Токен содержит отметку времени и исходный DCID, заверенные кодом
				 *       аутентичности от адреса клиента: проверка выполняется без
				 *       сохранения состояния на стороне сервера
				 *
				 * @param mark   метка формата токена (выдан пакетом Retry либо фреймом NEW_TOKEN)
				 * @param odcid  исходный DCID первого пакета Initial клиента (пустой для фрейма NEW_TOKEN)
				 * @param output сформированный токен проверки адреса
				 * @return       результат формирования (false - ошибка генератора либо кода аутентичности)
				 *
				 * \~english
				 * @brief Method of the forming of a token of the checking of the address of a client (RFC 9000 §8.1.4)
				 *
				 * @note The token contains a time mark and the source DCID certified by a code
				 *       of the authenticity from the address of the client: the checking is performed without
				 *       a storing of a state on the side of the server
				 *
				 * @param mark   mark of the format of the token (issued by a Retry packet or by a NEW_TOKEN frame)
				 * @param odcid  source DCID of the first Initial packet of the client (empty for a NEW_TOKEN frame)
				 * @param output formed token of the checking of the address
				 * @return       result of the forming (false - an error of the generator or of the code of the authenticity)
				 *
				 * \~
				 */
				bool token(const uint8_t mark, const cid_t & odcid, string & output) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки токена проверки адреса клиента (RFC 9000 §8.1.4)
				 *
				 * @param token   принятый токен проверки адреса
				 * @param odcid   восстановленный исходный DCID первого пакета Initial клиента
				 * @param retried признак выдачи токена пакетом Retry, а не фреймом NEW_TOKEN
				 * @return        результат проверки (true - токен выдан этому адресу и не истёк)
				 *
				 * \~english
				 * @brief Method of the checking of a token of the checking of the address of a client (RFC 9000 §8.1.4)
				 *
				 * @param token   accepted token of the checking of the address
				 * @param odcid   restored source DCID of the first Initial packet of the client
				 * @param retried flag of the issue of the token by a Retry packet rather than by a NEW_TOKEN frame
				 * @return        result of the checking (true - the token is issued to this address and has not expired)
				 *
				 * \~
				 */
				bool validate(string_view token, cid_t & odcid, bool & retried) const noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления доступного к отправке объёма данных (RFC 9000 §8.1)
				 *
				 * @note До подтверждения адреса удалённого эндпоинта сервер не вправе
				 *       отправить более трёхкратного объёма принятых от него октетов
				 *
				 * @return доступный к отправке объём данных в октетах
				 *
				 * \~english
				 * @brief Method of the calculation of the volume of the data available for the sending (RFC 9000 §8.1)
				 *
				 * @note Before the confirmation of the address of the remote endpoint a server is not entitled
				 *       to send more than a threefold volume of the octets accepted from it
				 *
				 * @return volume of the data available for the sending in octets
				 *
				 * \~
				 */
				size_t allowance() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки невозможности отправки под лимитом анти-амплификации (RFC 9002 §6.2.2.1)
				 *
				 * @note Остаток лимита меньше минимального размера защищённого пакета
				 *       отправку запрещает не менее надёжно, чем исчерпанный нацело:
				 *       ни один пакет в него не помещается
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the impossibility of a sending under the limit of the anti-amplification (RFC 9002 §6.2.2.1)
				 *
				 * @note A remainder of the limit smaller than the smallest size of a protected packet
				 *       prohibits the sending not less reliably than an entirely exhausted one:
				 *       not a single packet fits into it
				 *
				 * @return result of the checking
				 *
				 * \~
				 */
				bool stalled() const noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления завершённых потоков приложения
				 *
				 * @note Поток удаляется только когда обе стороны достигли терминального
				 *       состояния и на него не ссылаются очередь ретрансмиссии и учётные
				 *       записи неподтверждённых пакетов
				 *
				 * \~english
				 * @brief Method of the removal of the completed streams of the application
				 *
				 * @note A stream is removed only when both the sides have reached a terminal
				 *       state and neither the queue of the retransmission nor the account
				 *       records of the unacknowledged packets refer to it
				 *
				 * \~
				 */
				void collect() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод вывода ключей следующей фазы уровня приложения (RFC 9001 §6)
				 *
				 * @note Вызывается после подтверждения хендшейка - ключи следующей
				 *       фазы выводятся заранее для приёма обновления от пира
				 *
				 * \~english
				 * @brief Method of the derivation of the keys of the next phase of the level of the application (RFC 9001 §6)
				 *
				 * @note It is called after the confirmation of the handshake - the keys of the next
				 *       phase are derived beforehand for the acceptance of an update from the peer
				 *
				 * \~
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод переключения на следующую фазу ключей уровня приложения (RFC 9001 §6)
				 *
				 * @note Текущие ключи чтения сохраняются для отставших пакетов
				 *       предыдущей фазы, выводятся ключи новой следующей фазы
				 *
				 * \~english
				 * @brief Method of the switching to the next phase of the keys of the level of the application (RFC 9001 §6)
				 *
				 * @note The current keys of the reading are preserved for the belated packets
				 *       of the previous phase, the keys of the new next phase are derived
				 *
				 * \~
				 */
				void promote() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод учёта подтверждённого пакета в congestion control (RFC 9002 §7.3.1)
				 *
				 * @param packet учётная запись подтверждённого пакета
				 *
				 * \~english
				 * @brief Method of the account of an acknowledged packet in the congestion control (RFC 9002 §7.3.1)
				 *
				 * @param packet account record of the acknowledged packet
				 *
				 * \~
				 */
				void acked(const sent_t & packet) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события перегрузки при детекте потерь (RFC 9002 §7.3.2)
				 *
				 * @param time время отправки наиболее позднего потерянного пакета
				 *
				 * \~english
				 * @brief Method of the processing of an event of a congestion at the detection of the losses (RFC 9002 §7.3.2)
				 *
				 * @param time time of the sending of the latest lost packet
				 *
				 * \~
				 */
				void congestion(const uint64_t time) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод выдачи дополнительных идентификаторов соединения (RFC 9000 §5.1.1)
				 *
				 * @note Вызывается после установления соединения - количество
				 *       ограничено лимитом active_connection_id_limit удалённого эндпоинта
				 *
				 * \~english
				 * @brief Method of the issue of the additional identifiers of the connection (RFC 9000 §5.1.1)
				 *
				 * @note It is called after the establishment of the connection - the number
				 *       is limited by the limit active_connection_id_limit of the remote endpoint
				 *
				 * \~
				 */
				void issue() noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки закреплённости идентификатора за предпочтительным адресом (RFC 9000 §5.1.1)
				 *
				 * @note Идентификатор с порядковым номером 1 выдан сервером вместе с
				 *       предпочтительным адресом и предназначен для переезда на него.
				 *       Занятый прочими нуждами, он делает переезд невозможным
				 *
				 * @param seq порядковый номер идентификатора соединения
				 * @return    результат проверки
				 *
				 * \~english
				 * @brief Method of checking the attachment of an identifier to the preferred address (RFC 9000 §5.1.1)
				 *
				 * @note The identifier with the ordinal number 1 is issued by the server together with
				 *       the preferred address and is intended for the moving onto it.
				 *       Occupied by the other needs, it makes the moving impossible
				 *
				 * @param seq ordinal number of the identifier of the connection
				 * @return    result of the checking
				 *
				 * \~
				 */
				bool reserved(const uint64_t seq) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки возможности отправки данных в поток локальным эндпоинтом
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки (true - отправка допустима)
				 *
				 * \~english
				 * @brief Method of checking the possibility of a sending of the data into a stream by the local endpoint
				 *
				 * @param sid identifier of the stream
				 * @return    result of the checking (true - the sending is admissible)
				 *
				 * \~
				 */
				bool sendable(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки возможности приёма данных потока локальным эндпоинтом
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки (true - приём допустим)
				 *
				 * \~english
				 * @brief Method of checking the possibility of an acceptance of the data of a stream by the local endpoint
				 *
				 * @param sid identifier of the stream
				 * @return    result of the checking (true - the acceptance is admissible)
				 *
				 * \~
				 */
				bool receivable(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения начального лимита приёма потока из локальных параметров
				 *
				 * @param sid идентификатор потока
				 * @return    начальный лимит приёма потока в октетах
				 *
				 * \~english
				 * @brief Method of getting the initial limit of the acceptance of a stream from the local parameters
				 *
				 * @param sid identifier of the stream
				 * @return    initial limit of the acceptance of the stream in octets
				 *
				 * \~
				 */
				uint64_t rxWindow(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения начального лимита отправки потока из параметров удалённого эндпоинта
				 *
				 * @param sid идентификатор потока
				 * @return    начальный лимит отправки потока в октетах
				 *
				 * \~english
				 * @brief Method of getting the initial limit of the sending of a stream from the parameters of the remote endpoint
				 *
				 * @param sid identifier of the stream
				 * @return    initial limit of the sending of the stream in octets
				 *
				 * \~
				 */
				uint64_t txWindow(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска либо создания потока по принятому фрейму (RFC 9000 §3.2)
				 *
				 * @note Проверяет допустимость идентификатора и лимиты потоков;
				 *       поток удалённого эндпоинта создаётся неявно
				 *
				 * @param sid   идентификатор потока
				 * @param error код ошибки транспорта
				 * @return      состояние потока (nullptr - нарушение протокола)
				 *
				 * \~english
				 * @brief Method of the search or of the creation of a stream by an accepted frame (RFC 9000 §3.2)
				 *
				 * @note It checks the admissibility of the identifier and the limits of the streams;
				 *       a stream of the remote endpoint is created implicitly
				 *
				 * @param sid   identifier of the stream
				 * @param error error code of the transport
				 * @return      state of the stream (a nullptr - a violation of the protocol)
				 *
				 * \~
				 */
				stream_data_t * accept(const uint64_t sid, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки закрытости потока (RFC 9000 §3.2)
				 *
				 * @note Поток закрыт, если он был открыт (локально либо удалённым
				 *       эндпоинтом, в том числе неявно), но в списке потоков отсутствует -
				 *       собран сборщиком. Фреймы такого потока являются ретрансмиссией
				 *       уже завершённого потока и должны игнорироваться, а не воскрешать
				 *       поток заново (ngtcp2-подобная модель через эгерную материализацию
				 *       неявно открытых потоков в accept())
				 *
				 * @param sid идентификатор потока
				 * @return    результат проверки (true - поток был открыт и уже собран)
				 *
				 * \~english
				 * @brief Method of checking the closedness of a stream (RFC 9000 §3.2)
				 *
				 * @note A stream is closed, if it has been opened (locally or by the remote
				 *       endpoint, including implicitly) but is absent from the list of the streams -
				 *       collected by the collector. The frames of such a stream are a retransmission
				 *       of an already completed stream and are obliged to be ignored rather than to resurrect
				 *       the stream anew (an ngtcp2-like model through an eager materialization
				 *       of the implicitly opened streams in accept())
				 *
				 * @param sid identifier of the stream
				 * @return    result of the checking (true - the stream has been opened and is already collected)
				 *
				 * \~
				 */
				bool closed(const uint64_t sid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки принятого фрейма STREAM (RFC 9000 §19.8)
				 *
				 * @param frame принятый фрейм данных потока приложения
				 * @return      результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the processing of an accepted STREAM frame (RFC 9000 §19.8)
				 *
				 * @param frame accepted frame of the data of a stream of the application
				 * @return      result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				status_t inputStream(const frame::stream_t & frame) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения транспортных параметров удалённого эндпоинта после хендшейка
				 *
				 * @return результат применения (true - параметры применены)
				 *
				 * \~english
				 * @brief Method of the application of the transport parameters of the remote endpoint after the handshake
				 *
				 * @return result of the application (true - the parameters are applied)
				 *
				 * \~
				 */
				bool established() noexcept;
				/**
				 * \~russian
				 * @brief Метод учёта завершения потока удалённого эндпоинта в лимите MAX_STREAMS
				 *
				 * @param sid    идентификатор потока
				 * @param stream состояние потока
				 *
				 * \~english
				 * @brief Method of the account of the completion of a stream of the remote endpoint in the limit MAX_STREAMS
				 *
				 * @param sid    identifier of the stream
				 * @param stream state of the stream
				 *
				 * \~
				 */
				void credit(const uint64_t sid, stream_data_t & stream) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки готовности потока к выдаче данных приложению
				 *
				 * @param stream состояние потока
				 * @return       результат проверки (true - есть что выдать приложению)
				 *
				 * \~english
				 * @brief Method of checking the readiness of a stream for the issue of the data to the application
				 *
				 * @param stream state of the stream
				 * @return       result of the checking (true - there is something to issue to the application)
				 *
				 * \~
				 */
				bool ready(const stream_data_t & stream) const noexcept;
				/**
				 * \~russian
				 * @brief Метод постановки потока в список готовых к выдаче
				 *
				 * @note Вызывается в местах, где готовность потока может появиться.
				 *       Утрата готовности отдельной обработки не требует: устаревшие
				 *       записи отсеиваются при чтении списка
				 *
				 * @param sid    идентификатор потока
				 * @param stream состояние потока
				 *
				 * \~english
				 * @brief Method of the putting of a stream into the list of those ready for the issue
				 *
				 * @note It is called at the places where a readiness of a stream may appear.
				 *       A loss of the readiness does not require a separate processing: the outdated
				 *       records are sifted out at the reading of the list
				 *
				 * @param sid    identifier of the stream
				 * @param stream state of the stream
				 *
				 * \~
				 */
				void notify(const uint64_t sid, stream_data_t & stream) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки входящих CRYPTO-данных со сборкой по смещениям
				 *
				 * @param level  уровень шифрования пакета с CRYPTO-фреймом
				 * @param offset смещение данных в потоке криптографического хендшейка
				 * @param data   данные CRYPTO-фрейма
				 * @return       результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the processing of the incoming CRYPTO data with an assembly by the displacements
				 *
				 * @param level  level of the encryption of the packet with the CRYPTO frame
				 * @param offset displacement of the data in the stream of the cryptographic handshake
				 * @param data   data of the CRYPTO frame
				 * @return       result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				status_t input(const level_t level, const uint64_t offset, string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора и диспетчеризации фреймов расшифрованной нагрузки пакета
				 *
				 * @param level      уровень шифрования пакета
				 * @param data       буфер расшифрованной нагрузки
				 * @param size       размер расшифрованной нагрузки
				 * @param nonProbing признак наличия непробирующего фрейма (RFC 9000 §9.1)
				 * @return           результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the parsing and of the dispatching of the frames of the decrypted payload of a packet
				 *
				 * @param level      level of the encryption of the packet
				 * @param data       buffer of the decrypted payload
				 * @param size       size of the decrypted payload
				 * @param nonProbing flag of the presence of a non-probing frame (RFC 9000 §9.1)
				 * @return           result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				status_t frames(const level_t level, const uint8_t * data, const size_t size, bool & nonProbing) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сборки нагрузки очередного пакета уровня шифрования
				 *
				 * @param level   уровень шифрования пакета
				 * @param budget  доступно октетов в датаграмме для нагрузки
				 * @param output  собранная нагрузка пакета (фреймы)
				 * @param meta    учётная запись пакета для восстановления потерь
				 * @param elicit  флаг наличия ack-eliciting фреймов в нагрузке
				 * @param limited флаг исчерпанного окна перегрузки (только подтверждения)
				 * @return        результат сборки (true - нагрузка не пустая)
				 *
				 * \~english
				 * @brief Method of the assembly of the payload of the next packet of a level of the encryption
				 *
				 * @param level   level of the encryption of the packet
				 * @param budget  octets available in the datagram for the payload
				 * @param output  assembled payload of the packet (the frames)
				 * @param meta    account record of the packet for the recovery of the losses
				 * @param elicit  flag of the presence of the ack-eliciting frames in the payload
				 * @param limited flag of an exhausted window of the congestion (only the acknowledgements)
				 * @return        result of the assembly (true - the payload is not empty)
				 *
				 * \~
				 */
				bool payload(const level_t level, const size_t budget, string & output, sent_t & meta, bool & elicit, const bool limited) noexcept;
				/**
				 * \~russian
				 * @brief Метод вычисления размера заголовка пакета уровня шифрования
				 *
				 * @param level  уровень шифрования пакета
				 * @param length значение поля Length (номер пакета + нагрузка + тег AEAD)
				 * @param pnSize размер кодирования номера пакета в октетах
				 * @param dcid   идентификатор соединения получателя пакета
				 * @return       размер заголовка пакета в октетах
				 *
				 * \~english
				 * @brief Method of the calculation of the size of the header of a packet of a level of the encryption
				 *
				 * @param level  level of the encryption of the packet
				 * @param length value of the field Length (the number of the packet + the payload + the tag AEAD)
				 * @param pnSize size of the encoding of the number of the packet in octets
				 * @param dcid   identifier of the connection of the receiver of the packet
				 * @return       size of the header of the packet in octets
				 *
				 * \~
				 */
				size_t headerSize(const level_t level, const uint64_t length, const size_t pnSize, const cid_t & dcid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки и защиты пакета уровня шифрования (заголовок + нагрузка)
				 *
				 * @param output  выходной буфер датаграммы (пакет дописывается)
				 * @param level   уровень шифрования пакета
				 * @param payload нагрузка пакета (фреймы)
				 * @param dcid    идентификатор соединения получателя пакета
				 * @return        результат сборки (false - ошибка криптографической библиотеки)
				 *
				 * \~english
				 * @brief Method of the assembly and of the protection of a packet of a level of the encryption (the header + the payload)
				 *
				 * @param output  output buffer of the datagram (the packet is appended)
				 * @param level   level of the encryption of the packet
				 * @param payload payload of the packet (the frames)
				 * @param dcid    identifier of the connection of the receiver of the packet
				 * @return        result of the assembly (false - an error of the cryptographic library)
				 *
				 * \~
				 */
				bool seal(string & output, const level_t level, string_view payload, const cid_t & dcid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения согласованного ALPN-протокола
				 *
				 * @note Список поддерживаемых протоколов задаётся на контексте кодера
				 *       транспортной безопасности, из которого создано соединение
				 *
				 * @return согласованный ALPN-протокол (пустое название - согласование не выполнено)
				 *
				 * \~english
				 * @brief Method of the extraction of the agreed ALPN protocol
				 *
				 * @note The list of the supported protocols is set at the context of the coder
				 *       of the transport safety out of which the connection is created
				 *
				 * @return agreed ALPN protocol (an empty name - the agreement is not performed)
				 *
				 * \~
				 */
				tls::coder::alpn_t alpn() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения возобновляемой сессии соединения (RFC 9001 §4.6)
				 *
				 * @note Сессия становится доступна после приёма билета возобновления,
				 *       который сервер присылает уже после установления соединения.
				 *       Сохранив её, вызывающий код возобновляет соединение с тем же
				 *       сервером, не выполняя полного хендшейка
				 *
				 * @return сериализованная сессия (пусто - сессия недоступна)
				 *
				 * \~english
				 * @brief Method of the extraction of the resumable session of the connection (RFC 9001 §4.6)
				 *
				 * @note The session becomes accessible after the acceptance of a ticket of the resumption
				 *       which the server sends already after the establishment of the connection.
				 *       Having preserved it, the calling code resumes a connection with the same
				 *       server without performing a full handshake
				 *
				 * @return serialized session (empty - the session is inaccessible)
				 *
				 * \~
				 */
				string session() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки возобновляемой сессии соединения (RFC 9001 §4.6)
				 *
				 * @note Устанавливается до начала соединения и только на клиенте
				 *
				 * @param session сериализованная сессия
				 * @return        результат установки
				 *
				 * \~english
				 * @brief Method of setting the resumable session of the connection (RFC 9001 §4.6)
				 *
				 * @note It is set before the beginning of the connection and only at a client
				 *
				 * @param session serialized session
				 * @return        result of the setting
				 *
				 * \~
				 */
				bool session(string_view session) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки принятия ранних данных удалённым узлом (RFC 9001 §4.6.2)
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the acceptance of the early data by the remote node (RFC 9001 §4.6.2)
				 *
				 * @return result of the checking
				 *
				 * \~
				 */
				bool early() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки готовности соединения к отправке данных приложения
				 *
				 * @note Готовность наступает и до завершения хендшейка, если сессия
				 *       возобновлена: вызывающему коду это нужно, чтобы поставить
				 *       ранние данные в очередь вовремя (RFC 9001 §4.6)
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the readiness of the connection for the sending of the data of the application
				 *
				 * @note The readiness comes also before the completion of the handshake, if the session
				 *       is resumed: the calling code needs this in order to put
				 *       the early data into the queue in time (RFC 9001 §4.6)
				 *
				 * @return result of the checking
				 *
				 * \~
				 */
				bool writable() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки водяных меток буфера отправки одного потока (backpressure)
				 *
				 * @note Верхняя метка ограничивает объём несобранных данных, принимаемых в
				 *       очередь одного потока: сверх неё send() принимает данные лишь частично
				 *       (возвращает число принятых байт), остаток дописывает приложение. Больше,
				 *       чем движок способен держать в полёте, буферизировать бессмысленно, а
				 *       неограниченный буфер - латентный источник неограниченного роста памяти.
				 *       Когда буфер, дойдя до верхней метки, дренируется ниже нижней, поток
				 *       попадает в список drained() - сигнал приложению возобновить выдачу.
				 *       Верхняя метка ноль снимает ограничение (буфер растёт по мере поступления)
				 *
				 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
				 * @param low  нижняя водяная метка (порог сигнала возобновления writable)
				 *
				 * \~english
				 * @brief Method of setting the water marks of the buffer of the sending of a single stream (a backpressure)
				 *
				 * @note The upper mark limits the volume of the unassembled data accepted into
				 *       the queue of a single stream: above it send() accepts the data only partially
				 *       (it returns the number of the accepted octets), the remainder is appended by the application. To buffer more
				 *       than the engine is capable of holding in a flight is senseless, while
				 *       an unlimited buffer is a latent source of an unlimited growth of the memory.
				 *       When the buffer, having reached the upper mark, drains below the lower one, the stream
				 *       gets into the list drained() - a signal to the application to resume the issue.
				 *       An upper mark of zero removes the limitation (the buffer grows as the data arrives)
				 *
				 * @param high upper water mark (the capacity of the buffer of the sending of the stream)
				 * @param low  lower water mark (the threshold of the signal of the resumption writable)
				 *
				 * \~
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод регистрации отброшенного пакета
				 *
				 * @note Отбрасывание пакетов - штатная часть работы протокола: на порт
				 *       соединения приходит и посторонний трафик, и устаревшие пакеты,
				 *       и подделки. Поэтому диагностика ведётся только при сборке с
				 *       отладкой, иначе поток сообщений забил бы лог
				 *
				 * @param reason причина отбрасывания пакета
				 *
				 * \~english
				 * @brief Method of the registration of a discarded packet
				 *
				 * @note The discarding of the packets is a regular part of the work of the protocol: onto the port
				 *       of a connection both a foreign traffic and the outdated packets
				 *       and the forgeries arrive. Therefore the diagnostics is conducted only at an assembly with
				 *       a debugging, otherwise the stream of the messages would clog the log
				 *
				 * @param reason reason of the discarding of the packet
				 *
				 * \~
				 */
				void drop(const char * reason) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
				 *
				 * @note Идентификаторы соединения (initial_source_connection_id и
				 *       original_destination_connection_id) заполняются автоматически
				 *
				 * @param params локальные транспортные параметры
				 *
				 * \~english
				 * @brief Method of setting the local transport parameters (RFC 9000 §7.4)
				 *
				 * @note The identifiers of the connection (initial_source_connection_id and
				 *       original_destination_connection_id) are filled in automatically
				 *
				 * @param params local transport parameters
				 *
				 * \~
				 */
				void params(const quic::params::params_t & params) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения транспортных параметров удалённого узла (RFC 9000 §7.4)
				 *
				 * @param params транспортные параметры удалённого узла
				 * @param error  код ошибки транспорта
				 * @return       результат извлечения (OK/INCOMPLETE/ERROR)
				 *
				 * \~english
				 * @brief Method of the extraction of the transport parameters of the remote node (RFC 9000 §7.4)
				 *
				 * @param params transport parameters of the remote node
				 * @param error  error code of the transport
				 * @return       result of the extraction (OK/INCOMPLETE/ERROR)
				 *
				 * \~
				 */
				status_t peer(quic::params::params_t & params, error_t & error) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса удалённого эндпоинта (RFC 9000 §8.1.4)
				 *
				 * @note Адрес задаётся штатной структурой сетевого адреса фреймворка
				 *       net::addr_t (как её отдаёт DNS и все сетевые юниты) вместе с
				 *       портом эндпоинта; из них формируется опаковое сравнимое
				 *       представление пути. К нему привязывается токен проверки адреса
				 *       пакета Retry: без адреса токен остаётся пригодным к повтору с
				 *       чужого адреса
				 *
				 * @param addr структура сетевого адреса удалённого эндпоинта
				 * @param port порт удалённого эндпоинта
				 *
				 * \~english
				 * @brief Method of setting the address of the remote endpoint (RFC 9000 §8.1.4)
				 *
				 * @note The address is set by the regular structure of a network address of the framework
				 *       net::addr_t (as the DNS and all the network units issue it) together with
				 *       the port of the endpoint; out of them an opaque comparable
				 *       representation of the path is formed. To it the token of the checking of the address
				 *       of a Retry packet is attached: without the address the token remains suitable for a repetition from
				 *       a foreign address
				 *
				 * @param addr structure of the network address of the remote endpoint
				 * @param port port of the remote endpoint
				 *
				 * \~
				 */
				void address(const net::addr_t * addr, const uint16_t port) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса удалённого эндпоинта (RFC 9000 §8.1.4)
				 *
				 * @note Адрес и порт задаются единой штатной структурой атрибутов
				 *       подключения net::attr_t (как её отдают сетевые юниты); из
				 *       сетевого адреса и порта формируется опаковое сравнимое
				 *       представление пути
				 *
				 * @param attr структура атрибутов подключения удалённого эндпоинта
				 *
				 * \~english
				 * @brief Method of setting the address of the remote endpoint (RFC 9000 §8.1.4)
				 *
				 * @note The address and the port are set by a single regular structure of the attributes
				 *       of a connection net::attr_t (as the network units issue it); out of
				 *       the network address and the port an opaque comparable
				 *       representation of the path is formed
				 *
				 * @param attr structure of the attributes of the connection of the remote endpoint
				 *
				 * \~
				 */
				void address(const net::attr_t * attr) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
				 *
				 * @note Вызывается сервером до приёма первой датаграммы: первый пакет
				 *       Initial без токена получает в ответ пакет Retry с токеном,
				 *       соединение продолжается только с корректным токеном
				 *
				 * @param mode режим проверки адреса клиента
				 *
				 * \~english
				 * @brief Method of setting the checking of the address of a client through a Retry packet (RFC 9000 §8.1.2)
				 *
				 * @note It is called by a server before the acceptance of the first datagram: the first
				 *       Initial packet without a token gets in an answer a Retry packet with a token,
				 *       the connection continues only with a correct token
				 *
				 * @param mode mode of the checking of the address of the client
				 *
				 * \~
				 */
				void retry(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки токена проверки адреса для первого пакета (RFC 9000 §8.1.3)
				 *
				 * @note Вызывается клиентом до начала соединения. Токен выдан сервером
				 *       фреймом NEW_TOKEN на прошлом соединении и позволяет пропустить
				 *       обмен пакетом Retry, сэкономив круг задержки
				 *
				 * @param token токен проверки адреса
				 *
				 * \~english
				 * @brief Method of setting the token of the checking of the address for the first packet (RFC 9000 §8.1.3)
				 *
				 * @note It is called by a client before the beginning of the connection. The token is issued by the server
				 *       by a NEW_TOKEN frame on the past connection and allows to skip
				 *       the exchange by a Retry packet, saving a round of the delay
				 *
				 * @param token token of the checking of the address
				 *
				 * \~
				 */
				void token(string_view token) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
				 *
				 * @note Вызывается до начала соединения. Токены выдаваемых идентификаторов
				 *       выводятся из ключа, поэтому воспроизводятся и после утраты
				 *       состояния соединения - на этом и держится сброс без
				 *       сохранения состояния
				 *
				 * @param key общий ключ вывода токенов сброса
				 *
				 * \~english
				 * @brief Method of setting the common key of the derivation of the tokens of a reset (RFC 9000 §10.3.2)
				 *
				 * @note It is called before the beginning of the connection. The tokens of the issued identifiers
				 *       are derived from the key, therefore they are reproduced also after a loss of
				 *       the state of the connection - on this exactly a stateless reset
				 *       holds
				 *
				 * @param key common key of the derivation of the tokens of a reset
				 *
				 * \~
				 */
				void resetKey(string_view key) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения токена проверки адреса для будущих соединений (RFC 9000 §8.1.3)
				 *
				 * @note Токен присылается сервером фреймом NEW_TOKEN уже после
				 *       установления соединения, поэтому запрашивается по ходу работы
				 *
				 * @return токен проверки адреса (пусто - токен не присылался)
				 *
				 * \~english
				 * @brief Method of getting the token of the checking of the address for the future connections (RFC 9000 §8.1.3)
				 *
				 * @note The token is sent by the server by a NEW_TOKEN frame already after
				 *       the establishment of the connection, therefore it is requested in the course of the work
				 *
				 * @return token of the checking of the address (empty - the token has not been sent)
				 *
				 * \~
				 */
				const string & token() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки маркировки исходящих датаграмм поддержкой ECN (RFC 9000 §13.4)
				 *
				 * @note Маркировку накладывает вызывающий код на заголовок IP-пакета,
				 *       соединение лишь ведёт проверку пути и сообщает, какую маркировку
				 *       накладывать далее - см. marking()
				 *
				 * @param mode режим маркировки исходящих датаграмм
				 *
				 * \~english
				 * @brief Method of setting the marking of the outgoing datagrams by the support of ECN (RFC 9000 §13.4)
				 *
				 * @note The marking is imposed by the calling code onto the header of the IP packet,
				 *       the connection only conducts the checking of the path and reports which marking
				 *       to impose further - see marking()
				 *
				 * @param mode mode of the marking of the outgoing datagrams
				 *
				 * \~
				 */
				void ecn(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима следования за миграцией удалённого эндпоинта (RFC 9000 §9)
				 *
				 * @note Управляет реакцией на смену адреса удалённого эндпоинта: во
				 *       включённом режиме (по умолчанию) соединение следует за пиром на
				 *       новый адрес при приёме от него аутентифицированного непробирующего
				 *       пакета - так переживается смена сети и NAT-rebind любой стороны.
				 *       В выключенном режиме реакции нет, что соответствует строгой модели
				 *       §9, где миграцию отслеживает только сервер: клиенту его тогда
				 *       следует отключить, а на сервере оставить включённым
				 *
				 * @param mode режим следования за миграцией удалённого эндпоинта
				 *
				 * \~english
				 * @brief Method of setting the mode of the following of the migration of the remote endpoint (RFC 9000 §9)
				 *
				 * @note It controls the reaction to a change of the address of the remote endpoint: in the
				 *       enabled mode (by default) the connection follows the peer onto a
				 *       new address at the acceptance from it of an authenticated non-probing
				 *       packet - that way a change of the network and a NAT-rebind of any side is survived.
				 *       In the disabled mode there is no reaction, which corresponds to the strict model
				 *       of §9 where the migration is tracked only by a server: a client should then
				 *       disable it, while at a server it should be left enabled
				 *
				 * @param mode mode of the following of the migration of the remote endpoint
				 *
				 * \~
				 */
				void roaming(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения маркировки для исходящих датаграмм (RFC 9000 §13.4.2)
				 *
				 * @note Значение меняется по ходу соединения: непройденная проверка пути
				 *       снимает маркировку, а смена пути начинает проверку заново.
				 *       Запрашивается перед отправкой каждой датаграммы
				 *
				 * @return маркировка ECN для исходящих датаграмм
				 *
				 * \~english
				 * @brief Method of getting the marking for the outgoing datagrams (RFC 9000 §13.4.2)
				 *
				 * @note The value changes in the course of the connection: a not passed check of the path
				 *       removes the marking, while a change of the path begins the checking anew.
				 *       It is requested before the sending of every datagram
				 *
				 * @return marking ECN for the outgoing datagrams
				 *
				 * \~
				 */
				event::ecn_t marking() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод начала соединения клиентом
				 *
				 * @note Генерирует случайные идентификаторы соединения, выводит ключи
				 *       Initial и формирует ClientHello - первая датаграмма будет
				 *       доступна через write()
				 *
				 * @return результат начала соединения (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the beginning of a connection by a client
				 *
				 * @note It generates the random identifiers of the connection, derives the keys
				 *       Initial and forms a ClientHello - the first datagram will be
				 *       accessible through write()
				 *
				 * @return result of the beginning of the connection (OK/ERROR)
				 *
				 * \~
				 */
				status_t connect() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод обработки входящей UDP-датаграммы
				 *
				 * @note Датаграмма может содержать несколько коалесцированных пакетов.
				 *       Сервер инициализируется первым пакетом Initial клиента.
				 *       Пакеты, которые невозможно расшифровать, отбрасываются
				 *
				 * @param data буфер входящей UDP-датаграммы
				 * @param size размер входящей UDP-датаграммы
				 * @param now  текущее время в миллисекундах
				 * @return     результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the processing of an incoming UDP datagram
				 *
				 * @note A datagram may contain several coalesced packets.
				 *       A server is initialized by the first Initial packet of a client.
				 *       The packets which it is impossible to decrypt are discarded
				 *
				 * @param data buffer of the incoming UDP datagram
				 * @param size size of the incoming UDP datagram
				 * @param now  current time in milliseconds
				 * @return     result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				status_t read(const uint8_t * data, const size_t size, const uint64_t now) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки входящей датаграммы с маркировкой ECN (RFC 9000 §13.4)
				 *
				 * @note Маркировка берётся из заголовка IP-пакета и сообщается вызывающим
				 *       кодом: сетевого уровня соединение не имеет. Счётчики маркировок
				 *       возвращаются пиру эхом во фрейме ACK_ECN, по их приросту он судит
				 *       о перегрузке пути, не дожидаясь потерь
				 *
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 * @param now  текущее время в миллисекундах
				 * @param ecn  маркировка ECN заголовка IP-пакета
				 * @return     результат обработки (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the processing of an incoming datagram with a marking ECN (RFC 9000 §13.4)
				 *
				 * @note The marking is taken from the header of the IP packet and is reported by the calling
				 *       code: the connection has no network level. The counters of the markings
				 *       are returned to the peer by an echo in an ACK_ECN frame, by their increment it judges
				 *       about the congestion of the path without waiting for the losses
				 *
				 * @param data data of the datagram
				 * @param size size of the datagram
				 * @param now  current time in milliseconds
				 * @param ecn  marking ECN of the header of the IP packet
				 * @return     result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				status_t read(const uint8_t * data, const size_t size, const uint64_t now, const event::ecn_t ecn) noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки исходящей UDP-датаграммы
				 *
				 * @note Собирает пакеты доступных уровней шифрования с коалесценцией.
				 *       Датаграмма с пакетом Initial дополняется до 1200 октетов
				 *       (RFC 9000 §14.1). Вызывается циклически до пустого результата
				 *
				 * @param output буфер исходящей UDP-датаграммы (очищается)
				 * @param now    текущее время в миллисекундах
				 * @return       результат сборки (true - датаграмма готова к отправке)
				 *
				 * \~english
				 * @brief Method of the assembly of an outgoing UDP datagram
				 *
				 * @note It assembles the packets of the accessible levels of the encryption with a coalescence.
				 *       A datagram with an Initial packet is supplemented up to 1200 octets
				 *       (RFC 9000 §14.1). It is called cyclically until an empty result
				 *
				 * @param output buffer of the outgoing UDP datagram (is cleared)
				 * @param now    current time in milliseconds
				 * @return       result of the assembly (true - the datagram is ready for the sending)
				 *
				 * \~
				 */
				bool write(string & output, const uint64_t now) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения дедлайна ближайшего события таймера (RFC 9002 §6)
				 *
				 * @note Вызывающий код взводит таймер на полученное время и по его
				 *       истечении вызывает tick(), затем отправляет датаграммы write()
				 *
				 * @return дедлайн ближайшего события в миллисекундах (0 - таймер не требуется)
				 *
				 * \~english
				 * @brief Method of getting the deadline of the nearest event of the timer (RFC 9002 §6)
				 *
				 * @note The calling code raises a timer to the obtained time and at its
				 *       expiration calls tick(), then sends the datagrams write()
				 *
				 * @return deadline of the nearest event in milliseconds (0 - a timer is not required)
				 *
				 * \~
				 */
				uint64_t timeout() const noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки просроченных таймеров (RFC 9002 §6.2)
				 *
				 * @note По таймеру детекта потерь выполняется детект и ретрансмиссия,
				 *       по таймеру PTO - постановка зондирующих данных в очередь
				 *
				 * @param now текущее время в миллисекундах
				 *
				 * \~english
				 * @brief Method of the processing of the expired timers (RFC 9002 §6.2)
				 *
				 * @note By the timer of the detection of the losses the detection and the retransmission are performed,
				 *       by the timer PTO - the putting of the probing data into the queue
				 *
				 * @param now current time in milliseconds
				 *
				 * \~
				 */
				void tick(const uint64_t now) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Значение недопустимого идентификатора потока
				 *
				 * \~english
				 * @brief Value of an inadmissible identifier of a stream
				 *
				 * \~
				 */
				static constexpr uint64_t INVALID_STREAM = 0xFFFFFFFFFFFFFFFF;
				/**
				 * \~russian
				 * @brief Метод открытия нового потока приложения (RFC 9000 §2.1)
				 *
				 * @note Доступен после установления соединения; лимит удалённого
				 *       эндпоинта (MAX_STREAMS) проверяется автоматически
				 *
				 * @param unidirectional флаг однонаправленного потока
				 * @return               идентификатор потока (INVALID_STREAM - открытие невозможно)
				 *
				 * \~english
				 * @brief Method of the opening of a new stream of the application (RFC 9000 §2.1)
				 *
				 * @note It is accessible after the establishment of the connection; the limit of the remote
				 *       endpoint (MAX_STREAMS) is checked automatically
				 *
				 * @param unidirectional flag of a unidirectional stream
				 * @return               identifier of the stream (INVALID_STREAM - the opening is impossible)
				 *
				 * \~
				 */
				uint64_t open(const bool unidirectional) noexcept;
				/**
				 * \~russian
				 * @brief Метод постановки данных потока в очередь отправки (RFC 9000 §2.2)
				 *
				 * @note Данные копируются во внутренний буфер потока и упаковываются во фреймы
				 *       STREAM с учётом flow control при сборке датаграмм write(). Принимается
				 *       столько байт, сколько влезает до верхней водяной метки (частичный приём):
				 *       если возвращено меньше size, буфер заполнен - остаток дописывается позже,
				 *       по сигналу drained(). Флаг FIN применяется только при приёме данных целиком.
				 *       Возврат 0 означает либо заполненный буфер, либо непригодность потока к
				 *       отправке (закрыт, сброшен, соединение не готово) - причину отражает состояние
				 *
				 * @param sid  идентификатор потока
				 * @param data данные потока приложения
				 * @param fin  флаг завершения потока (FIN)
				 * @return     число принятых в очередь байт (0..size)
				 *
				 * \~english
				 * @brief Method of the putting of the data of a stream into the queue of the sending (RFC 9000 §2.2)
				 *
				 * @note The data is copied into the internal buffer of the stream and is packed into the STREAM
				 *       frames with the account of the flow control at the assembly of the datagrams write(). As many
				 *       octets are accepted as fit up to the upper water mark (a partial acceptance):
				 *       if less than size is returned, the buffer is filled - the remainder is appended later,
				 *       by the signal drained(). The flag FIN is applied only at an acceptance of the data entirely.
				 *       A return of 0 means either a filled buffer or an unsuitability of the stream for
				 *       the sending (it is closed, reset, the connection is not ready) - the reason is reflected by the state
				 *
				 * @param sid  identifier of the stream
				 * @param data data of the stream of the application
				 * @param fin  flag of the completion of the stream (FIN)
				 * @return     number of the octets accepted into the queue (0..size)
				 *
				 * \~
				 */
				size_t send(const uint64_t sid, string_view data, const bool fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод назначения pull-источника данных потока (RFC 9000 §2.2)
				 *
				 * @note Альтернатива send() для больших тел: движок сам запрашивает данные у
				 *       источника по мере места в буфере отправки потока ниже верхней водяной
				 *       метки и открытого окна, не требуя от приложения держать копию всего тела.
				 *       Источник заполняет буфер, выставляет eof в конце и возвращает число байт
				 *       (-1 аварийно завершает поток). Пустой источник снимает pull-режим
				 *
				 * @param sid    идентификатор потока
				 * @param source pull-источник данных тела потока
				 *
				 * \~english
				 * @brief Method of the assignment of the pull source of the data of a stream (RFC 9000 §2.2)
				 *
				 * @note An alternative to send() for the big bodies: the engine itself requests the data from a
				 *       source as there is a place in the buffer of the sending of the stream below the upper water
				 *       mark and the window is open, without requiring from the application to hold a copy of the whole body.
				 *       The source fills the buffer, sets eof at the end and returns the number of the octets
				 *       (-1 emergently completes the stream). An empty source removes the pull mode
				 *
				 * @param sid    identifier of the stream
				 * @param source pull source of the data of the body of the stream
				 *
				 * \~
				 */
				void dataSource(const uint64_t sid, data_source_callback_t source) noexcept;
				/**
				 * \~russian
				 * @brief Метод постановки датаграммы приложения в очередь отправки (RFC 9221 §4)
				 *
				 * @note Доставка датаграмм ненадёжна: потерянная датаграмма повторно
				 *       не отправляется, порядок доставки не гарантируется, а размер
				 *       ограничен анонсированным удалённым узлом пределом
				 *
				 * @param data данные датаграммы приложения
				 * @return     результат постановки (ERROR - датаграммы не поддерживаются либо размер превышен)
				 *
				 * \~english
				 * @brief Method of the putting of a datagram of the application into the queue of the sending (RFC 9221 §4)
				 *
				 * @note The delivery of the datagrams is unreliable: a lost datagram is not sent
				 *       repeatedly, the order of the delivery is not guaranteed, while the size
				 *       is limited by the limit announced by the remote node
				 *
				 * @param data data of the datagram of the application
				 * @return     result of the putting (ERROR - the datagrams are not supported or the size is exceeded)
				 *
				 * \~
				 */
				status_t datagram(string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения принятой датаграммы приложения (RFC 9221 §4)
				 *
				 * @param output буфер принятой датаграммы приложения
				 * @return       результат извлечения (false - принятых датаграмм нет)
				 *
				 * \~english
				 * @brief Method of the extraction of an accepted datagram of the application (RFC 9221 §4)
				 *
				 * @param output buffer of the accepted datagram of the application
				 * @return       result of the extraction (false - there are no accepted datagrams)
				 *
				 * \~
				 */
				bool datagram(string & output) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
				 *
				 * @note Предел анонсируется удалённым узлом и известен только после
				 *       завершения хендшейка. Нулевое значение означает, что датаграммы
				 *       удалённым узлом не принимаются
				 *
				 * @return предельный размер данных отправляемой датаграммы в октетах
				 *
				 * \~english
				 * @brief Method of getting the limiting size of a datagram being sent (RFC 9221 §3)
				 *
				 * @note The limit is announced by the remote node and is known only after
				 *       the completion of the handshake. A zero value means that the datagrams
				 *       are not accepted by the remote node
				 *
				 * @return limiting size of the data of a datagram being sent in octets
				 *
				 * \~
				 */
				size_t datagrams() const noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи потоков, буфер отправки которых освободился (сигнал writable)
				 *
				 * @note Возвращает идентификаторы потоков, чей буфер отправки после заполнения до
				 *       верхней водяной метки дренировался ниже нижней. Приложение, получившее в
				 *       send() частичный приём, по этому сигналу возобновляет выдачу данных потока.
				 *       Список вычитывается (очищается) при каждом вызове, как и readable()
				 *
				 * @param output список идентификаторов потоков, готовых принять данные
				 *
				 * \~english
				 * @brief Method of the issue of the streams the buffer of the sending of which has been freed (the signal writable)
				 *
				 * @note It returns the identifiers of the streams whose buffer of the sending after a filling up to
				 *       the upper water mark has drained below the lower one. An application which has obtained in
				 *       send() a partial acceptance, by this signal resumes the issue of the data of the stream.
				 *       The list is read out (cleared) at every call, as readable()
				 *
				 * @param output list of the identifiers of the streams ready to accept the data
				 *
				 * \~
				 */
				void drained(vector <uint64_t> & output) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения списка потоков с данными для приложения
				 *
				 * @note Список заполняется в буфер вызывающего кода: возврат нового
				 *       контейнера стоил бы выделения памяти на каждый вызов, а вызов
				 *       выполняется в цикле приёма. Выдача данных потока через receive()
				 *       во время обхода полученного списка безопасна - список является
				 *       копией и от состояния соединения не зависит
				 *
				 * @param output список идентификаторов потоков с собранными данными либо завершением
				 *
				 * \~english
				 * @brief Method of getting the list of the streams with the data for the application
				 *
				 * @note The list is filled into the buffer of the calling code: a return of a new
				 *       container would cost an allocation of the memory per every call, while the call
				 *       is performed in a cycle of the acceptance. The issue of the data of a stream through receive()
				 *       during the traversal of the obtained list is safe - the list is
				 *       a copy and does not depend on the state of the connection
				 *
				 * @param output list of the identifiers of the streams with the assembled data or with a completion
				 *
				 * \~
				 */
				void readable(vector <uint64_t> & output) noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи собранных данных потока приложению (RFC 9000 §2.2)
				 *
				 * @note Выдаются только непрерывно собранные данные; лимиты flow
				 *       control обновляются автоматически по мере потребления
				 *
				 * @param sid    идентификатор потока
				 * @param output собранные данные потока (дописываются)
				 * @param fin    флаг завершения потока удалённым эндпоинтом (FIN)
				 * @return       результат выдачи (OK/ERROR - поток неизвестен либо сброшен)
				 *
				 * \~english
				 * @brief Method of the issue of the assembled data of a stream to the application (RFC 9000 §2.2)
				 *
				 * @note Only the continuously assembled data is issued; the limits of the flow
				 *       control are updated automatically as it is consumed
				 *
				 * @param sid    identifier of the stream
				 * @param output assembled data of the stream (is appended)
				 * @param fin    flag of the completion of the stream by the remote endpoint (FIN)
				 * @return       result of the issue (OK/ERROR - the stream is unknown or reset)
				 *
				 * \~
				 */
				status_t receive(const uint64_t sid, string & output, bool & fin) noexcept;
				/**
				 * \~russian
				 * @brief Метод аварийного завершения отправки потока (RFC 9000 §2.4)
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки приложения
				 *
				 * \~english
				 * @brief Method of the emergency completion of the sending of a stream (RFC 9000 §2.4)
				 *
				 * @param sid  identifier of the stream
				 * @param code error code of the application
				 *
				 * \~
				 */
				void reset(const uint64_t sid, const uint64_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод запроса прекращения передачи удалённым эндпоинтом (RFC 9000 §3.5)
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки приложения
				 *
				 * \~english
				 * @brief Method of the request of a cessation of the transmission by the remote endpoint (RFC 9000 §3.5)
				 *
				 * @param sid  identifier of the stream
				 * @param code error code of the application
				 *
				 * \~
				 */
				void stop(const uint64_t sid, const uint64_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки аварийного завершения потока удалённым эндпоинтом
				 *
				 * @param sid  идентификатор потока
				 * @param code код ошибки приложения принятого фрейма RESET_STREAM
				 * @return     результат проверки (true - поток сброшен удалённым эндпоинтом)
				 *
				 * \~english
				 * @brief Method of checking an emergency completion of a stream by the remote endpoint
				 *
				 * @param sid  identifier of the stream
				 * @param code error code of the application of the accepted RESET_STREAM frame
				 * @return     result of the checking (true - the stream is reset by the remote endpoint)
				 *
				 * \~
				 */
				bool aborted(const uint64_t sid, uint64_t & code) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод инициирования обновления ключей уровня приложения (RFC 9001 §6)
				 *
				 * @note Доступен после подтверждения хендшейка; повторное обновление
				 *       возможно только после подтверждения пакета текущей фазы
				 *
				 * @param now текущее время в миллисекундах
				 * @return    результат инициирования (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of the initiation of an update of the keys of the level of the application (RFC 9001 §6)
				 *
				 * @note It is accessible after the confirmation of the handshake; a repeated update
				 *       is possible only after the acknowledgement of a packet of the current phase
				 *
				 * @param now current time in milliseconds
				 * @return    result of the initiation (OK/ERROR)
				 *
				 * \~
				 */
				status_t rekey(const uint64_t now) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения бита фазы ключей уровня приложения (RFC 9001 §6)
				 *
				 * @return бит фазы ключей
				 *
				 * \~english
				 * @brief Method of getting the bit of the phase of the keys of the level of the application (RFC 9001 §6)
				 *
				 * @return bit of the phase of the keys
				 *
				 * \~
				 */
				bool phase() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки лимитов AEAD текущего соединения (RFC 9001 §6.6)
				 *
				 * @note Значения ограничиваются сверху пределами спецификации: метод
				 *       способен только ужесточить лимиты, но не ослабить их. Служит для
				 *       проверки путей обновления ключей и завершения по достижении лимита,
				 *       недостижимых при штатных значениях (2²³ пакетов и 2³⁶ отказов)
				 *
				 * @param confidentiality предельное число пакетов на одном ключе
				 * @param integrity       предельное число неудачных снятий защиты
				 *
				 * \~english
				 * @brief Method of setting the limits AEAD of the current connection (RFC 9001 §6.6)
				 *
				 * @note The values are limited from above by the limits of the specification: the method
				 *       is capable only of toughening the limits but not of weakening them. It serves for
				 *       the checking of the paths of an update of the keys and of a completion at the reaching of a limit,
				 *       unreachable at the regular values (2²³ packets and 2³⁶ refusals)
				 *
				 * @param confidentiality limiting number of the packets on one key
				 * @param integrity       limiting number of the unsuccessful removals of the protection
				 *
				 * \~
				 */
				void aeadLimits(const uint64_t confidentiality, const uint64_t integrity) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сброса состояния пути соединения (RFC 9000 §9.4)
				 *
				 * @note Окно перегрузки, оценка задержки и размер пути характеризуют
				 *       конкретный путь и на новом пути неприменимы. Лимит анти-амплификации
				 *       снимается не всегда: он относится к адресу удалённого эндпоинта,
				 *       поэтому смена лишь собственного адреса его не возвращает
				 *
				 * @param remote признак смены адреса удалённого эндпоинта
				 *
				 * \~english
				 * @brief Method of the reset of the state of the path of the connection (RFC 9000 §9.4)
				 *
				 * @note The window of the congestion, the estimation of the delay and the size of the path characterize
				 *       a particular path and are inapplicable on a new path. The limit of the anti-amplification
				 *       is not always removed: it relates to the address of the remote endpoint,
				 *       therefore a change of one's own address alone does not return it
				 *
				 * @param remote flag of a change of the address of the remote endpoint
				 *
				 * \~
				 */
				void repath(const bool remote) noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения переезда на предпочтительный адрес (RFC 9000 §9.6.2)
				 *
				 * @note Вызывается по подтверждению достижимости предпочтительного адреса:
				 *       соединение переключается на его идентификатор, а состояние пути
				 *       сбрасывается как при любой смене пути
				 *
				 * \~english
				 * @brief Method of the completion of the moving onto the preferred address (RFC 9000 §9.6.2)
				 *
				 * @note It is called at the confirmation of the reachability of the preferred address:
				 *       the connection switches onto its identifier, while the state of the path
				 *       is reset as at any change of the path
				 *
				 * \~
				 */
				void settle() noexcept;
				/**
				 * \~russian
				 * @brief Метод отказа от проверки достижимости пути (RFC 9000 §8.2.4)
				 *
				 * @note Путь, не ответивший на проверку за отведённое время, признаётся
				 *       непригодным: переотправка проверки прекращается. Соединение при
				 *       этом не завершается - непригодность пути его разрывом не является
				 *
				 * @param revert признак возврата на последний проверенный адрес. Возврат
				 *               уместен лишь когда идти больше некуда: при отказе ради
				 *               перехода на очередной новый адрес он затёр бы этот адрес
				 *
				 * \~english
				 * @brief Method of the refusal of the checking of the reachability of a path (RFC 9000 §8.2.4)
				 *
				 * @note A path which has not answered the checking in the allotted time is recognized
				 *       unsuitable: the resending of the checking ceases. The connection thereby
				 *       is not completed - an unsuitability of a path is not a break of it
				 *
				 * @param revert flag of a return onto the last checked address. A return
				 *               is appropriate only when there is nowhere else to go: at a refusal for the sake of
				 *               a transition onto the next new address it would erase this address
				 *
				 * \~
				 */
				void abandon(const bool revert) noexcept;
				/**
				 * \~russian
				 * @brief Метод инициирования проверки достижимости пути (RFC 9000 §8.2)
				 *
				 * @note Отправляет удалённому эндпоинту фрейм PATH_CHALLENGE со
				 *       случайными данными. Путь считается подтверждённым при приёме
				 *       фрейма PATH_RESPONSE с теми же данными; ответ с иными данными
				 *       завершает соединение нарушением протокола. Повторный вызов
				 *       до получения ответа отвергается
				 *
				 * @return результат инициирования (false - проверка уже выполняется либо соединение не установлено)
				 *
				 * \~english
				 * @brief Method of the initiation of a checking of the reachability of the path (RFC 9000 §8.2)
				 *
				 * @note It sends to the remote endpoint a PATH_CHALLENGE frame with the
				 *       random data. The path is considered confirmed at the acceptance of a
				 *       PATH_RESPONSE frame with the same data; an answer with other data
				 *       completes the connection by a violation of the protocol. A repeated call
				 *       before the receipt of the answer is rejected
				 *
				 * @return result of the initiation (false - the checking is already being performed or the connection is not established)
				 *
				 * \~
				 */
				bool probe() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния проверки достижимости пути (RFC 9000 §8.2)
				 *
				 * @return состояние проверки (true - путь подтверждён ответом удалённого эндпоинта)
				 *
				 * \~english
				 * @brief Method of getting the state of the checking of the reachability of the path (RFC 9000 §8.2)
				 *
				 * @return state of the checking (true - the path is confirmed by an answer of the remote endpoint)
				 *
				 * \~
				 */
				bool validated() const noexcept;
				/**
				 * \~russian
				 * @brief Метод инициирования миграции соединения на новый путь (RFC 9000 §9)
				 *
				 * @note Вызывается после смены локального сетевого адреса. Соединение
				 *       переключается на неиспользованный идентификатор удалённого
				 *       эндпоинта, чтобы новый путь не связывался с прежним по
				 *       идентификатору (RFC 9000 §9.5), сбрасывает состояние пути
				 *       и начинает проверку его достижимости
				 *
				 * @return результат инициирования (false - соединение не установлено либо нет неиспользованных идентификаторов)
				 *
				 * \~english
				 * @brief Method of the initiation of a migration of the connection onto a new path (RFC 9000 §9)
				 *
				 * @note It is called after a change of the local network address. The connection
				 *       switches onto an unused identifier of the remote
				 *       endpoint, so that the new path would not be linked with the previous one by an
				 *       identifier (RFC 9000 §9.5), resets the state of the path
				 *       and begins the checking of its reachability
				 *
				 * @return result of the initiation (false - the connection is not established or there are no unused identifiers)
				 *
				 * \~
				 */
				bool migrate() noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки возможности переезда на предпочтительный адрес сервера (RFC 9000 §9.6)
				 *
				 * @note Переезд возможен на клиенте по установленному соединению, когда
				 *       сервер анонсировал предпочтительный адрес и переезд ещё
				 *       не выполнялся
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the possibility of a moving onto the preferred address of the server (RFC 9000 §9.6)
				 *
				 * @note A moving is possible at a client on an established connection, when
				 *       the server has announced a preferred address and the moving has not yet
				 *       been performed
				 *
				 * @return result of the checking
				 *
				 * \~
				 */
				bool relocatable() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения предпочтительного адреса сервера (RFC 9000 §9.6)
				 *
				 * @note Адрес отдаётся в сетевом порядке октетов: слой соединения работает
				 *       без ввода-вывода, а переподключение сокета на предпочтительный
				 *       адрес выполняет вызывающий код
				 *
				 * @param ipv6 флаг извлечения адреса семейства IPv6
				 * @param ip   адрес сервера в сетевом порядке октетов (4 октета IPv4 либо 16 октетов IPv6)
				 * @param port порт сервера
				 * @return     результат извлечения (false - адрес семейства не анонсирован)
				 *
				 * \~english
				 * @brief Method of the extraction of the preferred address of the server (RFC 9000 §9.6)
				 *
				 * @note The address is issued in the network order of the octets: the layer of the connection works
				 *       without an input-output, while the reconnection of the socket onto the preferred
				 *       address is performed by the calling code
				 *
				 * @param ipv6 flag of the extraction of an address of the family IPv6
				 * @param ip   address of the server in the network order of the octets (4 octets of IPv4 or 16 octets of IPv6)
				 * @param port port of the server
				 * @return     result of the extraction (false - an address of the family is not announced)
				 *
				 * \~
				 */
				bool preferred(const bool ipv6, string & ip, uint16_t & port) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса удалённого эндпоинта текущего пути (RFC 9000 §9.3)
				 *
				 * @note Соединение выбирает путь само: смена адреса удалённого эндпоинта
				 *       принимается по приходу датаграммы, а непройденная проверка нового
				 *       адреса возвращает соединение на последний проверенный. Вызывающий
				 *       код отправляет датаграммы именно по этому адресу
				 *
				 * @return адрес удалённого эндпоинта в заданном вызывающим кодом представлении
				 *
				 * \~english
				 * @brief Method of getting the address of the remote endpoint of the current path (RFC 9000 §9.3)
				 *
				 * @note The connection chooses the path itself: a change of the address of the remote endpoint
				 *       is accepted at the arrival of a datagram, while a not passed check of a new
				 *       address returns the connection onto the last checked one. The calling
				 *       code sends the datagrams exactly to this address
				 *
				 * @return address of the remote endpoint in the representation set by the calling code
				 *
				 * \~
				 */
				const string & path() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адресата собранной датаграммы (RFC 9000 §9.6.2)
				 *
				 * @note Запрашивается сразу после сборки датаграммы. Пока достижимость
				 *       предпочтительного адреса не подтверждена, соединение отправляет
				 *       по двум адресам: обычные датаграммы уходят по текущему пути, а
				 *       пробирующие - на предпочтительный адрес, полученный из preferred()
				 *
				 * @return результат проверки (true - датаграмма адресована предпочтительному адресу)
				 *
				 * \~english
				 * @brief Method of getting the addressee of an assembled datagram (RFC 9000 §9.6.2)
				 *
				 * @note It is requested right after the assembly of a datagram. While the reachability
				 *       of the preferred address is not confirmed, the connection sends
				 *       to two addresses: the ordinary datagrams go away by the current path, while
				 *       the probing ones - to the preferred address obtained from preferred()
				 *
				 * @return result of the checking (true - the datagram is addressed to the preferred address)
				 *
				 * \~
				 */
				bool alternate() const noexcept;
				/**
				 * \~russian
				 * @brief Метод переезда соединения на предпочтительный адрес сервера (RFC 9000 §9.6)
				 *
				 * @note Вызывается после переподключения сокета и установки нового адреса
				 *       методом address(). Соединение переключается на идентификатор
				 *       предпочтительного адреса, сбрасывает состояние пути и начинает
				 *       проверку его достижимости
				 *
				 * @return результат инициирования переезда (false - переезд невозможен)
				 *
				 * \~english
				 * @brief Method of the moving of the connection onto the preferred address of the server (RFC 9000 §9.6)
				 *
				 * @note It is called after the reconnection of the socket and the setting of the new address
				 *       by the method address(). The connection switches onto the identifier
				 *       of the preferred address, resets the state of the path and begins
				 *       the checking of its reachability
				 *
				 * @return result of the initiation of the moving (false - the moving is impossible)
				 *
				 * \~
				 */
				bool relocate() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения количества обслуживаемых потоков приложения
				 *
				 * @note Поток удерживается в списке, пока обе его стороны не завершены
				 *       и на него не ссылаются очереди отправки, после чего освобождается
				 *
				 * @return количество обслуживаемых потоков приложения
				 *
				 * \~english
				 * @brief Method of getting the number of the served streams of the application
				 *
				 * @note A stream is held in the list while both its sides are not completed
				 *       and the queues of the sending do not refer to it, after which it is released
				 *
				 * @return number of the served streams of the application
				 *
				 * \~
				 */
				size_t streams() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки санитарной границы анонсируемого начального лимита потоков (RFC 9000 §4.6)
				 *
				 * @note Граница ограничивает окно конкурентности потоков удалённого
				 *       эндпоинта и защищает от чрезмерной эгерной материализации неявно
				 *       открытых потоков. Ограничивает лишь стартовый лимит, но не
				 *       суммарное число потоков за жизнь соединения (оно восполняется
				 *       фреймами MAX_STREAMS). Применяется до установки транспортных
				 *       параметров методом params()
				 *
				 * @param limit верхняя граница анонсируемого начального лимита потоков одного направления
				 *
				 * \~english
				 * @brief Method of setting the sanitary boundary of the announced initial limit of the streams (RFC 9000 §4.6)
				 *
				 * @note The boundary limits the window of the concurrency of the streams of the remote
				 *       endpoint and protects from an excessive eager materialization of the implicitly
				 *       opened streams. It limits only the starting limit but not
				 *       the total number of the streams over the life of the connection (it is replenished
				 *       by the MAX_STREAMS frames). It is applied before the setting of the transport
				 *       parameters by the method params()
				 *
				 * @param limit upper boundary of the announced initial limit of the streams of one direction
				 *
				 * \~
				 */
				void streams(const uint64_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения количества выполненных смен пути соединения
				 *
				 * @return количество выполненных смен пути
				 *
				 * \~english
				 * @brief Method of getting the number of the performed changes of the path of the connection
				 *
				 * @return number of the performed changes of the path
				 *
				 * \~
				 */
				uint64_t migrations() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения подтверждённого размера исходящей датаграммы (RFC 8899)
				 *
				 * @return подтверждённый размер исходящей датаграммы в октетах
				 *
				 * \~english
				 * @brief Method of getting the confirmed size of an outgoing datagram (RFC 8899)
				 *
				 * @return confirmed size of an outgoing datagram in octets
				 *
				 * \~
				 */
				size_t pmtu() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки верхней границы поиска размера пути (RFC 8899 §5.1)
				 *
				 * @note Ограничивает поиск сверху локальным пределом: интерфейс отправки
				 *       может не пропускать датаграммы вплоть до анонсированного
				 *       удалённым узлом предела
				 *
				 * @param limit верхняя граница размера исходящей датаграммы в октетах
				 *
				 * \~english
				 * @brief Method of setting the upper boundary of the search of the size of the path (RFC 8899 §5.1)
				 *
				 * @note It limits the search from above by a local limit: the interface of the sending
				 *       may not let the datagrams through up to the limit announced
				 *       by the remote node
				 *
				 * @param limit upper boundary of the size of an outgoing datagram in octets
				 *
				 * \~
				 */
				void pmtu(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод ротации идентификатора соединения удалённого эндпоинта (RFC 9000 §5.1.1)
				 *
				 * @note Переключает DCID на неиспользованный идентификатор из анонсированных
				 *       фреймами NEW_CONNECTION_ID, прежний выводится из обращения
				 *
				 * @return результат ротации (false - неиспользованных идентификаторов нет)
				 *
				 * \~english
				 * @brief Method of the rotation of the identifier of the connection of the remote endpoint (RFC 9000 §5.1.1)
				 *
				 * @note It switches the DCID onto an unused identifier out of those announced
				 *       by the NEW_CONNECTION_ID frames, the previous one is withdrawn from the circulation
				 *
				 * @return result of the rotation (false - there are no unused identifiers)
				 *
				 * \~
				 */
				bool rotate() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения окна перегрузки congestion control (RFC 9002 §7)
				 *
				 * @return окно перегрузки в октетах
				 *
				 * \~english
				 * @brief Method of getting the window of the congestion of the congestion control (RFC 9002 §7)
				 *
				 * @return window of the congestion in octets
				 *
				 * \~
				 */
				uint64_t cwnd() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения количества неподтверждённых октетов в полёте
				 *
				 * @return количество неподтверждённых октетов в полёте
				 *
				 * \~english
				 * @brief Method of getting the number of the unacknowledged octets in a flight
				 *
				 * @return number of the unacknowledged octets in a flight
				 *
				 * \~
				 */
				uint64_t inflight() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
				 *
				 * @param code   код ошибки приложения
				 * @param reason человекочитаемая причина завершения
				 *
				 * \~english
				 * @brief Method of the completion of the connection by the application (RFC 9000 §10.2)
				 *
				 * @param code   error code of the application
				 * @param reason human-readable reason of the completion
				 *
				 * \~
				 */
				void close(const uint64_t code, string_view reason) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения состояния соединения
				 *
				 * @return состояние соединения
				 *
				 * \~english
				 * @brief Method of getting the state of the connection
				 *
				 * @return state of the connection
				 *
				 * \~
				 */
				state_t state() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения кода ошибки транспорта соединения
				 *
				 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
				 *
				 * \~english
				 * @brief Method of getting the error code of the transport of the connection
				 *
				 * @return error code of the transport (NO_ERROR - there is no error)
				 *
				 * \~
				 */
				error_t error() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения идентификатора соединения локального эндпоинта
				 *
				 * @return идентификатор соединения локального эндпоинта
				 *
				 * \~english
				 * @brief Method of getting the identifier of the connection of the local endpoint
				 *
				 * @return identifier of the connection of the local endpoint
				 *
				 * \~
				 */
				const cid_t & scid() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения изменений набора идентификаторов локального эндпоинта
				 *
				 * @note Соединение адресуется набором идентификаторов, который меняется
				 *       по ходу работы: к выданному при установлении добавляются
				 *       анонсированные фреймами NEW_CONNECTION_ID, а выведенные из
				 *       обращения удаляются. Накопленные изменения выдаются однократно
				 *       и сбрасываются - вызывающий код синхронизирует по ним
				 *       маршрутизацию входящих датаграмм
				 *
				 * @param added   идентификаторы, введённые в обращение
				 * @param removed идентификаторы, выведенные из обращения
				 *
				 * \~english
				 * @brief Method of the extraction of the changes of the collection of the identifiers of the local endpoint
				 *
				 * @note A connection is addressed by a collection of the identifiers which changes
				 *       in the course of the work: to that issued at the establishment the ones
				 *       announced by the NEW_CONNECTION_ID frames are added, while the withdrawn from the
				 *       circulation are removed. The accumulated changes are issued once
				 *       and are reset - the calling code synchronizes by them
				 *       the routing of the incoming datagrams
				 *
				 * @param added   identifiers brought into the circulation
				 * @param removed identifiers withdrawn from the circulation
				 *
				 * \~
				 */
				void issued(vector <cid_t> & added, vector <cid_t> & removed) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения идентификатора соединения удалённого эндпоинта
				 *
				 * @return идентификатор соединения удалённого эндпоинта
				 *
				 * \~english
				 * @brief Method of getting the identifier of the connection of the remote endpoint
				 *
				 * @return identifier of the connection of the remote endpoint
				 *
				 * \~
				 */
				const cid_t & dcid() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод доступа к машине криптографического хендшейка
				 *
				 * @return машина криптографического хендшейка
				 *
				 * \~english
				 * @brief Method of the access to the machine of the cryptographic handshake
				 *
				 * @return machine of the cryptographic handshake
				 *
				 * \~
				 */
				const handshake_t & handshake() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Запрещаем копирование и перемещение (хендшейк-машина владеет
				 *        TLS-соединением с обратным указателем)
				 *
				 * \~english
				 * @brief Forbid the copying and the moving (the handshake machine owns
				 *        a TLS connection with a back pointer)
				 *
				 * \~
				 */
				Connection(const Connection &) = delete;
				Connection(Connection &&) = delete;
				Connection & operator = (const Connection &) = delete;
				Connection & operator = (Connection &&) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @note Криптография соединения целиком задаётся шаблоном контекста
				 *       кодера транспортной безопасности: сертификаты, доверенные
				 *       центры, проверка узла, доменное имя и список ALPN-протоколов
				 *       настраиваются там и здесь не дублируются. Владение контекстом
				 *       остаётся за модулем транспортной безопасности
				 *
				 * @param endpoint роль локального эндпоинта на соединении
				 * @param ctx      идентификатор шаблона контекста безопасности
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @note The cryptography of a connection is entirely set by the template of the context
				 *       of the coder of the transport safety: the certificates, the trusted
				 *       centres, the checking of the node, the domain name and the list of the ALPN protocols
				 *       are configured there and are not duplicated here. The ownership of the context
				 *       remains at the transport security module
				 *
				 * @param endpoint role of the local endpoint on the connection
				 * @param ctx      identifier of the template of the context of the safety
				 *
				 * \~
				 */
				explicit Connection(const endpoint_t endpoint, const tls::coder::id_t ctx) noexcept;
		} connection_t;
	};
};
