/**
 * @file: ping.cpp
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
#include <net/ping.hpp>
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
	// Создаём объект сетевого ядра
	core_t core(&fmk, &log);
	// Создаём объект ICMP-клиента
	ping_t ping(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("PING");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Выполняем инициализацию подключения
	const double result = ping.ping("api.telegram.org", 10);
	// const double result = ping.ping("127.0.0.1", 10);
	// Выводим результат пинга
	log.print("PING result=%.1f", log_t::flag_t::INFO, result);
	// Выводим результат
	return EXIT_SUCCESS;
}
