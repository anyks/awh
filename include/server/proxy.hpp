/**
 * @file: proxy.hpp
 * @date: 2023-11-06
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_HTTP_PROXY__
#define __AWH_HTTP_PROXY__

/**
 * Стандартные модули
 */
#include <set>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * Наши модули
 */
#include "awh.hpp"
#include "../client/awh.hpp"
#include "../sys/queue.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * server клиентское пространство имён
	 */
	namespace server {
		/**
		 * Proxy Класс Proxy-сервера
		 */
		typedef class AWHSHARED_EXPORT Proxy {
			public:
				/**
				 * Брокеры учавствующие в передаче данных
				 */
				enum class broker_t : uint8_t {
					CLIENT = 0x01, // Агент является клиентом
					SERVER = 0x02  // Агент является сервером
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE                        = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO                     = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP                     = 0x03, // Флаг запрета остановки биндинга
					NOT_PING                     = 0x04, // Флаг запрещающий выполнение пингов
					SYNCPROTO                    = 0x05, // Флаг синхронизации протоколов клиента и сервера
					REDIRECTS                    = 0x06, // Флаг разрешающий автоматическое перенаправление запросов
					RECOMPRESS                   = 0x07, // Флаг выполнения рекомпрессинга передаваемых данных
					CONNECT_METHOD_CLIENT_ENABLE = 0x08, // Флаг разрешающий метод CONNECT на прокси-клиенте
					CONNECT_METHOD_SERVER_ENABLE = 0x09  // Флаг разрешающий метод CONNECT на сервере
				};
			private:
				/**
				 * Request Объект параметров запроса
				 */
				typedef struct Request {
					awh::web_t::req_t params;                         // Параметры запроса
					vector <char> entity;                             // Тело запроса
					std::unordered_multimap <string, string> headers; // Заголовки запроса
				} request_t;
				/**
				 * Response Объект параметров ответа
				 */
				typedef struct Response {
					awh::web_t::res_t params;                         // Параметры ответа
					vector <char> entity;                             // Тело ответа
					std::unordered_multimap <string, string> headers; // Заголовки ответа
				} response_t;
				/**
				 * KeepAlive Структура параметров жизни подключения
				 */
				typedef struct KeepAlive {
					int32_t cnt;   // Максимальное количество попыток
					int32_t idle;  // Интервал времени в секундах через которое происходит проверка подключения
					int32_t intvl; // Интервал времени в секундах между попытками
					/**
					 * KeepAlive Конструктор
					 */
					KeepAlive() noexcept : cnt(0), idle(0), intvl(0) {}
				} __attribute__((packed)) ka_t;
				/**
				 * DNS Структура параметров DNS-резолвера
				 */
				typedef struct DNS {
					uint8_t timeout;                                    // Время ожидания выполнения запроса
					string hosts;                                       // Адрес файла с локальными хостами
					string prefix;                                      // Префикс переменной окружения для извлечения серверов имён
					std::unordered_multimap <string, string> blacklist; // Чёрный список доменных имён
					/**
					 * DNS Конструктор
					 */
					DNS() noexcept : timeout(0), hosts{""}, prefix{""} {}
				} dns_t;
				/**
				 * Ident Структура идентификации сервиса
				 */
				typedef struct Ident {
					string id;   // Идентификатор сервиса
					string ver;  // Версия сервиса
					string name; // Название сервиса
					/**
					 * Ident Конструктор
					 */
					Ident() noexcept :
					 id{AWH_SHORT_NAME},
					 ver{AWH_VERSION}, name{AWH_NAME} {}
				} ident_t;
				/**
				 * Auth Структура параметров авторизации
				 */
				typedef struct Auth {
					awh::auth_t::type_t type; // Тип авторизации
					awh::auth_t::hash_t hash; // Алгоритм шифрования для Digest-авторизации
					/**
					 * Auth Конструктор
					 */
					Auth() noexcept :
					 type(awh::auth_t::type_t::BASIC),
					 hash(awh::auth_t::hash_t::MD5) {}
				} __attribute__((packed)) auth_t;
				/**
				 * WaitTimeDetect Структура таймаутов на обмен данными в миллисекундах
				 */
				typedef struct WaitTimeDetect {
					uint16_t wait;    // Время ожидания получения данных
					uint16_t read;    // Время ожидания на получение данных
					uint16_t write;   // Время ожидания на отправку данных
					uint16_t connect; // Время ожидания подключения
					/**
					 * WaitTimeDetect Конструктор
					 */
					WaitTimeDetect() noexcept :
					 wait(0), read(READ_TIMEOUT),
					 write(WRITE_TIMEOUT),
					 connect(CONNECT_TIMEOUT) {}
				} __attribute__((packed)) wtd_t;
				/**
				 * ProxyClient Структура параметров прокси-клиента
				 */
				typedef struct ProxyClient {
					string uri;                    // Параметры запроса на прокси-сервер
					auth_t auth;                   // Параметры авторизации на прокси-сервере
					scheme_t::family_t family;     // Cемейстово интернет протоколов (IPV4 / IPV6 / IPC)
					client::scheme_t::work_t work; // Флаг активации-деактивации прокси-клиента
					/**
					 * ProxyClient Конструктор
					 */
					ProxyClient() noexcept :
					 uri{""}, family(scheme_t::family_t::IPV4),
					 work(client::scheme_t::work_t::DISALLOW) {}
				} proxy_t;
				/**
				 * Encryption Структура параметров шифрования
				 */
				typedef struct Encryption {
					bool mode;               // Флаг активности механизма шифрования
					string pass;             // Пароль шифрования передаваемых данных
					string salt;             // Соль шифрования передаваемых данных
					hash_t::cipher_t cipher; // Размер шифрования передаваемых данных
					/**
					 * Encryption Конструктор
					 */
					Encryption() noexcept :
					 mode(false), pass{""}, salt{""},
					 cipher(hash_t::cipher_t::AES128) {}
				} encryption_t;
				/**
				 * Client Объект клиента
				 */
				typedef struct Client {
					// Идентификатор потока
					int32_t sid;
					// Флаг постановки занятости
					bool busy;
					// Флаг отправки результата
					bool sending;
					// Флаг переключения протокола
					bool upgrade;
					// Активный метод подключения
					awh::web_t::method_t method;
					// Агент активного клиента
					client::web_t::agent_t agent;
					// Объект параметров запроса
					request_t request;
					// Объект параметров ответа
					response_t response;
					// Объект сетевого ядра
					client::core_t core;
					// Объект активного клиента
					client::awh_t awh;
					// Список доступных потоков
					std::map <uint64_t, int32_t> streams;
					/**
					 * Client Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Client(const fmk_t * fmk, const log_t * log) noexcept :
					 sid(-1), busy(false), sending(false), upgrade(false),
					 method(awh::web_t::method_t::NONE), agent(client::web_t::agent_t::HTTP),
					 core(fmk, log), awh(&core, fmk, log) {}
				} client_t;
				/**
				 * Settings Структура параметров клиента
				 */
				typedef struct Settings {
					ka_t ka;                                   // Параметры жизни подключения
					wtd_t wtd;                                 // Таймауты на обмен данными
					dns_t dns;                                 // Параметры DNS-резолвера
					auth_t auth;                               // Параметры авторизации на сервере
					size_t chunk;                              // Размер передаваемых чанков
					uint8_t attempts;                          // Количество попыток выполнения редиректов
					scheme_t::sonet_t sonet;                   // Тип сокета подключения (TCP / UDP)
					scheme_t::family_t family;                 // Cемейстово интернет протоколов (IPV4 / IPV6 / IPC)
					proxy_t proxy;                             // Параметры прокси-клиента для подключения к прокси-серверу
					string login;                              // Логин пользователя для авторизации на сервере
					string password;                           // Пароль пользователя для авторизации на сервере
					string userAgent;                          // Название заголовка User-Agent для HTTP-запроса
					node_t::ssl_t ssl;                         // Объект работы с SSL-шифрованием
					encryption_t encryption;                   // Объект параметров шифрования
					vector <string> ns;                        // Список серверов имён, через которые необходимо производить резолвинг доменов
					vector <string> ips;                       // Список IP-адресов компьютера с которых разрешено выходить в интернет
					vector <http_t::compressor_t> compressors; // Список поддерживаемых компрессоров
					/**
					 * Settings Конструктор
					 */
					Settings() noexcept :
					 chunk(0), attempts(15),
					 sonet(scheme_t::sonet_t::TCP),
					 family(scheme_t::family_t::IPV4),
					 login{""}, password{""}, userAgent{""},
					 compressors({
					 	http_t::compressor_t::ZSTD,
						http_t::compressor_t::BROTLI,
						http_t::compressor_t::GZIP,
						http_t::compressor_t::DEFLATE
					}) {}
				} settings_t;
			private:
				// Объект работы с URI
				uri_t _uri;
				// Объект идентификации сервиса
				ident_t _ident;
				// Объект параметров клиента
				settings_t _settings;
				// Хранилище функций обратного вызова
				callback_t _callback;
			private:
				// Объект сетевого ядра
				server::core_t _core;
			private:
				// Объект активного сервера
				awh_t _server;
			private:
				// Максимальный размер памяти для хранений полезной нагрузки всех брокеров
				size_t _memoryAvailableSize;
				// Максимальный размер хранимой полезной нагрузки для одного брокера
				size_t _brokerAvailableSize;
			private:
				// Компрессор для рекомпрессии пересылаемых данных
				http_t::compressor_t _compressor;
			private:
				// Список флагов приложения
				std::set <flag_t> _flags;
			private:
				// Буферы отправляемой полезной нагрузки
				std::map <uint64_t, std::unique_ptr <queue_t>> _payloads;
				// Список активных клиентов
				std::map <uint64_t, std::unique_ptr <client_t>> _clients;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * launchedEvents Метод получения события запуска сервера
				 * @param host хост запущенного сервера
				 * @param port порт запущенного сервера
				 */
				void launchedEvents(const string & host, const uint32_t port) noexcept;
			private:
				/**
				 * passwordEvents Метод извлечения пароля (для авторизации методом Digest)
				 * @param bid   идентификатор брокера (клиента)
				 * @param login логин пользователя
				 * @return      пароль пользователя хранящийся в базе данных
				 */
				string passwordEvents(const uint64_t bid, const string & login) noexcept;
				/**
				 * authEvents Метод проверки авторизации пользователя (для авторизации методом Basic)
				 * @param bid      идентификатор брокера (клиента)
				 * @param login    логин пользователя (от клиента)
				 * @param password пароль пользователя (от клиента)
				 * @return         результат авторизации
				 */
				bool authEvents(const uint64_t bid, const string & login, const string & password) noexcept;
			private:
				/**
				 * acceptEvents Метод активации клиента на сервере
				 * @param ip   адрес интернет подключения
				 * @param mac  аппаратный адрес подключения
				 * @param port порт подключения
				 * @return     результат проверки
				 */
				bool acceptEvents(const string & ip, const string & mac, const uint32_t port) noexcept;
			private:
				/**
				 * callbackEvents Метод отлавливания событий контейнера функций обратного вызова
				 * @param event событие контейнера функций обратного вызова
				 * @param fid   идентификатор функции обратного вызова
				 */
				void callbackEvents(const callback_t::event_t event, const uint64_t fid, const callback_t::fn_t &) noexcept;
			private:
				/**
				 * available Метод получения событий освобождения памяти буфера полезной нагрузки
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param bid    идентификатор брокера
				 * @param size   размер буфера полезной нагрузки
				 * @param core   объект сетевого ядра
				 */
				void available(const broker_t broker, const uint64_t bid, const size_t size, awh::core_t * core) noexcept;
				/**
				 * unavailable Метод получения событий недоступности памяти буфера полезной нагрузки
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param bid    идентификатор брокера
				 * @param buffer буфер полезной нагрузки которую не получилось отправить
				 * @param size   размер буфера полезной нагрузки
				 */
				void unavailable(const broker_t broker, const uint64_t bid, const char * buffer, const size_t size) noexcept;
			private:
				/** 
				 * eraseClient Метод удаления подключённого клиента
				 * @param bid идентификатор брокера
				 */
				void eraseClient(const uint64_t bid) noexcept;
			private:
				/**
				 * endClient Метод завершения запроса клиента
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param rid    идентификатор запроса
				 * @param direct направление передачи данных
				 */
				void endClient(const int32_t sid, const uint64_t bid, const uint64_t rid, const client::web_t::direct_t direct) noexcept;
			private:
				/**
				 * responseClient Метод получения сообщения с удалённого сервера
				 * @param id      идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param rid     идентификатор запроса
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 */
				void responseClient(const int32_t sid, const uint64_t bid, const uint64_t rid, const uint32_t code, const string & message) noexcept;
			private:
				/**
				 * activeServer Метод идентификации активности на Web сервере (для сервера)
				 * @param bid  идентификатор брокера (клиента)
				 * @param mode режим события подключения
				 */
				void activeServer(const uint64_t bid, const server::web_t::mode_t mode) noexcept;
				/**
				 * activeClient Метод идентификации активности на Web сервере (для клиента)
				 * @param bid  идентификатор брокера (клиента)
				 * @param mode режим события подключения
				 */
				void activeClient(const uint64_t bid, const client::web_t::mode_t mode) noexcept;
			private:
				/**
				 * entityServer Метод получения тела запроса с клиента на сервере
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера (клиента)
				 * @param method метод запроса на уделённый сервер
				 * @param url    URL-адрес параметров запроса
				 * @param entity тело запроса с клиента на сервере
				 */
				void entityServer(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity) noexcept;
				/**
				 * entityClient Метод получения тела ответа с сервера клиенту
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param rid     идентификатор запроса
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param entity  тело ответа клиенту с сервера
				 */
				void entityClient(const int32_t sid, const uint64_t bid, const uint64_t rid, const uint32_t code, const string & message, const vector <char> & entity) noexcept;
			private:
				/**
				 * headersServer Метод получения заголовков запроса с клиента на сервере
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param method  метод запроса на уделённый сервер
				 * @param url     URL-адрес параметров запроса
				 * @param headers заголовки HTTP-запроса
				 */
				void headersServer(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const std::unordered_multimap <string, string> & headers) noexcept;
				/**
				 * headersClient Метод получения заголовков ответа с сервера клиенту
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param rid     идентификатор запроса
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param headers заголовки HTTP-ответа
				 */
				void headersClient(const int32_t sid, const uint64_t bid, const uint64_t rid, const uint32_t code, const string & message, const std::unordered_multimap <string, string> & headers) noexcept;
			private:
				/**
				 * pushClient Метод получения заголовков выполненного запроса (PUSH HTTP/2)
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param rid     идентификатор запроса
				 * @param method  метод запроса на уделённый сервер
				 * @param url     URL-адрес параметров запроса
				 * @param headers заголовки HTTP-запроса
				 */
				void pushClient(const int32_t sid, const uint64_t bid, const uint64_t rid, const awh::web_t::method_t method, const uri_t::url_t & url, const std::unordered_multimap <string, string> & headers) noexcept;
			private:
				/**
				 * handshake Метод получения удачного запроса (для сервера)
				 * @param sid   идентификатор потока
				 * @param bid   идентификатор брокера
				 * @param agent идентификатор агента клиента
				 */
				void handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent) noexcept;
			private:
				/**
				 * raw Метод получения сырых данных с сервера и клиента
				 * @param bid    идентификатор брокера (клиента)
				 * @param broker брокер получивший данные
				 * @param buffer буфер бинарных данных
				 * @param size   разбмер буфера бинарных данных
				 * @return       флаг обязательной следующей обработки данных
				 */
				bool raw(const uint64_t bid, const broker_t broker, const char * buffer, const size_t size) noexcept;
			private:
				/**
				 * via Метод генерации заголовка Via
				 * @param sid       идентификатор потока
				 * @param bid       идентификатор брокера (клиента)
				 * @param mediators список предыдущих посредников
				 * @return          сгенерированный заголовок
				 */
				string via(const int32_t sid, const uint64_t bid, const vector <string> & mediators = {}) const noexcept;
			private:
				/**
				 * completed Метод завершения получения данных
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера (клиента)
				 */
				void completed(const int32_t sid, const uint64_t bid) noexcept;
			public:
				/**
				 * proto Метод извлечения поддерживаемого протокола подключения
				 * @param bid идентификатор брокера
				 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
				 */
				engine_t::proto_t proto(const uint64_t bid) const noexcept;
			public:
				/**
				 * parser Метод извлечения объекта HTTP-парсера
				 * @param sid идентификатор потока
				 * @param bid идентификатор брокера
				 * @return    объект HTTP-парсера
				 */
				const awh::http_t * parser(const int32_t sid, const uint64_t bid) const noexcept;
			public:
				/**
				 * init Метод инициализации PROXY-сервера
				 * @param socket     unix-сокет для биндинга
				 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
				 */
				void init(const string & socket, const http_t::compressor_t compressor = http_t::compressor_t::NONE) noexcept;
				/**
				 * init Метод инициализации PROXY-сервера
				 * @param port       порт сервера
				 * @param host       хост сервера
				 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
				 * @param family     тип протокола интернета (IPV4 / IPV6 / IPC)
				 */
				void init(const uint32_t port = SERVER_PROXY_PORT, const string & host = "", const http_t::compressor_t compressor = http_t::compressor_t::NONE, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * callback Метод установки функций обратного вызова
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const char * name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(name != nullptr)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const string & name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(fid > 0)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (fid, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param A    тип идентификатора функции
				 * @param B    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename A, typename B, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const A fid, Args... args) noexcept -> uint64_t {
					// Если мы получили на вход число
					if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <B> (static_cast <uint64_t> (fid), args...);
					// Выводим результат по умолчанию
					return 0;
				}
			public:
				/**
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				uint32_t port(const uint64_t bid) const noexcept;
				/**
				 * ip Метод получения IP-адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес интернет подключения брокера
				 */
				const string & ip(const uint64_t bid) const noexcept;
				/**
				 * mac Метод получения MAC-адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес устройства брокера
				 */
				const string & mac(const uint64_t bid) const noexcept;
			public:
				/**
				 * stop Метод остановки сервера
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска сервера
				 */
				void start() noexcept;
			public:
				/**
				 * bind Метод подключения модуля ядра к текущей базе событий
				 * @param core модуль ядра для подключения
				 */
				void bind(awh::core_t * core) noexcept;
				/**
				 * unbind Метод отключения модуля ядра от текущей базы событий
				 * @param core модуль ядра для отключения
				 */
				void unbind(awh::core_t * core) noexcept;
			public:
				/**
				 * close Метод закрытия подключения брокера
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
				/**
				 * cluster Метод установки количества процессов кластера
				 * @param mode флаг активации/деактивации кластера
				 * @param size количество рабочих процессов
				 */
				void cluster(const awh::scheme_t::mode_t mode, const uint16_t size = 0) noexcept;
			public:
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t total) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const std::set <flag_t> & flags) noexcept;
			public:
				/**
				 * addOrigin Метод добавления разрешённого источника
				 * @param origin разрешённый источнико
				 */
				void addOrigin(const string & origin) noexcept;
				/**
				 * setOrigin Метод установки списка разрешённых источников
				 * @param origins список разрешённых источников
				 */
				void setOrigin(const vector <string> & origins) noexcept;
			public:
				/**
				 * addAltSvc Метод добавления альтернативного сервиса
				 * @param origin название альтернативного сервиса
				 * @param field  поле альтернативного сервиса
				 */
				void addAltSvc(const string & origin, const string & field) noexcept;
				/**
				 * setAltSvc Метод установки списка разрешённых источников
				 * @param origins список альтернативных сервисов
				 */
				void setAltSvc(const std::unordered_multimap <string, string> & origins) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const std::map <awh::http2_t::settings_t, uint32_t> & settings = {}) noexcept;
			public:
				/**
				 * realm Метод установки название сервера
				 * @param realm название сервера
				 */
				void realm(const string & realm) noexcept;
				/**
				 * opaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(const string & opaque) noexcept;
			public:
				/**
				 * maxRequests Метод установки максимального количества запросов
				 * @param max максимальное количество запросов
				 */
				void maxRequests(const uint32_t max) noexcept;
			public:
				/**
				 * ssl Метод установки параметров SSL-шифрования
				 * @param ssl объект параметров SSL-шифрования
				 */
				void ssl(const node_t::ssl_t & ssl) noexcept;
			public:
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param bid  идентификатор брокера
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * memoryAvailableSize Метод получения максимального рамзера памяти для хранения полезной нагрузки всех брокеров
				 * @return размер памяти для хранения полезной нагрузки всех брокеров
				 */
				size_t memoryAvailableSize() const noexcept;
				/**
				 * memoryAvailableSize Метод установки максимального рамзера памяти для хранения полезной нагрузки всех брокеров
				 * @param size размер памяти для хранения полезной нагрузки всех брокеров
				 */
				void memoryAvailableSize(const size_t size) noexcept;
			public:
				/**
				 * brokerAvailableSize Метод получения максимального размера хранимой полезной нагрузки для одного брокера
				 * @return размер хранимой полезной нагрузки для одного брокера
				 */
				size_t brokerAvailableSize() const noexcept;
				/**
				 * brokerAvailableSize Метод установки максимального размера хранимой полезной нагрузки для одного брокера
				 * @param size размер хранимой полезной нагрузки для одного брокера
				 */
				void brokerAvailableSize(const size_t size) noexcept;
			public:
				/**
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
				/**
				 * bandwidth Метод установки пропускной способности сети
				 * @param bid   идентификатор брокера
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const size_t bid, const string & read = "", const string & write = "") noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param size   размер чанка для установки
				 */
				void chunk(const broker_t broker, const size_t size) noexcept;
			public:
				/**
				 * hosts Метод загрузки файла со списком хостов
				 * @param broker   брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param filename адрес файла для загрузки
				 */
				void hosts(const broker_t broker, const string & filename) noexcept;
			public:
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param broker      брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const broker_t broker, const vector <http_t::compressor_t> & compressors) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param cnt    максимальное количество попыток
				 * @param idle   интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl  интервал времени в секундах между попытками
				 */
				void keepAlive(const broker_t broker, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * waitMessage Метод ожидания входящих сообщений
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param sec    интервал времени в секундах
				 */
				void waitMessage(const broker_t broker, const uint16_t sec) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param broker  брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для подключения к серверу
				 */
				void waitTimeDetect(const broker_t broker, const uint16_t read, const uint16_t write, const uint16_t connect = 0) noexcept;
			public:
				/**
				 * sonet Метод установки типа сокета подключения
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param sonet  тип сокета подключения (TCP / UDP / SCTP)
				 */
				void sonet(const broker_t broker, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
				/**
				 * family Метод установки типа протокола интернета
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
				 */
				void family(const broker_t broker, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
				/**
				 * network Метод установки параметров сети
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				void network(const broker_t broker, const vector <string> & ips = {}, const vector <string> & ns = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
			public:
				/**
				 * userAgent Метод установки User-Agent для HTTP-запроса
				 * @param userAgent агент пользователя для HTTP-запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * user Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
				/**
				 * ident Метод установки идентификации клиента
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * proxy Метод активации/деактивации прокси-склиента
				 * @param work флаг активации/деактивации прокси-клиента
				 */
				void proxy(const client::scheme_t::work_t work) noexcept;
				/**
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / IPC)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * flushDNS Метод сброса кэша DNS-резолвера
				 * @param bid идентификатор брокера
				 * @return    результат работы функции
				 */
				bool flushDNS(const uint64_t bid) noexcept;
			public:
				/**
				 * timeoutDNS Метод установки времени ожидания выполнения запроса
				 * @param sec интервал времени выполнения запроса в секундах
				 */
				void timeoutDNS(const uint8_t sec) noexcept;
			public:
				/**
				 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
				 * @param prefix префикс переменной окружения для установки
				 */
				void prefixDNS(const string & prefix) noexcept;
			public:
				/**
				 * clearDNSBlackList Метод очистки чёрного списка
				 * @param domain доменное имя для которого очищается чёрный список
				 */
				void clearDNSBlackList(const string & domain) noexcept;
				/**
				 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void delInDNSBlackList(const string & domain, const string & ip) noexcept;
				/**
				 * setToDNSBlackList Метод добавления IP-адреса в чёрный список
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в чёрный список
				 */
				void setToDNSBlackList(const string & domain, const string & ip) noexcept;
			public:
				/**
				 * cork Метод отключения/включения алгоритма TCP/CORK
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param bid    идентификатор брокера
				 * @param mode   режим применимой операции
				 * @return       результат выполенния операции
				 */
				bool cork(const broker_t broker, const uint64_t bid, const engine_t::mode_t mode) noexcept;
				/**
				 * nodelay Метод отключения/включения алгоритма Нейгла
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param bid    идентификатор брокера
				 * @param mode   режим применимой операции
				 * @return       результат выполенния операции
				 */
				bool nodelay(const broker_t broker, const uint64_t bid, const engine_t::mode_t mode) noexcept;
			public:
				/**
				 * authTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authTypeProxy(const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
				/**
				 * authType Метод установки типа авторизации
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param type   тип авторизации
				 * @param hash   алгоритм шифрования для Digest авторизации
				 */
				void authType(const broker_t broker, const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * encrypt Метод активации шифрования для клиента
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера
				 * @param mode   флаг активации шифрования
				 */
				void encrypt(const broker_t broker, const int32_t sid, const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * encryption Метод активации шифрования
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param mode   флаг активации шифрования
				 */
				void encryption(const broker_t broker, const bool mode) noexcept;
				/**
				 * encryption Метод установки параметров шифрования
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void encryption(const broker_t broker, const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * Proxy Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Proxy(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Proxy Деструктор
				 */
				~Proxy() noexcept {}
		} proxy_t;
	};
};

#endif // __AWH_HTTP_PROXY__
