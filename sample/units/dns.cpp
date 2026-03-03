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
	// Устанавливаем количество попыток резолвинга доменного имени
	dns.setAttempts(10);
	// Устанавливаем таймаут ожидания ответа от DNS-сервера в миллисекундах
	dns.setTimeout(eid, 3000);
	// Добавляем DNS-сервер для резолвинга доменных имён (фейковый)
	// dns.addServer(eid, "127.0.0.1");
	// Устанавливаем функцию обратного вызова на событие получения канонического имени при резолвинге доменного имени
	dns.on <void (const event::id_t, const unordered_multimap <string, string> &)> ("cname", [&log](const event::id_t eid, const unordered_multimap <string, string> & cname) noexcept -> void {
		// Выводим информацию об успешно резолвленных канонических именах
		for(const auto & [fqdn, ip] : cname)
			// Выводим информацию об успешно резолвленном каноническом имени
			log.print("Успешно резолвлено каноническое имя %s в IP-адрес: %s", log_t::flag_t::INFO, fqdn.c_str(), ip.c_str());
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения IP-адреса при резолвинге доменного имени
	dns.on <void (const event::id_t, const event::family_t, string_view, string_view)> ("address", [&log](const event::id_t eid, const event::family_t family, string_view fqdn, string_view ip) noexcept -> void {
		// Выводим информацию об успешно резолвленном доменном имени
		log.print("Успешно резолвлено доменное имя %s в IP-адрес: %s (%s)", log_t::flag_t::INFO, fqdn.data(), ip.data(), (family == event::family_t::IPV4 ? "IPv4" : "IPv6"));
	}, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
	// Устанавливаем функцию обратного вызова на событие количества попыток резолвинга доменного имени
	dns.on <void (const event::id_t, const uint8_t)> ("attempts", [&log](const event::id_t eid, const uint8_t attempts) noexcept -> void {
		// Выводим количество попыток резолвинга доменного имени
		log.print("Количество попыток резолвинга доменного имени: %d", log_t::flag_t::INFO, attempts);
	}, placeholders::_1, placeholders::_2);
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
					// Кодируем доменное имя в Punycode
					log.print("Доменное имя закодированно: %s", log_t::flag_t::INFO, dns.encode("ремпрофи.рф").c_str());
					// Декодируем доменное имя из Punycode
					log.print("Доменное имя декодированно: %s", log_t::flag_t::INFO, dns.decode("xn--e1agliedd7a.xn--p1ai").c_str());
					// Выполняем пересортировку адресов в кэше для доменного имени
					dns.shuffle(event::family_t::IPV4, "www.google.com");
					// Выполняем поиск доменного имени соответствующему IP-адресу
					if(!dns.search(eid, event::family_t::IPV4, "77.88.44.242"))
						// Выводим сообщение об ошибке
						log.print("Не удалось выполнить поиск доменного имени", log_t::flag_t::CRITICAL);
					// Выполняем резолвинг доменного имени
					if(!dns.resolve(eid, event::family_t::IPV4, "www.google.com"))
						// Выводим сообщение об ошибке
						log.print("Не удалось выполнить резолвинг доменного имени", log_t::flag_t::CRITICAL);
					// Выполняем резолвинг доменного имени
					if(!dns.resolve(eid, event::family_t::IPV4, "gitlab.pgr.local"))
						// Выводим сообщение об ошибке
						log.print("Не удалось выполнить резолвинг доменного имени", log_t::flag_t::CRITICAL);
					// Выполняем запрос на получение записей CNAME
					if(!dns.request(eid, unit::dns_t::record_t::CNAME, "dns.rambler-co.ru"))
						// Выводим сообщение об ошибке
						log.print("Не удалось выполнить запрос на получение CNAME доменного имени", log_t::flag_t::CRITICAL);
					// Выполняем запрос на получение всех записей доменного имени
					if(!dns.request(eid, unit::dns_t::record_t::ANY, "ya.ru"))
						// Выводим сообщение об ошибке
						log.print("Не удалось выполнить запрос на получение всех записей доменного имени", log_t::flag_t::CRITICAL);
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
