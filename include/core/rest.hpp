/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_CORE_REST__
#define __AWH_CORE_REST__

/**
 * Стандартная библиотека
 */
#include <nlohmann/json.hpp>

/**
 * Наши модули
 */
#include <http.hpp>
#include <core/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Rest Класс работы с REST клиентом
	 */
	typedef class Rest {
		private:
			/**
			 * Response Структура ответа сервера
			 */
			typedef struct Response {
				bool ok;                                     // Флаг удачного ответа
				u_short code;                                // Код ответа сервера
				string mess;                                 // Сообщение ответа сервера
				string entity;                               // Тело ответа сервера
				unordered_multimap <string, string> headers; // Заголовки сервера
				/**
				 * Response Конструктор
				 */
				Response() : ok(false), code(0), mess(""), entity(""), headers({}) {}
			} res_t;
		private:
			// Параметры ответа
			res_t res;
			// Объект рабочего
			worker_t worker;
			// Метод выполняемого запроса
			http_t::method_t method;
		private:
			// Выполнять анбиндинг после завершения запроса
			bool unbind = true;
			// Флаг проверки аутентификации
			bool failAuth = false;
		private:
			// Тело запроса (если требуется)
			const string * entity = nullptr;
			// Список заголовков запроса (если требуется)
			const unordered_multimap <string, string> * headers = nullptr;
		private:
			// Создаём объект для работы с HTTP
			http_t * http = nullptr;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
			// Создаём объект биндинга TCP/IP
			const core_t * core = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
		private:
			/**
			 * chunking Метод обработки получения чанков
			 * @param chunk бинарный буфер чанка
			 * @param ctx   контекст объекта http
			 */
			static void chunking(const vector <char> & chunk, const http_t * ctx) noexcept;
		private:
			/**
			 * openCallback Функция обратного вызова при подключении к серверу
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void openCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * closeCallback Функция обратного вызова при отключении от сервера
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void closeCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * startCallback Функция обратного вызова при запуске работы
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void startCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * openProxyCallback Функция обратного вызова при подключении к прокси-серверу
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void openProxyCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * readCallback Функция обратного вызова при чтении сообщения с сервера
			 * @param buffer бинарный буфер содержащий сообщение
			 * @param size   размер бинарного буфера содержащего сообщение
			 * @param wid    идентификатор воркера
			 * @param core   объект биндинга TCP/IP
			 * @param ctx    передаваемый контекст модуля
			 */
			static void readCallback(const char * buffer, const size_t size, const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * readProxyCallback Функция обратного вызова при чтении сообщения с прокси-сервера
			 * @param buffer бинарный буфер содержащий сообщение
			 * @param size   размер бинарного буфера содержащего сообщение
			 * @param wid    идентификатор воркера
			 * @param core   объект биндинга TCP/IP
			 * @param ctx    передаваемый контекст модуля
			 */
			static void readProxyCallback(const char * buffer, const size_t size, const size_t wid, core_t * core, void * ctx) noexcept;
		public:
			/**
			 * GET Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
		public:
			/**
			 * setChunkingFn Метод установки функции обратного вызова для получения чанков
			 * @param callback функция обратного вызова
			 */
			void setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept;
		public:
			/**
			 * setBytesDetect Метод детекции сообщений по количеству байт
			 * @param read  количество байт для детекции по чтению
			 * @param write количество байт для детекции по записи
			 */
			void setBytesDetect(const size_t read, const size_t write) noexcept;
			/**
			 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
			 * @param read  количество секунд для детекции по чтению
			 * @param write количество секунд для детекции по записи
			 */
			void setWaitTimeDetect(const time_t read, const time_t write) noexcept;
		public:
			/**
			 * setUnbind Метод установки флага анбиндинга
			 * @param mode флаг анбиндинга после завершения запроса
			 */
			void setUnbind(const bool mode) noexcept;
			/**
			 * setKeepAlive Метод установки флага автоматического поддержания подключения
			 * @param mode флаг автоматического поддержания подключения
			 */
			void setKeepAlive(const bool mode) noexcept;
			/**
			 * setChunkSize Метод установки размера чанка
			 * @param size размер чанка для установки
			 */
			void setChunkSize(const size_t size) noexcept;
			/**
			 * setWaitMessage Метод установки флага ожидания входящих сообщений
			 * @param mode флаг состояния разрешения проверки
			 */
			void setWaitMessage(const bool mode) noexcept;
			/**
			 * setAttempts Метод установки количества попыток переподключения
			 * @param count количество попыток переподключения
			 */
			void setAttempts(const u_short count) noexcept;
			/**
			 * setUserAgent Метод установки User-Agent для HTTP запроса
			 * @param userAgent агент пользователя для HTTP запроса
			 */
			void setUserAgent(const string & userAgent) noexcept;
			/**
			 * setCompress Метод установки метода сжатия
			 * @param метод сжатия сообщений
			 */
			void setCompress(const http_t::compress_t compress) noexcept;
			/**
			 * setUser Метод установки параметров авторизации
			 * @param login    логин пользователя для авторизации на сервере
			 * @param password пароль пользователя для авторизации на сервере
			 */
			void setUser(const string & login, const string & password) noexcept;
			/**
			 * setProxyServer Метод установки прокси-сервера
			 * @param uri  параметры прокси-сервера
			 * @param type тип прокси-сервера
			 */
			void setProxyServer(const string & uri, const proxy_t::type_t type) noexcept;
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
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type      тип авторизации
			 * @param algorithm алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::algorithm_t algorithm = auth_t::algorithm_t::MD5) noexcept;
			/**
			 * setAuthTypeProxy Метод установки типа авторизации прокси-сервера
			 * @param type      тип авторизации
			 * @param algorithm алгоритм шифрования для Digest авторизации
			 */
			void setAuthTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::algorithm_t algorithm = auth_t::algorithm_t::MD5) noexcept;
		public:
			/**
			 * Rest Конструктор
			 * @param core объект биндинга TCP/IP
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Rest(const core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Rest Деструктор
			 */
			~Rest() noexcept;
	} rest_t;
};

#endif // __AWH_CORE_REST__
