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
#include <unordered_map>
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
#include <dns.hpp>
#include <hash.hpp>
#include <timer.hpp>
#include <socket.hpp>
#include <ws/frame.hpp>
#include <http/client.hpp>

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
			// Минимальный размер сегмента
			static constexpr size_t MIN_FRAME_SIZE = 0xFA000;
			// Максимальный размер сегмента
			static constexpr size_t MAX_FRAME_SIZE = 0x9C4000;
		private:
			// Сетевые параметры
			net_t net;
			// Параметры постоянного подключения
			alive_t alive;
			// Таймер для пинга сервера
			timer_t timerPing;
			// Таймер для контроля подключения
			timer_t timerConnect;
		private:
			// Параметры адреса для запроса
			uri_t::url_t url;
			// Контекст SSL
			ssl_t::ctx_t sslctx;
		private:
			// Сокет сервера для подключения
			evutil_socket_t fd = -1;
			// Полученный опкод сообщения
			frame_t::opcode_t opcode = frame_t::opcode_t::TEXT;
		private:
			// Поддерживаемые сабпротоколы
			vector <string> subs;
			// Данные фрагметрированного сообщения
			vector <char> fragmes;
		private:
			// Флаг остановки работы
			bool halt = true;
			// Флаги работы с сжатыми данными
			bool gzip = false;
			// Флаг разрешения работы
			bool mode = false;
			// Флаг фриза работы клиента
			bool freeze = false;
			// Флаг автоматического переподключения
			bool reconnect = false;
			// Флаг полученных данных в сжатом виде
			bool compressed = false;
			// Минимальный размер сегмента
			size_t min = MIN_FRAME_SIZE;
			// Максимальный размер сегмента
			size_t max = MAX_FRAME_SIZE;
			// Флаг инициализации WinSock
			mutable bool winSock = false;
		private:
			// Создаём объект DNS резолвера
			dns_t * dns = nullptr;
			// Создаём объект для работы с SSL
			ssl_t * ssl = nullptr;
			// Создаём объект для работы с HTTP
			http_t * http = nullptr;
			// Создаём объект для компрессии-декомпрессии данных
			hash_t * hash = nullptr;
			// Создаём объект для работы с фреймом WebSocket
			frame_t * frame = nullptr;
			// Буфер событий для сервера
			struct bufferevent * bev = nullptr;
			// База данных событий
			struct event_base * base = nullptr;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
		private:
			// Создаём объект данных вебсокета
			const char * wdt = nullptr;
			// Адрес файла для сохранения логов
			const char * logfile = nullptr;
		private:
			// Функция обратного вызова, при запуске или остановки подключения к серверу
			function <void (bool, Client *)> openStopFn = nullptr;
			// Функция обратного вызова, при получении ответа от сервера
			function <void (const string &, Client *)> pongFn = nullptr;
			// Функция обратного вызова, при получении ошибки работы клиента
			function <void (u_short, const string &, Client *)> errorFn = nullptr;
			// Функция обратного вызова, при получении сообщения с сервера
			function <void (const vector <char> &, const bool, Client *)> messageFn = nullptr;
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
			 * request Метод выполнения HTTP запроса
			 */
			void request() noexcept;
			/**
			 * delay Метод фриза потока на указанное количество секунд
			 * @param seconds количество секунд для фриза потока
			 */
			void delay(const size_t seconds) const noexcept;
			/**
			 * error Метод вывода сообщений об ошибках работы клиента
			 * @param message сообщение с описанием ошибки
			 */
			void error(const mess_t & message) const noexcept;
			/**
			 * extraction Метод извлечения полученных данных
			 * @param buffer данные в чистом виде полученные с сервера
			 * @param utf8   данные передаётся в текстовом виде
			 */
			void extraction(const vector <char> & buffer, const bool utf8) const noexcept;
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
			 * ping Метод проверки доступности сервера
			 * @param message сообщение для отправки
			 */
			void ping(const string & message = "") noexcept;
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
			/**
			 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
			 * @param callback функция обратного вызова
			 */
			void on(function <void (bool, Client *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения PONG
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const string &, Client *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения ошибок
			 * @param callback функция обратного вызова
			 */
			void on(function <void (u_short, const string &, Client *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения сообщений
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const vector <char> &, const bool, Client *)> callback) noexcept;
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
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::algorithm_t algorithm = auth_t::algorithm_t::MD5) noexcept;
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
