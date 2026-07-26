/**
 * @file: main.hpp
 * @date: 2025-12-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общий заголовочный файл набора автоматических тестов —
 *        подключение Google Test и Google Mock и настройка глобальных параметров тестового окружения
 *
 * @copyright: Copyright © 2025
 *
 */
 
#ifndef __AWH_TESTS__
#define __AWH_TESTS__

/**
 * Отключаем поддержку POSIX регулярных выражений в Google Test
 */
#define GTEST_HAS_POSIX_RE 0

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#endif // __AWH_TESTS__
