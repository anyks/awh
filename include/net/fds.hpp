/**
 * @file: fds.hpp
 * @date: 2025-10-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля партнёрских сокетов — класс Files_Descriptors для создания связанных пар файловых
 *        дескрипторов и передачи дескрипторов между процессами через управляющие сообщения
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_EVENT_FDS_BASE__
#define __AWH_EVENT_FDS_BASE__

/**
 * Стандартный заголовочный файл
 */
#include <cstdint>

/**
 * Системный заголовочный файл
 */
#include <unistd.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/log.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс партнёрских сокетов
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Files_Descriptors {
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод вывода в лог справочной помощи
			 *
			 * @param actual  текущее значение установленных файловых дескрипторов
			 * @param desired желаемое значение для установки файловых дескрипторов
			 *
			 */
			void help(const uint32_t actual, const uint32_t desired) const noexcept;
		public:
			/**
			 * @brief Метод установки нужного количества файловых дескрипторов
			 *
			 * @param limit желаемое количество файловых дескрипторов
			 * @return      результат установки
			 *
			 */
			bool limit(const uint32_t limit) const noexcept;
			/**
			 * @brief Метод получения лимита файловых дескрипторов установленных в операционной системе
			 *
			 * @return количество файловых дескрипторов установленных в файловой системе
			 *
			 */
			std::pair <uint32_t, uint32_t> limit() const noexcept;
			/**
			 * @brief Метод оценки лимита одновременно открытых сокетов
			 *
			 * @param max верхний предел пробинга (0 - использовать значение по умолчанию)
			 * @return    пара значений (оценка доступного количества сокетов, верхний предел пробинга)
			 *
			 */
			std::pair <uint32_t, uint32_t> sockets(const uint32_t max = 0) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 *
			 */
			explicit Files_Descriptors(const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Files_Descriptors() noexcept;
	} fds_t;
};

#endif // __AWH_EVENT_FDS_BASE__
