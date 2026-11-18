/**
 * @file common.cpp
 * @date 2026-10-13
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
 * @brief Реализация конструкторов типов данных модуля регулярных выражений,
 *        чьи тела конструкторов недопустимы в заголовке по §5.5
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <regex/common.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Пространство имён модуля регулярных выражений
 */
namespace awh {
	/**
	 * @brief Конструктор
	 *
	 * Пространство имён типов и алгоритмов разбора регулярных выражений
	 *
	 */
	namespace regex {
		/**
		 * @brief Конструктор
		 *
		 * Конструктор по умолчанию
		 *
		 */
		ClassRef::ClassRef() noexcept :
		 negative(false), ranges(0),
		 rangeCount(0), properties(0),
		 propertyCount(0) {}

		/**
		 * @brief Конструктор
		 *
		 * Конструктор по умолчанию
		 *
		 */
		Range::Range() noexcept : end(0), begin(0) {}

		/**
		 * @brief Конструктор
		 *
		 * Конструктор по нижней и верхней границам
		 *
		 * @param begin нижняя граница диапазона включительно
		 * @param end   верхняя граница диапазона включительно
		 *
		 */
		Range::Range(const uint32_t begin, const uint32_t end) noexcept : end(end), begin(begin) {}

		/**
		 * Конструктор по умолчанию
		 */
		Property::Property() noexcept : id(0), negative(false) {}

		/**
		 * @brief Конструктор
		 *
		 * Конструктор по идентификатору и признаку отрицания
		 *
		 * @param id       идентификатор свойства Юникода
		 * @param negative флаг отрицания свойства
		 *
		 */
		Property::Property(const uint16_t id, const bool negative) noexcept : id(id), negative(negative ? 1 : 0) {}

		/**
		 * @brief Конструктор
		 *
		 * Конструктор по умолчанию
		 *
		 */
		Class::Class() noexcept : negative(false) {}

		/**
		 * @brief Конструктор
		 *
		 * Конструктор по умолчанию
		 *
		 */
		ClassView::ClassView() noexcept : negative(false) {}
	};
};
