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
#include <chrono>
#include <thread>
#include <string>
#include <functional>
#include <unordered_map>
#include <errno.h>
#include <libev/ev++.h>

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
#include <net/ssl.hpp>
// #include <net/dns.hpp>
#include <net/socket.hpp>
#include <worker/core.hpp>
#include <sys/fmk.hpp>
#include <sys/threadpool.hpp>

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
		private:
			// Worker Устанавливаем дружбу с классом сетевого ядра
			friend class Worker;
		protected:
			/**
			 * Статус работы сетевого ядра
			 */
			enum class status_t : uint8_t {STOP, START};
			/**
			 * Тип запускаемого ядра
			 */
			enum class type_t : uint8_t {CLIENT, SERVER};
		public:
			/**
			 * Основные методы режимов работы
			 */
			enum class method_t : uint8_t {READ, WRITE};
		private:
			/**
			 * Timer Класс таймера
			 */
			typedef class Timer {
				public:
					// Идентификатор таймера
					u_short id;
				public:
					// Задержка времени в секундах
					float delay;
				public:
					// Флаг персистентной работы
					bool persist;
				public:
					// Объект события таймера
					ev::timer timer;
				public:
					// Передаваемый контекст
					void * ctx;
				public:
					// Родительский объект
					Core * core;
				public:
					// Внешняя функция обратного вызова
					function <void (const u_short, Core *, void *)> fn;
				public:
					/**
					 * callback Функция обратного вызова
					 * @param timer   объект события таймера
					 * @param revents идентификатор события
					 */
					void callback(ev::timer & timer, int revents) noexcept;
				public:
					/**
					 * Timer Конструктор
					 */
					Timer() noexcept : id(0), delay(0.f), persist(false), ctx(nullptr), core(nullptr), fn(nullptr) {}
			} timer_t;
		protected:
			/**
			 * Mutex Объект основных мютексов
			 */
			typedef struct Mutex {
				recursive_mutex core;   // Для работы с ядрами
				recursive_mutex bind;   // Для работы с биндингом ядра
				recursive_mutex stop;   // Для контроля остановки модуля
				recursive_mutex main;   // Для работы с параметрами модуля
				recursive_mutex start;  // Для контроля запуска модуля
				recursive_mutex timer;  // Для работы с таймерами
				recursive_mutex worker; // Для работы с воркерами
			} mtx_t;
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
			 * Sockaddr Структура адресного пространства сокета
			 */
			typedef struct Sockaddr {
				int fd;                      // Файловый дескриптор
				struct sockaddr_in client;   // Параметры подключения клиента IPv4
				struct sockaddr_in server;   // Параметры подключения сервера IPv4
				struct sockaddr_in6 client6; // Параметры подключения клиента IPv6
				struct sockaddr_in6 server6; // Параметры подключения сервера IPv6
				/**
				 * Sockaddr Конструктор
				 */
				Sockaddr() : fd(-1), client({}), server({}), client6({}), server6({}) {}
			} sockaddr_t;
		private:
			/**
			 * Dispatch Класс работы с событиями
			 */
			typedef class Dispatch {
				private:
					// Core Устанавливаем дружбу с классом ядра
					friend class Core;
				private:
					// Флаг разрешения работы
					bool mode;
					// Флаг работы модуля
					bool work;
				private:
					// База событий
					ev::loop_ref base;
				private:
					// Мютекс для блокировки потока
					recursive_mutex mtx;
				public:
					/**
					 * kick Метод отправки пинка
					 */
					void kick() noexcept;
					/**
					 * stop Метод остановки чтения базы событий
					 */
					void stop() noexcept;
					/**
					 * start Метод запуска чтения базы событий
					 */
					void start() noexcept;					
				public:
					/**
					 * freeze Метод заморозки чтения данных
					 * @param mode флаг активации
					 */
					void freeze(const bool mode) noexcept;
				public:
					/**
					 * setBase Метод установки базы событий
					 * @param base база событий
					 */
					void setBase(struct ev_loop * base) noexcept;
				public:
					/**
					 * Dispatch Конструктор
					 */
					Dispatch() noexcept : mode(false), work(false), base(nullptr) {}
					/**
					 * Dispatch Конструктор
					 * @param base база событий
					 */
					Dispatch(struct ev_loop * base) noexcept : mode(false), work(false), base(base) {}
			} dispatch_t;
		protected:
			// Мютекс для блокировки основного потока
			mtx_t mtx;
			// Сетевые параметры
			net_t net;
			// Создаём объект работы с URI
			uri_t uri;
			// Создаём объект для работы с SSL
			ssl_t ssl;
			/*
			// Создаём объект DNS IPv4 резолвера
			dns_t dns4;
			// Создаём объект DNS IPv6 резолвера
			dns_t dns6;
			*/
			// Параметры постоянного подключения
			alive_t alive;
			// Создаем объект сети
			network_t nwk;
			// Создаем пул потоков
			poolthr_t pool;
			// Объект для работы с сокетами
			socket_t socket;
			// Объект для работы с чтением базы событий
			dispatch_t dispatch;
		private:
			// Объект события таймера
			ev::periodic timer;
		protected:
			// Тип запускаемого ядра
			type_t type = type_t::CLIENT;
			// Статус сетевого ядра
			status_t status = status_t::STOP;
		private:
			// Список активных таймеров
			map <u_short, unique_ptr <timer_t>> timers;
		protected:
			// Список активных воркеров
			map <size_t, const worker_t *> workers;
			// Список сторонних сетевых ядер
			// map <Core *, struct event_base **> cores;
			// Список подключённых клиентов
			map <size_t, const worker_t::adj_t *> adjutants;
		protected:
			// Флаг разрешения работы
			bool mode = false;
			// Флаг использования многопоточного режима
			bool multi = false;
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
			struct ev_loop * base = nullptr;
		protected:
			// Функция обратного вызова при запуске/остановке модуля
			function <void (const bool, Core * core, void *)> callbackFn = nullptr;
		private:
			/**
			 * launching Метод вызова при активации базы событий
			 */
			void launching() noexcept;
		private:
			/**
			 * persistent Функция персистентного вызова по таймеру
			 * @param timer   объект события таймера
			 * @param revents идентификатор события
			 */
			void persistent(ev::periodic & timer, int revents) noexcept;
		protected:
			/**
			 * clean Метод буфера событий
			 * @param bev буфер событий для очистки
			 */
			void clean(worker_t::bev_t & bev) noexcept;
		protected:
			/**
			 * sockaddr Метод создания адресного пространства сокета
			 * @param ip     адрес для которого нужно создать сокет
			 * @param port   порт сервера для которого нужно создать сокет
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       параметры подключения к серверу
			 */
			const sockaddr_t sockaddr(const string & ip, const u_int port, const int family = AF_INET) const noexcept;
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
			 * close Метод отключения всех воркеров
			 */
			virtual void close() noexcept;
			/**
			 * remove Метод удаления всех воркеров
			 */
			virtual void remove() noexcept;
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
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
		protected:
			/**
			 * error Метод вывода описание ошибок
			 * @param bytes количество записанных/прочитанных байт в сокет
			 * @param aid   идентификатор адъютанта
			 */
			void error(const int64_t bytes, const size_t aid) const noexcept;
		public:
			/**
			 * write Метод записи буфера данных воркером
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param aid    идентификатор адъютанта
			 */
			void write(const char * buffer, const size_t size, const size_t aid) noexcept;
		public:
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
		public:
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
			 * freeze Метод заморозки чтения данных
			 * @param mode флаг активации заморозки чтения данных
			 */
			void freeze(const bool mode) noexcept;
		public:
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
			 * setMultiThreads Метод активации режима мультипотоковой обработки данных
			 * @param mode флаг мультипотоковой обработки
			 */
			void setMultiThreads(const bool mode) noexcept;
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
			Core(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Core Деструктор
			 */
			virtual ~Core() noexcept;
	} core_t;
};

#endif // __AWH_CORE__
