/**
 * @file http.hpp
 * @date 2026-07-08
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
 * @brief Заголовочный файл общих типов HTTP-протокола — перечисления направления обмена, версий протокола,
 *        поддерживаемых прикладных протоколов и методов запроса, общие для парсеров HTTP/1.x,
 *        HTTP/2 и провайдеров сообщений
 *
 * \~english
 * @brief Header file of the common types of the HTTP protocol — the enumerations of the direction of the exchange, of the versions of the protocol,
 *        of the supported application protocols and of the request methods, common to the parsers of HTTP/1.x,
 *        of HTTP/2 and to the providers of the messages
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_HTTP__
#define __AWH_HTTP__

/**
 * Стандартный заголовочный файл
 */
#include <cstdint>

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
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
		 * @brief Направление трафика (запрос/ответ)
		 *
		 * \~english
		 * @brief Direction of the traffic (request/response)
		 *
		 * \~
		 */
		enum class direct_t : uint8_t {
			NONE     = 0x00, // Нет направления (не определено)
			REQUEST  = 0x01, // Запрос клиента
			RESPONSE = 0x02  // Ответ сервера
		};

		/**
		 * \~russian
		 * @brief Версии HTTP-протоколов
		 *
		 * \~english
		 * @brief Versions of the HTTP protocols
		 *
		 * \~
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
		 * \~russian
		 * @brief Версии HTTP-протоколов соответствия
		 *
		 * \~english
		 * @brief Versions of the HTTP protocols of the correspondence
		 *
		 * \~
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

		/**
		 * \~russian
		 * @brief Распознанный HTTP-метод
		 *
		 * @details Помимо основных методов HTTP/1.1 распознаются методы WebDAV (RFC 4918)
		 *          и ряд расширений (RFC 5789, UPnP, ICAL и др.). Любой нераспознанный, но
		 *          синтаксически корректный метод даёт UNKNOWN, при этом оригинальное
		 *          написание метода сохраняется в поле request_t::methodName (прозрачное
		 *          проксирование экзотических методов).
		 *
		 * \~english
		 * @brief Recognized HTTP method
		 * @details Besides the main methods of HTTP/1.1 the WebDAV methods (RFC 4918)
		 *          and a number of the extensions (RFC 5789, UPnP, ICAL and others) are recognized. Any unrecognized but
		 *          syntactically correct method gives UNKNOWN, while the original
		 *          spelling of the method is preserved in the request_t::methodName field (a transparent
		 *          proxying of the exotic methods).
		 *
		 * \~
		 */
		enum class method_t : uint8_t {
			// Метод не установлен
			NONE = 0x00,
			/**
			 * Основные (RFC 7231) + PATCH (RFC 5789)
			 */
			GET     = 0x01, // Метод GET (RFC 7231)
			PUT     = 0x02, // Метод PUT (RFC 7231)
			POST    = 0x03, // Метод POST (RFC 7231)
			HEAD    = 0x04, // Метод HEAD (RFC 7231)
			PATCH   = 0x05, // Метод PATCH (RFC 5789)
			TRACE   = 0x06, // Метод TRACE (RFC 7231)
			DELETE  = 0x07, // Метод DELETE (RFC 7231)
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
			MKCALENDAR  = 0x22, // Метод MKCALENDAR (RFC 4791)
			UNSUBSCRIBE = 0x23, // Метод UNSUBSCRIBE (UPnP)
			/**
			 * Нераспознанные методы
			 */
			UNKNOWN = 0x24  // Синтаксически корректный, но нераспознанный метод (оригинал в request_t::methodName)
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_HTTP__
