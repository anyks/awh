/**
 * @file: fds.hpp
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

#ifndef __AWH_EVENT_FDS_BASE__
#define __AWH_EVENT_FDS_BASE__

/**
 * Стандартные модули
 */
#include <cerrno>
#include <string>
#include <cstring>
#include <unistd.h>

/**
 * Наши модули
 */
#include "../sys/log.hpp"

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
	typedef class AWHSHARED_EXPORT FDS {
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод вывода в лог справочной помощи
			 *
			 * @param actual  текущее значение установленных файловых дескрипторов
			 * @param desired желаемое значение для установки файловых дескрипторов
			 */
			void help(const uint64_t actual, const uint64_t desired) const noexcept;
		public:
			/**
			 * @brief Метод установки нужного количества файловых дескрипторов
			 *
			 * @param limit желаемое количество файловых дескрипторов
			 * @return      результат установки
			 */
			bool limit(const uint64_t limit) const noexcept;
			/**
			 * @brief Метод получения лимита файловых дескрипторов установленных в операционной системе
			 *
			 * @return количество файловых дескрипторов установленных в файловой системе
			 */
			std::pair <uint64_t, uint64_t> limit() const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 */
			FDS(const log_t * log) noexcept : _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~FDS() noexcept {}
	} fds_t;
};

#endif // __AWH_EVENT_FDS_BASE__
