/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WEBSOCKET_SERVER__
#define __AWH_WEBSOCKET_SERVER__

/**
 * Наши модули
 */
#include <ws/worker.hpp>
#include <server/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WebSocketServer Класс работы с WebSocket сервером
	 */
	typedef class WebSocketServer {
		public:
			/**
			 * Основные флаги приложения
			 */
			enum class flag_t : uint8_t {
				DEFER    = 0x01, // Флаг отложенных вызовов событий сокета
				NOINFO   = 0x02, // Флаг запрещающий вывод информационных сообщений
				WAITMESS = 0x04  // Флаг ожидания входящих сообщений
			};
		private:
			// Хости сервера
			string host = "";
			// Порт сервера
			u_int port = SERVER_PORT;
		private:
			// Создаём объект для работы с фреймом WebSocket
			frame_t frame;
			// Объект рабочего
			workSrvWss_t worker;
			// Создаём объект для компрессии-декомпрессии данных
			mutable hash_t hash;
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
			auth_t::type_t authType = auth_t::type_t::BASIC;
			// Функция обратного вызова для извлечения пароля
			function <string (const string &, void *)> extractPassFn = nullptr;
			// Функция обратного вызова для обработки авторизации
			function <bool (const string &, const string &, void *)> checkAuthFn = nullptr;
		private:
			// Поддерживаемые сабпротоколы
			vector <string> subs;
		private:
			// Флаг шифрования сообщений
			bool crypt = false;
			// Минимальный размер сегмента
			size_t frameSize = 0xFA000;
		private:
			// Список контекстов передаваемых объектов
			vector <void *> ctx = {
				nullptr, nullptr, nullptr,
				nullptr, nullptr, nullptr
			};
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект биндинга TCP/IP
			const coreSrv_t * core = nullptr;
		private:
			// Функция обратного вызова, при запуске или остановки подключения к серверу
			function <void (const size_t, const bool, WebSocketServer *, void *)> openStopFn = nullptr;
			// Функция обратного вызова, при получении ошибки работы клиента
			function <void (const size_t, const u_short, const string &, WebSocketServer *, void *)> errorFn = nullptr;
			// Функция обратного вызова, при получении сообщения с сервера
			function <void (const size_t, const vector <char> &, const bool, WebSocketServer *, void *)> messageFn = nullptr;
		private:
			// Функция разрешения подключения клиента на сервере
			function <bool (const string &, const string &, WebSocketServer *, void *)> acceptFn = nullptr;
		private:
			/**
			 * openCallback Функция обратного вызова при запуске работы
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void openCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * pingCallback Метод пинга адъютанта
			 * @param aid  идентификатор адъютанта
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void pingCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
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
			 * writeCallback Функция обратного вызова при записи сообщения на клиенте
			 * @param size размер записанных в сокет байт
			 * @param aid  идентификатор адъютанта
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void writeCallback(const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
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
		private:
			/**
			 * error Метод вывода сообщений об ошибках работы клиента
			 * @param aid     идентификатор адъютанта
			 * @param message сообщение с описанием ошибки
			 */
			void error(const size_t aid, const mess_t & message) const noexcept;
			/**
			 * extraction Метод извлечения полученных данных
			 * @param adj    параметры подключения адъютанта
			 * @param aid    идентификатор адъютанта
			 * @param core   объект биндинга TCP/IP
			 * @param buffer данные в чистом виде полученные с сервера
			 * @param utf8   данные передаются в текстовом виде
			 */
			void extraction(workSrvWss_t::adjp_t * adj, const size_t aid, core_t * core, const vector <char> & buffer, const bool utf8) const noexcept;
		private:
			/**
			 * pong Метод ответа на проверку о доступности сервера
			 * @param aid  идентификатор адъютанта
			 * @param core объект биндинга TCP/IP
			 * @param      message сообщение для отправки
			 */
			void pong(const size_t aid, core_t * core, const string & message = "") noexcept;
			/**
			 * ping Метод проверки доступности сервера
			 * @param aid  идентификатор адъютанта
			 * @param core объект биндинга TCP/IP
			 * @param      message сообщение для отправки
			 */
			void ping(const size_t aid, core_t * core, const string & message = "") noexcept;
		public:
			/**
			 * init Метод инициализации WebSocket клиента
			 * @param port     порт сервера
			 * @param host     хост сервера
			 * @param compress метод сжатия передаваемых сообщений
			 */
			void init(const u_int port, const string & host = "", const http_t::compress_t compress = http_t::compress_t::DEFLATE) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const size_t, const bool, WebSocketServer *, void *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения ошибок
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const size_t, const u_short, const string &, WebSocketServer *, void *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения сообщений
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const size_t, const vector <char> &, const bool, WebSocketServer *, void *)> callback) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова на событие активации клиента на сервере
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <bool (const string &, const string &, WebSocketServer *, void *)> callback) noexcept;
		public:
			/**
			 * sendError Метод отправки сообщения об ошибке
			 * @param aid  идентификатор адъютанта
			 * @param mess отправляемое сообщение об ошибке
			 */
			void sendError(const size_t aid, const mess_t & mess) const noexcept;
			/**
			 * send Метод отправки сообщения на сервер
			 * @param aid     идентификатор адъютанта
			 * @param message буфер сообщения в бинарном виде
			 * @param size    размер сообщения в байтах
			 * @param utf8    данные передаются в текстовом виде
			 */
			void send(const size_t aid, const char * message, const size_t size, const bool utf8 = true) noexcept;
		public:
			/**
			 * sub Метод получения выбранного сабпротокола
			 * @param aid идентификатор адъютанта
			 * @return    название поддерживаемого сабпротокола
			 */
			const string sub(const size_t aid) const noexcept;
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
			 * stop Метод остановки клиента
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска клиента
			 */
			void start() noexcept;
		public:
			/**
			 * setSub Метод установки подпротокола поддерживаемого сервером
			 * @param sub подпротокол для установки
			 */
			void setSub(const string & sub) noexcept;
			/**
			 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
			 * @param subs подпротоколы для установки
			 */
			void setSubs(const vector <string> & subs) noexcept;
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
			 * setFrameSize Метод установки размеров сегментов фрейма
			 * @param size минимальный размер сегмента
			 */
			void setFrameSize(const size_t size) noexcept;
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
			 * WebSocketServer Конструктор
			 * @param core объект биндинга TCP/IP
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			WebSocketServer(const coreSrv_t * core, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~WebSocketServer Деструктор
			 */
			~WebSocketServer() noexcept {}
	} wsSrv_t;
};

#endif // __AWH_WEBSOCKET_SERVER__
