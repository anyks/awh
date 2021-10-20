/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WEBSOCKET_CLIENT__
#define __AWH_WEBSOCKET_CLIENT__

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/client.hpp>
#include <client/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WebSocketClient Класс работы с WebSocket клиентом
	 */
	typedef class WebSocketClient {
		public:
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
		private:
			// Создаём объект работы с URI ссылками
			uri_t uri;
			// Создаём объект для работы с HTTP
			wsc_t http;
			// Создаём объект для работы с фреймом WebSocket
			frame_t frame;
			// Создаем объект для работы с сетью
			network_t nwk;
			// Объект рабочего
			workCli_t worker;
			// Создаём объект для компрессии-декомпрессии данных
			mutable hash_t hash;
		private:
			// Данные фрагметрированного сообщения
			vector <char> fragmes;
		private:
			// Флаг шифрования сообщений
			bool crypt = false;
			// Выполнять анбиндинг после завершения запроса
			bool unbind = true;
			// Флаг фриза работы клиента
			bool freeze = false;
			// Локер ожидания завершения запроса
			bool locker = false;
			// Флаг запрета вывода информационных сообщений
			bool noinfo = false;
			// Флаг проверки аутентификации
			bool failAuth = false;
			// Флаг переданных сжатых данных
			bool compressed = false;
		private:
			// Идентификатор адъютанта
			size_t aid = 0;
			// Код ответа сервера
			u_short code = 0;
			// Контрольная точка ответа на пинг
			time_t checkPoint = 0;
			// Минимальный размер сегмента
			size_t frameSize = 0xFA000;
		public:
			// Полученный опкод сообщения
			frame_t::opcode_t opcode = frame_t::opcode_t::TEXT;
			// Флаги работы с сжатыми данными
			http_t::compress_t compress = http_t::compress_t::NONE;
		private:
			// Список контекстов передаваемых объектов
			vector <void *> ctx = {nullptr, nullptr, nullptr};
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект биндинга TCP/IP
			const coreCli_t * core = nullptr;
		private:
			// Функция обратного вызова, при запуске или остановки подключения к серверу
			function <void (const bool, WebSocketClient *, void *)> openStopFn = nullptr;
			// Функция обратного вызова, при получении ошибки работы клиента
			function <void (const u_short, const string &, WebSocketClient *, void *)> errorFn = nullptr;
			// Функция обратного вызова, при получении сообщения с сервера
			function <void (const vector <char> &, const bool, WebSocketClient *, void *)> messageFn = nullptr;
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
			 * connectProxyCallback Функция обратного вызова при подключении к прокси-серверу
			 * @param aid  идентификатор адъютанта
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void connectProxyCallback(const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * readCallback Функция обратного вызова при чтении сообщения с сервера
			 * @param buffer бинарный буфер содержащий сообщение
			 * @param size   размер бинарного буфера содержащего сообщение
			 * @param aid    идентификатор адъютанта
			 * @param wid    идентификатор воркера
			 * @param core   объект биндинга TCP/IP
			 * @param ctx    передаваемый контекст модуля
			 */
			static void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * readProxyCallback Функция обратного вызова при чтении сообщения с прокси-сервера
			 * @param buffer бинарный буфер содержащий сообщение
			 * @param size   размер бинарного буфера содержащего сообщение
			 * @param aid    идентификатор адъютанта
			 * @param wid    идентификатор воркера
			 * @param core   объект биндинга TCP/IP
			 * @param ctx    передаваемый контекст модуля
			 */
			static void readProxyCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, core_t * core, void * ctx) noexcept;
		private:
			/**
			 * error Метод вывода сообщений об ошибках работы клиента
			 * @param message сообщение с описанием ошибки
			 */
			void error(const mess_t & message) const noexcept;
			/**
			 * extraction Метод извлечения полученных данных
			 * @param buffer данные в чистом виде полученные с сервера
			 * @param utf8   данные передаются в текстовом виде
			 */
			void extraction(const vector <char> & buffer, const bool utf8) const noexcept;
		private:
			/**
			 * pong Метод ответа на проверку о доступности сервера
			 * @param message сообщение для отправки
			 */
			void pong(const string & message = "") noexcept;
			/**
			 * ping Метод проверки доступности сервера
			 * @param message сообщение для отправки
			 */
			void ping(const string & message = "") noexcept;
		public:
			/**
			 * init Метод инициализации WebSocket клиента
			 * @param url      адрес WebSocket сервера
			 * @param compress метод сжатия передаваемых сообщений
			 */
			void init(const string & url, const http_t::compress_t compress = http_t::compress_t::DEFLATE) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const bool, WebSocketClient *, void *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения ошибок
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const u_short, const string &, WebSocketClient *, void *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения сообщений
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void on(void * ctx, function <void (const vector <char> &, const bool, WebSocketClient *, void *)> callback) noexcept;
		public:
			/**
			 * sendError Метод отправки сообщения об ошибке
			 * @param mess отправляемое сообщение об ошибке
			 */
			void sendError(const mess_t & mess) const noexcept;
			/**
			 * send Метод отправки сообщения на сервер
			 * @param message буфер сообщения в бинарном виде
			 * @param size    размер сообщения в байтах
			 * @param utf8    данные передаются в текстовом виде
			 */
			void send(const char * message, const size_t size, const bool utf8 = true) noexcept;
		public:
			/**
			 * stop Метод остановки клиента
			 */
			void stop() noexcept;
			/**
			 * pause Метод установки на паузу клиента
			 */
			void pause() noexcept;
			/**
			 * start Метод запуска клиента
			 */
			void start() noexcept;
		public:
			/**
			 * getSub Метод получения выбранного сабпротокола
			 * @return выбранный сабпротокол
			 */
			const string & getSub() const noexcept;
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
			 * setFrameSize Метод установки размеров сегментов фрейма
			 * @param size минимальный размер сегмента
			 */
			void setFrameSize(const size_t size) noexcept;
			/**
			 * setAttempts Метод установки количества попыток переподключения
			 * @param count количество попыток переподключения
			 */
			void setAttempts(const u_short count) noexcept;
			/**
			 * setUserAgent Метод установки User-Agent для HTTP запроса
			 * @param userAgent агент пользователя для HTTP запроса
			 */
			void setUserAgent(const string & userAgent) noexcept;
			/**
			 * setCompress Метод установки метода сжатия
			 * @param метод сжатия сообщений
			 */
			void setCompress(const http_t::compress_t compress) noexcept;
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
			 * @param alg  алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::alg_t alg = auth_t::alg_t::MD5) noexcept;
			/**
			 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
			 * @param type тип авторизации
			 * @param alg  алгоритм шифрования для Digest авторизации
			 */
			void setAuthTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::alg_t alg = auth_t::alg_t::MD5) noexcept;
		public:
			/**
			 * WebSocketClient Конструктор
			 * @param core объект биндинга TCP/IP
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			WebSocketClient(const coreCli_t * core, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~WebSocketClient Деструктор
			 */
			~WebSocketClient() noexcept {}
	} wsCli_t;
};

#endif // __AWH_WEBSOCKET_CLIENT__
