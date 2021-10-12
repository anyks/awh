/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_HTTP_SERVER__
#define __AWH_HTTP_SERVER__

/**
 * Наши модули
 */
#include <core/http.hpp>
#include <server/auth.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * HttpServer Класс для работы с REST сервером
	 */
	typedef class HttpServer : public http_t {
		public:
			/**
			 * setRealm Метод установки название сервера
			 * @param realm название сервера
			 */
			void setRealm(const string & realm) noexcept;
			/**
			 * setNonce Метод установки уникального ключа клиента выданного сервером
			 * @param nonce уникальный ключ клиента
			 */
			void setNonce(const string & nonce) noexcept;
			/**
			 * setOpaque Метод установки временного ключа сессии сервера
			 * @param opaque временный ключ сессии сервера
			 */
			void setOpaque(const string & opaque) noexcept;
		public:
			/**
			 * setExtractPassCallback Метод добавления функции извлечения пароля
			 * @param callback функция обратного вызова для извлечения пароля
			 */
			void setExtractPassCallback(function <string (const string &)> callback) noexcept;
			/**
			 * setAuthCallback Метод добавления функции обработки авторизации
			 * @param callback функция обратного вызова для обработки авторизации
			 */
			void setAuthCallback(function <bool (const string &, const string &)> callback) noexcept;
		public:
			/**
			 * HttpServer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			HttpServer(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * ~HttpServer Деструктор
			 */
			~HttpServer() noexcept {}
	} httpSrv_t;
};

#endif // __AWH_HTTP_SERVER__
