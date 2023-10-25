/**
 * @file: web.hpp
 * @date: 2023-09-11
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

#ifndef __AWH_WEB_CLIENT__
#define __AWH_WEB_CLIENT__

/**
 * Стандартная библиотека
 */
#include <map>
#include <stack>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/hold.hpp>
#include <net/uri.hpp>
#include <http/http2.hpp>
#include <http/client.hpp>
#include <core/client.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Web Базовый класс web-клиента
		 */
		typedef class Web {
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					OPEN       = 0x01, // Открытие передачи данных
					CLOSE      = 0x02, // Закрытие передачи данных
					CONNECT    = 0x03, // Флаг подключения
					DISCONNECT = 0x04  // Флаг отключения
				};
				/**
				 * Направления передачи фреймов
				 */
				enum class direct_t : uint8_t {
					NONE = 0x00, // Направление не установлено
					SEND = 0x01, // Направление отправки
					RECV = 0x02  // Направление получения
				};
				/**
				 * Идентификатор агента
				 */
				enum class agent_t : uint8_t {
					NONE      = 0x00, // Агент не определён
					HTTP      = 0x01, // HTTP-клиент
					WEBSOCKET = 0x02  // WebSocket-клиент
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					ALIVE            = 0x01, // Флаг автоматического поддержания подключения
					NOT_INFO         = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP         = 0x03, // Флаг запрета остановки биндинга
					WAIT_MESS        = 0x04, // Флаг ожидания входящих сообщений
					VERIFY_SSL       = 0x05, // Флаг выполнения проверки сертификата SSL
					REDIRECTS        = 0x06, // Флаг разрешающий автоматическое перенаправление запросов
					PROXY_NOCONNECT  = 0x07, // Флаг отключающий метод CONNECT для прокси-клиента
					TAKEOVER_CLIENT  = 0x08, // Флаг ожидания входящих сообщений для клиента
					TAKEOVER_SERVER  = 0x09, // Флаг ожидания входящих сообщений для сервера
					WEBSOCKET_ENABLE = 0x0A  // Флаг разрешения использования WebSocket-клиента
				};
			public:
				/**
				 * Request Структура запроса клиента
				 */
				typedef struct Request {
					uri_t::url_t url;                            // URL-запроса запроса
					web_t::method_t method;                      // Метод запроса
					vector <char> entity;                        // Тело запроса
					vector <http_t::compress_t> compressors;     // Список поддерживаемых компрессоров
					unordered_multimap <string, string> headers; // Заголовки клиента
					/**
					 * Request Конструктор
					 */
					Request() noexcept : method(web_t::method_t::NONE) {}
				} request_t;
			protected:
				/**
				 * Proxy Структура работы с прокси-сервером
				 */
				typedef struct Proxy {
					int32_t sid;  // Идентификатор потока HTTP/2
					bool connect; // Флаг применения метода CONNECT
					u_int answer; // Статус ответа прокси-сервера
					/**
					 * Proxy Конструктор
					 */
					Proxy() noexcept : sid(-1), connect(true), answer(0) {}
				} __attribute__((packed)) proxy_t;
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
					Encryption() noexcept : mode(false), pass{""}, salt{""}, cipher(hash_t::cipher_t::AES128) {}
				} encryption_t;
			protected:
				/**
				 * Этапы обработки
				 */
				enum class status_t : uint8_t {
					STOP = 0x00, // Остановить обработку
					NEXT = 0x01, // Следующий этап обработки
					SKIP = 0x02  // Пропустить этап обработки
				};
				/**
				 * Идентификаторы текущего события
				 */
				enum class event_t : uint8_t {
					NONE          = 0x00, // Событие не установлено
					OPEN          = 0x01, // Событие открытия подключения
					READ          = 0x02, // Событие чтения данных с сервера
					SEND          = 0x03, // Событие отправки данных на сервер
					SUBMIT        = 0x04, // Событие HTTP-запроса на удаленный сервер
					CONNECT       = 0x05, // Событие подключения к серверу
					PROXY_READ    = 0x06, // Событие чтения данных с прокси-сервера
					PROXY_CONNECT = 0x07  // Событие подключения к прокси-серверу
				};
			protected:
				// Идентификатор подключения
				uint64_t _bid;
			protected:
				// Объект работы с URI ссылками
				uri_t _uri;
				// Объект параметров работы с прокси-сервером
				proxy_t _proxy;
				// Объект функций обратного вызова
				fn_t _callback;
				// Объект рабочего
				scheme_t _scheme;
				// Объект параметров шифрования
				encryption_t _encryption;
			protected:
				// Выполнять анбиндинг после завершения запроса
				bool _unbind;
				// Флаг принудительного отключения
				bool _active;
				// Флаг принудительной остановки
				bool _stopped;
				// Флаг выполнения редиректов
				bool _redirects;
			protected:
				// Количество попыток
				uint8_t _attempt;
				// Общее количество попыток
				uint8_t _attempts;
			private:
				// Ядро для локального таймера
				awh::core_t _timer;
			protected:
				// Объект буфера данных
				vector <char> _buffer;
			protected:
				// Список рабочих событий
				stack <event_t> _events;
			protected:
				// Список поддерживаемых компрессоров
				vector <http_t::compress_t> _compressors;
			protected:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект сетевого ядра
				const client::core_t * _core;
			protected:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openCallback(const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				virtual void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
			protected:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept = 0;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept = 0;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				virtual void readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept = 0;
			protected:
				/**
				 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void proxyConnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				virtual void proxyReadCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * enableSSLCallback Метод активации зашифрованного канала SSL
				 * @param url  адрес сервера для которого выполняется активация зашифрованного канала SSL
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат активации зашифрованного канала SSL
				 */
				bool enableSSLCallback(const uri_t::url_t & url, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			protected:
				/**
				 * chunking Метод обработки получения чанков
				 * @param bid   идентификатор брокера
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				virtual void chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept;
			protected:
				/**
				 * errors Метод вывода полученных ошибок протокола
				 * @param bid     идентификатор брокера
				 * @param flag    флаг типа сообщения
				 * @param error   тип полученной ошибки
				 * @param message сообщение полученной ошибки
				 */
				void errors(const uint64_t bid, const log_t::flag_t flag, const awh::http::error_t error, const string & message) noexcept;
			protected:
				/**
				 * flush Метод сброса параметров запроса
				 */
				virtual void flush() noexcept = 0;
			protected:
				/**
				 * pinging Метод таймера выполнения пинга удалённого сервера
				 * @param tid  идентификатор таймера
				 * @param core объект сетевого ядра
				 */
				virtual void pinging(const uint16_t tid, awh::core_t * core) noexcept = 0;
			protected:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param id   идентификатор запроса
				 * @param bid  идентификатор брокера
				 * @param core объект сетевого ядра
				 * @return     результат препарирования
				 */
				virtual status_t prepare(const int32_t id, const uint64_t bid, client::core_t * core) noexcept = 0;
			public:
				/**
				 * init Метод инициализации клиента
				 * @param dest        адрес назначения удалённого сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & dest, const vector <awh::http_t::compress_t> & compressors = {}) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const mode_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для перехвата полученных чанков
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибки
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept;
			public:
				/**
				 * on Метод установки функция обратного вызова активности потока
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const mode_t)> callback) noexcept;
				/**
				 * on on Метод установки функция обратного вызова при выполнении рукопожатия
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const agent_t)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова при завершении запроса
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const direct_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const u_int, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного заголовка с сервера
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного тела данных с сервера
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученных заголовков с сервера
				 * @param callback функция обратного вызова
				 */
				virtual void on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * sendTimeout Метод отправки сигнала таймаута
				 */
				void sendTimeout() noexcept;
			public:
				/**
				 * open Метод открытия подключения
				 */
				void open() noexcept;
			public:
				/**
				 * stop Метод остановки клиента
				 */
				virtual void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				virtual void start() noexcept;
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
				virtual void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * attempts Метод установки общего количества попыток
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				virtual void core(const client::core_t * core) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int cnt, const int idle, const int intvl) noexcept;
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <awh::http_t::compress_t> & compressors) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				virtual void mode(const set <flag_t> & flags) noexcept = 0;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				virtual void chunk(const size_t size) noexcept = 0;
				/**
				 * user Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				virtual void user(const string & login, const string & password) noexcept = 0;
			public:
				/**
				 * userAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
				 */
				virtual void userAgent(const string & userAgent) noexcept;
				/**
				 * ident Метод установки идентификации клиента
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				virtual void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				virtual void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept = 0;
				/**
				 * authTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				virtual void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * encryption Метод активации шифрования
				 * @param mode флаг активации шифрования
				 */
				virtual void encryption(const bool mode) noexcept;
				/**
				 * encryption Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				virtual void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * Web Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Web(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Web Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Web(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Web Деструктор
				 */
				virtual ~Web() noexcept {}
		} web_t;
		/**
		 * Web2 Базовый класс web2-клиента
		 */
		typedef class Web2 : public web_t {
			public:
				// Количество потоков по умолчанию
				static constexpr uint32_t CONCURRENT_STREAMS = 128;
				// Максимальный размер таблицы заголовков по умолчанию
				static constexpr uint32_t HEADER_TABLE_SIZE = 4096;
				// Минимальный размер фрейма по умолчанию
				static constexpr uint32_t MAX_FRAME_SIZE_MIN = 16384;
				// Максимальный размер фрейма по умолчанию
				static constexpr uint32_t MAX_FRAME_SIZE_MAX = 16777215;
				// Максимальный размер окна по умолчанию
				static constexpr uint32_t MAX_WINDOW_SIZE = 2147483647;
			public:
				/**
				 * Параметры настроек HTTP/2
				 */
				enum class settings_t : uint8_t {
					NONE              = 0x00, // Настройки не установлены
					STREAMS           = 0x01, // Максимальное количество потоков
					FRAME_SIZE        = 0x02, // Максимальный размер фрейма
					ENABLE_PUSH       = 0x03, // Разрешение присылать пуш-уведомления
					WINDOW_SIZE       = 0x04, // Максимальный размер окна полезной нагрузки
					HEADER_TABLE_SIZE = 0x05  // Максимальный размер таблицы заголовков
				};
			protected:
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
					Ident() noexcept : id{""}, ver{""}, name{""} {}
				} ident_t;
			protected:
				// Объект идентификации сервиса
				ident_t _ident;
				// Объект работы с фреймами Http2
				http2_t _http2;
			protected:
				// Логин пользователя для авторизации на сервере
				string _login;
				// Пароль пользователя для авторизации на сервере
				string _password;
			protected:
				// User-Agent для HTTP запроса
				string _userAgent;
			protected:
				// Размер одного чанка бинарных данных
				size_t _chunkSize;
			private:
				// Список параметров настроек протокола HTTP/2
				map <settings_t, uint32_t> _settings;
			protected:
				/**
				 * sendSignal Метод обратного вызова при отправки данных HTTP/2
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера данных для отправки
				 */
				void sendSignal(const uint8_t * buffer, const size_t size) noexcept;
			protected:
				/**
				 * frameProxySignal Метод обратного вызова при получении фрейма заголовков прокси-сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param direct направление передачи фрейма
				 * @param type   тип полученного фрейма
				 * @param flags  флаг полученного фрейма
				 * @return       статус полученных данных
				 */
				int frameProxySignal(const int32_t sid, const http2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept;
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param direct направление передачи фрейма
				 * @param type   тип полученного фрейма
				 * @param flags  флаг полученного фрейма
				 * @return       статус полученных данных
				 */
				virtual int frameSignal(const int32_t sid, const http2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept = 0;
			protected:
				/**
				 * chunkProxySignal Метод обратного вызова при получении чанка с прокси-сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int chunkProxySignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * chunkSignal Метод обратного вызова при получении чанка с сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				virtual int chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept = 0;
			protected:
				/**
				 * beginProxySignal Метод начала получения фрейма заголовков HTTP/2 прокси-сервера
				 * @param sid идентификатор потока
				 * @return    статус полученных данных
				 */
				int beginProxySignal(const int32_t sid) noexcept;
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @return    статус полученных данных
				 */
				virtual int beginSignal(const int32_t sid) noexcept = 0;
			protected:
				/**
				 * streamClosedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param error флаг ошибки HTTP/2 если присутствует
				 * @return      статус полученных данных
				 */
				virtual int closedSignal(const int32_t sid, const uint32_t error) noexcept = 0;
			protected:
				/**
				 * headerProxySignal Метод обратного вызова при получении заголовка HTTP/2 прокси-сервера
				 * @param sid идентификатор потока
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int headerProxySignal(const int32_t sid, const string & key, const string & val) noexcept;
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				virtual int headerSignal(const int32_t sid, const string & key, const string & val) noexcept = 0;
			protected:
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			protected:
				/**
				 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
				 * @param bid  идентификатор брокера
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void proxyConnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void proxyReadCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * implementation Метод выполнения активации сессии HTTP/2
				 * @param bid  идентификатор брокера
				 * @param core объект сетевого ядра
				 */
				void implementation(const uint64_t bid, client::core_t * core) noexcept;
			protected:
				/**
				 * ping Метод выполнения пинга сервера
				 * @return результат работы пинга
				 */
				bool ping() noexcept;
			public:
				/**
				 * send Метод отправки сообщения на сервер
				 * @param id     идентификатор потока HTTP/2
				 * @param buffer буфер бинарных данных передаваемых на сервер
				 * @param size   размер сообщения в байтах
				 * @param flag   флаг передаваемого потока по сети
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const int32_t id, const char * buffer, const size_t size, const http2_t::flag_t flag) noexcept;
				/**
				 * send Метод отправки заголовков на сервер
				 * @param id      идентификатор потока HTTP/2
				 * @param headers заголовки отправляемые на сервер
				 * @param flag    флаг передаваемого потока по сети
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const int32_t id, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				virtual void mode(const set <flag_t> & flags) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				virtual void settings(const map <settings_t, uint32_t> & settings = {}) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * userAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
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
				 * Web2 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Web2(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Web2 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Web2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Web2 Деструктор
				 */
				virtual ~Web2() noexcept {}
		} web2_t;
	};
};

#endif // __AWH_WEB_CLIENT__
