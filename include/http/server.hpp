/**
 * @file: server.hpp
 * @date: 2025-10-07
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

#ifndef __AWH_HTTP_SERVER__
#define __AWH_HTTP_SERVER__

/**
 * Наши модули
 */
#include "http.hpp"

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
		 * @brief Класс для работы сервера HTTP-протокола
		 *
		 */
		typedef class AWH_SHARED_EXPORT Http : public awh::http_t {
			private:
				/**
				 * @brief Метод проверки выполнения рукопожатия
				 *
				 * @return результат выполнения рукопожатия
				 */
				handshake_t handshake() noexcept;
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
				 * @brief Метод извлечения параметров авторизации
				 *
				 * @return параметры модуля авторизации
				 */
				server::auth_t::settings_t authorization() const noexcept;
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param settings параметры авторизации для установки
				 */
				void authorization(const server::auth_t::settings_t & settings) noexcept;
			public:
				/**
				 * @brief Метод добавления функции извлечения пароля
				 *
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void authCallback(function <string (const string &)> callback) noexcept;
				/**
				 * @brief Метод добавления функции обработки авторизации
				 *
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void authCallback(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * @brief Метод установки типа авторизации
				 *
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Http(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Http() noexcept {}
		} http_t;
	};
};

#endif // __AWH_HTTP_SERVER__
