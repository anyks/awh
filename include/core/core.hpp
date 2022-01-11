/**
 * @file: core.hpp
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

#ifndef __AWH_CORE__
#define __AWH_CORE__

/**
 * Стандартная библиотека
 */
#include <map>
#include <mutex>
#include <string>
#include <functional>
#include <unordered_map>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <time.h>
// Если - это Unix
#else
	#include <ctime>
#endif

/**
 * Если операционной системой является Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	// Подключаем заголовочный файл
	#include <synchapi.h>
/**
 * Для всех остальных операционных систем
 */
#else
	// Подключаем заголовочный файл
	#include <unistd.h>
#endif

/**
 * Наши модули
 */
#include <fmk.hpp>
#include <ssl.hpp>
#include <dns.hpp>
#include <socket.hpp>
#include <worker/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Класс ядра биндинга TCP/IP
	 */
	typedef class Core {
		protected:
			/**
			 * Тип запускаемого ядра
			 */
			enum class type_t : uint8_t {CLIENT, SERVER};
			/**
			 * Режимы работы клиента
			 */
			enum class mode_t : uint8_t {CONNECT, RECONNECT, DISCONNECT};
		public:
			/**
			 * Основные методы режимов работы
			 */
			enum class method_t : uint8_t {READ, WRITE};
		private:
			/**
			 * Timer Структура таймера
			 */
			typedef struct Timer {
				u_short id;                                               // Идентификатор таймера
				void * ctx;                                               // Передаваемый контекст
				Core * core;                                              // Родительский объект
				bool persist;                                             // Флаг персистентной работы
				struct event ev;                                          // Объект события
				struct timeval tv;                                        // Параметры интервала времени
				function <void (const u_short, Core *, void *)> callback; // Функция обратного вызова
				/**
				 * Timer Конструктор
				 */
				Timer() : id(0), ctx(nullptr), core(nullptr), persist(false), callback(nullptr) {}
			} timer_t;
		protected:
			/**
			 * KeepAlive Структура с параметрами для постоянного подключения
			 */
			typedef struct KeepAlive {
				int keepcnt;   // Максимальное количество попыток
				int keepidle;  // Интервал времени в секундах через которое происходит проверка подключения
				int keepintvl; // Интервал времени в секундах между попытками
				/**
				 * KeepAlive Конструктор
				 */
				KeepAlive() : keepcnt(3), keepidle(1), keepintvl(2) {}
			} __attribute__((packed)) alive_t;
			/**
			 * Network Структура текущих параметров сети
			 */
			typedef struct Network {
				// Тип протокола интернета AF_INET или AF_INET6
				int family;
				// Параметры для сети IPv4
				pair <vector <string>, vector <string>> v4;
				// Параметры для сети IPv6
				pair <vector <string>, vector <string>> v6;
				/**
				 * Network Конструктор
				 */
				Network() : family(AF_INET), v4({{"0.0.0.0"}, IPV4_RESOLVER}), v6({{"[::0]"}, IPV6_RESOLVER}) {}
			} net_t;
			/**
			 * Socket Структура сокета
			 */
			typedef struct Socket {
				evutil_socket_t fd;          // Файловый дескриптор
				struct sockaddr_in client;   // Параметры подключения клиента IPv4
				struct sockaddr_in server;   // Параметры подключения сервера IPv4
				struct sockaddr_in6 client6; // Параметры подключения клиента IPv6
				struct sockaddr_in6 server6; // Параметры подключения сервера IPv6
				/**
				 * Socket Конструктор
				 */
				Socket() : fd(-1), client({}), server({}), client6({}), server6({}) {}
			} socket_t;
		protected:
			// Сетевые параметры
			net_t net;
			// Создаём объект работы с URI
			uri_t uri;
			// Создаём объект для работы с SSL
			ssl_t ssl;
			// Создаём объект DNS IPv4 резолвера
			dns_t dns4;
			// Создаём объект DNS IPv6 резолвера
			dns_t dns6;
			// Параметры постоянного подключения
			alive_t alive;
			// Создаем объект сети
			network_t nwk;
			// Мютекс для блокировки потока
			mutex bloking;
		protected:
			// Тип запускаемого ядра
			type_t type = type_t::CLIENT;
		protected:
			// Событие таймаута запуска системы
			struct event timeout;
			// Событие интервала пинга
			struct event interval;
		protected:
			// Структура интервала таймаута
			struct timeval tvTimeout;
			// Структура интервала пинга
			struct timeval tvInterval;
		private:
			// Список активных таймеров
			map <u_short, timer_t> timers;
		protected:
			// Список активных воркеров
			map <size_t, const worker_t *> workers;
			// Список подключённых клиентов
			map <size_t, const worker_t::adj_t *> adjutants;
		protected:
			// Флаг отложенных вызовов событий сокета
			bool defer = true;
			// Флаг простого чтения базы событий
			bool easy = false;
			// Флаг разрешения работы
			bool mode = false;
			// Флаг блокировку инициализации базы событий
			bool locker = false;
			// Флаг запрета вывода информационных сообщений
			bool noinfo = false;
			// Флаг персистентного запуска каллбека
			bool persist = false;
			// Флаг разрешающий работу только с IPv6
			bool ipV6only = false;
		private:
			// Интервал персистентного таймера в миллисекундах
			time_t persistInterval = PERSIST_INTERVAL;
		protected:
			// Список контекстов передаваемых объектов
			vector <void *> ctx = {nullptr};
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
		protected:
			// База данных событий
			struct event_base * base = nullptr;
		protected:
			// Функция обратного вызова при запуске/остановке модуля
			function <void (const bool, Core * core, void *)> callbackFn = nullptr;
		private:
			/**
			 * run Функция вызова при активации базы событий
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 * @param ctx   передаваемый контекст
			 */
			static void run(evutil_socket_t fd, short event, void * ctx) noexcept;
			/**
			 * timer Функция обработки события пользовательского таймера
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 * @param ctx   передаваемый контекст
			 */
			static void timer(evutil_socket_t fd, short event, void * ctx) noexcept;
			/**
			 * reconnect Функция задержки времени на реконнект
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 * @param ctx   передаваемый контекст
			 */
			static void reconnect(evutil_socket_t fd, short event, void * ctx) noexcept;
			/**
			 * persistent Функция персистентного вызова по таймеру
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 * @param ctx   передаваемый контекст
			 */
			static void persistent(evutil_socket_t fd, short event, void * ctx) noexcept;
		protected:
			/**
			 * reconnect Метод запуска переподключения
			 * @param wid идентификатор воркера
			 */
			void reconnect(const size_t wid) noexcept;
			/**
			 * connect Метод создания подключения к удаленному серверу
			 * @param wid идентификатор воркера
			 */
			virtual void connect(const size_t wid) noexcept;
		protected:
			/**
			 * clean Метод буфера событий
			 * @param bev буфер событий для очистки
			 */
			void clean(struct bufferevent * bev) noexcept;
		protected:
			/**
			 * socket Метод создания сокета
			 * @param ip     адрес для которого нужно создать сокет
			 * @param port   порт сервера для которого нужно создать сокет
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       параметры подключения к серверу
			 */
			const socket_t socket(const string & ip, const u_int port, const int family = AF_INET) const noexcept;
		public:
			/**
			 * bind Метод подключения модуля ядра к текущей базе событий
			 * @param core модуль ядра для подключения
			 */
			void bind(Core * core) noexcept;
			/**
			 * unbind Метод отключения модуля ядра от текущей базы событий
			 * @param core модуль ядра для отключения
			 */
			void unbind(Core * core) noexcept;
		public:
			/**
			 * setCallback Метод установки функции обратного вызова при запуске/остановки работы модуля
			 * @param ctx      передаваемый объект контекста
			 * @param callback функция обратного вызова для установки
			 */
			void setCallback(void * ctx, function <void (const bool, Core * core, void *)> callback) noexcept;
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
			 * working Метод проверки на запуск работы
			 * @return результат проверки
			 */
			bool working() const noexcept;
		public:
			/**
			 * add Метод добавления воркера в биндинг
			 * @param worker воркер для добавления
			 * @return       идентификатор воркера в биндинге
			 */
			size_t add(const worker_t * worker) noexcept;
		public:
			/**
			 * closeAll Метод отключения всех воркеров
			 */
			virtual void closeAll() noexcept;
			/**
			 * removeAll Метод удаления всех воркеров
			 */
			virtual void removeAll() noexcept;
		public:
			/**
			 * run Метод запуска сервера воркером
			 * @param wid идентификатор воркера
			 */
			virtual void run(const size_t wid) noexcept;
			/**
			 * open Метод открытия подключения воркером
			 * @param wid идентификатор воркера
			 */
			virtual void open(const size_t wid) noexcept;
			/**
			 * remove Метод удаления воркера из биндинга
			 * @param wid идентификатор воркера
			 */
			virtual void remove(const size_t wid) noexcept;
		public:
			/**
			 * close Метод закрытия подключения воркера
			 * @param aid идентификатор адъютанта
			 */
			virtual void close(const size_t aid) noexcept;
		public:
			/**
			 * setBandwidth Метод установки пропускной способности сети
			 * @param aid   идентификатор адъютанта
			 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
			 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
			 */
			virtual void setBandwidth(const size_t aid, const string & read, const string & write) noexcept;
		public:
			/**
			 * write Метод записи буфера данных воркером
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param aid    идентификатор адъютанта
			 */
			void write(const char * buffer, const size_t size, const size_t aid) noexcept;
			/**
			 * setLockMethod Метод блокировки метода режима работы
			 * @param method метод режима работы
			 * @param mode   флаг блокировки метода
			 * @param aid    идентификатор адъютанта
			 */
			void setLockMethod(const method_t method, const bool mode, const size_t aid) noexcept;
			/**
			 * setDataTimeout Метод установки таймаута ожидания появления данных
			 * @param method  метод режима работы
			 * @param seconds время ожидания в секундах
			 * @param aid     идентификатор адъютанта
			 */
			void setDataTimeout(const method_t method, const time_t seconds, const size_t aid) noexcept;
			/**
			 * setMark Метод установки маркера на размер детектируемых байт
			 * @param method метод режима работы
			 * @param min    минимальный размер детектируемых байт
			 * @param min    максимальный размер детектируемых байт
			 * @param aid    идентификатор адъютанта
			 */
			void setMark(const method_t method, const size_t min, const size_t max, const size_t aid) noexcept;
		public:
			/**
			 * clearTimers Метод очистки всех таймеров
			 */
			void clearTimers() noexcept;
			/**
			 * clearTimer Метод очистки таймера
			 * @param id идентификатор таймера для очистки
			 */
			void clearTimer(const u_short id) noexcept;
			/**
			 * setTimeout Метод установки таймаута
			 * @param ctx      передаваемый контекст
			 * @param delay    задержка времени в миллисекундах
			 * @param callback функция обратного вызова
			 * @return         идентификатор созданного таймера
			 */
			u_short setTimeout(void * ctx, const time_t delay, function <void (const u_short, Core *, void *)> callback) noexcept;
			/**
			 * setInterval Метод установки интервала времени
			 * @param ctx      передаваемый контекст
			 * @param delay    задержка времени в миллисекундах
			 * @param callback функция обратного вызова
			 * @return         идентификатор созданного таймера
			 */
			u_short setInterval(void * ctx, const time_t delay, function <void (const u_short, Core *, void *)> callback) noexcept;
		public:
			/**
			 * setEasy Разрешаем использовать простое чтение базы событий
			 */
			void setEasy() noexcept;
			/**
			 * setDefer Метод установки флага отложенных вызовов событий сокета
			 * @param mode флаг отложенных вызовов событий сокета
			 */
			void setDefer(const bool mode) noexcept;
			/**
			 * setNoInfo Метод установки флага запрета вывода информационных сообщений
			 * @param mode флаг запрета вывода информационных сообщений
			 */
			void setNoInfo(const bool mode) noexcept;
			/**
			 * setPersist Метод установки персистентного флага
			 * @param mode флаг персистентного запуска каллбека
			 */
			void setPersist(const bool mode) noexcept;
			/**
			 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void setVerifySSL(const bool mode) noexcept;
			/**
			 * setPersistInterval Метод установки персистентного таймера
			 * @param itv интервал персистентного таймера в миллисекундах
			 */
			void setPersistInterval(const time_t itv) noexcept;
			/**
			 * setFamily Метод установки тип протокола интернета
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void setFamily(const int family = AF_INET) noexcept;
			/**
			 * setCA Метод установки CA-файла корневого SSL сертификата
			 * @param cafile адрес CA-файла
			 * @param capath адрес каталога где находится CA-файл
			 */
			void setCA(const string & cafile, const string & capath = "") noexcept;
			/**
			 * setNet Метод установки параметров сети
			 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
			 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void setNet(const vector <string> & ip = {}, const vector <string> & ns = {}, const int family = AF_INET) noexcept;
		public:
			/**
			 * Core Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Core(const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), uri(fmk, &nwk), ssl(fmk, log, &uri), dns4(fmk, log, &nwk), dns6(fmk, log, &nwk), fmk(fmk), log(log) {}
			/**
			 * ~Core Деструктор
			 */
			virtual ~Core() noexcept;
	} core_t;
};

#endif // __AWH_CORE__
