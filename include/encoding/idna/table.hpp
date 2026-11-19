/**
 * @file table.hpp
 * @date 2026-08-03
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
 * @brief Заголовочный файл таблиц приведения доменных имён — объявления таблицы
 *        преобразований символов, набора кодовых значений преобразований
 *        и таблицы видов соединения символов
 *
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
	 * @brief Пространство имён модуля приведения доменных имён
	 *
	 */
	namespace idna {
		/**
		 * @brief Таблицы приведения доменных имён, порождаемые средством «tools/encoding/idna/generate.py»
		 *
		 */
		// Таблица преобразований символов
		extern __AWH_SHARED_EXPORT__ const mapping_t MAPPINGS[];
		// Число преобразований символов
		extern __AWH_SHARED_EXPORT__ const size_t MAPPINGS_COUNT;
		// Наборы кодовых значений преобразований
		extern __AWH_SHARED_EXPORT__ const uint32_t MAPPING_SETS[];
		// Таблица соединений символов
		extern __AWH_SHARED_EXPORT__ const joining_range_t JOININGS[];
		// Число соединений символов
		extern __AWH_SHARED_EXPORT__ const size_t JOININGS_COUNT;
	};
};
