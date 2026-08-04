/**
 * @file: table.hpp
 * @date: 2026-08-04
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл встроенного набора шаблонов модуля Grok —
 *        объявление набора именованных шаблонов, поставляемого вместе с модулем
 *
 * @copyright: Copyright © 2026
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
#include "../sys/global.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модуля Grok
	 *
	 */
	namespace grok {
		/**
		 * @brief Запись встроенного набора шаблонов
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Entry {
			// Название шаблона
			const char * name;
			// Текст шаблона, допускающий ссылки вида «%{NAME}»
			const char * body;
		} entry_t;

		/**
		 * @brief Количество записей встроенного набора шаблонов
		 *
		 */
		__AWH_SHARED_EXPORT__ extern const size_t PATTERNS_COUNT;

		/**
		 * @brief Встроенный набор шаблонов
		 *
		 * @details Набор поставляется вместе с модулем и служит основанием
		 *          реестра: разбор пользовательских шаблонов опирается на него
		 *          ссылками вида «%{NAME}». Записи следуют в порядке объявления,
		 *          а не в порядке разрешения ссылок: разворот выполняется по
		 *          требованию, поэтому шаблону дозволено ссылаться на шаблон,
		 *          объявленный ниже.
		 *
		 */
		__AWH_SHARED_EXPORT__ extern const entry_t PATTERNS[];
	}
}

#endif // __AWH_GROK_TABLE__
