/**
 * @file: procre.hpp
 * @date: 2026-01-26
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

#ifndef __AWH_PROCRE__
#define __AWH_PROCRE__

/**
 * Стандартные модули
 */
#include <string>
#include <unistd.h>

/**
 * Наши модули
 */
#include "log.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс работы с резольвером процессов
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Process_Resolver {
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод получения названия приложения по идентификатору процесса
			 *
			 * @param pid идентификатор процесса
			 * @return    название приложения которому принадлежит процесс
			 */
			string name(const pid_t pid = ::getpid()) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 */
			explicit Process_Resolver(const log_t * log) noexcept : _log(log) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~Process_Resolver() noexcept {}
	} procre_t;
};

#endif // __AWH_PROCRE__
