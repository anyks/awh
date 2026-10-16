/**
 * @file common.cpp
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
 * @brief Файл реализации общих типов модуля Grok —
 *        коды ошибок разбора шаблона, виды значений полей, описание поля
 *        и собранный шаблон
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <regex/grok/common.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::grok::Field::Field() noexcept :
 number(0), kind(kind_t::TEXT) {}

/**
 * @brief Конструктор
 *
 */
awh::grok::Value::Value() noexcept :
 kind(kind_t::TEXT) {}

/**
 * @brief Конструктор
 *
 */
awh::grok::Expression::Expression() noexcept :
 pattern{""}, expression{""} {}
