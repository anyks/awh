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
		private:
			/**
			 * checkAuth Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			stath_t checkAuth() noexcept;
		public:
			/**
			 * setRealm Метод установки название сервера
			 * @param realm название сервера
			 */
			void setRealm(const string & realm) noexcept;
			/**
			 * setOpaque Метод установки временного ключа сессии сервера
			 * @param opaque временный ключ сессии сервера
			 */
			void setOpaque(const string & opaque) noexcept;
		public:
			/**
			 * setExtractPassCallback Метод добавления функции извлечения пароля
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова для извлечения пароля
			 */
			void setExtractPassCallback(void * ctx, function <string (const string &, void *)> callback) noexcept;
			/**
			 * setAuthCallback Метод добавления функции обработки авторизации
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова для обработки авторизации
			 */
			void setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept;
		public:
			/**
			 * setAuthType Метод установки типа авторизации
			 * @param type тип авторизации
			 * @param alg  алгоритм шифрования для Digest авторизации
			 */
			void setAuthType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::alg_t alg = auth_t::alg_t::MD5) noexcept;
		public:
			/**
			 * HttpServer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			HttpServer(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : http_t(fmk, log, uri) {
				// Устанавливаем тип HTTP модуля
				this->web.init(web_t::hid_t::SERVER);
			}
			/**
			 * ~HttpServer Деструктор
			 */
			~HttpServer() noexcept {}
	} httpSrv_t;
};

#endif // __AWH_HTTP_SERVER__
