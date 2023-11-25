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
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_HTTP_PROXY__
#define __AWH_HTTP_PROXY__

/**
 * Стандартная библиотека
 */
#include <set>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * Наши модули
 */
#include <client/awh.hpp>
#include <server/awh.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server клиентское пространство имён
	 */
	namespace server {
		/**
		 * Proxy Класс Proxy-сервера
		 */
		typedef class Proxy {
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
					WAIT_MESS                    = 0x04, // Флаг ожидания входящих сообщений
					DECRYPT                      = 0x05, // Флаг предписывающий выполнять расшифровку зашифрованного контента при передаче клиенту
					VERIFY_SSL                   = 0x06, // Флаг выполнения проверки сертификата SSL
					RECOMPRESS                   = 0x07, // Флаг выполнения рекомпрессинга передаваемых данных
					CONNECT_METHOD_CLIENT_ENABLE = 0x08, // Флаг разрешающий метод CONNECT на прокси-клиенте
					CONNECT_METHOD_SERVER_ENABLE = 0x09  // Флаг разрешающий метод CONNECT на сервере
				};
			private:
				/**
				 * CA Структура параметров CA-файла
				 */
				typedef struct CA {
					string path;    // Адрес каталога где находится сертификат (CA-файл)
					string trusted; // Адрес доверенного сертификата (CA-файла)
					/**
					 * CA Конструктор
					 */
					CA() noexcept : path{""}, trusted{""} {}
				} ca_t;
				/**
				 * Request Объект параметров запроса
				 */
				typedef struct Request {
					awh::web_t::req_t params;                    // Параметры запроса
					vector <char> entity;                        // Тело запроса
					unordered_multimap <string, string> headers; // Заголовки запроса
				} request_t;
				/**
				 * Response Объект параметров ответа
				 */
				typedef struct Response {
					awh::web_t::res_t params;                    // Параметры ответа
					vector <char> entity;                        // Тело ответа
					unordered_multimap <string, string> headers; // Заголовки ответа
				} response_t;
				/**
				 * KeepAlive Структура параметров жизни подключения
				 */
				typedef struct KeepAlive {
					int cnt;   // Максимальное количество попыток
					int idle;  // Интервал времени в секундах через которое происходит проверка подключения
					int intvl; // Интервал времени в секундах между попытками
					/**
					 * KeepAlive Конструктор
					 */
					KeepAlive() noexcept : cnt(0), idle(0), intvl(0) {}
				} __attribute__((packed)) ka_t;
				/**
				 * DNS Структура параметров DNS-резолвера
				 */
				typedef struct DNS {
					time_t ttl;                                    // Времени жизни DNS-кэша
					uint8_t timeout;                               // Время ожидания выполнения запроса
					string hosts;                                  // Адрес файла с локальными хостами
					string prefix;                                 // Префикс переменной окружения для извлечения серверов имён
					unordered_multimap <string, string> blacklist; // Чёрный список доменных имён
					/**
					 * DNS Конструктор
					 */
					DNS() noexcept : ttl(0), timeout(0), hosts{""}, prefix{""} {}
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
					Ident() noexcept : id{AWH_SHORT_NAME}, ver{AWH_VERSION}, name{AWH_NAME} {}
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
					Auth() noexcept : type(awh::auth_t::type_t::BASIC), hash(awh::auth_t::hash_t::MD5) {}
				} __attribute__((packed)) auth_t;
				/**
				 * WaitTimeDetect Структура таймаутов на обмен данными в миллисекундах
				 */
				typedef struct WaitTimeDetect {
					time_t read;    // Время ожидания на получение данных
					time_t write;   // Врежмя ожидания на отправку данных
					time_t connect; // Время ожидания подключения
					/**
					 * WaitTimeDetect Конструктор
					 */
					WaitTimeDetect() noexcept : read(READ_TIMEOUT), write(WRITE_TIMEOUT), connect(CONNECT_TIMEOUT) {}
				} __attribute__((packed)) wtd_t;
				/**
				 * ProxyClient Структура параметров прокси-клиента
				 */
				typedef struct ProxyClient {
					string uri;                // Параметры запроса на прокси-сервер
					auth_t auth;               // Параметры авторизации на прокси-сервере
					scheme_t::family_t family; // Cемейстово интернет протоколов (IPV4 / IPV6 / NIX)
					/**
					 * ProxyClient Конструктор
					 */
					ProxyClient() noexcept : uri{""}, family(scheme_t::family_t::IPV4) {}
				} proxy_t;
				/**
				 * Encryption Структура параметров шифрования
				 */
				typedef struct Encryption {
					bool mode;               // Флаг активности механизма шифрования
					bool verify;             // Флаг выполнение верификации доменного имени
					string pass;             // Пароль шифрования передаваемых данных
					string salt;             // Соль шифрования передаваемых данных
					hash_t::cipher_t cipher; // Размер шифрования передаваемых данных
					vector <string> ciphers; // Список алгоритмов шифрования
					/**
					 * Encryption Конструктор
					 */
					Encryption() noexcept : mode(false), verify(false), pass{""}, salt{""}, cipher(hash_t::cipher_t::AES128) {}
				} encryption_t;
				/**
				 * Client Объект клиента
				 */
				typedef struct Client {
					// Идентификатор потока
					int32_t sid;
					// Флаг отправки результата
					bool sending;
					// Активный метод подключения
					awh::web_t::method_t method;
					// Объект параметров запроса
					request_t request;
					// Объект параметров ответа
					response_t response;
					// Объект сетевого ядра
					client::core_t core;
					// Объект активного клиента
					client::awh_t awh;
					/**
					 * Client Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Client(const fmk_t * fmk, const log_t * log) noexcept :
					 sid(-1), sending(false), method(awh::web_t::method_t::NONE), core(fmk, log), awh(&core, fmk, log) {}
				} client_t;
				/**
				 * Settings Структура параметров клиента
				 */
				typedef struct Settings {
					ka_t ka;                                 // Параметры жизни подключения
					ca_t ca;                                 // Параметры CA-файла сертификата
					wtd_t wtd;                               // Таймауты на обмен данными
					dns_t dns;                               // Параметры DNS-резолвера
					auth_t auth;                             // Параметры авторизации на сервере
					size_t chunk;                            // Размер передаваемых чанков
					uint8_t attempts;                        // Количество попыток выполнения редиректов
					scheme_t::sonet_t sonet;                 // Тип сокета подключения (TCP / UDP)
					scheme_t::family_t family;               // Cемейстово интернет протоколов (IPV4 / IPV6 / NIX)
					scheme_t::marker_t marker;               // Объект маркеров детектирования обмена данных
					proxy_t proxy;                           // Параметры прокси-клиента для подключения к прокси-серверу
					string login;                            // Логин пользователя для авторизации на сервере
					string password;                         // Пароль пользователя для авторизации на сервере
					string userAgent;                        // Название заголовка User-Agent для HTTP-запроса
					encryption_t encryption;                 // Объект параметров шифрования
					vector <string> ns;                      // Список серверов имён, через которые необходимо производить резолвинг доменов
					vector <string> ips;                     // Список IP-адресов компьютера с которых разрешено выходить в интернет
					vector <http_t::compress_t> compressors; // Список поддерживаемых компрессоров
					/**
					 * Settings Конструктор
					 */
					Settings() noexcept :
					 chunk(0), attempts(15),
					 sonet(scheme_t::sonet_t::TCP),
					 family(scheme_t::family_t::IPV4),
					 login{""}, password{""}, userAgent{""},
					 compressors({http_t::compress_t::BROTLI, http_t::compress_t::GZIP, http_t::compress_t::DEFLATE}) {}
				} settings_t;
			private:
				// Объект работы с URI ссылками
				uri_t _uri;
				// Объект сетевого ядра
				core_t _core;
				// Объект активного сервера
				awh_t _server;
				// Объект идентификации сервиса
				ident_t _ident;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект параметров клиента
				settings_t _settings;
			private:
				// Компрессор для рекомпрессии пересылаемых данных
				http_t::compress_t _compressor;
			private:
				// Список флагов приложения
				set <flag_t> _flags;
			private:
				// Список активных клиентов
				map <uint64_t, unique_ptr <client_t>> _clients;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * passwordCallback Метод извлечения пароля (для авторизации методом Digest)
				 * @param bid   идентификатор брокера (клиента)
				 * @param login логин пользователя
				 * @return      пароль пользователя хранящийся в базе данных
				 */
				string passwordCallback(const uint64_t bid, const string & login) noexcept;
				/**
				 * authCallback Метод проверки авторизации пользователя (для авторизации методом Basic)
				 * @param bid      идентификатор брокера (клиента)
				 * @param login    логин пользователя (от клиента)
				 * @param password пароль пользователя (от клиента)
				 * @return         результат авторизации
				 */
				bool authCallback(const uint64_t bid, const string & login, const string & password) noexcept;
			private:
				/**
				 * acceptServer Метод активации клиента на сервере
				 * @param ip   адрес интернет подключения
				 * @param mac  аппаратный адрес подключения
				 * @param port порт подключения
				 * @return     результат проверки
				 */
				bool acceptServer(const string & ip, const string & mac, const u_int port) noexcept;
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
				 * @param direct направление передачи данных
				 */
				void endClient(const int32_t sid, const uint64_t bid, const client::web_t::direct_t direct) noexcept;
			private:
				/**
				 * responseClient Метод получения сообщения с удалённого сервера
				 * @param id      идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 */
				void responseClient(const int32_t sid, const uint64_t bid, const u_int code, const string & message) noexcept;
			private:
				/**
				 * activeClient Метод идентификации активности на Web сервере (для клиента)
				 * @param bid  идентификатор брокера (клиента)
				 * @param mode режим события подключения
				 */
				void activeClient(const uint64_t bid, const client::web_t::mode_t mode) noexcept;
				/**
				 * activeServer Метод идентификации активности на Web сервере (для сервера)
				 * @param bid  идентификатор брокера (клиента)
				 * @param mode режим события подключения
				 */
				void activeServer(const uint64_t bid, const server::web_t::mode_t mode) noexcept;
			private:
				/**
				 * entityClient Метод получения тела ответа с сервера клиенту
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param entity  тело ответа клиенту с сервера
				 */
				void entityClient(const int32_t sid, const uint64_t bid, const u_int code, const string & message, const vector <char> & entity) noexcept;
				/**
				 * entityServer Метод получения тела запроса с клиента на сервере
				 * @param sid    идентификатор потока
				 * @param bid    идентификатор брокера (клиента)
				 * @param method метод запроса на уделённый сервер
				 * @param url    URL-адрес параметров запроса
				 * @param entity тело запроса с клиента на сервере
				 */
				void entityServer(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity) noexcept;
			private:
				/**
				 * headersClient Метод получения заголовков ответа с сервера клиенту
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param headers заголовки HTTP-ответа
				 */
				void headersClient(const int32_t sid, const uint64_t bid, const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept;
				/**
				 * headersServer Метод получения заголовков запроса с клиента на сервере
				 * @param sid     идентификатор потока
				 * @param bid     идентификатор брокера (клиента)
				 * @param method  метод запроса на уделённый сервер
				 * @param url     URL-адрес параметров запроса
				 * @param headers заголовки HTTP-запроса
				 */
				void headersServer(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept;
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
				 * via Метод генерации заголовка Via
				 * @param bid       идентификатор брокера (клиента)
				 * @param mediators список предыдущих посредников
				 * @return          сгенерированный заголовок
				 */
				string via(const uint64_t bid, const vector <string> & mediators = {}) const noexcept;
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
				 * completed Метод завершения получения данных
				 * @param bid идентификатор брокера (клиента)
				 */
				void completed(const uint64_t bid) noexcept;
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
				 * @param bid идентификатор брокера
				 * @return    объект HTTP-парсера
				 */
				const awh::http_t * parser(const uint64_t bid) const noexcept;
			public:
				/**
				 * init Метод инициализации PROXY-сервера
				 * @param socket     unix-сокет для биндинга
				 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
				 */
				void init(const string & socket, const http_t::compress_t compressor = http_t::compress_t::NONE) noexcept;
				/**
				 * init Метод инициализации PROXY-сервера
				 * @param port       порт сервера
				 * @param host       хост сервера
				 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
				 */
				void init(const u_int port, const string & host = "", const http_t::compress_t compressor = http_t::compress_t::NONE) noexcept;
			public:
				/**
				 * on Метод установки функция обратного вызова при удаление клиента из стека сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова при выполнении рукопожатия на прокси-сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const engine_t::proto_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для извлечения пароля
				 * @param callback функция обратного вызова
				 */
				void on(function <string (const uint64_t, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для обработки авторизации
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие активации брокера на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова при получении источников подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const vector <string> &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова при получении альтернативных сервисов
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const string &, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const broker_t, const web_t::mode_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const broker_t, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного заголовка с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const broker_t, const string &, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибки
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const broker_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения тела ответа с удалённого сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const u_int, const string &, vector <char> *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения тела запроса на прокси-сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, vector <char> *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения заголовков ответа с удалённого сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const u_int, const string &, unordered_multimap <string, string> *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения заголовков запроса на прокси-сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, unordered_multimap <string, string> *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие формирования готового ответа клиенту подключённого к прокси-серверу
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const u_int, const string &, const vector <char> &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода полученных заголовков с клиента
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				u_int port(const uint64_t bid) const noexcept;
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
				 * clusterSize Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void clusterSize(const uint16_t size = 0) noexcept;
			public:
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const u_short total) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
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
				void setAltSvc(const unordered_multimap <string, string> & origins) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const map <awh::http2_t::settings_t, uint32_t> & settings = {}) noexcept;
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
				void maxRequests(const size_t max) noexcept;
			public:
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
				/**
				 * alive Метод установки времени жизни подключения
				 * @param time время жизни подключения
				 */
				void alive(const time_t time) noexcept;
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param bid  идентификатор брокера
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
				/**
				 * bandWidth Метод установки пропускной способности сети
				 * @param bid   идентификатор брокера
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const size_t bid, const string & read, const string & write) noexcept;
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
				 * certificate Метод установки файлов сертификата
				 * @param chain файл цепочки сертификатов
				 * @param key   приватный ключ сертификата
				 */
				void certificate(const string & chain, const string & key) noexcept;
			public:
				/**
				 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param mode   флаг состояния разрешения проверки
				 */
				void verifySSL(const broker_t broker, const bool mode) noexcept;
				/**
				 * ciphers Метод установки алгоритмов шифрования
				 * @param broker  брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param ciphers список алгоритмов шифрования для установки
				 */
				void ciphers(const broker_t broker, const vector <string> & ciphers) noexcept;
				/**
				 * ca Метод установки доверенного сертификата (CA-файла)
				 * @param broker  брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param trusted адрес доверенного сертификата (CA-файла)
				 * @param path    адрес каталога где находится сертификат (CA-файл)
				 */
				void ca(const broker_t broker, const string & trusted, const string & path = "") noexcept;
			public:
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param cnt    максимальное количество попыток
				 * @param idle   интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl  интервал времени в секундах между попытками
				 */
				void keepAlive(const broker_t broker, const int cnt, const int idle, const int intvl) noexcept;
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param broker      брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const broker_t broker, const vector <http_t::compress_t> & compressors) noexcept;
			public:
				/**
				 * bytesDetect Метод детекции сообщений по количеству байт
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param read   количество байт для детекции по чтению
				 * @param write  количество байт для детекции по записи
				 */
				void bytesDetect(const broker_t broker, const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param broker  брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для подключения к серверу
				 */
				void waitTimeDetect(const broker_t broker, const time_t read, const time_t write, const time_t connect = 0) noexcept;
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
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 */
				void family(const broker_t broker, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
				/**
				 * network Метод установки параметров сети
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
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
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
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
				/**
				 * timeToLiveDNS Метод установки времени жизни DNS-кэша
				 * @param ttl время жизни DNS-кэша в миллисекундах
				 */
				void timeToLiveDNS(const time_t ttl) noexcept;
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
				 * crypted Метод получения флага шифрования
				 * @param bid идентификатор брокера
				 * @return    результат проверки
				 */
				bool crypted(const uint64_t bid) const noexcept;
				/**
				 * encrypt Метод активации шифрования для клиента
				 * @param bid  идентификатор брокера
				 * @param mode флаг активации шифрования
				 */
				void encrypt(const uint64_t bid, const bool mode) noexcept;
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
