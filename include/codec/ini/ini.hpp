/**
 * @file ini.hpp
 * @date 2026-08-09
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
 * @brief Заголовочный файл контейнера INI — единая точка подключения потокового чтения
 *        текста настроек и приведения его к кодировке UTF-8
 *
 * \~english
 * @brief Header file of the INI container — the single point of the inclusion of the streaming reading
 *        of a settings text and of its conversion to the UTF-8 encoding
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_INI__
#define __AWH_CODEC_INI__

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "document.hpp"

#endif // __AWH_CODEC_INI__
