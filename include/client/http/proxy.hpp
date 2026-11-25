/**
 * @file httpproxy.hpp
 * @date 2026-11-23
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
 * @brief Заголовочный файл клиента HTTP-прокси — публичный API класса client::HttpProxy,
 *        выполняющего установку туннеля через прокси-сервер методом CONNECT, пересылку обычных
 *        HTTP-запросов в absolute-form, авторизацию на прокси (Basic/Digest/Bearer)
 *        и проксирование прикладного трафика поверх базового клиента
 *
 * \~english
 * @brief Header file of the HTTP proxy client — the public API of the client::HttpProxy class
 *        performing the establishment of a tunnel through a proxy server by the method CONNECT, the forwarding of ordinary
 *        HTTP requests in absolute-form, the authorization on the proxy (Basic/Digest/Bearer)
 *        and the proxying of application traffic on top of the base client
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовка
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../client.hpp"
#include "../../proto/http/proxy/client.hpp"

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
		 * @brief Класс клиента HTTP-прокси
		 *
		 * @details Подключается к прокси-серверу (при необходимости поверх TLS),
		 *          устанавливает туннель к целевому хосту методом CONNECT либо
		 *          пересылает обычные HTTP-запросы в absolute-form. После установки
		 *          туннеля прикладной трафик идёт прозрачно, при необходимости
		 *          поверх отдельного TLS-соединения с целевым хостом.
		 *
		 * \~english
		 * @brief HTTP proxy client class
		 *
		 * @details Connects to a proxy server (over TLS if required),
		 *          establishes a tunnel to the target host by the method CONNECT or
		 *          forwards ordinary HTTP requests in absolute-form. After the establishment
		 *          of the tunnel the application traffic goes transparently, if required
		 *          over a separate TLS connection with the target host.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ HttpProxy : public client_t {
			private:
				// Идентификатор контекста безопасности TLS к целевому хосту
				tls::coder::id_t _ctl;
			private:
				// Объект для работы с протоколом HTTP-прокси
				http::proxy::client_t _proxy;
			private:
				// Контекст для хранения параметров обмена с прокси
				http::proxy::client_t::ctx_t _ctx;
			private:
				// Конечная точка клиента, работающего через прокси
				unique_ptr <net::attr_t> _endpoint;
			private:
				/**
				 * \~russian
				 * @brief Метод изменения статуса клиента
				 *
				 * @param index  индекс очереди запускаемого события
				 * @param status новый статус клиента
				 *
				 * \~english
				 * @brief Method of the changing of the client status
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
				 * @brief Метод обработки событий подключения клиента к прокси-серверу
				 *
				 * @param eid идентификатор клиента
				 * @param ok  результат подключения
				 *
				 * \~english
				 * @brief Method processing the events of the client connecting to a proxy server
				 *
				 * @param eid client identifier
				 * @param ok  connection result
				 *
				 * \~
				 */
				void connect(const event::id_t eid, const bool ok) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий записи данных клиентом
				 *
				 * @param eid  идентификатор клиента
				 * @param size размер данных для записи
				 *
				 * \~english
				 * @brief Method processing the events of data being written by the client
				 *
				 * @param eid  client identifier
				 * @param size size of the data to write
				 *
				 * \~
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
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
				 * @brief Метод разрешения доменного имени прокси-сервера в сетевой адрес
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param family семейство адресов (IPv4/IPv6)
				 * @param domain доменное имя для разрешения
				 * @param addr   указатель на структуру для хранения результата разрешения
				 *
				 * \~english
				 * @brief Method resolving the domain name of a proxy server into a network address
				 *
				 * @param id     DNS request identifier
				 * @param family address family (IPv4/IPv6)
				 * @param domain domain name to resolve
				 * @param addr   pointer to the structure for storing the resolution result
				 *
				 * \~
				 */
				void resolve(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод получения состояния TLS к прокси-серверу
				 *
				 * @param ctl   идентификатор TLS
				 * @param state состояние TLS
				 *
				 * \~english
				 * @brief Method of the obtaining of the TLS state for the proxy server
				 *
				 * @param ctl   TLS identifier
				 * @param state TLS state
				 *
				 * \~
				 */
				void stateTLS(const tls::coder::id_t ctl, const tls::coder::state_t state) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения событий шифрования/дешифрования данных TLS к прокси
				 *
				 * @param ctl    идентификатор TLS
				 * @param event  тип события TLS
				 * @param buffer буфер данных для события шифрования/дешифрования TLS
				 * @param size   размер данных для события шифрования/дешифрования TLS
				 *
				 * \~english
				 * @brief Method obtaining the events of TLS data encryption/decryption for the proxy
				 *
				 * @param ctl    TLS identifier
				 * @param event  TLS event type
				 * @param buffer data buffer for the TLS encryption/decryption event
				 * @param size   data size for the TLS encryption/decryption event
				 *
				 * \~
				 */
				void processTLS(const tls::coder::id_t ctl, const tls::coder::event_t event, const uint8_t * buffer, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод получения состояния TLS к целевому хосту
				 *
				 * @param ctl   идентификатор TLS
				 * @param state состояние TLS
				 *
				 * \~english
				 * @brief Method of the obtaining of the TLS state for the target host
				 *
				 * @param ctl   TLS identifier
				 * @param state TLS state
				 *
				 * \~
				 */
				void stateTargetTLS(const tls::coder::id_t ctl, const tls::coder::state_t state) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения ошибок TLS к целевому хосту
				 *
				 * @param ctl     идентификатор TLS
				 * @param error   код ошибки TLS
				 * @param message сообщение об ошибке TLS
				 *
				 * \~english
				 * @brief Method of the obtaining of the TLS errors for the target host
				 *
				 * @param ctl     TLS identifier
				 * @param error   TLS error code
				 * @param message TLS error message
				 *
				 * \~
				 */
				void errorTargetTLS(const tls::coder::id_t ctl, const tls::coder::error_t error, const string & message) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения событий шифрования/дешифрования данных TLS к цели
				 *
				 * @param ctl    идентификатор TLS
				 * @param event  тип события TLS
				 * @param buffer буфер данных для события шифрования/дешифрования TLS
				 * @param size   размер данных для события шифрования/дешифрования TLS
				 *
				 * \~english
				 * @brief Method obtaining the events of TLS data encryption/decryption for the target
				 *
				 * @param ctl    TLS identifier
				 * @param event  TLS event type
				 * @param buffer data buffer for the TLS encryption/decryption event
				 * @param size   data size for the TLS encryption/decryption event
				 *
				 * \~
				 */
				void processTargetTLS(const tls::coder::id_t ctl, const tls::coder::event_t event, const uint8_t * buffer, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод выдачи данных туннеля прикладному обмену
				 *
				 * @param buffer буфер данных туннеля
				 * @param size   размер данных туннеля
				 *
				 * \~english
				 * @brief Method of the issuing of the data of the tunnel to the application exchange
				 *
				 * @param buffer data buffer of the tunnel
				 * @param size   size of the data of the tunnel
				 *
				 * \~
				 */
				void tunnel(const uint8_t * buffer, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод отправки запроса к прокси-серверу
				 *
				 * @details Извлекает сформированный кодеком запрос и отправляет его
				 *          прокси-серверу (поверх TLS к прокси, если он установлен)
				 *
				 * \~english
				 * @brief Method of the sending of a request to a proxy server
				 *
				 * @details Extracts the request formed by the codec and sends it
				 *          to the proxy server (over TLS to the proxy if it is established)
				 *
				 * \~
				 */
				void sendRequest() noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных прокси-серверу
				 *
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных прокси-серверу
				 *
				 * \~english
				 * @brief Method of the sending of data to a proxy server
				 *
				 * @param buffer data buffer to send
				 * @param size   size of the data to send
				 * @return       number of data bytes sent to the proxy server
				 *
				 * \~
				 */
				size_t sendToProxy(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод приостановки работы клиента
				 *
				 * @return результат выполнения приостановки работы
				 *
				 * \~english
				 * @brief Method of the suspension of the client operation
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
				 * @brief Method of the resumption of the client operation
				 *
				 * @return result of performing the operation resumption
				 *
				 * \~
				 */
				bool resume() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод мультиподключения клиентов к удалённым хостам (заглушка для клиента HTTP-прокси)
				 *
				 * @return результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of multi-connecting clients to remote hosts (a stub for the HTTP proxy client)
				 *
				 * @return result of performing the connection
				 *
				 * \~
				 */
				bool connect() noexcept;
				/**
				 * \~russian
				 * @brief Метод отключения клиента от удалённого сервера (заглушка для клиента HTTP-прокси)
				 *
				 * @return результат выполнения отключения
				 *
				 * \~english
				 * @brief Method disconnecting the client from a remote server (a stub for the HTTP proxy client)
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
				 * @details В режиме TUNNEL отправляет прикладные данные через туннель
				 *          (после его установки). В режиме FORWARD принимает HTTP-запрос
				 *          пользователя и пересылает его прокси в absolute-form
				 *
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных серверу
				 *
				 * \~english
				 * @brief Method of sending data to the server
				 *
				 * @details In the mode TUNNEL sends application data through the tunnel
				 *          (after its establishment). In the mode FORWARD accepts an HTTP request
				 *          of the user and forwards it to the proxy in absolute-form
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
				 * @brief Метод объединения данных между клиентом и другим событием (заглушка для клиента HTTP-прокси)
				 *
				 * @return результат выполнения объединения
				 *
				 * \~english
				 * @brief Method of splicing data between the client and another event (a stub for the HTTP proxy client)
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
				 * @brief Method of the setting of the client bandwidth
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
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента HTTP-прокси)
				 *
				 * @return результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group (a stub for the HTTP proxy client)
				 *
				 * @return result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента HTTP-прокси)
				 *
				 * @return результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group (a stub for the HTTP proxy client)
				 *
				 * @return result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки контекста безопасности TLS к целевому хосту
				 *
				 * @details TLS к целевому хосту поднимается поверх установленного туннеля
				 *          и не зависит от TLS к самому прокси-серверу
				 *
				 * @param ctl идентификатор контекста безопасности TLS к целевому хосту
				 * @return    результат выполнения установки
				 *
				 * \~english
				 * @brief Method of the setting of the security context of TLS to the target host
				 *
				 * @details TLS to the target host is raised on top of the established tunnel
				 *          and does not depend on TLS to the proxy server itself
				 *
				 * @param ctl identifier of the security context of TLS to the target host
				 * @return    result of performing the setting
				 *
				 * \~
				 */
				bool targetTLS(const tls::coder::id_t ctl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки токена доступа BEARER для авторизации на прокси
				 *
				 * @param token токен доступа для авторизации на прокси
				 *
				 * \~english
				 * @brief Method of the setting of the access token BEARER for the authorization on the proxy
				 *
				 * @param token access token for the authorization on the proxy
				 *
				 * \~
				 */
				void setToken(const string & token) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки параметров авторизации на прокси
				 *
				 * @param username имя пользователя для авторизации на прокси
				 * @param password пароль пользователя для авторизации на прокси
				 *
				 * \~english
				 * @brief Method of the setting of the authorization parameters on the proxy
				 *
				 * @param username user name for authorization on the proxy
				 * @param password user password for authorization on the proxy
				 *
				 * \~
				 */
				void setUser(const string & username, const string & password) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки режима превентивной авторизации на прокси
				 *
				 * @param mode флаг превентивной отправки учётных данных
				 *
				 * \~english
				 * @brief Method of the setting of the mode of the preemptive authorization on the proxy
				 *
				 * @param mode flag of the preemptive sending of the credentials
				 *
				 * \~
				 */
				void preemptive(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима работы через прокси
				 *
				 * @param mode режим работы (TUNNEL — туннель CONNECT, FORWARD — пересылка запросов)
				 *
				 * \~english
				 * @brief Method of the setting of the mode of the work through a proxy
				 *
				 * @param mode mode of the work (TUNNEL — tunnel CONNECT, FORWARD — forwarding of requests)
				 *
				 * \~
				 */
				void mode(const http::proxy::client_t::mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки конечной точки клиента
				 *
				 * @param attr параметры подключения для установки конечной точки
				 * @return     результат выполнения установки конечной точки
				 *
				 * \~english
				 * @brief Method of the setting of the client endpoint
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
				 * @brief Method of the setting of the client endpoint
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
				HttpProxy(const HttpProxy &) = delete;
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
				HttpProxy & operator = (const HttpProxy &) = delete;
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
				explicit HttpProxy() noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param dns объект DNS-резолвера
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param dns DNS resolver object
				 *
				 * \~
				 */
				explicit HttpProxy(unit::dns_t * dns) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param ctl идентификатор контекста безопасности TLS к прокси-серверу
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param ctl security context identifier of TLS to the proxy server
				 *
				 * \~
				 */
				explicit HttpProxy(const tls::coder::id_t ctl) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param ctl идентификатор контекста безопасности TLS к прокси-серверу
				 * @param dns объект DNS-резолвера
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param ctl security context identifier of TLS to the proxy server
				 * @param dns DNS resolver object
				 *
				 * \~
				 */
				explicit HttpProxy(const tls::coder::id_t ctl, unit::dns_t * dns) noexcept;
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
				~HttpProxy() noexcept;
		} http_proxy_t;
	};
};
