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
#include <string>
#include <functional>
#include <unordered_map>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

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
#include <hash.hpp>
#include <socket.hpp>
#include <client/worker.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Класс ядра биндинга TCP/IP
	 */
	typedef class Core {
		private:
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
			} alive_t;
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
		private:
			// Сетевые параметры
			net_t net;
			// Параметры постоянного подключения
			alive_t alive;
			// Список активных воркеров
			map <size_t, const worker_t *> workers;
		private:
			// Флаг разрешения работы
			bool mode = false;
			// Флаг инициализации WinSock
			mutable bool winSock = false;
		private:
			// Создаём объект для работы с SSL
			ssl_t * ssl = nullptr;
			// Создаём объект DNS IPv4 резолвера
			dns_t * dns4 = nullptr;
			// Создаём объект DNS IPv6 резолвера
			dns_t * dns6 = nullptr;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
		private:
			// База данных событий
			struct event_base * base = nullptr;
		private:
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
			 * delay Метод фриза потока на указанное количество секунд
			 * @param seconds количество секунд для фриза потока
			 */
			void delay(const size_t seconds) const noexcept;
		private:
			/**
			 * read Метод чтения данных с сокета сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void read(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * write Метод записи данных в сокет сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void write(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * tuning Метод тюннинга буфера событий
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void tuning(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * event Метод обработка входящих событий с сервера
			 * @param bev    буфер события
			 * @param events произошедшее событие
			 * @param ctx    передаваемый контекст
			 */
			static void event(struct bufferevent * bev, const short events, void * ctx) noexcept;
		private:
			/**
			 * connect Метод создания подключения к удаленному серверу
			 * @param worker воркер для подключения
			 * @return       результат подключения
			 */
			const bool connect(const worker_t * worker) noexcept;
			/**
			 * socket Метод создания сокета
			 * @param ip     адрес для которого нужно создать сокет
			 * @param port   порт сервера для которого нужно создать сокет
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       параметры подключения к серверу
			 */
			const socket_t socket(const string & ip, const u_int port, const int family = AF_INET) const noexcept;
		private:
			/**
			 * close Метод закрытия подключения воркера
			 * @param worker воркер для закрытия подключения
			 */
			void close(const worker_t * worker) noexcept;
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
			 * isStart Метод проверки на запуск бинда TCP/IP
			 * @return результат проверки
			 */
			bool isStart() const noexcept;
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
			void closeAll() noexcept;
			/**
			 * removeAll Метод удаления всех воркеров
			 */
			void removeAll() noexcept;
		public:
			/**
			 * open Метод открытия подключения воркером
			 * @param wid идентификатор воркера
			 */
			void open(const size_t wid) noexcept;
			/**
			 * close Метод закрытия подключения воркером
			 * @param wid идентификатор воркера
			 */
			void close(const size_t wid) noexcept;
			/**
			 * remove Метод удаления воркера из биндинга
			 * @param wid идентификатор воркера
			 */
			void remove(const size_t wid) noexcept;
			/**
			 * switchProxy Метод переключения с прокси-сервера
			 * @param wid идентификатор воркера
			 */
			void switchProxy(const size_t wid) noexcept;
			/**
			 * write Метод записи буфера данных воркером
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param wid    идентификатор воркера
			 */
			void write(const char * buffer, const size_t size, const size_t wid) noexcept;
		public:
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
			Core(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Core Деструктор
			 */
			~Core() noexcept;
	} core_t;
};

#endif // __AWH_CORE__
