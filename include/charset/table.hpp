/**
 * @file: table.hpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл таблиц соответствия однобайтовых кодировок Юникоду —
 *        объявления набора таблиц кодировок и таблицы имён, которыми кодировки
 *        обозначаются в заголовках сетевых протоколов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CHARSET_TABLE__
#define __AWH_CHARSET_TABLE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "types.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён модуля перекодировки
	 *
	 */
	namespace charset {
		/**
		 * Таблицы кодировок, порождаемые средством «tools/charset/generate.py»
		 */
		extern __AWH_SHARED_EXPORT__ const codepage_t CODEPAGES[];
		extern __AWH_SHARED_EXPORT__ const size_t CODEPAGES_COUNT;
		extern __AWH_SHARED_EXPORT__ const labeling_t LABELS[];
		extern __AWH_SHARED_EXPORT__ const size_t LABELS_COUNT;
	};
};

#endif // __AWH_CHARSET_TABLE__
