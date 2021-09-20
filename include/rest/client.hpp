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
		public:
			/**
			 * Response Структура ответа сервера
			 */
			typedef struct Response {
				bool ok;                                // Флаг удачного ответа
				u_short code;                           // Код ответа сервера
				string mess;                            // Сообщение ответа сервера
				string body;                            // Тело ответа сервера
				const Rest * ctx;                       // Контекст родительского объекта
				struct bufferevent * bev;               // Буфер событий
				unordered_map <string, string> headers; // Заголовки сервера
				/**
				 * Response Конструктор
				 */
				Response() : ok(false), code(500), mess(""), body(""), ctx(nullptr), bev(nullptr), headers({}) {}
			} res_t;
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
			// Сетевые параметры
			net_t net;
			// Параметры постоянного подключения
			alive_t alive;
		private:
			// User-Agent для HTTP запроса
			string userAgent = USER_AGENT;
		private:
			// Флаги работы с сжатыми данными
			bool gzip = true;
			// Флаг шифрования сообщений
			bool crypt = false;
			// Флаг передачи тела запроса чанками
			bool chunked = true;
			// Флаг инициализации WinSock
			mutable bool winSock = false;
		private:
			// Создаём объект для работы с SSL
			ssl_t * ssl = nullptr;
			// Создаём объект для работы с авторизацией
			auth_t * auth = nullptr;
			// Создаём объект для компрессии-декомпрессии данных
			hash_t * hash = nullptr;
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
		public:
			/**
			 * GET Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string GET(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * DEL Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string DEL(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) const noexcept;
		public:
			/**
			 * PUT Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * PUT Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * PUT Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers = {}) const noexcept;
		public:
			/**
			 * POST Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * POST Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * POST Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers = {}) const noexcept;
		public:
			/**
			 * PATCH Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * PATCH Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * PATCH Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers = {}) const noexcept;
		public:
			/**
			 * HEAD Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_map <string, string> HEAD(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * TRACE Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_map <string, string> TRACE(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) const noexcept;
			/**
			 * OPTIONS Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_map <string, string> OPTIONS(const uri_t::url_t & url, const unordered_map <string, string> & headers = {}) const noexcept;
		public:
			/**
			 * REST Метод выполнения REST запроса на сервер
			 * @param url параметры адреса запроса
			 * @param type тип REST запроса
			 * @param headers список заголовков для REST запроса
			 * @param body    телоо REST запроса
			 * @return        результат REST запроса
			 */
			const res_t REST(const uri_t::url_t & url, evhttp_cmd_type type = EVHTTP_REQ_GET, const unordered_map <string, string> & headers = {}, const string & body = {}) const noexcept;
		public:
			/**
			 * setGzip Метод активации работы с сжатым контентом
			 * @param mode флаг активации сжатого контента
			 */
			void setGzip(const bool mode) noexcept;
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
