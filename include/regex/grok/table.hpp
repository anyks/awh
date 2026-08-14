/**
 * @file table.hpp
 * @date 2026-08-04
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
 * @brief Заголовочный файл встроенного набора шаблонов модуля Grok —
 *        объявление набора именованных шаблонов, поставляемого вместе с модулем
 *
 * \~english
 * @brief Header file of the built-in pattern set of the Grok module —
 *        the declaration of the set of named patterns shipped together with the module
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_GROK_TABLE__
#define __AWH_GROK_TABLE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён модуля Grok
	 *
	 * \~english
	 * @brief Namespace of the Grok module
	 *
	 * \~
	 */
	namespace grok {
		/**
		 * \~russian
		 * @brief Запись встроенного набора шаблонов
		 *
		 * \~english
		 * @brief Record of the built-in pattern set
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Entry {
			// Название шаблона
			const char * name;
			// Текст шаблона, допускающий ссылки вида «%{NAME}»
			const char * body;
		} entry_t;

		/**
		 * \~russian
		 * @brief Количество записей встроенного набора шаблонов
		 *
		 * \~english
		 * @brief Number of records of the built-in pattern set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ extern const size_t PATTERNS_COUNT;

		/**
		 * \~russian
		 * @brief Встроенный набор шаблонов
		 *
		 * @details Набор поставляется вместе с модулем и служит основанием
		 *          реестра: разбор пользовательских шаблонов опирается на него
		 *          ссылками вида «%{NAME}». Записи следуют в порядке объявления,
		 *          а не в порядке разрешения ссылок: разворот выполняется по
		 *          требованию, поэтому шаблону дозволено ссылаться на шаблон,
		 *          объявленный ниже.
		 *
		 * \~english
		 * @brief Built-in pattern set
		 * @details The set is shipped together with the module and serves as the foundation
		 *          of the registry: parsing the user patterns rests on it
		 *          by references of the «%{NAME}» form. The records follow in the order of declaration
		 *          rather than in the order of resolving the references: the expansion is performed on
		 *          demand, therefore a pattern is allowed to refer to a pattern
		 *          declared below.
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ extern const entry_t PATTERNS[];
	}
}

#endif // __AWH_GROK_TABLE__
