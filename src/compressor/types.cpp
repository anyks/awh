/**
 * @file: types.cpp
 * @date: 2026-07-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация общих типов подсистемы компрессии —
 *        конструирование и инициализация структуры параметров потоковой сессии значениями по умолчанию
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <compressor/types.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::compressor::Params::Params() noexcept : wbits(15), level(0) {}
