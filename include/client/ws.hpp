/**
 * @file: ws.hpp
 * @date: 2022-09-03
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

#ifndef __AWH_WEBSOCKET_CLIENT__
#define __AWH_WEBSOCKET_CLIENT__

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/client.hpp>
#include <core/client.hpp>
#include <sys/threadpool.hpp>

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
					ALIVE       = 0x01, // Флаг автоматического поддержания подключения
					NOINFO      = 0x02, // Флаг запрещающий вывод информационных сообщений
					NOTSTOP     = 0x04, // Флаг запрета остановки биндинга
					WAITMESS    = 0x08, // Флаг ожидания входящих сообщений
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
				/**
				 * Buffer Структура буфера данных
				 */
				typedef struct Buffer {
					vector <char> payload; // Бинарный буфер полезной нагрузки
					vector <char> fragmes; // Данные фрагметрированного сообщения
				} buffer_t;
				/**
				 * Callback Структура функций обратного вызова
				 */
				typedef struct Callback {
					// Функция обратного вызова, при запуске или остановки подключения к серверу
					function <void (const mode_t, WebSocket *)> active;
					// Функция обратного вызова, при получении ошибки работы клиента
					function <void (const u_int, const string &, WebSocket *)> error;
					// Функция обратного вызова, при получении сообщения с сервера
					function <void (const vector <char> &, const bool, WebSocket *)> message;
					/**
					 * Callback Конструктор
					 */
					Callback() noexcept : active(nullptr), error(nullptr), message(nullptr) {}
				} fn_t;
			private:
				// Создаем объект для работы с сетью
				net_t _net;
			private:
				// Объект для компрессии-декомпрессии данных
				mutable hash_t _hash;
			private:
				// Мютекс для блокировки потока
				recursive_mutex _mtx;
			private:
				// Объект тредпула для работы с потоками
				thr_t _thr;
				// Объект работы с URI ссылками
				uri_t _uri;
				// Объект для работы с HTTP
				wss_t _http;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект для работы с фреймом WebSocket
				frame_t _frame;
				// Объект разрешения обмена данными
				allow_t _allow;
				// Объект рабочего
				scheme_t _scheme;
				// Объект блокировщика
				locker_t _locker;
				// Экшен события
				action_t _action;
				// Объект буфера данных
				buffer_t _buffer;
			private:
				// Полученный опкод сообщения
				frame_t::opcode_t _opcode;
				// Метод компрессии данных
				http_t::compress_t _compress;
			private:
				// Флаг шифрования сообщений
				bool _crypt;
				// Флаг завершения работы клиента
				bool _close;
				// Выполнять анбиндинг после завершения запроса
				bool _unbind;
				// Флаг фриза работы клиента
				bool _freeze;
				// Флаг запрета вывода информационных сообщений
				bool _noinfo;
				// Флаг принудительной остановки
				bool _stopped;
				// Флаг переданных сжатых данных
				bool _compressed;
				// Флаг переиспользования контекста клиента
				bool _takeOverCli;
				// Флаг переиспользования контекста сервера
				bool _takeOverSrv;
			private:
				// Количество попыток
				uint8_t _attempt;
				// Общее количество попыток
				uint8_t _attempts;
			private:
				// Размер скользящего окна клиента
				short _wbitClient;
				// Размер скользящего окна сервера
				short _wbitServer;
			private:
				// Идентификатор адъютанта
				size_t _aid;
				// Код ответа сервера
				u_int _code;
				// Контрольная точка ответа на пинг
				time_t _checkPoint;
			private:
				// Минимальный размер сегмента
				size_t _frameSize;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект сетевого ядра
				const client::core_t * _core;
			private:
				/**
				 * openCallback Метод обратного вызова при запуске работы
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void openCallback(const size_t sid, awh::core_t * core) noexcept;
				/**
				 * persistCallback Метод персистентного вызова
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
				 * start Метод запуска клиента
				 */
				void start() noexcept;
				/**
				 * pause Метод установки на паузу клиента
				 */
				void pause() noexcept;
			public:
				/**
				 * sub Метод получения выбранного сабпротокола
				 * @return выбранный сабпротокол
				 */
				const string & sub() const noexcept;
				/**
				 * sub Метод установки подпротокола поддерживаемого сервером
				 * @param sub подпротокол для установки
				 */
				void sub(const string & sub) noexcept;
				/**
				 * subs Метод установки списка подпротоколов поддерживаемых сервером
				 * @param subs подпротоколы для установки
				 */
				void subs(const vector <string> & subs) noexcept;
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
				 * multiThreads Метод активации многопоточности
				 * @param threads количество потоков для активации
				 * @param mode    флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const size_t threads = 0, const bool mode = true) noexcept;
			public:
				/**
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * mode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void mode(const u_short flag) noexcept;
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * attempts Метод установки общего количества попыток
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
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
				 * WebSocket Конструктор
				 * @param core объект сетевого ядра
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
