/**
 * @file: evpipe.hpp
 * @date: 2024-07-03
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

#ifndef __AWH_EVENT_PIPE__
#define __AWH_EVENT_PIPE__

/**
 * Стандартные модули
 */
#include <array>
#include <string>
#include <cstring>
#include <algorithm>

/**
 * Наши модули
 */
#include <net/socket.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * EventPIPE Класс передачи данных между процессами
	 */
	typedef class AWHSHARED_EXPORT EventPIPE {
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * create Метод создания файловых дескрипторов
			 * @return файловые дескрипторы для обмена данными
			 */
			array <SOCKET, 2> create() noexcept;
		public:
			/**
			 * read Метод чтения из файлового дескриптора в буфер данных
			 * @param fd        файловый дескриптор (сокет) для чтения
			 * @param timestamp время прошедшее с момента запуска таймера
			 * @return          размер прочитанных байт
			 */
			int8_t read(const SOCKET fd, uint64_t & timestamp) noexcept;
			/**
			 * send Метод отправки файловому дескриптору сообщения
			 * @param fd        файловый дескриптор (сокет) для чтения
			 * @param timestamp время прошедшее с момента запуска таймера
			 * @return          размер записанных байт
			 */
			int8_t send(const SOCKET fd, const uint64_t timestamp) noexcept;
		public:
			/**
			 * EventPIPE Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			EventPIPE(const fmk_t * fmk, const log_t * log) noexcept : _socket(fmk, log), _log(log) {}
			/**
			 * ~EventPIPE Деструктор
			 */
			~EventPIPE() noexcept {}
	} evpipe_t;
};

#endif // __AWH_EVENT_PIPE__
