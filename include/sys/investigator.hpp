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

/**
 * Для оснастки MSVC, не несущей слоя POSIX
 */
#if defined(_MSC_VER)
	/**
	 * Подключаем стандартные модули
	 */
	#include <io.h>
	#include <process.h>
/**
 * Для оснастки, несущей слой POSIX
 */
#else
	/**
	 * Подключаем стандартные модули
	 */
	#include <unistd.h>
#endif

/**
 * Системный модуль
 */
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
 * Реализация под Sun Solaris
 */
#elif __sun__
	#include <fstream>
	#include <sstream>
	#include <procfs.h>
/**
 * Для операционной системы Windows
 */
#elif _WIN32 || _WIN64
	/**
	 * Подключаем основные заголовочные файлы
	 *
	 * Порядок обязателен: winsock2.h идёт строго до windows.h, иначе windows.h
	 * втягивает устаревший winsock.h, и объявления сокетов сталкиваются -
	 * оснастка MSVC отвечает на это переопределением типов sockaddr и fd_set
	 */
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	#include <psapi.h>
	#include <cstdint>
#endif

/**
 * Разрешаем сборку под Windows
 */
#include "global.hpp"

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
			explicit Investigator() noexcept = default;
			/**
			 * @brief Деструктор
			 *
			 */
			~Investigator() noexcept = default;
	} igtr_t;
};

#endif // __AWH_INVESTIGATOR__
