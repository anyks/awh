/**
 * @file: detect.hpp
 * @date: 2026-07-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LEXICAL_DETECT__
#define __AWH_LEXICAL_DETECT__

/**
 * Стандартные заголовочные файлы
 */
#include <cfloat>
#include <cassert>

/**
 * Если архитектура соответствует 64-битной, но не определена разрядность по SIZE_MAX - считаем платформу 64-битной
 */
#if (__x86_64 || __x86_64__ || _M_X64 || \
	 __amd64 || __aarch64__ || _M_ARM64 || \
	 __MINGW64__ || __s390x__ || \
	 __ppc64__ || __PPC64__ || \
	 __ppc64le__ || __PPC64LE__ || \
	 __loongarch64 || (__riscv && (__riscv_xlen == 64)))
	/**
	 * Платформа является 64-битной
	 */
	#define AWH_LEXICAL_64BIT 1
/**
 * Если архитектура соответствует 32-битной, но не определена разрядность по SIZE_MAX - считаем платформу 32-битной
 */
#elif (__i386 || __i386__ || _M_IX86 || \
	 __arm__ || _M_ARM || __ppc__ || \
	 __MINGW32__ || __EMSCRIPTEN__ || \
	 (__riscv && (__riscv_xlen == 32)))
	/**
	 * Платформа является 32-битной
	 */
	#define AWH_LEXICAL_32BIT 1
/** 
 * Если архитектура поддерживает 128-битные регистры, но не определена разрядность по SIZE_MAX - считаем платформу 64-битной
 */
#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFFULL)
	/**
	 * Разрядность определена по максимальному размеру объекта
	 */
	#define AWH_LEXICAL_64BIT 1
/**
 * Если архитектура поддерживает 64-битные регистры, но не определена разрядность по SIZE_MAX - считаем платформу 32-битной
 */
#elif (SIZE_MAX == 0xFFFFFFFFUL)
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
#if _MSC_VER && !__clang__
	/**
	 * Сборка выполняется компилятором Visual Studio
	 */
	#define AWH_LEXICAL_VISUAL_STUDIO 1
#endif

/**
 * Подключаем интринсики Visual Studio для 128-битного умножения
 */
#if (AWH_LEXICAL_VISUAL_STUDIO && (_WIN32 || _WIN64)) || \
	(_M_ARM64 && !__MINGW32__ && !__clang__)
	// Подключаем интринсики Visual Studio для 128-битного умножения
	#include <intrin.h>
#endif

/**
 * Определяем порядок байт платформы
 *
 * @details Каждая ветвь задаёт макрос ровно один раз, повторных определений нет.
 */
#if __BYTE_ORDER__ && __ORDER_BIG_ENDIAN__
	/**
	 * Порядок байт получен из встроенных макросов компилятора
	 */
	#define AWH_LEXICAL_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
/**
 * Определяем порядок байт платформы для Visual Studio
 */
#elif _WIN32 || _M_IX86 || _M_X64 || _M_ARM || _M_ARM64
	/**
	 * Платформы Windows всегда little-endian
	 */
	#define AWH_LEXICAL_BIG_ENDIAN 0
/**
 * Если порядок байт платформы для GCC и Clang сответствует big-endian - определяем макрос как 1, иначе как 0
 */
#elif __BIG_ENDIAN__ || __ARMEB__ || __THUMBEB__ || \
	 __AARCH64EB__ || _MIBSEB || __MIBSEB || __MIBSEB__
	/**
	 * Платформа объявлена как big-endian
	 */
	#define AWH_LEXICAL_BIG_ENDIAN 1
/**
 * Если порядок байт платформы для GCC и Clang сответствует little-endian - определяем макрос как 0, иначе как 1
 */
#elif __LITTLE_ENDIAN__ || __ARMEL__ || __THUMBEL__ || \
	 __AARCH64EL__ || _MIPSEL || __MIPSEL || __MIPSEL__
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
	#if __APPLE__ || __FreeBSD__
		// Подключаем системный заголовок с описанием порядка байт
		#include <machine/endian.h>
	/**
	 * Если операционная система соответствует Solaris или OpenIndiana
	 */
	#elif sun || __sun
		// Подключаем системный заголовок с описанием порядка байт
		#include <sys/byteorder.h>
	/**
	 * Если операционная система соответствует Windows
	 */
	#elif __MVS__
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
	#if BYTE_ORDER && BIG_ENDIAN
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
#if __SSE2__ || (AWH_LEXICAL_VISUAL_STUDIO && \
	(_M_AMD64 || _M_X64 || (_M_IX86_FP && (_M_IX86_FP == 2))))
	/**
	 * Набор инструкций SSE2 доступен
	 */
	#define AWH_LEXICAL_SSE2 1
#endif

/**
 * Определяем поддержку набора инструкций NEON
 */
#if __aarch64__ || _M_ARM64
	/**
	 * Набор инструкций NEON доступен
	 */
	#define AWH_LEXICAL_NEON 1
#endif

/**
 * Определяем общую доступность векторных инструкций
 */
#if AWH_LEXICAL_SSE2 || AWH_LEXICAL_NEON
	/**
	 * Векторные инструкции доступны
	 */
	#define AWH_LEXICAL_SIMD 1
#endif

/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#if __GNUC__
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
#ifdef AWH_LEXICAL_VISUAL_STUDIO
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_LEXICAL_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_LEXICAL_INLINE inline __attribute__((always_inline))
#endif

/**
 * Определяем проверку внутренних инвариантов модуля
 *
 * @details Проверка активна только в отладочной сборке, в релизе разворачивается
 *          в пустую операцию. Логика модуля никогда не полагается на побочные
 *          эффекты выражения: результат всех операций, способных завершиться
 *          отказом, проверяется отдельно возвращаемым значением.
 */
#ifndef AWH_LEXICAL_ASSERT
	/**
	 * Проверка внутренних инвариантов модуля активна только в отладочной сборке
	 */
	#define AWH_LEXICAL_ASSERT(x) assert(x)
#endif

/**
 * Проверяем корректность подключения стандартных заголовочных файлов
 */
#ifndef FLT_EVAL_METHOD
	/**
	 * Стандартный заголовочный файл <cfloat> не подключён, либо компилятор не поддерживает
	 * стандарт C99 и выше, либо платформа не поддерживается: требуется исправить подключение
	 * заголовочных файлов и/или обновить компилятор и/или сменить платформу на поддерживаемую
	 */
	#error "AWH lexical: FLT_EVAL_METHOD is not defined, <cfloat> is required"
#endif

#endif // __AWH_LEXICAL_DETECT__
