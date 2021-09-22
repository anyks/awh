/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_REST_CLIENT__
#define __AWH_REST_CLIENT__

/**
 * Стандартная библиотека
 */
#include <set>
#include <thread>
#include <string>
#include <functional>
#include <unordered_map>
#include <event2/http.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/keyvalq_struct.h>
#include <event2/bufferevent_struct.h>
#include <nlohmann/json.hpp>

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
#include <log.hpp>
#include <ssl.hpp>
#include <uri.hpp>
#include <dns.hpp>
#include <auth.hpp>
#include <hash.hpp>
#include <socket.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Rest Класс для работы с клиентом REST
	 */
	typedef class Rest {
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
		public:
			/**
			 * Response Структура ответа сервера
			 */
			typedef struct Response {
				bool ok;                                // Флаг удачного ответа
				u_short code;                           // Код ответа сервера
				string mess;                            // Сообщение ответа сервера
				string body;                            // Тело ответа сервера
				unordered_map <string, string> headers; // Заголовки сервера
				/**
				 * Response Конструктор
				 */
				Response() : ok(false), code(500), mess(""), body(""), headers({}) {}
			} res_t;
		private:
			/**
			 * Request Структура запроса к серверу
			 */
			typedef struct Request {
				evhttp_cmd_type method;                         // Метод выполняемого запроса
				const string * body;                            // Тело запроса (если требуется)
				const uri_t::url_t * uri;                       // Параметры запроса
				const unordered_map <string, string> * headers; // Список заголовков запроса
			public:
				/**
				 * operator= Оператор установки параметров запроса
				 * @param uri объект параметра запроса
				 * @return    ссылка на контекст объекта
				 */
				Request & operator=(const uri_t::url_t * uri) noexcept {
					// Устанавливаем текст сообщения
					if(uri != nullptr){
						// Устанавливаем параметры запроса
						this->uri = uri;
						// Сбрасываем данные тела запроса
						this->body = nullptr;
						// Сбрасываем данные заголовков запроса
						this->headers = nullptr;
						// Сбрасываем метод выполняемого запроса
						this->method = EVHTTP_REQ_GET;
					}
					// Выводим контекст текущего объекта
					return (* this);
				}
			public:
				/**
				 * Request Конструктор
				 */
				Request() : method(EVHTTP_REQ_GET), body(nullptr), uri(nullptr), headers(nullptr) {}
			} req_t;
			/**
			 * EvBuffer Структура работы с BufferEvent
			 */
			typedef struct EvBuffer {
				struct evdns_base * dns;          // База событий DNS
				struct bufferevent * bev;         // Буфер событий
				struct event_base * base;         // База событий
				struct evhttp_connection * evcon; // Событие подключения
				/**
				 * EvBuffer Конструктор
				 */
				EvBuffer() : dns(nullptr), bev(nullptr), base(nullptr), evcon(nullptr) {}
			} evbuf_t;
		public:
			/**
			 * Формат сжатия тела запроса
			 */
			enum class zip_t : u_short {NONE, GZIP, DEFLATE};
			/**
			 * Тип прокси-сервера
			 */
			enum class proxy_t : u_short {NONE, HTTP, SOCKS};
		private:
			/**
			 * Типы основных заголовков
			 */
			enum class header_t : u_short {
				HOST,          // Host
				ACCEPT,        // Accept
				ORIGIN,        // Origin
				USERAGENT,     // User-Agent
				CONNECTION,    // Connection
				ACCEPTLANGUAGE // Accept-Language
			};
		private:
			// Параметры запроса
			req_t req;
			// Параметры ответа
			res_t res;
			// Сетевые параметры
			net_t net;
			// Параметры BufferEvent
			evbuf_t evbuf;
			// Параметры постоянного подключения
			alive_t alive;
		private:
			// Буфер событий SSL
			ssl_t::ctx_t sslctx;
			// Данные прокси-сервера
			uri_t::url_t proxyUrl;
			// Тип выбранного прокси-сервера
			proxy_t proxyType = proxy_t::NONE;
		private:
			// Сокет сервера для подключения
			evutil_socket_t fd = -1;
			// Флаги работы с сжатыми данными
			zip_t zip = zip_t::GZIP;
			// User-Agent для HTTP запроса
			string userAgent = USER_AGENT;
		private:

			bool flg = false;

			// Флаг шифрования сообщений
			bool crypt = false;
			// Флаг передачи тела запроса чанками
			bool chunked = true;
			// Флаг инициализации WinSock
			mutable bool winSock = false;
			// Флаг проведённой попытки выполнения авторизации
			mutable bool checkAuth = false;
		private:
			// Создаём объект DNS резолвера
			dns_t * dns = nullptr;
			// Создаём объект для работы с SSL
			ssl_t * ssl = nullptr;
			// Создаём объект для работы с авторизацией
			auth_t * auth = nullptr;
			// Создаём объект для компрессии-декомпрессии данных
			hash_t * hash = nullptr;
		private:
			// Создаём объект данных вебсокета
			const char * hdt = nullptr;
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
			 * requestProxy Метод выполнения HTTP запроса к прокси-серверу
			 */
			void requestProxy() noexcept;
			/**
			 * connectProxy Метод создания подключения к удаленному прокси-серверу
			 * @return результат подключения
			 */
			const bool connectProxy() noexcept;
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
			 * readProxy Метод чтения данных с сокета прокси-сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void readProxy(struct bufferevent * bev, void * ctx);
			/**
			 * eventProxy Метод обработка входящих событий с прокси-сервера
			 * @param bev    буфер события
			 * @param events произошедшее событие
			 * @param ctx    передаваемый контекст
			 */
			static void eventProxy(struct bufferevent * bev, const short events, void * ctx) noexcept;
		private:
			/**
			 * resolve Метод выполняющая резолвинг хоста сервера
			 * @param url      параметры хоста, для которого нужно получить IP адрес
			 * @param callback функция обратного вызова
			 */
			void resolve(const uri_t::url_t & url, function <void (const string &)> callback) noexcept;
		public:
			/**
			 * GET Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string GET(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * DEL Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string DEL(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) noexcept;
		public:
			/**
			 * PUT Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * PUT Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * PUT Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers = {}) noexcept;
		public:
			/**
			 * POST Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * POST Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * POST Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers = {}) noexcept;
		public:
			/**
			 * PATCH Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * PATCH Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * PATCH Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers = {}) noexcept;
		public:
			/**
			 * HEAD Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_map <string, string> HEAD(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * TRACE Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_map <string, string> TRACE(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) noexcept;
			/**
			 * OPTIONS Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_map <string, string> OPTIONS(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) noexcept;
		public:

			void clear() noexcept;

			/**
			 * callback Функция вывода результата получения данных
			 * @param req объект REST запроса
			 * @param ctx контекст родительского объекта
			 */
			static void callback(struct evhttp_request * req, void * ctx) noexcept;

			static void makeHeaders(struct evhttp_request * req, const uri_t::url_t & url, const unordered_map <string, string> & headers, void * ctx) noexcept;

			static void makeBody(struct evhttp_request * req, const string & body, void * ctx) noexcept;

			// static bool makeRequest(struct evhttp_connection * evcon, struct evhttp_request * req, const uri_t::url_t & url, evhttp_cmd_type type, const unordered_map <string, string> & headers, const string & body, void * ctx) noexcept;

			/**
			 * proxy Функция вывода результата получения данных
			 * @param req объект REST запроса
			 * @param ctx контекст родительского объекта
			 */
			static void proxyFn(struct evhttp_request * req, void * ctx);



			void PROXY2() noexcept;


			/**
			 * PROXY Метод выполнения REST запроса на сервер через прокси-сервер
			 */
			void PROXY() noexcept;


			/**
			 * REST Метод выполнения REST запроса на сервер
			 */
			void REST() noexcept;
		public:
			/**
			 * setZip Метод активации работы с сжатым контентом
			 * @param method метод установки формата сжатия
			 */
			void setZip(const zip_t method) noexcept;
			/**
			 * setChunked Метод активации режима передачи тела запроса чанками
			 * @param mode флаг активации режима передачи тела запроса чанками
			 */
			void setChunked(const bool mode) noexcept;
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
			 * setUserAgent Метод установки User-Agent для HTTP запроса
			 * @param userAgent агент пользователя для HTTP запроса
			 */
			void setUserAgent(const string & userAgent) noexcept;
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
			 * setProxy Метод установки прокси-сервера
			 * @param uri  параметры подключения к прокси-серверу
			 * @param type тип используемого прокси-сервера
			 */
			void setProxy(const string & uri, const proxy_t type = proxy_t::HTTP) noexcept;
			/**
			 * setNet Метод установки параметров сети
			 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
			 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 */
			void setNet(const vector <string> & ip = {}, const vector <string> & ns = {}, const int family = AF_INET) noexcept;
			/**
			 * setCrypt Метод установки параметров шифрования
			 * @param pass пароль шифрования передаваемых данных
			 * @param salt соль шифрования передаваемых данных
			 * @param aes  размер шифрования передаваемых данных
			 */
			void setCrypt(const string & pass, const string & salt = "", const hash_t::aes_t aes = hash_t::aes_t::AES128) noexcept;
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type      тип авторизации
			 * @param algorithm алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::algorithm_t algorithm = auth_t::algorithm_t::MD5) noexcept;
		public:
			/**
			 * Rest Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 * @param nwk объект методов для работы с сетью
			 */
			Rest(const fmk_t * fmk, const log_t * log, const uri_t * uri, const network_t * nwk) noexcept;
			/**
			 * ~Rest Деструктор
			 */
			~Rest() noexcept;
	} rest_t;
};

#endif // __AWH_REST_CLIENT__
