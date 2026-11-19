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
 * @brief Заголовочный файл макросов AWH v5, сведённых к макросам базового заголовка v4
 *
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * @brief Защита от повторной инициализации
 *
 */
#ifndef __AWH_MACRO_GLOBAL__
/**
 * @brief Объявление стража заголовка
 *
 */
#define __AWH_MACRO_GLOBAL__

/**
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
 */
#include "../global.hpp"

/**
 * @brief Если активирован экспорт динамической библиотеки
 *
 */
#if AWH_SHARED_LIBRARY_EXPORT
	/**
	 * @brief Отдача символов при экспорте
	 *
	 */
	#define __AWH_SHARED_EXPORT__ DECL_EXPORT
/**
 * @brief Если активирован импорт динамической библиотеки
 *
 */
#elif AWH_SHARED_LIBRARY_IMPORT
	/**
	 * @brief Взятие символов при импорте
	 *
	 */
	#define __AWH_SHARED_EXPORT__ DECL_IMPORT
/**
 * @brief Если библиотека собрана как статическая
 *
 */
#else
	/**
	 * @brief Объявление остаётся без атрибута отдачи
	 *
	 */
	#define __AWH_SHARED_EXPORT__
#endif

/**
 * @brief Если компилятор принадлежит семейству Visual Studio
 *
 */
#if defined(_MSC_VER)
	/**
	 * @brief Принудительная подстановка средствами Visual Studio
	 *
	 */
	#define __AWH_INLINE__ __forceinline
/**
 * @brief Если компилятор принадлежит семейству GCC или Clang
 *
 */
#else
	/**
	 * @brief Принудительная подстановка средствами GCC и Clang
	 *
	 */
	#define __AWH_INLINE__ inline __attribute__((always_inline))
#endif

#endif
