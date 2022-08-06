/**
 * @file: server.hpp
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

#ifndef __AWH_AUTH_SERVER__
#define __AWH_AUTH_SERVER__

/**
 * Наши модули
 */
#include <auth/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * AuthServer Класс работы с авторизацией на сервере
		 */
		typedef class Auth : public auth_t {
			private:
				/**
				 * Callback Структура функций обратного вызова
				 */
				typedef struct Callback {
					// Внешняя функция получения пароля пользователя авторизации Digest
					function <string (const string &)> extractPass;
					// Внешняя функция проверки авторизации Basic
					function <bool (const string &, const string &)> auth;
					/**
					 * Callback Конструктор
					 */
					Callback() noexcept : extractPass(nullptr), auth(nullptr) {}
				} fn_t;
			private:
				// Логин пользователя
				string _user;
				// Пароль пользователя
				string _pass;
			private:
				// Объявляем функции обратного вызова
				fn_t _callback;
				// Параметры Digest авторизации пользователя
				digest_t _userDigest;
			public:
				/**
				 * check Метод проверки авторизации
				 * @param method метод HTTP запроса
				 * @return       результат проверки авторизации
				 */
				const bool check(const string & method) noexcept;
			public:
				/**
				 * realm Метод установки название сервера
				 * @param realm название сервера
				 */
				void realm(const string & realm) noexcept;
				/**
				 * opaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(const string & opaque) noexcept;
			public:
				/**
				 * extractPassCallback Метод добавления функции извлечения пароля
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void extractPassCallback(function <string (const string &)> callback) noexcept;
				/**
				 * authCallback Метод добавления функции обработки авторизации
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void authCallback(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * header Метод установки параметров авторизации из заголовков
				 * @param header заголовок HTTP с параметрами авторизации
				 */
				void header(const string & header) noexcept;
				/**
				 * header Метод получения строки авторизации HTTP заголовка
				 * @param mode режим вывода только значения заголовка
				 * @return     строка авторизации
				 */
				const string header(const bool mode = false) noexcept;
			public:
				/**
				 * Auth Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept :
				 auth_t(fmk, log), _user(""), _pass("") {}
		} auth_t;
	};
};

#endif // __AWH_AUTH_SERVER__
