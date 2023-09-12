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
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/uri.hpp>
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
			protected:
				/**
				 * Этапы обработки
				 */
				enum class status_t : uint8_t {
					STOP = 0x00, // Остановить обработку
					NEXT = 0x01, // Следующий этап обработки
					SKIP = 0x02  // Пропустить этап обработки
				};
			protected:
				// Идентификатор подключения
				size_t _aid;
			protected:
				// Объект работы с URI ссылками
				uri_t _uri;
				// Объект работы с функциями обратного вызова
				fn_t _callback;
				// Объект рабочего
				scheme_t _scheme;
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
			protected:
				// Метод компрессии данных
				http_t::compress_t _compress;
			protected:
				// Объект буфера данных
				vector <char> _buffer;
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
				void openCallback(const size_t sid, awh::core_t * core) noexcept;
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				virtual void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
			protected:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept = 0;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept = 0;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				virtual void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept = 0;
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
				 * chunking Метод обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const vector <char> & chunk, const awh::http_t * http) noexcept;
			protected:
				/**
				 * flush Метод сброса параметров запроса
				 */
				virtual void flush() noexcept = 0;
			protected:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @return результат препарирования
				 */
				virtual status_t prepare() noexcept = 0;
			public:
				/**
				 * init Метод инициализации WEB клиента
				 * @param dest     адрес назначения удалённого сервера
				 * @param compress метод компрессии передаваемых сообщений
				 */
				void init(const string & dest, const http_t::compress_t compress = http_t::compress_t::ALL_COMPRESS) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const mode_t)> callback) noexcept;
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
				 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного заголовка с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного тела данных с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученных заголовков с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept;
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
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * attempts Метод установки общего количества попыток
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
				/**
				 * compress Метод установки метода компрессии
				 * @param compress метод компрессии сообщений
				 */
				void compress(const http_t::compress_t compress) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int cnt, const int idle, const int intvl) noexcept;
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
				 * serv Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				virtual void serv(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * authTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				virtual void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept = 0;
			public:
				/**
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				virtual void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept = 0;
			public:
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
			protected:
				/**
				 * Serv Структура идентификации сервиса
				 */
				typedef struct Serv {
					string id;   // Идентификатор сервиса
					string ver;  // Версия сервиса
					string name; // Название сервиса
					/**
					 * Serv Конструктор
					 */
					Serv() noexcept : id{""}, ver{""}, name{""} {}
				} serv_t;
				/**
				 * Crypto Структура параметров шифрования
				 */
				typedef struct Crypto {
					string pass;             // Пароль шифрования передаваемых данных
					string salt;             // Соль шифрования передаваемых данных
					hash_t::cipher_t cipher; // Размер шифрования передаваемых данных
					/**
					 * Crypto Конструктор
					 */
					Crypto() noexcept : pass{""}, salt{""}, cipher(hash_t::cipher_t::AES128) {}
				} crypto_t;
			protected:
				// Объект идентификации сервиса
				serv_t _serv;
				// Объект параметров шифрования
				crypto_t _crypto;
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
			protected:
				// Тип авторизации
				auth_t::type_t _authType;
				// Алгоритм шифрования для Digest-авторизации
				auth_t::hash_t _authHash;
			protected:
				// Флаг активации сессии HTTP/2
				bool _sessionMode;
				// Ессия HTTP/2 подключения
				nghttp2_session * _session;
			protected:
				/**
				 * debug Функция обратного вызова при получении отладочной информации
				 * @param format формат вывода отладочной информации
				 * @param args   список аргументов отладочной информации
				 */
				static void debug(const char * format, va_list args) noexcept;
			protected:
				/**
				 * onFrame Функция обратного вызова при получении фрейма заголовков HTTP/2 с сервера
				 * @param session объект сессии HTTP/2
				 * @param frame   объект фрейма заголовков HTTP/2
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученных данных
				 */
				static int onFrame(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
				/**
				 * onClose Функция закрытия подключения с сервером HTTP/2
				 * @param session объект сессии HTTP/2
				 * @param sid     идентификатор сессии HTTP/2
				 * @param error   флаг ошибки HTTP/2 если присутствует
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученного события
				 */
				static int onClose(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept;
				/**
				 * onChunk Функция обратного вызова при получении чанка с сервера HTTP/2
				 * @param session объект сессии HTTP/2
				 * @param flags   флаги события для сессии HTTP/2
				 * @param sid     идентификатор сессии HTTP/2
				 * @param buffer  буфер данных который содержит полученный чанк
				 * @param size    размер полученного буфера данных чанка
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученных данных
				 */
				static int onChunk(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			protected:
				/**
				 * onBeginHeaders Функция начала получения фрейма заголовков HTTP/2
				 * @param session объект сессии HTTP/2
				 * @param frame   объект фрейма заголовков HTTP/2
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        статус полученных данных
				 */
				static int onBeginHeaders(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
				/**
				 * onHeader Функция обратного вызова при получении заголовка HTTP/2
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
				static int onHeader(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept;
			protected:
				/**
				 * onSend Функция обратного вызова при подготовки данных для отправки на сервер
				 * @param session объект сессии HTTP/2
				 * @param buffer  буфер данных которые следует отправить
				 * @param size    размер буфера данных для отправки
				 * @param flags   флаги события для сессии HTTP/2
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        количество отправленных байт
				 */
				static ssize_t onSend(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept;
				/**
				 * onRead Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить на HTTP/2 сервер
				 * @param session объект сессии HTTP/2
				 * @param sid     идентификатор сессии HTTP/2
				 * @param buffer  буфер данных которые следует отправить
				 * @param size    размер буфера данных для отправки
				 * @param flags   флаги события для сессии HTTP/2
				 * @param source  объект промежуточных данных локального подключения
				 * @param ctx     передаваемый промежуточный контекст
				 * @return        количество отправленных байт
				 */
				static ssize_t onRead(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept;
			protected:
				/**
				 * receivedFrame Метод обратного вызова при получении фрейма заголовков HTTP/2 с сервера
				 * @param frame   объект фрейма заголовков HTTP/2
				 * @return        статус полученных данных
				 */
				virtual int receivedFrame(const nghttp2_frame * frame) noexcept = 0;
				/**
				 * receivedChunk Метод обратного вызова при получении чанка с сервера HTTP/2
				 * @param sid    идентификатор сессии HTTP/2
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				virtual int receivedChunk(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept = 0;
			protected:
				/**
				 * receivedBeginHeaders Метод начала получения фрейма заголовков HTTP/2
				 * @param sid идентификатор сессии HTTP/2
				 * @return    статус полученных данных
				 */
				virtual int receivedBeginHeaders(const int32_t sid) noexcept = 0;
				/**
				 * receivedHeader Метод обратного вызова при получении заголовка HTTP/2
				 * @param sid идентификатор сессии HTTP/2
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				virtual int receivedHeader(const int32_t sid, const string & key, const string & val) noexcept = 0;
			protected:
				/**
				 * eventsCallback Функция обратного вызова при активации ядра сервера
				 * @param status флаг запуска/остановки
				 * @param core   объект сетевого ядра
				 */
				void eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept;
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			protected:
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				virtual void persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept = 0;
			protected:
				/**
				 * ping Метод выполнения пинга сервера
				 * @return результат работы пинга
				 */
				bool ping() noexcept;
			public:
				/**
				 * send Метод отправки сообщения на сервер
				 * @param id      идентификатор потока HTTP/2
				 * @param message сообщение передаваемое на сервер
				 * @param size    размер сообщения в байтах
				 * @param end     флаг последнего сообщения после которого поток закрывается
				 */
				void send(const int32_t id, const char * message, const size_t size, const bool end) noexcept;
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
				 * serv Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void serv(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				virtual void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
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
