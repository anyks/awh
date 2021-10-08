/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_HTTP__
#define __AWH_HTTP__

/**
 * Стандартная библиотека
 */
#include <set>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>
#include <unordered_map>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <time.h>
// Если - это Unix
#else
	#include <ctime>
#endif

/**
 * Наши модули
 */
#include <fmk.hpp>
#include <log.hpp>
#include <uri.hpp>
#include <hash.hpp>
#include <client/auth.hpp>
#include <server/auth.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
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
				GOOD,  // Авторизация прошла успешно
				EMPTY, // Проверка авторизации не проводилась
				RETRY, // Требуется повторить попытку
				FAULT  // Авторизация не удалась
			};
			/**
			 * Формат сжатия тела запроса
			 */
			enum class compress_t : uint8_t {NONE, BROTLI, GZIP, DEFLATE};
			/**
			 * Методы HTTP запроса
			 */
			enum class method_t : uint8_t {NONE, GET, PUT, DEL, POST, HEAD, PATCH, TRACE, OPTIONS, CONNECT};
		public:
			/**
			 * Query Структура запроса
			 */
			typedef struct Query {
				u_short code;    // Код ответа сервера
				double ver;      // Версия протокола
				method_t method; // Метод запроса
				string uri;      // Параметры запроса
				string message;  // Сообщение сервера
				/**
				 * Query Конструктор
				 */
				Query() : code(0), ver(HTTP_VERSION), method(method_t::NONE), uri(""), message("") {}
			} query_t;
		protected:
			// Список HTTP сообщений
			map <u_short, string> messages = {
				{100, "Continue"},
				{101, "Switching Protocol"},
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
		private:
			/**
			 * Chunk Структура собираемого чанка
			 */
			typedef struct Chunk {
				public:
					size_t size;        // Размер чанка
					vector <char> data; // Данные чанка
				public:
					/**
					 * clear Метод очистки данных чанка
					 */
					void clear() noexcept {
						// Обнуляем размер чанка
						this->size = 0;
						// Обнуляем буфер данных
						this->data.clear();
					}
				public:
					/**
					 * Chunk Конструктор
					 */
					Chunk() : size(0) {}
			} chunk_t;
			/**
			 * Режимы работы модуля
			 */
			enum class mode_t : uint8_t {NONE, REQUEST, RESPONSE};
		protected:
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				BODY,     // Режим чтения тела сообщения
				GOOD,     // Режим завершения сбора данных
				QUERY,    // Режим ожидания получения запроса
				BROKEN,   // Режим бракованных данных
				HEADERS,  // Режим чтения заголовков
				HANDSHAKE // Режим выполненного рукопожатия
			};
		protected:
			// Объект параметров запроса
			mutable query_t query;
			// Объект собираемого чанка
			mutable chunk_t chunk;
			// Параметры выполняемого запроса
			mutable uri_t::url_t url;
		protected:
			// Флаг зашифрованных данных
			bool crypt = false;
			// Флаг проверки аутентификации
			mutable bool failAuth = false;
			// Флаг разрешающий передавать тело чанками
			mutable bool chunking = false;
		protected:
			// Размер тела сообщения
			int64_t bodySize = -1;
			// Размер одного чанка
			size_t chunkSize = BUFFER_CHUNK;
		protected:
			// Название сервиса
			string servName = AWH_NAME;
			// Версия сервиса
			string servVer = AWH_VERSION;
			// Идентификатор сервиса
			string servId = AWH_SHORT_NAME;
			// User-Agent для HTTP запроса
			mutable string userAgent = HTTP_HEADER_AGENT;
		protected:
			// Режим работы модуля
			mode_t mode = mode_t::NONE;
			// Стейт проверки авторизации
			stath_t stath = stath_t::EMPTY;
			// Стейт текущего запроса
			state_t state = state_t::QUERY;
			// Метод сжатия данных запроса/ответа
			compress_t compress = compress_t::GZIP;
		protected:
			// Полученное тело HTTP запроса
			mutable vector <char> body;
			// Чёрный список заголовков
			mutable unordered_set <string> black;
			// Полученные HTTP заголовки
			mutable unordered_multimap <string, string> headers;
		private:
			// Функция вызова при получении чанка
			function <void (const vector <char> &, const Http *)> chunkingFn = nullptr;
		protected:
			// Создаём объект для работы с авторизацией
			auth_t * auth = nullptr;
			// Создаём объект для работы с жатыми данными
			hash_t * hash = nullptr;
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
		protected:
			/**
			 * date Метод получения текущей даты для HTTP запроса
			 * @return текущая дата
			 */
			const string date() const noexcept;
			/**
			 * checkAuth Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			virtual stath_t checkAuth() noexcept;
		protected:
			/**
			 * update Метод обновления входящих данных
			 */
			virtual void update() noexcept;
		public:
			/**
			 * clear Метод очистки собранных данных
			 */
			virtual void clear() noexcept;
		public:
			/**
			 * addBlack Метод добавления заголовка в чёрный список
			 * @param key ключ заголовка
			 */
			void addBlack(const string & key) noexcept;
			/**
			 * parse Метод парсинга сырых данных
			 * @param buffer буфер данных для обработки
			 * @param size   размер буфера данных
			 */
			void parse(const char * buffer, const size_t size) noexcept;
			/**
			 * addBody Метод добавления буфера тела данных запроса
			 * @param buffer буфер данных тела запроса
			 * @param size   размер буфера данных
			 */
			void addBody(const char * buffer, const size_t size) noexcept;
			/**
			 * addHeader Метод добавления заголовка
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void addHeader(const string & key, const string & val) noexcept;
		public:
			/**
			 * getBody Метод получения данных тела запроса
			 * @return буфер данных тела запроса
			 */
			const vector <char> & getBody() const noexcept;
			/**
			 * chunkBody Метод чтения чанка тела запроса
			 * @return текущий чанк запроса
			 */
			const vector <char> chunkBody() const noexcept;
			/**
			 * getHeader Метод получения данных заголовка
			 * @param key ключ заголовка
			 * @return    значение заголовка
			 */
			const string & getHeader(const string & key) const noexcept;
			/**
			 * getHeaders Метод получения списка заголовков
			 * @return список существующих заголовков
			 */
			const unordered_multimap <string, string> & getHeaders() const noexcept;
		private:
			/**
			 * readHeader Функция чтения заголовков из буфера данных
			 * @param buffer   буфер данных для чтения
			 * @param size     размер буфера данных для чтения
			 * @param callback функция обратного вызова
			 */
			static void readHeader(const char * buffer, const size_t size, function <void (string)> callback) noexcept;
		public:
			/**
			 * getAuth Метод проверки статуса авторизации
			 * @return результат проверки
			 */
			stath_t getAuth() const noexcept;
		public:
			/**
			 * getCompress Метод получения метода сжатия
			 * @return метод сжатия сообщений
			 */
			virtual compress_t getCompress() const noexcept;
			/**
			 * setCompress Метод установки метода сжатия
			 * @param метод сжатия сообщений
			 */
			virtual void setCompress(const compress_t compress) noexcept;
		public:
			/**
			 * getUrl Метод извлечения параметров запроса
			 * @return установленные параметры запроса
			 */
			const uri_t::url_t & getUrl() const noexcept;
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
			/**
			 * isHandshake Метод проверки рукопожатия
			 * @return проверка рукопожатия
			 */
			virtual bool isHandshake() noexcept;
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
			 * getQuery Метод получения объекта запроса сервера
			 * @return объект запроса сервера
			 */
			const query_t & getQuery() const noexcept;
			/**
			 * setQuery Метод добавления объекта запроса клиента
			 * @param query объект запроса клиента
			 */
			void setQuery(const query_t & query) noexcept;
		public:
			/**
			 * getMessage Метод получения HTTP сообщения
			 * @param code код сообщения для получение
			 * @return     соответствующее коду HTTP сообщение
			 */
			const string & getMessage(const u_short code) const noexcept;
		public:
			/**
			 * proxy Метод создания запроса для авторизации на прокси-сервере
			 * @param url объект параметров REST запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			vector <char> proxy(const uri_t::url_t & url) noexcept;
			/**
			 * reject Метод создания отрицательного ответа
			 * @param code код ответа
			 * @return     буфер данных запроса в бинарном виде
			 */
			vector <char> reject(const u_short code) const noexcept;
			/**
			 * response Метод создания ответа
			 * @param code код ответа
			 * @return     буфер данных запроса в бинарном виде
			 */
			vector <char> response(const u_short code) const noexcept;
			/**
			 * request Метод создания запроса
			 * @param url    объект параметров REST запроса
			 * @param method метод REST запроса
			 * @return       буфер данных запроса в бинарном виде
			 */
			vector <char> request(const uri_t::url_t & url, const method_t method) const noexcept;
		public:
			/**
			 * setChunkingFn Метод установки функции обратного вызова для получения чанков
			 * @param callback функция обратного вызова
			 */
			void setChunkingFn(function <void (const vector <char> &, const Http *)> callback) noexcept;
		public:
			/**
			 * setChunkSize Метод установки размера чанка
			 * @param size размер чанка для установки
			 */
			void setChunkSize(const size_t size) noexcept;
			/**
			 * setUserAgent Метод установки User-Agent для HTTP запроса
			 * @param userAgent агент пользователя для HTTP запроса
			 */
			void setUserAgent(const string & userAgent) noexcept;
		public:
			/**
			 * setServ Метод установки данных сервиса
			 * @param id   идентификатор сервиса
			 * @param name название сервиса
			 * @param ver  версия сервиса
			 */
			void setServ(const string & id, const string & name, const string & ver) noexcept;
			/**
			 * setCrypt Метод установки параметров шифрования
			 * @param pass пароль шифрования передаваемых данных
			 * @param salt соль шифрования передаваемых данных
			 * @param aes  размер шифрования передаваемых данных
			 */
			void setCrypt(const string & pass, const string & salt = "", const hash_t::aes_t aes = hash_t::aes_t::AES128) noexcept;
		public:
			/**
			 * Http Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			Http(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * ~Http Деструктор
			 */
			virtual ~Http() noexcept;
	} http_t;
};

#endif // __AWH_HTTP__
