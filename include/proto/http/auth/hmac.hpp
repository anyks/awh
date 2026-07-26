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
 * @copyright: Copyright © 2026
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
		 * @brief Схема авторизации подписью запроса HMAC (RFC 9421, HTTP Message Signatures)
		 *
		 * @details Схема формирует и проверяет пару заголовков Signature-Input и Signature.
		 *          Подпись рассчитывается методом HMAC над канонической базой подписи,
		 *          собранной из покрываемых компонентов запроса и параметров подписи.
		 */
		typedef class Hmac : public auth_t::scheme_t {
			private:
				/**
				 * @brief Метод получения текстового имени алгоритма подписи
				 *
				 * @return название алгоритма подписи (hmac-sha256 и т.д.)
				 */
				string algName() const noexcept;
				/**
				 * @brief Метод получения значения покрываемого компонента по имени
				 *
				 * @param name имя компонента
				 * @return     значение компонента (пустая строка, если не найден)
				 */
				string value(string_view name) const noexcept;
			private:
				/**
				 * @brief Метод формирования значения параметров подписи (@signature-params)
				 *
				 * @return значение параметров подписи
				 */
				string params() noexcept;
				/**
				 * @brief Метод формирования канонической базы подписи
				 *
				 * @param params значение параметров подписи (@signature-params)
				 * @return       каноническая база подписи
				 */
				string base(const string & params) const noexcept;
				/**
				 * @brief Метод расчёта BASE64-подписи по канонической базе
				 *
				 * @param base каноническая база подписи
				 * @param key  секретный ключ подписи
				 * @return     подпись в формате BASE64
				 */
				string sign(const string & base, const string & key) const noexcept;
			private:
				/**
				 * @brief Метод формирования заголовков подписи (клиент)
				 *
				 * @param input     ссылка для записи значения заголовка Signature-Input
				 * @param signature ссылка для записи значения заголовка Signature
				 * @return          результат формирования
				 */
				bool build(string & input, string & signature) noexcept;
			public:
				/**
				 * @brief Метод разбора входящего заголовка авторизации
				 *
				 * @param header значение заголовка (Signature-Input либо Signature)
				 * @return       результат разбора
				 */
				bool parse(const string_view header) noexcept override;
				/**
				 * @brief Метод разбора входящего заголовка авторизации с указанием имени
				 *
				 * @param name   имя входящего заголовка (Signature-Input либо Signature)
				 * @param header значение входящего заголовка
				 * @return       результат разбора
				 */
				bool parse(const string_view name, const string_view header) noexcept override;
			public:
				/**
				 * @brief Метод проверки учётных данных (только для сервера)
				 *
				 * @return результат проверки
				 */
				bool check() noexcept override;
				/**
				 * @brief Метод формирования исходящего заголовка авторизации
				 *
				 * @details При full = false возвращается значение заголовка Signature,
				 *          при full = true возвращаются оба заголовка (Signature-Input и Signature)
				 *
				 * @param full режим вывода вместе с именами заголовков
				 * @return     значение заголовка (заголовков) авторизации
				 */
				string header(const bool full = false) noexcept override;
				/**
				 * @brief Метод формирования набора исходящих заголовков авторизации
				 *
				 * @param result контейнер для набора заголовков (Signature-Input и Signature)
				 */
				void headers(vector <pair <string, string>> & result) noexcept override;
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
				explicit Hmac(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Hmac() noexcept;
		} hmac_t;
	};
};

#endif // __AWH_AUTH_HMAC__
