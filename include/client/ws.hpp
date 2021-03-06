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
					DEFER       = 0x01, // Флаг отложенных вызовов событий сокета
					NOINFO      = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOTSTOP     = 0x04, // Флаг запрета остановки биндинга
					WAITMESS    = 0x08, // Флаг ожидания входящих сообщений
					KEEPALIVE   = 0x10, // Флаг автоматического поддержания подключения
					VERIFYSSL   = 0x20, // Флаг выполнения проверки сертификата SSL
					TAKEOVERCLI = 0x40, // Флаг ожидания входящих сообщений для клиента
					TAKEOVERSRV = 0x80  // Флаг ожидания входящих сообщений для сервера
				};
			private:
				// Создаем объект для работы с сетью
				network_t nwk;
			private:
				// Создаём объект работы с URI ссылками
				uri_t uri;
				// Создаём флаг режима работы модуля
				mode_t mode;
				// Создаём объект для работы с фреймом WebSocket
				frame_t frame;
				// Создаём объект для компрессии-декомпрессии данных
				mutable hash_t hash;
			private:
				// Создаём объект для работы с HTTP
				client::wss_t http;
				// Объект рабочего
				client::worker_t worker;
			private:
				// Буфер бинарных необработанных данных
				vector <char> buffer;
				// Данные фрагметрированного сообщения
				vector <char> fragmes;
			private:
				// Флаг шифрования сообщений
				bool crypt = false;
				// Флаг завершения работы клиента
				bool close = false;
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
			private:
				// Количество полученных байт для закрытия подключения
				size_t readBytes = 0;
				// Количество байт для закрытия подключения
				size_t stopBytes = 0;
				// Минимальный размер сегмента
				size_t frameSize = 0xFA000;
			public:
				// Полученный опкод сообщения
				frame_t::opcode_t opcode = frame_t::opcode_t::TEXT;
				// Метод компрессии данных
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
				const client::core_t * core = nullptr;
			private:
				// Функция обратного вызова, при запуске или остановки подключения к серверу
				function <void (const mode_t, WebSocket *, void *)> activeFn = nullptr;
				// Функция обратного вызова, при получении ошибки работы клиента
				function <void (const u_int, const string &, WebSocket *, void *)> errorFn = nullptr;
				// Функция обратного вызова, при получении сообщения с сервера
				function <void (const vector <char> &, const bool, WebSocket *, void *)> messageFn = nullptr;
			private:
				/**
				 * openCallback Функция обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void openCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void persistCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
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
				 * writeCallback Функция обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 * @param ctx    передаваемый контекст модуля
				 */
				static void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
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
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <void (const mode_t, WebSocket *, void *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибок
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <void (const u_int, const string &, WebSocket *, void *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <void (const vector <char> &, const bool, WebSocket *, void *)> callback) noexcept;
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
