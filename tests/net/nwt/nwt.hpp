/**
 * @file nwt.hpp
 * @date 2025-12-14
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
 * @brief Заголовочный файл тестовой фикстуры модуля определения типов сетевых адресов —
 *        объявление фикстуры Google Test, общей для проверок набора
 *
 * @copyright Copyright © 2025
 *
 */
 
#pragma once

/**
 * Стандартные заголовочные файлы
 */
/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/net/nwt.hpp"

/**
 * @brief Класс фикстуры для тестов работы со списком параметров URL
 *
 */
class NwtFixture : public testing::Test {};
