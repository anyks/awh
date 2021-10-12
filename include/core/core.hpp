/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
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
#include <core/worker.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * CoreClient Прототип клиентского класса ядра
	 */
	class CoreClient;
	/**
	 * Core Класс ядра биндинга TCP/IP
	 */
	typedef class Core {
		public:
			/**
			 * Основные методы режимов работы
			 */
			enum class method_t : uint8_t {READ, WRITE};
		private:
			/**
			 * CoreClient Устанавливаем дружбу с клиентским классом ядра
			 */
			friend class CoreClient;
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
			// Событие таймаута запуска системы
			struct event timeout;
			// Устанавливаем таймаут ожидания активации базы событий
			struct timeval basetv;
		protected:
			// Список активных воркеров
			map <size_t, const worker_t *> workers;
			// Список подключённых клиентов
			map <size_t, const worker_t::adj_t *> adjutants;
		protected:
			// Флаг разрешения работы
			bool mode = false;
			// Флаг блокировку инициализации базы событий
			bool locker = false;
			// Флаг запрета вывода информационных сообщений
			bool noinfo = false;
			// Флаг инициализации WinSock
			mutable bool winSock = false;
		protected:
			// Промежуточный контекст
			void * ctx = nullptr;
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
		protected:
			/**
			 * Если - это Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				/**
				 * winSocketInit Метод инициализации WinSock
				 */
				void winSocketInit() const noexcept;
				/**
				 * winSocketClean Метод очистки WinSock
				 */
				void winSocketClean() const noexcept;
			#endif
		private:
			/**
			 * run Функция обратного вызова при активации базы событий
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 * @param ctx   передаваемый контекст
			 */
			static void run(evutil_socket_t fd, short event, void * ctx) noexcept;
		protected:
			/**
			 * delay Метод фриза потока на указанное количество секунд
			 * @param seconds количество секунд для фриза потока
			 */
			void delay(const size_t seconds) const noexcept;
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
			 * remove Метод удаления воркера из биндинга
			 * @param wid идентификатор воркера
			 */
			virtual void remove(const size_t wid) noexcept;
			/**
			 * open Метод открытия подключения воркером
			 * @param wid идентификатор воркера
			 */
			virtual void open(const size_t wid) noexcept = 0;
		public:
			/**
			 * close Метод закрытия подключения воркера
			 * @param aid идентификатор адъютанта
			 */
			virtual void close(const size_t aid) noexcept;
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
			 * setTimeout Метод установки таймаута ожидания появления данных
			 * @param method  метод режима работы
			 * @param seconds время ожидания в секундах
			 * @param aid     идентификатор адъютанта
			 */
			void setTimeout(const method_t method, const time_t seconds, const size_t aid) noexcept;
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
			 * setNoInfo Метод установки флага запрета вывода информационных сообщений
			 * @param mode флаг запрета вывода информационных сообщений
			 */
			void setNoInfo(const bool mode) noexcept;
			/**
			 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void setVerifySSL(const bool mode) noexcept;
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
