/**
 * @file: partners.hpp
 * @date: 2025-09-17
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

#ifndef __AWH_EVENT_PARTNERS_BASE__
#define __AWH_EVENT_PARTNERS_BASE__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>

/**
 * Наши модули
 */
#include "../net/socket.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс партнёрских сокетов
	 *
	 */
	typedef class AWHSHARED_EXPORT Partners {
		private:
			// Мютекс для блокировки потока
			std::mutex _mtx;
		private:
			// База партнёрских сокетов
			std::map <SOCKET, SOCKET> _base;
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод проверки существования сокета
			 *
			 * @param sock сокет для проверки
			 * @return     результат проверки
			 */
			bool has(const SOCKET sock) noexcept;
		public:
			/**
			 * @brief Метод удаления сокета
			 *
			 * @param sock сокет для удаления
			 */
			void del(const SOCKET sock) noexcept;
		public:
			/**
			 * @brief Метод объединения партнёрских сокетов
			 * 
			 * @param sock1 первый сокет для добавления
			 * @param sock2 второй сокет для добавления
			 * @return      результат объединения
			 */
			bool merge(const SOCKET sock1, const SOCKET sock2) noexcept;
		public:
			/**
			 * @brief Конструктор
			 * 
			 * @param log объект для работы с логами
			 */
			Partners(const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 * 
			 */
			~Partners() noexcept;
	} partners_t;
};

#endif // __AWH_EVENT_PARTNERS_BASE__
