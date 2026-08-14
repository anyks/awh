/**
 * @file quic.hpp
 * @date 2026-07-22
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
 * @brief Заголовочный файл модулей QUIC — классы unit::QuicServer и unit::QuicClient,
 *        связывающие конечный автомат соединения QUIC с движком ввода-вывода: управление сессиями,
 *        миграция клиентского адреса, кластеризация сервера и возобновление сессии по 0-RTT
 *
 * \~english
 * @brief Header file of the QUIC modules — the unit::QuicServer and unit::QuicClient classes,
 *        which link the state machine of a QUIC connection with the input-output engine: the management of the sessions,
 *        the migration of the client address, the clustering of the server and the resumption of a session by 0-RTT
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_QUIC__
#define __AWH_UNIT_QUIC__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "unit.hpp"
#include "cluster.hpp"
#include "../cryptography/tls/coder.hpp"
#include "../proto/quic/connection.hpp"

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
	 * \~russian
	 * @brief Пространство имён модулей
	 *
	 *
	 * \~english
	 * @brief Modules namespace
	 *
	 * \~
	 */
	namespace unit {
		/**
		 * Подписываемся на стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс модуля сервера транспортного протокола QUIC
		 *
		 * @details Связывает соединения QUIC с асинхронным сетевым движком.
		 *          Соединение адресуется набором идентификаторов, который меняется
		 *          по ходу работы: к выданному при установлении добавляются
		 *          анонсированные фреймами NEW_CONNECTION_ID, а выведенные из
		 *          обращения удаляются. Модуль синхронизирует этот набор с
		 *          маршрутизацией движка после каждой обработанной датаграммы,
		 *          поэтому смена адреса клиента соединение не разрывает
		 *          (RFC 9000 §9), а забыть синхронизацию невозможно.
		 *
		 * \~english
		 * @brief Class of the server module of the QUIC transport protocol
		 * @details Links the QUIC connections with the asynchronous network engine.
		 *          A connection is addressed by a set of identifiers which changes
		 *          in the course of the work: to the one issued at the establishment there are added the ones
		 *          announced by the NEW_CONNECTION_ID frames, while the ones withdrawn from
		 *          the circulation are removed. The module synchronizes this set with
		 *          the routing of the engine after every processed datagram,
		 *          therefore a change of the address of the client does not break the connection
		 *          (RFC 9000 §9), while it is impossible to forget the synchronization.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ QuicServer : public unit_t {
			private:
				/**
				 * \~russian
				 * @brief Структура сессии соединения
				 *
				 * \~english
				 * @brief Structure of the session of a connection
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Session {
					// Объект соединения QUIC
					unique_ptr <quic::connection_t> connection;
					// Флаг оповещения приложения об установленном соединении
					bool connected;
					/**
					 * Идентификатор события-приёмника объединения данных (splice): собранные
					 * данные потоков этой сессии перенаправляются в это событие вместо
					 * выдачи приложению (0 - объединение не установлено)
					 */
					event::id_t dest;
					/**
					 * Идентификатор туннельного потока для входящих объединённых данных:
					 * байты, поступающие в эту сессию из события-источника, отправляются
					 * этим потоком приложения (INVALID_STREAM - поток ещё не открыт)
					 */
					uint64_t tunnel;
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
					explicit Session() noexcept;
				} session_t;
				/**
				 * \~russian
				 * @brief Структура параметров кластера
				 *
				 * @details Параметры кластера задаются при его создании и не могут быть изменены в процессе работы.
				 *
				 * \~english
				 * @brief Structure of the parameters of the cluster
				 * @details The parameters of the cluster are given at its creation and cannot be changed in the course of the work.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ ClusterParams {
					// Имя кластера
					string name;
					// Флаг пересоздания процесса при его завершении
					bool rebirth;
					// Максимальное количество процессов в кластере
					uint16_t count;
					// Максимальное число подряд идущих быстрых падений процессов до остановки кластера (0 — без ограничения, по умолчанию 10)
					uint16_t restartLimit;
					// Временное окно «быстрого» (раннего) падения процесса в миллисекундах (по умолчанию 30000)
					uint64_t restartWindow;
					// Режим активации кластера (по умолчанию event::mode_t::DISABLED)
					event::mode_t mode;
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
					explicit ClusterParams() noexcept;
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
					~ClusterParams() noexcept = default;
				} cluster_params_t;
			private:
				// Идентификатор события сервера
				event::id_t _eid;
				// Идентификатор события интервала таймеров соединений
				event::id_t _tid;
				/**
				 * Унаследованный до fork идентификатор события сервера. В режиме кластера
				 * дочерний процесс пересоздаёт собственное событие сервера с новым
				 * идентификатором, а фасад продолжает обращаться к унаследованному: значение
				 * сохраняется для приведения унаследованного идентификатора к собственному
				 * (см. actual())
				 */
				event::id_t _inheritedEid;
			private:
				// Флаг проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
				bool _retry;
				// Флаг уведомления о перегрузке пути (RFC 9000 §13.4)
				bool _ecn;
				// Семейство адресов события сервера
				event::family_t _family;
				/**
				 * Маркировка, установленная на сокете события сервера. Маркировка
				 * накладывается на сокет целиком, а проверку пути соединения ведут
				 * порознь: значение кешируется, чтобы менять его только при
				 * расхождении с требуемым, а не перед каждой датаграммой
				 */
				event::ecn_t _marking;
			private:
				/**
				 * Общий ключ вывода токенов сброса без сохранения состояния. Генерируется
				 * при запуске сервера, если не задан приложением: сохранённый между
				 * запусками ключ позволяет сбрасывать соединения, о которых сервер
				 * после перезапуска уже ничего не помнит (RFC 9000 §10.3.2)
				 */
				string _resetKey;
			private:
				// Идентификатор шаблона контекста безопасности
				tls::coder_t::id_t _ctx;
				// Объект кодера транспортной безопасности
				const tls::coder_t * _coder;
			private:
				// Локальные транспортные параметры соединений
				quic::params::params_t _params;
			private:
				// Список сессий соединений по идентификаторам событий
				map <event::id_t, session_t> _sessions;
			private:
				// Объект работы с кластером
				unique_ptr <cluster_t> _cluster;
			private:
				// Параметры кластера
				cluster_params_t _clusterParams;
			private:
				/**
				 * Идентификаторы дочерних процессов, не получивших порт прослушивания при
				 * раздаче родительским процессом (работают в холостую без сокета сервера).
				 * Заполняется только на системах, где порт выделяется дочернему процессу
				 * монопольно (macOS/Solaris/OpenBSD/NetBSD), и позволяет доотправить порт
				 * такому процессу вручную (clusterIdle()/clusterAssign())
				 */
				unordered_set <pid_t> _idle;
				/**
				 * Порт прослушивания, выделенный каждому дочернему процессу при раздаче
				 * родительским процессом. Позволяет автоматически вернуть возрождённому
				 * (после падения) дочернему процессу тот же порт, что был у него до гибели
				 */
				unordered_map <pid_t, uint16_t> _ports;
			private:
				/**
				 * Параметры прослушивания, кэшируемые для пересоздания сокета в дочернем
				 * процессе кластера. Фасад настраивает событие сервера в родительском
				 * процессе, а каждый дочерний процесс поднимает собственный сокет после
				 * fork, поэтому адрес, порт и размер очереди сохраняются заранее
				 */
				// Тип адреса прослушивания (IPv4 или IPv6)
				event::address_t _listenType;
				// Флаг ожидания дочерним процессом выделенного родительским процессом порта
				bool _awaitingPort;
				// Максимальный размер очереди ожидания соединений
				uint32_t _backlog;
				// Порт прослушивания сервера
				uint16_t _listenPort;
				// Начальный порт диапазона выделения портов дочерним процессам (0 - использовать порт прослушивания)
				uint16_t _portBegin;
				// Конечный порт диапазона выделения портов дочерним процессам (0 - использовать порт прослушивания)
				uint16_t _portEnd;
				// Хост прослушивания сервера
				string _listenHost;
			private:
				/**
				 * \~russian
				 * @brief Метод получения текущего времени в миллисекундах
				 *
				 * @return текущее время в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the current time in milliseconds
				 * @return current time in milliseconds
				 *
				 * \~
				 */
				uint64_t date() const noexcept;
				/**
				 * \~russian
				 * @brief Метод формирования адреса удалённого эндпоинта сессии
				 *
				 * @param oid идентификатор события сессии
				 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
				 *
				 * \~english
				 * @brief Method of forming the address of the remote endpoint of a session
				 * @param oid event identifier of the session
				 * @return    address of the remote endpoint in the form "address:port"
				 *
				 * \~
				 */
				string peer(const event::id_t oid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса удалённого эндпоинта соединению из движка
				 *
				 * @note Адрес источника извлекается штатной структурой сетевого адреса
				 *       фреймворка net::addr_t, порт - штатным методом движка; смена
				 *       адреса при установленном соединении означает миграцию пути
				 *       (RFC 9000 §9)
				 *
				 * @param oid        идентификатор события сессии
				 * @param connection соединение, которому устанавливается адрес эндпоинта
				 *
				 * \~english
				 * @brief Method of setting the address of the remote endpoint to a connection from the engine
				 * @note The address of the source is extracted by the regular structure of the network address
				 *       of the framework net::addr_t, the port — by the regular method of the engine; a change of the
				 *       address on an established connection means a migration of the path
				 *       (RFC 9000 §9)
				 * @param oid        event identifier of the session
				 * @param connection connection to which the address of the endpoint is set
				 *
				 * \~
				 */
				void endpoint(const event::id_t oid, quic::connection_t * connection) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод определения сессии принятой датаграммы (RFC 9000 §17.2)
				 *
				 * @param eid  идентификатор события сервера
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 * @param key  выводимый ключ сессии
				 * @return     результат определения сессии
				 *
				 * \~english
				 * @brief Method of determining the session of an accepted datagram (RFC 9000 §17.2)
				 * @param eid  event identifier of the server
				 * @param data data of the datagram
				 * @param size size of the datagram
				 * @param key  session key being derived
				 * @return     result of determining the session
				 *
				 * \~
				 */
				bool origin(const event::id_t eid, const uint8_t * data, const size_t size, net::origin_key_t & key) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания сессии нового соединения
				 *
				 * @param eid идентификатор события сервера
				 * @param oid идентификатор события сессии
				 *
				 * \~english
				 * @brief Method of creating a session of a new connection
				 * @param eid event identifier of the server
				 * @param oid event identifier of the session
				 *
				 * \~
				 */
				void accept(const event::id_t eid, const event::id_t oid) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки принятой датаграммы сессии
				 *
				 * @param oid  идентификатор события сессии
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 *
				 * \~english
				 * @brief Method of processing an accepted datagram of a session
				 * @param oid  event identifier of the session
				 * @param data data of the datagram
				 * @param size size of the datagram
				 *
				 * \~
				 */
				void read(const event::id_t oid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки просроченных таймеров соединений
				 *
				 * @param eid    идентификатор события интервала
				 * @param status статус события интервала
				 *
				 * \~english
				 * @brief Method of processing the expired timers of the connections
				 * @param eid    event identifier of the interval
				 * @param status status of the interval event
				 *
				 * \~
				 */
				void tick(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод синхронизации маршрутизации соединения
				 *
				 * @note Идентификаторы, введённые соединением в обращение, привязываются
				 *       к сессии движка, а выведенные - снимаются. Без синхронизации
				 *       датаграмма с новым идентификатором была бы принята за новое
				 *       соединение
				 *
				 * @param oid     идентификатор события сессии
				 * @param session сессия соединения
				 *
				 * \~english
				 * @brief Method of synchronizing the routing of a connection
				 * @note The identifiers introduced into the circulation by the connection are bound
				 *       to the session of the engine, while the withdrawn ones are removed. Without the synchronization
				 *       a datagram with a new identifier would be taken for a new
				 *       connection
				 * @param oid     event identifier of the session
				 * @param session session of the connection
				 *
				 * \~
				 */
				void reroute(const event::id_t oid, session_t & session) noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи собранных данных потоков приложения
				 *
				 * @param oid идентификатор события сессии
				 *
				 * \~english
				 * @brief Method of issuing the assembled data of the application streams
				 * @param oid event identifier of the session
				 *
				 * \~
				 */
				void process(const event::id_t oid) noexcept;
				/**
				 * \~russian
				 * @brief Метод применения маркировки соединения к сокету события сервера
				 *
				 * @param marking требуемая маркировка исходящих датаграмм
				 *
				 * \~english
				 * @brief Method of applying the marking of a connection to the socket of the server event
				 * @param marking required marking of the outgoing datagrams
				 *
				 * \~
				 */
				void mark(const event::ecn_t marking) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки готовых исходящих датаграмм соединения
				 *
				 * @param oid     идентификатор события сессии
				 * @param session сессия соединения
				 *
				 * \~english
				 * @brief Method of sending the ready outgoing datagrams of a connection
				 * @param oid     event identifier of the session
				 * @param session session of the connection
				 *
				 * \~
				 */
				bool flush(const event::id_t oid, session_t & session) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки объединённых данных в туннельный поток сессии
				 *
				 * @note Байты, поступившие из события-источника объединения (splice) - будь
				 *       то обычный сокет через сетевой движок либо другая QUIC-сессия -
				 *       отправляются туннельным потоком сессии-приёмника с их шифрованием
				 *       на уровне соединения (RFC 9000 §2.1)
				 *
				 * @param oid  идентификатор события сессии-приёмника
				 * @param data данные для отправки в туннельный поток
				 * @param size размер данных для отправки
				 * @return     результат постановки данных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending the joined data into the tunnel stream of a session
				 * @note The bytes that have arrived from the source event of the joining (splice) — be
				 *       it an ordinary socket through the network engine or another QUIC session —
				 *       are sent by the tunnel stream of the destination session with their encryption
				 *       at the level of the connection (RFC 9000 §2.1)
				 * @param oid  event identifier of the destination session
				 * @param data data to be sent into the tunnel stream
				 * @param size size of the data to be sent
				 * @return     result of placing the data into the sending queue
				 *
				 * \~
				 */
				bool inject(const event::id_t oid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод перенаправления собранных данных сессии в событие-приёмник объединения
				 *
				 * @note Если приёмник - другая QUIC-сессия, данные перешифровываются в её
				 *       туннельный поток; если обычное событие движка (TCP/UDP) - отправляются
				 *       сырыми байтами
				 *
				 * @param dest идентификатор события-приёмника объединения данных
				 * @param data данные для перенаправления
				 *
				 * \~english
				 * @brief Method of redirecting the assembled data of a session into the destination event of the joining
				 * @note If the destination is another QUIC session, the data is re-encrypted into its
				 *       tunnel stream; if it is an ordinary event of the engine (TCP/UDP) — it is sent
				 *       as raw bytes
				 * @param dest event identifier of the destination of the data joining
				 * @param data data to be redirected
				 *
				 * \~
				 */
				void forward(const event::id_t dest, string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки сброса без сохранения состояния (RFC 9000 §10.3)
				 *
				 * @note Отправляется в ответ на датаграмму, адресованную соединению,
				 *       о котором сервер ничего не помнит: без сброса удалённый узел
				 *       продолжит отправку до самого таймаута простоя
				 *
				 * @param oid  идентификатор события сессии
				 * @param data данные вызвавшей сброс датаграммы
				 * @param size размер вызвавшей сброс датаграммы
				 * @return     результат отправки
				 *
				 * \~english
				 * @brief Method of sending a stateless reset (RFC 9000 §10.3)
				 * @note Sent in answer to a datagram addressed to a connection
				 *       about which the server remembers nothing: without the reset the remote node
				 *       will continue the sending right up to the idle timeout
				 * @param oid  event identifier of the session
				 * @param data data of the datagram that has caused the reset
				 * @param size size of the datagram that has caused the reset
				 * @return     result of the sending
				 *
				 * \~
				 */
				bool drop(const event::id_t oid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения сессии соединения
				 *
				 * @param oid идентификатор события сессии
				 *
				 * \~english
				 * @brief Method of terminating the session of a connection
				 * @param oid event identifier of the session
				 *
				 * \~
				 */
				void erase(const event::id_t oid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки события пересоздания процесса кластера
				 *
				 * @param old старый идентификатор процесса
				 * @param pid текущий идентификатор процесса
				 *
				 * \~english
				 * @brief Method of processing the event of the recreation of a process of the cluster
				 * @param old old process identifier
				 * @param pid current process identifier
				 *
				 * \~
				 */
				void rebase(const pid_t old, const pid_t pid) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения события завершения работы процесса кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param status состояние, с которым завершился процесс
				 *
				 * \~english
				 * @brief Method of receiving the event of the termination of the work of a process of the cluster
				 * @param pid    process identifier
				 * @param status state with which the process has terminated
				 *
				 * \~
				 */
				void exit(const pid_t pid, const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события отправки сообщения процессу кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param size размер отправленного сообщения
				 *
				 * \~english
				 * @brief Method of processing the event of sending a message to a process of the cluster
				 * @param pid  process identifier
				 * @param size size of the sent message
				 *
				 * \~
				 */
				void sending(const pid_t pid, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения событий активации/деактивации кластера
				 *
				 * @note На событие START дочернего процесса поднимается независимый
				 *       сервер QUIC: сокет создаётся и привязывается уже в самом
				 *       дочернем процессе, поэтому каждый воркер владеет собственным
				 *       сокетом (SO_REUSEPORT на общем порту для Linux/FreeBSD либо
				 *       выделенный из диапазона порт для прочих систем)
				 *
				 * @param pid   идентификатор процесса
				 * @param event флаг события кластера
				 *
				 * \~english
				 * @brief Method of receiving the activation/deactivation events of the cluster
				 * @note At the START event of a child process an independent QUIC server
				 *       is raised: the socket is created and bound already in the very
				 *       child process, therefore every worker owns its own
				 *       socket (SO_REUSEPORT on a common port for Linux/FreeBSD or
				 *       a port allocated from a range for the other systems)
				 * @param pid   process identifier
				 * @param event flag of the cluster event
				 *
				 * \~
				 */
				void cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события получения сообщения от процесса кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param data данные полученного сообщения
				 * @param size размер данных полученного сообщения
				 *
				 * \~english
				 * @brief Method of processing the event of receiving a message from a process of the cluster
				 * @param pid  process identifier
				 * @param data data of the received message
				 * @param size data size of the received message
				 *
				 * \~
				 */
				void message(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения статуса процесса кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param status новый статус процесса кластера
				 *
				 * \~english
				 * @brief Method of processing status change events of a process of the cluster
				 * @param pid    process identifier
				 * @param status new status of the process of the cluster
				 *
				 * \~
				 */
				void status(const pid_t pid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий ошибок кластера
				 *
				 * @param pid         идентификатор процесса
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 *
				 * \~english
				 * @brief Method of processing cluster error events
				 * @param pid         process identifier
				 * @param error       error type
				 * @param description error description
				 *
				 * \~
				 */
				void error(const pid_t pid, const event::error_t error, const string & description) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 *
				 * \~english
				 * @brief Method of processing availability/unavailability events of the outgoing message queue of the cluster
				 * @param pid    process identifier
				 * @param status queue availability status
				 * @param size   size of the available queue data
				 *
				 * \~
				 */
				void available(const pid_t pid, const event::status_t status, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки выделенного порта прослушивания дочернему процессу кластера
				 *
				 * @param pid  идентификатор дочернего процесса
				 * @param port выделяемый порт прослушивания
				 * @return     результат отправки
				 *
				 * \~english
				 * @brief Method of sending an allocated listening port to a child process of the cluster
				 * @param pid  identifier of the child process
				 * @param port listening port being allocated
				 * @return     result of the sending
				 *
				 * \~
				 */
				bool sendPort(const pid_t pid, const uint16_t port) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод запуска/остановки работы сервера
				 *
				 * @param status статус запуска/остановки сервера
				 *
				 * \~english
				 * @brief Method of launching/stopping the work of the server
				 * @param status status of the launch/stop of the server
				 *
				 * \~
				 */
				void launch(const event::status_t status) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки актуальности события сервера
				 *
				 * @note Событие актуально, если это событие сервера либо активная сессия
				 *       соединения (адресуемая идентификатором события сессии)
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 *
				 * \~english
				 * @brief Method of checking the relevance of a server event
				 * @note An event is relevant if it is a server event or an active session
				 *       of a connection (addressed by the event identifier of the session)
				 * @param eid event identifier
				 * @return    result of checking the relevance of the event
				 *
				 * \~
				 */
				bool isActual(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения переданного фасадом идентификатора события сервера к актуальному
				 *
				 * @note В режиме кластера дочерний процесс пересоздаёт собственное событие
				 *       сервера с новым идентификатором (см. message()), а фасад продолжает
				 *       обращаться к унаследованному до fork идентификатору. Метод подменяет
				 *       унаследованный идентификатор на собственный идентификатор события
				 *       сервера дочернего процесса; активная сессия с таким же идентификатором
				 *       (при повторном выделении идентификатора) имеет приоритет
				 *
				 * @param eid идентификатор события, переданный фасадом
				 * @return    актуальный идентификатор события сервера
				 *
				 * \~english
				 * @brief Method of bringing the server event identifier passed by the facade to the relevant one
				 * @note In the cluster mode a child process recreates its own server
				 *       event with a new identifier (see message()), while the facade continues
				 *       to address the identifier inherited before the fork. The method substitutes
				 *       the inherited identifier with the own identifier of the server event
				 *       of the child process; an active session with the same identifier
				 *       (at a repeated allocation of the identifier) has the priority
				 * @param eid event identifier passed by the facade
				 * @return    relevant identifier of the server event
				 *
				 * \~
				 */
				event::id_t actual(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки шаблона контекста безопасности соединений
				 *
				 * @note Криптография соединений задаётся целиком на шаблоне контекста:
				 *       сертификаты, доверенные центры, проверка узла и список
				 *       ALPN-протоколов настраиваются там. Кодер обязан пережить модуль
				 *
				 * @param coder объект кодера транспортной безопасности
				 * @param ctx   идентификатор шаблона контекста безопасности
				 *
				 * \~english
				 * @brief Method of setting the template of the security context of the connections
				 * @note The cryptography of the connections is given entirely on the template of the context:
				 *       the certificates, the trusted authorities, the verification of the node and the list
				 *       of the ALPN protocols are configured there. The coder is obliged to outlive the module
				 * @param coder object of the coder of the transport security
				 * @param ctx   identifier of the template of the security context
				 *
				 * \~
				 */
				void context(const tls::coder_t & coder, const tls::coder_t::id_t ctx) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки локальных транспортных параметров соединений (RFC 9000 §7.4)
				 *
				 * @param params локальные транспортные параметры
				 *
				 * \~english
				 * @brief Method of setting the local transport parameters of the connections (RFC 9000 §7.4)
				 * @param params local transport parameters
				 *
				 * \~
				 */
				void params(const quic::params::params_t & params) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
				 *
				 * @param mode режим проверки адреса клиента
				 *
				 * \~english
				 * @brief Method of setting the verification of the address of the client through a Retry packet (RFC 9000 §8.1.2)
				 * @param mode mode of the verification of the address of the client
				 *
				 * \~
				 */
				void retry(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
				 *
				 * @note Исходящие датаграммы помечаются поддержкой ECN, а маркировка
				 *       входящих сообщается соединениям: маршрутизатор на пути
				 *       сигнализирует о заторе, не отбрасывая пакет, и окно перегрузки
				 *       сокращается раньше и без утраты данных. Режим включается до
				 *       запуска сервера и требует извлечения метаданных каждой
				 *       датаграммы, что снижает пропускную способность приёма
				 *
				 * @param mode режим уведомления о перегрузке пути
				 *
				 * \~english
				 * @brief Method of setting the notification about the congestion of the path (RFC 9000 §13.4)
				 * @note The outgoing datagrams are marked with the support of ECN, while the marking of the
				 *       incoming ones is reported to the connections: a router on the path
				 *       signals about a congestion without discarding the packet, and the congestion window
				 *       is reduced earlier and without a loss of the data. The mode is enabled before
				 *       the launch of the server and requires the extraction of the metadata of every
				 *       datagram, which lowers the throughput of the reception
				 * @param mode mode of the notification about the congestion of the path
				 *
				 * \~
				 */
				void ecn(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
				 *
				 * @note Вызывается до запуска сервера. Без явной установки ключ генерируется
				 *       случайно при запуске: сброс без сохранения состояния будет работать
				 *       в пределах жизни процесса, но не переживёт его перезапуск
				 *
				 * @param key общий ключ вывода токенов сброса
				 *
				 * \~english
				 * @brief Method of setting the common key of the derivation of the reset tokens (RFC 9000 §10.3.2)
				 * @note Called before the launch of the server. Without an explicit setting the key is generated
				 *       randomly at the launch: the stateless reset will work
				 *       within the life of the process but will not outlive its restart
				 * @param key common key of the derivation of the reset tokens
				 *
				 * \~
				 */
				void resetKey(string_view key) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания события сервера QUIC поверх UDP
				 *
				 * @note Транспорт QUIC работает поверх дейтаграммного UDP-сокета: тип и
				 *       протокол принудительно приводятся к DATAGRAM/UDP независимо от
				 *       переданных значений (RFC 9000)
				 *
				 * @param family   семейство адресов события сервера
				 * @param type     тип события (игнорируется, приводится к DATAGRAM)
				 * @param protocol протокол события (игнорируется, приводится к UDP)
				 * @return         идентификатор созданного события сервера
				 *
				 * \~english
				 * @brief Method of creating a QUIC server event on top of UDP
				 * @note The QUIC transport works on top of a datagram UDP socket: the type and the
				 *       protocol are forcibly brought to DATAGRAM/UDP regardless of the
				 *       passed values (RFC 9000)
				 * @param family   address family of the server event
				 * @param type     event type (ignored, brought to DATAGRAM)
				 * @param protocol event protocol (ignored, brought to UDP)
				 * @return         identifier of the created server event
				 *
				 * \~
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of committing the settings of a server event
				 * @param eid event identifier of the server
				 * @return    result of performing the commit
				 *
				 * \~
				 */
				bool commit(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска работы события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения запуска
				 *
				 * \~english
				 * @brief Method of launching the work of a server event
				 * @param eid event identifier of the server
				 * @return    result of performing the launch
				 *
				 * \~
				 */
				bool launch(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод прослушивания порта сервера для приёма входящих соединений
				 *
				 * @param eid идентификатор события сервера
				 * @param max максимальный размер очереди ожидания соединений
				 * @return    результат выполнения прослушивания
				 *
				 * \~english
				 * @brief Method of listening on the port of the server for accepting the incoming connections
				 * @param eid event identifier of the server
				 * @param max maximum size of the queue of the waiting connections
				 * @return    result of performing the listening
				 *
				 * \~
				 */
				bool listen(const event::id_t eid, const uint32_t max) noexcept;
				/**
				 * \~russian
				 * @brief Метод приостановки работы события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения приостановки работы
				 *
				 * \~english
				 * @brief Method of suspending the work of a server event
				 * @param eid event identifier of the server
				 * @return    result of performing the suspension of the work
				 *
				 * \~
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления работы события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения возобновления работы
				 *
				 * \~english
				 * @brief Method of resuming the work of a server event
				 * @param eid event identifier of the server
				 * @return    result of performing the resumption of the work
				 *
				 * \~
				 */
				bool resume(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения данных от клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 *
				 * \~english
				 * @brief Method of receiving data from a client
				 * @param eid client event identifier
				 * @return    result of receiving the data
				 *
				 * \~
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных клиенту
				 *
				 * @note Отправляет прикладные данные сессии соединения потоком по
				 *       умолчанию через открытие двунаправленного потока (RFC 9000 §2.1);
				 *       для явного выбора потока используется перегрузка send(oid, sid, data, fin)
				 *
				 * @param eid    идентификатор события сессии
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт, поставленных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending data to a client
				 * @note Sends the application data of the session of a connection by the default stream
				 *       through the opening of a bidirectional stream (RFC 9000 §2.1);
				 *       for an explicit choice of the stream the send(oid, sid, data, fin) overload is used
				 * @param eid    event identifier of the session
				 * @param buffer data buffer to be sent
				 * @param size   size of the data to be sent
				 * @return       number of bytes placed into the sending queue
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод объединения потоков данных между двумя событиями
				 *
				 * @note Для транспорта QUIC объединение на уровне сокета не поддерживается:
				 *       данные шифруются на уровне соединения, поэтому метод всегда
				 *       возвращает отрицательный результат
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат объединения
				 *
				 * \~english
				 * @brief Method of joining the data streams between two events
				 * @note For the QUIC transport the joining at the level of the socket is not supported:
				 *       the data is encrypted at the level of the connection, therefore the method always
				 *       returns a negative result
				 * @param eid  identifier of the source event
				 * @param dest identifier of the destination event
				 * @return     result of the joining
				 *
				 * \~
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки контекста события клиента
				 *
				 * @note Для транспорта QUIC контекст сессии ведётся самим модулем по
				 *       идентификатору события сессии, поэтому внешний контекст не
				 *       устанавливается
				 *
				 * @param eid идентификатор события клиента
				 * @param ctx контекст события клиента
				 * @return    результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the context of a client event
				 * @note For the QUIC transport the context of a session is conducted by the module itself by
				 *       the event identifier of the session, therefore an external context is not
				 *       set
				 * @param eid client event identifier
				 * @param ctx context of the client event
				 * @return    result of performing the setting
				 *
				 * \~
				 */
				bool setContext(const event::id_t eid, void * ctx) noexcept;
				/**
				 * \~russian
				 * @brief Метод уничтожения события сервера
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 * \~english
				 * @brief Method of destroying a server event
				 * @param eid identifier of the event to be destroyed
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    опции события сервера
				 *
				 * \~english
				 * @brief Method of getting the options of a server event
				 * @param eid event identifier of the server
				 * @return    options of the server event
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций события сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param options опции события сервера для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the options of a server event
				 * @param eid     event identifier of the server
				 * @param options options of the server event to be set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции события сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param option опция события сервера для установки
				 * @param mode   режим установки опции события сервера
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting an option of a server event
				 * @param eid    event identifier of the server
				 * @param option option of the server event to be set
				 * @param mode   mode of setting the option of the server event
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 *
				 * \~english
				 * @brief Method of getting the metadata of the last received datagram packet
				 * @param eid event identifier of the server
				 * @return    metadata of the last received datagram packet
				 *
				 * \~
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    количество хопов последнего принятого пакета
				 *
				 * \~english
				 * @brief Method of getting the number of the hops of the last received packet
				 * @param eid event identifier of the server
				 * @return    number of the hops of the last received packet
				 *
				 * \~
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the number of the hops of the last received packet
				 * @param eid  event identifier of the server
				 * @param hops number of the hops of the last received packet
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события сервера
				 * @return    максимальное количество хопов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the hops through which a packet can pass
				 * @param eid event identifier of the server
				 * @return    maximum number of the hops
				 *
				 * \~
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the hops through which a packet can pass
				 * @param eid  event identifier of the server
				 * @param hops maximum number of the hops
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    сетевой интерфейс сервера
				 *
				 * \~english
				 * @brief Method of getting the network interface of the server
				 * @param eid event identifier of the server
				 * @return    network interface of the server
				 *
				 * \~
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network interface of the server
				 * @param eid  event identifier of the server
				 * @param name name of the network interface to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения семейства адресов события сервера
				 *
				 * @note Переопределяет базовый метод для приведения унаследованного дочерним
				 *       процессом кластера идентификатора события к собственному (см. actual())
				 *
				 * @param eid идентификатор события сервера
				 * @return    семейство адресов события сервера
				 *
				 * \~english
				 * @brief Method of getting the address family of a server event
				 * @note Overrides the base method in order to bring the identifier of an event inherited by a child
				 *       process of the cluster to its own one (see actual())
				 * @param eid event identifier of the server
				 * @return    address family of the server event
				 *
				 * \~
				 */
				event::family_t family(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения порта сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    порт сервера
				 *
				 * \~english
				 * @brief Method of getting the port of the server
				 * @param eid event identifier of the server
				 * @return    port of the server
				 *
				 * \~
				 */
				uint16_t getPort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки порта сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param port порт сервера для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the port of the server
				 * @param eid  event identifier of the server
				 * @param port port of the server to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @return        значение адреса сервера
				 *
				 * \~english
				 * @brief Method of getting the address of the server
				 * @param eid     event identifier of the server
				 * @param address address type of the server
				 * @return        value of the address of the server
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of the server
				 * @param eid     event identifier of the server
				 * @param address address type of the server
				 * @param value   value of the address of the server
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   структура сетевого адреса сервера
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of the server
				 * @param eid     event identifier of the server
				 * @param address address type of the server
				 * @param value   structure of the network address of the server
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   объект для извлечения адреса сервера
				 * @return        результат выполнения извлечения адреса сервера
				 *
				 * \~english
				 * @brief Method of getting the address of the server
				 * @param eid     event identifier of the server
				 * @param address address type of the server
				 * @param value   object for extracting the address of the server
				 * @return        result of extracting the address of the server
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @return    MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of getting the MTU of the network interface
				 * @param eid event identifier of the server
				 * @return    MTU of the network interface
				 *
				 * \~
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of setting the MTU of the network interface
				 * @param eid event identifier of the server
				 * @param mtu MTU size of the interface
				 * @return    result of setting the MTU of the network interface
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события сервера
				 * @return    текущий режим обнаружения MTU
				 *
				 * \~english
				 * @brief Method of getting the discovery mode of the maximum packet size (MTU)
				 * @param eid event identifier of the server
				 * @return    current MTU discovery mode
				 *
				 * \~
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события сервера
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the discovery mode of the maximum packet size (MTU)
				 * @param eid  event identifier of the server
				 * @param mode discovery mode of the maximum packet size (MTU)
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима трансляции пакетов сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 * \~english
				 * @brief Method of getting the packet delivery mode of the server
				 * @param eid event identifier of the server
				 * @return    packet delivery mode (unicast, multicast, broadcast)
				 *
				 * \~
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима трансляции пакетов сервера
				 *
				 * @param eid      идентификатор события сервера
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the packet delivery mode of the server
				 * @param eid      event identifier of the server
				 * @param delivery packet delivery mode (unicast, multicast, broadcast)
				 * @return         result of performing the setting
				 *
				 * \~
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       размер буфера сервера
				 *
				 * \~english
				 * @brief Method of getting the buffer size of the server
				 * @param eid    event identifier of the server
				 * @param action action type of the server
				 * @return       buffer size of the server
				 *
				 * \~
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @param size   размер буфера сервера
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the buffer size of the server
				 * @param eid    event identifier of the server
				 * @param action action type of the server
				 * @param size   buffer size of the server
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 *
				 * \~english
				 * @brief Method of getting the usage mode of the read timeout of the event
				 * @param eid event identifier
				 * @return    usage mode of the read timeout of the event
				 *
				 * \~
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 *
				 * \~english
				 * @brief Method of setting the usage mode of the read timeout of the event
				 * @param eid   event identifier
				 * @param usage usage mode of the read timeout of the event (reusable or disposable)
				 *
				 * \~
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения таймаута сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of the server
				 * @param eid    event identifier of the server
				 * @param action action type of the server
				 * @return       value of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param action  тип действия сервера
				 * @param timeout значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the timeout of the server
				 * @param eid     event identifier of the server
				 * @param action  action type of the server
				 * @param timeout value of the timeout in milliseconds
				 *
				 * \~
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности сервера
				 *
				 * @param eid       идентификатор события сервера
				 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
				 * @param bandwidth пропускная способность сервера для установки
				 * @return          результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of the server
				 * @param eid       event identifier of the server
				 * @param limiting  mode of limiting the bandwidth of the server (egress or ingress)
				 * @param bandwidth bandwidth of the server to be set
				 * @return          result of performing the setting
				 *
				 * \~
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки параметров keep-alive для сервера
				 *
				 * @param eid   идентификатор события сервера
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the keep-alive parameters for the server
				 * @param eid   event identifier of the server
				 * @param cnt   number of the keep-alive packets
				 * @param idle  idle time before sending the first keep-alive packet in seconds
				 * @param intvl interval between the keep-alive packets in seconds
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    значение DSCP
				 *
				 * \~english
				 * @brief Method of getting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid event identifier of the server
				 * @return    DSCP value
				 *
				 * \~
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid  event identifier of the server
				 * @param dscp DSCP value
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    event identifier of the server
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    event identifier of the server
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 *
				 * \~english
				 * @brief Method of setting the name of the cluster
				 * @param name name of the cluster to be set
				 *
				 * \~
				 */
				void clusterName(string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения семейства кластера
				 *
				 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
				 *
				 * \~english
				 * @brief Method of getting the family of the cluster
				 * @return family to which the cluster belongs (MASTER or CHILDREN)
				 *
				 * \~
				 */
				cluster_t::family_t clusterFamily() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима активации кластера
				 *
				 * @return режим активации кластера
				 *
				 * \~english
				 * @brief Method of getting the activation mode of the cluster
				 * @return activation mode of the cluster
				 *
				 * \~
				 */
				event::mode_t clusterMode() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима работы кластера
				 *
				 * @param mode режим активации/деактивации кластера
				 *
				 * \~english
				 * @brief Method of setting the working mode of the cluster
				 * @param mode activation/deactivation mode of the cluster
				 *
				 * \~
				 */
				void clusterMode(const event::mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества процессов
				 *
				 * @return максимальное количество процессов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the processes
				 * @return maximum number of the processes
				 *
				 * \~
				 */
				uint16_t clusterCount() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества процессов
				 *
				 * @param count максимальное количество процессов
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the processes
				 * @param count maximum number of the processes
				 *
				 * \~
				 */
				void clusterCount(const uint16_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 *
				 * \~english
				 * @brief Method of getting the list of the child processes
				 * @return list of the child processes
				 *
				 * \~
				 */
				unordered_set <pid_t> clusterWorkers() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки диапазона портов для выделения дочерним процессам кластера
				 *
				 * @note Родительский процесс раздаёт порты из диапазона дочерним процессам,
				 *       каждый из которых поднимает собственный сокет сервера. На Linux/FreeBSD
				 *       порт может повторяться (несколько процессов делят его через SO_REUSEPORT),
				 *       на прочих системах порт выделяется дочернему процессу монопольно.
				 *       Пустой диапазон означает использование единственного порта прослушивания
				 *
				 * @param begin начальный порт диапазона (0 - использовать порт прослушивания)
				 * @param end   конечный порт диапазона (0 - использовать порт прослушивания)
				 *
				 * \~english
				 * @brief Method of setting the range of the ports for allocating to the child processes of the cluster
				 * @note The parent process distributes the ports from the range to the child processes,
				 *       each of which raises its own server socket. On Linux/FreeBSD
				 *       a port may repeat (several processes share it through SO_REUSEPORT),
				 *       on the other systems a port is allocated to a child process exclusively.
				 *       An empty range means the use of a single listening port
				 * @param begin initial port of the range (0 — use the listening port)
				 * @param end   final port of the range (0 — use the listening port)
				 *
				 * \~
				 */
				void clusterRange(const uint16_t begin, const uint16_t end) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка дочерних процессов, не получивших порт прослушивания
				 *
				 * @note На системах, где порт выделяется дочернему процессу монопольно
				 *       (macOS/Solaris/OpenBSD/NetBSD), при нехватке портов диапазона часть
				 *       дочерних процессов остаётся без сокета сервера. Их идентификаторы
				 *       возвращаются здесь, чтобы приложение могло доотправить им порт
				 *       вручную методом clusterAssign()
				 *
				 * @return список идентификаторов дочерних процессов, работающих в холостую
				 *
				 * \~english
				 * @brief Method of getting the list of the child processes that have not received a listening port
				 * @note On the systems where a port is allocated to a child process exclusively
				 *       (macOS/Solaris/OpenBSD/NetBSD), at a shortage of the ports of the range a part of the
				 *       child processes is left without a server socket. Their identifiers
				 *       are returned here so that the application can send them a port additionally
				 *       by hand with the clusterAssign() method
				 * @return list of the identifiers of the child processes working idly
				 *
				 * \~
				 */
				unordered_set <pid_t> clusterIdle() const noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки порта прослушивания конкретному дочернему процессу кластера
				 *
				 * @note Позволяет вручную поднять сервер на дочернем процессе, которому при
				 *       автоматической раздаче порт не достался (см. clusterIdle()). Дочерний
				 *       процесс поднимает собственный сокет на полученном порту
				 *
				 * @param pid  идентификатор дочернего процесса
				 * @param port порт прослушивания для дочернего процесса
				 * @return     результат отправки порта дочернему процессу
				 *
				 * \~english
				 * @brief Method of sending a listening port to a particular child process of the cluster
				 * @note Makes it possible to raise a server by hand on a child process which has not got a port at
				 *       the automatic distribution (see clusterIdle()). The child
				 *       process raises its own socket on the received port
				 * @param pid  identifier of the child process
				 * @param port listening port for the child process
				 * @return     result of sending the port to the child process
				 *
				 * \~
				 */
				bool clusterAssign(const pid_t pid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to the parent process
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t clusterSend(const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to a child process
				 * @param pid    identifier of the process for receiving the message
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to all the child processes
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки флага автоматического возрождения процессов
				 *
				 * @param mode флаг возрождения процессов
				 *
				 * \~english
				 * @brief Method of setting the flag of the automatic revival of the processes
				 * @param mode flag of the revival of the processes
				 *
				 * \~
				 */
				void clusterRebirth(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
				 *
				 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
				 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the parameters of the protection against a loop of restarts of the processes of the cluster
				 * @param limit  maximum number of consecutive fast falls before the cluster is stopped (0 — without a limit)
				 * @param window time window of a «fast» (early) fall of a process in milliseconds
				 *
				 * \~
				 */
				void clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа протокола передачи данных между воркерами
				 *
				 * @return тип протокола передачи данных между воркерами
				 *
				 * \~english
				 * @brief Method of getting the type of the data transfer protocol between the workers
				 * @return type of the data transfer protocol between the workers
				 *
				 * \~
				 */
				event::type_t clusterGetTypeEventMessage() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки типа протокола передачи данных между воркерами
				 *
				 * @param type тип протокола передачи данных между воркерами для установки
				 *
				 * \~english
				 * @brief Method of setting the type of the data transfer protocol between the workers
				 * @param type type of the data transfer protocol between the workers to be set
				 *
				 * \~
				 */
				void clusterSetTypeEventMessage(const event::type_t type) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 * \~english
				 * @brief Method of getting the event buffer size
				 * @param pid    process identifier
				 * @param action event action type
				 * @return       event buffer size
				 *
				 * \~
				 */
				size_t clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the event buffer size
				 * @param pid    process identifier
				 * @param action event action type
				 * @param size   event buffer size
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод остановки сервера
				 *
				 * \~english
				 * @brief Method of stopping the server
				 *
				 * \~
				 */
				void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска сервера
				 *
				 * \~english
				 * @brief Method of launching the server
				 *
				 * \~
				 */
				void start() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод открытия потока приложения соединения
				 *
				 * @param oid  идентификатор события сессии
				 * @param mode режим однонаправленного потока
				 * @return     идентификатор открытого потока
				 *
				 * \~english
				 * @brief Method of opening an application stream of a connection
				 * @param oid  event identifier of the session
				 * @param mode mode of a unidirectional stream
				 * @return     identifier of the opened stream
				 *
				 * \~
				 */
				uint64_t open(const event::id_t oid, const bool mode = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных в поток приложения соединения
				 *
				 * @param oid  идентификатор события сессии
				 * @param sid  идентификатор потока приложения
				 * @param data отправляемые данные
				 * @param fin  флаг завершения потока
				 * @return     результат постановки данных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending data into an application stream of a connection
				 * @param oid  event identifier of the session
				 * @param sid  identifier of the application stream
				 * @param data data being sent
				 * @param fin  flag of the termination of the stream
				 * @return     result of placing the data into the sending queue
				 *
				 * \~
				 */
				size_t send(const event::id_t oid, const uint64_t sid, string_view data, const bool fin = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки водяных меток буфера отправки потоков соединения (backpressure)
				 *
				 * @note Верхняя метка ограничивает объём несобранных данных на поток: сверх неё send()
				 *       принимает данные лишь частично, а по опустошению буфера ниже нижней метки поток
				 *       сигнализируется колбэком "writable". Ноль снимает ограничение (буфер не ограничен)
				 *
				 * @param oid  идентификатор события сессии
				 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
				 * @param low  нижняя водяная метка (порог сигнала "writable")
				 *
				 * \~english
				 * @brief Method of setting the watermarks of the sending buffer of the streams of a connection (backpressure)
				 * @note The upper mark limits the volume of the unassembled data per stream: beyond it send()
				 *       accepts the data only partially, while upon the emptying of the buffer below the lower mark the stream
				 *       is signalled by the "writable" callback. Zero removes the limit (the buffer is not limited)
				 * @param oid  event identifier of the session
				 * @param high upper watermark (capacity of the sending buffer of the stream)
				 * @param low  lower watermark (threshold of the "writable" signal)
				 *
				 * \~
				 */
				void sendWaterMarks(const event::id_t oid, const size_t high, const size_t low) noexcept;
				/**
				 * \~russian
				 * @brief Метод назначения pull-источника данных потока (RFC 9000 §2.2)
				 *
				 * @note Альтернатива send() для больших тел: движок сам запрашивает данные у источника
				 *       по мере места в буфере отправки потока, не требуя держать копию всего тела
				 *
				 * @param oid    идентификатор события сессии
				 * @param sid    идентификатор потока приложения
				 * @param source pull-источник данных тела потока
				 *
				 * \~english
				 * @brief Method of assigning a pull source of the data of a stream (RFC 9000 §2.2)
				 * @note An alternative to send() for large bodies: the engine itself requests the data from the source
				 *       as the space in the sending buffer of the stream appears, without requiring to keep a copy of the whole body
				 * @param oid    event identifier of the session
				 * @param sid    identifier of the application stream
				 * @param source pull source of the data of the body of the stream
				 *
				 * \~
				 */
				void dataSource(const event::id_t oid, const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки датаграммы приложения соединению (RFC 9221)
				 *
				 * @note Доставка датаграмм ненадёжна: потерянная датаграмма повторно
				 *       не отправляется, порядок доставки не гарантируется. Отправка
				 *       возможна только когда удалённый узел анонсировал их приём
				 *
				 * @param oid  идентификатор события сессии
				 * @param data данные датаграммы приложения
				 * @return     результат отправки
				 *
				 * \~english
				 * @brief Method of sending an application datagram to a connection (RFC 9221)
				 * @note The delivery of the datagrams is unreliable: a lost datagram is not
				 *       sent again, the order of the delivery is not guaranteed. The sending
				 *       is possible only when the remote node has announced their reception
				 * @param oid  event identifier of the session
				 * @param data data of the application datagram
				 * @return     result of the sending
				 *
				 * \~
				 */
				bool datagram(const event::id_t oid, string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
				 *
				 * @param oid идентификатор события сессии
				 * @return    предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
				 *
				 * \~english
				 * @brief Method of getting the limit size of a datagram being sent (RFC 9221 §3)
				 * @param oid event identifier of the session
				 * @return    limit size of the data of a datagram in octets (0 — the datagrams are not supported)
				 *
				 * \~
				 */
				size_t datagrams(const event::id_t oid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
				 *
				 * @param oid    идентификатор события сессии
				 * @param code   код ошибки приложения
				 * @param reason человекочитаемая причина завершения
				 *
				 * \~english
				 * @brief Method of the termination of a connection by the application (RFC 9000 §10.2)
				 * @param oid    event identifier of the session
				 * @param code   error code of the application
				 * @param reason human-readable reason of the termination
				 *
				 * \~
				 */
				void close(const event::id_t oid, const uint64_t code = 0, string_view reason = "") noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения согласованного ALPN-протокола соединения
				 *
				 * @param oid идентификатор события сессии
				 * @return    согласованный ALPN-протокол
				 *
				 * \~english
				 * @brief Method of getting the negotiated ALPN protocol of a connection
				 * @param oid event identifier of the session
				 * @return    negotiated ALPN protocol
				 *
				 * \~
				 */
				tls::coder_t::alpn_t alpn(const event::id_t oid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса удалённого эндпоинта соединения
				 *
				 * @param oid идентификатор события сессии
				 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
				 *
				 * \~english
				 * @brief Method of getting the address of the remote endpoint of a connection
				 * @param oid event identifier of the session
				 * @return    address of the remote endpoint in the form "address:port"
				 *
				 * \~
				 */
				string address(const event::id_t oid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @note Поддерживаются функции обратного вызова: "open" на установленное
				 *       соединение, "read" на собранные данные потока приложения,
				 *       "datagram" на принятую датаграмму приложения и "close"
				 *       на завершённое соединение
				 *
				 * @param callback функции обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @note The following callback functions are supported: "open" on an established
				 *       connection, "read" on the assembled data of an application stream,
				 *       "datagram" on an accepted application datagram and "close"
				 *       on a terminated connection
				 * @param callback callback functions
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				QuicServer(const QuicServer &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 * @return current value of the object
				 *
				 * \~
				 */
				QuicServer & operator = (const QuicServer &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit QuicServer(const fmk_t * fmk, const log_t * log) noexcept;
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
				~QuicServer() noexcept;
		} quic_server_t;

		/**
		 * \~russian
		 * @brief Класс модуля клиента транспортного протокола QUIC
		 *
		 * @details Ведёт одно соединение с удалённым сервером поверх асинхронного
		 *          сетевого движка. Билет возобновления, присланный сервером после
		 *          установления соединения, сохраняется модулем и подставляется
		 *          при следующем подключении, поэтому повторное соединение с тем
		 *          же сервером обходится без полного хендшейка (RFC 9001 §4.6)
		 *
		 * \~english
		 * @brief Class of the client module of the QUIC transport protocol
		 * @details Conducts a single connection with a remote server on top of the asynchronous
		 *          network engine. The resumption ticket sent by the server after
		 *          the establishment of the connection is preserved by the module and is substituted
		 *          at the next connection, therefore a repeated connection with the same
		 *          server makes do without a full handshake (RFC 9001 §4.6)
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ QuicClient : public unit_t {
			private:
				// Идентификатор события клиента
				event::id_t _eid;
				// Идентификатор события интервала таймеров соединения
				event::id_t _tid;
			private:
				// Флаг оповещения приложения об установленном соединении
				bool _connected;
				// Флаг выполненного оповещения приложения о завершённом соединении
				bool _notified;
				// Флаг уведомления о перегрузке пути (RFC 9000 §13.4)
				bool _ecn;
			private:
				// Семейство адресов события клиента
				event::family_t _family;
				// Маркировка, установленная на сокете события клиента
				event::ecn_t _marking;
			private:
				// Идентификатор шаблона контекста безопасности
				tls::coder_t::id_t _ctx;
				// Объект кодера транспортной безопасности
				const tls::coder_t * _coder;
			private:
				/**
				 * Сохранённый токен проверки адреса фрейма NEW_TOKEN. Подставляется
				 * в первый пакет следующего соединения и позволяет пропустить обмен
				 * пакетом Retry, сэкономив круг задержки (RFC 9000 §8.1.3)
				 */
				string _token;
			private:
				// Локальные транспортные параметры соединения
				quic::params::params_t _params;
			private:
				// Порт удалённого сервера (устанавливается фасадом до подключения)
				uint16_t _targetPort;
				// Структура сетевого адреса удалённого сервера (устанавливается фасадом до подключения)
				unique_ptr <net::addr_t> _target;
			private:
				// Объект соединения QUIC
				unique_ptr <quic::connection_t> _connection;
			private:
				/**
				 * Идентификатор события-приёмника объединения данных (splice): собранные
				 * данные потоков соединения перенаправляются в это событие вместо выдачи
				 * приложению (0 - объединение не установлено)
				 */
				event::id_t _dest;
				/**
				 * Идентификатор туннельного потока для входящих объединённых данных:
				 * байты, поступающие в соединение из события-источника, отправляются
				 * этим потоком приложения (INVALID_STREAM - поток ещё не открыт)
				 */
				uint64_t _tunnel;
			private:
				/**
				 * \~russian
				 * @brief Метод получения текущего времени в миллисекундах
				 *
				 * @return текущее время в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the current time in milliseconds
				 * @return current time in milliseconds
				 *
				 * \~
				 */
				uint64_t date() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки принятой датаграммы соединения
				 *
				 * @param eid  идентификатор события клиента
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 *
				 * \~english
				 * @brief Method of processing an accepted datagram of a connection
				 * @param eid  client event identifier
				 * @param data data of the datagram
				 * @param size size of the datagram
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки просроченных таймеров соединения
				 *
				 * @param eid    идентификатор события интервала
				 * @param status статус события интервала
				 *
				 * \~english
				 * @brief Method of processing the expired timers of a connection
				 * @param eid    event identifier of the interval
				 * @param status status of the interval event
				 *
				 * \~
				 */
				void tick(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки завершения подключения к серверу
				 *
				 * @param eid идентификатор события клиента
				 * @param ok  результат подключения к серверу
				 *
				 * \~english
				 * @brief Method of processing the completion of the connection to the server
				 * @param eid client event identifier
				 * @param ok  result of the connection to the server
				 *
				 * \~
				 */
				void connected(const event::id_t eid, const bool ok) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод выдачи собранных данных потоков приложения
				 *
				 * \~english
				 * @brief Method of issuing the assembled data of the application streams
				 *
				 * \~
				 */
				void process() noexcept;
				/**
				 * \~russian
				 * @brief Метод оповещения приложения о завершённом соединении
				 *
				 * @note Соединение завершается как удалённым эндпоинтом, так и по
				 *       истечении периода завершения, выдерживаемого после завершения
				 *       соединения самим приложением (RFC 9000 §10.2). Оповещение
				 *       выполняется однократно
				 *
				 * \~english
				 * @brief Method of notifying the application about a terminated connection
				 * @note A connection is terminated both by the remote endpoint and by
				 *       the expiration of the closing period held after the termination of the
				 *       connection by the application itself (RFC 9000 §10.2). The notification
				 *       is performed once
				 *
				 * \~
				 */
				void complete() noexcept;
				/**
				 * \~russian
				 * @brief Метод применения маркировки соединения к сокету события клиента
				 *
				 * @param marking требуемая маркировка исходящих датаграмм
				 *
				 * \~english
				 * @brief Method of applying the marking of a connection to the socket of the client event
				 * @param marking required marking of the outgoing datagrams
				 *
				 * \~
				 */
				void mark(const event::ecn_t marking) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки готовых исходящих датаграмм соединения
				 *
				 * \~english
				 * @brief Method of sending the ready outgoing datagrams of a connection
				 *
				 * \~
				 */
				void flush() noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки объединённых данных в туннельный поток соединения
				 *
				 * @note Байты, поступившие из события-источника объединения (splice),
				 *       отправляются туннельным потоком соединения с их шифрованием
				 *       на уровне соединения (RFC 9000 §2.1)
				 *
				 * @param data данные для отправки в туннельный поток
				 * @param size размер данных для отправки
				 * @return     результат постановки данных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending the joined data into the tunnel stream of a connection
				 * @note The bytes that have arrived from the source event of the joining (splice)
				 *       are sent by the tunnel stream of the connection with their encryption
				 *       at the level of the connection (RFC 9000 §2.1)
				 * @param data data to be sent into the tunnel stream
				 * @param size size of the data to be sent
				 * @return     result of placing the data into the sending queue
				 *
				 * \~
				 */
				bool inject(const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод перенаправления собранных данных соединения в событие-приёмник объединения
				 *
				 * @param data данные для перенаправления
				 *
				 * \~english
				 * @brief Method of redirecting the assembled data of a connection into the destination event of the joining
				 * @param data data to be redirected
				 *
				 * \~
				 */
				void forward(string_view data) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки актуальности события клиента
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 *
				 * \~english
				 * @brief Method of checking the relevance of a client event
				 * @param eid event identifier
				 * @return    result of checking the relevance of the event
				 *
				 * \~
				 */
				bool isActual(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска/остановки работы клиента
				 *
				 * @param status статус запуска/остановки клиента
				 *
				 * \~english
				 * @brief Method of launching/stopping the work of the client
				 * @param status status of the launch/stop of the client
				 *
				 * \~
				 */
				void launch(const event::status_t status) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки шаблона контекста безопасности соединения
				 *
				 * @param coder объект кодера транспортной безопасности
				 * @param ctx   идентификатор шаблона контекста безопасности
				 *
				 * \~english
				 * @brief Method of setting the template of the security context of the connection
				 * @param coder object of the coder of the transport security
				 * @param ctx   identifier of the template of the security context
				 *
				 * \~
				 */
				void context(const tls::coder_t & coder, const tls::coder_t::id_t ctx) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
				 *
				 * @note Исходящие датаграммы помечаются поддержкой ECN, а маркировка
				 *       принятых извлекается из заголовка IP-пакета. Путь, стирающий
				 *       маркировку, соединение выявляет само и маркировку снимает
				 *
				 * @param mode режим уведомления о перегрузке пути
				 *
				 * \~english
				 * @brief Method of setting the notification about the congestion of the path (RFC 9000 §13.4)
				 * @note The outgoing datagrams are marked with the support of ECN, while the marking of the
				 *       accepted ones is extracted from the header of the IP packet. A path that erases
				 *       the marking is revealed by the connection itself, and it removes the marking
				 * @param mode mode of the notification about the congestion of the path
				 *
				 * \~
				 */
				void ecn(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки локальных транспортных параметров соединения (RFC 9000 §7.4)
				 *
				 * @param params локальные транспортные параметры
				 *
				 * \~english
				 * @brief Method of setting the local transport parameters of the connection (RFC 9000 §7.4)
				 * @param params local transport parameters
				 *
				 * \~
				 */
				void params(const quic::params::params_t & params) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения сохранённого токена проверки адреса (RFC 9000 §8.1.3)
				 *
				 * @note Токен присылается сервером по завершении соединения и сохраняется
				 *       модулем самостоятельно, переживая соединение и пригодный
				 *       к предъявлению между запусками приложения
				 *
				 * @return токен проверки адреса (пусто - токен не получен)
				 *
				 * \~english
				 * @brief Method of extracting the preserved address verification token (RFC 9000 §8.1.3)
				 * @note The token is sent by the server upon the completion of the connection and is preserved
				 *       by the module by itself, outliving the connection and being suitable
				 *       for a presentation between the runs of the application
				 * @return address verification token (empty — the token has not been received)
				 *
				 * \~
				 */
				const string & token() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сохранённого токена проверки адреса (RFC 9000 §8.1.3)
				 *
				 * @param token токен проверки адреса
				 *
				 * \~english
				 * @brief Method of setting the preserved address verification token (RFC 9000 §8.1.3)
				 * @param token address verification token
				 *
				 * \~
				 */
				void token(string_view token) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки принятия ранних данных удалённым сервером (RFC 9001 §4.6.2)
				 *
				 * @note Сервер вправе отказать в ранних данных, и тогда отправленное
				 *       ими содержимое передаётся заново после хендшейка. Проверка
				 *       нужна приложению, которому важно знать, состоялся ли 0-RTT
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the acceptance of the early data by the remote server (RFC 9001 §4.6.2)
				 * @note The server has the right to refuse the early data, and then the content sent
				 *       by it is transmitted anew after the handshake. The check
				 *       is needed by an application to which it is important to know whether the 0-RTT has taken place
				 * @return result of the check
				 *
				 * \~
				 */
				bool early() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания события клиента QUIC поверх UDP
				 *
				 * @note Транспорт QUIC работает поверх дейтаграммного UDP-сокета: тип и
				 *       протокол принудительно приводятся к DATAGRAM/UDP независимо от
				 *       переданных значений (RFC 9000)
				 *
				 * @param family   семейство адресов события клиента
				 * @param type     тип события (игнорируется, приводится к DATAGRAM)
				 * @param protocol протокол события (игнорируется, приводится к UDP)
				 * @return         идентификатор созданного события клиента
				 *
				 * \~english
				 * @brief Method of creating a QUIC client event on top of UDP
				 * @note The QUIC transport works on top of a datagram UDP socket: the type and the
				 *       protocol are forcibly brought to DATAGRAM/UDP regardless of the
				 *       passed values (RFC 9000)
				 * @param family   address family of the client event
				 * @param type     event type (ignored, brought to DATAGRAM)
				 * @param protocol event protocol (ignored, brought to UDP)
				 * @return         identifier of the created client event
				 *
				 * \~
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of committing the settings of a client event
				 * @param eid client event identifier
				 * @return    result of performing the commit
				 *
				 * \~
				 */
				bool commit(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения запуска
				 *
				 * \~english
				 * @brief Method of launching the work of a client event
				 * @param eid client event identifier
				 * @return    result of performing the launch
				 *
				 * \~
				 */
				bool launch(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод приостановки работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения приостановки работы
				 *
				 * \~english
				 * @brief Method of suspending the work of a client event
				 * @param eid client event identifier
				 * @return    result of performing the suspension of the work
				 *
				 * \~
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения возобновления работы
				 *
				 * \~english
				 * @brief Method of resuming the work of a client event
				 * @param eid client event identifier
				 * @return    result of performing the resumption of the work
				 *
				 * \~
				 */
				bool resume(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод отключения клиента от удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения отключения
				 *
				 * \~english
				 * @brief Method of disconnecting the client from the remote server
				 * @param eid client event identifier
				 * @return    result of performing the disconnection
				 *
				 * \~
				 */
				bool disconnect(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод подключения клиента к удалённому серверу
				 *
				 * @note Создаёт соединение QUIC на шаблоне контекста безопасности и
				 *       начинает хендшейк (RFC 9001). Адрес и порт удалённого сервера
				 *       берутся из ранее установленной штатной структуры сетевого адреса
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of connecting the client to a remote server
				 * @note Creates a QUIC connection on the template of the security context and
				 *       begins the handshake (RFC 9001). The address and the port of the remote server
				 *       are taken from the previously set regular structure of the network address
				 * @param eid client event identifier
				 * @return    result of performing the connection
				 *
				 * \~
				 */
				bool connect(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод подключения клиента к удалённому серверу
				 *
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of connecting the client to a remote server
				 * @param ids list of the event identifiers to be connected
				 * @return    result of performing the connection
				 *
				 * \~
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения данных от удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 *
				 * \~english
				 * @brief Method of receiving data from the remote server
				 * @param eid client event identifier
				 * @return    result of receiving the data
				 *
				 * \~
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных удалённому серверу
				 *
				 * @note Отправляет прикладные данные потоком по умолчанию через открытие
				 *       двунаправленного потока (RFC 9000 §2.1); для явного выбора потока
				 *       используется перегрузка send(sid, data, fin)
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт, поставленных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending data to the remote server
				 * @note Sends the application data by the default stream through the opening
				 *       of a bidirectional stream (RFC 9000 §2.1); for an explicit choice of the stream
				 *       the send(sid, data, fin) overload is used
				 * @param eid    client event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data to be sent
				 * @return       number of bytes placed into the sending queue
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод объединения потоков данных между двумя событиями
				 *
				 * @note Для транспорта QUIC объединение на уровне сокета не поддерживается:
				 *       данные шифруются на уровне соединения, поэтому метод всегда
				 *       возвращает отрицательный результат
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат объединения
				 *
				 * \~english
				 * @brief Method of joining the data streams between two events
				 * @note For the QUIC transport the joining at the level of the socket is not supported:
				 *       the data is encrypted at the level of the connection, therefore the method always
				 *       returns a negative result
				 * @param eid  identifier of the source event
				 * @param dest identifier of the destination event
				 * @return     result of the joining
				 *
				 * \~
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
				/**
				 * \~russian
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 * \~english
				 * @brief Method of destroying a client event
				 * @param eid identifier of the event to be destroyed
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции события клиента
				 *
				 * \~english
				 * @brief Method of getting the options of a client event
				 * @param eid client event identifier
				 * @return    options of the client event
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций события клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции события клиента для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the options of a client event
				 * @param eid     client event identifier
				 * @param options options of the client event to be set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции события клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция события клиента для установки
				 * @param mode   режим установки опции события клиента
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting an option of a client event
				 * @param eid    client event identifier
				 * @param option option of the client event to be set
				 * @param mode   mode of setting the option of the client event
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 *
				 * \~english
				 * @brief Method of getting the metadata of the last received datagram packet
				 * @param eid client event identifier
				 * @return    metadata of the last received datagram packet
				 *
				 * \~
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    количество хопов последнего принятого пакета
				 *
				 * \~english
				 * @brief Method of getting the number of the hops of the last received packet
				 * @param eid client event identifier
				 * @return    number of the hops of the last received packet
				 *
				 * \~
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the number of the hops of the last received packet
				 * @param eid  client event identifier
				 * @param hops number of the hops of the last received packet
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события клиента
				 * @return    максимальное количество хопов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the hops through which a packet can pass
				 * @param eid client event identifier
				 * @return    maximum number of the hops
				 *
				 * \~
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the hops through which a packet can pass
				 * @param eid  client event identifier
				 * @param hops maximum number of the hops
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    сетевой интерфейс клиента
				 *
				 * \~english
				 * @brief Method of getting the network interface of the client
				 * @param eid client event identifier
				 * @return    network interface of the client
				 *
				 * \~
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса клиента
				 *
				 * @param eid  идентификатор события клиента
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network interface of the client
				 * @param eid  client event identifier
				 * @param name name of the network interface to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения внутреннего порта события
				 *
				 * @param eid идентификатор события
				 * @return    внутренний порт события
				 *
				 * \~english
				 * @brief Method of getting the internal port of the event
				 * @param eid event identifier
				 * @return    internal port of the event
				 *
				 * \~
				 */
				uint16_t getSourcePort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки внутреннего порта события
				 *
				 * @param eid  идентификатор события
				 * @param port внутренний порт события
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the internal port of the event
				 * @param eid  event identifier
				 * @param port internal port of the event
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setSourcePort(const event::id_t eid, const uint16_t port) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения порта удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    порт удалённого сервера
				 *
				 * \~english
				 * @brief Method of getting the port of the remote server
				 * @param eid client event identifier
				 * @return    port of the remote server
				 *
				 * \~
				 */
				uint16_t getTargetPort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки порта удалённого сервера
				 *
				 * @param eid  идентификатор события клиента
				 * @param port порт удалённого сервера для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the port of the remote server
				 * @param eid  client event identifier
				 * @param port port of the remote server to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setTargetPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid client event identifier
				 * @return    host address of the target machine
				 *
				 * \~
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    client event identifier
				 * @param target host address of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t eid, string_view target) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target структура сетевого адреса хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    client event identifier
				 * @param target structure of the network address of the host of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t eid, const net::addr_t * target) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid    client event identifier
				 * @param target object for extracting the host address of the target machine
				 * @return       result of extracting the host address of the target machine
				 *
				 * \~
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @return        значение адреса клиента
				 *
				 * \~english
				 * @brief Method of getting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @return        value of the client address
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   значение адреса клиента
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   value of the client address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   структура сетевого адреса клиента
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   structure of the network address of the client
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   объект для извлечения адреса клиента
				 * @return        результат выполнения извлечения адреса клиента
				 *
				 * \~english
				 * @brief Method of getting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   object for extracting the client address
				 * @return        result of extracting the client address
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @return    MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of getting the MTU of the network interface
				 * @param eid client event identifier
				 * @return    MTU of the network interface
				 *
				 * \~
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of setting the MTU of the network interface
				 * @param eid client event identifier
				 * @param mtu MTU size of the interface
				 * @return    result of setting the MTU of the network interface
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события клиента
				 * @return    текущий режим обнаружения MTU
				 *
				 * \~english
				 * @brief Method of getting the discovery mode of the maximum packet size (MTU)
				 * @param eid client event identifier
				 * @return    current MTU discovery mode
				 *
				 * \~
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события клиента
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the discovery mode of the maximum packet size (MTU)
				 * @param eid  client event identifier
				 * @param mode discovery mode of the maximum packet size (MTU)
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима трансляции пакетов клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 * \~english
				 * @brief Method of getting the packet delivery mode of the client
				 * @param eid client event identifier
				 * @return    packet delivery mode (unicast, multicast, broadcast)
				 *
				 * \~
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима трансляции пакетов клиента
				 *
				 * @param eid      идентификатор события клиента
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the packet delivery mode of the client
				 * @param eid      client event identifier
				 * @param delivery packet delivery mode (unicast, multicast, broadcast)
				 * @return         result of performing the setting
				 *
				 * \~
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       размер буфера клиента
				 *
				 * \~english
				 * @brief Method of getting the buffer size of the client
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @return       buffer size of the client
				 *
				 * \~
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @param size   размер буфера клиента
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the buffer size of the client
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @param size   buffer size of the client
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 *
				 * \~english
				 * @brief Method of getting the usage mode of the read timeout of the event
				 * @param eid event identifier
				 * @return    usage mode of the read timeout of the event
				 *
				 * \~
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 *
				 * \~english
				 * @brief Method of setting the usage mode of the read timeout of the event
				 * @param eid   event identifier
				 * @param usage usage mode of the read timeout of the event (reusable or disposable)
				 *
				 * \~
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of the client
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @return       value of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the timeout of the client
				 * @param eid     client event identifier
				 * @param action  action type of the client
				 * @param timeout value of the timeout in milliseconds
				 *
				 * \~
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки
				 * @return          результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of the client
				 * @param eid       client event identifier
				 * @param limiting  mode of limiting the bandwidth of the client (egress or ingress)
				 * @param bandwidth bandwidth of the client to be set
				 * @return          result of performing the setting
				 *
				 * \~
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки параметров keep-alive для клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the keep-alive parameters for the client
				 * @param eid   client event identifier
				 * @param cnt   number of the keep-alive packets
				 * @param idle  idle time before sending the first keep-alive packet in seconds
				 * @param intvl interval between the keep-alive packets in seconds
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    значение DSCP
				 *
				 * \~english
				 * @brief Method of getting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid client event identifier
				 * @return    DSCP value
				 *
				 * \~
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid  client event identifier
				 * @param dscp DSCP value
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    client event identifier
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    client event identifier
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод остановки клиента
				 *
				 * \~english
				 * @brief Method of stopping the client
				 *
				 * \~
				 */
				void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска клиента
				 *
				 * \~english
				 * @brief Method of launching the client
				 *
				 * \~
				 */
				void start() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод открытия потока приложения соединения
				 *
				 * @param mode режим однонаправленного потока
				 * @return     идентификатор открытого потока
				 *
				 * \~english
				 * @brief Method of opening an application stream of the connection
				 * @param mode mode of a unidirectional stream
				 * @return     identifier of the opened stream
				 *
				 * \~
				 */
				uint64_t open(const bool mode = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных в поток приложения соединения
				 *
				 * @param sid  идентификатор потока приложения
				 * @param data отправляемые данные
				 * @param fin  флаг завершения потока
				 * @return     результат постановки данных в очередь отправки
				 *
				 * \~english
				 * @brief Method of sending data into an application stream of the connection
				 * @param sid  identifier of the application stream
				 * @param data data being sent
				 * @param fin  flag of the termination of the stream
				 * @return     result of placing the data into the sending queue
				 *
				 * \~
				 */
				size_t send(const uint64_t sid, string_view data, const bool fin = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки водяных меток буфера отправки потоков соединения (backpressure)
				 *
				 * @note Верхняя метка ограничивает объём несобранных данных на поток: сверх неё send()
				 *       принимает данные лишь частично, а по опустошению буфера ниже нижней метки поток
				 *       сигнализируется колбэком "writable". Ноль снимает ограничение (буфер не ограничен)
				 *
				 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
				 * @param low  нижняя водяная метка (порог сигнала "writable")
				 *
				 * \~english
				 * @brief Method of setting the watermarks of the sending buffer of the streams of the connection (backpressure)
				 * @note The upper mark limits the volume of the unassembled data per stream: beyond it send()
				 *       accepts the data only partially, while upon the emptying of the buffer below the lower mark the stream
				 *       is signalled by the "writable" callback. Zero removes the limit (the buffer is not limited)
				 * @param high upper watermark (capacity of the sending buffer of the stream)
				 * @param low  lower watermark (threshold of the "writable" signal)
				 *
				 * \~
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
				/**
				 * \~russian
				 * @brief Метод назначения pull-источника данных потока (RFC 9000 §2.2)
				 *
				 * @note Альтернатива send() для больших тел: движок сам запрашивает данные у источника
				 *       по мере места в буфере отправки потока, не требуя держать копию всего тела
				 *
				 * @param sid    идентификатор потока приложения
				 * @param source pull-источник данных тела потока
				 *
				 * \~english
				 * @brief Method of assigning a pull source of the data of a stream (RFC 9000 §2.2)
				 * @note An alternative to send() for large bodies: the engine itself requests the data from the source
				 *       as the space in the sending buffer of the stream appears, without requiring to keep a copy of the whole body
				 * @param sid    identifier of the application stream
				 * @param source pull source of the data of the body of the stream
				 *
				 * \~
				 */
				void dataSource(const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки датаграммы приложения серверу (RFC 9221)
				 *
				 * @note Доставка датаграмм ненадёжна: потерянная датаграмма повторно
				 *       не отправляется, порядок доставки не гарантируется. Отправка
				 *       возможна только когда сервер анонсировал их приём
				 *
				 * @param data данные датаграммы приложения
				 * @return     результат отправки
				 *
				 * \~english
				 * @brief Method of sending an application datagram to the server (RFC 9221)
				 * @note The delivery of the datagrams is unreliable: a lost datagram is not
				 *       sent again, the order of the delivery is not guaranteed. The sending
				 *       is possible only when the server has announced their reception
				 * @param data data of the application datagram
				 * @return     result of the sending
				 *
				 * \~
				 */
				bool datagram(string_view data) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
				 *
				 * @return предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
				 *
				 * \~english
				 * @brief Method of getting the limit size of a datagram being sent (RFC 9221 §3)
				 * @return limit size of the data of a datagram in octets (0 — the datagrams are not supported)
				 *
				 * \~
				 */
				size_t datagrams() const noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
				 *
				 * @param code   код ошибки приложения
				 * @param reason человекочитаемая причина завершения
				 *
				 * \~english
				 * @brief Method of the termination of the connection by the application (RFC 9000 §10.2)
				 * @param code   error code of the application
				 * @param reason human-readable reason of the termination
				 *
				 * \~
				 */
				void close(const uint64_t code = 0, string_view reason = "") noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения согласованного ALPN-протокола соединения
				 *
				 * @return согласованный ALPN-протокол
				 *
				 * \~english
				 * @brief Method of getting the negotiated ALPN protocol of the connection
				 * @return negotiated ALPN protocol
				 *
				 * \~
				 */
				tls::coder_t::alpn_t alpn() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @note Поддерживаются функции обратного вызова: "open" на установленное
				 *       соединение, "read" на собранные данные потока приложения,
				 *       "datagram" на принятую датаграмму приложения и "close"
				 *       на завершённое соединение
				 *
				 * @param callback функции обратного вызова
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @note The following callback functions are supported: "open" on an established
				 *       connection, "read" on the assembled data of an application stream,
				 *       "datagram" on an accepted application datagram and "close"
				 *       on a terminated connection
				 * @param callback callback functions
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				QuicClient(const QuicClient &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 * @return current value of the object
				 *
				 * \~
				 */
				QuicClient & operator = (const QuicClient &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit QuicClient(const fmk_t * fmk, const log_t * log) noexcept;
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
				~QuicClient() noexcept;
		} quic_client_t;
	};
};

#endif // __AWH_UNIT_QUIC__
