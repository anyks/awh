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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>

/**
 * Для операционной системы Linux
 */
#ifdef __linux__
	#include <sys/file.h>
	#include <sys/stat.h>
/**
 * Для операционной системы FreeBSD
 */
#elif __FreeBSD__
	#include <libutil.h>
	#include <sys/user.h>
/**
 * Для операционной системы MacOS X
 */
#elif __APPLE__ || __MACH__
	#include <libproc.h>
/**
 * Для операционной системы NetBSD или OpenBSD
 */
#elif __NetBSD__ || __OpenBSD__
	#include <fstream>
	#include <sstream>
/**
 * Для операционной системы Windows
 */
#elif _WIN32 || _WIN64
	#include <windows.h>
	#include <psapi.h>
	#include <cstdint>
#endif

/**
 * Разрешаем сборку под Windows
 */
#include <sys/global.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Investigator Класс работы с дознователем
	 */
	typedef class AWHSHARED_EXPORT Investigator {
		public:
			/**
			 * inquiry Метод проведения дознания
			 * @param pid идентификатор процесса
			 * @return    название приложения которому принадлежит процесс
			 */
			string inquiry(const pid_t pid = ::getpid()) const noexcept;
		public:
			/**
			 * Investigator Конструктор
			 */
			Investigator() noexcept {}
			/**
			 * ~Investigator Деструктор
			 */
			~Investigator() noexcept {}
	} igtr_t;
};

#endif // __AWH_INVESTIGATOR__
