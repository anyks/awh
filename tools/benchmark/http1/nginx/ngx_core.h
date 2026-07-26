/**
 * @file: ngx_core.h
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Обвязка окружения парсера nginx — ядро сервера
 *
 * @details Модуль `ngx_http_parse.c` подключает три заголовочных файла окружения
 *          сервера. Состав обвязки целиком собран в `ngx_config.h`, а оставшиеся
 *          два заголовочных файла существуют только затем, чтобы исходные тексты
 *          парсера собирались без единого изменения
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_NGINX_CORE__
#define __AWH_BENCHMARK_RIVAL_NGINX_CORE__

/**
 * Подключаем обвязку окружения парсера
 */
#include <ngx_config.h>

#endif // __AWH_BENCHMARK_RIVAL_NGINX_CORE__
