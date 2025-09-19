/**
 * @file: global.hpp
 * @date: 2024-11-22
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

#ifndef __AWH_GLOBAL__
#define __AWH_GLOBAL__

/**
 * Для операционной системы MS Windows
 */
#if _MSC_VER || WIN64 || _WIN64 || __WIN64__ || WIN32 || _WIN32 || __WIN32__ || __NT__
	#define DECL_EXPORT __declspec(dllexport)
	#define DECL_IMPORT __declspec(dllimport)
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	#define DECL_EXPORT __attribute__((visibility("default")))
	#define DECL_IMPORT __attribute__((visibility("default")))
#endif

/**
 * Если активирован экспорт динамической библиотеки
 */
#if AWH_SHARED_LIBRARY_EXPORT
	#define AWHSHARED_EXPORT DECL_EXPORT
/**
 * Если активирован импорт динамической библиотеки
 */
#elif AWH_SHARED_LIBRARY_IMPORT
	#define AWHSHARED_EXPORT DECL_IMPORT
/**
 * Если мы работаем со статической библиотекой
 */
#else
	#define AWHSHARED_EXPORT
#endif

#endif // __AWH_GLOBAL__
