/**
 * @file: investigator.hpp
 * @date: 2024-11-18
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

#ifndef __AWH_INVESTIGATOR__
#define __AWH_INVESTIGATOR__

/**
 * Стандартные модули
 */
#include <string>
#include <unistd.h>

/**
 * Разрешаем сборку под Windows
 */
#include "global.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс работы с дознователем
	 *
	 */
	typedef class AWH_SHARED_EXPORT Investigator {
		public:
			/**
			 * @brief Метод проведения дознания
			 *
			 * @param pid идентификатор процесса
			 * @return    название приложения которому принадлежит процесс
			 */
			string inquiry(const pid_t pid = ::getpid()) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Investigator() noexcept {}
			/**
			 * @brief Деструктор
			 *
			 */
			~Investigator() noexcept {}
	} igtr_t;
};

#endif // __AWH_INVESTIGATOR__
