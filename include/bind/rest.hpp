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
#include <bind/bind.hpp>

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
			// Параметры прокси-сервера
			proxy_t proxy;
		private:
			// Флаг ожидания входящих сообщений
			bool wait = false;
			// Флаг автоматического поддержания подключения
			bool alive = false;
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
			const bind_t * bind = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
		private:
			/**
			 * chunking Метод обработки получения чанков
			 * @param chunk бинарный буфер чанка
			 * @param ctx   контекст объекта http
			 */
			static void chunking(const vector <char> & chunk, const http_t * ctx) noexcept;
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
			 * @param bind объект биндинга TCP/IP
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Rest(const bind_t * bind, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Rest Деструктор
			 */
			~Rest() noexcept;
	} rest_t;
};

#endif // __AWH_CORE_REST__
