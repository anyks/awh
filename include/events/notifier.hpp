/**
 * @file: notifier.hpp
 * @date: 2025-09-16
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

#ifndef __AWH_EVENT_NOTIFIER__
#define __AWH_EVENT_NOTIFIER__

/**
 * Для операционной системы MacOS X, FreeBSD, NetBSD или Linux
 */
#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __linux__
	/**
	 * Стандартные модули
	 */
	#include <queue>
	#include <mutex>
#endif

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
	 * @brief Класс уведомителя событий
	 *
	 */
	typedef class AWH_SHARED_EXPORT Notifier {
		private:
			/**
			 * Для операционной системы MS Windows, OpenBSD или Sun Solaris
			 */
			#if _WIN32 || _WIN64 || __OpenBSD__ || __sun__
				// Основные сокеты уведомителя
				SOCKET _socks[2];
			/**
			 * Для операционной системы MacOS X, FreeBSD, NetBSD или Linux
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __linux__
				// Основной сокет уведомителя
				SOCKET _sock;
			#endif
			/**
			 * Для операционной системы MacOS X, FreeBSD, NetBSD или Linux.
			 * На Linux eventfd складывает записанные значения в один счётчик: два уведомления
			 * до одного чтения давали сумму идентификаторов. Поэтому идентификаторы хранятся
			 * в очереди, а eventfd служит только сигналом пробуждения
			 */
			#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __linux__
				private:
					// Мютекс для блокировки потока
					std::mutex _mtx;
				private:
					// Список передаваемых событий
					std::queue <uint64_t> _events;
				#endif
		private:
			/**
			 * Для операционной системы MS Windows, OpenBSD или Sun Solaris
			 */
			#if _WIN32 || _WIN64 || __OpenBSD__ || __sun__
				// Объект работы с сокетами
				socket_t _socket;
			#endif
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод сброса уведомителя
			 *
			 */
			void reset() noexcept;
		public:
			/**
			 * @brief Метод инициализации уведомителя
			 *
			 * @return содержимое сокета для извлечения
			 */
			SOCKET init() noexcept;
		public:
			/**
			 * @brief Метод извлечения идентификатора события
			 *
			 * @return идентификатор события
			 */
			uint64_t event() noexcept;
			/**
			 * @brief Метод извлечения идентификатора события с признаком наличия
			 *
			 * Идентификатор 0 допустим, поэтому наличие события сообщается отдельно.
			 * Метод вызывается до получения false, чтобы извлечь все накопившиеся события
			 *
			 * @param id идентификатор извлечённого события
			 * @return   результат извлечения (false, если событий нет)
			 */
			bool event(uint64_t & id) noexcept;
		public:
			/**
			 * @brief Метод отправки уведомления
			 *
			 * @param id идентификатор для отправки
			 */
			void notify(const uint64_t id) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Notifier(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Notifier() noexcept;
	} notifier_t;
};

#endif // __AWH_EVENT_NOTIFIER__
