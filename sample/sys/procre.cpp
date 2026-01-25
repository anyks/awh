/**
 * @file: procre.cpp
 * @date: 2026-01-26
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
 * Подключаем заголовочные файлы проекта
 */
#include <sys/procre.hpp>

// Подключаем пространство имён
using namespace awh;

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект резольвера процессов
	procre_t procre(&log);
	// Устанавливаем название сервиса
	log.name("Process Resolver");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Если количество параметров больше одного
	if(argc > 1){
		// Выполняем получение пида
		const pid_t pid = static_cast <pid_t> (::stoi(argv[1]));
		// Выводим в лог название приложения
		log.print("Process Resolver: NAME=%s", log_t::flag_t::INFO, procre.name(pid).c_str());
	// Выводим в лог название текущего проекта
	} else log.print("Process Resolver: NAME=%s", log_t::flag_t::INFO, procre.name().c_str());
	// Выводим результат
	return EXIT_SUCCESS;
}
