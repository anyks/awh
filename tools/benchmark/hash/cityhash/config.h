/**
 * @file config.h
 * @date 2026-07-31
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
 * @brief Обвязка сборки библиотеки CityHash — заголовочный файл параметров сборки,
 *        подставляемый вместо порождаемого сценарием configure подмодуля
 *
 * @details Исходные тексты подмодуля не изменяются: вместо запуска сценария
 *          configure, которому для порождения config.h нужны autotools, стенд
 *          подставляет этот файл каталогом заголовочных файлов. Библиотека
 *          спрашивает у сборки ровно два свойства - порядок байтов процессора
 *          и наличие подсказки предсказателю переходов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_CITYHASH_CONFIG__
#define __AWH_BENCHMARK_RIVAL_CITYHASH_CONFIG__

/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#if defined(__GNUC__) || defined(__clang__)
	/**
	 * Разрешаем подсказку предсказателю переходов
	 */
	#define HAVE_BUILTIN_EXPECT 1
#else
	/**
	 * Запрещаем подсказку предсказателю переходов
	 */
	#define HAVE_BUILTIN_EXPECT 0
#endif

/**
 * Если порядок байтов процессора от старшего байта к младшему
 */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	/**
	 * Сообщаем библиотеке о порядке байтов от старшего к младшему
	 */
	#define WORDS_BIGENDIAN 1
#endif

#endif // __AWH_BENCHMARK_RIVAL_CITYHASH_CONFIG__
