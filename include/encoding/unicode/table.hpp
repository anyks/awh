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
 * @brief Заголовочный файл таблиц базы данных символов Юникода — объявления таблиц
 *        общих категорий, письменностей, их расширений, двоичных свойств, классов
 *        двунаправленности, приведения регистра, графемных кластеров и сочетаний
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
	 * @brief Пространство имён модуля Юникода
	 *
	 */
	namespace unicode {
		/**
		 * @brief Таблицы свойств Юникода, порождаемые средством «tools/encoding/unicode/generate.py»
		 *
		 */
		// Диапазоны категорий символов
		extern __AWH_SHARED_EXPORT__ const interval_t CATEGORIES[];
		// Число диапазонов категорий
		extern __AWH_SHARED_EXPORT__ const size_t CATEGORIES_COUNT;
		// Диапазоны письменностей
		extern __AWH_SHARED_EXPORT__ const interval_t SCRIPTS[];
		// Число диапазонов письменностей
		extern __AWH_SHARED_EXPORT__ const size_t SCRIPTS_COUNT;
		// Диапазоны расширений письменностей
		extern __AWH_SHARED_EXPORT__ const interval_t EXTENSIONS[];
		// Число расширений письменностей
		extern __AWH_SHARED_EXPORT__ const size_t EXTENSIONS_COUNT;
		// Диапазоны классов двунаправленности
		extern __AWH_SHARED_EXPORT__ const interval_t BIDIRECTIONAL[];
		// Число классов двунаправленности
		extern __AWH_SHARED_EXPORT__ const size_t BIDIRECTIONAL_COUNT;
		// Диапазоны двоичных свойств
		extern __AWH_SHARED_EXPORT__ const interval_t BINARIES[];
		// Число диапазонов двоичных свойств
		extern __AWH_SHARED_EXPORT__ const size_t BINARIES_COUNT;
		// Наборы номеров письменностей расширений
		extern __AWH_SHARED_EXPORT__ const uint16_t EXTENSION_SETS[];
		// Смещения наборов письменностей расширений
		extern __AWH_SHARED_EXPORT__ const uint32_t EXTENSION_OFFSETS[];
		// Участки двоичных свойств
		extern __AWH_SHARED_EXPORT__ const uint32_t BINARY_SPANS[];
		// Число участков двоичных свойств
		extern __AWH_SHARED_EXPORT__ const size_t BINARY_COUNT;
		// Таблица имён символов
		extern __AWH_SHARED_EXPORT__ const naming_t NAMES[];
		// Число имён символов
		extern __AWH_SHARED_EXPORT__ const size_t NAMES_COUNT;
		// Таблица свёрток регистра
		extern __AWH_SHARED_EXPORT__ const folding_t FOLDING[];
		// Число свёрток регистра
		extern __AWH_SHARED_EXPORT__ const size_t FOLDING_COUNT;
		// Диапазоны орбит символов
		extern __AWH_SHARED_EXPORT__ const interval_t ORBITS[];
		// Число орбит символов
		extern __AWH_SHARED_EXPORT__ const size_t ORBITS_COUNT;
		// Наборы орбит символов
		extern __AWH_SHARED_EXPORT__ const uint32_t ORBIT_SETS[];
		// Диапазоны кластеров символов
		extern __AWH_SHARED_EXPORT__ const interval_t CLUSTERS[];
		// Число кластеров символов
		extern __AWH_SHARED_EXPORT__ const size_t CLUSTERS_COUNT;
		// Диапазоны индийских слоговых свойств
		extern __AWH_SHARED_EXPORT__ const interval_t INDIC[];
		// Число индийских слоговых свойств
		extern __AWH_SHARED_EXPORT__ const size_t INDIC_COUNT;
		// Таблица комбинирующих классов
		extern __AWH_SHARED_EXPORT__ const interval_t COMBINING[];
		// Число комбинирующих классов
		extern __AWH_SHARED_EXPORT__ const size_t COMBINING_COUNT;
		// Таблица разложений символов
		extern __AWH_SHARED_EXPORT__ const decomposition_t DECOMPOSITIONS[];
		// Число разложений символов
		extern __AWH_SHARED_EXPORT__ const size_t DECOMPOSITIONS_COUNT;
		// Наборы кодовых значений разложений
		extern __AWH_SHARED_EXPORT__ const uint32_t DECOMPOSITION_SETS[];
		// Таблица составлений символов
		extern __AWH_SHARED_EXPORT__ const composition_t COMPOSITIONS[];
		// Число составлений символов
		extern __AWH_SHARED_EXPORT__ const size_t COMPOSITIONS_COUNT;
	};
};
