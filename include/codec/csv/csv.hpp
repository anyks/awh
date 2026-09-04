/**
 * @file csv.hpp
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
 * @brief Заголовочный файл контейнера CSV — единая точка подключения потокового чтения
 *        текста, записи его и таблицы, удерживаемой целиком
 *
 * @details Владеющего значения (`value_t`) у CSV нет НАМЕРЕННО, тогда как у прочих
 * шести кодеков оно есть. Причины, по каким оно там заведено, здесь не действуют:
 *
 * 1. Владение содержимым. У древесных кодеков дерево указывает в чужой буфер, и
 *    владеющее значение заводится ради собственной памяти. Таблица CSV владеет сама:
 *    поля лежат в её хранилище знаков, указания на них - в перечне отрезков.
 * 2. Отделяемое поддерево. Вложенности у таблицы нет вовсе - строки и поля, глубина
 *    ровно два, - и отделять от неё нечего.
 * 3. Единый договор. Договор владеющего значения держится на понятиях дерева -
 *    `type`, `at`, `push`, `erase`, `graft`, - и натянуть их на таблицу можно лишь
 *    выдумав вложенность, какой в наречии нет.
 *
 * @note Записано решением, а не оставлено пробелом: разбор 21.08.2026 открывал этот
 *       вопрос заново, и без записи его откроют снова
 *
 * \~english
 * @brief Header file of the CSV container — the single point of the inclusion of the streaming reading
 *        of a text, of the writing of it and of the table held in full
 * @details CSV has no owning value (`value_t`) DELIBERATELY, while the other six containers have it.
 * The reasons for which it is introduced there do not apply here: the table owns its content itself,
 * it has no nesting at all, and the contract of the owning value rests upon the notions of a tree
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CSV__
#define __AWH_CODEC_CSV__

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "document.hpp"
#include "value.hpp"

#endif // __AWH_CODEC_CSV__
