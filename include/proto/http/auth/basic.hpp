/**
 * @file basic.hpp
 * @date 2026-07-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл схемы BASIC-авторизации (RFC 7617) —
 *        формирование и разбор заголовка Authorization с логином и паролем в кодировке Base64
 *
 * \~english
 * @brief Header file of the BASIC authorization scheme (RFC 7617) —
 *        the forming and the parsing of the Authorization header with a login and a password in the Base64 encoding
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_AUTH_BASIC__
#define __AWH_AUTH_BASIC__

/**
 * Подключаем заголовочный файл проекта
 */
#include "auth.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён HTTP-протокола
	 *
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Схема BASIC-авторизации (RFC 7617)
		 *
		 * \~english
		 * @brief BASIC authorization scheme (RFC 7617)
		 *
		 * \~
		 */
		typedef class Basic : public auth_t::scheme_t {
			public:
				/**
				 * \~russian
				 * @brief Метод проверки учётных данных (только для сервера)
				 *
				 * @return результат проверки
				 *
				 *
				 * \~english
				 * @brief Method of checking the credentials (for the server only)
				 * @return result of the check
				 *
				 * \~
				 */
				bool check() noexcept override;
				/**
				 * \~russian
				 * @brief Метод разбора входящего заголовка авторизации
				 *
				 * @param header значение заголовка (клиент: вызов, сервер: учётные данные)
				 * @return       результат разбора
				 *
				 *
				 * \~english
				 * @brief Method of parsing an incoming authorization header
				 * @param header value of the header (client: the challenge, server: the credentials)
				 * @return       result of the parsing
				 *
				 * \~
				 */
				bool parse(const string_view header) noexcept override;
				/**
				 * \~russian
				 * @brief Метод формирования исходящего заголовка авторизации
				 *
				 * @param full режим вывода вместе с именем заголовка
				 * @return     значение заголовка авторизации
				 *
				 *
				 * \~english
				 * @brief Method of forming an outgoing authorization header
				 * @param full mode of the output together with the name of the header
				 * @return     value of the authorization header
				 *
				 * \~
				 */
				string header(const bool full = false) noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param owner  сторона работы (клиент/сервер)
				 * @param params общие параметры авторизации
				 * @param crypto объект криптографии
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param owner  side of the work (client/server)
				 * @param params common parameters of the authorization
				 * @param crypto cryptography object
				 * @param fmk    framework object
				 * @param log    object for working with logs
				 *
				 * \~
				 */
				explicit Basic(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Basic() noexcept;
		} basic_t;
	};
};

#endif // __AWH_AUTH_BASIC__
