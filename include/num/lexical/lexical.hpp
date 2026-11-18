/**
 * @file lexical.hpp
 * @date 2026-07-22
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
 * @brief Заголовочный файл модуля конвертации чисел из строкового представления — функции разбора
 *        целых чисел и чисел с плавающей точкой без выделения памяти и без исключений с побитовым совпадением
 *        результата с strtod в локали «C»
 *
 * \~english
 * @brief Header file of the module converting numbers from their string representation — the parsing functions
 *        of integers and floating-point numbers without allocating memory and without exceptions, with a bit-exact match
 *        of the result with strtod in the «C» locale
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Подключаем заголовочные файлы модуля
 */
#include "api.hpp"
