/**
 * @file: notifier.hpp
 * @date: 2025-10-27
 * @license: LicenseRef-AWH-1.0
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
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	#define SOCKET int32_t
	#define INVALID_SOCKET -1
/**
 * Для операционной системы MS Windows
 */
#else
	// Если не определён макрос WIN32_LEAN_AND_MEAN
	#ifndef WIN32_LEAN_AND_MEAN
		// Минимизируем включение редко используемых компонентов Windows headers
		#define WIN32_LEAN_AND_MEAN
		/**
		 * Системные заголовочные файлы
		 */
		#include <winsock2.h>
		#include <ws2tcpip.h>

		// Основные сокеты уведомителя
		#pragma comment(lib, "ws2_32.lib")
	#endif
#endif

/**
 * Для операционной системы macOS, FreeBSD, NetBSD или Linux
 */
#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __linux__
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <queue>
	#include <mutex>
#endif

/**
 * Стандартный заголовочный файл
 */
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../sys/locker.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс уведомителя событий
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Notifier {
		private:
			/**
			 * Для операционной системы MS Windows, OpenBSD или Sun Solaris
			 */
			#if _WIN32 || _WIN64 || __OpenBSD__ || __sun__
				// Основные сокеты уведомителя
				SOCKET _socks[2];
			/**
			 * Для операционной системы macOS, FreeBSD, NetBSD или Linux
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __linux__
				// Основной сокет уведомителя
				SOCKET _sock;
			private:
				// Список передаваемых событий
				std::queue <uint32_t> _events;
			private:
				// Мютекс для блокировки потока
				lock_state_t <std::mutex> _mtx;
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
			uint32_t event() noexcept;
		public:
			/**
			 * @brief Метод отправки уведомления
			 *
			 * @param id идентификатор для отправки
			 */
			void notify(const uint32_t id) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Notifier(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Notifier() noexcept;
	} notifier_t;
};

#endif // __AWH_EVENT_NOTIFIER__
