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
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_GLOBAL__
#define __AWH_GLOBAL__

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#define DECL_EXPORT __declspec(dllexport)
	#define DECL_IMPORT __declspec(dllimport)
#else
	#define DECL_EXPORT __attribute__((visibility("default")))
	#define DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(AWH_SHARED_LIBRARY_EXPORT)
	#define AWHSHARED_EXPORT DECL_EXPORT
#elif defined(AWH_SHARED_LIBRARY_IMPORT)
	#define AWHSHARED_EXPORT DECL_IMPORT
#else
	#define AWHSHARED_EXPORT
#endif

#endif // __AWH_GLOBAL__
