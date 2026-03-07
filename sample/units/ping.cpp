/**
 * @file: ping.cpp
 * @date: 2026-03-07
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
 * Подключаем заголовочный файл проекта
 */
#include <units/icmp.hpp>

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
	// Объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Создаём объект узла ICMP-клиента
	unit::icmp_t icmp(event::family_t::IPV4, &fmk, &log);
	// Добавляем удалённый сервер для ICMP-запросов
	icmp.setTarget("api.telegram.org");
	// Если событие ICMP-запроса запущено
	if(icmp.commit()){
		// Устанавливаем функцию обратного вызова на событие получения ответа от ICMP-сервера
		icmp.on <void (const unit::icmp_t::id_t, const uint16_t, const uint64_t, const net::addr_t *)> ("ping", [&addr, &log](const unit::icmp_t::id_t identifier, const uint16_t sequence, const uint64_t elapsed, const net::addr_t * ip) noexcept -> void {
			// Устанавливаем IP-адрес события
			addr.source(ip);
			// Выводим информацию о полученном ответе от удалённого сервера
			log.print("Ответ от %s: icmp_seq=%d time=%dms (ID: %d)", log_t::flag_t::INFO, static_cast <string> (addr).c_str(), sequence, elapsed, identifier);
		}, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
		// Выполняем ICMP-запрос к удалённому серверу
		if(icmp.ping(icmp.issue(), 10, unit::icmp_t::mode_t::SYNC, 3000)){
		
		}
		
		
		#ifdef __AWH_DISABLE__
		
		// Устанавливаем функцию обратного вызова на событие получения ответа от ICMP-сервера
		icmp.on <void (const uint64_t)> ("timestamp", [&chrono, &log](const uint64_t timestamp) noexcept -> void {
			// Выводим информацию о полученном времени
			log.print("Получено дата от ICMP-сервера: %s", log_t::flag_t::INFO, chrono.format(timestamp, "%H:%M:%S %d.%m.%Y").c_str());
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие количества попыток запроса времени к ICMP-серверу
		ntp.on <void (const uint8_t)> ("attempts", [&log](const uint8_t attempts) noexcept -> void {
			// Выводим количество попыток запроса времени к ICMP-серверу
			log.print("Количество попыток запроса времени к ICMP-серверу attempts=%d", log_t::flag_t::WARNING, attempts);
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие ICMP-клиента
		ntp.on <void (const event::status_t)> ("status", [&ntp, &log](const event::status_t status) noexcept -> void {
			/**
			 * В зависимости от статуса события ICMP-клиента выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие ICMP-клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Выводим сообщение о запуске события ICMP-клиента
					log.print("Событие ICMP-клиента было запущено", log_t::flag_t::INFO);
					// Выполняем синхронизацию времени с ICMP-сервером
					if(!ntp.sync(unit::ntp_t::ver_t::V4, 5000))
						// Выводим сообщение об ошибке
						log.print("Не удалось выполнить синхронизацию времени с ICMP-сервером", log_t::flag_t::CRITICAL);
				} break;
				// Если событие ICMP-клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Выводим сообщение об остановке события ICMP-клиента
					log.print("Событие ICMP-клиента было остановлено", log_t::flag_t::INFO);
				break;
			}
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие получения ошибок ICMP-клиента
		ntp.on <void (const event::id_t, const event::error_t, const string &)> ("error", [&log](const event::id_t, const event::error_t error, const string & description) noexcept -> void {
			// Выводим информацию об ошибке
			log.print("ICMP error: %s (code: %d)", log_t::flag_t::CRITICAL, description.c_str(), static_cast <uint16_t> (error));
		}, placeholders::_1, placeholders::_2, placeholders::_3);
		// Запускаем ICMP-клиент
		ntp.start();
		#endif

	// Выводим сообщение об ошибке
	} else log.print("Не удалось запустить событие ICMP-клиента", log_t::flag_t::CRITICAL);
	// Выводим результат
	return EXIT_SUCCESS;
}
