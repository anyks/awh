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
 * Подключаем заголовочные файлы проекта
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
	 * Пространство имён типов и алгоритмов разбора регулярных выражений
	 */
	namespace regex {
		/**
		 * Конструктор по умолчанию
		 */
		Range::Range() noexcept : begin(0), end(0) {}

		/**
		 * Конструктор по нижней и верхней границам
		 *
		 * @param begin нижняя граница диапазона включительно
		 * @param end   верхняя граница диапазона включительно
		 */
		Range::Range(const uint32_t begin, const uint32_t end) noexcept : begin(begin), end(end) {}

		/**
		 * Конструктор по умолчанию
		 */
		Property::Property() noexcept : id(0), negative(false) {}

		/**
		 * Конструктор по идентификатору и признаку отрицания
		 *
		 * @param id       идентификатор свойства Юникода
		 * @param negative признак отрицания свойства Юникода
		 */
		Property::Property(const uint16_t id, const bool negative) noexcept : id(id), negative(negative ? 1 : 0) {}

		/**
		 * Конструктор по умолчанию
		 */
		Class::Class() noexcept : negative(false) {}

		/**
		 * Конструктор по умолчанию
		 */
		ClassRef::ClassRef() noexcept : negative(false), ranges(0), rangeCount(0), properties(0), propertyCount(0) {}

		/**
		 * Конструктор по умолчанию
		 */
		ClassView::ClassView() noexcept : negative(false) {}
	};
};
