/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_AUTH_SERVER__
#define __AWH_AUTH_SERVER__

/**
 * Наши модули
 */
#include <auth/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/*
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * AuthServer Класс работы с авторизацией на сервере
		 */
		typedef class Auth : public auth_t {
			private:
				// Логин пользователя
				string user = "";
				// Пароль пользователя
				string pass = "";
				// Параметры Digest авторизации пользователя
				digest_t userDigest;
			private:
				// Список контекстов передаваемых объектов
				vector <void *> ctx = {nullptr, nullptr};
			private:
				// Внешняя функция получения пароля пользователя авторизации Digest
				function <string (const string &, void *)> extractPassFn = nullptr;
				// Внешняя функция проверки авторизации Basic
				function <bool (const string &, const string &, void *)> authFn = nullptr;
			public:
				/**
				 * check Метод проверки авторизации
				 * @param method метод HTTP запроса
				 * @return       результат проверки авторизации
				 */
				const bool check(const string & method) noexcept;
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
				 * setHeader Метод установки параметров авторизации из заголовков
				 * @param header заголовок HTTP с параметрами авторизации
				 */
				void setHeader(const string & header) noexcept;
				/**
				 * getHeader Метод получения строки авторизации HTTP заголовка
				 * @param mode режим вывода только значения заголовка
				 * @return     строка авторизации
				 */
				const string getHeader(const bool mode = false) noexcept;
			public:
				/**
				 * Auth Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept : auth_t(fmk, log) {}
		} auth_t;
	};
};

#endif // __AWH_AUTH_SERVER__
