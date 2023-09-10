/**
 * @file: web.hpp
 * @date: 2022-11-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_WEB_CLIENT__
#define __AWH_WEB_CLIENT__

/**
 * Стандартная библиотека
 */
#include <functional>
#include <nlohmann/json.hpp>

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/client.hpp>
#include <http/client.hpp>
#include <core/client.hpp>
#include <sys/threadpool.hpp>

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
		 * WEB Класс работы с WEB клиентом
		 */
		typedef class WEB {
			private:
				/**
				 * Этапы обработки
				 */
				enum class status_t : uint8_t {
					STOP = 0x00, // Остановить обработку
					NEXT = 0x01, // Следующий этап обработки
					SKIP = 0x02  // Пропустить этап обработки
				};
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
				 * Идентификатор агента
				 */
				enum class agent_t : uint8_t {
					HTTP      = 0x01, // HTTP-клиент
					WEBSOCKET = 0x02  // WebSocket-клиент
				};
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Флаг подключения
					DISCONNECT = 0x02  // Флаг отключения
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE           = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO        = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP        = 0x03, // Флаг запрета остановки биндинга
					WAIT_MESS       = 0x04, // Флаг ожидания входящих сообщений
					VERIFY_SSL      = 0x05, // Флаг выполнения проверки сертификата SSL
					REDIRECTS       = 0x06, // Флаг разрешающий автоматическое перенаправление запросов
					TAKEOVER_CLIENT = 0x07, // Флаг ожидания входящих сообщений для клиента
					TAKEOVER_SERVER = 0x08  // Флаг ожидания входящих сообщений для сервера
				};
			public:
				/**
				 * Request Структура запроса клиента
				 */
				typedef struct Request {
					string query;                                // Строка HTTP запроса
					web_t::method_t method;                      // Метод запроса
					vector <char> entity;                        // Тело запроса
					unordered_multimap <string, string> headers; // Заголовки клиента
					/**
					 * Request Конструктор
					 */
					Request() noexcept : query(""), method(web_t::method_t::NONE) {}
				} req_t;
			private:
				/**
				 * Buffer Структура буфера данных
				 */
				typedef struct Buffer {
					vector <char> payload; // Бинарный буфер полезной нагрузки
					vector <char> fragmes; // Данные фрагметрированного сообщения
				} buffer_t;
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
				/**
				 * Allow Структура флагов разрешения обменом данных
				 */
				typedef struct Allow {
					bool send;    // Флаг разрешения отправки данных
					bool receive; // Флаг разрешения чтения данных
					/**
					 * Allow Конструктор
					 */
					Allow() noexcept : send(true), receive(true) {}
				} __attribute__((packed)) allow_t;
				/**
				 * Partner Структура партнёра
				 */
				typedef struct Partner {
					short wbit;    // Размер скользящего окна
					bool takeOver; // Флаг скользящего контекста сжатия
					/**
					 * Partner Конструктор
					 */
					Partner() noexcept : wbit(0), takeOver(false) {}
				} __attribute__((packed)) partner_t;
				/**
				 * Http2 Структура работы с клиентом HTTP/2
				 */
				typedef struct Http2 {
					bool mode;             // Флаг активации модуля HTTP/2
					int32_t id;            // Идентификатор сессии
					nghttp2_session * ctx; // Контекст сессии
					/**
					 * Http2 Конструктор
					 */
					Http2() noexcept : mode(false), id(-1), ctx(nullptr) {}
				} http2_t;
				/**
				 * Frame Объект фрейма WebSocket
				 */
				typedef struct Frame {
					size_t size;                  // Минимальный размер сегмента
					ws::frame_t methods;          // Методы работы с фреймом WebSocket
					ws::frame_t::opcode_t opcode; // Полученный опкод сообщения
					/**
					 * Frame Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Frame(const fmk_t * fmk, const log_t * log) noexcept :
					 size(0xFA000), methods(fmk, log), opcode(ws::frame_t::opcode_t::TEXT) {}
				} frame_t;
				/**
				 * WebHttp Структура HTTP-клиента
				 */
				typedef struct WebHttp {
					uri_t uri;               // Объект работы с URI ссылками
					http_t http;             // Объект для работы с HTTP
					fn_t callback;           // Объект работы с функциями обратного вызова
					vector <char> buffer;    // Объект буфера данных
					vector <req_t> requests; // Список активых запросов
					/**
					 * WebHttp Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					WebHttp(const fmk_t * fmk, const log_t * log) noexcept : uri(fmk), http(fmk, log, &uri), callback(log) {}
				} web_http_t;
				/**
				 * WebSocket Структура WebSocket-клиента
				 */
				typedef struct WebSocket {
					bool crypt;       // Флаг шифрования сообщений
					bool close;       // Флаг завершения работы клиента
					bool noinfo;      // Флаг запрета вывода информационных сообщений
					bool freeze;      // Флаг фриза работы клиента
					bool deflate;     // Флаг переданных сжатых данных
					time_t point;     // Контрольная точка ответа на пинг
					uri_t uri;        // Объект работы с URI ссылками
					ws_t http;        // Объект для работы с HTTP запросами
					hash_t hash;      // Объект для компрессии-декомпрессии данных
					frame_t frame;    // Объект для работы с фреймом WebSocket
					allow_t allow;    // Объект разрешения обмена данными
					thr_t threads;    // Объект тредпула для работы с потоками
					ws::mess_t mess;  // Объект сформированного сообщения
					buffer_t buffer;  // Объект буфера данных
					partner_t client; // Объект партнёра клиента
					partner_t server; // Объект партнёра сервера
					/**
					 * WebSocket Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					WebSocket(const fmk_t * fmk, const log_t * log) noexcept :
					 crypt(false), close(false), noinfo(false), freeze(false), deflate(false),
					 point(0), uri(fmk), http(fmk, log, &uri), hash(log), frame(fmk, log) {}
				} web_socket_t;
			private:
				// Идентификатор подключения
				size_t _aid;
			private:
				// Выполнять анбиндинг после завершения запроса
				bool _unbind;
				// Флаг принудительного отключения
				bool _active;
				// Флаг принудительной остановки
				bool _stopped;
				// Флаг выполнения редиректов
				bool _redirects;
			private:
				// Количество попыток
				uint8_t _attempt;
				// Общее количество попыток
				uint8_t _attempts;
			private:
				// Объект работы с Web-клиентом
				web_http_t _web;
				// Объект работы с WebSocket-клиентом
				web_socket_t _ws;
			private:
				// Протокол поддерживаемый клиентом
				agent_t _agent;
				// Экшен события
				action_t _action;
			private:
				// Метод компрессии данных
				http_t::compress_t _compress;
			private:
				// Объект для работы с HTTP/2
				http2_t _http2;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего
				scheme_t _scheme;
				// Объект блокировщика
				locker_t _locker;
			private:
				// Список доступных источников
				vector <string> _origins;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект сетевого ядра
				const client::core_t * _core;
			private:
				/**
				 * debugHttp2 Функция обратного вызова при получении отладочной информации
				 * @param format формат вывода отладочной информации
				 * @param args   список аргументов отладочной информации
				 */
				static void debugHttp2(const char * format, va_list args) noexcept;
			private:
				/**
				 * onFrameHttp2 Функция обратного вызова при получении фрейма заголовков HTTP/2 с сервера
				 * @param session объект сессии HTTP/2
				 * @param frame   объект фрейма заголовков HTTP/2
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученных данных
				 */
				static int onFrameHttp2(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
				/**
				 * onCloseHttp2 Метод закрытия подключения с сервером HTTP/2
				 * @param session объект сессии HTTP/2
				 * @param sid     идентификатор сессии HTTP/2
				 * @param error   флаг ошибки HTTP/2 если присутствует
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученного события
				 */
				static int onCloseHttp2(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept;
				/**
				 * onChunkHttp2 Функция обратного вызова при получении чанка с сервера HTTP/2
				 * @param session объект сессии HTTP/2
				 * @param flags   флаги события для сессии HTTP/2
				 * @param sid     идентификатор сессии HTTP/2
				 * @param buffer  буфер данных который содержит полученный чанк
				 * @param size    размер полученного буфера данных чанка
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученных данных
				 */
				static int onChunkHttp2(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * onBeginHeadersHttp2 Функция начала получения фрейма заголовков HTTP/2
				 * @param session объект сессии HTTP/2
				 * @param frame   объект фрейма заголовков HTTP/2
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученных данных
				 */
				static int onBeginHeadersHttp2(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
				/**
				 * onHeaderHttp2 Функция обратного вызова при получении заголовка HTTP/2
				 * @param session объект сессии HTTP/2
				 * @param frame   объект фрейма заголовков HTTP/2
				 * @param key     данные ключа заголовка
				 * @param keySize размер ключа заголовка
				 * @param val     данные значения заголовка
				 * @param valSize размер значения заголовка
				 * @param flags   флаги события для сессии HTTP/2
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученных данных
				 */
				static int onHeaderHttp2(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept;
			private:
				/**
				 * sendHttp2 Функция обратного вызова при подготовки данных для отправки на сервер
				 * @param session объект сессии HTTP/2
				 * @param buffer  буфер данных которые следует отправить
				 * @param size    размер буфера данных для отправки
				 * @param flags   флаги события для сессии HTTP/2
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        количество отправленных байт
				 */
				static ssize_t sendHttp2(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept;
				/**
				 * readHttp2 Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить на HTTP/2 сервер
				 * @param session объект сессии HTTP/2
				 * @param sid     идентификатор сессии HTTP/2
				 * @param buffer  буфер данных которые следует отправить
				 * @param size    размер буфера данных для отправки
				 * @param flags   флаги события для сессии HTTP/2
				 * @param source  объект промежуточных данных локального подключения
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        количество отправленных байт
				 */
				static ssize_t readHttp2(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const vector <char> & chunk, const awh::http_t * http) noexcept;
			private:
				/**
				 * ping2 Метод выполнения пинга сервера
				 * @return результат работы пинга
				 */
				bool ping2() noexcept;
				/**
				 * ping Метод проверки доступности сервера
				 * @param message сообщение для отправки
				 */
				void ping(const string & message) noexcept;
				/**
				 * pong Метод ответа на проверку о доступности сервера
				 * @param message сообщение для отправки
				 */
				void pong(const string & message) noexcept;
			private:
				/** 
				 * submit Метод выполнения удалённого запроса на сервер
				 * @param request объект запроса на удалённый сервер
				 */
				void submit(const req_t & request) noexcept;
			private:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openCallback(const size_t sid, awh::core_t * core) noexcept;
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Метод обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * enableTLSCallback Метод активации зашифрованного канала TLS
				 * @param url  адрес сервера для которого выполняется активация зашифрованного канала TLS
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат активации зашифрованного канала TLS
				 */
				bool enableTLSCallback(const uri_t::url_t & url, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void proxyConnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
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
				 * prepare Метод выполнения препарирования полученных данных
				 * @return результат препарирования
				 */
				status_t prepare() noexcept;
			private:
				/**
				 * error Метод вывода сообщений об ошибках работы клиента
				 * @param message сообщение с описанием ошибки
				 */
				void error(const ws::mess_t & message) const noexcept;
				/**
				 * extraction Метод извлечения полученных данных
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param utf8   данные передаются в текстовом виде
				 */
				void extraction(const vector <char> & buffer, const bool utf8) noexcept;
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
				 * REQUEST Метод выполнения запроса HTTP
				 * @param method  метод запроса
				 * @param url     адрес запроса
				 * @param entity  тело запроса
				 * @param headers заголовки запроса
				 */
				void REQUEST(const web_t::method_t method, const uri_t::url_t & url, vector <char> & entity, unordered_multimap <string, string> & headers) noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
			public:
				/**
				 * init Метод инициализации WEB клиента
				 * @param url      адрес WEB сервера
				 * @param compress метод компрессии передаваемых сообщений
				 */
				void init(const string & url, const http_t::compress_t compress = http_t::compress_t::ALL_COMPRESS) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const mode_t)> callback) noexcept;
				/** 
				 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &)> callback) noexcept;
				/** 
				 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const u_int, const string &)> callback) noexcept;
				/** 
				 * on Метод установки функции вывода полученного заголовка с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const string &, const string &)> callback) noexcept;
				/** 
				 * on Метод установки функции вывода полученного тела данных с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const u_int, const string &, const vector <char> &)> callback) noexcept;
				/** 
				 * on Метод установки функции вывода полученных заголовков с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для перехвата полученных чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept;
			public:
				/**
				 * sendTimeout Метод отправки сигнала таймаута
				 */
				void sendTimeout() noexcept;
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const ws::mess_t & mess) noexcept;
				/**
				 * send Метод отправки сообщения на сервер
				 * @param reqs список запросов
				 */
				void send(const vector <req_t> & reqs = {}) noexcept;
			public:
				/**
				 * setOrigin Метод установки списка разрешенных источников для HTTP/2
				 * @param origins список разрешённых источников
				 */
				void setOrigin(const vector <string> & origins) noexcept;
				/**
				 * sendOrigin Метод отправки списка разрешенных источников для HTTP/2
				 * @param origins список разрешённых источников
				 */
				void sendOrigin(const vector <string> & origins) noexcept;
			public:
				/**
				 * open Метод открытия подключения
				 */
				void open() noexcept;
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
			public:
				/**
				 * bytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const time_t read = READ_TIMEOUT, const time_t write = WRITE_TIMEOUT, const time_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * attempts Метод установки общего количества попыток
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
				/**
				 * userAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * compress Метод установки метода компрессии
				 * @param compress метод компрессии сообщений
				 */
				void compress(const http_t::compress_t compress) noexcept;
				/**
				 * user Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int cnt, const int idle, const int intvl) noexcept;
				/**
				 * serv Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void serv(const string & id, const string & name, const string & ver) noexcept;
				/**
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
				/**
				 * authTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * WEB Конструктор
				 * @param agent идентификатор агента
				 * @param core  объект сетевого ядра
				 * @param fmk   объект фреймворка
				 * @param log   объект для работы с логами
				 */
				WEB(const agent_t agent, const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~WEB Деструктор
				 */
				~WEB() noexcept {}
		} web_t;
	};
};

#endif // __AWH_WEB_CLIENT__
