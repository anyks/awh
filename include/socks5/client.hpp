/**
 * @file: client.hpp
 * @date: 2021-12-19
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

#ifndef __AWH_SOCKS5_CLIENT__
#define __AWH_SOCKS5_CLIENT__

/**
 * Наши модули
 */
#include "core.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief клиентское пространство имён
	 *
	 */
	namespace client {
		/**
		 * @brief Класс клиента для работы с socks5 прокси-сервером
		 *
		 */
		typedef class AWHSHARED_EXPORT Socks5 : public awh::socks5_t {
			private:
				// Пароль пользователя
				string _pass;
				// Логин пользователя
				string _login;
			private:
				/**
				 * @brief Метод получения бинарного буфера запроса
				 *
				 */
				void cmd() const noexcept;
				/**
				 * @brief Метод получения бинарного буфера авторизации на сервере
				 *
				 */
				void auth() const noexcept;
				/**
				 * @brief Метод получения бинарного буфера опроса методов подключения
				 *
				 */
				void methods() const noexcept;
			public:
				/**
				 * @brief Метод парсинга входящих данных
				 *
				 * @param buffer бинарный буфер входящих данных
				 * @param size   размер бинарного буфера входящих данных
				 */
				void parse(const char * buffer = nullptr, const size_t size = 0) noexcept;
			public:
				/**
				 * @brief Метод сброса собранных данных
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * @brief Метод очистки списка пользователей
				 *
				 */
				void clearUser() noexcept;
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param login логин пользователя для авторизации на сервере
				 * @param pass  пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & pass) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param log объект для работы с логами
				 */
				Socks5(const log_t * log) noexcept : awh::socks5_t(log), _pass{""}, _login{""} {}
				/**
				 * @brief Деструктор
				 *
				 */
				~Socks5() noexcept {}
		} socks5_t;
	};
};

#endif // __AWH_SOCKS5_CLIENT__
