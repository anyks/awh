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
					DEFER     = 0x01, // Флаг отложенных вызовов событий сокета
					NOINFO    = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOTSTOP   = 0x04, // Флаг запрета остановки биндинга
					WAITMESS  = 0x08, // Флаг ожидания входящих сообщений
					KEEPALIVE = 0x10, // Флаг автоматического поддержания подключения
					VERIFYSSL = 0x20  // Флаг выполнения проверки сертификата SSL
				};
				/**
				 * Request Структура запроса клиента
				 */
				typedef struct Request {
					bool failAuth;                               // Флаг проверки аутентификации
					web_t::method_t method;                      // Метод запроса
					uri_t::url_t url;                            // Параметры адреса для запроса
					vector <char> entity;                        // Тело запроса
					unordered_multimap <string, string> headers; // Заголовки клиента
					/**
					 * Request Конструктор
					 */
					Request() : failAuth(false), method(web_t::method_t::NONE) {}
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
					Response() : ok(false), code(0), message("") {}
				} res_t;
			private:
				// Создаём объект работы с URI ссылками
				uri_t uri;
				// Создаём объект для работы с HTTP
				http_t http;
				// Создаем объект для работы с сетью
				network_t nwk;
				// Объект рабочего
				worker_t worker;
				// Метод компрессии данных
				awh::http_t::compress_t compress;
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
				bool forstop = false;
			private:
				// Тело получения ответа
				vector <char> entity;
			private:
				// Список контекстов передаваемых объектов
				vector <void *> ctx = {nullptr, nullptr};
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
				// Создаём объект биндинга TCP/IP
				const client::core_t * core = nullptr;
			private:
				// Функция обратного вызова при подключении/отключении
				function <void (const mode_t, Rest *, void *)> openStopFn = nullptr;
				// Функция обратного вызова, вывода сообщения при его получении
				function <void (const res_t &, Rest *, void *)> messageFn = nullptr;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 * @param ctx   передаваемый контекст модуля
				 */
				static void chunking(const vector <char> & chunk, const awh::http_t * http, void * ctx) noexcept;
			private:
				/**
				 * openCallback Функция обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void openCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * connectCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void connectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * disconnectCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * connectProxyCallback Функция обратного вызова при подключении к прокси-серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void connectProxyCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * readCallback Функция обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 * @param ctx    передаваемый контекст модуля
				 */
				static void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * readProxyCallback Функция обратного вызова при чтении сообщения с прокси-сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 * @param ctx    передаваемый контекст модуля
				 */
				static void readProxyCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
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
				 * on Метод установки функции обратного вызова при подключении/отключении
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <void (const mode_t, Rest *, void *)> callback) noexcept;
				/**
				 * setMessageCallback Метод установки функции обратного вызова при получении сообщения
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <void (const res_t &, Rest *, void *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для получения чанков
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <void (const vector <char> &, const awh::http_t *, void *)> callback) noexcept;
			public:
				/**
				 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void setWaitTimeDetect(const time_t read, const time_t write) noexcept;
				/**
				 * setBytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept;
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
				 * setUserAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
				 */
				void setUserAgent(const string & userAgent) noexcept;
				/**
				 * setCompress Метод установки метода сжатия
				 * @param метод сжатия сообщений
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
