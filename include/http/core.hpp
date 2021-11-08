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
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>

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
#include <http/web.hpp>
#include <auth/client.hpp>
#include <auth/server.hpp>

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
				NONE,  // Проверка авторизации не проводилась
				GOOD,  // Авторизация прошла успешно
				RETRY, // Требуется повторить попытку
				FAULT  // Авторизация не удалась
			};
			/**
			 * Формат сжатия тела запроса
			 */
			enum class compress_t : uint8_t {NONE, BROTLI, GZIP, DEFLATE};
		protected:
			// Список HTTP сообщений
			map <u_short, string> messages = {
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
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				NONE,     // Режим стейта не выставлен
				GOOD,     // Режим удачного выполнения запроса
				BROKEN,   // Режим бракованных данных
				HANDSHAKE // Режим выполненного рукопожатия
			};
		protected:
			// Создаём объект HTTP парсера
			mutable web_t web;
			// Создаём объект для работы с жатыми данными
			mutable hash_t hash;
			// Параметры выполняемого запроса
			mutable uri_t::url_t url;
		protected:
			// Создаём объект для работы с клиентской авторизацией
			mutable authCli_t authCli;
			// Создаём объект для работы с серверной авторизацией
			mutable authSrv_t authSrv;
		protected:
			// Флаг зашифрованных данных
			mutable bool crypt = false;
			// Флаг проверки аутентификации
			mutable bool failAuth = false;
			// Флаг разрешающий передавать тело чанками
			mutable bool chunking = false;
		protected:
			// Размер одного чанка
			size_t chunkSize = BUFFER_CHUNK;
		protected:
			// Название сервиса
			string servName = AWH_NAME;
			// Версия библиотеки приложения
			string servVer = AWH_VERSION;
			// Идентификатор сервиса
			string servId = AWH_SHORT_NAME;
			// User-Agent для HTTP запроса
			mutable string userAgent = HTTP_HEADER_AGENT;
		protected:
			// Стейт проверки авторизации
			stath_t stath = stath_t::NONE;
			// Стейт текущего запроса
			state_t state = state_t::NONE;
			// Метод сжатия данных запроса/ответа
			compress_t compress = compress_t::GZIP;
		protected:
			// Чёрный список заголовков
			mutable unordered_set <string> black;
		private:
			// Список контекстов передаваемых объектов
			vector <void *> ctx = {nullptr};
			// Функция вызова при получении чанка
			function <void (const vector <char> &, const Http *, void *)> chunkingFn = nullptr;
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
		private:
			/**
			 * chunkingCallback Функция вывода полученных чанков полезной нагрузки
			 * @param buffer буфер данных чанка полезной нагрузки
			 * @param web    объект HTTP парсера
			 * @param ctx    передаваемый контекст модуля
			 */
			static void chunkingCallback(const vector <char> & buffer, const web_t * web, void * ctx) noexcept;
		protected:
			/**
			 * update Метод обновления входящих данных
			 */
			virtual void update() noexcept;
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
			 * setBody Метод установки данных тела
			 * @param body буфер тела для установки
			 */
			void setBody(const vector <char> & body) noexcept;
			/**
			 * addBody Метод добавления буфера тела данных запроса
			 * @param buffer буфер данных тела запроса
			 * @param size   размер буфера данных
			 */
			void addBody(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * addHeader Метод добавления заголовка
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void addHeader(const string & key, const string & val) noexcept;
			/**
			 * setHeaders Метод установки списка заголовков
			 * @param headers список заголовков для установки
			 */
			void setHeaders(const unordered_multimap <string, string> & headers) noexcept;
		public:
			/**
			 * payload Метод чтения чанка тела запроса
			 * @return текущий чанк запроса
			 */
			const vector <char> payload() const noexcept;
			/**
			 * getBody Метод получения данных тела запроса
			 * @return буфер данных тела запроса
			 */
			const vector <char> & getBody() const noexcept;
		public:
			/**
			 * rmHeader Метод удаления заголовка
			 * @param key ключ заголовка
			 */
			void rmHeader(const string & key) noexcept;
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
			const web_t::query_t & getQuery() const noexcept;
			/**
			 * setQuery Метод добавления объекта запроса клиента
			 * @param query объект запроса клиента
			 */
			void setQuery(const web_t::query_t & query) noexcept;
		public:
			/**
			 * date Метод получения текущей даты для HTTP запроса
			 * @param stamp штамп времени в числовом виде
			 * @return      штамп времени в текстовом виде
			 */
			const string date(const time_t stamp = 0) const noexcept;
			/**
			 * getMessage Метод получения HTTP сообщения
			 * @param code код сообщения для получение
			 * @return     соответствующее коду HTTP сообщение
			 */
			const string & getMessage(const u_int code) const noexcept;
		public:
			/**
			 * request Метод создания запроса как он есть
			 * @param nobody флаг запрета подготовки тела
			 * @return       буфер данных запроса в бинарном виде
			 */
			vector <char> request(const bool nobody = false) const noexcept;
			/**
			 * response Метод создания ответа как он есть
			 * @param nobody флаг запрета подготовки тела
			 * @return       буфер данных ответа в бинарном виде
			 */
			vector <char> response(const bool nobody = false) const noexcept;
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
			 * @param mess сообщение ответа
			 * @return     буфер данных запроса в бинарном виде
			 */
			vector <char> reject(const u_int code, const string & mess = "") const noexcept;
			/**
			 * response Метод создания ответа
			 * @param code код ответа
			 * @param mess сообщение ответа
			 * @return     буфер данных запроса в бинарном виде
			 */
			vector <char> response(const u_int code, const string & mess = "") const noexcept;
			/**
			 * request Метод создания запроса
			 * @param url    объект параметров REST запроса
			 * @param method метод REST запроса
			 * @return       буфер данных запроса в бинарном виде
			 */
			vector <char> request(const uri_t::url_t & url, const web_t::method_t method) const noexcept;
		public:
			/**
			 * setChunkingFn Метод установки функции обратного вызова для получения чанков
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void setChunkingFn(void * ctx, function <void (const vector <char> &, const Http *, void *)> callback) noexcept;
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
			virtual ~Http() noexcept {}
	} http_t;
};

#endif // __AWH_HTTP__
