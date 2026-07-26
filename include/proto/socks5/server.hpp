/**
 * @file: server.hpp
 * @date: 2026-05-24
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
#ifndef __AWH_PROTO_SOCKS5_SERVER__
#define __AWH_PROTO_SOCKS5_SERVER__

/**
 * Подключаем заголовочный файл проекта
 */
#include "socks5.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён протоколов
	 *
	 */
	namespace proto {
		/**
		 * @brief Класс сервера для работы с socks5 прокси
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Server_Socks5 : public socks5_t {
			private:
				/**
				 * @brief Внешняя функция проверки авторизации
				 *
				 * @details Если установлена - парсер сам проверяет авторизацию и отдаёт
				 *          результат проверки через функцию обратного вызова. Если не установлена -
				 *          авторизация не проверяется (по умолчанию).
				 *
				 * @param login    логин пользователя
				 * @param password пароль пользователя
				 * @return         результат проверки авторизации (true - авторизация успешна, false - авторизация неуспешна)
				 */
				function <bool (const string &, const string &)> _callback;
			public:
				/**
				 * @brief Метод парсинга входящих данных
				 *
				 * @param buffer бинарный буфер входящих данных
				 * @param size   размер бинарного буфера входящих данных
				 * @param ctx    объект для извлечения параметров сообщения
				 * @return       результат парсинга входящих данных
				 */
				bool parse(const void * buffer, const size_t size, ctx_t & ctx) noexcept;
				/**
				 * @brief Метод парсинга входящих данных
				 *
				 * @param buffer бинарный буфер входящих данных
				 * @param size   размер бинарного буфера входящих данных
				 * @param udp    объект для извлечения параметров UDP заголовка
				 * @return       результат парсинга входящих данных
				 */
				bool parse(const void * buffer, const size_t size, udp_head_t & udp) noexcept;
			public:
				/**
				 * @brief Метод извлечения буфера запроса/ответа
				 *
				 * @param buffer указатель на буфер для извлечения данных
				 * @param size   ссылка на размер буфера для извлечения данных
				 * @param ctx    объект для установки параметров сообщения
				 * @return 	     результат извлечения данных в буфер
				 */
				bool buffer(uint8_t ** buffer, size_t & size, ctx_t & ctx) const noexcept;
				/**
				 * @brief Метод извлечения буфера запроса/ответа
				 *
				 * @param buffer указатель на буфер для извлечения данных
				 * @param size   ссылка на размер буфера для извлечения данных
				 * @param udp    объект для установки параметров UDP заголовка
				 * @return 	     результат извлечения данных в буфер
				 */
				bool buffer(uint8_t ** buffer, size_t & size, const udp_head_t & udp) const noexcept;
			public:
				/**
				 * @brief Метод добавления функции обработки авторизации
				 *
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void on(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Server_Socks5(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Server_Socks5() noexcept;
		} server_socks5_t;
	};
};

#endif // __AWH_PROTO_SOCKS5_SERVER__
