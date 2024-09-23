/**
 * @file: server.hpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_AUTH_SERVER__
#define __AWH_AUTH_SERVER__

/**
 * Наши модули
 */
#include <sys/fn.hpp>
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
		typedef class AWHSHARED_EXPORT Auth : public auth_t {
			public:
				/**
				 * Data Структура данных авторизации
				 */
				typedef struct Data {
					const type_t * type;     // Тип авторизации
					const digest_t * digest; // Параметры Digest авторизации
					const digest_t * locale; // Параметры Digest авторизации пользователя
					const string * user;     // Логин пользователя
					const string * pass;     // Пароль пользователя
					/**
					 * Data Конструктор
					 */
					Data() noexcept : type(nullptr), digest(nullptr), locale(nullptr), user(nullptr), pass(nullptr) {}
				} data_t;
			private:
				// Логин пользователя
				string _user;
				// Пароль пользователя
				string _pass;
			private:
				// Хранилище функций обратного вызова
				fn_t _callbacks;
				// Параметры Digest авторизации пользователя
				digest_t _locale;
			public:
				/**
				 * data Метод извлечения данных авторизации
				 * @return данные модуля авторизации
				 */
				data_t data() const noexcept;
				/**
				 * data Метод установки данных авторизации
				 * @param data данные авторизации для установки
				 */
				void data(const data_t & data) noexcept;
			public:
				/**
				 * check Метод проверки авторизации
				 * @param method метод HTTP запроса
				 * @return       результат проверки авторизации
				 */
				bool check(const string & method) noexcept;
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
			public:
				/**
				 * Оператор вывода строки авторизации
				 * @return строка авторизации
				 */
				operator std::string() noexcept;
			public:
				/**
				 * Auth Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept :
				 auth_t(fmk, log), _user{""}, _pass{""}, _callbacks(log) {}
		} auth_t;
	};
};

#endif // __AWH_AUTH_SERVER__
