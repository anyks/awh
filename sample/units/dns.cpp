/**
 * @file: dns.cpp
 * @date: 2026-03-01
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <chrono>

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/dns.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект узла DNS
	unit::dns_t dns(&fmk, &log);
	// Устанавливаем адрес файла хостов
	dns.setFilenameHosts("../hosts");
	// Устанавливаем адрес файла дампа кэша и интервал сохранения дампа кэша в миллисекундах
	dns.setFilenameDump("../dump.adb", 60000);
	// Запускаем DNS-резолвер
	dns.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
