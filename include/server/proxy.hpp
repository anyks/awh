/**
 * @file: proxy.hpp
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

#ifndef __AWH_PROXY_SERVER__
#define __AWH_PROXY_SERVER__

/**
 * Наши модули
 */
#include <core/client.hpp>
#include <core/server.hpp>
#include <worker/proxy.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Proxy Класс работы с HTTP PROXY сервером
		 */
		typedef class Proxy {
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01,
					DISCONNECT = 0x02
				};
				/**
				 * Режим событие клиента
				 */
				enum class event_t : uint8_t {
					REQUEST  = 0x01,
					RESPONSE = 0x02
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					DEFER     = 0x01, // Флаг отложенных вызовов событий сокета
					NOINFO    = 0x02, // Флаг запрещающий вывод информационных сообщений
					WAITMESS  = 0x04, // Флаг ожидания входящих сообщений
					NOCONNECT = 0x08  // Флаг запрещающий метод CONNECT
				};
			private:
				/**
				 * Core Объект биндинга TCP/IP
				 */
				typedef struct Core {
					client::core_t client; // Объект биндинга TCP/IP для клиента
					server::core_t server; // Объект биндинга TCP/IP для сервера
					/**
					 * Core Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Core(const fmk_t * fmk, const log_t * log) noexcept : client(fmk, log), server(fmk, log) {}
				} core_t;
			private:
				// Хости сервера
				string _host = "";
				// Порт сервера
				u_int _port = SERVER_PORT;
			private:
				// Объект биндинга TCP/IP
				core_t _core;
				// Объект рабочего для сервера
				workerProxy_t _worker;
			private:
				// Название сервера
				string _name = AWH_NAME;
				// Идентификатор сервера
				string _sid = AWH_SHORT_NAME;
				// Версия сервера
				string _version = AWH_VERSION;
			private:
				// Пароль шифрования передаваемых данных
				string _pass = "";
				// Соль шифрования передаваемых данных
				string _salt = "";
				// Размер шифрования передаваемых данных
				hash_t::aes_t _aes = hash_t::aes_t::AES128;
			private:
				// Название сервера
				string _realm = "";
				// Временный ключ сессии сервера
				string _opaque = "";
				// Алгоритм шифрования для Digest авторизации
				auth_t::hash_t _authHash = auth_t::hash_t::MD5;
				// Тип авторизации
				auth_t::type_t _authType = auth_t::type_t::NONE;
				// Функция обратного вызова для извлечения пароля
				function <string (const string &)> _extractPassFn = nullptr;
				// Функция обратного вызова для обработки авторизации
				function <bool (const string &, const string &)> _checkAuthFn = nullptr;
			private:
				// Флаг шифрования сообщений
				bool _crypt = false;
				// Флаг долгоживущего подключения
				bool _alive = false;
				// Флаг запрещающий метод CONNECT
				bool _noConnect = false;
			private:
				// Размер одного чанка
				size_t _chunkSize = BUFFER_CHUNK;
				// Максимальный интервал времени жизни подключения
				size_t _keepAlive = KEEPALIVE_TIMEOUT;
				// Максимальное количество запросов
				size_t _maxRequests = SERVER_MAX_REQUESTS;
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
			private:
				// Функция обратного вызова, при получении HTTP чанков от клиента
				function <void (const vector <char> &, const http_t *)> _chunkingFn = nullptr;
			private:
				// Функция обратного вызова, при запуске или остановки подключения к серверу
				function <void (const size_t, const mode_t, Proxy *)> _activeFn = nullptr;
				// Функция обратного вызова, при получении сообщения с сервера
				function <bool (const size_t, const event_t, http_t *, Proxy *)> _messageFn = nullptr;
				// Функция обратного вызова, при получении сообщения в бинарном виде с сервера
				function <bool (const size_t, const event_t, const char *, const size_t, Proxy *)> _binaryFn = nullptr;
			private:
				// Функция разрешения подключения клиента на сервере
				function <bool (const string &, const string &, Proxy *)> _acceptFn = nullptr;
			private:
				/**
				 * runCallback Функция обратного вызова при активации ядра сервера
				 * @param mode флаг запуска/остановки
				 * @param core объект биндинга TCP/IP
				 */
				static void runCallback(const bool mode, awh::core_t * core) noexcept;
				/**
				 * chunkingCallback Функция обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				static void chunkingCallback(const vector <char> & chunk, const http_t * http) noexcept;
			private:
				/**
				 * openServerCallback Функция обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				static void openServerCallback(const size_t wid, awh::core_t * core) noexcept;
				/**
				 * persistServerCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				static void persistServerCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * connectClientCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				static void connectClientCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * connectServerCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				static void connectServerCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * disconnectClientCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				static void disconnectClientCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * disconnectServerCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				static void disconnectServerCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
				 * @param ip   адрес интернет подключения клиента
				 * @param mac  мак-адрес подключившегося клиента
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @return     результат разрешения к подключению клиента
				 */
				static bool acceptServerCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				static void readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				static void readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				static void writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
			private:
				/**
				 * prepare Метод обработки входящих данных
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void prepare(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
			public:
				/**
				 * init Метод инициализации WebSocket клиента
				 * @param port     порт сервера
				 * @param host     хост сервера
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const u_int port, const string & host = "", const http_t::compress_t compress = http_t::compress_t::NONE) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const mode_t, Proxy *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const size_t, const event_t, http_t *, Proxy *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const size_t, const event_t, const char *, const size_t, Proxy *)> callback) noexcept;
			public:
				/**
				 * on Метод добавления функции извлечения пароля
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void on(function <string (const string &)> callback) noexcept;
				/**
				 * on Метод добавления функции обработки авторизации
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void on(function <bool (const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для получения чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const http_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие активации клиента на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, Proxy *)> callback) noexcept;
			public:
				/**
				 * reject Метод отправки сообщения об ошибке
				 * @param aid     идентификатор адъютанта
				 * @param code    код сообщения для клиента
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void reject(const size_t aid, const u_int code, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) const noexcept;
				/**
				 * response Метод отправки сообщения клиенту
				 * @param aid     идентификатор адъютанта
				 * @param code    код сообщения для клиента
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void response(const size_t aid, const u_int code = 200, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) const noexcept;
			public:
				/**
				 * ip Метод получения IP адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес интернет подключения адъютанта
				 */
				const string & ip(const size_t aid) const noexcept;
				/**
				 * mac Метод получения MAC адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес устройства адъютанта
				 */
				const string & mac(const size_t aid) const noexcept;
			public:
				/**
				 * setAlive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void setAlive(const bool mode) noexcept;
				/**
				 * setAlive Метод установки долгоживущего подключения
				 * @param aid  идентификатор адъютанта
				 * @param mode флаг долгоживущего подключения
				 */
				void setAlive(const size_t aid, const bool mode) noexcept;
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
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
			public:
				/**
				 * setRealm Метод установки название сервера
				 * @param realm название сервера
				 */
				void setRealm(const string & realm) noexcept;
				/**
				 * setOpaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				void setOpaque(const string & opaque) noexcept;
			public:
				/**
				 * setAuthType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * setMode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void setMode(const u_short flag) noexcept;
				/**
				 * setTotal Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void setTotal(const u_short total) noexcept;
				/**
				 * setChunkSize Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void setChunkSize(const size_t size) noexcept;
				/**
				 * setKeepAlive Метод установки времени жизни подключения
				 * @param time время жизни подключения
				 */
				void setKeepAlive(const size_t time) noexcept;
				/**
				 * setMaxRequests Метод установки максимального количества запросов
				 * @param max максимальное количество запросов
				 */
				void setMaxRequests(const size_t max) noexcept;
				/**
				 * setCompress Метод установки метода сжатия
				 * @param метод сжатия сообщений
				 */
				void setCompress(const http_t::compress_t compress) noexcept;
				/**
				 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void setWaitTimeDetect(const time_t read, const time_t write) noexcept;
				/**
				 * setServ Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void setServ(const string & id, const string & name, const string & ver) noexcept;
				/**
				 * setBytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept;
				/**
				 * setCrypt Метод установки параметров шифрования
				 * @param pass пароль шифрования передаваемых данных
				 * @param salt соль шифрования передаваемых данных
				 * @param aes  размер шифрования передаваемых данных
				 */
				void setCrypt(const string & pass, const string & salt = "", const hash_t::aes_t aes = hash_t::aes_t::AES128) noexcept;
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

#endif // __AWH_PROXY_SERVER__
