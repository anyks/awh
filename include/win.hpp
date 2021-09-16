/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WINDOWS__
#define __AWH_WINDOWS__

/**
 * Устанавливаем типы данных для Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
	#define u_int uint32_t         // unsigned int
	#define u_char unsigned char   // unsigned char
	#define u_short unsigned short // unsigned short
	#define __uint64_t uint64_t    // unsigned int 64
	// Файловый разделитель Windows
	#define FS_SEPARATOR "\\"
	// Устанавливаем кодировку UTF-8
	#pragma execution_character_set("utf-8")
// Для всех остальных OS
#else
	// Файловый разделитель Unix
	#define FS_SEPARATOR "/"
#endif

#endif // __AWH_WINDOWS__
