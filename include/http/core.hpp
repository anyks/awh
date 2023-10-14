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

#ifndef __AWH_HTTP__
#define __AWH_HTTP__

/**
 * Стандартная библиотека
 */
#include <ctime>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <http/web.hpp>
#include <hash/hash.hpp>
#include <auth/client.hpp>
#include <auth/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Http Класс для работы с REST
	 */
	typedef class Http {
		public:
			/**
			 * Статусы проверки авторизации
			 */
			enum class stath_t : uint8_t {
				NONE  = 0x00, // Проверка авторизации не проводилась
				GOOD  = 0x01, // Авторизация прошла успешно
				RETRY = 0x02, // Требуется повторить попытку
				FAULT = 0x03  // Авторизация не удалась
			};
			/**
			 * Флаг выполняемого процесса
			 */
			enum class process_t : uint8_t {
				NONE     = 0x00, // Операция не установлена
				REQUEST  = 0x01, // Операция запроса
				RESPONSE = 0x02  // Операция ответа
			};
			/**
			 * Идентичность протокола
			 */
			enum class identity_t : uint8_t {
				NONE  = 0x00, // Протокол не установлен
				WS    = 0x01, // Протокол WebSocket
				HTTP  = 0x02, // Протокол HTTP
				PROXY = 0x03  // Протокол Proxy
			};
			/**
			 * Формат сжатия тела запроса
			 */
			enum class compress_t : uint8_t {
				NONE           = 0x00, // -
				GZIP           = 0x01, // gzip
				BROTLI         = 0x02, // br
				DEFLATE        = 0x03, // deflate
				GZIP_BROTLI    = 0x04, // gzip, br
				GZIP_DEFLATE   = 0x05, // gzip, deflate
				ALL_COMPRESS   = 0x06, // gzip, deflate, br
				DEFLATE_BROTLI = 0x07  // deflate, br
			};
		public:
			/**
			 * Ident Структура идентификации сервиса
			 */
			typedef struct Ident {
				// Идентификатор сервиса
				string id;
				// Название сервиса
				string name;
				// Версия модуля приложения
				string version;
				/**
				 * Ident Конструктор
				 */
				Ident() noexcept : id{AWH_SHORT_NAME}, name{AWH_NAME}, version{AWH_VERSION} {}
			} ident_t;
		protected:
			/**
			 * Auth Структура объекта авторизации
			 */
			typedef struct Auth {
				client::auth_t client; // Объект для работы с клиентской авторизацией
				server::auth_t server; // Объект для работы с серверной авторизацией
				/**
				 * Auth Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) : client(fmk, log), server(fmk, log) {}
			} auth_t;
		protected:
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				NONE      = 0x00, // Режим стейта не выставлен
				GOOD      = 0x01, // Режим удачного выполнения запроса
				BROKEN    = 0x02, // Режим бракованных данных
				HANDSHAKE = 0x03  // Режим выполненного рукопожатия
			};
		private:
			/**
			 * Список HTTP-сообщений
			 */
			map <u_short, string> messages = {
				{0, "Not Answer"},
				{100, "Continue"},
				{101, "Switching Protocols"},
				{102, "Processing"},
				{103, "Early Hints"},
				{200, "OK"},
				{201, "Created"},
				{202, "Accepted"},
				{203, "Non-Authoritative Information"},
				{204, "No Content"},
				{205, "Reset Content"},
				{206, "Partial Content"},
				{300, "Multiple Choice"},
				{301, "Moved Permanently"},
				{302, "Found"},
				{303, "See Other"},
				{304, "Not Modified"},
				{305, "Use Proxy"},
				{306, "Switch Proxy"},
				{307, "Temporary Redirect"},
				{308, "Permanent Redirect"},
				{400, "Bad Request"},
				{401, "Authentication Required"},
				{402, "Payment Required"},
				{403, "Forbidden"},
				{404, "Not Found"},
				{405, "Method Not Allowed"},
				{406, "Not Acceptable"},
				{407, "Proxy Authentication Required"},
				{408, "Request Timeout"},
				{409, "Conflict"},
				{410, "Gone"},
				{411, "Length Required"},
				{412, "Precondition Failed"},
				{413, "Request Entity Too Large"},
				{414, "Request-URI Too Long"},
				{415, "Unsupported Media Type"},
				{416, "Requested Range Not Satisfiable"},
				{417, "Expectation Failed"},
				{500, "Internal Server Error"},
				{501, "Not Implemented"},
				{502, "Bad Gateway"},
				{503, "Service Unavailable"},
				{504, "Gateway Timeout"},
				{505, "HTTP Version Not Supported"}
			};
		protected:
			// Создаём объект работы с URI
			uri_t _uri;
		protected:
			// Стейт проверки авторизации
			stath_t _stath;
			// Стейт текущего запроса
			state_t _state;
		protected:
			// Идентичность протокола
			identity_t _identity;
		private:
			// Объявляем функции обратного вызова
			fn_t _callback;
		protected:
			// Создаём объект HTTP парсера
			mutable web_t _web;
			// Создаём объект для работы с авторизацией
			mutable auth_t _auth;
		protected:
			// Создаём объект для работы с сжатыми данными
			mutable hash_t _hash;
			// Создаём объект для работы с временными сжатыми данными
			mutable hash_t _dhash;
		protected:
			// Флаг зашифрованных данных
			bool _crypt;
			// Флаг зашифрованной полезной нагрузки
			bool _crypted;
		private:
			// Флаг разрешающий передавать тело чанками
			mutable bool _chunking;
		private:
			// Размер одного чанка
			size_t _chunk;
		private:
			// Идентификация сервиса
			ident_t _ident;
		protected:
			// Флаг компрессии полезной нагрузки
			compress_t _inflated;
			// Метод компрессии полезной нагрузки
			compress_t _compress;
		private:
			// User-Agent для HTTP запроса
			mutable string _userAgent;
		protected:
			// Чёрный список заголовков
			mutable unordered_set <string> _black;
		protected:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * chunkingCallback Функция вывода полученных чанков полезной нагрузки
			 * @param id     идентификатор объекта
			 * @param buffer буфер данных чанка полезной нагрузки
			 * @param web    объект HTTP парсера
			 */
			void chunkingCallback(const uint64_t id, const vector <char> & buffer, const web_t * web) noexcept;
		private:
			/**
			 * encrypt Метод выполнения шифрования полезной нагрузки
			 */
			void encrypt() noexcept;
			/**
			 * decrypt Метод выполнения дешифровани полезной нагрузки
			 */
			void decrypt() noexcept;
		private:
			/**
			 * inflate Метод выполнения декомпрессии полезной нагрузки
			 */
			void inflate() noexcept;
			/**
			 * deflate Метод выполнения компрессии полезной нагрузки
			 */
			void deflate() noexcept;
		public:
			/**
			 * commit Метод применения полученных результатов
			 */
			virtual void commit() noexcept;
		protected:
			/**
			 * checkAuth Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			virtual stath_t checkAuth() noexcept = 0;
		public:
			/**
			 * clear Метод очистки собранных данных
			 */
			virtual void clear() noexcept;
			/**
			 * reset Метод сброса параметров запроса
			 */
			virtual void reset() noexcept;
		public:
			/**
			 * rmBlack Метод удаления заголовка из чёрного списка
			 * @param key ключ заголовка
			 */
			void rmBlack(const string & key) noexcept;
			/**
			 * addBlack Метод добавления заголовка в чёрный список
			 * @param key ключ заголовка
			 */
			void addBlack(const string & key) noexcept;
		public:
			/**
			 * parse Метод парсинга сырых данных
			 * @param buffer буфер данных для обработки
			 * @param size   размер буфера данных
			 * @return       размер обработанных данных
			 */
			size_t parse(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * payload Метод чтения чанка полезной нагрузки
			 * @return текущий чанк полезной нагрузки
			 */
			const vector <char> payload() const noexcept;
			/**
			 * payload Метод установки чанка полезной нагрузки
			 * @param payload буфер чанка полезной нагрузки
			 */
			void payload(const vector <char> & payload) noexcept;
		public:
			/**
			 * clearBody Метод очистки данных тела
			 */
			void clearBody() const noexcept;
			/**
			 * clearHeaders Метод очистки списка заголовков
			 */
			void clearHeaders() const noexcept;
		public:
			/**
			 * body Метод получения данных тела запроса
			 * @return буфер данных тела запроса
			 */
			const vector <char> & body() const noexcept;
			/**
			 * body Метод установки данных тела
			 * @param body буфер тела для установки
			 */
			void body(const vector <char> & body) noexcept;
		public:
			/**
			 * rmHeader Метод удаления заголовка
			 * @param key ключ заголовка
			 */
			void rmHeader(const string & key) noexcept;
			/**
			 * header Метод получения данных заголовка
			 * @param key ключ заголовка
			 * @return    значение заголовка
			 */
			const string header(const string & key) const noexcept;
			/**
			 * header Метод добавления заголовка
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void header(const string & key, const string & val) noexcept;
		public:
			/**
			 * headers Метод получения списка заголовков
			 * @return список существующих заголовков
			 */
			const unordered_multimap <string, string> & headers() const noexcept;
			/**
			 * headers Метод установки списка заголовков
			 * @param headers список заголовков для установки
			 */
			void headers(const unordered_multimap <string, string> & headers) noexcept;
		public:
			/**
			 * header2 Метод добавления заголовка в формате HTTP/2
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void header2(const string & key, const string & val) noexcept;
			/**
			 * headers2 Метод установки списка заголовков в формате HTTP/2
			 * @param headers список заголовков для установки
			 */
			void headers2(const vector <pair<string, string>> & headers) noexcept;
		public:
			/**
			 * getAuth Метод проверки статуса авторизации
			 * @return результат проверки
			 */
			stath_t getAuth() const noexcept;
			/**
			 * getAuth Метод извлечения строки авторизации
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     строка авторизации на удалённом сервере
			 */
			string getAuth(const process_t flag, const web_t::provider_t & prov) const noexcept;
		public:
			/**
			 * getUrl Метод извлечения параметров запроса
			 * @return установленные параметры запроса
			 */
			const uri_t::url_t & getUrl() const noexcept;
		public:
			/**
			 * compression Метод извлечения метода компрессии
			 * @return метод компрессии
			 */
			compress_t compression() const noexcept;
			/**
			 * compress Метод получения метода компрессии
			 * @return метод компрессии сообщений
			 */
			virtual compress_t compress() const noexcept;
			/**
			 * compress Метод установки метода компрессии
			 * @param compress метод компрессии сообщений
			 */
			virtual void compress(const compress_t compress) noexcept;
		public:
			/**
			 * dump Метод получения бинарного дампа
			 * @return бинарный дамп данных
			 */
			vector <char> dump() const noexcept;
			/**
			 * dump Метод установки бинарного дампа
			 * @param data бинарный дамп данных
			 */
			void dump(const vector <char> & data) noexcept;
		public:
			/**
			 * isEnd Метод проверки завершения обработки
			 * @return результат проверки
			 */
			bool isEnd() const noexcept;
			/**
			 * isCrypt Метод проверки на зашифрованные данные
			 * @return флаг проверки на зашифрованные данные
			 */
			bool isCrypt() const noexcept;
			/**
			 * isAlive Метод проверки на постоянное подключение
			 * @return результат проверки
			 */
			bool isAlive() const noexcept;
		public:
			/**
			 * isHandshake Метод проверки рукопожатия (Метод не должен быть виртуальным, так-как он должен быть переопределён в других модулях)
			 * @return проверка рукопожатия
			 */
			bool isHandshake() noexcept;
		public:
			/**
			 * isBlack Метод проверки существования заголовка в чёрный списоке
			 * @param key ключ заголовка для проверки
			 * @return    результат проверки
			 */
			bool isBlack(const string & key) const noexcept;
			/**
			 * isHeader Метод проверки существования заголовка
			 * @param key ключ заголовка для проверки
			 * @return    результат проверки
			 */
			bool isHeader(const string & key) const noexcept;
		public:
			/**
			 * request Метод получения объекта запроса на сервер
			 * @return объект запроса на сервер
			 */
			const web_t::req_t & request() const noexcept;
			/**
			 * request Метод добавления объекта запроса на сервер
			 * @param req объект запроса на сервер
			 */
			void request(const web_t::req_t & req) noexcept;
		public:
			/**
			 * response Метод получения объекта ответа сервера
			 * @return объект ответа сервера
			 */
			const web_t::res_t & response() const noexcept;
			/**
			 * response Метод добавления объекта ответа сервера
			 * @param res объект ответа сервера
			 */
			void response(const web_t::res_t & res) noexcept;
		public:
			/**
			 * date Метод получения текущей даты для HTTP запроса
			 * @param stamp штамп времени в числовом виде
			 * @return      штамп времени в текстовом виде
			 */
			const string date(const time_t stamp = 0) const noexcept;
			/**
			 * message Метод получения HTTP сообщения
			 * @param code код сообщения для получение
			 * @return     соответствующее коду HTTP сообщение
			 */
			const string & message(const u_int code) const noexcept;
		public:
			/**
			 * mapping Метод маппинга полученных данных
			 * @param flag флаг выполняемого процесса
			 * @param http объект для маппинга
			 */
			void mapping(const process_t flag, Http & http) noexcept;
		public:
			/**
			 * proxy Метод создания запроса для авторизации на прокси-сервере
			 * @param req объект параметров REST-запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			virtual vector <char> proxy(const web_t::req_t & req) const noexcept;
			/**
			 * proxy2 Метод создания запроса для авторизации на прокси-сервере (для протокола HTTP/2)
			 * @param req объект параметров REST-запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			virtual vector <pair <string, string>> proxy2(const web_t::req_t & req) const noexcept;
		public:
			/**
			 * reject Метод создания отрицательного ответа
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			virtual vector <char> reject(const web_t::res_t & res) const noexcept;
			/**
			 * reject2 Метод создания отрицательного ответа (для протокола HTTP/2)
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			virtual vector <pair <string, string>> reject2(const web_t::res_t & res) const noexcept;
		public:
			/**
			 * process Метод создания выполняемого процесса в бинарном виде
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			virtual vector <char> process(const process_t flag, const web_t::provider_t & prov) const noexcept;
			/**
			 * process2 Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			virtual vector <pair <string, string>> process2(const process_t flag, const web_t::provider_t & prov) const noexcept;
		public:
			/** 
			 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept;
			/** 
			 * on Метод установки функции вывода запроса клиента на выполненный запрос к серверу
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const web_t::method_t, const uri_t::url_t &)> callback) noexcept;
		public:
			/** 
			 * on Метод установки функции вывода полученного заголовка с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const string &, const string &)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова для получения чанков
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const vector <char> &, const Http *)> callback) noexcept;
		public:
			/** 
			 * on Метод установки функции вывода полученного тела данных с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const u_int, const string &, const vector <char> &)> callback) noexcept;
			/** 
			 * on Метод установки функции вывода полученного тела данных с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept;
		public:
			/** 
			 * on Метод установки функции вывода полученных заголовков с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept;
			/** 
			 * on Метод установки функции вывода полученных заголовков с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept;
		public:
			/**
			 * id Метод получения идентификатора объекта
			 * @return идентификатор объекта
			 */
			uint64_t id() const noexcept;
			/**
			 * id Метод установки идентификатора объекта
			 * @param id идентификатор объекта
			 */
			void id(const uint64_t id) noexcept;
		public:
			/**
			 * identity Метод извлечения идентичности протокола модуля
			 * @return флаг идентичности протокола модуля
			 */
			identity_t identity() const noexcept;
			/**
			 * identity Метод установки идентичности протокола модуля
			 * @param identity идентичность протокола модуля
			 */
			void identity(const identity_t identity) noexcept;
		public:
			/**
			 * chunk Метод установки размера чанка
			 * @param size размер чанка для установки
			 */
			void chunk(const size_t size) noexcept;
			/**
			 * userAgent Метод установки User-Agent для HTTP запроса
			 * @param userAgent агент пользователя для HTTP запроса
			 */
			void userAgent(const string & userAgent) noexcept;
		public:
			/**
			 * ident Метод установки идентификации сервера
			 * @param id   идентификатор сервиса
			 * @param name название сервиса
			 * @param ver  версия сервиса
			 */
			void ident(const string & id, const string & name, const string & ver) noexcept;
			/**
			 * crypto Метод установки параметров шифрования
			 * @param pass   пароль шифрования передаваемых данных
			 * @param salt   соль шифрования передаваемых данных
			 * @param cipher размер шифрования передаваемых данных
			 */
			void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
		public:
			/**
			 * Http Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Http(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Http Деструктор
			 */
			virtual ~Http() noexcept {}
	} http_t;
};

#endif // __AWH_HTTP__
