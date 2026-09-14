/**
 * @file detect.hpp
 * @date 2026-07-22
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл определения свойств платформы для модуля разбора чисел — детект разрядности архитектуры,
 *        доступности векторных инструкций SSE2 и NEON, поддержки 128-битных целых и особенностей компилятора
 *
 * \~english
 * @brief Header file of the detection of the platform properties for the number parsing module — detection of the bitness of the architecture,
 *        of the availability of the SSE2 and NEON vector instructions, of the support of 128-bit integers and of the compiler peculiarities
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <cfloat>
#include <cassert>

/**
 * Если архитектура соответствует 64-битной, но не определена разрядность по SIZE_MAX - считаем платформу 64-битной
 */
#if (defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || \
	 defined(__amd64) || defined(__aarch64__) || defined(_M_ARM64) || \
	 defined(__MINGW64__) || defined(__s390x__) || \
	 defined(__ppc64__) || defined(__PPC64__) || \
	 defined(__ppc64le__) || defined(__PPC64LE__) || \
	 defined(__loongarch64) || (defined(__riscv) && (__riscv_xlen == 64)))
	/**
	 * Платформа является 64-битной
	 */
	#define AWH_LEXICAL_64BIT 1
/**
 * Если архитектура соответствует 32-битной, но не определена разрядность по SIZE_MAX - считаем платформу 32-битной
 */
#elif (defined(__i386) || defined(__i386__) || defined(_M_IX86) || \
	 defined(__arm__) || defined(_M_ARM) || defined(__ppc__) || \
	 defined(__MINGW32__) || defined(__EMSCRIPTEN__) || \
	 (defined(__riscv) && (__riscv_xlen == 32)))
	/**
	 * Платформа является 32-битной
	 */
	#define AWH_LEXICAL_32BIT 1
/** 
 * Если архитектура поддерживает 128-битные регистры, но не определена разрядность по SIZE_MAX - считаем платформу 64-битной
 */
#elif defined(SIZE_MAX) && (SIZE_MAX == 0xFFFFFFFFFFFFFFFFULL)
	/**
	 * Разрядность определена по максимальному размеру объекта
	 */
	#define AWH_LEXICAL_64BIT 1
/**
 * Если архитектура поддерживает 64-битные регистры, но не определена разрядность по SIZE_MAX - считаем платформу 32-битной
 */
#elif defined(SIZE_MAX) && (SIZE_MAX == 0xFFFFFFFFUL)
	/**
	 * Разрядность определена по максимальному размеру объекта
	 */
	#define AWH_LEXICAL_32BIT 1
/**
 * Если архитектура не поддерживает 32-битные и 64-битные регистры - считаем платформу неподдерживаемой
 */
#else
	/**
	 * Платформа не поддерживается
	 */
	#error "AWH lexical: unsupported platform, 32-bit or 64-bit is required"
#endif

/**
 * Определяем компилятор Visual Studio
 */
#if defined(_MSC_VER) && !defined(__clang__)
	/**
	 * Сборка выполняется компилятором Visual Studio
	 */
	#define AWH_LEXICAL_VISUAL_STUDIO 1
#endif

/**
 * Подключаем интринсики Visual Studio для 128-битного умножения
 */
#if (defined(AWH_LEXICAL_VISUAL_STUDIO) && (defined(_WIN32) || defined(_WIN64))) || \
	(defined(_M_ARM64) && !defined(__MINGW32__) && !defined(__clang__))
	// Подключаем интринсики Visual Studio для 128-битного умножения
	#include <intrin.h>
#endif

/**
 * \~russian
 * Определяем порядок байт платформы
 *
 * @details Каждая ветвь задаёт макрос ровно один раз, повторных определений нет.
 *
 * \~english
 * Determine the byte order of the platform
 * @details Every branch sets the macro exactly once, there are no repeated definitions.
 *
 * \~
 */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
	/**
	 * Порядок байт получен из встроенных макросов компилятора
	 */
	#define AWH_LEXICAL_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
/**
 * Определяем порядок байт платформы для Visual Studio
 */
#elif defined(_WIN32) || defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64)
	/**
	 * Платформы Windows всегда little-endian
	 */
	#define AWH_LEXICAL_BIG_ENDIAN 0
/**
 * Если порядок байт платформы для GCC и Clang сответствует big-endian - определяем макрос как 1, иначе как 0
 */
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
	 defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
	/**
	 * Платформа объявлена как big-endian
	 */
	#define AWH_LEXICAL_BIG_ENDIAN 1
/**
 * Если порядок байт платформы для GCC и Clang сответствует little-endian - определяем макрос как 0, иначе как 1
 */
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) || \
	 defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
	/**
	 * Платформа объявлена как little-endian
	 */
	#define AWH_LEXICAL_BIG_ENDIAN 0
/**
 * Если порядок байт платформы не определён - подключаем системный заголовок с описанием порядка байт
 */
