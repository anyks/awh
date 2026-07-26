/**
 * @file: ping.cpp
 * @date: 2026-03-07
 * @license: LicenseRef-AWH-1.0
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
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Объект работы с датой и временем
	chrono_t chrono(&fmk, &log);
	// Объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Создаём объект узла ICMP-клиента
	unit::icmp_t icmp(&fmk, &log);
	// Добавляем удалённый сервер для ICMP-запросов
	icmp.setTarget("api.telegram.org");
	// Устанавливаем функцию обратного вызова на событие получения ответа от ICMP-сервера
	icmp.on <void (const unit::icmp_t::id_t, const unit::icmp_t::response_t &)> ("ping", [&addr, &chrono, &log](const unit::icmp_t::id_t identifier, const unit::icmp_t::response_t & response) noexcept -> void {
		// Лейбл единиц измерений
		string label = "";
		// Получаем аббревиатуру даты
		const auto & abbr = chrono.abbreviation(response.elapsed);
		/**
		 * Определяем тип аббревиатуры
		 */
		switch(static_cast <uint8_t> (abbr.first)){
			// Если мы получили год
			case static_cast <uint8_t> (chrono_t::type_t::YEAR):
				// Устанавливаем лейбл единиц измерений
				label = "year";
			break;
			// Если мы получили месяц
			case static_cast <uint8_t> (chrono_t::type_t::MONTH):
				// Устанавливаем лейбл единиц измерений
				label = "month";
			break;
			// Если мы получили неделя
			case static_cast <uint8_t> (chrono_t::type_t::WEEK):
				// Устанавливаем лейбл единиц измерений
				label = "week";
			break;
			// Если мы получили день
			case static_cast <uint8_t> (chrono_t::type_t::DAY):
				// Устанавливаем лейбл единиц измерений
				label = "day";
			break;
			// Если мы получили час
			case static_cast <uint8_t> (chrono_t::type_t::HOUR):
				// Устанавливаем лейбл единиц измерений
				label = "hour";
			break;
			// Если мы получили минуты
			case static_cast <uint8_t> (chrono_t::type_t::MINUTES):
				// Устанавливаем лейбл единиц измерений
				label = "min";
			break;
			// Если мы получили секунды
			case static_cast <uint8_t> (chrono_t::type_t::SECONDS):
				// Устанавливаем лейбл единиц измерений
				label = "sec";
			break;
			// Если мы получили миллисекунды
			case static_cast <uint8_t> (chrono_t::type_t::MILLISECONDS):
				// Устанавливаем лейбл единиц измерений
				label = "msec";
			break;
		}
		// Устанавливаем IP-адрес события
		addr.source(response.address);
		// Записываем в лог информацию о полученном ответе от удалённого сервера (добавить размер отправляемого пакета)
		// log.print("Ответ от %s: icmp_seq=%d time=%dms (ID: %d)", log_t::flag_t::INFO, static_cast <string> (addr).c_str(), sequence, elapsed, identifier);
		log.print("%zu bytes from %s: icmp_seq=%u ttl=%u time=%.1f %s", log_t::flag_t::INFO, response.size, static_cast <string> (addr).c_str(), response.sequence, response.timeToLive, abbr.second, label.c_str());
	}, placeholders::_1, placeholders::_2);
	// Выполняем ICMP-запрос к удалённому серверу
	if(icmp.ping(icmp.issue(), 10, unit::icmp_t::mode_t::SYNC)){
		// Выполняем инициализацию ICMP-клиента
		if(icmp.init(event::family_t::IPV4)){
			// Устанавливаем функцию обратного вызова на событие ICMP-клиента
			icmp.on <void (const event::status_t)> ("status", [&icmp, &log](const event::status_t status) noexcept -> void {
				/**
				 * В зависимости от статуса события ICMP-клиента выполняем определённые действия
				 */
				switch(static_cast <uint8_t> (status)){
					// Если событие ICMP-клиента запущено
					case static_cast <uint8_t> (event::status_t::LAUNCHED): {
						// Записываем в лог сообщение о запуске события ICMP-клиента
						log.print("Событие ICMP-клиента было запущено", log_t::flag_t::INFO);
						// Выполняем проверку существования удалённого сервера
						if(!icmp.ping(icmp.issue(), 10, unit::icmp_t::mode_t::ASYNC))
							// Записываем ошибку в лог
							log.print("Не удалось проверить существование удалённого сервера", log_t::flag_t::CRITICAL);
					} break;
					// Если событие ICMP-клиента остановлено
					case static_cast <uint8_t> (event::status_t::DESTROYED):
						// Записываем в лог сообщение об остановке события ICMP-клиента
						log.print("Событие ICMP-клиента было остановлено", log_t::flag_t::INFO);
					break;
				}
			}, placeholders::_1);
			// Устанавливаем функцию обратного вызова на событие получения ошибок ICMP-клиента
			icmp.on <void (const event::id_t, const event::error_t, const string &)> ("error", [&log](const event::id_t, const event::error_t error, const string & description) noexcept -> void {
				// Записываем в лог информацию об ошибке
				log.print("ICMP error: %s (code: %d)", log_t::flag_t::CRITICAL, description.c_str(), static_cast <uint16_t> (error));
			}, placeholders::_1, placeholders::_2, placeholders::_3);
			// Запускаем ICMP-клиент
			icmp.start();
		// Записываем ошибку в лог
		} else log.print("Не удалось запустить событие ICMP-клиента", log_t::flag_t::CRITICAL);
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
