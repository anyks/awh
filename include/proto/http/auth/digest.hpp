/**
 * @file digest.hpp
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
 * @brief Заголовочный файл схемы DIGEST-авторизации (RFC 7616) — расчёт и проверка дайджеста по nonce, cnonce,
 *        nc и qop с контролем срока жизни nonce и защитой от повторного использования
 *
 * \~english
 * @brief Header file of the DIGEST authorization scheme (RFC 7616) — the computation and the check of the digest by the nonce, the cnonce,
 *        the nc and the qop with a control of the lifetime of the nonce and with a protection from a repeated use
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
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
		 * @brief Схема DIGEST-авторизации (RFC 7616)
		 *
		 * \~english
		 * @brief DIGEST authorization scheme (RFC 7616)
		 *
		 * \~
		 */
		typedef class Digest : public auth_t::scheme_t {
			private:
				/**
				 * \~russian
				 * @brief Метод перевода алгоритма хэширования в текстовое представление
				 *
				 * @return название алгоритма хэширования (MD5, SHA-256 и т.д.)
				 *
				 * \~english
				 * @brief Method of converting the hashing algorithm into a text representation
				 * @return name of the hashing algorithm (MD5, SHA-256 and so on)
				 *
				 * \~
				 */
				string algorithm() const noexcept;
				/**
				 * \~russian
				 * @brief Метод расчёта ответа Digest-авторизации
				 *
				 * @param user логин пользователя
				 * @param pass пароль пользователя
				 * @return     ответ в шестнадцатеричном виде
				 *
				 * \~english
				 * @brief Method of computing the response of the Digest authorization
				 * @param user login of the user
				 * @param pass password of the user
				 * @return     response in the hexadecimal form
				 *
				 * \~
				 */
				string response(const string & user, const string & pass) const noexcept;
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
				explicit Digest(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept;
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
				~Digest() noexcept;
		} digest_scheme_t;
	};
};

#endif // __AWH_AUTH_DIGEST__
