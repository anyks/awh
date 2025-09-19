/**
 * @file: ntp.cpp
 * @date: 2025-03-02
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/ntp.hpp>
#include <core/core.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

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
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект работы с датой и временем
	chrono_t chrono(&fmk);
	// Создаём объект NTP-клиента
	ntp_t ntp(&fmk, &log);
	// Создаём объект сетевого ядра
	core_t core(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("NTP");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Выполняем установку серверов имён
	ntp.ns({"77.88.8.88", "77.88.8.2"});
	// Выполняем установку списка сервером времени
	ntp.servers({"0.ru.pool.ntp.org", "1.ru.pool.ntp.org", "2.ru.pool.ntp.org", "3.ru.pool.ntp.org"});
	// Выполняем запрос на получение первого времени
	log.print("Time: %s", log_t::flag_t::INFO, chrono.format(ntp.request(), "%H:%M:%S %d.%m.%Y").c_str());
	// Выводим результат
	return EXIT_SUCCESS;
}
