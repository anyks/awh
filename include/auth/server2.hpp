/**
 * @file: server.hpp
 * @date: 2025-10-08
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_AUTH_SERVER__
#define __AWH_AUTH_SERVER__

/**
 * Наши модули
 */
#include "auth.hpp"
#include "../sys/callback.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief серверное пространство имён
	 *
	 */
	namespace server {
		/**
		 * @brief Класс работы с авторизацией на сервере
		 *
		 */
		typedef class AWH_SHARED_EXPORT Auth : public auth_t {
			public:
				/**
				 * @brief Структура данных авторизации
				 *
				 */
				typedef struct Settings {
					const char * user;       // Логин пользователя
					const char * pass;       // Пароль пользователя
					const type_t * type;     // Тип авторизации
					const digest_t * digest; // Параметры Digest авторизации
					const digest_t * locale; // Параметры Digest авторизации пользователя
					/**
					 * @brief Конструктор
					 *
					 */
					Settings() noexcept :
					 user(nullptr), pass(nullptr),
					 type(nullptr), digest(nullptr), locale(nullptr) {}
				} __attribute__((packed)) settings_t;
			private:
				// Логин пользователя
				string _user;
				// Пароль пользователя
				string _pass;
			private:
				// Параметры Digest авторизации пользователя
				digest_t _locale;
				// Хранилище функций обратного вызова
				callback_t _callback;
			public:
				/**
				 * @brief Метод проверки авторизации
				 *
				 * @param method метод HTTP запроса
				 * @return       результат проверки авторизации
				 */
				bool check(const string & method) noexcept;
			public:
				/**
				 * @brief Метод установки название сервера
				 *
				 * @param realm название сервера
				 */
				void realm(const string & realm) noexcept;
				/**
				 * @brief Метод установки временного ключа сессии сервера
				 *
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(const string & opaque) noexcept;
			public:
				/**
				 * @brief Метод добавления функции извлечения пароля
				 *
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void callback(function <string (const string &)> && callback) noexcept;
				/**
				 * @brief Метод добавления функции обработки авторизации
				 *
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void callback(function <bool (const string &, const string &)> && callback) noexcept;
			public:
				/**
				 * @brief Метод установки параметров авторизации из заголовков
				 *
				 * @param header заголовок HTTP с параметрами авторизации
				 */
				void header(const string & header) noexcept;
			public:
				/**
				 * @brief Метод извлечения параметров авторизации
				 *
				 * @return параметры модуля авторизации
				 */
				settings_t settings() const noexcept;
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param settings параметры авторизации для установки
				 */
				void settings(const settings_t & settings) noexcept;
			public:
				/**
				 * @brief Оператор вывода строки авторизации
				 *
				 * @return строка авторизации
				 */
				operator string() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 * 
				 */
				~Auth() noexcept {}
		} auth_t;
	};
};

#endif // __AWH_AUTH_SERVER__
