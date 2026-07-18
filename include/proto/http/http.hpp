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
 * Стандартный заголовочный файл
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
		enum class direct_t : uint8_t {
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
		 * @brief Распознанный HTTP-метод
		 *
		 * Помимо основных методов HTTP/1.1 распознаются методы WebDAV (RFC 4918)
		 * и ряд расширений (RFC 5789, UPnP, ICAL и др.). Любой нераспознанный, но
		 * синтаксически корректный метод даёт UNKNOWN, при этом Message::methodName
		 * всегда содержит оригинальную строку.
		 */
		enum class method_t : uint8_t {
			// Метод не установлен
			NONE = 0x00,
			/**
			 * Основные (RFC 7231) + PATCH (RFC 5789)
			 */
			GET     = 0x01, // Метод GET (RFC 7231)
			PUT     = 0x02, // Метод PUT (RFC 7231)
			DEL     = 0x03, // Метод DELETE (RFC 7231)
			POST    = 0x04, // Метод POST (RFC 7231)
			HEAD    = 0x05, // Метод HEAD (RFC 7231)
			PATCH   = 0x06, // Метод PATCH (RFC 5789)
			TRACE   = 0x07, // Метод TRACE (RFC 7231)
			OPTIONS = 0x08, // Метод OPTIONS (RFC 7231)
			CONNECT = 0x09, // Метод CONNECT (RFC 7231)
			/**
			 * WebDAV (RFC 4918) и расширения версионирования (RFC 3253)
			 */
			ACL        = 0x0A, // Метод ACL (WebDAV)
			COPY       = 0x0B, // Метод COPY (WebDAV)
			LOCK       = 0x0C, // Метод LOCK (WebDAV)
			MOVE       = 0x0D, // Метод MOVE (WebDAV)
			BIND       = 0x0E, // Метод BIND (WebDAV)
			MKCOL      = 0x0F, // Метод MKCOL (WebDAV)
			MERGE      = 0x10, // Метод MERGE (WebDAV)
			REPORT     = 0x11, // Метод REPORT (WebDAV)
			SEARCH     = 0x12, // Метод SEARCH (WebDAV)
			UNLOCK     = 0x13, // Метод UNLOCK (WebDAV)
			REBIND     = 0x14, // Метод REBIND (WebDAV)
			UNBIND     = 0x15, // Метод UNBIND (WebDAV)
			CHECKOUT   = 0x16, // Метод CHECKOUT (WebDAV)
			PROPFIND   = 0x17, // Метод PROPFIND (WebDAV)
			PROPPATCH  = 0x18, // Метод PROPPATCH (WebDAV)
			MKACTIVITY = 0x19, // Метод MKACTIVITY (WebDAV)
			/**
			 * Прочие распространённые расширения
			 */
			PRI         = 0x1A, // Метод PRI (HTTP/2)
			LINK        = 0x1B, // Метод LINK (RFC 2068)
			PURGE       = 0x1C, // Метод PURGE (Varnish)
			NOTIFY      = 0x1D, // Метод NOTIFY (UPnP)
			UNLINK      = 0x1E, // Метод UNLINK (RFC 2068)
			SOURCE      = 0x1F, // Метод SOURCE (RFC 2068)
			MSEARCH     = 0x20, // Метод MSEARCH (UPnP)
			SUBSCRIBE   = 0x21, // Метод SUBSCRIBE (UPnP)
			UNSUBSCRIBE = 0x22, // Метод UNSUBSCRIBE (UPnP)
			MKCALENDAR  = 0x23  // Метод MKCALENDAR (RFC 4791)
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
