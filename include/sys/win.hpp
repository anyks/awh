/**
 * @file: win.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_WINDOWS__
#define __AWH_WINDOWS__

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем основные заголовочные файлы
	 */
	#include <windows.h>
	#include <process.h>
	// Заменяем типы данных
	#define u_int uint32_t         // unsigned int
	#define u_char unsigned char   // unsigned char
	#define u_short unsigned short // unsigned short
	#define __uint64_t uint64_t    // unsigned int 64
	// Заменяем вызов функции
	#define getpid _getpid
	// Файловый разделитель Windows
	#define FS_SEPARATOR "\\"
	// Устанавливаем кодировку UTF-8
	#pragma execution_character_set("utf-8")
/**
 * Если операционной системой является Nix-подобная
 */
#else
	// Файловый разделитель Unix
	#define FS_SEPARATOR "/"
#endif

#endif // __AWH_WINDOWS__
