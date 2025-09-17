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
 * Для операционной системы MacOS X, FreeBSD или NetBSD
 */
#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
	/**
	 * Стандартные модули
	 */
	#include <queue>
#endif

/**
 * Стандартные модули
 */
#include <array>

/**
 * Наши модули
 */
#include "../net/socket.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Notifier Класс уведомителя событий
	 */
	typedef class AWHSHARED_EXPORT Notifier {
		private:
			/**
			 * Для операционной системы OS Windows, OpenBSD или Sun Solaris
			 */
			#if _WIN32 || _WIN64 || __OpenBSD__ || __sun__
				// Основные сокеты уведомителя
				SOCKET _socks[2];
			/**
			 * Для операционной системы MacOS X, FreeBSD, NetBSD или Linux
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ ||  || __linux__
				// Основной сокет уведомителя
				SOCKET _sock;
			#endif
			/**
			 * Для операционной системы MacOS X, FreeBSD или NetBSD
			 */
			#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
				private:
					// Мютекс для блокировки потока
					std::mutex _mtx;
				private:
					// Список передаваемых событий
					std::queue <uint64_t> _events;
				#endif
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * reset Метод сброса уведомителя
			 */
			void reset() noexcept;
		public:
			/**
			 * init Метод инициализации уведомителя
			 * @return содержимое сокета для извлечения
			 */
			std::array <SOCKET, 2> init() noexcept;
		public:
			/**
			 * event Метод извлечения идентификатора события
			 * @return идентификатор события
			 */
			uint64_t event() noexcept;
		public:
			/**
			 * notify Метод отправки уведомления
			 * @param id идентификатор для отправки
			 */
			void notify(const uint64_t id) noexcept;
		public:
			/**
			 * Notifier Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Notifier(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Notifier Деструктор
			 */
			~Notifier() noexcept;
	} notifier_t;
};

#endif // __AWH_EVENT_NOTIFIER__
