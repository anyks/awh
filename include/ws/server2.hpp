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

#ifndef __AWH_WS_SERVER__
#define __AWH_WS_SERVER__

/**
 * Наши модули
 */
#include "ws.hpp"
#include "../auth/server.hpp"

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
		 * @brief Класс для работы с сервером WebSocket
		 *
		 */
		typedef class AWH_SHARED_EXPORT Websocket : public awh::ws_t {
			public:
				/**
				 * @brief Метод применения полученных результатов
				 *
				 */
				void commit() noexcept;
			public:
				/**
				 * @brief Метод проверки выполнения рукопожатия
				 *
				 * @return результат выполнения рукопожатия
				 */
				handshake_t handshake() noexcept;
			public:
				/**
				 * @brief Метод проверки шагов рукопожатия
				 *
				 * @param step флаг выполнения проверки
				 * @return     результат проверки соответствия
				 */
				bool step(const step_t step) noexcept;
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
				 * @brief Метод извлечения данных авторизации
				 *
				 * @return данные модуля авторизации
				 */
				server::auth_t::data_t authorization() const noexcept;
				/**
				 * @brief Метод установки данных авторизации
				 *
				 * @param data данные авторизации для установки
				 */
				void authorization(const server::auth_t::data_t & data) noexcept;
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
				Websocket(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Websocket() noexcept {}
		} ws_t;
	};
};

#endif // __AWH_WS_SERVER__
