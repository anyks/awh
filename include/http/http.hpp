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
		protected:
			/**
			 * Стейты работы модуля
			 */
			enum class state_t: u_short {
				QUERY,    // Режим ожидания получения запроса
				HEADERS,  // Режим чтения заголовков
				HANDSHAKE // Режим выполненного рукопожатия
			};
			// Размер максимального значения окна для сжатия данных GZIP
			static constexpr int GZIP_MAX_WBITS = 15;
			// Версия протокола WebSocket
			static constexpr u_short WS_VERSION = 13;
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
				{401, "Unauthorized"},
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
			// Флаг разрешающий сжатие данных
			bool gzip = false;
			// Размер скользящего окна клиента
			short wbitClient = GZIP_MAX_WBITS;
			// Размер скользящего окна сервера
			short wbitServer = GZIP_MAX_WBITS;
		protected:
			// Код ответа сервера
			u_short code = 0;
			// Версия протокола
			double version = HTTP_VERSION;
			// Стейт проверки авторизации
			stath_t stath = stath_t::EMPTY;
			// Стейт текущего запроса
			state_t state = state_t::QUERY;
		protected:
			// Поддерживаемый сабпротокол
			string sub = "";
			// Заголовок HTTP запроса Origin
			string origin = "";
			// Сообщение сервера
			string message = "";
			// Ключ клиента
			string clientKey = "";
			// User-Agent для HTTP запроса
			string userAgent = USER_AGENT;
		protected:
			// Поддерживаемые сабпротоколы
			set <string> subs;
			// Полученные HTTP заголовки
			unordered_multimap <string, string> headers;
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
			 * key Метод генерации ключа для WebSocket
			 * @return сгенерированный ключ для WebSocket
			 */
			const string key() const noexcept;
			/**
			 * date Метод получения текущей даты для HTTP запроса
			 * @return текущая дата
			 */
			const string date() const noexcept;
			/**
			 * generateHash Метод генерации хэша ключа
			 * @return сгенерированный хэш ключа клиента
			 */
			const string generateHash() const noexcept;
		protected:
			/**
			 * updateExtensions Метод проверки полученных расширений
			 */
			virtual void updateExtensions() noexcept = 0;
			/**
			 * updateSubProtocol Метод извлечения доступного сабпротокола
			 */
			virtual void updateSubProtocol() noexcept = 0;
		protected:
			/**
			 * checkKey Метод проверки ключа сервера
			 * @return результат проверки
			 */
			virtual bool checkKey() noexcept = 0;
			/**
			 * checkVersion Метод проверки на версию протокола WebSocket
			 * @return результат проверки соответствия
			 */
			virtual bool checkVersion() noexcept = 0;
		protected:
			/**
			 * checkUpgrade Метод получения флага переключения протокола
			 * @return флага переключения протокола
			 */
			virtual bool checkUpgrade() const noexcept;
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
			/**
			 * add Метод добавления данных заголовков
			 * @param buffer буфер данных с заголовками
			 * @param size   размер буфера данных с заголовками
			 * @return       результат завершения сбора данных
			 */
			virtual bool add(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * isGzip Метод получения флага сжатого контента в GZIP
			 * @return значение флага сжатого контента в GZIP
			 */
			bool isGzip() const noexcept;
			/**
			 * isHandshake Метод получения флага рукопожатия
			 * @return флаг получения рукопожатия
			 */
			bool isHandshake() const noexcept;
			/**
			 * isAuth Метод проверки статуса авторизации
			 * @return результат проверки
			 */
			stath_t isAuth() const noexcept;
		public:
			/**
			 * getCode Метод получения кода ответа сервера
			 * @return код ответа сервера
			 */
			u_short getCode() const noexcept;
			/**
			 * getVersion Метод получения версии HTTP протокола
			 * @return версия HTTP протокола
			 */
			double getVersion() const noexcept;
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
			 * restReject Метод получения буфера ответа HTML реджекта
			 * @return собранный HTML буфер
			 */
			vector <char> restReject() const noexcept;
			/**
			 * restResponse Метод получения буфера HTML ответа
			 * @return собранный HTML буфер
			 */
			vector <char> restResponse() const noexcept;
			/**
			 * restUnauthorized Метод получения буфера запроса HTML авторизации
			 * @return собранный HTML буфер
			 */
			vector <char> restUnauthorized() const noexcept;
			/**
			 * restRequest Метод получения буфера HTML запроса
			 * @param gzip флаг просьбы предоставить контент в сжатом виде
			 * @return     собранный HTML буфер
			 */
			vector <char> restRequest(const bool gzip = false) noexcept;
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
			 * setOrigin Метод установки Origin запроса
			 * @param origin HTTP заголовок запроса
			 */
			void setOrigin(const string & origin) noexcept;
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
