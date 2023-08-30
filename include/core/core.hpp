/**
 * @file: core.hpp
 * @date: 2022-09-15
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_LIB_CORE__
#define __AWH_LIB_CORE__

/**
 * Если компилятор выполняется MS VisualStudio
 */
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4324)
#endif

/**
 * Если компилятор выполняется MS VisualStudio и версия ниже 2013
 */
#if defined(_MSC_VER) && (_MSC_VER < 1800)
	/*
	 * MSVC < 2013 не имеет inttypes.h, поскольку он не совместим с C99.
	 * Подробнее про макросы компилятора и номер версии в https://sourceforge.net/p/predef/wiki/Compilers.
	 */
	#include <stdint.h>
/**
 * Если компилятор не относится к MS VisualStudio
 */
#else
	#include <inttypes.h>
#endif

/**
 * Если производится статическая сборка библиотеки
 */
#ifdef AWH_STATICLIB
	#define AWH_EXTERN
/**
 * Методы только для OS Windows
 */
#elif defined(_WIN32) || defined(_WIN64)
	#ifdef BUILDING_AWH
		#define AWH_EXTERN __declspec(dllexport)
	#else
		#define AWH_EXTERN __declspec(dllimport)
	#endif
/**
 * Для всех остальных операционных систем
 */
#else
	#ifdef BUILDING_AWH
		#define AWH_EXTERN __attribute__((visibility("default")))
	#else
		#define AWH_EXTERN
	#endif
#endif

/**
 * Если компилятор выполняется MS VisualStudio
 */
#ifdef _MSC_VER
	#define AWH_ALIGN(N) __declspec(align(N))
/**
 * Если компилятор не относится к MS VisualStudio
 */
#else
	#define AWH_ALIGN(N) __attribute__((aligned(N)))
#endif

/**
 * Если используется модуль LibEvent2
 */
#if defined(AWH_EVENT2)
	/**
	 * Библиотеки LibEvent2
	 */
	#include <lib/event2/core/core.hpp>
/**
 * Если используется модуль LibEv
 */
#elif defined(AWH_EV)
	/**
	 * Библиотеки LibEv
	 */
	#include <lib/ev/core/core.hpp>
#endif

#endif // __AWH_LIB_CORE__
