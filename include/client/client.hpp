/**
 * @file client.hpp
 * @date 2026-04-05
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
 * @brief Заголовочный файл фасада клиента — публичный API класса Client, объединяющего транспорт, TLS,
 *        DNS-резолвинг и подписку на события в единую точку входа для построения клиентских подключений по TCP, UDP,
 *        SCTP, UDS, DTLS и QUIC
 *
 * \~english
 * @brief Header file of the client facade — the public API of the Client class uniting the transport, TLS,
 *        DNS resolving and event subscription into a single entry point for building client connections over TCP, UDP,
 *        SCTP, UDS, DTLS and QUIC
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_CLIENT__
#define __AWH_CLIENT__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../unit/dns.hpp"
#include "../unit/quic.hpp"
#include "../unit/client.hpp"
#include "../cryptography/tls/coder.hpp"
#include "../proto/quic/connection.hpp"

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
	 * @brief Класс клиента
	 *
	 * \~english
	 * @brief Client class
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Client {
		protected:
			/**
			 * \~russian
			 * @brief Структура для хранения параметров DNS-резолвера
			 *
			 * \~english
			 * @brief Structure for storing the DNS resolver parameters
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
				// Контекст безопасности TLS
				tls::coder_t::id_t ctl;
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
			 * @brief Структура параметров клиента
			 *
			 * \~english
			 * @brief Structure of the client parameters
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Params {
				// Идентификатор потока по умолчанию для отправки без явного sid (INVALID_STREAM - не открыт)
				uint64_t stream;
				// Тип сокета транспорта клиента (STREAM/DATAGRAM/SEQPACKET - определяет доступность датаграмм)
				event::type_t type;
				// Протокол транспорта клиента (выбирается при инициализации, определяет обработку данных)
				event::protocol_t protocol;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Params() noexcept;
			} params_t;
			/**
			 * \~russian
			 * @brief Структура юнита клиента
			 *
			 * \~english
			 * @brief Structure of the client unit
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Unit {
				// Объект работы с сетевыми адресами
				net_addr_t addr;
				// Объект юнита клиента (транспорты TCP/UDP/SCTP и прикладные протоколы поверх них)
				unit::client_t client;
				// Объект юнита клиента QUIC (выбирается при инициализации транспортом protocol_t::QUIC)
				unit::quic_client_t quic;
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
			// Идентификатор клиента
			id_t _id;
		protected:
			// Объект DNS-резолвера
			dns_t _dns;
		protected:
			// Адрес хоста целевой машины
			string _host;
		protected:
			// Параметры клиента
			params_t _params;
		protected:
			// Функция обратного вызова для обработки клиента
			callback_t _callback;
		protected:
			// Объект юнита клиента
			unique_ptr <unit_t> _unit;
		protected:
			// Объект транспортного уровня безопасности
			tls::coder_t * _coder;
		protected:
			/**
			 * \~russian
			 * @brief Флаг завершения отправки данных потоковым транспортом
			 *
			 * @details Устанавливается при отправке с флагом fin, а по факту записи данных
			 *          в сокет соединение завершается (для потоковых транспортов клиент
			 *          всегда единственный)
			 *
			 * \~english
			 * @brief Flag of the completion of data sending by a stream transport
			 *
			 * @details Set upon sending with the fin flag, and upon the fact of writing the data into
			 *          the socket the connection is terminated (for stream transports the client is
			 *          always the only one)
			 *
			 * \~
			 */
			bool _fin;
		protected:
			// Позиция чтения выданного сетевому движку шифротекста TLS
			size_t _offset;
			/**
			 * \~russian
			 * @brief Буфер шифротекста TLS, ожидающего отправки
			 *
			 * @details Шифротекст выдаётся сетевому движку вытягивающей моделью: движок
			 *          забирает его ровно тогда, когда готов отправить, поэтому при
			 *          переполнении очереди отправки записи TLS не теряются. Потеря даже
			 *          одной записи рвёт поток шифрования и обрывает соединение
			 *
			 * \~english
			 * @brief Buffer of the TLS ciphertext awaiting the sending
			 * @details The ciphertext is given out to the network engine by the pull model: the engine
			 *          takes it exactly when it is ready to send it, so at an overflow of the queue
			 *          of the sending the TLS records are not lost. A loss of even a single record
			 *          breaks the stream of the encryption and terminates the connection
			 *
			 * \~
			 */
			string _residue;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		protected:
			/**
			 * \~russian
			 * @brief Метод проверки рабочего состояния клиента
			 *
			 * @note Проверяет рабочее состояние DNS-резолвера либо активного юнита
			 *       транспорта, выбираемого по протоколу (QUIC - выделенный юнит,
			 *       остальные транспорты - общий юнит клиента)
			 *
			 * @return результат проверки рабочего состояния
			 *
			 * \~english
			 * @brief Method checking the working state of the client
			 *
			 * @note Checks the working state of the DNS resolver or of the active transport unit
			 *       selected by the protocol (QUIC - the dedicated unit, the remaining transports -
			 *       the common client unit)
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
			 * @note Транспорт выбирается по протоколу клиента: для protocol_t::QUIC
			 *       работает выделенный юнит клиента QUIC, для остальных транспортов -
			 *       общий юнит клиента. Событием во всех случаях выступает _id.eid
			 *
			 * \~english
			 * @brief Methods of dispatching to the active transport unit
			 *
			 * @note The transport is selected by the client protocol: for protocol_t::QUIC the dedicated
			 *       QUIC client unit works, for the remaining transports - the common client unit. The
			 *       event in all cases is _id.eid
			 *
			 * \~
			 */
			bool commitUnit() noexcept;
			bool launchUnit() noexcept;
			void startUnit() noexcept;
			void stopUnit() noexcept;
			void destroyUnit() noexcept;
			bool pauseUnit() noexcept;
			bool resumeUnit() noexcept;
			bool connectUnit() noexcept;
			bool disconnectUnit() noexcept;
			bool recvUnit() noexcept;
			size_t sendUnit(const void * buffer, const size_t size) noexcept;
			event::family_t familyUnit() const noexcept;
			event::status_t statusUnit() const noexcept;
			string getTargetUnit() const noexcept;
			uint16_t getTargetPortUnit() const noexcept;
			bool setTargetUnit(string_view target) noexcept;
			bool setTargetUnit(const net::addr_t * target) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод изменения статуса клиента
			 *
			 * @param index  индекс очереди запускаемого события
			 * @param status новый статус клиента
			 *
			 * \~english
			 * @brief Client status change method
			 *
			 * @param index  index of the queue of the event being started
			 * @param status new client status
			 *
			 * \~
			 */
			virtual void status(const uint8_t index, const event::status_t status) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод обработки событий подключения клиента к удалённому серверу
			 *
			 * @param    идентификатор клиента
			 * @param ok результат подключения
			 *
			 * \~english
			 * @brief Method processing the events of the client connecting to a remote server
			 *
			 * @param    client identifier
			 * @param ok connection result
			 *
			 * \~
			 */
			virtual void connect(const event::id_t, const bool ok) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий записи данных клиентом
			 *
			 * @param      идентификатор клиента
			 * @param size размер данных для записи
			 *
			 * \~english
			 * @brief Method processing the events of data being written by the client
			 *
			 * @param      client identifier
			 * @param size size of the data to write
			 *
			 * \~
			 */
			virtual void write(const event::id_t, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий изменения состояния клиента
			 *
			 * @param        идентификатор клиента
			 * @param status новый статус клиента
			 *
			 * \~english
			 * @brief Method processing the events of the client state changing
			 *
			 * @param        client identifier
			 * @param status new client status
			 *
			 * \~
			 */
			virtual void state(const event::id_t, const event::status_t status) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки действий клиента
			 *
			 * @param        идентификатор клиента
			 * @param action действие клиента
			 *
			 * \~english
			 * @brief Method processing the client actions
			 *
			 * @param        client identifier
			 * @param action client action
			 *
			 * \~
			 */
			virtual void action(const event::id_t, const event::action_t action) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
			 *
			 * @param      идентификатор события
			 * @param info информационные метаданные о дейтаграммном пакете
			 *
			 * \~english
			 * @brief Method processing the informational metadata about a datagram packet
			 *
			 * @param      event identifier
			 * @param info informational metadata about the datagram packet
			 *
			 * \~
			 */
			virtual void traffic(const event::id_t, const net::dgram_info_t & info) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий получения данных клиентом
			 *
			 * @param        идентификатор клиента
			 * @param buffer буфер данных клиента
			 * @param size   размер данных клиента
			 *
			 * \~english
			 * @brief Method processing the events of data being received by the client
			 *
			 * @param        client identifier
			 * @param buffer client data buffer
			 * @param size   client data size
			 *
			 * \~
			 */
			virtual void read(const event::id_t, const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки собранных данных потока соединения QUIC
			 *
			 * @param      идентификатор события
			 * @param sid  идентификатор потока приложения
			 * @param data собранные данные потока
			 * @param fin  флаг завершения потока удалённым эндпоинтом
			 *
			 * \~english
			 * @brief Method processing the assembled data of a QUIC connection stream
			 *
			 * @param      event identifier
			 * @param sid  application stream identifier
			 * @param data assembled stream data
			 * @param fin  flag of the stream completion by the remote endpoint
			 *
			 * \~
			 */
			virtual void stream(const event::id_t, const uint64_t sid, const string & data, const bool fin) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки освобождения буфера отправки потока соединения QUIC (сигнал writable)
			 *
			 * @param     идентификатор события
			 * @param sid идентификатор потока приложения
			 *
			 * \~english
			 * @brief Method processing the release of the send buffer of a QUIC connection stream (the writable signal)
			 *
			 * @param     event identifier
			 * @param sid application stream identifier
			 *
			 * \~
			 */
			virtual void writable(const event::id_t, const uint64_t sid) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки принятой датаграммы приложения QUIC (RFC 9221)
			 *
			 * @param      идентификатор события
			 * @param data данные принятой датаграммы
			 *
			 * \~english
			 * @brief Method processing a received QUIC application datagram (RFC 9221)
			 *
			 * @param      event identifier
			 * @param data data of the received datagram
			 *
			 * \~
			 */
			virtual void message(const event::id_t, const string & data) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки готовности к отправке ранних данных QUIC (RFC 9001 §4.6)
			 *
			 * @param идентификатор события
			 *
			 * \~english
			 * @brief Method processing the readiness to send QUIC early data (RFC 9001 §4.6)
			 *
			 * @param event identifier
			 *
			 * \~
			 */
			virtual void earlyData(const event::id_t) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки завершения соединения QUIC (RFC 9000 §10)
			 *
			 * @param       идентификатор события
			 * @param error код ошибки завершения соединения
			 *
			 * \~english
			 * @brief Method processing the termination of a QUIC connection (RFC 9000 §10)
			 *
			 * @param       event identifier
			 * @param error error code of the connection termination
			 *
			 * \~
			 */
			virtual void closed(const event::id_t, const quic::error_t error) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки события ошибки
			 *
			 * @param         идентификатор события
			 * @param error   код ошибки
			 * @param message сообщение об ошибке
			 *
			 * \~english
			 * @brief Method processing an error event
			 *
			 * @param         event identifier
			 * @param error   error code
			 * @param message error message
			 *
			 * \~
			 */
			virtual void error(const event::id_t, const event::error_t error, const string & message) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
			 *
			 * @param        идентификатор клиента
			 * @param status статус доступности очереди
			 * @param size   размер доступных данных очереди
			 *
			 * \~english
			 * @brief Method processing the events of availability/unavailability of the client outgoing data queue
			 *
			 * @param        client identifier
			 * @param status status of the queue availability
			 * @param size   size of the available data of the queue
			 *
			 * \~
			 */
			virtual void available(const event::id_t, const event::status_t status, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки событий истечения таймаута клиента
			 *
			 * @param        идентификатор клиента
			 * @param action тип действия для истекшего таймаута
			 * @param delay  задержка таймаута в миллисекундах
			 * @return       нужно ли завершить клиента после истечения таймаута
			 *
			 * \~english
			 * @brief Method processing the events of the client timeout expiring
			 *
			 * @param        client identifier
			 * @param action action type for the expired timeout
			 * @param delay  timeout delay in milliseconds
			 * @return       whether the client should be terminated after the timeout expires
			 *
			 * \~
			 */
			virtual bool timeout(const event::id_t, const event::action_t action, const uint32_t delay) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки попыток подключения клиента к удалённому серверу
			 *
			 * @param          идентификатор DNS-запроса
			 * @param domain   доменное имя для резолвинга
			 * @param attempts количество попыток подключения
			 *
			 * \~english
			 * @brief Method processing the attempts of the client to connect to a remote server
			 *
			 * @param          DNS request identifier
			 * @param domain   domain name to resolve
			 * @param attempts number of connection attempts
			 *
			 * \~
			 */
			virtual void attempts(const unit::dns_t::id_t, const string & domain, const uint8_t attempts) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки неудачного резолвинга доменного имени
			 *
			 * @param        идентификатор DNS-запроса
			 * @param record тип записи DNS
			 * @param domain доменное имя
			 *
			 * \~english
			 * @brief Method processing an unsuccessful domain name resolving
			 *
			 * @param        DNS request identifier
			 * @param record DNS record type
			 * @param domain domain name
			 *
			 * \~
			 */
			virtual void failure(const unit::dns_t::id_t, const unit::dns_t::record_t record, const string & domain) noexcept;
			/**
			 * \~russian
			 * @brief Метод обработки события неотправленных данных клиента
			 *
			 * @param        идентификатор клиента
			 * @param error  тип ошибки отправки данных
			 * @param buffer данные, которые не удалось отправить
			 * @param size   размер данных, которые не удалось отправить
			 *
			 * \~english
			 * @brief Method processing the event of unsent client data
			 *
			 * @param        client identifier
			 * @param error  type of the data sending error
			 * @param buffer data that could not be sent
			 * @param size   size of the data that could not be sent
			 *
			 * \~
			 */
			virtual void spool(const event::id_t, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод резолвинга доменного имени в сетевой адрес
			 *
			 * @param        идентификатор DNS-запроса
			 * @param family семейство адресов (IPv4/IPv6)
			 * @param domain доменное имя для резолвинга
			 * @param addr   указатель на структуру для хранения результата резолвинга
			 *
			 * \~english
			 * @brief Method resolving a domain name into a network address
			 *
			 * @param        DNS request identifier
			 * @param family address family (IPv4/IPv6)
			 * @param domain domain name to resolve
			 * @param addr   pointer to the structure for storing the resolving result
			 *
			 * \~
			 */
			virtual void resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод выдачи шифротекста TLS сетевому движку
			 *
			 * @note Функция-источник вытягивающей модели: движок спрашивает данные ровно
			 *       тогда, когда готов их отправить, поэтому записи TLS при переполнении
			 *       очереди отправки не теряются
			 * @param        идентификатор клиента
			 * @param buffer адрес указателя на буфер выдаваемых данных
			 * @param size   ёмкость запроса на входе и размер выданных данных на выходе
			 * @return       признак продолжения вытягивания данных
			 *
			 * \~english
			 * @brief Method of giving out the TLS ciphertext to the network engine
			 * @note Source function of the pull model: the engine asks for the data exactly
			 *       when it is ready to send them, so the TLS records are not lost at an
			 *       overflow of the queue of the sending
			 * @param        client identifier
			 * @param buffer address of the pointer to the buffer of the given out data
			 * @param size   capacity of the request at the input and size of the given out data at the output
			 * @return       flag of the continuation of the pulling of the data
			 *
			 * \~
			 */
			virtual bool source(const event::id_t, const uint8_t ** buffer, size_t & size) noexcept;
		protected:
			/**
			 * \~russian
			 * @brief Метод постановки шифротекста TLS в очередь на отправку
			 *
			 * @note Записи TLS отправляются строго в порядке шифрования, поэтому новая
			 *       запись становится в хвост буфера, а не идёт в сокет мимо ожидающих
			 * @param buffer буфер шифротекста
			 * @param size   размер шифротекста
			 *
			 * \~english
			 * @brief Method of the putting of the TLS ciphertext into the queue for the sending
			 * @note The TLS records are sent strictly in the order of the encryption, so a new
			 *       record is put into the tail of the buffer instead of going into the socket
			 *       past the awaiting ones
			 * @param buffer buffer of the ciphertext
			 * @param size   size of the ciphertext
			 *
			 * \~
			 */
			virtual void residueTLS(const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения состояния TLS
			 *
			 * @param       идентификатор TLS
			 * @param state состояние TLS
			 *
			 * \~english
			 * @brief TLS state obtaining method
			 *
			 * @param       TLS identifier
			 * @param state TLS state
			 *
			 * \~
			 */
			virtual void stateTLS(const tls::coder_t::id_t, const tls::coder_t::state_t state) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения ошибок TLS
			 *
			 * @param         идентификатор TLS
			 * @param error   код ошибки TLS
			 * @param message сообщение об ошибке TLS
			 *
			 * \~english
			 * @brief TLS errors obtaining method
			 *
			 * @param         TLS identifier
			 * @param error   TLS error code
			 * @param message TLS error message
			 *
			 * \~
			 */
			virtual void errorTLS(const tls::coder_t::id_t, const tls::coder_t::error_t error, const string & message) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения событий шифрования/дешифрования данных TLS
			 *
			 * @param        идентификатор TLS
			 * @param event  тип события TLS
			 * @param buffer буфер данных для события шифрования/дешифрования TLS
			 * @param size   размер полезной нагрузки в буфере для события шифрования/дешифрования TLS
			 *
			 * \~english
			 * @brief Method obtaining the events of TLS data encryption/decryption
			 *
			 * @param        TLS identifier
			 * @param event  TLS event type
			 * @param buffer data buffer for the TLS encryption/decryption event
			 * @param size   size of the payload in the buffer for the TLS encryption/decryption event
			 *
			 * \~
			 */
			virtual void processTLS(const tls::coder_t::id_t, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод остановки клиента
			 *
			 * \~english
			 * @brief Client stopping method
			 *
			 * \~
			 */
			virtual void stop() noexcept;
			/**
			 * \~russian
			 * @brief Метод запуска клиента
			 *
			 * \~english
			 * @brief Client starting method
			 *
			 * \~
			 */
			virtual void start() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод приостановки работы клиента
			 *
			 * @return результат выполнения приостановки работы
			 *
			 * \~english
			 * @brief Client operation suspension method
			 *
			 * @return result of performing the operation suspension
			 *
			 * \~
			 */
			virtual bool pause() noexcept;
			/**
			 * \~russian
			 * @brief Метод возобновления работы клиента
			 *
			 * @return результат выполнения возобновления работы
			 *
			 * \~english
			 * @brief Client operation resumption method
			 *
			 * @return result of performing the operation resumption
			 *
			 * \~
			 */
			virtual bool resume() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод уничтожения события клиента
			 *
			 * \~english
			 * @brief Client event destruction method
			 *
			 * \~
			 */
			virtual void destroy() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки, жив ли клиент
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method checking whether the client is alive
			 *
			 * @return check result
			 *
			 * \~
			 */
			virtual bool isAlive() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод подключения клиента к удалённому хосту
			 *
			 * @return результат выполнения подключения
			 *
			 * \~english
			 * @brief Method connecting the client to a remote host
			 *
			 * @return result of performing the connection
			 *
			 * \~
			 */
			virtual bool connect() noexcept;
			/**
			 * \~russian
			 * @brief Метод отключения клиента от удалённого сервера
			 *
			 * @return результат выполнения отключения
			 *
			 * \~english
			 * @brief Method disconnecting the client from a remote server
			 *
			 * @return result of performing the disconnection
			 *
			 * \~
			 */
			virtual bool disconnect() noexcept;
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
			 * @brief Метод получения данных от сервера
			 *
			 * @return результат получения данных
			 *
			 * \~english
			 * @brief Method of receiving data from the server
			 *
			 * @return data receiving result
			 *
			 * \~
			 */
			virtual bool recv() noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки данных серверу
			 *
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @return       количество байт данных, отправленных серверу
			 *
			 * \~english
			 * @brief Method of sending data to the server
			 *
			 * @param buffer data buffer to send
			 * @param size   size of the data to send
			 * @return       number of data bytes sent to the server
			 *
			 * \~
			 */
			virtual size_t send(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки локальных транспортных параметров соединения QUIC (RFC 9000 §7.4)
			 *
			 * @param params локальные транспортные параметры
			 *
			 * \~english
			 * @brief Method setting the local transport parameters of a QUIC connection (RFC 9000 §7.4)
			 *
			 * @param params local transport parameters
			 *
			 * \~
			 */
			virtual void params(const quic::params::params_t & params) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки уведомления о перегрузке пути QUIC (RFC 9000 §13.4)
			 *
			 * @param mode режим уведомления о перегрузке пути
			 *
			 * \~english
			 * @brief Method setting the QUIC path congestion notification (RFC 9000 §13.4)
			 *
			 * @param mode path congestion notification mode
			 *
			 * \~
			 */
			virtual void ecn(const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения сохранённого токена проверки адреса QUIC (RFC 9000 §8.1.3)
			 *
			 * @return токен проверки адреса (пусто - токен не получен)
			 *
			 * \~english
			 * @brief Method extracting the stored QUIC address validation token (RFC 9000 §8.1.3)
			 *
			 * @return address validation token (empty - the token has not been received)
			 *
			 * \~
			 */
			virtual const string & token() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки сохранённого токена проверки адреса QUIC (RFC 9000 §8.1.3)
			 *
			 * @param token токен проверки адреса
			 *
			 * \~english
			 * @brief Method setting the stored QUIC address validation token (RFC 9000 §8.1.3)
			 *
			 * @param token address validation token
			 *
			 * \~
			 */
			virtual void token(string_view token) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки принятия ранних данных удалённым сервером QUIC (RFC 9001 §4.6.2)
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method checking the acceptance of early data by the remote QUIC server (RFC 9001 §4.6.2)
			 *
			 * @return check result
			 *
			 * \~
			 */
			virtual bool early() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод открытия потока данных соединения (QUIC-поток; для потоковых транспортов - единственный поток)
			 *
			 * @param mode режим однонаправленного потока
			 * @return     идентификатор открытого потока
			 *
			 * \~english
			 * @brief Method opening a connection data stream (a QUIC stream; for stream transports - the only stream)
			 *
			 * @param mode unidirectional stream mode
			 * @return     identifier of the opened stream
			 *
			 * \~
			 */
			virtual uint64_t open(const bool mode = false) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки данных в поток соединения (QUIC/HTTP2-поток либо единственный поток потокового транспорта)
			 *
			 * @param sid    идентификатор потока
			 * @param buffer буфер данных для отправки
			 * @param size   размер данных для отправки
			 * @param fin    флаг завершения потока
			 * @return       количество байт данных, поставленных в очередь отправки
			 *
			 * \~english
			 * @brief Method sending data into a connection stream (a QUIC/HTTP2 stream or the only stream of a stream transport)
			 *
			 * @param sid    stream identifier
			 * @param buffer data buffer to send
			 * @param size   size of the data to send
			 * @param fin    stream completion flag
			 * @return       number of data bytes queued for sending
			 *
			 * \~
			 */
			virtual size_t send(const uint64_t sid, const void * buffer, const size_t size, const bool fin = false) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки водяных меток буфера отправки потоков соединения QUIC (backpressure)
			 *
			 * @note Верхняя метка ограничивает несобранный буфер потока (send() принимает частично),
			 *       по опустошению ниже нижней метки поток сигнализируется колбэком "writable". Ноль -
			 *       ограничение снято. Для потоковых транспортов метод не действует
			 *
			 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
			 * @param low  нижняя водяная метка (порог сигнала "writable")
			 *
			 * \~english
			 * @brief Method setting the water marks of the send buffer of QUIC connection streams (backpressure)
			 *
			 * @note The high mark limits the unassembled buffer of the stream (send() accepts partially),
			 *       upon emptying below the low mark the stream is signalled by the "writable" callback. Zero -
			 *       the limit is lifted. For stream transports the method has no effect
			 *
			 * @param high high water mark (capacity of the stream send buffer)
			 * @param low  low water mark (threshold of the "writable" signal)
			 *
			 * \~
			 */
			virtual void sendWaterMarks(const size_t high, const size_t low) noexcept;
			/**
			 * \~russian
			 * @brief Метод назначения pull-источника данных потока соединения QUIC (RFC 9000 §2.2)
			 *
			 * @note Альтернатива send() для больших тел: движок сам тянет данные у источника по мере
			 *       места в буфере отправки, не требуя держать копию тела. Для потоковых транспортов не действует
			 *
			 * @param sid    идентификатор потока приложения
			 * @param source pull-источник данных тела потока
			 *
			 * \~english
			 * @brief Method assigning a pull source of the data of a QUIC connection stream (RFC 9000 §2.2)
			 *
			 * @note An alternative to send() for large bodies: the engine itself pulls the data from the source as
			 *       room appears in the send buffer, without requiring a copy of the body to be held. For stream transports it has no effect
			 *
			 * @param sid    application stream identifier
			 * @param source pull source of the stream body data
			 *
			 * \~
			 */
			virtual void dataSource(const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки датаграммы соединению (QUIC DATAGRAM по RFC 9221 либо дейтаграмма UDP)
			 *
			 * @param buffer буфер данных датаграммы для отправки
			 * @param size   размер данных датаграммы для отправки
			 * @return       результат отправки
			 *
			 * \~english
			 * @brief Method sending a datagram to the connection (a QUIC DATAGRAM per RFC 9221 or a UDP datagram)
			 *
			 * @param buffer buffer of the datagram data to send
			 * @param size   size of the datagram data to send
			 * @return       sending result
			 *
			 * \~
			 */
			virtual bool datagram(const void * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения предельного размера отправляемой датаграммы QUIC (RFC 9221 §3)
			 *
			 * @return предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
			 *
			 * \~english
			 * @brief Method obtaining the limiting size of a QUIC datagram being sent (RFC 9221 §3)
			 *
			 * @return limiting size of the datagram data in octets (0 - datagrams are not supported)
			 *
			 * \~
			 */
			virtual size_t datagrams() const noexcept;
			/**
			 * \~russian
			 * @brief Метод завершения соединения (QUIC CONNECTION_CLOSE по RFC 9000 §10.2 либо уничтожение события для остальных транспортов)
			 *
			 * @param code   код ошибки приложения
			 * @param reason человекочитаемая причина завершения
			 *
			 * \~english
			 * @brief Connection termination method (a QUIC CONNECTION_CLOSE per RFC 9000 §10.2 or destruction of the event for the remaining transports)
			 *
			 * @param code   application error code
			 * @param reason human-readable reason of the termination
			 *
			 * \~
			 */
			virtual void close(const uint64_t code = 0, string_view reason = "") noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод объединения данных между клиентом и другим событием
			 *
			 * @param eid    идентификатор события
			 * @param direct направление объединения данных (клиент -> событие, событие -> клиент)
			 * @return       результат выполнения объединения
			 *
			 * \~english
			 * @brief Method of splicing data between the client and another event
			 *
			 * @param eid    event identifier
			 * @param direct direction of the data splicing (client -> event, event -> client)
			 * @return       result of performing the splicing
			 *
			 * \~
			 */
			virtual bool splice(const event::id_t eid, const event::direct_t direct) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения опций клиента
			 *
			 * @return опции клиента
			 *
			 * \~english
			 * @brief Client options obtaining method
			 *
			 * @return client options
			 *
			 * \~
			 */
			virtual uint16_t getOptions() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки опций клиента
			 *
			 * @param options опции клиента для установки
			 * @return        результат выполнения установки
			 *
			 * \~english
			 * @brief Client options setting method
			 *
			 * @param options client options to set
			 * @return        result of performing the setting
			 *
			 * \~
			 */
			virtual bool setOptions(const uint16_t options) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки опции клиента
			 *
			 * @param option опция клиента для установки
			 * @param mode   режим установки опции клиента
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Client option setting method
			 *
			 * @param option client option to set
			 * @param mode   client option setting mode
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool setOption(const uint16_t option, const bool mode) noexcept;
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
			 * @return максимальное количество хопов
			 *
			 * \~english
			 * @brief Method obtaining the maximum number of hops a packet can pass through
			 *
			 * @return maximum number of hops
			 *
			 * \~
			 */
			virtual event::hops_t getHops() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
			 *
			 * @param hops максимальное количество хопов
			 * @return     результат работы функции
			 *
			 * \~english
			 * @brief Method setting the maximum number of hops a packet can pass through
			 *
			 * @param hops maximum number of hops
			 * @return     result of the function work
			 *
			 * \~
			 */
			virtual bool setHops(const event::hops_t hops) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения сетевого интерфейса клиента
			 *
			 * @return сетевой интерфейс клиента
			 *
			 * \~english
			 * @brief Method obtaining the client network interface
			 *
			 * @return client network interface
			 *
			 * \~
			 */
			virtual string getIface() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки сетевого интерфейса клиента
			 *
			 * @param name имя сетевого интерфейса для установки
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the client network interface
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
			 * @brief Метод получения внутреннего порта события
			 *
			 * @return внутренний порт события
			 *
			 * \~english
			 * @brief Method obtaining the internal port of the event
			 *
			 * @return internal port of the event
			 *
			 * \~
			 */
			virtual uint16_t getSourcePort() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки внутреннего порта события
			 *
			 * @param port внутренний порт события
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the internal port of the event
			 *
			 * @param port internal port of the event
			 * @return     result of performing the setting
			 *
			 * \~
			 */
			virtual bool setSourcePort(const uint16_t port) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения порта удаленного сервера
			 *
			 * @return порт удаленного сервера
			 *
			 * \~english
			 * @brief Method obtaining the port of the remote server
			 *
			 * @return port of the remote server
			 *
			 * \~
			 */
			virtual uint16_t getTargetPort() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки порта удаленного сервера
			 *
			 * @param port порт удаленного сервера для установки
			 * @return     результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the port of the remote server
			 *
			 * @param port port of the remote server to set
			 * @return     result of performing the setting
			 *
			 * \~
			 */
			virtual bool setTargetPort(const uint16_t port) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @return адрес хоста целевой машины
			 *
			 * \~english
			 * @brief Method obtaining the host address of the target machine
			 *
			 * @return host address of the target machine
			 *
			 * \~
			 */
			virtual string getTarget() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the host address of the target machine
			 *
			 * @param target host address of the target machine
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool setTarget(string_view target) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки адреса хоста целевой машины
			 *
			 * @param target адрес хоста целевой машины
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the host address of the target machine
			 *
			 * @param target host address of the target machine
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool setTarget(const net::addr_t * target) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения адреса хоста целевой машины
			 *
			 * @param target объект для извлечения адреса хоста целевой машины
			 * @return       результат выполнения извлечения адреса хоста целевой машины
			 *
			 * \~english
			 * @brief Method obtaining the host address of the target machine
			 *
			 * @param target object for extracting the host address of the target machine
			 * @return       result of performing the extraction of the host address of the target machine
			 *
			 * \~
			 */
			virtual bool getTarget(unique_ptr <net::addr_t> & target) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @return        значение адреса клиента
			 *
			 * \~english
			 * @brief Client address obtaining method
			 *
			 * @param address client address type
			 * @return        client address value
			 *
			 * \~
			 */
			virtual string getAddress(const event::address_t address) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   значение адреса клиента
			 * @return        результат выполнения установки
			 *
			 * \~english
			 * @brief Client address setting method
			 *
			 * @param address client address type
			 * @param value   client address value
			 * @return        result of performing the setting
			 *
			 * \~
			 */
			virtual bool setAddress(const event::address_t address, string_view value) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   значение адреса клиента
			 * @return        результат выполнения установки
			 *
			 * \~english
			 * @brief Client address setting method
			 *
			 * @param address client address type
			 * @param value   client address value
			 * @return        result of performing the setting
			 *
			 * \~
			 */
			virtual bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения адреса клиента
			 *
			 * @param address тип адреса клиента
			 * @param value   объект для извлечения адреса клиента
			 * @return        результат выполнения извлечения адреса клиента
			 *
			 * \~english
			 * @brief Client address obtaining method
			 *
			 * @param address client address type
			 * @param value   object for extracting the client address
			 * @return        result of performing the extraction of the client address
			 *
			 * \~
			 */
			virtual bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения режима трансляции пакетов клиента
			 *
			 * @return режим трансляции пакетов (unicast, multicast, broadcast)
			 *
			 * \~english
			 * @brief Method obtaining the client packet delivery mode
			 *
			 * @return packet delivery mode (unicast, multicast, broadcast)
			 *
			 * \~
			 */
			virtual event::delivery_mode_t getDelivery() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки режима трансляции пакетов клиента
			 *
			 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
			 * @return         результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the client packet delivery mode
			 *
			 * @param delivery packet delivery mode (unicast, multicast, broadcast)
			 * @return         result of performing the setting
			 *
			 * \~
			 */
			virtual bool setDelivery(const event::delivery_mode_t delivery) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения размера буфера клиента
			 *
			 * @param action тип действия клиента
			 * @return       размер буфера клиента
			 *
			 * \~english
			 * @brief Client buffer size obtaining method
			 *
			 * @param action client action type
			 * @return       client buffer size
			 *
			 * \~
			 */
			virtual size_t getBufferSize(const event::action_t action) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки размера буфера клиента
			 *
			 * @param action тип действия клиента
			 * @param size   размер буфера клиента
			 * @return       результат выполнения установки
			 *
			 * \~english
			 * @brief Client buffer size setting method
			 *
			 * @param action client action type
			 * @param size   client buffer size
			 * @return       result of performing the setting
			 *
			 * \~
			 */
			virtual bool setBufferSize(const event::action_t action, const size_t size) noexcept;
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
		public:
			/**
			 * \~russian
			 * @brief Метод получения таймаута клиента
			 *
			 * @param action тип действия клиента
			 * @return       значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Client timeout obtaining method
			 *
			 * @param action client action type
			 * @return       timeout value in milliseconds
			 *
			 * \~
			 */
			virtual uint32_t getTimeout(const event::action_t action) const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки таймаута клиента
			 *
			 * @param action  тип действия клиента
			 * @param timeout значение таймаута в миллисекундах
			 *
			 * \~english
			 * @brief Client timeout setting method
			 *
			 * @param action  client action type
			 * @param timeout timeout value in milliseconds
			 *
			 * \~
			 */
			virtual void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки пропускной способности клиента
			 *
			 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
			 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
			 * @return          результат выполнения установки
			 *
			 * \~english
			 * @brief Client bandwidth setting method
			 *
			 * @param limiting  client bandwidth limiting mode (egress or ingress)
			 * @param bandwidth client bandwidth to set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
			 * @return          result of performing the setting
			 *
			 * \~
			 */
			virtual bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки параметров keep-alive для клиента
			 *
			 * @param cnt   количество пакетов keep-alive
			 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
			 * @param intvl интервал между пакетами keep-alive в секундах
			 * @return      результат выполнения установки
			 *
			 * \~english
			 * @brief Method setting the keep-alive parameters for the client
			 *
			 * @param cnt   number of keep-alive packets
			 * @param idle  idle time before sending the first keep-alive packet in seconds
			 * @param intvl interval between keep-alive packets in seconds
			 * @return      result of performing the setting
			 *
			 * \~
			 */
			virtual bool keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
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
			 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
			 *
			 * @return режим обнаружения максимального размера пакета (MTU)
			 *
			 * \~english
			 * @brief Method obtaining the maximum transmission unit (MTU) discovery mode
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
			 * @brief Метод инициализации клиента
			 *
			 * @param family   семейство адресов
			 * @param type     тип события
			 * @param protocol протокол события
			 * @return         идентификатор созданного клиента
			 *
			 * \~english
			 * @brief Client initialization method
			 *
			 * @param family   address family
			 * @param type     event type
			 * @param protocol event protocol
			 * @return         identifier of the created client
			 *
			 * \~
			 */
			virtual event::id_t init(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода подключения функции обратного вызова
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
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
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
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
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
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
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
			 * @brief Метод подключения функции обратного вызова
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
			 * @brief Шаблон метода подключения функции обратного вызова
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
			 * @brief Метод подключения функции обратного вызова
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
			Client(const Client &) = delete;
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
			Client & operator = (const Client &) = delete;
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
			explicit Client(const fmk_t * fmk, const log_t * log) noexcept;
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
			explicit Client(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param ctl   идентификатор контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param ctl   security context identifier
			 * @param coder transport layer security object
			 * @param fmk   framework object
			 * @param log   object for working with logs
			 *
			 * \~
			 */
			explicit Client(const tls::coder_t::id_t ctl, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param ctl   идентификатор контекста безопасности
			 * @param coder объект транспортного уровня безопасности
			 * @param dns   объект DNS-резолвера
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param ctl   security context identifier
			 * @param coder transport layer security object
			 * @param dns   DNS resolver object
			 * @param fmk   framework object
			 * @param log   object for working with logs
			 *
			 * \~
			 */
			explicit Client(const tls::coder_t::id_t ctl, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
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
			virtual ~Client() noexcept;
	} client_t;
};

#endif // __AWH_CLIENT__
