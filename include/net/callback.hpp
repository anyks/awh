/**
 * @file: callback.hpp
 * @date: 2026-03-10
 * @license: GPL-3.0
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
#ifndef __AWH_ENGINE_CALLBACK__
#define __AWH_ENGINE_CALLBACK__

/**
 * Стандартные модули
 */
#include <functional>

/**
 * Наши модули
 */
#include "net.hpp"
#include "event.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Пространство имён движков ввода-вывода
	 *
	 */
	namespace engine {
		/**
		 * @brief пространство имён работы с обратными вызовами
		 *
		 */
		namespace callback {
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				/**
				 * @brief Пространство имён для работы с SCTP
				 *
				 */
				namespace sctp {
					/**
					 * @brief Функция обратного вызова срабатывающая при получении информационных сообщений SCTP
					 *
					 */
					using minfo_t = function <void (const event::id_t, const net::sctp::minfo_t &)>;
					/**
					 * @brief Функция обратного вызова срабатывающая при получении событий SCTP
					 *
					 */
					using events_t = function <void (const event::id_t, unique_ptr <net::sctp::event_t>)>;
				};
			#endif
			/**
			 * @brief Функция обратного вызова срабатывающая при подключении события
			 *
			 */
			using connect_t = function <void (const event::id_t, const bool)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при записи в событие
			 *
			 */
			using write_t = function <void (const event::id_t, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при принятии события
			 *
			 */
			using accept_t = function <void (const event::id_t, const event::id_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при общем событии
			 *
			 */
			using event_t = function <void (const event::id_t, const event::action_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при изменении статуса события
			 *
			 */
			using status_t = function <void (const event::id_t, const event::status_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при чтении из события
			 *
			 */
			using read_t = function <void (const event::id_t, const uint8_t *, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при ошибке события
			 *
			 */
			using error_t = function <void (const event::id_t, const event::error_t, const string &)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при доступности очереди на отправку данных
			 *
			 */
			using available_t = function <void (const event::id_t, const event::status_t, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при истечении таймера события
			 *
			 */
			using timeout_t = function <bool (const event::id_t, const event::action_t, const uint32_t)>;
			/**
			 * @brief Функция обратного вызова возвращающая неотправленные данные события
			 *
			 */
			using spool_t = function <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при изменении каталога
			 *
			 */
			using vnode_t = function <void (const event::id_t, const event::action_t, const event::vnode_t, const string &)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при получении информации о пакетах в туннельном интерфейсе
			 *
			 */
			using tuninfo_t = function <void (const event::id_t, const event::id_t, const event::action_t, const net::tun_info_t &)>;
		};
	};
};

#endif // __AWH_ENGINE_CALLBACK__
