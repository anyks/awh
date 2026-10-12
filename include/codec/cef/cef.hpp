/**
 * @file cef.hpp
 * @date 2026-09-04
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
 * @brief Заголовочный файл контейнера CEF — единая точка включения потокового чтения записей,
 *        записи событий, словаря расширений и события, удерживаемого целиком
 *
 * @details Своего владеющего значения (`value_t`) у контейнера CEF НЕТ, и это решение, а не
 *          пробел: основанием событию служит дерево контейнера ABC. Система видов его вмещает виды
 *          записи CEF с запасом, а ходы обхода и правки даны им уже; заводить поверх второй владеющий
 *          вид значило бы держать один договор в двух местах и переводить дерево в дерево на всяком
 *          обращении. Довод записан здесь, а не оставлен молчанием: разбор состава кодеков вопрос этот
 *          откроет, и без записи его откроют снова.
 *
 * \~english
 * @brief Header file of the CEF container — the single point of the inclusion of the streaming reading
 *        of the records, of the writing of the events, of the dictionary of the extensions and of an event held in full
 *
 * @details The CEF container has NO owning value (`value_t`) of its own, and this is a decision rather than
 *          a gap: a tree of the ABC container serves as the foundation of an event. Its system of kinds
 *          covers the kinds of the CEF record with a margin, and the methods of the walk and of the editing
 *          are already given by it; to raise a second owning kind on top of them would mean keeping one
 *          contract in two places and translating tree into tree on every access. The argument is written
 *          down here rather than left in silence: the review of the composition of the codecs will open this
 *          question, and without this record it will be opened again.
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
