/**
 * @file: ws.hpp
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

#ifndef __AWH_WEBSOCKET_SERVER__
#define __AWH_WEBSOCKET_SERVER__

/**
 * Наши модули
 */
#include <worker/ws.hpp>
#include <core/server.hpp>
#include <sys/threadpool.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * WebSocket Класс работы с WebSocket сервером
		 */
		typedef class WebSocket {
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
					NOINFO      = 0x01, // Флаг запрещающий вывод информационных сообщений
					WAITMESS    = 0x02, // Флаг ожидания входящих сообщений
					TAKEOVERCLI = 0x04, // Флаг переиспользования контекста клиента
					TAKEOVERSRV = 0x08  // Флаг переиспользования контекста сервера
				};
			private:
				// Хости сервера
				string host = "";
				// Порт сервера
				u_int port = SERVER_PORT;
			private:
				// Объект тредпула для работы с потоками
				thr_t thr;
				// Создаём объект для работы с фреймом WebSocket
				frame_t frame;
				// Объект рабочего
				ws_worker_t worker;
			private:
				// Название сервера
				string name = AWH_NAME;
				// Идентификатор сервера
				string sid = AWH_SHORT_NAME;
				// Версия сервера
				string version = AWH_VERSION;
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
			private:
				// Алгоритм шифрования для Digest авторизации
				auth_t::hash_t authHash = auth_t::hash_t::MD5;
				// Тип авторизации
				auth_t::type_t authType = auth_t::type_t::NONE;
			private:
				// Поддерживаемые сабпротоколы
				vector <string> subs;
			private:
				// Флаг шифрования сообщений
				bool crypt = false;
				// Флаг переиспользования контекста клиента
				bool takeOverCli = false;
				// Флаг переиспользования контекста сервера
				bool takeOverSrv = false;
				// Минимальный размер сегмента
				size_t frameSize = 0xFA000;
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
				// Создаём объект биндинга TCP/IP
				const server::core_t * core = nullptr;
			private:
				// Функция обратного вызова для извлечения пароля
				function <string (const string &)> extractPassFn = nullptr;
				// Функция обратного вызова для обработки авторизации
				function <bool (const string &, const string &)> checkAuthFn = nullptr;
			private:
				// Функция обратного вызова, при запуске или остановки подключения к серверу
				function <void (const size_t, const mode_t, WebSocket *)> activeFn = nullptr;
				// Функция разрешения подключения клиента на сервере
				function <bool (const string &, const string &, WebSocket *)> acceptFn = nullptr;
				// Функция обратного вызова, при получении ошибки работы клиента
				function <void (const size_t, const u_int, const string &, WebSocket *)> errorFn = nullptr;
				// Функция обратного вызова, при получении сообщения с сервера
				function <void (const size_t, const vector <char> &, const bool, WebSocket *)> messageFn = nullptr;
			private:
				/**
				 * openCallback Функция обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void openCallback(const size_t wid, awh::core_t * core) noexcept;
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void persistCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * connectCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void connectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void disconnectCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * acceptCallback Функция обратного вызова при проверке подключения клиента
				 * @param ip   адрес интернет подключения клиента
				 * @param mac  мак-адрес подключившегося клиента
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @return     результат разрешения к подключению клиента
				 */
				bool acceptCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * readCallback Функция обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Функция обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
			private:
				/**
				 * handler Метод управления входящими методами
				 * @param aid идентификатор адъютанта
				 */
				void handler(const size_t aid) noexcept;
			private:
				/**
				 * actionRead Метод обработки экшена чтения с сервера
				 * @param aid идентификатор адъютанта
				 */
				void actionRead(const size_t aid) noexcept;
				/**
				 * actionConnect Метод обработки экшена подключения к серверу
				 * @param aid идентификатор адъютанта
				 */
				void actionConnect(const size_t aid) noexcept;
				/**
				 * actionDisconnect Метод обработки экшена отключения от сервера
				 * @param aid идентификатор адъютанта
				 */
				void actionDisconnect(const size_t aid) noexcept;
			private:
				/**
				 * error Метод вывода сообщений об ошибках работы клиента
				 * @param aid     идентификатор адъютанта
				 * @param message сообщение с описанием ошибки
				 */
				void error(const size_t aid, const mess_t & message) const noexcept;
				/**
				 * extraction Метод извлечения полученных данных
				 * @param aid    идентификатор адъютанта
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param utf8   данные передаются в текстовом виде
				 */
				void extraction(const size_t aid, const vector <char> & buffer, const bool utf8) const noexcept;
			private:
				/**
				 * pong Метод ответа на проверку о доступности сервера
				 * @param aid  идентификатор адъютанта
				 * @param core объект биндинга TCP/IP
				 * @param      message сообщение для отправки
				 */
				void pong(const size_t aid, awh::core_t * core, const string & message = "") noexcept;
				/**
				 * ping Метод проверки доступности сервера
				 * @param aid  идентификатор адъютанта
				 * @param core объект биндинга TCP/IP
				 * @param      message сообщение для отправки
				 */
				void ping(const size_t aid, awh::core_t * core, const string & message = "") noexcept;
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
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const mode_t, WebSocket *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибок
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const u_int, const string &, WebSocket *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const size_t, const vector <char> &, const bool, WebSocket *)> callback) noexcept;
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
				 * on Метод установки функции обратного вызова на событие активации клиента на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, WebSocket *)> callback) noexcept;
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
				 * @param unix Флаг запуска для работы с UnixSocket
				 */
				void start(const bool unix = false) noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param threads количество потоков для активации
				 * @param mode    флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const size_t threads = 0, const bool mode = true) noexcept;
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
				/**
				 * getSub Метод получения выбранного сабпротокола
				 * @param aid идентификатор адъютанта
				 * @return    название поддерживаемого сабпротокола
				 */
				const string getSub(const size_t aid) const noexcept;
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
				 * WebSocket Конструктор
				 * @param core объект биндинга TCP/IP
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				WebSocket(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~WebSocket Деструктор
				 */
				~WebSocket() noexcept {}
		} ws_t;
	};
};

#endif // __AWH_WEBSOCKET_SERVER__
