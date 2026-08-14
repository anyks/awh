/**
 * @file: hmac.hpp
 * @date: 2026-07-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл схемы HMAC-авторизации HTTP-сообщений — расчёт и проверка подписи над канонической базой,
 *        собранной из покрываемых компонентов запроса, с формированием заголовков Signature и Signature-Input
 *
 * \~english
 * @brief Header file of the HMAC authorization scheme of the HTTP messages — the computation and the check of the signature over the canonical base,
 *        assembled from the covered components of the request, with the forming of the Signature and Signature-Input headers
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_AUTH_HMAC__
#define __AWH_AUTH_HMAC__

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
		 * @brief Схема авторизации подписью запроса HMAC (RFC 9421, HTTP Message Signatures)
		 *
		 * @details Схема формирует и проверяет пару заголовков Signature-Input и Signature.
		 *          Подпись рассчитывается методом HMAC над канонической базой подписи,
		 *          собранной из покрываемых компонентов запроса и параметров подписи.
		 *
		 * \~english
		 * @brief Authorization scheme by an HMAC signature of a request (RFC 9421, HTTP Message Signatures)
		 * @details The scheme forms and checks the pair of the Signature-Input and Signature headers.
		 *          The signature is computed by the HMAC method over the canonical base of the signature,
		 *          assembled from the covered components of the request and from the parameters of the signature.
		 *
		 * \~
		 */
		typedef class Hmac : public auth_t::scheme_t {
			private:
				/**
				 * \~russian
				 * @brief Метод получения текстового имени алгоритма подписи
				 *
				 * @return название алгоритма подписи (hmac-sha256 и т.д.)
				 *
				 * \~english
				 * @brief Method of getting the text name of the signature algorithm
				 * @return name of the signature algorithm (hmac-sha256 and so on)
				 *
				 * \~
				 */
				string algName() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения значения покрываемого компонента по имени
				 *
				 * @param name имя компонента
				 * @return     значение компонента (пустая строка, если не найден)
				 *
				 * \~english
				 * @brief Method of getting the value of a covered component by a name
				 * @param name name of the component
				 * @return     value of the component (an empty string if it has not been found)
				 *
				 * \~
				 */
				string value(string_view name) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод формирования значения параметров подписи (@signature-params)
				 *
				 * @return значение параметров подписи
				 *
				 * \~english
				 * @brief Method of forming the value of the parameters of the signature (@signature-params)
				 * @return value of the parameters of the signature
				 *
				 * \~
				 */
				string params() noexcept;
				/**
				 * \~russian
				 * @brief Метод формирования канонической базы подписи
				 *
				 * @param params значение параметров подписи (@signature-params)
				 * @return       каноническая база подписи
				 *
				 * \~english
				 * @brief Method of forming the canonical base of the signature
				 * @param params value of the parameters of the signature (@signature-params)
				 * @return       canonical base of the signature
				 *
				 * \~
				 */
				string base(const string & params) const noexcept;
				/**
				 * \~russian
				 * @brief Метод расчёта BASE64-подписи по канонической базе
				 *
				 * @param base каноническая база подписи
				 * @param key  секретный ключ подписи
				 * @return     подпись в формате BASE64
				 *
				 * \~english
				 * @brief Method of computing the BASE64 signature over the canonical base
				 * @param base canonical base of the signature
				 * @param key  secret key of the signature
				 * @return     signature in the BASE64 format
				 *
				 * \~
				 */
				string sign(const string & base, const string & key) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод формирования заголовков подписи (клиент)
				 *
				 * @param input     ссылка для записи значения заголовка Signature-Input
				 * @param signature ссылка для записи значения заголовка Signature
				 * @return          результат формирования
				 *
				 * \~english
				 * @brief Method of forming the headers of the signature (the client)
				 * @param input     reference for writing the value of the Signature-Input header
				 * @param signature reference for writing the value of the Signature header
				 * @return          result of the forming
				 *
				 * \~
				 */
				bool build(string & input, string & signature) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора входящего заголовка авторизации
				 *
				 * @param header значение заголовка (Signature-Input либо Signature)
				 * @return       результат разбора
				 *
				 * \~english
				 * @brief Method of parsing an incoming authorization header
				 * @param header value of the header (Signature-Input or Signature)
				 * @return       result of the parsing
				 *
				 * \~
				 */
				bool parse(const string_view header) noexcept override;
				/**
				 * \~russian
				 * @brief Метод разбора входящего заголовка авторизации с указанием имени
				 *
				 * @param name   имя входящего заголовка (Signature-Input либо Signature)
				 * @param header значение входящего заголовка
				 * @return       результат разбора
				 *
				 * \~english
				 * @brief Method of parsing an incoming authorization header with an indication of the name
				 * @param name   name of the incoming header (Signature-Input or Signature)
				 * @param header value of the incoming header
				 * @return       result of the parsing
				 *
				 * \~
				 */
				bool parse(const string_view name, const string_view header) noexcept override;
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
				 * @brief Метод формирования исходящего заголовка авторизации
				 *
				 * @details При full = false возвращается значение заголовка Signature,
				 *          при full = true возвращаются оба заголовка (Signature-Input и Signature)
				 *
				 * @param full режим вывода вместе с именами заголовков
				 * @return     значение заголовка (заголовков) авторизации
				 *
				 * \~english
				 * @brief Method of forming an outgoing authorization header
				 * @details At full = false the value of the Signature header is returned,
				 *          at full = true both headers are returned (Signature-Input and Signature)
				 * @param full mode of the output together with the names of the headers
				 * @return     value of the authorization header (headers)
				 *
				 * \~
				 */
				string header(const bool full = false) noexcept override;
				/**
				 * \~russian
				 * @brief Метод формирования набора исходящих заголовков авторизации
				 *
				 * @param result контейнер для набора заголовков (Signature-Input и Signature)
				 *
				 * \~english
				 * @brief Method of forming the set of the outgoing authorization headers
				 * @param result container for the set of the headers (Signature-Input and Signature)
				 *
				 * \~
				 */
				void headers(vector <pair <string, string>> & result) noexcept override;
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
				explicit Hmac(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept;
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
				~Hmac() noexcept;
		} hmac_t;
	};
};

#endif // __AWH_AUTH_HMAC__
