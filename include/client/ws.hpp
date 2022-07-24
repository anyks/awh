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

#ifndef __AWH_WEBSOCKET_CLIENT__
#define __AWH_WEBSOCKET_CLIENT__

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/client.hpp>
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
		 * WebSocket Класс работы с WebSocket клиентом
		 */
		typedef class WebSocket {
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
					CONNECT    = 0x01, // Флаг подключения
					DISCONNECT = 0x02  // Флаг отключения
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					NOINFO      = 0x01, // Флаг запрещающий вывод информационных сообщений
					NOTSTOP     = 0x02, // Флаг запрета остановки биндинга
					WAITMESS    = 0x04, // Флаг ожидания входящих сообщений
					KEEPALIVE   = 0x08, // Флаг автоматического поддержания подключения
					VERIFYSSL   = 0x10, // Флаг выполнения проверки сертификата SSL
					TAKEOVERCLI = 0x20, // Флаг ожидания входящих сообщений для клиента
					TAKEOVERSRV = 0x40  // Флаг ожидания входящих сообщений для сервера
				};
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
				} allow_t;
			private:
				// Создаем объект для работы с сетью
				network_t nwk;
			private:
				// Объект для компрессии-декомпрессии данных
				mutable hash_t hash;
			private:
				// Объект работы с URI ссылками
				uri_t uri;
				// Объект для работы с HTTP
				wss_t http;
				// Объект для работы с фреймом WebSocket
				frame_t frame;
				// Объект разрешения обмена данными
				allow_t allow;
				// Объект рабочего
				worker_t worker;
				// Объект блокировщика
				locker_t locker;
				// Экшен события
				action_t action;
			private:
				// Данные фрагметрированного сообщения
				vector <char> fragmes;
				// Буфер бинарных необработанных данных
				vector <char> bufferRead;
				// Буфер бинарных обработанных данных
				vector <char> bufferWrite;
			public:
				// Полученный опкод сообщения
				frame_t::opcode_t opcode;
				// Метод компрессии данных
				http_t::compress_t compress;
			private:
				// Флаг шифрования сообщений
				bool crypt = false;
				// Флаг завершения работы клиента
				bool close = false;
				// Выполнять анбиндинг после завершения запроса
				bool unbind = true;
				// Флаг фриза работы клиента
				bool freeze = false;
				// Флаг запрета вывода информационных сообщений
				bool noinfo = false;
				// Флаг принудительной остановки
				bool stopped = false;
				// Флаг проверки аутентификации
				bool failAuth = false;
				// Флаг переданных сжатых данных
				bool compressed = false;
				// Флаг переиспользования контекста клиента
				bool takeOverCli = false;
				// Флаг переиспользования контекста сервера
				bool takeOverSrv = false;
			private:
				// Идентификатор адъютанта
				size_t aid = 0;
				// Код ответа сервера
				u_int code = 0;
				// Контрольная точка ответа на пинг
				time_t checkPoint = 0;
				// Минимальный размер сегмента
				size_t frameSize = 0xFA000;
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
				// Создаём объект биндинга TCP/IP
				const client::core_t * core = nullptr;
			private:
				// Функция обратного вызова, при запуске или остановки подключения к серверу
				function <void (const mode_t, WebSocket *)> activeFn = nullptr;
				// Функция обратного вызова, при получении ошибки работы клиента
				function <void (const u_int, const string &, WebSocket *)> errorFn = nullptr;
				// Функция обратного вызова, при получении сообщения с сервера
				function <void (const vector <char> &, const bool, WebSocket *)> messageFn = nullptr;
			private:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void openCallback(const size_t wid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Метод обратного вызова при записи сообщения на клиенте
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void writeCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * persistCallback Метод персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 */
				void persistCallback(const size_t aid, const size_t wid, awh::core_t * core) noexcept;
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
				 * error Метод вывода сообщений об ошибках работы клиента
				 * @param message сообщение с описанием ошибки
				 */
				void error(const mess_t & message) const noexcept;
				/**
				 * extraction Метод извлечения полученных данных
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param utf8   данные передаются в текстовом виде
				 */
				void extraction(const vector <char> & buffer, const bool utf8) noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
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
				 * @param compress метод компрессии передаваемых сообщений
				 */
				void init(const string & url, const http_t::compress_t compress = http_t::compress_t::DEFLATE) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const mode_t, WebSocket *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибок
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const u_int, const string &, WebSocket *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const bool, WebSocket *)> callback) noexcept;
			public:
				/**
				 * sendTimeout Метод отправки сигнала таймаута
				 */
				void sendTimeout() noexcept;
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const mess_t & mess) noexcept;
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
				 * setUserAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
				 */
				void setUserAgent(const string & userAgent) noexcept;
				/**
				 * setCompress Метод установки метода компрессии
				 * @param compress метод компрессии сообщений
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
				 * WebSocket Конструктор
				 * @param core объект биндинга TCP/IP
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				WebSocket(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~WebSocket Деструктор
				 */
				~WebSocket() noexcept {}
		} ws_t;
	};
};

#endif // __AWH_WEBSOCKET_CLIENT__
