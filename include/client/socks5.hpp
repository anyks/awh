/**
 * @file: socks5.hpp
 * @date: 2026-05-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл клиента SOCKS5-прокси — публичный API класса client::Socks5,
 *        выполняющего согласование методов авторизации, установку туннеля через прокси-сервер (CONNECT, BIND,
 *        UDP ASSOCIATE) и проксирование прикладного трафика поверх базового клиента
 *
 * \~english
 * @brief Header file of the SOCKS5 proxy client — the public API of the client::Socks5 class
 *        performing the negotiation of authorization methods, the establishment of a tunnel through a proxy server (CONNECT, BIND,
 *        UDP ASSOCIATE) and the proxying of application traffic on top of the base client
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовка
 */
#ifndef __AWH_CLIENT_SOCKS5__
#define __AWH_CLIENT_SOCKS5__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "client.hpp"
#include "../proto/socks5/client.hpp"

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
	 * @brief Пространство имён клиента
	 *
	 * \~english
	 * @brief Client namespace
	 *
	 * \~
	 */
	namespace client {
		/**
		 * \~russian
		 * @brief Класс клиента SOCKS5-прокси
		 *
		 * \~english
		 * @brief SOCKS5 proxy client class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Socks5 : public client_t {
			private:
				/**
				 * \~russian
				 * @brief Структура конечной точки клиента, работающего через прокси
				 *
				 * \~english
				 * @brief Structure of the endpoint of a client working through a proxy
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Endpoint {
					/**
					 * \~russian
					 * @brief Идентификатор события UDP-клиента
					 *
					 * \~english
					 * @brief Identifier of the UDP client event
					 *
					 * \~
					 */
					struct {
						// Идентификатор события клиента
						event::id_t eid = 0;
						// Объект контекста заголовка UDP пакета
						proto::socks5_t::udp_head_t ctx{};
					} udp;
					// Атрибуты сети для конечной точки
					unique_ptr <net::attr_t> attr;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Endpoint() noexcept;
				} endpoint_t;
			private:
				// Конечная точка клиента, работающего через прокси
				endpoint_t _endpoint;
			private:
				// Контекст для хранения параметров сообщений
				proto::socks5_t::ctx_t _ctx;
			private:
				// Объект для работы с протоколом SOCKS5
				proto::client_socks5_t _socks5;
			private:
				// Буфер накопления входящих SOCKS5-кадров по TCP
				vector <uint8_t> _rx;
			private:
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
				void status(const uint8_t index, const event::status_t status) noexcept;
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
				void connect(const event::id_t eid, const bool ok) noexcept;
			private:
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
				void write(const event::id_t, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения состояния клиента
				 *
				 * @param eid    идентификатор клиента
				 * @param status новый статус клиента
				 *
				 * \~english
				 * @brief Method processing the events of the client state changing
				 *
				 * @param eid    client identifier
				 * @param status new client status
				 *
				 * \~
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
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
				void read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разрешения доменного имени удалённого хоста в сетевой адрес
				 *
				 * @param        идентификатор DNS-запроса
				 * @param family семейство адресов (IPv4/IPv6)
				 * @param domain доменное имя для разрешения
				 * @param addr   указатель на структуру для хранения результата разрешения
				 *
				 * \~english
				 * @brief Method resolving the domain name of a remote host into a network address
				 *
				 * @param        DNS request identifier
				 * @param family address family (IPv4/IPv6)
				 * @param domain domain name to resolve
				 * @param addr   pointer to the structure for storing the resolution result
				 *
				 * \~
				 */
				void resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
			private:
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
				void stateTLS(const tls::coder_t::id_t, const tls::coder_t::state_t state) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения событий шифрования/дешифрования данных TLS
				 *
				 * @param        идентификатор TLS
				 * @param event  тип события TLS
				 * @param buffer буфер данных для события шифрования/дешифрования TLS
				 * @param size   размер данных для события шифрования/дешифрования TLS
				 *
				 * \~english
				 * @brief Method obtaining the events of TLS data encryption/decryption
				 *
				 * @param        TLS identifier
				 * @param event  TLS event type
				 * @param buffer data buffer for the TLS encryption/decryption event
				 * @param size   data size for the TLS encryption/decryption event
				 *
				 * \~
				 */
				void processTLS(const tls::coder_t::id_t, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept;
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
				bool pause() noexcept;
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
				bool resume() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод мультиподключения клиентов к удалённым хостам (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of multi-connecting clients to remote hosts (a stub for the SOCKS5 client)
				 *
				 * @return result of performing the connection
				 *
				 * \~
				 */
				bool connect() noexcept;
				/**
				 * \~russian
				 * @brief Метод отключения клиента от удалённого сервера (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения отключения
				 *
				 * \~english
				 * @brief Method disconnecting the client from a remote server (a stub for the SOCKS5 client)
				 *
				 * @return result of performing the disconnection
				 *
				 * \~
				 */
				bool disconnect() noexcept;
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
				bool recv() noexcept;
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
				size_t send(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод объединения данных между клиентом и другим событием (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения объединения
				 *
				 * \~english
				 * @brief Method of splicing data between the client and another event (a stub for the SOCKS5 client)
				 *
				 * @return result of performing the splicing
				 *
				 * \~
				 */
				bool splice(const event::id_t, const event::direct_t) noexcept;
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
				bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group (a stub for the SOCKS5 client)
				 *
				 * @return result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group (a stub for the SOCKS5 client)
				 *
				 * @return result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки параметров авторизации
				 *
				 * @param username имя пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 *
				 * \~english
				 * @brief Authorization parameters setting method
				 *
				 * @param username user name for authorization on the server
				 * @param password user password for authorization on the server
				 *
				 * \~
				 */
				void setUser(const string & username, const string & password) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки исходящего адреса для UDP-клиента
				 *
				 * @param addr исходящий адрес для UDP-клиента
				 * @return 	   результат выполнения установки исходящего адреса для UDP-клиента
				 *
				 * \~english
				 * @brief Method of setting the outgoing address for the UDP client
				 *
				 * @param addr outgoing address for the UDP client
				 * @return     result of performing the setting of the outgoing address for the UDP client
				 *
				 * \~
				 */
				bool udp(const net::attr_net_t * addr) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки исходящего адреса для UDP-клиента
				 *
				 * @param addr исходящий адрес для UDP-клиента
				 * @param port исходящий порт для UDP-клиента
				 * @return     результат выполнения установки исходящего адреса для UDP-клиента
				 *
				 * \~english
				 * @brief Method of setting the outgoing address for the UDP client
				 *
				 * @param addr outgoing address for the UDP client
				 * @param port outgoing port for the UDP client
				 * @return     result of performing the setting of the outgoing address for the UDP client
				 *
				 * \~
				 */
				bool udp(string_view addr, const uint16_t port = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки конечной точки клиента
				 *
				 * @param attr параметры подключения для установки конечной точки
				 * @return     результат выполнения установки конечной точки
				 *
				 * \~english
				 * @brief Client endpoint setting method
				 *
				 * @param attr connection parameters for setting the endpoint
				 * @return     result of performing the endpoint setting
				 *
				 * \~
				 */
				bool endpoint(const net::attr_t * attr) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки конечной точки клиента
				 *
				 * @param addr адрес хоста для установки
				 * @param port порт хоста для установки
				 * @return     результат выполнения установки конечной точки
				 *
				 * \~english
				 * @brief Client endpoint setting method
				 *
				 * @param addr host address to set
				 * @param port host port to set
				 * @return     result of performing the endpoint setting
				 *
				 * \~
				 */
				bool endpoint(string_view addr, const uint16_t port) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещён)
				 *
				 * \~english
				 * @brief Copy constructor (forbidden)
				 *
				 * \~
				 */
				Socks5(const Socks5 &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещён)
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
				explicit Socks5(const tls::coder_t::id_t ctl, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
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
				explicit Socks5(const tls::coder_t::id_t ctl, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
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

#endif // __AWH_CLIENT_SOCKS5__
