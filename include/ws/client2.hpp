/**
 * @file: client.hpp
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

#ifndef __AWH_WS_CLIENT__
#define __AWH_WS_CLIENT__

/**
 * Наши модули
 */
#include "ws.hpp"

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
	 * @brief клиентское пространство имён
	 *
	 */
	namespace client {
		/**
		 * @brief Класс для работы с клиентом WebSocket
		 *
		 */
		typedef class AWH_SHARED_EXPORT Websocket : public awh::ws_t {
			private:
				/**
				 * @brief Функция выбора типа компрессора
				 *
				 * @param compressor название компрессора в текстовом виде
				 * @return           результат работы функции
				 */
				bool matchingCompressor(const string & compressor) noexcept;
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
				 * @brief Метод извлечения параметров авторизации
				 *
				 * @return параметры модуля авторизации
				 */
				client::auth_t::settings_t authorization() const noexcept;
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param settings параметры авторизации для установки
				 */
				void authorization(const client::auth_t::settings_t & settings) noexcept;
			public:
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param user логин пользователя для авторизации на сервере
				 * @param pass пароль пользователя для авторизации на сервере
				 */
				void user(const string & user, const string & pass) noexcept;
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

#endif // __AWH_WS_CLIENT__
