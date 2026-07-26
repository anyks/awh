/**
 * @file: main.cpp
 * @date: 2025-12-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Точка входа набора автоматических тестов библиотеки — инициализация Google Test и Google Mock,
 *        разбор параметров командной строки и запуск всех зарегистрированных наборов тестов
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файл главного модуля тестов
 */
#include "main.hpp"

/**
 * @brief Главная функция тестового приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char ** argv){
	// Инициализируем Google Test и Google Mock
	::testing::InitGoogleTest(&argc, argv);
	::testing::InitGoogleMock(&argc, argv);

	// Запускаем все тесты
	return RUN_ALL_TESTS();
}
