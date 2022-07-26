/**
 * @file: rest.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_REST_CLIENT__
#define __AWH_REST_CLIENT__

/**
 * Стандартная библиотека
 */
#include <functional>
#include <nlohmann/json.hpp>

/**
 * Наши модули
 */
#include <http/client.hpp>
#include <core/client.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Rest Класс работы с REST клиентом
		 */
		typedef class Rest {
			private:
				/**
				 * Основные экшены
				 */
				enum class action_t : uint8_t {
					NONE          = 0x01, // Отсутствие события
					OPEN          = 0x02, // Событие открытия подключения
					READ          = 0x03, // Событие чтения с сервера
					CONNECT       = 0x04, // Событие подключения к серверу
					DISCONNECT    = 0x05, // Событие отключения от сервера
					PROXY_READ    = 0x06, // Событие чтения с прокси-сервера
					PROXY_CONNECT = 0x07  // Событие подключения к прокси-серверу
				};
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01,
					DISCONNECT = 0x02
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					NOINFO    = 0x01, // Флаг запрещающий вывод информационных сообщений
					NOTSTOP   = 0x02, // Флаг запрета остановки биндинга
					WAITMESS  = 0x04, // Флаг ожидания входящих сообщений
					KEEPALIVE = 0x08, // Флаг автоматического поддержания подключения
					VERIFYSSL = 0x10, // Флаг выполнения проверки сертификата SSL
					REDIRECTS = 0x20  // Флаг разрешающий автоматическое перенаправление запросов
				};
				/**
				 * Request Структура запроса клиента
				 */
				typedef struct Request {
					uint8_t attempts;                            // Количество попыток
					web_t::method_t method;                      // Метод запроса
					uri_t::url_t url;                            // Параметры адреса для запроса
					vector <char> entity;                        // Тело запроса
					unordered_multimap <string, string> headers; // Заголовки клиента
					/**
					 * Request Конструктор
					 */
					Request() noexcept : attempts(0), method(web_t::method_t::NONE) {}
				} req_t;
				/**
				 * Response Структура ответа сервера
				 */
				typedef struct Response {
					bool ok;                                     // Флаг удачного ответа
					u_int code;                                  // Код ответа сервера
					string message;                              // Сообщение ответа сервера
					vector <char> entity;                        // Тело ответа сервера
					unordered_multimap <string, string> headers; // Заголовки сервера
					/**
					 * Response Конструктор
					 */
					Response() noexcept : ok(false), code(0), message("") {}
				} res_t;
			private:
				/**
				 * Locker Структура локера
				 */
				typedef struct Locker {
					bool mode;           // Флаг блокировки
					recursive_mutex mtx; // Мютекс для блокировки потока
					/**
					 * Locker Конструктор
					 */
					Locker() noexcept : mode(false) {}
				} locker_t;
			private:
				// Объект для работы с сетью
				network_t nwk;
			private:
				// Объект работы с URI ссылками
				uri_t uri;
				// Объект для работы с HTTP
				http_t http;
				// Объект рабочего
				worker_t worker;
				// Объект блокировщика
				locker_t locker;
				// Экшен события
				action_t action;
				// Метод компрессии данных
				awh::http_t::compress_t compress;
			private:
				// Буфер бинарных данных
				vector <char> buffer;
			private:
				// Список запросов
				vector <req_t> requests;
				// Список ответов
				vector <res_t> responses;
			private:
				// Идентификатор подключения
				size_t aid = 0;
			private:
				// Выполнять анбиндинг после завершения запроса
				bool unbind = true;
				// Флаг принудительного отключения
				bool active = false;
				// Флаг выполнения редиректов
				bool redirects = false;
			private:
				// Общее количество попыток
				uint8_t totalAttempts = 10;
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
				// Создаём объект биндинга TCP/IP
				const client::core_t * core = nullptr;
			private:
				// Функция обратного вызова при подключении/отключении
				function <void (const mode_t, Rest *)> activeFn = nullptr;
				// Функция обратного вызова, вывода сообщения при его получении
				function <void (const res_t &, Rest *)> messageFn = nullptr;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const vector <char> & chunk, const awh::http_t * http) noexcept;
			private:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void openCallback(const size_t wid, awh::core_t * core) noexcept;
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void connectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
			private:
				/**
				 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void proxyConnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
			private:
				/**
				 * handler Метод управления входящими методами
				 */
				void handler() noexcept;
			private:
				/**
				 * actionOpen Метод обработки экшена открытия подключения
				 */
				void actionOpen() noexcept;
				/**
				 * actionRead Метод обработки экшена чтения с сервера
				 */
				void actionRead() noexcept;
				/**
				 * actionConnect Метод обработки экшена подключения к серверу
				 */
				void actionConnect() noexcept;
				/**
				 * actionDisconnect Метод обработки экшена отключения от сервера
				 */
				void actionDisconnect() noexcept;
			private:
				/**
				 * actionProxyRead Метод обработки экшена чтения с прокси-сервера
				 */
				void actionProxyRead() noexcept;
				/**
				 * actionProxyConnect Метод обработки экшена подключения к прокси-серверу
				 */
				void actionProxyConnect() noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
			public:
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
			public:
				/**
				 * close Метод закрытия подключения клиента
				 */
				void close() noexcept;
			public:
				/**
				 * GET Метод запроса в формате HTTP методом GET
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * DEL Метод запроса в формате HTTP методом DEL
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * PUT Метод запроса в формате HTTP методом PUT
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PUT Метод запроса в формате HTTP методом PUT
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PUT Метод запроса в формате HTTP методом PUT
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * POST Метод запроса в формате HTTP методом POST
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * POST Метод запроса в формате HTTP методом POST
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * POST Метод запроса в формате HTTP методом POST
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * PATCH Метод запроса в формате HTTP методом PATCH
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PATCH Метод запроса в формате HTTP методом PATCH
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * PATCH Метод запроса в формате HTTP методом PATCH
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				vector <char> PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * HEAD Метод запроса в формате HTTP методом HEAD
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				unordered_multimap <string, string> HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * TRACE Метод запроса в формате HTTP методом TRACE
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				unordered_multimap <string, string> TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
				/**
				 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
				 * @param url     адрес запроса
				 * @param headers заголовки запроса
				 * @return        результат запроса
				 */
				unordered_multimap <string, string> OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
			public:
				/**
				 * REST Метод запроса в формате HTTP
				 * @param request список запросов
				 */
				void REST(const vector <req_t> & request) noexcept;
			public:
				/**
				 * sendTimeout Метод отправки сигнала таймаута
				 */
				void sendTimeout() noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова при подключении/отключении
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const mode_t, Rest *)> callback) noexcept;
				/**
				 * setMessageCallback Метод установки функции обратного вызова при получении сообщения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const res_t &, Rest *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для получения чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept;
			public:
				/**
				 * setBytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept;
				/**
				 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void setWaitTimeDetect(const time_t read = READ_TIMEOUT, const time_t write = WRITE_TIMEOUT, const time_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * setMode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void setMode(const u_short flag) noexcept;
				/**
				 * setProxy Метод установки прокси-сервера
				 * @param uri параметры прокси-сервера
				 */
				void setProxy(const string & uri) noexcept;
				/**
				 * setChunkSize Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void setChunkSize(const size_t size) noexcept;
				/**
				 * setAttempts Метод установки общего количества попыток
				 * @param attempts общее количество попыток
				 */
				void setAttempts(const uint8_t attempts) noexcept;
				/**
				 * setUserAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
				 */
				void setUserAgent(const string & userAgent) noexcept;
				/**
				 * setCompress Метод установки метода компрессии
				 * @param compress метод компрессии сообщений
				 */
				void setCompress(const awh::http_t::compress_t compress) noexcept;
				/**
				 * setUser Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void setUser(const string & login, const string & password) noexcept;
				/**
				 * setServ Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void setServ(const string & id, const string & name, const string & ver) noexcept;
				/**
				 * setCrypt Метод установки параметров шифрования
				 * @param pass пароль шифрования передаваемых данных
				 * @param salt соль шифрования передаваемых данных
				 * @param aes  размер шифрования передаваемых данных
				 */
				void setCrypt(const string & pass, const string & salt = "", const hash_t::aes_t aes = hash_t::aes_t::AES128) noexcept;
				/**
				 * setAuthType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
				/**
				 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void setAuthTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * Rest Конструктор
				 * @param core объект биндинга TCP/IP
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Rest(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Rest Деструктор
				 */
				~Rest() noexcept {}
		} rest_t;
	};
};

#endif // __AWH_REST_CLIENT__
