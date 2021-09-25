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
#include <map>
#include <string>
#include <vector>
#include <random>
#include <cstring>
#include <algorithm>
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
#include <auth.hpp>
#include <base64.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Http Класс для работы с REST запросами
	 */
	typedef class Http {
		public:
			/**
			 * Статусы проверки авторизации
			 */
			enum class stath_t: u_short {
				GOOD,  // Авторизация прошла успешно
				EMPTY, // Авторизация не выполнялась
				RETRY, // Требуется повторить попытку
				FAULT  // Авторизация не удалась
			};
			/**
			 * Формат сжатия тела запроса
			 */
			enum class compress_t : u_short {NONE, BR, GZIP, DEFLATE};
			/**
			 * Методы HTTP запроса
			 */
			enum class method_t : u_short {GET, DEL, PUT, POST, HEAD, PATCH, TRACE, OPTIONS};
		public:
			/**
			 * Query Структура запроса
			 */
			typedef struct Query {
				u_short code;   // Код ответа сервера
				double ver;     // Версия протокола
				string uri;     // Параметры запроса
				string method;  // Метод запроса
				string message; // Сообщение сервера
				/**
				 * Query Конструктор
				 */
				Query() : code(0), ver(HTTP_VERSION), uri(""), method(""), message("") {}
			} query_t;
		protected:
			// Размер максимального значения окна для сжатия данных GZIP
			static constexpr int GZIP_MAX_WBITS = 15;
			// Версия протокола WebSocket
			static constexpr u_short WS_VERSION = 13;
		protected:
			// Список HTTP сообщений
			map <u_short, pair <string, string>> messages = {
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
			 * Типы основных заголовков
			 */
			enum class header_t : u_short {
				HOST,          // Host
				ACCEPT,        // Accept
				ORIGIN,        // Origin
				USERAGENT,     // User-Agent
				CONNECTION,    // Connection
				CONTANTTYPE,   // Content-Type
				CONTENTLENGTH, // Content-Length
				ACCEPTLANGUAGE // Accept-Language
			};
			/**
			 * Стейты работы модуля
			 */
			enum class state_t: u_short {
				BODY,     // Режим чтения тела сообщения
				GOOD,     // Режим завершения сбора данных
				QUERY,    // Режим ожидания получения запроса
				BROKEN,   // Режим бракованных данных
				HEADERS,  // Режим чтения заголовков
				HANDSHAKE // Режим выполненного рукопожатия
			};
		protected:
			// Объект параметров запроса
			query_t query;
			// Объект собираемого чанка
			chunk_t chunk;
		protected:
			// Размер одного чанка
			size_t chunkSize = BUFFER_CHUNK;
			// Размер скользящего окна клиента
			short wbitClient = GZIP_MAX_WBITS;
			// Размер скользящего окна сервера
			short wbitServer = GZIP_MAX_WBITS;
			// Флаги работы с сжатыми данными
			compress_t compress = compress_t::DEFLATE;
		protected:
			// Флаг зашифрованных данных
			bool crypt = false;
			// Флаг разрешающий передавать тело чанками
			bool chunking = true;
		protected:
			// Стейт проверки авторизации
			stath_t stath = stath_t::EMPTY;
			// Стейт текущего запроса
			state_t state = state_t::QUERY;
		protected:
			// Поддерживаемый сабпротокол
			string sub = "";
			// Ключ клиента
			mutable string keyWebSocket = "";
		protected:
			// Поддерживаемые сабпротоколы
			set <string> subs;
			// Полученное тело HTTP запроса
			mutable vector <char> body;
			// Полученные HTTP заголовки
			mutable unordered_multimap <string, string> headers;
		private:
			// Функция вызова при получении чанка
			function <void (const vector <char> &, const Http *)> chunkingFn = nullptr;
		protected:
			// Создаём объект для работы с авторизацией
			auth_t * auth = nullptr;
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
			// Создаём URL адрес запроса
			const uri_t::url_t * url = nullptr;
		protected:
			/**
			 * date Метод получения текущей даты для HTTP запроса
			 * @return текущая дата
			 */
			const string date() const noexcept;
			/**
			 * wsKey Метод генерации ключа для WebSocket
			 * @return сгенерированный ключ для WebSocket
			 */
			const string wsKey() const noexcept;
			/**
			 * wsHash Метод генерации хэша ключа
			 * @return сгенерированный хэш ключа клиента
			 */
			const string wsHash() const noexcept;
		protected:
			/**
			 * updateExtensions Метод проверки полученных расширений
			 */
			virtual void updateExtensions() noexcept = 0;
			/**
			 * updateSubProtocol Метод извлечения доступного сабпротокола
			 */
			virtual void updateSubProtocol() noexcept = 0;
		public:
			/**
			 * checkUpgrade Метод получения флага переключения протокола
			 * @return флага переключения протокола
			 */
			virtual bool checkUpgrade() const noexcept;
			/**
			 * checkKeyWebSocket Метод проверки ключа сервера WebSocket
			 * @return результат проверки
			 */
			virtual bool checkKeyWebSocket() noexcept = 0;
			/**
			 * checkVerWebSocket Метод проверки на версию протокола WebSocket
			 * @return результат проверки соответствия
			 */
			virtual bool checkVerWebSocket() noexcept = 0;
			/**
			 * checkAuthenticate Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			virtual stath_t checkAuthenticate() noexcept = 0;
		public:
			/**
			 * clear Метод очистки собранных данных
			 */
			virtual void clear() noexcept;
		public:
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
			/**
			 * getCompress Метод получения метода сжатия
			 * @return метод сжатия сообщений
			 */
			compress_t getCompress() const noexcept;
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
			 * isHandshake Метод получения флага рукопожатия
			 * @return флаг получения рукопожатия
			 */
			bool isHandshake() const noexcept;
			/**
			 * isHeader Метод проверки существования заголовка
			 * @param key ключ заголовка для проверки
			 * @return    результат проверки
			 */
			bool isHeader(const string & key) const noexcept;
		public:
			/**
			 * getWbitClient Метод получения размер скользящего окна для клиента
			 * @return размер скользящего окна
			 */
			short getWbitClient() const noexcept;
			/**
			 * getWbitServer Метод получения размер скользящего окна для сервера
			 * @return размер скользящего окна
			 */
			short getWbitServer() const noexcept;
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
			 * getSub Метод получения выбранного сабпротокола
			 * @return выбранный сабпротокол
			 */
			const string & getSub() const noexcept;
			/**
			 * getMessage Метод получения HTTP сообщения
			 * @param code код сообщения для получение
			 * @return     соответствующее коду HTTP сообщение
			 */
			const string & getMessage(const u_short code) const noexcept;
		public:
			/**
			 * websocket Метод создания ответа для WebSocket
			 * @return буфер данных запроса в бинарном виде
			 */
			vector <char> websocket() const noexcept;
			/**
			 * websocket Метод создания запроса для WebSocket
			 * @param url объект параметров REST запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			vector <char> websocket(const uri_t::url_t & url) const noexcept;
		public:
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
			 * setChunkingFn Метод установки функции обратного вызова для получения чанков
			 * @param callback функция обратного вызова
			 */
			void setChunkingFn(function <void (const vector <char> &, const Http *)> callback) noexcept;
		public:
			/**
			 * setUser Метод установки параметров авторизации
			 * @param login    логин пользователя для авторизации на сервере
			 * @param password пароль пользователя для авторизации на сервере
			 */
			void setUser(const string & login, const string & password) noexcept;
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type      тип авторизации
			 * @param algorithm алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::algorithm_t algorithm = auth_t::algorithm_t::MD5) noexcept;
		public:
			/**
			 * Http Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			Http(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * Http Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 * @param url объект URL адреса сервера
			 */
			Http(const fmk_t * fmk, const log_t * log, const uri_t * uri, const uri_t::url_t * url) noexcept;
			/**
			 * ~Http Деструктор
			 */
			virtual ~Http() noexcept;
	} http_t;
};

#endif // __AWH_HTTP__
