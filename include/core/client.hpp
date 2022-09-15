/**
 * @file: client.hpp
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

#ifndef __AWH_LIB_CORE_CLIENT__
#define __AWH_LIB_CORE_CLIENT__

/**
 * Если используется модуль LibEvent2
 */
#if defined(AWH_EVENT2)
	/**
	 * Библиотеки LibEvent2
	 */
	#include <lib/event2/core/client.hpp>
#endif

/**
 * Если используется модуль LibEv
 */
#if defined(AWH_EV)
	/**
	 * Библиотеки LibEv
	 */
	#include <lib/ev/core/client.hpp>
#endif

#endif // __AWH_LIB_CORE_CLIENT__