#else
	/**
	 * Если операционная система соответствует Apple
	 */
	#if defined(__APPLE__) || defined(__FreeBSD__)
		// Подключаем системный заголовок с описанием порядка байт
		#include <machine/endian.h>
	/**
	 * Если операционная система соответствует Solaris или OpenIndiana
	 */
	#elif defined(sun) || defined(__sun)
		// Подключаем системный заголовок с описанием порядка байт
		#include <sys/byteorder.h>
	/**
	 * Если операционная система соответствует Windows
	 */
	#elif defined(__MVS__)
		// Подключаем системный заголовок с описанием порядка байт
		#include <sys/endian.h>
	/**
	 * Если операционная система поддерживает стандарт C11 - подключаем системный заголовок с описанием порядка байт
	 */
	#elif __has_include
		/**
		 * Если системный заголовок с описанием порядка байт доступен - подключаем его
		 */
		#if __has_include(<endian.h>)
			// Подключаем системный заголовок с описанием порядка байт
			#include <endian.h>
		#endif
	#endif
	/**
	 * Если порядок байт платформы получен из системного заголовка
	 */
	#if defined(BYTE_ORDER) && defined(BIG_ENDIAN)
		/**
		 * Порядок байт получен из системного заголовка
		 */
		#define AWH_LEXICAL_BIG_ENDIAN (BYTE_ORDER == BIG_ENDIAN)
	/**
	 * Если порядок байт платформы не определён
	 */
	#else
		/**
		 * Порядок байт определить не удалось, считаем платформу little-endian
		 */
		#define AWH_LEXICAL_BIG_ENDIAN 0
	#endif
#endif

/**
 * Определяем поддержку набора инструкций SSE2
 */
#if defined(__SSE2__) || (defined(AWH_LEXICAL_VISUAL_STUDIO) && \
	(defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))))
	/**
	 * Набор инструкций SSE2 доступен
	 */
	#define AWH_LEXICAL_SSE2 1
#endif

/**
 * Определяем поддержку набора инструкций NEON
 */
#if defined(__aarch64__) || defined(_M_ARM64)
	/**
	 * Набор инструкций NEON доступен
	 */
	#define AWH_LEXICAL_NEON 1
#endif

/**
 * Определяем общую доступность векторных инструкций
 */
#if defined(AWH_LEXICAL_SSE2) || defined(AWH_LEXICAL_NEON)
	/**
	 * Векторные инструкции доступны
	 */
	#define AWH_LEXICAL_SIMD 1
#endif

/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#if defined(__GNUC__)
	/**
	 * Отключаем предупреждения о выравнивании указателей
	 */
	#define AWH_LEXICAL_SIMD_DISABLE_WARNINGS \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wcast-align\"")
	/**
	 * Восстанавливаем предупреждения о выравнивании указателей
	 */
	#define AWH_LEXICAL_SIMD_RESTORE_WARNINGS _Pragma("GCC diagnostic pop")
/**
 * Если компилятор не принадлежит к семейству GCC или Clang - управление предупреждениями не требуется
 */
#else
	/**
	 * Управление предупреждениями не требуется
	 */
	#define AWH_LEXICAL_SIMD_DISABLE_WARNINGS
	/**
	 * Управление предупреждениями не требуется
	 */
	#define AWH_LEXICAL_SIMD_RESTORE_WARNINGS
#endif

/**
 * Определяем поддержку встроенного контроля переполнения при сложении
 */
#if defined(__has_builtin)
	/**
	 * Если компилятор поддерживает встроенный контроль переполнения при сложении
	 */
	#if __has_builtin(__builtin_add_overflow)
		/**
		 * Встроенный контроль переполнения при сложении доступен
		 */
		#define AWH_LEXICAL_ADD_OVERFLOW 1
	#endif
	/**
	 * Если компилятор поддерживает встроенный контроль переполнения при умножении
	 */
	#if __has_builtin(__builtin_mul_overflow)
		/**
		 * Встроенный контроль переполнения при умножении доступен
		 */
		#define AWH_LEXICAL_MUL_OVERFLOW 1
	#endif
#endif

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(AWH_LEXICAL_VISUAL_STUDIO)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_ASCII_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_ASCII_INLINE inline __attribute__((always_inline))
#endif

/**
 * \~russian
 * Определяем проверку внутренних инвариантов модуля
 *
 * @details Проверка активна только в отладочной сборке, в релизе разворачивается
 *          в пустую операцию. Логика модуля никогда не полагается на побочные
 *          эффекты выражения: результат всех операций, способных завершиться
 *          отказом, проверяется отдельно возвращаемым значением.
 *
 * \~english
 * Define the check of the internal invariants of the module
 * @details The check is active only in a debug build, in a release one it expands
 *          into an empty operation. The logic of the module never relies on the side
 *          effects of the expression: the result of all the operations able to end
 *          in a failure is checked separately by the return value.
 *
 * \~
 */
#if !defined(AWH_LEXICAL_ASSERT)
	/**
	 * Проверка внутренних инвариантов модуля активна только в отладочной сборке
	 */
	#define AWH_LEXICAL_ASSERT(x) assert(x)
#endif

/**
 * Проверяем корректность подключения стандартных заголовочных файлов
 */
#if !defined(FLT_EVAL_METHOD)
	/**
	 * Стандартный заголовочный файл <cfloat> не подключён, либо компилятор не поддерживает
	 * стандарт C99 и выше, либо платформа не поддерживается: требуется исправить подключение
	 * заголовочных файлов и/или обновить компилятор и/или сменить платформу на поддерживаемую
	 */
	#error "AWH lexical: FLT_EVAL_METHOD is not defined, <cfloat> is required"
#endif
