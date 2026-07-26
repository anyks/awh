/**
 * @file: global.hpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл глобальных макросов сборки — определение атрибутов экспорта и импорта символов
 *        динамической библиотеки для MS Windows и остальных операционных систем, а также режима статической сборки
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_GLOBAL__
#define __AWH_GLOBAL__

/**
 * Для операционной системы MS Windows
 */
#if _MSC_VER || WIN64 || _WIN64 || __WIN64__ || WIN32 || _WIN32 || __WIN32__ || __NT__
	#define __AWH_DECL_EXPORT__ __declspec(dllexport)
	#define __AWH_DECL_IMPORT__ __declspec(dllimport)
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	#define __AWH_DECL_EXPORT__ __attribute__((visibility("default")))
	#define __AWH_DECL_IMPORT__ __attribute__((visibility("default")))
#endif

/**
 * Если активирован экспорт динамической библиотеки
 */
#if __AWH_SHARED_LIBRARY_EXPORT__
	#define __AWH_SHARED_EXPORT__ __AWH_DECL_EXPORT__
/**
 * Если активирован импорт динамической библиотеки
 */
#elif __AWH_SHARED_LIBRARY_IMPORT__
	#define __AWH_SHARED_EXPORT__ __AWH_DECL_IMPORT__
/**
 * Если мы работаем со статической библиотекой
 */
#else
	#define __AWH_SHARED_EXPORT__
#endif

#endif // __AWH_GLOBAL__
