/**
 * @file: digest.hpp
 * @date: 2026-07-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_AUTH_DIGEST__
#define __AWH_AUTH_DIGEST__

/**
 * Подключаем заголовочный файл проекта
 */
#include "auth.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Схема DIGEST-авторизации (RFC 7616)
		 *
		 */
		typedef class Digest : public auth_t::scheme_t {
			private:
				/**
				 * @brief Метод перевода алгоритма хэширования в текстовое представление
				 *
				 * @return название алгоритма хэширования (MD5, SHA-256 и т.д.)
				 */
				string algorithm() const noexcept;
				/**
				 * @brief Метод расчёта ответа Digest-авторизации
				 *
				 * @param user логин пользователя
				 * @param pass пароль пользователя
				 * @return     ответ в шестнадцатеричном виде
				 */
				string response(const string & user, const string & pass) const noexcept;
			public:
				/**
				 * @brief Метод проверки учётных данных (только для сервера)
				 *
				 * @return результат проверки
				 */
				bool check() noexcept override;
				/**
				 * @brief Метод разбора входящего заголовка авторизации
				 *
				 * @param header значение заголовка (клиент: вызов, сервер: учётные данные)
				 * @return       результат разбора
				 */
				bool parse(const string_view header) noexcept override;
				/**
				 * @brief Метод формирования исходящего заголовка авторизации
				 *
				 * @param full режим вывода вместе с именем заголовка
				 * @return     значение заголовка авторизации
				 */
				string header(const bool full = false) noexcept override;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param owner  сторона работы (клиент/сервер)
				 * @param params общие параметры авторизации
				 * @param crypto объект криптографии
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 */
				explicit Digest(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Digest() noexcept;
		} digest_scheme_t;
	};
};

#endif // __AWH_AUTH_DIGEST__
