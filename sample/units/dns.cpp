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
	dns.setHostsAddress("../hosts");
	// Устанавливаем адрес файла дампа кэша и интервал сохранения дампа кэша в миллисекундах
	dns.setDumpAddress("../dump.adb", 60000);
	// Создаём новое событие DNS-резолвера для IPv4
	const event::id_t eid = dns.create(event::family_t::IPV4);
	// Устанавливаем функцию обратного вызова на событие таймера
	dns.on <void (const event::status_t)> ("status", [eid, &dns, &log](const event::status_t status) noexcept -> void {
		/**
		 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
		 */
		switch(static_cast <uint8_t> (status)){
			// Если событие DNS-резолвера запущено
			case static_cast <uint8_t> (event::status_t::LAUNCHED): {
				// Выводим сообщение о запуске события DNS-резолвера
				log.print("Событие DNS-резолвера было запущено", log_t::flag_t::INFO);
				// Если событие DNS-резолвера запущено
				if(dns.commit(eid)){
					// Выполняем резолвинг доменного имени
					if(!dns.resolve(eid, "anyks.com"))
						// Выводим сообщение об ошибке
						log.print("Не удалось выполнить резолвинг доменного имени", log_t::flag_t::CRITICAL);
				// Выводим сообщение об ошибке
				} else log.print("Не удалось запустить событие DNS-резолвера", log_t::flag_t::CRITICAL);
			} break;
			// Если событие DNS-резолвера остановлено
			case static_cast <uint8_t> (event::status_t::DESTROYED):
				// Выводим сообщение об остановке события DNS-резолвера
				log.print("Событие DNS-резолвера было остановлено", log_t::flag_t::INFO);
			break;
		}
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на событие получения ошибок DNS-резолвера
	dns.on <void (const event::id_t, const event::error_t, const string &)> ("error", [&log](const event::id_t, const event::error_t error, const string & description) noexcept -> void {
		// Выводим информацию об ошибке
		log.print("DNS error: %s (code: %d)", log_t::flag_t::CRITICAL, description.c_str(), static_cast <uint16_t> (error));
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Запускаем DNS-резолвер
	dns.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
