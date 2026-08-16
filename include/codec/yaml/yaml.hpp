/**
 * @file yaml.hpp
 * @date 2026-08-17
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
 * @brief Заголовочный файл контейнера YAML — единая точка подключения потокового чтения
 *        текста, записи его и документа, удерживаемого целиком
 *
 * @note Заводится модуль по частям, и точка эта подключает пока лишь готовое: прочие
 *       заголовочные файлы дописываются сюда по мере их появления
 *
 * \~english
 * @brief Header file of the YAML container — the single point of the inclusion of the streaming reading
 *        of a text, of the writing of it and of the document held in full
 * @note The module is being created by the parts, and this point includes so far only what is ready: the other
 *       header files are added here as they appear
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_YAML__
#define __AWH_CODEC_YAML__

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"
#include "reader.hpp"

#endif // __AWH_CODEC_YAML__
