/**
 * @file socks5.hpp
 * @date 2026-05-30
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
 * @brief Заголовочный файл сервера SOCKS5-прокси — публичный API класса server::Socks5,
 *        выполняющего авторизацию клиентов, установку исходящих соединений по командам CONNECT и BIND и проксирование
 *        UDP-трафика через пул выделенных UDP-серверов
 *
 * \~english
 * @brief Header file of the SOCKS5 proxy server — the public API of the server::Socks5 class
 *        performing the authorization of clients, the establishment of outgoing connections by the CONNECT and BIND commands and the proxying of
 *        UDP traffic through a pool of dedicated UDP servers
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовка
 */
#ifndef __AWH_SERVER_SOCKS5__
#define __AWH_SERVER_SOCKS5__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "server.hpp"
#include "../unit/client.hpp"
#include "../proto/socks5/server.hpp"

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
	 * @brief Пространство имён сервера
	 *
	 * \~english
	 * @brief Server namespace
	 *
	 * \~
	 */
	namespace server {
		/**
		 * \~russian
		 * @brief Класс сервера SOCKS5-прокси
		 *
		 * \~english
		 * @brief SOCKS5 proxy server class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Socks5 : public server_t {
			private:
				/**
				 * \~russian
				 * @brief Класс идентификатора сессии клиента, работающего через прокси
				 *
				 * \~english
				 * @brief Class of the session identifier of a client working through a proxy
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Origin {
					public:
						// Тип адреса инициатора запроса
						net::type_t type;
					public:
						/**
						 * \~russian
						 * @brief Универсальная структура для хранения различных типов адресов
						 *
						 * \~english
						 * @brief Universal structure for storing various address types
						 *
						 * \~
						 */
						union {
							/**
							 * \~russian
							 * @brief Структура FQDN адреса инициатора запроса
							 *
							 * \~english
							 * @brief Structure of the FQDN address of the request originator
							 *
							 * \~
							 */
							struct {
								// Порт инициатора запроса
								uint16_t port = 0;
								// Данные доменного имени инициатора запроса
								char data[256] = {0};
							} fqdn;
							/**
							 * \~russian
							 * @brief Структура IPv4 адреса инициатора запроса
							 *
							 * \~english
							 * @brief Structure of the IPv4 address of the request originator
							 *
							 * \~
							 */
							struct {
								// Порт инициатора запроса
								uint16_t port = 0;
								// Адрес инициатора запроса
								uint32_t address = 0;
							} ip4;
							/**
							 * \~russian
							 * @brief Структура IPv6 адреса инициатора запроса
							 *
							 * \~english
							 * @brief Structure of the IPv6 address of the request originator
							 *
							 * \~
							 */
							struct {
								// Порт инициатора запроса
								uint16_t port = 0;
								// Адрес инициатора запроса
								array <uint8_t, 16> address = {0};
							} ip6;
						};
					public:
						/**
						 * \~russian
						 * @brief Фабричный метод создания идентификатора инициатора запроса
						 *
						 * @param addr объект параметров подключения инициатора запроса
						 * @return     идентификатор инициатора запроса
						 *
						 * \~english
						 * @brief Factory method creating the identifier of the request originator
						 *
						 * @param addr object of the connection parameters of the request originator
						 * @return     identifier of the request originator
						 *
						 * \~
						 */
						Origin & from(const net::attr_t * addr) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения
						 *
						 * @param other другой объект для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Comparison operator
						 *
						 * @param other another object to compare with
						 * @return      comparison result
						 *
						 * \~
						 */
						bool operator == (const Origin & other) const noexcept;
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
						explicit Origin() noexcept;
				} origin_t;
				/**
				 * \~russian
				 * @brief Специализация хеш-функции для структуры идентификатора инициатора запроса
				 *
				 * \~english
				 * @brief Specialization of the hash function for the structure of the request originator identifier
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Origin_Hash {
					public:
						/**
						 * \~russian
						 * @brief Оператор вычисления хеш-кода
						 *
						 * @param id объект для вычисления хеш-кода
						 * @return   хеш-код объекта
						 *
						 * \~english
						 * @brief Hash code computation operator
						 *
						 * @param id object to compute the hash code for
						 * @return   hash code of the object
						 *
						 * \~
						 */
						size_t operator()(const origin_t & id) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @note Ключевого слова explicit здесь быть не должно: unordered_map
						 *       создаёт объект хеш-функции списочной инициализацией, а та
						 *       явный конструктор не берёт
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * @note The explicit keyword must not be here: unordered_map creates the hash function
						 *       object by list initialization, and that does not take an explicit constructor
						 *
						 * \~
						 */
						Origin_Hash() noexcept = default;
				} origin_hash_t;
			private:
				/**
				 * \~russian
				 * @brief Структура для хранения информации о пирах
				 *
				 * @details Пир — это удалённый клиент, который подключился к прокси-серверу и выполняет через него свои запросы.
				 *
				 * \~english
				 * @brief Structure for storing the information about peers
				 *
				 * @details A peer is a remote client that has connected to the proxy server and performs its requests through it.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Peer {
					// Идентификатор события клиента для конечной точки
					event::id_t eid;
					// Идентификатор DNS-резолвера
					unit::dns_t::id_t did;
					// Контекст для хранения параметров сообщений
					proto::socks5_t::ctx_t ctx;
					// Контекст UDP-заголовка для текущего пира
					proto::socks5_t::udp_head_t udp;
					// Буфер накопления входящих SOCKS5-кадров по TCP
					vector <uint8_t> rx;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Peer() noexcept;
				} peer_t;
				/**
				 * \~russian
				 * @brief Структура для хранения информации о UDP-серверах
				 *
				 * @details UDP-серверы используются для обработки UDP-запросов от клиентов,
				 *          которые подключаются к прокси-серверу через протокол SOCKS5.
				 *          Каждый UDP-сервер имеет диапазон портов, на которых он может принимать запросы от клиентов.
				 *          При получении UDP-запроса от клиента, прокси-сервер выбирает свободный порт из диапазона и перенаправляет запрос на соответствующий UDP-сервер.
				 *          После обработки запроса, UDP-сервер отправляет ответ обратно на прокси-сервер, который затем пересылает его клиенту.
				 *
				 * \~english
				 * @brief Structure for storing the information about UDP servers
				 *
				 * @details UDP servers are used for processing UDP requests from clients
				 *          that connect to the proxy server through the SOCKS5 protocol.
				 *          Each UDP server has a range of ports on which it can accept requests from clients.
				 *          Upon receiving a UDP request from a client, the proxy server picks a free port from the range and forwards the request to the corresponding UDP server.
				 *          After processing the request, the UDP server sends the answer back to the proxy server, which then forwards it to the client.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ UDP_Server {
					// Начальный порт диапазона для выделения портов UDP серверов
					uint16_t begin;
					// Конечный порт диапазона для выделения портов UDP серверов
					uint16_t end;
					// Количество выделенных портов для UDP-серверов
					uint16_t count;
					// Список идентификаторов активных событий UDP-серверов
					vector <event::id_t> events;
					// Объект контекста заголовка UDP пакета
					proto::socks5_t::udp_head_t ctx;
					// Адрес для запуска UDP-серверов
					unique_ptr <net::addr_t> address;
					// Множество идентификаторов UDP-серверов для быстрой проверки
					unordered_set <event::id_t> eventSet;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit UDP_Server() noexcept;
				} udp_server_t;
			private:
				// Объект работы с сетью
				eth_t _eth;
			private:
				// Объект UDP-серверов для SOCKS5-прокси
				udp_server_t _udp;
			private:
				// Объект юнита клиента
				unit::client_t _client;
			private:
				// Объект для работы с протоколом SOCKS5
				proto::server_socks5_t _socks5;
			private:
				// Список для сопоставления идентификаторов пиров с удалёнными клиентами
				unordered_map <event::id_t, peer_t> _peers;
				// Список для сопоставления идентификаторов клиентов с пирами, которым они принадлежат
				unordered_map <event::id_t, event::id_t> _clients;
				// Список для сопоставления идентификаторов DNS-запросов с пирами
				unordered_map <unit::dns_t::id_t, event::id_t> _resolves;
			private:
				// Отображение идентификаторов событий клиентов для конечных точек
				unordered_map <event::id_t, origin_t> _mapping;
				// Алиасы для внутренних адресов если мы работаем за NAT
				unordered_map <origin_t, unique_ptr <net::attr_t>, origin_hash_t> _aliases;
				// Активные сессии клиентов, работающих через прокси
				unordered_map <origin_t, pair <event::id_t, event::id_t>, origin_hash_t> _sessions;
			private:
				/**
				 * \~russian
				 * @brief Метод удаления связи DNS-запроса с пиром
				 *
				 * @param did идентификатор DNS-запроса
				 *
				 * \~english
				 * @brief Method removing the association of a DNS request with a peer
				 *
				 * @param did DNS request identifier
				 *
				 * \~
				 */
				void dropResolve(const unit::dns_t::id_t did) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки SOCKS5-ответа прокси-клиенту
				 *
				 * @param eid      идентификатор пира
				 * @param ctx      контекст протокола SOCKS5
				 * @param dropPeer закрыть пира после ответа об ошибке
				 * @return         результат отправки ответа
				 *
				 * \~english
				 * @brief Method sending a SOCKS5 answer to the proxy client
				 *
				 * @param eid      peer identifier
				 * @param ctx      SOCKS5 protocol context
				 * @param dropPeer close the peer after an error answer
				 * @return         answer sending result
				 *
				 * \~
				 */
				bool sendReply(const event::id_t eid, proto::socks5_t::ctx_t & ctx, const bool dropPeer = false) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод изменения статуса сервера
				 *
				 * @param index  индекс очереди запускаемого события
				 * @param status новый статус сервера
				 *
				 * \~english
				 * @brief Server status change method
				 *
				 * @param index  index of the queue of the event being started
				 * @param status new server status
				 *
				 * \~
				 */
				void status(const uint8_t index, const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод инициализации запуска или остановки кластера
				 *
				 * @param pid   идентификатор процесса
				 * @param event событие кластера
				 *
				 * \~english
				 * @brief Method initializing the start or the stop of the cluster
				 *
				 * @param pid   process identifier
				 * @param event cluster event
				 *
				 * \~
				 */
				void eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
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
				void messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий подключения клиента к удалённому серверу
				 *
				 * @param eid идентификатор клиента
				 * @param ok  результат подключения
				 *
				 * \~english
				 * @brief Method processing the events of the client connecting to a remote server
				 *
				 * @param eid client identifier
				 * @param ok  connection result
				 *
				 * \~
				 */
				void connectClient(const event::id_t eid, const bool ok) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения состояния клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param status новый статус события
				 *
				 * \~english
				 * @brief Method processing the events of the client state changing
				 *
				 * @param eid    client event identifier
				 * @param status new event status
				 *
				 * \~
				 */
				void statusClient(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий получения данных клиентом
				 *
				 * @param eid    идентификатор клиента
				 * @param buffer буфер данных клиента
				 * @param size   размер данных клиента
				 *
				 * \~english
				 * @brief Method processing the events of data being received by the client
				 *
				 * @param eid    client identifier
				 * @param buffer client data buffer
				 * @param size   client data size
				 *
				 * \~
				 */
				void readClient(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения события ошибок
				 *
				 * @param eid     идентификатор события
				 * @param error   код ошибки
				 * @param message сообщение об ошибке
				 *
				 * \~english
				 * @brief Method obtaining the error event
				 *
				 * @param eid     event identifier
				 * @param error   error code
				 * @param message error message
				 *
				 * \~
				 */
				void errorClient(const event::id_t eid, const event::error_t error, const string & message) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий истечения таймаута клиента
				 *
				 * @param eid    идентификатор клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  задержка таймаута в миллисекундах
				 * @return       нужно ли завершить клиента после истечения таймаута
				 *
				 * \~english
				 * @brief Method processing the events of the client timeout expiring
				 *
				 * @param eid    client identifier
				 * @param action action type for the expired timeout
				 * @param delay  timeout delay in milliseconds
				 * @return       whether the client should be terminated after the timeout expires
				 *
				 * \~
				 */
				bool timeoutClient(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
			private:
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
				void accept(const event::id_t eid, const event::id_t cid) noexcept;
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
				void state(const event::id_t eid, const event::status_t status, void * ctx) noexcept;
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
				void read(const event::id_t eid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки неудачного резолвинга доменного имени
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param record тип записи DNS
				 * @param domain доменное имя
				 *
				 * \~english
				 * @brief Method processing an unsuccessful domain name resolving
				 *
				 * @param id     DNS request identifier
				 * @param record DNS record type
				 * @param domain domain name
				 *
				 * \~
				 */
				void failure(const unit::dns_t::id_t id, const unit::dns_t::record_t record, const string & domain) noexcept;
				/**
				 * \~russian
				 * @brief Метод резолвинга доменного имени удалённого хоста в сетевой адрес
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param family семейство адресов (IPv4/IPv6)
				 * @param domain доменное имя для резолвинга
				 * @param addr   указатель на структуру для хранения результата резолвинга
				 *
				 * \~english
				 * @brief Method resolving the domain name of a remote host into a network address
				 *
				 * @param id     DNS request identifier
				 * @param family address family (IPv4/IPv6)
				 * @param domain domain name to resolve
				 * @param addr   pointer to the structure for storing the resolving result
				 *
				 * \~
				 */
				void resolve(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
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
				void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска сервера
				 *
				 * \~english
				 * @brief Server starting method
				 *
				 * \~
				 */
				void start() noexcept;
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
				bool pause(const event::id_t eid) noexcept;
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
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события клиента для уничтожения
				 *
				 * \~english
				 * @brief Client event destruction method
				 *
				 * @param eid identifier of the client event to destroy
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
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
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения данных от клиента (заглушка для сервера SOCKS5)
				 *
				 * @return результат получения данных
				 *
				 * \~english
				 * @brief Method of receiving data from the client (a stub for the SOCKS5 server)
				 *
				 * @return data receiving result
				 *
				 * \~
				 */
				bool recv(const event::id_t) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных клиенту (заглушка для сервера SOCKS5)
				 *
				 * @return количество байт данных, отправленных клиенту
				 *
				 * \~english
				 * @brief Method of sending data to the client (a stub for the SOCKS5 server)
				 *
				 * @return number of data bytes sent to the client
				 *
				 * \~
				 */
				size_t send(const event::id_t, const void *, const size_t) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перемещения данных между сервером и другим событием (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения перемещения
				 *
				 * \~english
				 * @brief Method of moving data between the server and another event (a stub for the SOCKS5 server)
				 *
				 * @return result of performing the moving
				 *
				 * \~
				 */
				bool splice(const event::id_t, const event::id_t) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции клиента
				 *
				 * \~english
				 * @brief Client options obtaining method
				 *
				 * @param eid client event identifier
				 * @return    client options
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции клиента для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Client options setting method
				 *
				 * @param eid     client event identifier
				 * @param options client options to set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция клиента для установки
				 * @param mode   режим установки опции клиента
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Client option setting method
				 *
				 * @param eid    client event identifier
				 * @param option client option to set
				 * @param mode   client option setting mode
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса для подключения к сети клиентов
				 *
				 * @return сетевой интерфейс сервера
				 *
				 * \~english
				 * @brief Method obtaining the network interface for connecting to the network of clients
				 *
				 * @return server network interface
				 *
				 * \~
				 */
				string getIface() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    сетевой интерфейс сервера
				 *
				 * \~english
				 * @brief Method obtaining the server network interface
				 *
				 * @param eid server event identifier
				 * @return    server network interface
				 *
				 * \~
				 */
				string getIface(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса для подключения к сети клиентов
				 *
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method setting the network interface for connecting to the network of clients
				 *
				 * @param name name of the network interface to set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(string_view name) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method setting the server network interface
				 *
				 * @param eid  server event identifier
				 * @param name name of the network interface to set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
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
				uint16_t getSourcePort() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения внутреннего порта клиента, подключённого к серверу
				 *
				 * @param eid идентификатор события клиента
				 * @return    внутренний порт клиента
				 *
				 * \~english
				 * @brief Method obtaining the internal port of a client connected to the server
				 *
				 * @param eid client event identifier
				 * @return    internal port of the client
				 *
				 * \~
				 */
				uint16_t getSourcePort(const event::id_t eid) const noexcept;
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
				bool setSourcePort(const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения порта удалённого клиента или текущего сервера
				 *
				 * @param eid идентификатор события клиента или сервера
				 * @return    порт удалённого клиента или текущего сервера
				 *
				 * \~english
				 * @brief Method obtaining the port of a remote client or of the current server
				 *
				 * @param eid identifier of the client or server event
				 * @return    port of the remote client or of the current server
				 *
				 * \~
				 */
				uint16_t getTargetPort(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 *
				 * \~english
				 * @brief Method obtaining the host address of the target machine
				 *
				 * @param eid client event identifier
				 * @return    host address of the target machine
				 *
				 * \~
				 */
				string getTarget(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 * \~english
				 * @brief Method obtaining the host address of the target machine
				 *
				 * @param eid    client event identifier
				 * @param target object for extracting the host address of the target machine
				 * @return       result of performing the extraction of the host address of the target machine
				 *
				 * \~
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
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
				bool setAddress(const event::address_t address, string_view value) noexcept;
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
				bool setAddress(const event::address_t address, const net::addr_t * value) noexcept;
			public:
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
				 * @brief Server address setting method
				 *
				 * @param eid     server event identifier
				 * @param address server address type
				 * @param value   server address value
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
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Server address setting method
				 *
				 * @param eid     server event identifier
				 * @param address server address type
				 * @param value   server address value
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
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
				string getAddress(const event::address_t address) const noexcept;
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
				bool getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента или текущего сервера
				 *
				 * @param eid     идентификатор события клиента или сервера
				 * @param address тип адреса клиента или сервера
				 * @return        значение адреса клиента или сервера
				 *
				 * \~english
				 * @brief Method obtaining the address of a client or of the current server
				 *
				 * @param eid     identifier of the client or server event
				 * @param address type of the client or server address
				 * @return        value of the client or server address
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента или текущего сервера
				 *
				 * @param eid     идентификатор события клиента или сервера
				 * @param address тип адреса клиента или сервера
				 * @param value   объект для извлечения адреса клиента или сервера
				 * @return        результат выполнения извлечения адреса клиента или сервера
				 *
				 * \~english
				 * @brief Method obtaining the address of a client or of the current server
				 *
				 * @param eid     identifier of the client or server event
				 * @param address type of the client or server address
				 * @param value   object for extracting the address of the client or server
				 * @return        result of performing the extraction of the address of the client or server
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
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
				 * @brief Client buffer size obtaining method
				 *
				 * @param eid    client event identifier
				 * @param action client action type
				 * @return       client buffer size
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
				 * @brief Client buffer size setting method
				 *
				 * @param eid    client event identifier
				 * @param action client action type
				 * @param size   client buffer size
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
				 * @return режим использования таймаута на чтение события
				 *
				 * \~english
				 * @brief Method obtaining the usage mode of the event read timeout
				 *
				 * @return usage mode of the event read timeout
				 *
				 * \~
				 */
				event::usage_t getUsageReadTimeout() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения режима использования таймаута на чтение события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим использования таймаута на чтение события клиента
				 *
				 * \~english
				 * @brief Method obtaining the usage mode of the client event read timeout
				 *
				 * @param eid client event identifier
				 * @return    usage mode of the client event read timeout
				 *
				 * \~
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
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
				void setUsageReadTimeout(const event::usage_t usage) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима использования таймаута на чтение события клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param usage режим использования таймаута на чтение события клиента (reusable или disposable)
				 *
				 * \~english
				 * @brief Method setting the usage mode of the client event read timeout
				 *
				 * @param eid   client event identifier
				 * @param usage usage mode of the client event read timeout (reusable or disposable)
				 *
				 * \~
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
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
				uint32_t getTimeout(const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Client timeout obtaining method
				 *
				 * @param eid    client event identifier
				 * @param action client action type
				 * @return       timeout value in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
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
				void setTimeout(const event::action_t action, const uint32_t timeout) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Client timeout setting method
				 *
				 * @param eid     client event identifier
				 * @param action  client action type
				 * @param timeout timeout value in milliseconds
				 *
				 * \~
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
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
				bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 * \~english
				 * @brief Client bandwidth setting method
				 *
				 * @param eid       client event identifier
				 * @param limiting  client bandwidth limiting mode (egress or ingress)
				 * @param bandwidth client bandwidth to set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
				 * @return          result of performing the setting
				 *
				 * \~
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
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
				 * @brief Method setting the keep-alive parameters for the client
				 *
				 * @param eid   client event identifier
				 * @param cnt   number of keep-alive packets
				 * @param idle  idle time before sending the first keep-alive packet in seconds
				 * @param intvl interval between keep-alive packets in seconds
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group (a stub for the SOCKS5 server)
				 *
				 * @return result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group (a stub for the SOCKS5 server)
				 *
				 * @return result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept;
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
				 * @brief Method sending a message to a child process
				 *
				 * @param pid    identifier of the process to receive the message
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
				 * @brief Method sending a message to all child processes
				 *
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
				 * @brief Метод установки диапазона портов для выделения портов UDP серверов
				 *
				 * @param count количество портов для выделения
				 * @param begin начальный порт диапазона для выделения
				 * @param end   конечный порт диапазона для выделения
				 * @param addr  адрес для запуска UDP-серверов
				 *
				 * \~english
				 * @brief Method setting the range of ports for allocating the ports of UDP servers
				 *
				 * @param count number of ports to allocate
				 * @param begin starting port of the range for allocation
				 * @param end   ending port of the range for allocation
				 * @param addr  address for starting the UDP servers
				 *
				 * \~
				 */
				void udp(const uint16_t count, const uint16_t begin, const uint16_t end, string_view addr) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки диапазона портов для выделения портов UDP серверов
				 *
				 * @param count количество портов для выделения
				 * @param begin начальный порт диапазона для выделения
				 * @param end   конечный порт диапазона для выделения
				 * @param addr  адрес для запуска UDP-серверов
				 *
				 * \~english
				 * @brief Method setting the range of ports for allocating the ports of UDP servers
				 *
				 * @param count number of ports to allocate
				 * @param begin starting port of the range for allocation
				 * @param end   ending port of the range for allocation
				 * @param addr  address for starting the UDP servers
				 *
				 * \~
				 */
				void udp(const uint16_t count, const uint16_t begin, const uint16_t end, const net::addr_t * addr) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки алиаса для внутреннего адреса при работе за NAT
				 *
				 * @param addr  объект параметров подключения внутреннего адреса
				 * @param alias объект параметров подключения алиаса для внутреннего адреса
				 *
				 * \~english
				 * @brief Method setting an alias for an internal address when working behind NAT
				 *
				 * @param addr  object of the connection parameters of the internal address
				 * @param alias object of the connection parameters of the alias for the internal address
				 *
				 * \~
				 */
				void setAlias(const net::attr_t * addr, const net::attr_t * alias) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки алиаса для внутреннего адреса при работе за NAT
				 *
				 * @param addr    внутренний адрес работающий за NAT
				 * @param intPort порт внутреннего адреса работающий за NAT
				 * @param alias   внешний адрес для алиаса внутреннего адреса
				 * @param extPort внешний порт для алиаса внутреннего адреса
				 *
				 * \~english
				 * @brief Method setting an alias for an internal address when working behind NAT
				 *
				 * @param addr    internal address working behind NAT
				 * @param intPort port of the internal address working behind NAT
				 * @param alias   external address for the alias of the internal address
				 * @param extPort external port for the alias of the internal address
				 *
				 * \~
				 */
				void setAlias(string_view addr, const uint16_t intPort, string_view alias, const uint16_t extPort = 0) noexcept;
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
				Socks5(const Socks5 &) = delete;
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
				Socks5 & operator = (const Socks5 &) = delete;
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
				explicit Socks5(const fmk_t * fmk, const log_t * log) noexcept;
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
				explicit Socks5(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
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
				~Socks5() noexcept;
		} socks5_t;
	};
};

#endif // __AWH_SERVER_SOCKS5__
