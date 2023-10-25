/**
 * @file: errors.hpp
 * @date: 2023-09-27
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_WEB_ERRORS__
#define __AWH_WEB_ERRORS__

/**
 * Стандартная библиотека
 */
#include <cstdint>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * http пространство имён
	 */
	namespace http {
		/**
		 * Коды ошибок клиента
		 */
		enum class error_t : uint8_t {
			NONE                      = 0x00, // Ошибка не установлена
			PROTOCOL                  = 0x01, // Ошибка работы протокола
			WEBSOCKET                 = 0x02, // Ошибка WebSocket-клиента
			HTTP1_SEND                = 0x03, // Ошибка отправки сообщения на протокол HTTP/1.1
			HTTP1_RECV                = 0x04, // Ошибка чтения данных из протокола HTTP/1.1
			HTTP2_SEND                = 0x05, // Ошибка отправки сообщения на протокол HTTP/2
			HTTP2_PING                = 0x06, // Ошибка выполнения пинга на протокол HTTP/2
			HTTP2_RECV                = 0x07, // Ошибка чтения данных из протокола HTTP/2
			HTTP2_CANCEL              = 0x08, // Ошибка подключения, произведена отмена передачи данных по протоколу HTTP/2
			HTTP2_SUBMIT              = 0x09, // Ошибка предоставления данных для протокола HTTP/2
			HTTP2_CONNECT             = 0x0A, // Ошибка выполнения подключения по протоколу HTTP/2
			HTTP2_PROTOCOL            = 0x0B, // Ошибка протокола HTTP/2
			HTTP2_INTERNAL            = 0x0C, // Ошибка внутренняя у протокола HTTP/2
			HTTP2_SETTINGS            = 0x0D, // Ошибка установки настроек для протокола HTTP/2
			HTTP2_PIPE_INIT           = 0x0E, // Ошибка инициализации PIPE для протокола HTTP/2
			HTTP2_PIPE_WRITE          = 0x0F, // Ошибка записи данных в PIPE для протокола HTTP/2
			HTTP2_FRAME_SIZE          = 0x10, // Ошибка размера фрейма для протокола HTTP/2
			HTTP2_COMPRESSION         = 0x11, // Ошибка сжатия данных для протокола HTTP/2
			HTTP2_FLOW_CONTROL        = 0x12, // Ошибка управления потоком для протокола HTTP/2
			HTTP2_STREAM_CLOSED       = 0x13, // Ошибка закрытия потока протокола HTTP/2
			HTTP2_REFUSED_STREAM      = 0x14, // Ошибка раннего закрытия подключения протокола HTTP/2 
			HTTP2_SETTINGS_TIMEOUT    = 0x15, // Ошибка настроек таймера для протокола HTTP/2
			HTTP2_HTTP_1_1_REQUIRED   = 0x16, // Ошибка обмена данными, требуется протокола HTTP/1.1 а используется HTTP/2
			HTTP2_ENHANCE_YOUR_CALM   = 0x17, // Ошибка повышения спокойствия для протокола HTTP/2
			HTTP2_INADEQUATE_SECURITY = 0x18, // Ошибка недостаточной безопасности для протокола HTTP/2
			PROXY_HTTP2_RECV          = 0x19, // Ошибка получения данных прокси-сервера для протокола HTTP/2
			PROXY_HTTP2_SEND          = 0x1A, // Ошибка отправки сообщения прокси-серверу на протокол HTTP/2
			PROXY_HTTP2_SUBMIT        = 0x1B, // Ошибка предоставления данных прокси-серверу для протокола HTTP/2
			PROXY_HTTP2_NO_INIT       = 0x1C  // Ошибка инициализации протокола HTTP/2 для прокси-сервера
		};
	};
};

#endif // __AWH_WEB_ERRORS__
