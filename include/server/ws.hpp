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
			private:
				/**
				 * Статусы игольного ушка
				 */
				enum class needle_t : uint8_t {
					NONE       = 0x00, // Статус не установлен
					MESSAGE    = 0x01, // Сообщение
					CONNECT    = 0x02, // Подключение
					DISCONNECT = 0x03  // Отключение
				};
			public:
				/**
				 * Режим работы адъютанта
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Метод подключения адъютанта
					DISCONNECT = 0x02  // Метод отключения адъютанта
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
				/**
				 * Adjutant Структура активного адъютанта при работе с игольным ушком
				 */
				typedef struct Adjutant {
					u_int port; // Порт адъютанта
					string ip;  // IP адрес адъютанта
					string mac; // MAC адрес адъютанта
					string sub; // Выбранный сабпротокол
					/**
					 * Adjutant Конструктор
					 */
					Adjutant() noexcept : port(0), ip(""), mac(""), sub("") {}
				} adjutant_t;
				/**
				 * Callback Структура функций обратного вызова
				 */
				typedef struct Callback {
					// Функция обратного вызова для извлечения пароля
					function <string (const string &)> extractPass;
					// Функция обратного вызова для обработки авторизации
					function <bool (const string &, const string &)> checkAuth;
					// Функция обратного вызова, при запуске или остановки подключения к серверу
					function <void (const size_t, const mode_t, WebSocket *)> active;
					// Функция обратного вызова, при получении ошибки работы адъютанта
					function <void (const size_t, const u_int, const string &, WebSocket *)> error;
					// Функция разрешения подключения адъютанта на сервере
					function <bool (const string &, const string &, const u_int, WebSocket *)> accept;
					// Функция обратного вызова, при получении сообщения с сервера
					function <void (const size_t, const vector <char> &, const bool, WebSocket *)> message;
					/**
					 * Callback Конструктор
					 */
					Callback() noexcept : extractPass(nullptr), checkAuth(nullptr), active(nullptr), error(nullptr), accept(nullptr), message(nullptr) {}
				} fn_t;
			private:
				// Идентификатор основного процесса
				pid_t _pid;
			private:
				// Порт сервера
				u_int _port;
				// Хости сервера
				string _host;
			private:
				// Объект тредпула для работы с потоками
				thr_t _thr;
				// Создаём объект для работы с фреймом WebSocket
				frame_t _frame;
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Объект рабочего
				ws_worker_t _worker;
			private:
				// Идентификатор сервера
				string _sid;
				// Версия сервера
				string _ver;
				// Название сервера
				string _name;
			private:
				// Название сервера
				string _realm;
				// Временный ключ сессии сервера
				string _opaque;
			private:
				// Пароль шифрования передаваемых данных
				string _pass;
				// Соль шифрования передаваемых данных
				string _salt;
				// Размер шифрования передаваемых данных
				hash_t::cipher_t _cipher;
			private:
				// Алгоритм шифрования для Digest авторизации
				auth_t::hash_t _authHash;
				// Тип авторизации
				auth_t::type_t _authType;
			private:
				// Флаг шифрования сообщений
				bool _crypt;
				// Флаг игольного ушка
				bool _needleEye;
				// Флаг переиспользования контекста клиента
				bool _takeOverCli;
				// Флаг переиспользования контекста сервера
				bool _takeOverSrv;
			private:
				// Минимальный размер сегмента
				size_t _frameSize;
			private:
				// Количество активных потоков
				size_t _threadsCount;
				// Флаг активации работы тредпула
				bool _threadsEnabled;
			private:
				// Поддерживаемые сабпротоколы
				vector <string> _subs;
			private:
				// Список активных адъютантов при работе с игольным ушком
				map <size_t, adjutant_t> _adjutants;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
				// Создаём объект биндинга TCP/IP
				const server::core_t * _core;
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
				 * readCallback Функция обратного вызова при чтении сообщения с адъютанта
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Функция обратного вызова при записи сообщений адъютанту
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 */
				void writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * acceptCallback Функция обратного вызова при проверке подключения адъютанта
				 * @param ip   адрес интернет подключения адъютанта
				 * @param mac  мак-адрес подключившегося адъютанта
				 * @param port порт подключившегося адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @return     результат разрешения к подключению адъютанта
				 */
				bool acceptCallback(const string & ip, const string & mac, const u_int port, const size_t wid, awh::core_t * core) noexcept;
				/**
				 * messageCallback Функция обратного вызова при получении сообщений сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param wid    идентификатор воркера
				 * @param aid    идентификатор адъютанта
				 * @param pid    идентификатор дочернего процесса
				 * @param core   объект биндинга TCP/IP
				 */
				void messageCallback(const char * buffer, const size_t size, const size_t wid, const size_t aid, const pid_t pid, awh::core_t * core) noexcept;
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
				 * error Метод вывода сообщений об ошибках работы адъютанта
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
				 * init Метод инициализации WebSocket адъютанта
				 * @param socket   unix socket для биндинга
				 * @param compress метод сжатия передаваемых сообщений
				 */
				void init(const string & socket, const http_t::compress_t compress = http_t::compress_t::DEFLATE) noexcept;
				/**
				 * init Метод инициализации WebSocket адъютанта
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
				 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
				 * @param callback функция обратного вызова
				 */
				void on(function <bool (const string &, const string &, const u_int, WebSocket *)> callback) noexcept;
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
				 * port Метод получения порта подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    порт подключения адъютанта
				 */
				u_int port(const size_t aid) const noexcept;
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
				 * stop Метод остановки сервера
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска сервера
				 */
				void start() noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param threads количество потоков для активации
				 * @param mode    флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const size_t threads = 0, const bool mode = true) noexcept;
			public:
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
				/**
				 * sub Метод получения выбранного сабпротокола
				 * @param aid идентификатор адъютанта
				 * @return    название поддерживаемого сабпротокола
				 */
				const string sub(const size_t aid) const noexcept;
			public:
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void waitTimeDetect(const time_t read, const time_t write) noexcept;
				/**
				 * bytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void bytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept;
			public:
				/**
				 * realm Метод установки название сервера
				 * @param realm название сервера
				 */
				void realm(const string & realm) noexcept;
				/**
				 * opaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(const string & opaque) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * mode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void mode(const u_short flag) noexcept;
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const u_short total) noexcept;
				/**
				 * needleEye Метод установки флага использования игольного ушка
				 * @param mode флаг активации
				 */
				void needleEye(const bool mode) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * compress Метод установки метода сжатия
				 * @param метод сжатия сообщений
				 */
				void compress(const http_t::compress_t compress) noexcept;
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
				 * WebSocket Конструктор
				 * @param core объект биндинга TCP/IP
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				WebSocket(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~WebSocket Деструктор
				 */
				~WebSocket() noexcept;
		} ws_t;
	};
};

#endif // __AWH_WEBSOCKET_SERVER__
