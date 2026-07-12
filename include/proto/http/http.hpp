/**
 * @file: http.hpp
 * @date: 2026-07-08
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

#ifndef __AWH_HTTP__
#define __AWH_HTTP__

/**
 * Стандартная библиотека
 */
#include <cstdint>

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
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Направление трафика (запрос/ответ)
		 *
		 */
		enum class traffic_t : uint8_t {
			NONE     = 0x00, // Нет направления (не определено)
			REQUEST  = 0x01, // Запрос клиента
			RESPONSE = 0x02  // Ответ сервера
		};
		
		/**
		 * @brief Версии HTTP-протоколов
		 *
		 */
		enum class version_t : uint8_t {
			NONE    = 0x00, // Версия протокола не определена
			HTTP1_0 = 0x01, // Версия протокола HTTP/1.0
			HTTP1_1 = 0x02, // Версия протокола HTTP/1.1
			HTTP2   = 0x03, // Версия протокола HTTP/2
			HTTP3   = 0x04, // Версия протокола HTTP/3
			HTTP4   = 0x05, // Версия протокола HTTP/4
			HTTP5   = 0x06  // Версия протокола HTTP/5
		};

		/**
		 * @brief Методы HTTP-запроса
		 *
		 */
		enum class method_t : uint8_t {
			NONE    = 0x00, // Метод не установлен
			GET     = 0x01, // Метод GET
			PUT     = 0x02, // Метод PUT
			DEL     = 0x03, // Метод DELETE
			POST    = 0x04, // Метод POST
			HEAD    = 0x05, // Метод HEAD
			PATCH   = 0x06, // Метод PATCH
			TRACE   = 0x07, // Метод TRACE
			OPTIONS = 0x08, // Метод OPTIONS
			CONNECT = 0x09  // Метод CONNECT
		};

		/**
		 * @brief Версии HTTP-протоколов соответствия
		 *
		 */
		enum class proto_t : uint8_t {
			NONE       = 0x00, // Протокол не установлен
			UNKNOWN    = 0x01, // Протокол неизвестный
			HTTP1      = 0x02, // Протокол принадлежит HTTP/1.1
			HTTP2      = 0x03, // Протокол принадлежит HTTP/2
			HTTP3      = 0x04, // Протокол принадлежит HTTP/3
			PROXY1     = 0x05, // Протокол принадлежит Proxy over HTTP/1.1
			PROXY2     = 0x06, // Протокол принадлежит Proxy over HTTP/2
			PROXY3     = 0x07, // Протокол принадлежит Proxy over HTTP/3
			WEBSOCKET1 = 0x08, // Протокол принадлежит Websocket over HTTP/1.1
			WEBSOCKET2 = 0x09, // Протокол принадлежит Websocket over HTTP/2
			WEBSOCKET3 = 0x0A  // Протокол принадлежит Websocket over HTTP/3
		};
	};
};

#endif // __AWH_HTTP__
