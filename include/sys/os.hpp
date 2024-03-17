/**
 * @file: os.hpp
 * @date: 2024-03-17
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_OPERATING_SYSTEM__
#define __AWH_OPERATING_SYSTEM__

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем основные заголовочные файлы
	 */
	#include <windows.h>
	#include <process.h>
	#include <processthreadsapi.h>
	/**
	 * Заменяем переменную AWH ERROR
	 */
	#define AWH_ERROR() (WSAGetLastError())
	/**
	 * Заменяем типы данных
	 */
	#define uid_t uint16_t         // unsigned short
	#define gid_t uint16_t         // unsigned short
	#define u_int uint32_t         // unsigned int
	#define u_char unsigned char   // unsigned char
	#define u_short unsigned short // unsigned short
	#define __uint64_t uint64_t    // unsigned int 64
	/**
	 * Заменяем вызов функции
	 */
	#define getpid _getpid
	#define getppid GetCurrentProcessId
	/**
	 * Файловый разделитель Windows
	 */
	#define FS_SEPARATOR "\\"
	/**
	 * Устанавливаем кодировку UTF-8
	 */
	#pragma execution_character_set("utf-8")
/**
 * Если операционной системой является Nix-подобная
 */
#else
	/**
	 * Подключаем основные заголовочные файлы
	 */
	#include <cerrno>
	/**
	 * Заменяем переменную AWH ERROR
	 */
	#define AWH_ERROR() (errno)
	/**
	 * Файловый разделитель UNIX-подобных систем
	 */
	#define FS_SEPARATOR "/"
#endif

#endif // __AWH_OPERATING_SYSTEM__
