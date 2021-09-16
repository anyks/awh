/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_CLIENT__
#define __AWH_CLIENT__

/**
 * Стандартная библиотека
 */
#include <set>
#include <ctime>
#include <string>
#include <functional>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

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
#include <ssl.hpp>
#include <uri.hpp>
#include <dns.hpp>
#include <auth.hpp>
#include <hash.hpp>
#include <timer.hpp>
#include <socket.hpp>
#include <ws/frame.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Client Класс для работы с клиентом WebSocket
	 */
	typedef class Client {
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
			// Версия протокола WebSocket
			static constexpr u_short WS_VERSION = 13;
			// Минимальный размер сегмента
			static constexpr size_t MIN_FRAME_SIZE = 0xFA000;
			// Максимальный размер сегмента
			static constexpr size_t MAX_FRAME_SIZE = 0x9C4000;
		private:
			// Сетевые параметры
			net_t net;
			// Таймер для контроля подключения
			timer_t timer;
			// Параметры постоянного подключения
			alive_t alive;
			// Подпротоколы поддерживаемые сервером
			set <string> subs;
		private:
			// Параметры адреса для запроса
			uri_t::url_t url;
			// Контекст SSL
			ssl_t::ctx_t sslctx;
		private:
			// Флаг остановки работы
			bool stop = true;
			// Флаги работы с сжатыми данными
			bool gzip = false;
			// Флаг разрешения работы
			bool mode = false;
			// Флаг автоматического переподключения
			bool reconnect = false;
			// Минимальный размер сегмента
			size_t min = MIN_FRAME_SIZE;
			// Максимальный размер сегмента
			size_t max = MAX_FRAME_SIZE;
			// Флаг инициализации WinSock
			mutable bool winSock = false;
		private:
			// Сокет сервера для подключения
			evutil_socket_t fd = -1;
			// User-Agent для HTTP запроса
			string userAgent = USER_AGENT;
		private:
			// Создаём объект DNS резолвера
			dns_t * dns = nullptr;
			// Создаём объект для работы с SSL
			ssl_t * ssl = nullptr;
			// Создаём объект для работы с авторизацией
			auth_t * auth = nullptr;
			// Буфер событий для сервера
			struct bufferevent * bev = nullptr;
			// База данных событий
			struct event_base * base = nullptr;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
			// Адрес файла для сохранения логов
			const char * logfile = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
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
			 * key Метод генерации ключа для WebSocket
			 * @return сгенерированный ключ для WebSocket
			 */
			const string key() const noexcept;
			/**
			 * date Метод получения текущей даты для HTTP запроса
			 * @return текущая дата
			 */
			const string date() const noexcept;
		private:
			/**
			 * request Метод выполнения HTTP запроса
			 */
			void request() noexcept;
			/**
			 * delay Метод фриза потока на указанное количество секунд
			 * @param seconds количество секунд для фриза потока
			 */
			void delay(const size_t seconds) const noexcept;
		private:
			/**
			 * connect Метод создания сокета для подключения к удаленному серверу
			 * @return результат подключения
			 */
			const bool connect() noexcept;
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
			 * read Метод чтения данных с сокета сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void read(struct bufferevent * bev, void * ctx);
			/**
			 * write Метод записи данных в сокет сервера
			 * @param bev буфер события
			 * @param ctx передаваемый объект
			 */
			// static void write(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * event Метод обработка входящих событий с сервера
			 * @param bev    буфер события
			 * @param events произошедшее событие
			 * @param ctx    передаваемый контекст
			 */
			static void event(struct bufferevent * bev, const short events, void * ctx) noexcept;
		private:
			/**
			 * close Метод закрытия соединения сервера
			 */
			void close() noexcept;
			/**
			 * resolve Метод выполняющая резолвинг хоста http запроса
			 * @param url      параметры хоста, для которого нужно получить IP адрес
			 * @param callback функция обратного вызова
			 */
			void resolve(const uri_t::url_t & url, function <void (const string &)> callback) noexcept;
		public:
			/**
			 * init Метод инициализации WebSocket клиента
			 * @param url  адрес WebSocket сервера
			 * @param gzip флаг активации сжатия данных
			 */
			void init(const string & url, const bool gzip = false);
		public:

			void onOpen() noexcept;
			void onError() noexcept;
			void onClose() noexcept;
			void onMessage() noexcept;
		public:
			void send() noexcept;
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
			 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void setVerifySSL(const bool mode) noexcept;
			/**
			 * setAutoReconnect Метод установки флага автоматического переподключения
			 * @param mode флаг автоматического переподключения
			 */
			void setAutoReconnect(const bool mode) noexcept;
			/**
			 * setFamily Метод установки тип протокола интернета
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void setFamily(const int family = AF_INET) noexcept;
			/**
			 * setUserAgent Метод установки User-Agent для HTTP запроса
			 * @param userAgent агент пользователя для HTTP запроса
			 */
			void setUserAgent(const string & userAgent) noexcept;
			/**
			 * setFrameSize Метод установки размеров сегментов фрейма
			 * @param min минимальный размер сегмента
			 * @param max максимальный размер сегмента
			 */
			void setFrameSize(const size_t min, const size_t max) noexcept;
			/**
			 * setUser Метод установки параметров авторизации
			 * @param login    логин пользователя для авторизации на сервере
			 * @param password пароль пользователя для авторизации на сервере
			 */
			void setUser(const string & login, const string & password) noexcept;
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
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type      тип авторизации
			 * @param algorithm алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::algorithm_t algorithm = algorithm_t::MD5) noexcept;
		public:
			/**
			 * Client Конструктор
			 * @param fmk     объект фреймворка
			 * @param uri     объект работы с URI
			 * @param nwk     объект методов для работы с сетью
			 * @param logfile адрес файла для сохранения логов
			 */
			Client(const fmk_t * fmk, const uri_t * uri, const network_t * nwk, const char * logfile = nullptr) noexcept;
			/**
			 * ~Client Деструктор
			 */
			~Client() noexcept;
	} client_t;
};

#endif // __AWH_CLIENT__
