/**
 * @file syslog.hpp
 * @date 2026-09-07
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
 * @brief Заголовочный файл контейнера SysLog — единой точки включения потокового чтения записей,
 *        записи событий, словаря источников сообщений и события, удерживаемого целиком
 * @details Своего владеющего значения (`value_t`) у контейнера SysLog НЕТ, и это решение, а не
 * пробел: основанием событию служит дерево контейнера ABC
 *
 * \~english
 * @brief Header file of the SysLog container — the single point of the inclusion of the streaming reading
 *        of the records, of the writing of the events, of the dictionary of the sources and of an event held in full
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
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "document.hpp"
#include "dictionary.hpp"
