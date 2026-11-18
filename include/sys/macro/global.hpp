/**
 * @file global.hpp
 * @date 2026-11-18
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
 * @brief Заголовочный файл макросов AWH v5, сведённых к макросам базового заголовка v4
 *
 * \~english
 * @brief Header file of the AWH v5 macros reduced to the macros of the v4 base header
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * \~russian
 * @brief Защита от повторной инициализации
 *
 * \~english
 * @brief Re-initialization guard
 *
 * \~
 */
#ifndef __AWH_MACRO_GLOBAL__
/**
 * \~russian
 * @brief Объявление стража заголовка
 *
 * \~english
 * @brief Declaration of the header guard
 *
 * \~
 */
#define __AWH_MACRO_GLOBAL__

/**
 * \~russian
 * @brief Подключение базовых макросов дерева v4
 *
 * @details Модули, перенесённые из v5 (regex, encoding, cryptography/hash, num,
 *          alloc, proto/http/parser/http2), объявляют сущности через
 *          __AWH_SHARED_EXPORT__ и просят подстановки через __AWH_INLINE__.
 *          Дерево v4 ведёт отдачу символов своими именами
 *          (AWH_SHARED_LIBRARY_EXPORT/IMPORT, DECL_EXPORT/IMPORT), и заводить
 *          второй набор правил незачем: имена v5 сводятся к уже объявленным
 *          макросам базового заголовка
 *
 * \~english
 * @brief Inclusion of the v4 tree base macros
 *
 * @details The modules ported from v5 (regex, encoding, cryptography/hash, num,
 *          alloc, proto/http/parser/http2) declare entities through
 *          __AWH_SHARED_EXPORT__ and request inlining through __AWH_INLINE__.
 *          The v4 tree manages symbol export with its own names
 *          (AWH_SHARED_LIBRARY_EXPORT/IMPORT, DECL_EXPORT/IMPORT), and there is
 *          no reason to keep a second set of rules: the v5 names are reduced to
 *          the macros already declared in the base header
 *
 * \~
 */
#include "../global.hpp"

/**
 * \~russian
 * @brief Если активирован экспорт динамической библиотеки
 *
 * \~english
 * @brief If the shared library export is activated
 *
 * \~
 */
#if AWH_SHARED_LIBRARY_EXPORT
	/**
	 * \~russian
	 * @brief Отдача символов при экспорте
	 *
	 * \~english
	 * @brief Symbol export on export
	 *
	 * \~
	 */
	#define __AWH_SHARED_EXPORT__ DECL_EXPORT
/**
 * \~russian
 * @brief Если активирован импорт динамической библиотеки
 *
 * \~english
 * @brief If the shared library import is activated
 *
 * \~
 */
#elif AWH_SHARED_LIBRARY_IMPORT
	/**
	 * \~russian
	 * @brief Взятие символов при импорте
	 *
	 * \~english
	 * @brief Symbol acquisition on import
	 *
	 * \~
	 */
	#define __AWH_SHARED_EXPORT__ DECL_IMPORT
/**
 * \~russian
 * @brief Если библиотека собрана как статическая
 *
 * \~english
 * @brief If the library is built as a static one
 *
 * \~
 */
#else
	/**
	 * \~russian
	 * @brief Объявление остаётся без атрибута отдачи
	 *
	 * \~english
	 * @brief The declaration stays without an export attribute
	 *
	 * \~
	 */
	#define __AWH_SHARED_EXPORT__
#endif

/**
 * \~russian
 * @brief Если компилятор принадлежит семейству Visual Studio
 *
 * \~english
 * @brief If the compiler belongs to the Visual Studio family
 *
 * \~
 */
#if defined(_MSC_VER)
	/**
	 * \~russian
	 * @brief Принудительная подстановка средствами Visual Studio
	 *
	 * \~english
	 * @brief Forced substitution using Visual Studio tools
	 *
	 * \~
	 */
	#define __AWH_INLINE__ __forceinline
/**
 * \~russian
 * @brief Если компилятор принадлежит семейству GCC или Clang
 *
 * \~english
 * @brief If the compiler belongs to the GCC or Clang family
 *
 * \~
 */
#else
	/**
	 * \~russian
	 * @brief Принудительная подстановка средствами GCC и Clang
	 *
	 * \~english
	 * @brief Forced inlining via GCC and Clang
	 *
	 * \~
	 */
	#define __AWH_INLINE__ inline __attribute__((always_inline))
#endif

#endif
