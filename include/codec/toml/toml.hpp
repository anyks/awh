/**
 * @file toml.hpp
 * @date 2026-08-12
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
 * @brief Заголовочный файл контейнера TOML — единая точка подключения потокового чтения
 *        текста настроек, записи его и дерева настроек
 *
 * \~english
 * @brief Header file of the TOML container — the single point of the inclusion of the streaming reading
 *        of a settings text, of the writing of it and of the settings tree
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_TOML__
#define __AWH_CODEC_TOML__

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "document.hpp"
#include "value.hpp"

#endif // __AWH_CODEC_TOML__
