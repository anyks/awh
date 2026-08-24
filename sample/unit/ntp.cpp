/**
 * @file ntp.cpp
 * @date 2026-03-06
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
 * @brief Пример работы с NTP-клиентом —
 *        демонстрация синхронизации времени с пулом NTP-серверов и расчёта смещения относительно локальных часов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <unit/ntp.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Объект работы с датой и временем
	chrono_t chrono(&fmk, &log);
	// Создаём объект узла NTP
	unit::ntp_t ntp(&fmk, &log);
	// Устанавливаем количество попыток резолвинга доменного имени
	// ntp.setAttempts(10);
	// Добавляем NTP-сервер для синхронизации времени (фейковый)
	// ntp.addServer("194.190.168.1");
	// Выполняем инициализацию NTP-клиента
	if(ntp.init(event::family_t::IPV4)){
		// Устанавливаем функцию обратного вызова на событие получения времени от NTP-сервера
		ntp.on <void (const uint64_t)> ("timestamp", [&chrono, &log](const uint64_t timestamp) noexcept -> void {
			// Записываем в лог информацию о полученном времени
			log.print("Получено дата от NTP-сервера: %s", log_t::flag_t::INFO, chrono.format(timestamp, "%H:%M:%S %d.%m.%Y").c_str());
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие количества попыток запроса времени к NTP-серверу
		ntp.on <void (const uint8_t)> ("attempts", [&ntp, &log](const uint8_t attempts) noexcept -> void {
			// Переинициализируем клиента
			ntp.init(event::family_t::IPV4);
			// Возвращаем количество попыток запроса времени к NTP-серверу
			log.print("Количество попыток запроса времени к NTP-серверу attempts=%d", log_t::flag_t::WARNING, attempts);
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие NTP-клиента
		ntp.on <void (const event::status_t)> ("status", [&ntp, &log](const event::status_t status) noexcept -> void {
			/**
			 * В зависимости от статуса события NTP-клиента выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие NTP-клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Записываем в лог сообщение о запуске события NTP-клиента
					log.print("Событие NTP-клиента было запущено", log_t::flag_t::INFO);
					// Выполняем синхронизацию времени с NTP-сервером
					if(!ntp.sync(unit::ntp_t::version_t::V4))
						// Записываем ошибку в лог
						log.print("Не удалось выполнить синхронизацию времени с NTP-сервером", log_t::flag_t::CRITICAL);
				} break;
				// Если событие NTP-клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события NTP-клиента
					log.print("Событие NTP-клиента было остановлено", log_t::flag_t::INFO);
				break;
			}
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие получения ошибок NTP-клиента
		ntp.on <void (const event::id_t, const event::error_t, const string &)> ("error", [&log](const event::id_t, const event::error_t error, const string & description) noexcept -> void {
			// Записываем в лог информацию об ошибке
			log.print("NTP error: %s (code: %d)", log_t::flag_t::CRITICAL, description.c_str(), static_cast <uint16_t> (error));
		}, placeholders::_1, placeholders::_2, placeholders::_3);
		// Запускаем NTP-клиент
		ntp.start();
	// Записываем ошибку в лог
	} else log.print("Не удалось запустить событие NTP-клиента", log_t::flag_t::CRITICAL);
	// Возвращаем результат
	return EXIT_SUCCESS;
}
