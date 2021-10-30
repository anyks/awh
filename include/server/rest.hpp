/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_REST_SERVER__
#define __AWH_REST_SERVER__

/**
 * Наши модули
 */
#include <core/server.hpp>
#include <worker/rest.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * RestServer Класс работы с REST сервером
	 */
	typedef class RestServer {
		public:
			/**
			 * Основные флаги приложения
			 */
			enum class flag_t : uint8_t {
				DEFER    = 0x01, // Флаг отложенных вызовов событий сокета
				NOINFO   = 0x02, // Флаг запрещающий вывод информационных сообщений
				WAITMESS = 0x04  // Флаг ожидания входящих сообщений
			};
			/**
			 * Request Структура запроса клиента
			 */
			typedef struct Request {
				string ip, mac;                              // IP и MAC адрес клиента
				web_t::method_t method;                      // Метод запроса
				vector <string> path;                        // Путь URL запроса
				vector <char> entity;                        // Тело запроса
				unordered_map <string, string> params;       // Параметры URL запроса
				unordered_multimap <string, string> headers; // Заголовки клиента
				/**
				 * Request Конструктор
				 */
				Request() : ip(""), mac(""), method(web_t::method_t::NONE) {}
			} req_t;
		private:
			// Хости сервера
			string host = "";
			// Порт сервера
			u_int port = SERVER_PORT;
		private:
			// Создаём объект для компрессии-декомпрессии данных
			mutable hash_t hash;
			// Объект рабочего
			workSrvRest_t worker;
		private:
			// Идентификатор сервера
			string sid = "";
			// Название сервера
			string name = "";
			// Версия сервера
			string version = "";
		private:
			// Пароль шифрования передаваемых данных
			string pass = "";
			// Соль шифрования передаваемых данных
			string salt = "";
			// Размер шифрования передаваемых данных
			hash_t::aes_t aes = hash_t::aes_t::AES128;
		private:
			// Название сервера
			string realm = "";
			// Уникальный ключ клиента
			string nonce = "";
			// Временный ключ сессии сервера
			string opaque = "";
			// Алгоритм шифрования для Digest авторизации
			auth_t::alg_t authAlg = auth_t::alg_t::MD5;
			// Тип авторизации
			auth_t::type_t authType = auth_t::type_t::NONE;
			// Функция обратного вызова для извлечения пароля
			function <string (const string &, void *)> extractPassFn = nullptr;
			// Функция обратного вызова для обработки авторизации
			function <bool (const string &, const string &, void *)> checkAuthFn = nullptr;
		private:
			// Флаг шифрования сообщений
			bool crypt = false;
			// Флаг долгоживущего подключения
			bool alive = false;
		private:
			// Размер одного чанка
			size_t chunkSize = BUFFER_CHUNK;
			// Максимальное количество запросов
			size_t maxRequests = SERVER_MAX_REQUESTS;
		private:
			// Список контекстов передаваемых объектов
			vector <void *> ctx = {
				nullptr, nullptr,
				nullptr, nullptr,
				nullptr, nullptr
			};
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект биндинга TCP/IP
			const coreSrv_t * core = nullptr;
		private:
			// Функция обратного вызова, при получении HTTP чанков от клиента
			function <void (const vector <char> &, const http_t *, void *)> chunkingFn = nullptr;
		private:
			// Функция обратного вызова, при запуске или остановки подключения к серверу
			function <void (const size_t, const bool, RestServer *, void *)> openStopFn = nullptr;
			// Функция обратного вызова, при получении сообщения с сервера
			function <void (const size_t, const req_t &, RestServer *, void *)> messageFn = nullptr;
		private:
			// Функция разрешения подключения клиента на сервере
			function <bool (const string &, const string &, RestServer *, void *)> acceptFn = nullptr;
		private:
			/**
			 * chunking Метод обработки получения чанков
			 * @param chunk бинарный буфер чанка
			 * @param http  объект модуля HTTP
			 * @param ctx   передаваемый контекст модуля
			 */
			static void chunking(const vector <char> & chunk, const http_t * http, void * ctx) noexcept;
		private:
			/**
			 * openCallback Функция обратного вызова при запуске работы
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void openCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * persistCallback Функция персистентного вызова
			 * @param aid  идентификатор адъютанта
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void persistCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * connectCallback Функция обратного вызова при подключении к серверу
			 * @param aid  идентификатор адъютанта
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void connectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * disconnectCallback Функция обратного вызова при отключении от сервера
			 * @param aid  идентификатор адъютанта
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void disconnectCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * acceptCallback Функция обратного вызова при проверке подключения клиента
			 * @param ip   адрес интернет подключения клиента
			 * @param mac  мак-адрес подключившегося клиента
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 * @return     результат разрешения к подключению клиента
			 */
			static bool acceptCallback(const string & ip, const string & mac, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * readCallback Функция обратного вызова при чтении сообщения с клиента
			 * @param buffer бинарный буфер содержащий сообщение
			 * @param size   размер бинарного буфера содержащего сообщение
			 * @param aid    идентификатор адъютанта
			 * @param wid    идентификатор воркера
			 * @param core   объект биндинга TCP/IP
			 * @param ctx    передаваемый контекст модуля
			 */
			static void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * writeCallback Функция обратного вызова при записи сообщения на клиенте
			 * @param buffer бинарный буфер содержащий сообщение
			 * @param size   размер записанных в сокет байт
			 * @param aid    идентификатор адъютанта
			 * @param wid    идентификатор воркера
			 * @param core   объект биндинга TCP/IP
			 * @param ctx    передаваемый контекст модуля
			 */
			static void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
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
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const size_t, const bool, RestServer *, void *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения сообщений
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const size_t, const req_t &, RestServer *, void *)> callback) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова на событие активации клиента на сервере
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <bool (const string &, const string &, RestServer *, void *)> callback) noexcept;
		public:
			/**
			 * reject Метод отправки сообщения об ошибке
			 * @param aid     идентификатор адъютанта
			 * @param code    код сообщения для клиента
			 * @param mess    отправляемое сообщение об ошибке
			 * @param entity  данные полезной нагрузки (тело сообщения)
			 * @param headers HTTP заголовки сообщения
			 */
			void reject(const size_t aid, const u_short code, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) const noexcept;
			/**
			 * response Метод отправки сообщения клиенту
			 * @param aid     идентификатор адъютанта
			 * @param code    код сообщения для клиента
			 * @param mess    отправляемое сообщение об ошибке
			 * @param entity  данные полезной нагрузки (тело сообщения)
			 * @param headers HTTP заголовки сообщения
			 */
			void response(const size_t aid, const u_short code = 200, const string & mess = "", const vector <char> & entity = {}, const unordered_multimap <string, string> & headers = {}) const noexcept;
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
			 * setExtractPassCallback Метод добавления функции извлечения пароля
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова для извлечения пароля
			 */
			void setExtractPassCallback(void * ctx, function <string (const string &, void *)> callback) noexcept;
			/**
			 * setAuthCallback Метод добавления функции обработки авторизации
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова для обработки авторизации
			 */
			void setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept;
			/**
			 * setChunkingFn Метод установки функции обратного вызова для получения чанков
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void setChunkingFn(void * ctx, function <void (const vector <char> &, const http_t *, void *)> callback) noexcept;
		public:
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type тип авторизации
			 * @param alg  алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::alg_t alg = auth_t::alg_t::MD5) noexcept;
		public:
			/**
			 * setMode Метод установки флага модуля
			 * @param flag флаг модуля для установки
			 */
			void setMode(const u_short flag) noexcept;
			/**
			 * setChunkSize Метод установки размера чанка
			 * @param size размер чанка для установки
			 */
			void setChunkSize(const size_t size) noexcept;
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
		public:
			/**
			 * RestServer Конструктор
			 * @param core объект биндинга TCP/IP
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			RestServer(const coreSrv_t * core, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~RestServer Деструктор
			 */
			~RestServer() noexcept {}
	} restSrv_t;
};

#endif // __AWH_REST_SERVER__
