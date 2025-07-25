/**
 * @file: server.hpp
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

#ifndef __AWH_WS_CORE_SERVER__
#define __AWH_WS_CORE_SERVER__

/**
 * Наши модули
 */
#include "core.hpp"
#include "../auth/server.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * WS Класс для работы с сервером WebSocket
		 */
		typedef class AWHSHARED_EXPORT WS : public ws_core_t {
			public:
				/**
				 * commit Метод применения полученных результатов
				 */
				void commit() noexcept;
			public:
				/**
				 * status Метод проверки текущего статуса
				 * @return результат проверки текущего статуса
				 */
				status_t status() noexcept;
			public:
				/**
				 * check Метод проверки шагов рукопожатия
				 * @param flag флаг выполнения проверки
				 * @return     результат проверки соответствия
				 */
				bool check(const flag_t flag) noexcept;
			public:
				/**
				 * realm Метод установки название сервера
				 * @param realm название сервера
				 */
				void realm(const string & realm) noexcept;
				/**
				 * opaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(const string & opaque) noexcept;
			public:
				/**
				 * dataAuth Метод извлечения данных авторизации
				 * @return данные модуля авторизации
				 */
				server::auth_t::data_t dataAuth() const noexcept;
				/**
				 * dataAuth Метод установки данных авторизации
				 * @param data данные авторизации для установки
				 */
				void dataAuth(const server::auth_t::data_t & data) noexcept;
			public:
				/**
				 * extractPassCallback Метод добавления функции извлечения пароля
				 * @param callback функция обратного вызова для извлечения пароля
				 */
				void extractPassCallback(function <string (const string &)> callback) noexcept;
				/**
				 * authCallback Метод добавления функции обработки авторизации
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void authCallback(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const awh::auth_t::type_t type = awh::auth_t::type_t::BASIC, const awh::auth_t::hash_t hash = awh::auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * WS Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				WS(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~WS Деструктор
				 */
				~WS() noexcept {}
		} ws_t;
	};
};

#endif // __AWH_WS_CORE_SERVER__
