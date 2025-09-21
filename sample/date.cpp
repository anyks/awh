/**
 * @file: date.cpp
 * @date: 2025-03-23
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
#include <sys/log.hpp>
#include <sys/chrono.hpp>

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
	// Создаём объект Chrono
	chrono_t chrono(&fmk);
	// Выполняем парсинг даты
	uint64_t date = chrono.parse("2023-03-05T12:55:58.0490925Z", "%Y-%m-%dT%H:%M:%S.%s%Z");
	// Формируем дату в виде читаемом виде
	string result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("2024-08-06T11:08:55.101Z", "%Y-%m-%dT%H:%M:%S.%s%Z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("2024-08-06T14:47:34.741876+03:00", "%Y-%m-%dT%H:%M:%S.%s%o");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("2024-08-06T14:47:34.728093306+03:0", "%Y-%m-%dT%H:%M:%S.%s%o");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("7/26/2023 2:39:42 PM", "%m/%d/%Y %I:%M:%S %p");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("2023-07-26T14:39:4", "%Y-%m-%dT%H:%M:%S");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("7/26/2023 2:39:42 PM (2934007)", "%m/%d/%Y %I:%M:%S %p (%s)");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("2024-11-15 17:14:03,331", "%Y-%m-%d %H:%M:%S,%s");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Jun 11 09:56:56", "%h %d %H:%M:%S");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Tue Jul 16 10:45:40.020399 2024", "%a %h %d %H:%M:%S.%s %Y");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("05/Apr/2023:12:45:12.345678901 +0300", "%d/%h/%Y:%H:%M:%S.%s %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Aug 13 17:43:12", "%h %d %H:%M:%S");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("2024-10-16 10:30:45.789", "%Y-%m-%d %H:%M:%S.%s");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("[18/Jul/2024:13:34:00 +0300]", "%d/%h/%Y:%H:%M:%S %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("2024/07/18 13:33:17", "%Y/%m/%d %H:%M:%S");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("17.07.2023 13:25:53", "%d.%m.%Y %H:%M:%S");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("[04-22 13:54:55.343240]", "%m-%d %H:%M:%S.%s");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("17:54:49", "%H:%M:%S");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 19 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 19 2025 15:51:10", "%a %h %e %Y %H:%M:%S %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 19 15:51:10 GMT+0300", "%a %h %e %H:%M:%S %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 20 19:56:10 GMT+0300", "%a %h %e %H:%M:%S %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 20 19:56:10", "%a %h %e %H:%M:%S %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 20 19:56:10 GMT+0430", "%a %h %e %H:%M:%S %z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 30 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %Z%z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Dec 03 12:00 MSK+0332", "%h %d %H:%M %Z%z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("Wed Mar 31 00:51:10 11 GMT+0300 080", "%a %h %e %H:%M:%S %W %z %j");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	// Выполняем парсинг даты
	date = chrono.parse("20050809T183142+0330", "%Y%m%dT%H%M%S%z");
	// Формируем дату в виде читаемом виде
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	// Выводим сформированный результат даты
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);
	// Выводим результат
	return EXIT_SUCCESS;
}
