/**
 * @file server.hpp
 * @date 2026-05-17
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
 * @brief Заголовочный файл фасада сервера — публичный API класса Server, объединяющего приём подключений, транспорт,
 *        TLS с поддержкой нескольких сертификатов, DNS-резолвинг, кластеризацию и подписку на события для TCP, UDP,
 *        SCTP, UDS, DTLS и QUIC
 *
 * \~english
 * @brief Header file of the server facade — the public API of the Server class uniting the acceptance of connections, the transport,
 *        TLS with support for several certificates, DNS resolving, clustering and event subscription for TCP, UDP,
 *        SCTP, UDS, DTLS and QUIC
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SERVER__
#define __AWH_SERVER__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../units/dns.hpp"
#include "../units/quic.hpp"
#include "../units/server.hpp"
#include "../cryptography/tls/coder.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
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
	 * @brief Класс сервера
	 *
	 * \~english
	 * @brief Server class
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Server {
		protected:
			/**
			 * \~russian
			 * @brief Структура для хранения параметров DNS-резолвера
			 *
			 * @details Хранит идентификатор DNS-резолвера, время жизни DNS-запроса и объект DNS-резолвера.
			 *
			 * \~english
			 * @brief Structure for storing the DNS resolver parameters
			 *
			 * @details Stores the DNS resolver identifier, the lifetime of a DNS request and the DNS resolver object.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Domain_Name_System {
				// Идентификатор DNS-резолвера
				unit::dns_t::id_t id;
				// Время жизни DNS запроса (в миллисекундах, по умолчанию 15 секунд)
				atomic_uint32_t alive;
				// Объект DNS-резолвера
				unit::dns_t * client;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Domain_Name_System() noexcept;
			} dns_t;
			/**
			 * \~russian
			 * @brief Структура идентификаторов клиента
			 *
			 * @details Хранит идентификатор сервера и идентификатор безопасности TLS.
			 *
			 * @warning Структура упакована, и ссылку на её поле связывать нельзя: выравнивание
			 *          у полей упакованной структуры не обеспечено, а ссылка его требует. GCC
			 *          отвечает на такое отказом «cannot bind packed field», clang же берёт
			 *          молча, отчего на macOS и системах BSD этого не видно вовсе, а на Solaris
			 *          и Linux сборка встаёт
			 *
			 * @note Передавать поля отсюда следует значением - через static_cast к их же типу.
			 *       Ссылку связывают не только явные её объявления: доводы шаблонов, взятые как
			 *       Args &&, тоже, и оттого отказ вылезал у вызовов записи в журнал
			 *
			 * \~english
			 * @brief Structure of the client identifiers
			 *
			 * @details Stores the server identifier and the TLS security identifier.
			 *
			 * @warning The structure is packed, and a reference to its field must not be bound: alignment
			 *          of the fields of a packed structure is not ensured, while a reference requires it. GCC
			 *          answers such with the refusal "cannot bind packed field", whereas clang takes it
			 *          silently, whereby on macOS and BSD systems this is not visible at all, while on Solaris
			 *          and Linux the build comes to a halt
			 *
			 * @note Fields from here should be passed by value - through a static_cast to their own type.
			 *       References are bound not only by their explicit declarations: template arguments taken as
			 *       Args && do so too, and that is why the refusal surfaced at the calls writing to the log
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Identifier {
				// Идентификатор клиента
				event::id_t eid;
				// Контекст шаблона безопасности TLS
				tls::coder_t::id_t cts;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Identifier() noexcept;
			} __attribute__((packed)) id_t;
			/**
			 * \~russian
			 * @brief Структура для хранения параметров транспортного уровня безопасности
			 *
			 * @details Хранит объект транспортного уровня безопасности и список для сопоставления идентификаторов клиентов с идентификаторами TLS.
			 *
			 * \~english
			 * @brief Structure for storing the transport layer security parameters
			 *
			 * @details Stores the transport layer security object and the list for matching client identifiers with TLS identifiers.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ TLS {
				// Объект транспортного уровня безопасности
				tls::coder_t * coder;
				// Список для сопоставления идентификаторов клиентов с идентификаторами TLS
				unordered_map <event::id_t, tls::coder_t::id_t> safety;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit TLS() noexcept;
			} tls_t;
			/**
			 * \~russian
			 * @brief Структура юнита сервера
			 *
			 * @details Хранит объект работы с сетевыми адресами и объект юнита сервера.
			 *
			 * \~english
			 * @brief Structure of the server unit
			 *
			 * @details Stores the object for working with network addresses and the server unit object.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Unit {
				// Объект работы с сетевыми адресами
				net_addr_t addr;
				// Объект юнита сервера (транспорты TCP/UDP/SCTP и прикладные протоколы поверх них)
				unit::server_t server;
				// Объект юнита сервера QUIC (выбирается при инициализации транспортом protocol_t::QUIC)
				unit::quic_server_t quic;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Unit(const fmk_t * fmk, const log_t * log) noexcept;
			} unit_t;
		protected:
			// Идентификатор сервера
			id_t _id;
		protected:
			// Объект DNS-резолвера
			dns_t _dns;
		protected:
			// Объект параметров TLS
			tls_t _tls;
		protected:
			// Адрес хоста целевой машины
			string _host;
		protected:
			// Функция обратного вызова для обработки сервера
			callback_t _callback;
		protected:
			// Объект юнита сервера
			unique_ptr <unit_t> _unit;
		protected:
			// Протокол транспорта сервера (выбирается при инициализации, определяет обработку данных)
			event::protocol_t _protocol;
		protected:
			// Тип сокета транспорта сервера (STREAM/DATAGRAM/SEQPACKET - определяет доступность датаграмм)
			event::type_t _type;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		protected:
			/**
			 * \~russian
			 * @brief Метод проверки рабочего состояния сервера
			 *
			 * @note Проверяет рабочее состояние активного юнита транспорта, выбираемого
			 *       по протоколу (QUIC - выделенный юнит, остальные транспорты - общий
			 *       юнит сервера)
			 *
			 * @return результат проверки рабочего состояния
			 *
			 * \~english
			 * @brief Method checking the working state of the server
			 *
			 * @note Checks the working state of the active transport unit selected by the protocol
			 *       (QUIC - the dedicated unit, the remaining transports - the common server unit)
			 *
			 * @return result of checking the working state
			 *
			 * \~
			 */
			bool active() const noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Методы диспетчеризации к активному юниту транспорта
			 *
			 * @note Транспорт выбирается по протоколу сервера: для protocol_t::QUIC
			 *       работает выделенный юнит сервера QUIC, для остальных транспортов -
			 *       общий юнит сервера. Событием прослушивания выступает _id.eid
			 *
			 * \~english
			 * @brief Methods of dispatching to the active transport unit
			 *
			 * @note The transport is selected by the server protocol: for protocol_t::QUIC the dedicated
			 *       QUIC server unit works, for the remaining transports - the common server unit. The
			 *       listening event is _id.eid
			 *
			 * \~
			 */
			bool commitUnit() noexcept;
			bool launchUnit() noexcept;
			bool listenUnit(const uint32_t max) noexcept;
			void startUnit() noexcept;
			void stopUnit() noexcept;
			event::family_t familyUnit() const noexcept;
			event::status_t statusUnit() const noexcept;
			string getAddressUnit(const event::address_t address) const noexcept;
			uint16_t getPortUnit() const noexcept;
			bool setAddressUnit(const event::address_t address, string_view value) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод изменения статуса сервера
			 *
			 * @param index  индекс обрабатываемого события
			 * @param status новый статус сервера
			 *
			 * \~english
			 * @brief Server status change method
			 *
			 * @param index  index of the event being processed
			 * @param status new server status
			 *
			 * \~
			 */
			virtual void status(const uint8_t index, const event::status_t status) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод обработки события разрешения подключения
			 *
			 * @param eid идентификатор сервера
			 * @param cid идентификатор клиента
			 *
			 * \~english
			 * @brief Method processing the event of permitting a connection
			 *
			 * @param eid server identifier
			 * @param cid client identifier
			 *
			 * \~
			 */
			virtual void accept(const event::id_t eid, const event::id_t cid) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки установленного соединения QUIC (RFC 9000)
			 *
			 * @note Транслируется приложению как принятие нового соединения; рукопожатие
			 *       TLS 1.3 ведёт само соединение QUIC, поэтому слой TLS-over-stream
			 *       обходится (RFC 9001)
			 *
			 * @param cid идентификатор сессии соединения
			 *
			 * \~english
			 * @brief Method processing an established QUIC connection (RFC 9000)
			 *
			 * @note Relayed to the application as the acceptance of a new connection; the TLS 1.3
			 *       handshake is conducted by the QUIC connection itself, therefore the TLS-over-stream
			 *       layer is bypassed (RFC 9001)
			 *
			 * @param cid connection session identifier
			 *
			 * \~
			 */
			virtual void opened(const event::id_t cid) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки собранных данных потока соединения QUIC
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param sid  идентификатор потока приложения
			 * @param data собранные данные потока
			 * @param fin  флаг завершения потока удалённым эндпоинтом
			 *
			 * \~english
			 * @brief Method processing the assembled data of a QUIC connection stream
			 *
			 * @param cid  connection session identifier
			 * @param sid  application stream identifier
			 * @param data assembled stream data
			 * @param fin  flag of the stream completion by the remote endpoint
			 *
			 * \~
			 */
			virtual void stream(const event::id_t cid, const uint64_t sid, const string & data, const bool fin) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки освобождения буфера отправки потока соединения QUIC (сигнал writable)
			 *
			 * @param cid идентификатор сессии соединения
			 * @param sid идентификатор потока приложения
			 *
			 * \~english
			 * @brief Method processing the release of the send buffer of a QUIC connection stream (the writable signal)
			 *
			 * @param cid connection session identifier
			 * @param sid application stream identifier
			 *
			 * \~
			 */
			virtual void writable(const event::id_t cid, const uint64_t sid) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки принятой датаграммы приложения QUIC (RFC 9221)
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param data данные принятой датаграммы
			 *
			 * \~english
			 * @brief Method processing a received QUIC application datagram (RFC 9221)
			 *
			 * @param cid  connection session identifier
			 * @param data data of the received datagram
			 *
			 * \~
			 */
			virtual void message(const event::id_t cid, const string & data) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки завершения соединения QUIC (RFC 9000 §10)
			 *
			 * @param cid   идентификатор сессии соединения
			 * @param error код ошибки завершения соединения
			 *
			 * \~english
			 * @brief Method processing the termination of a QUIC connection (RFC 9000 §10)
			 *
			 * @param cid   connection session identifier
			 * @param error error code of the connection termination
			 *
			 * \~
			 */
			virtual void closed(const event::id_t cid, const quic::error_t error) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
			 *
			 * @param eid  идентификатор события
			 * @param info информационные метаданные о дейтаграммном пакете
			 *
			 * \~english
			 * @brief Method processing the informational metadata about a datagram packet
			 *
			 * @param eid  event identifier
			 * @param info informational metadata about the datagram packet
			 *
			 * \~
			 */
			virtual void traffic(const event::id_t eid, const net::dgram_info_t & info) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки попыток подключения клиента к удалённому серверу
			 *
			 * @param domain   доменное имя для разрешения
			 * @param attempts количество попыток подключения
			 *
			 * \~english
			 * @brief Method processing the attempts of the client to connect to a remote server
			 *
			 * @param domain   domain name to resolve
			 * @param attempts number of connection attempts
			 *
			 * \~
			 */
			virtual void attempts(const unit::dns_t::id_t, const string & domain, const uint8_t attempts) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки неудачного разрешения доменного имени
			 *
			 * @param id     идентификатор DNS-запроса
			 * @param record тип записи DNS
			 * @param domain доменное имя
			 *
			 * \~english
			 * @brief Method processing an unsuccessful domain name resolution
			 *
			 * @param id     DNS request identifier
			 * @param record DNS record type
			 * @param domain domain name
			 *
			 * \~
			 */
			virtual void failure(const unit::dns_t::id_t id, const unit::dns_t::record_t record, const string & domain) noexcept;
			/**
			 * \~russian
			 * @brief Метод разрешения доменного имени удалённого хоста в сетевой адрес
			 *
			 * @param family семейство адресов (IPv4/IPv6)
			 * @param domain доменное имя для разрешения
			 * @param addr   указатель на структуру для хранения результата разрешения
			 *
			 * \~english
			 * @brief Method resolving the domain name of a remote host into a network address
			 *
			 * @param family address family (IPv4/IPv6)
			 * @param domain domain name to resolve
			 * @param addr   pointer to the structure for storing the resolution result
			 *
			 * \~
			 */
			virtual void resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод обработки событий записи данных клиентом
			 *
			 * @param eid  идентификатор клиента
			 * @param size размер данных для записи
			 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing the events of data being written by the client
			 *
			 * @param eid  client identifier
			 * @param size size of the data to write
			 * @param ctx  intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void write(const event::id_t eid, const size_t size, void * ctx) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий изменения состояния сервера
			 *
			 * @param eid    идентификатор клиента
			 * @param status новый статус сервера
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing the events of the server state changing
			 *
			 * @param eid    client identifier
			 * @param status new server status
			 * @param ctx    intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void state(const event::id_t eid, const event::status_t status, void * ctx) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки действий сервера
			 *
			 * @param eid    идентификатор клиента
			 * @param action действие сервера
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing the server actions
			 *
			 * @param eid    client identifier
			 * @param action server action
			 * @param ctx    intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void action(const event::id_t eid, const event::action_t action, void * ctx) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий получения данных сервером
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер данных сервера
			 * @param size   размер данных сервера
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing the events of data being received by the server
			 *
			 * @param eid    client identifier
			 * @param buffer server data buffer
			 * @param size   server data size
			 * @param ctx    intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void read(const event::id_t eid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки события ошибки
			 *
			 * @param eid     идентификатор события
			 * @param error   код ошибки
			 * @param message сообщение об ошибке
			 * @param ctx     промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing an error event
			 *
			 * @param eid     event identifier
			 * @param error   error code
			 * @param message error message
			 * @param ctx     intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void error(const event::id_t eid, const event::error_t error, const string & message, void * ctx) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing the events of availability/unavailability of the client outgoing data queue
			 *
			 * @param eid    client identifier
			 * @param status status of the queue availability
			 * @param size   size of the available data of the queue
			 * @param ctx    intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void available(const event::id_t eid, const event::status_t status, const size_t size, void * ctx) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий истечения таймаута клиента
			 *
			 * @param eid    идентификатор клиента
			 * @param action тип действия для истёкшего таймаута
			 * @param delay  задержка таймаута в миллисекундах
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 * @return       нужно ли завершить клиента после истечения таймаута
			 *
			 * \~english
			 * @brief Method processing the events of the client timeout expiring
			 *
			 * @param eid    client identifier
			 * @param action action type for the expired timeout
			 * @param delay  timeout delay in milliseconds
			 * @param ctx    intermediate context for passing into the callback function
			 * @return       whether the client should be terminated after the timeout expires
			 *
			 * \~
			 */
			virtual bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay, void * ctx) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки события невозможности отправки данных клиенту
			 *
			 * @param eid    идентификатор клиента
			 * @param error  тип ошибки отправки данных
			 * @param buffer данные, которые не получилось отправить
			 * @param size   размер данных, которые не получилось отправить
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing the event of the impossibility of sending data to the client
			 *
			 * @param eid    client identifier
			 * @param error  type of the data sending error
			 * @param buffer data that could not be sent
			 * @param size   size of the data that could not be sent
			 * @param ctx    intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод обработки события пересоздания процесса
			 *
			 * @param old старый идентификатор процесса
			 * @param pid текущий идентификатор процесса
			 *
			 * \~english
			 * @brief Method processing the event of a process being recreated
			 *
			 * @param old old process identifier
			 * @param pid current process identifier
			 *
			 * \~
			 */
			virtual void rebaseCluster(const pid_t old, const pid_t pid) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения события завершения работы процесса
			 *
			 * @param pid    идентификатор процесса
			 * @param status состояние, с которым завершился процесс
			 *
			 * \~english
			 * @brief Method obtaining the event of a process terminating its work
			 *
			 * @param pid    process identifier
			 * @param status state the process terminated with
			 *
			 * \~
			 */
			virtual void exitCluster(const pid_t pid, const int32_t status) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки события отправки сообщения процессу кластера
			 *
			 * @param pid  идентификатор процесса
			 * @param size размер отправленного сообщения
			 *
			 * \~english
			 * @brief Method processing the event of sending a message to a cluster process
			 *
			 * @param pid  process identifier
			 * @param size size of the sent message
			 *
			 * \~
			 */
			virtual void sendingCluster(const pid_t pid, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий изменения статуса кластера
			 *
			 * @param pid    идентификатор события
			 * @param status новый статус кластера
			 *
			 * \~english
			 * @brief Method processing the events of the cluster status changing
			 *
			 * @param pid    event identifier
			 * @param status new cluster status
			 *
			 * \~
			 */
			virtual void stateCluster(const pid_t pid, const event::status_t status) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий активации/деактивации кластера
			 *
			 * @param pid   идентификатор процесса
			 * @param event флаг события кластера
			 *
			 * \~english
			 * @brief Method processing the events of the cluster activation/deactivation
			 *
			 * @param pid   process identifier
			 * @param event cluster event flag
			 *
			 * \~
			 */
			virtual void eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки события получения сообщения от процесса кластера
			 *
			 * @param pid  идентификатор процесса
			 * @param data данные полученного сообщения
			 * @param size размер данных полученного сообщения
			 *
			 * \~english
			 * @brief Method processing the event of receiving a message from a cluster process
			 *
			 * @param pid  process identifier
			 * @param data data of the received message
			 * @param size size of the data of the received message
			 *
			 * \~
			 */
			virtual void messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
			 *
			 * @param pid    идентификатор процесса
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 *
			 * \~english
			 * @brief Method processing the event of availability/unavailability of the cluster outgoing message queue
			 *
			 * @param pid    process identifier
			 * @param status status of the queue availability
			 * @param size   size of the available data of the queue
			 *
			 * \~
			 */
			virtual void availableCluster(const pid_t pid, const event::status_t status, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий ошибок кластера
			 *
			 * @param pid         идентификатор процесса
			 * @param error       тип ошибки
			 * @param description описание ошибки
			 *
			 * \~english
			 * @brief Method processing the cluster error events
			 *
			 * @param pid         process identifier
			 * @param error       error type
			 * @param description error description
			 *
			 * \~
			 */
			virtual void errorCluster(const pid_t pid, const event::error_t error, const string & description) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод получения состояния TLS
			 *
			 * @param id    идентификатор TLS
			 * @param eid   идентификатор клиента
			 * @param state состояние TLS
			 *
			 * \~english
			 * @brief TLS state obtaining method
			 *
			 * @param id    TLS identifier
			 * @param eid   client identifier
			 * @param state TLS state
			 *
			 * \~
			 */
			virtual void stateTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::state_t state) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения отпечатка TLS
			 *
			 * @param id      идентификатор TLS
			 * @param eid     идентификатор клиента
			 * @param browser информация о браузере для отпечатка TLS
			 *
			 * \~english
			 * @brief TLS fingerprint obtaining method
			 *
			 * @param id      TLS identifier
			 * @param eid     client identifier
			 * @param browser browser information for the TLS fingerprint
			 *
			 * \~
			 */
			virtual void fingerprintTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::fgp_t::browser_t & browser) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки ошибок TLS
			 *
			 * @param id      идентификатор TLS
			 * @param eid     идентификатор клиента
			 * @param error   код ошибки TLS
			 * @param message сообщение об ошибке TLS
			 *
			 * \~english
			 * @brief TLS errors processing method
			 *
			 * @param id      TLS identifier
			 * @param eid     client identifier
			 * @param error   TLS error code
			 * @param message TLS error message
			 *
			 * \~
			 */
			virtual void errorTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::error_t error, const string & message) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий шифрования/дешифрования данных TLS
			 *
			 * @param id     идентификатор TLS
			 * @param eid    идентификатор клиента
			 * @param event  тип события TLS
			 * @param buffer буфер данных для события шифрования/дешифрования TLS
			 * @param size   размер данных для события шифрования/дешифрования TLS
			 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
			 *
			 * \~english
			 * @brief Method processing the events of TLS data encryption/decryption
			 *
			 * @param id     TLS identifier
			 * @param eid    client identifier
			 * @param event  TLS event type
			 * @param buffer data buffer for the TLS encryption/decryption event
			 * @param size   data size for the TLS encryption/decryption event
			 * @param ctx    intermediate context for passing into the callback function
			 *
			 * \~
			 */
			virtual void processTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки чёрного списка события
			 *
			 * @param eid идентификатор события
			 * @return    результат выполнения очистки
			 *
			 * \~english
			 * @brief Method clearing the blacklist of an event
			 *
			 * @param eid event identifier
			 * @return    result of performing the clearing
			 *
			 * \~
			 */
			bool clearBlacklist(const event::id_t eid) noexcept;
			/**
			 * \~russian
			 * @brief Метод очистки белого списка события
			 *
			 * @param eid идентификатор события
			 * @return    результат выполнения очистки
			 *
			 * \~english
			 * @brief Method clearing the whitelist of an event
			 *
			 * @param eid event identifier
			 * @return    result of performing the clearing
			 *
			 * \~
			 */
			bool clearWhitelist(const event::id_t eid) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод добавления адреса в чёрный список события
			 *
			 * @param eid   идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method adding an address to the blacklist of an event
			 *
			 * @param eid   event identifier
			 * @param value event address value
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool addToBlacklist(const event::id_t eid, string_view value) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления адреса в белый список события
			 *
			 * @param eid   идентификатор события
			 * @param value значение адреса события
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method adding an address to the whitelist of an event
			 *
			 * @param eid   event identifier
			 * @param value event address value
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			bool addToWhitelist(const event::id_t eid, string_view value) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод удаления адреса из чёрного списка события
			 *
			 * @param eid   идентификатор события
			 * @param value адрес для удаления из чёрного списка
			 * @return      результат выполнения удаления
			 *
			 * \~english
			 * @brief Method removing an address from the blacklist of an event
			 *
			 * @param eid   event identifier
			 * @param value address to remove from the blacklist
			 * @return      result of performing the removal
			 *
			 * \~
			 */
			bool removeFromBlacklist(const event::id_t eid, string_view value) noexcept;
			/**
			 * \~russian
			 * @brief Метод удаления адреса из белого списка события
			 *
			 * @param eid   идентификатор события
			 * @param value адрес для удаления из белого списка
			 * @return      результат выполнения удаления
			 *
			 * \~english
			 * @brief Method removing an address from the whitelist of an event
			 *
			 * @param eid   event identifier
			 * @param value address to remove from the whitelist
			 * @return      result of performing the removal
			 *
			 * \~
			 */
			bool removeFromWhitelist(const event::id_t eid, string_view value) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения чёрного списка события
			 *
			 * @param eid идентификатор события
			 * @return    чёрный список события
			 *
			 * \~english
			 * @brief Method obtaining the blacklist of an event
			 *
			 * @param eid event identifier
			 * @return    blacklist of the event
			 *
			 * \~
			 */
			const unordered_map <string, event::address_t> & getFromBlacklist(const event::id_t eid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения белого списка события
			 *
			 * @param eid идентификатор события
			 * @return    белый список события
			 *
			 * \~english
			 * @brief Method obtaining the whitelist of an event
			 *
			 * @param eid event identifier
			 * @return    whitelist of the event
			 *
			 * \~
			 */
			const unordered_map <string, event::address_t> & getFromWhitelist(const event::id_t eid) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод остановки сервера
			 *
			 * \~english
			 * @brief Server stopping method
			 *
			 * \~
			 */
			virtual void stop() noexcept;
			/**
			 * \~russian
			 * @brief Метод запуска сервера
			 *
			 * \~english
			 * @brief Server starting method
			 *
			 * \~
			 */
			virtual void start() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод приостановки работы клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат выполнения приостановки работы
			 *
			 * \~english
			 * @brief Client operation suspension method
			 *
			 * @param eid client event identifier
			 * @return    result of performing the operation suspension
			 *
			 * \~
			 */
			virtual bool pause(const event::id_t eid) noexcept;
			/**
			 * \~russian
			 * @brief Метод возобновления работы клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат выполнения возобновления работы
			 *
			 * \~english
			 * @brief Client operation resumption method
			 *
			 * @param eid client event identifier
			 * @return    result of performing the operation resumption
			 *
			 * \~
			 */
			virtual bool resume(const event::id_t eid) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод уничтожения события клиента или сервера
			 *
			 * @param eid идентификатор события клиента для уничтожения
			 *
			 * \~english
			 * @brief Method destroying a client or server event
			 *
			 * @param eid identifier of the client event to destroy
			 *
			 * \~
			 */
			virtual void destroy(const event::id_t eid) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки, жив ли клиент или сервер
			 *
			 * @param eid идентификатор события клиента для проверки
			 * @return    результат проверки
			 *
			 * \~english
			 * @brief Method checking whether the client or the server is alive
			 *
			 * @param eid identifier of the client event to check
			 * @return    check result
			 *
			 * \~
			 */
			virtual bool isAlive(const event::id_t eid) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки промежуточного контекста события подключённого клиента
			 *
			 * @param eid идентификатор события сервера
			 * @param ctx указатель на контекст события
			 * @return    результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the intermediate context of the event of a connected client
			 *
			 * @param eid server event identifier
			 * @param ctx pointer to the event context
			 * @return    result of performing the setting
			 *
			 * \~
			 */
			virtual bool setContext(const event::id_t eid, void * ctx) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод перевода события в режим прослушивания входящих соединений
			 *
			 * @param max максимальное количество входящих соединений
			 * @return    результат выполнения перевода в режим прослушивания
			 *
			 * \~english
			 * @brief Method switching an event into the mode of listening for incoming connections
			 *
			 * @param max maximum number of incoming connections
			 * @return    result of performing the switch into the listening mode
			 *
			 * \~
			 */
			virtual bool listen(const uint16_t max) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 *
			 * \~english
			 * @brief Callback functions setting method
			 *
			 * @param callback callback functions
			 *
			 * \~
			 */
			virtual void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения данных от клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    результат получения данных
			 *
			 * \~english
			 * @brief Method of receiving data from the client
			 *
			 * @param eid client event identifier
			 * @return    data receiving result
			 *
			 * \~
			 */
			virtual bool recv(const event::id_t eid) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки данных клиенту
			 *
			 * @param eid    идентификатор события клиента
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных клиенту
			 *
			 * \~english
			 * @brief Method of sending data to the client
			 *
			 * @param eid    client event identifier
			 * @param buffer data buffer to send
			 * @param size   size of the data to send
			 * @return       number of data bytes sent to the client
			 *
			 * \~
			 */
			virtual size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод открытия потока приложения соединения QUIC
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param mode режим однонаправленного потока
			 * @return     идентификатор открытого потока
			 *
			 * \~english
			 * @brief Method opening an application stream of a QUIC connection
			 *
			 * @param cid  connection session identifier
			 * @param mode unidirectional stream mode
			 * @return     identifier of the opened stream
			 *
			 * \~
			 */
			virtual uint64_t open(const event::id_t cid, const bool mode = false) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки данных в поток приложения соединения QUIC
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param sid    идентификатор потока приложения
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @param fin    флаг завершения потока
			 * @return       количество байт данных, поставленных в очередь отправки
			 *
			 * \~english
			 * @brief Method sending data into an application stream of a QUIC connection
			 *
			 * @param cid    connection session identifier
			 * @param sid    application stream identifier
			 * @param buffer data buffer to send
			 * @param size   size of the data to send
			 * @param fin    stream completion flag
			 * @return       number of data bytes queued for sending
			 *
			 * \~
			 */
			virtual size_t send(const event::id_t cid, const uint64_t sid, const void * buffer, const size_t size, const bool fin = false) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки водяных меток буфера отправки потоков сессии соединения QUIC (backpressure)
			 *
			 * @note Верхняя метка ограничивает несобранный буфер потока (send() принимает частично),
			 *       по опустошению ниже нижней метки поток сигнализируется колбэком "writable". Ноль - снято
			 *
			 * @param cid  идентификатор сессии соединения
			 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
			 * @param low  нижняя водяная метка (порог сигнала "writable")
			 *
			 * \~english
			 * @brief Method setting the water marks of the send buffer of the streams of a QUIC connection session (backpressure)
			 *
			 * @note The high mark limits the unassembled buffer of the stream (send() accepts partially),
			 *       upon emptying below the low mark the stream is signalled by the "writable" callback. Zero - lifted
			 *
			 * @param cid  connection session identifier
			 * @param high high water mark (capacity of the stream send buffer)
			 * @param low  low water mark (threshold of the "writable" signal)
			 *
			 * \~
			 */
			virtual void sendWaterMarks(const event::id_t cid, const size_t high, const size_t low) noexcept;
			/**
			 * \~russian
			 * @brief Метод назначения pull-источника данных потока сессии соединения QUIC (RFC 9000 §2.2)
			 *
			 * @note Альтернатива send() для больших тел: движок сам тянет данные у источника по мере места
			 *       в буфере отправки, не требуя держать копию тела
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param sid    идентификатор потока приложения
			 * @param source pull-источник данных тела потока
			 *
			 * \~english
			 * @brief Method assigning a pull source of the data of a stream of a QUIC connection session (RFC 9000 §2.2)
			 *
			 * @note An alternative to send() for large bodies: the engine itself pulls the data from the source as room
			 *       appears in the send buffer, without requiring a copy of the body to be held
			 *
			 * @param cid    connection session identifier
			 * @param sid    application stream identifier
			 * @param source pull source of the stream body data
			 *
			 * \~
			 */
			virtual void dataSource(const event::id_t cid, const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки датаграммы приложения соединению QUIC (RFC 9221)
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param buffer буфер данных датаграммы для отправки
			 * @param size   размер данных датаграммы для отправки
			 * @return       результат отправки
			 *
			 * \~english
			 * @brief Method sending an application datagram to a QUIC connection (RFC 9221)
			 *
			 * @param cid    connection session identifier
			 * @param buffer buffer of the datagram data to send
			 * @param size   size of the datagram data to send
			 * @return       sending result
			 *
			 * \~
			 */
			virtual bool datagram(const event::id_t cid, const void * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения предельного размера отправляемой датаграммы QUIC (RFC 9221 §3)
			 *
			 * @param cid идентификатор сессии соединения
			 * @return    предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
			 *
			 * \~english
			 * @brief Method obtaining the limiting size of a QUIC datagram being sent (RFC 9221 §3)
			 *
			 * @param cid connection session identifier
			 * @return    limiting size of the datagram data in octets (0 - datagrams are not supported)
			 *
			 * \~
			 */
			virtual size_t datagrams(const event::id_t cid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод завершения соединения QUIC приложением (RFC 9000 §10.2)
			 *
			 * @param cid    идентификатор сессии соединения
			 * @param code   код ошибки приложения
			 * @param reason человекочитаемая причина завершения
			 *
			 * \~english
			 * @brief Method terminating a QUIC connection by the application (RFC 9000 §10.2)
			 *
			 * @param cid    connection session identifier
			 * @param code   application error code
			 * @param reason human-readable reason of the termination
			 *
			 * \~
			 */
			virtual void close(const event::id_t cid, const uint64_t code = 0, string_view reason = "") noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки локальных транспортных параметров соединений QUIC (RFC 9000 §7.4)
			 *
			 * @note Применяется только к транспорту QUIC. Задаётся до запуска сервера
			 *
			 * @param params локальные транспортные параметры
			 *
			 * \~english
			 * @brief Method setting the local transport parameters of QUIC connections (RFC 9000 §7.4)
			 *
			 * @note Applies only to the QUIC transport. Is set before the server is started
			 *
			 * @param params local transport parameters
			 *
			 * \~
			 */
			virtual void params(const quic::params::params_t & params) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки проверки адреса клиента через пакет Retry QUIC (RFC 9000 §8.1.2)
			 *
			 * @note Применяется только к транспорту QUIC. Задаётся до запуска сервера
			 *
			 * @param mode режим проверки адреса клиента
			 *
			 * \~english
			 * @brief Method setting the validation of the client address through a QUIC Retry packet (RFC 9000 §8.1.2)
			 *
			 * @note Applies only to the QUIC transport. Is set before the server is started
			 *
			 * @param mode client address validation mode
			 *
			 * \~
			 */
			virtual void retry(const bool mode) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки уведомления о перегрузке пути QUIC (RFC 9000 §13.4)
			 *
			 * @note Применяется только к транспорту QUIC. Задаётся до запуска сервера
			 *
			 * @param mode режим уведомления о перегрузке пути
			 *
			 * \~english
			 * @brief Method setting the QUIC path congestion notification (RFC 9000 §13.4)
			 *
			 * @note Applies only to the QUIC transport. Is set before the server is started
			 *
			 * @param mode path congestion notification mode
			 *
			 * \~
			 */
			virtual void ecn(const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод объединения данных между сервером и другим событием
			 *
			 * @param eid  идентификатор события-источника
			 * @param dest идентификатор события-приёмника
			 * @return     результат выполнения объединения
			 *
			 * \~english
			 * @brief Method of splicing data between the server and another event
			 *
			 * @param eid  identifier of the source event
			 * @param dest identifier of the destination event
			 * @return     result of performing the splicing
			 *
			 * \~
			 */
			virtual bool splice(const event::id_t eid, const event::id_t dest) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения опций сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    опции сервера или клиента
			 *
			 * \~english
			 * @brief Method obtaining the options of the server or of a client
			 *
			 * @param eid identifier of the server or client event
			 * @return    options of the server or of the client
			 *
			 * \~
			 */
			virtual uint16_t getOptions(const event::id_t eid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки опций сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param options опции сервера или клиента для установки
			 * @return        результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the options of the server or of a client
			 *
			 * @param eid     identifier of the server or client event
			 * @param options options of the server or of the client to set
			 * @return        result of performing the setting
			 *
			 * \~
			 */
			virtual bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки опции сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param option опция сервера или клиента для установки
			 * @param mode   режим установки опции сервера или клиента
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting an option of the server or of a client
			 *
			 * @param eid    identifier of the server or client event
			 * @param option option of the server or of the client to set
			 * @param mode   mode of setting the option of the server or of the client
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
			 *
			 * @return метаданные последнего принятого дейтаграммного пакета
			 *
			 * \~english
			 * @brief Method obtaining the metadata of the last received datagram packet
			 *
			 * @return metadata of the last received datagram packet
			 *
			 * \~
			 */
			virtual net::dgram_info_t getTrafficInfo() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения количества хопов последнего принятого пакета
			 *
			 * @return количество хопов последнего принятого пакета
			 *
			 * \~english
			 * @brief Method obtaining the number of hops of the last received packet
			 *
			 * @return number of hops of the last received packet
			 *
			 * \~
			 */
			virtual uint8_t getCountHops() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки количества хопов последнего принятого пакета
			 *
			 * @param hops количество хопов последнего принятого пакета
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the number of hops of the last received packet
			 *
			 * @param hops number of hops of the last received packet
			 * @return     result of performing the setting
			 *
			 * \~
			 */
			virtual bool setCountHops(const uint8_t hops) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
			 *
			 * @param eid идентификатор события сервера
			 * @return    максимальное количество хопов
			 *
			 * \~english
			 * @brief Method obtaining the maximum number of hops a packet can pass through
			 *
			 * @param eid server event identifier
			 * @return    maximum number of hops
			 *
			 * \~
			 */
			virtual event::hops_t getHops(const event::id_t eid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param eid  идентификатор события сервера
			 * @param hops максимальное количество хопов
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Method setting the maximum number of hops a packet can pass through
			 *
			 * @param eid  server event identifier
			 * @param hops maximum number of hops
			 * @return     result of the function work
			 *
			 * \~
			 */
			virtual bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения сетевого интерфейса сервера
			 *
			 * @return сетевой интерфейс сервера
			 *
			 * \~english
			 * @brief Method obtaining the server network interface
			 *
			 * @return server network interface
			 *
			 * \~
			 */
			virtual string getIface() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки сетевого интерфейса сервера
			 *
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the server network interface
			 *
			 * @param name name of the network interface to set
			 * @return     result of performing the setting
			 *
			 * \~
			 */
			virtual bool setIface(string_view name) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения порта сервера
			 *
			 * @return порт сервера
			 *
			 * \~english
			 * @brief Method obtaining the server port
			 *
			 * @return server port
			 *
			 * \~
			 */
			virtual uint16_t getPort() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки порта сервера
			 *
			 * @param port порт сервера для установки
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the server port
			 *
			 * @param port server port to set
			 * @return     result of performing the setting
			 *
			 * \~
			 */
			virtual bool setPort(const uint16_t port) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения порта подключённого клиента
			 *
			 * @param eid идентификатор события клиента
			 * @return    порт подключённого клиента
			 *
			 * \~english
			 * @brief Method obtaining the port of a connected client
			 *
			 * @param eid client event identifier
			 * @return    port of the connected client
			 *
			 * \~
			 */
			virtual uint16_t getPort(const event::id_t eid) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения адреса хоста текущей машины
			 *
			 * @return адрес хоста текущей машины
			 *
			 * \~english
			 * @brief Method obtaining the host address of the current machine
			 *
			 * @return host address of the current machine
			 *
			 * \~
			 */
			virtual const string & getHost() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки адреса хоста текущей машины
			 *
			 * @param host адрес хоста текущей машины
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the host address of the current machine
			 *
			 * @param host host address of the current machine
			 * @return     result of performing the setting
			 *
			 * \~
			 */
			virtual bool setHost(string_view host) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @return        значение адреса сервера
			 *
			 * \~english
			 * @brief Server address obtaining method
			 *
			 * @param address server address type
			 * @return        server address value
			 *
			 * \~
			 */
			virtual string getAddress(const event::address_t address) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   значение адреса сервера
			 * @return        результат выполнения установки
			 *
			 * \~english
			 * @brief Server address setting method
			 *
			 * @param address server address type
			 * @param value   server address value
			 * @return        result of performing the setting
			 *
			 * \~
			 */
			virtual bool setAddress(const event::address_t address, string_view value) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения адреса сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param address тип адреса сервера или клиента
			 * @return        значение адреса сервера или клиента
			 *
			 * \~english
			 * @brief Method obtaining the address of the server or of a client
			 *
			 * @param eid     identifier of the server or client event
			 * @param address type of the server or client address
			 * @return        value of the server or client address
			 *
			 * \~
			 */
			virtual string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   значение адреса сервера
			 * @return        результат выполнения установки
			 *
			 * \~english
			 * @brief Server address setting method
			 *
			 * @param address server address type
			 * @param value   server address value
			 * @return        result of performing the setting
			 *
			 * \~
			 */
			virtual bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения адреса сервера
			 *
			 * @param address тип адреса сервера
			 * @param value   объект для извлечения адреса сервера
			 * @return        результат выполнения извлечения адреса сервера
			 *
			 * \~english
			 * @brief Server address obtaining method
			 *
			 * @param address server address type
			 * @param value   object for extracting the server address
			 * @return        result of performing the extraction of the server address
			 *
			 * \~
			 */
			virtual bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения адреса сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param address тип адреса сервера или клиента
			 * @param value   объект для извлечения адреса сервера или клиента
			 * @return        результат выполнения извлечения адреса сервера или клиента
			 *
			 * \~english
			 * @brief Method obtaining the address of the server or of a client
			 *
			 * @param eid     identifier of the server or client event
			 * @param address type of the server or client address
			 * @param value   object for extracting the address of the server or client
			 * @return        result of performing the extraction of the address of the server or client
			 *
			 * \~
			 */
			virtual bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения MTU сетевого интерфейса
			 *
			 * @param eid идентификатор события сервера
			 * @return    MTU сетевого интерфейса
			 *
			 * \~english
			 * @brief Method obtaining the MTU of the network interface
			 *
			 * @param eid server event identifier
			 * @return    MTU of the network interface
			 *
			 * \~
			 */
			virtual uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки MTU сетевого интерфейса
			 *
			 * @param eid идентификатор события сервера
			 * @param mtu размер MTU интерфейса
			 * @return    результат установки MTU сетевого интерфейса
			 *
			 * \~english
			 * @brief Method setting the MTU of the network interface
			 *
			 * @param eid server event identifier
			 * @param mtu MTU size of the interface
			 * @return    result of setting the MTU of the network interface
			 *
			 * \~
			 */
			virtual bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения режима трансляции пакетов сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
			 *
			 * \~english
			 * @brief Method obtaining the packet delivery mode of the server or of a client
			 *
			 * @param eid identifier of the server or client event
			 * @return    packet delivery mode (unicast, multicast, broadcast)
			 *
			 * \~
			 */
			virtual event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки режима трансляции пакетов сервера или клиента
			 *
			 * @param eid      идентификатор события сервера или клиента
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return         результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the packet delivery mode of the server or of a client
			 *
			 * @param eid      identifier of the server or client event
			 * @param delivery packet delivery mode (unicast, multicast, broadcast)
			 * @return         result of performing the setting
			 *
			 * \~
			 */
			virtual bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения размера буфера сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @return       размер буфера сервера или клиента
			 *
			 * \~english
			 * @brief Method obtaining the buffer size of the server or of a client
			 *
			 * @param eid    identifier of the server or client event
			 * @param action action type of the server or of the client
			 * @return       buffer size of the server or of the client
			 *
			 * \~
			 */
			virtual size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки размера буфера сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @param size   размер буфера сервера или клиента
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the buffer size of the server or of a client
			 *
			 * @param eid    identifier of the server or client event
			 * @param action action type of the server or of the client
			 * @param size   buffer size of the server or of the client
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения времени жизни DNS запроса
			 *
			 * @return время жизни DNS запроса в миллисекундах
			 *
			 * \~english
			 * @brief Method obtaining the lifetime of a DNS request
			 *
			 * @return lifetime of a DNS request in milliseconds
			 *
			 * \~
			 */
			virtual uint32_t getAliveDNS() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки времени жизни DNS запроса
			 *
			 * @param alive время жизни DNS запроса в миллисекундах
			 *
			 * \~english
			 * @brief Method setting the lifetime of a DNS request
			 *
			 * @param alive lifetime of a DNS request in milliseconds
			 *
			 * \~
			 */
			virtual void setAliveDNS(const uint32_t alive) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения режима использования таймаута на чтение события
			 *
			 * @return режим использования таймаута на чтение события
			 *
			 * \~english
			 * @brief Method obtaining the usage mode of the event read timeout
			 *
			 * @return usage mode of the event read timeout
			 *
			 * \~
			 */
			virtual event::usage_t getUsageReadTimeout() const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения режима использования таймаута на чтение события сервера или клиента
			 *
			 * @param eid идентификатор события сервера или клиента
			 * @return    режим использования таймаута на чтение события сервера или клиента
			 *
			 * \~english
			 * @brief Method obtaining the usage mode of the read timeout of the server or client event
			 *
			 * @param eid identifier of the server or client event
			 * @return    usage mode of the read timeout of the server or client event
			 *
			 * \~
			 */
			virtual event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки режима использования таймаута на чтение события
			 *
			 * @param usage режим использования таймаута на чтение события (reusable или disposable)
			 *
			 * \~english
			 * @brief Method setting the usage mode of the event read timeout
			 *
			 * @param usage usage mode of the event read timeout (reusable or disposable)
			 *
			 * \~
			 */
			virtual void setUsageReadTimeout(const event::usage_t usage) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки режима использования таймаута на чтение события сервера или клиента
			 *
			 * @param eid   идентификатор события сервера или клиента
			 * @param usage режим использования таймаута на чтение события сервера или клиента (reusable или disposable)
			 *
			 * \~english
			 * @brief Method setting the usage mode of the read timeout of the server or client event
			 *
			 * @param eid   identifier of the server or client event
			 * @param usage usage mode of the read timeout of the server or client event (reusable or disposable)
			 *
			 * \~
			 */
			virtual void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения таймаута сервера
			 *
			 * @param action тип действия сервера
			 * @return       значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Server timeout obtaining method
			 *
			 * @param action server action type
			 * @return       timeout value in milliseconds
			 *
			 * \~
			 */
			virtual uint32_t getTimeout(const event::action_t action) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения таймаута сервера или клиента
			 *
			 * @param eid    идентификатор события сервера или клиента
			 * @param action тип действия сервера или клиента
			 * @return       значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Method obtaining the timeout of the server or of a client
			 *
			 * @param eid    identifier of the server or client event
			 * @param action action type of the server or of the client
			 * @return       timeout value in milliseconds
			 *
			 * \~
			 */
			virtual uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки таймаута сервера
			 *
			 * @param action  тип действия сервера
			 * @param timeout значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Server timeout setting method
			 *
			 * @param action  server action type
			 * @param timeout timeout value in milliseconds
			 *
			 * \~
			 */
			virtual void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки таймаута сервера или клиента
			 *
			 * @param eid     идентификатор события сервера или клиента
			 * @param action  тип действия сервера или клиента
			 * @param timeout значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Method setting the timeout of the server or of a client
			 *
			 * @param eid     identifier of the server or client event
			 * @param action  action type of the server or of the client
			 * @param timeout timeout value in milliseconds
			 *
			 * \~
			 */
			virtual void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки пропускной способности сервера
			 *
			 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
			 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 *
			 * \~english
			 * @brief Server bandwidth setting method
			 *
			 * @param limiting  server bandwidth limiting mode (egress or ingress)
			 * @param bandwidth server bandwidth to set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
			 * @return          result of performing the setting
			 *
			 * \~
			 */
			virtual bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки пропускной способности сервера или клиента
			 *
			 * @param eid       идентификатор события сервера или клиента
			 * @param limiting  режим ограничения пропускной способности сервера или клиента (egress или ingress)
			 * @param bandwidth пропускная способность сервера или клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the bandwidth of the server or of a client
			 *
			 * @param eid       identifier of the server or client event
			 * @param limiting  bandwidth limiting mode of the server or of the client (egress or ingress)
			 * @param bandwidth bandwidth of the server or of the client to set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
			 * @return          result of performing the setting
			 *
			 * \~
			 */
			virtual bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки параметров keep-alive для сервера или клиента
			 *
			 * @param eid   идентификатор события сервера или клиента
			 * @param cnt   количество пакетов keep-alive
			 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
			 * @param intvl интервал между пакетами keep-alive в секундах
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the keep-alive parameters for the server or for a client
			 *
			 * @param eid   identifier of the server or client event
			 * @param cnt   number of keep-alive packets
			 * @param idle  idle time before sending the first keep-alive packet in seconds
			 * @param intvl interval between keep-alive packets in seconds
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			virtual bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @return значение DSCP
			 *
			 * \~english
			 * @brief Method obtaining the value of the Differentiated Services Code Point (DSCP) field in the IP packet header
			 *
			 * @return DSCP value
			 *
			 * \~
			 */
			virtual event::dscp_t getDifferentiatedServicesCodePoint() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
			 *
			 * @param dscp значение DSCP
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Method setting the value of the Differentiated Services Code Point (DSCP) field in the IP packet header
			 *
			 * @param dscp DSCP value
			 * @return     result of the function work
			 *
			 * \~
			 */
			virtual bool setDifferentiatedServicesCodePoint(const event::dscp_t dscp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения обнаружения максимального размера пакета (MTU)
			 *
			 * @return режим обнаружения максимального размера пакета (MTU)
			 *
			 * \~english
			 * @brief Method obtaining the maximum transmission unit (MTU) discovery
			 *
			 * @return maximum transmission unit (MTU) discovery mode
			 *
			 * \~
			 */
			virtual event::mtu_discover_t getMaximumTransmissionUnitDiscover() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки обнаружения максимального размера пакета (MTU)
			 *
			 * @param mode режим обнаружения максимального размера пакета (MTU)
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Method setting the maximum transmission unit (MTU) discovery
			 *
			 * @param mode maximum transmission unit (MTU) discovery mode
			 * @return     result of the function work
			 *
			 * \~
			 */
			virtual bool setMaximumTransmissionUnitDiscover(const event::mtu_discover_t mode) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод активации/деактивации мультикаст группы
			 *
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @param port   порт мультикаст-группы с которого выполняется подписка
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Method of activating/deactivating a multicast group
			 *
			 * @param mode   activation/deactivation mode
			 * @param group  multicast group to activate/deactivate
			 * @param source address of the network interface the subscription is performed from
			 * @param port   port of the multicast group the subscription is performed from
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool membership(const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
			/**
			 * \~russian
			 * @brief Метод активации/деактивации мультикаст группы
			 *
			 * @param mode   режим активации/деактивации
			 * @param group  мультикаст-группа для активации/деактивации
			 * @param source адрес сетевого интерфейса с которого выполняется подписка
			 * @param port   порт мультикаст-группы с которого выполняется подписка
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Method of activating/deactivating a multicast group
			 *
			 * @param mode   activation/deactivation mode
			 * @param group  multicast group to activate/deactivate
			 * @param source address of the network interface the subscription is performed from
			 * @param port   port of the multicast group the subscription is performed from
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool membership(const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод инициализации сервера
			 *
			 * @param family   семейство адресов
			 * @param type     тип события
			 * @param protocol протокол события
			 * @return         идентификатор созданного сервера
			 *
			 * \~english
			 * @brief Server initialization method
			 *
			 * @param family   address family
			 * @param type     event type
			 * @param protocol event protocol
			 * @return         identifier of the created server
			 *
			 * \~
			 */
			virtual event::id_t init(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки названия кластера
			 *
			 * @param name название кластера для установки
			 *
			 * \~english
			 * @brief Cluster name setting method
			 *
			 * @param name cluster name to set
			 *
			 * \~
			 */
			virtual void clusterName(string_view name) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения семейства кластера
			 *
			 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
			 *
			 * \~english
			 * @brief Cluster family obtaining method
			 *
			 * @return family the cluster belongs to (MASTER or CHILDREN)
			 *
			 * \~
			 */
			virtual unit::cluster_t::family_t clusterFamily() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения режима активации кластера
			 *
			 * @return режим активации кластера
			 *
			 * \~english
			 * @brief Method obtaining the cluster activation mode
			 *
			 * @return cluster activation mode
			 *
			 * \~
			 */
			virtual event::mode_t clusterMode() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки количества процессов кластера
			 *
			 * @param mode флаг активации/деактивации кластера
			 * @param size количество рабочих процессов
			 *
			 * \~english
			 * @brief Method setting the number of cluster processes
			 *
			 * @param mode flag of the cluster activation/deactivation
			 * @param size number of worker processes
			 *
			 * \~
			 */
			virtual void clusterMode(const event::mode_t mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения максимального количества процессов
			 *
			 * @return максимальное количество процессов
			 *
			 * \~english
			 * @brief Method obtaining the maximum number of processes
			 *
			 * @return maximum number of processes
			 *
			 * \~
			 */
			virtual uint16_t clusterCount() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки максимального количества процессов
			 *
			 * @param count максимальное количество процессов
			 *
			 * \~english
			 * @brief Method setting the maximum number of processes
			 *
			 * @param count maximum number of processes
			 *
			 * \~
			 */
			virtual void clusterCount(const uint16_t count) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения списка дочерних процессов
			 *
			 * @return список дочерних процессов
			 *
			 * \~english
			 * @brief Method obtaining the list of child processes
			 *
			 * @return list of child processes
			 *
			 * \~
			 */
			virtual unordered_set <pid_t> clusterWorkers() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки диапазона портов для выделения дочерним процессам кластера
			 *
			 * @note Применяется только к транспорту QUIC: родительский процесс раздаёт
			 *       порты из диапазона дочерним процессам, каждый из которых поднимает
			 *       собственный сокет сервера. На Linux/FreeBSD порт может повторяться
			 *       (процессы делят его через SO_REUSEPORT), на прочих системах порт
			 *       выделяется дочернему процессу монопольно. Пустой диапазон означает
			 *       использование единственного порта прослушивания
			 *
			 * @param begin начальный порт диапазона (0 - использовать порт прослушивания)
			 * @param end   конечный порт диапазона (0 - использовать порт прослушивания)
			 *
			 * \~english
			 * @brief Method setting the range of ports for allotting to the child processes of the cluster
			 *
			 * @note Applies only to the QUIC transport: the parent process hands out ports from the
			 *       range to the child processes, each of which raises its own server socket. On
			 *       Linux/FreeBSD a port may repeat (the processes share it through SO_REUSEPORT),
			 *       on the other systems a port is allotted to a child process exclusively. An empty
			 *       range means using the single listening port
			 *
			 * @param begin starting port of the range (0 - use the listening port)
			 * @param end   ending port of the range (0 - use the listening port)
			 *
			 * \~
			 */
			virtual void clusterRange(const uint16_t begin, const uint16_t end) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения списка дочерних процессов, не получивших порт прослушивания
			 *
			 * @note Актуально только для транспорта QUIC на системах, где порт выделяется
			 *       дочернему процессу монопольно (macOS/Solaris/OpenBSD/NetBSD): при нехватке
			 *       портов диапазона часть дочерних процессов остаётся без сокета сервера.
			 *       Их порт можно доотправить вручную методом clusterAssign()
			 *
			 * @return список идентификаторов дочерних процессов, работающих в холостую
			 *
			 * \~english
			 * @brief Method obtaining the list of child processes that did not receive a listening port
			 *
			 * @note Relevant only for the QUIC transport on systems where a port is allotted to a child
			 *       process exclusively (macOS/Solaris/OpenBSD/NetBSD): when the ports of the range run
			 *       short, some of the child processes are left without a server socket.
			 *       Their port can be sent to them manually by the clusterAssign() method
			 *
			 * @return list of identifiers of the child processes running idle
			 *
			 * \~
			 */
			virtual unordered_set <pid_t> clusterIdle() const noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки порта прослушивания конкретному дочернему процессу кластера
			 *
			 * @note Применяется только к транспорту QUIC: позволяет вручную поднять сервер
			 *       на дочернем процессе, которому при автоматической раздаче порт не достался
			 *
			 * @param pid  идентификатор дочернего процесса
			 * @param port порт прослушивания для дочернего процесса
			 * @return     результат отправки порта дочернему процессу
			 *
			 * \~english
			 * @brief Method sending the listening port to a particular child process of the cluster
			 *
			 * @note Applies only to the QUIC transport: allows raising the server on a child process
			 *       manually when it did not get a port during the automatic handout
			 *
			 * @param pid  child process identifier
			 * @param port listening port for the child process
			 * @return     result of sending the port to the child process
			 *
			 * \~
			 */
			virtual bool clusterAssign(const pid_t pid, const uint16_t port) noexcept;
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
			 * @brief Method sending a message to the parent process
			 *
			 * @param buffer binary buffer for sending the message
			 * @param size   size of the binary buffer for sending the message
			 * @return       number of bytes of the sent message
			 *
			 * \~
			 */
			virtual size_t clusterSend(const void * buffer, const size_t size) noexcept;
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
			 * @brief Method sending a message to a child process
			 *
			 * @param pid    identifier of the process to receive the message
			 * @param buffer binary buffer for sending the message
			 * @param size   size of the binary buffer for sending the message
			 * @return       number of bytes of the sent message
			 *
			 * \~
			 */
			virtual size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
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
			 * @brief Method sending a message to all child processes
			 *
			 * @param buffer binary buffer for sending the message
			 * @param size   size of the binary buffer for sending the message
			 * @return       number of bytes of the sent message
			 *
			 * \~
			 */
			virtual size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки флага автоматического возрождения процессов
			 *
			 * @param mode флаг возрождения процессов
			 *
			 * \~english
			 * @brief Method setting the flag of automatic rebirth of processes
			 *
			 * @param mode flag of the rebirth of processes
			 *
			 * \~
			 */
			virtual void clusterRebirth(const bool mode) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
			 *
			 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
			 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
			 *
			 * \~english
			 * @brief Method setting the parameters of protection against a restart loop of the cluster processes
			 *
			 * @param limit  maximum number of consecutive fast crashes before stopping the cluster (0 — no limit)
			 * @param window time window of a "fast" (early) crash of a process in milliseconds
			 *
			 * \~
			 */
			virtual void clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения типа протокола передачи данных между воркерами
			 *
			 * @return тип протокола передачи данных между воркерами
			 *
			 * \~english
			 * @brief Method obtaining the type of the protocol of data transfer between the workers
			 *
			 * @return type of the protocol of data transfer between the workers
			 *
			 * \~
			 */
			virtual event::type_t clusterGetTypeEventMessage() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки типа протокола передачи данных между воркерами
			 *
			 * @param type тип протокола передачи данных между воркерами для установки
			 *
			 * \~english
			 * @brief Method setting the type of the protocol of data transfer between the workers
			 *
			 * @param type type of the protocol of data transfer between the workers to set
			 *
			 * \~
			 */
			virtual void clusterSetTypeEventMessage(const event::type_t type) noexcept;
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
			 * @brief Method obtaining the buffer size of an event
			 *
			 * @param pid    process identifier
			 * @param action event action type
			 * @return       buffer size of the event
			 *
			 * \~
			 */
			virtual size_t clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept;
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
			 * @brief Method setting the buffer size of an event
			 *
			 * @param pid    process identifier
			 * @param action event action type
			 * @param size   buffer size of the event
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the callback function connection method
			 *
			 * @tparam T    callback function type
			 * @tparam Args callback function arguments
			 *
			 * \~
			 */
			template <typename T, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Callback function connection method
			 *
			 * @param name callback function identifier
			 * @param args callback function arguments
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const char * name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the callback function connection method
			 *
			 * @tparam T    callback function type
			 * @tparam Args callback function arguments
			 *
			 * \~
			 */
			template <typename T, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Callback function connection method
			 *
			 * @param name callback function identifier
			 * @param args callback function arguments
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(string_view name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the callback function connection method
			 *
			 * @tparam T    callback function type
			 * @tparam Args callback function arguments
			 *
			 * \~
			 */
			template <typename T, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Callback function connection method
			 *
			 * @param name callback function identifier
			 * @param args callback function arguments
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const string & name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the callback function connection method
			 *
			 * @tparam T    callback function type
			 * @tparam Args callback function arguments
			 *
			 * \~
			 */
			template <typename T, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Callback function connection method
			 *
			 * @param fid  callback function identifier
			 * @param args callback function arguments
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const uint32_t fid, Args... args) noexcept -> uint32_t {
				// Если мы получили идентификатор функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (fid, args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the callback function connection method
			 *
			 * @tparam A    function identifier type
			 * @tparam B    callback function type
			 * @tparam Args callback function arguments
			 *
			 * \~
			 */
			template <typename A, typename B, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Callback function connection method
			 *
			 * @param fid  callback function identifier
			 * @param args callback function arguments
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const A fid, Args... args) noexcept -> uint32_t {
				// Если мы получили на вход число
				if constexpr (is_arithmetic_v <A> || is_enum_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <B> (static_cast <uint32_t> (fid), args...);
				// Возвращаем значение по умолчанию
				return 0;
			}
		private:
			/**
			 * \~russian
			 * @brief Конструктор копирования (запрещаем)
			 *
			 * \~english
			 * @brief Copy constructor (forbidden)
			 *
			 * \~
			 */
			Server(const Server &) = delete;
			/**
			 * \~russian
			 * @brief Оператор копирования (запрещаем)
			 *
			 * @return текущее значение объекта
			 *
			 * \~english
			 * @brief Copy assignment operator (forbidden)
			 *
			 * @return current value of the object
			 *
			 * \~
			 */
			Server & operator = (const Server &) = delete;
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
			 *
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Server(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param dns объект DNS-резолвера
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param dns DNS resolver object
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Server(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param cts   идентификатор шаблона контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param cts   security context template identifier
			 * @param coder transport layer security object
			 * @param fmk   framework object
			 * @param log   object for working with logs
			 *
			 * \~
			 */
			explicit Server(const tls::coder_t::id_t cts, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param cts   идентификатор шаблона контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param dns   объект DNS-резолвера
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param cts   security context template identifier
			 * @param coder transport layer security object
			 * @param dns   DNS resolver object
			 * @param fmk   framework object
			 * @param log   object for working with logs
			 *
			 * \~
			 */
			explicit Server(const tls::coder_t::id_t cts, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			virtual ~Server() noexcept;
	} server_t;
};

#endif // __AWH_SERVER__
