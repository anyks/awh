/**
 * @file dns.cpp
 * @date 2026-03-01
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
 * @brief Пример работы с DNS-резолвером — демонстрация асинхронного разрешения доменных имён по записям различных
 *        типов с использованием собственного списка DNS-серверов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/dns.hpp>

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
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Создаём объект узла DNS
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Устанавливаем путь к файлу хостов
	dns.setHostsAddress("../hosts");
	// Устанавливаем путь к файлу дампа кэша и интервал сохранения дампа кэша в миллисекундах
	dns.setDumpAddress("../dump.adb", 60000);
	// Устанавливаем количество попыток резолвинга доменного имени
	// dns.setAttempts(10);
	// Добавляем DNS-сервер для резолвинга доменных имён (фейковый)
	// dns.addServer("127.0.0.1");
	// Устанавливаем функцию обратного вызова на событие получения канонического имени при резолвинге доменного имени
	dns.on <void (const unit::dns_t::id_t, const unordered_multimap <string, string> &)> ("cname", [&log](const unit::dns_t::id_t did, const unordered_multimap <string, string> & cname) noexcept -> void {
		/**
		 * Записываем в лог информацию об успешно резолвленных канонических именах
		 */
		for(const auto & [fqdn, ip] : cname)
			// Записываем в лог информацию об успешно резолвленном каноническом имени
			log.print("Успешно резолвлено каноническое имя %s в IP-адрес: %s (ID: %u)", log_t::flag_t::INFO, fqdn.c_str(), ip.c_str(), did);
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения IP-адреса при резолвинге доменного имени
	dns.on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", [&addr, &log](const unit::dns_t::id_t did, const event::family_t family, const string & domain, const net::addr_t * ip) noexcept -> void {
		// Устанавливаем IP-адрес события
		addr.source(ip);
		// Записываем в лог информацию об успешно резолвленном доменном имени
		log.print("Успешно резолвлено доменное имя %s в IP-адрес: %s (%s) (ID: %u)", log_t::flag_t::INFO, domain.c_str(), static_cast <string> (addr).c_str(), (family == event::family_t::IPV4 ? "IPv4" : "IPv6"), did);
	}, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4);
	// Устанавливаем функцию обратного вызова на событие количества попыток резолвинга доменного имени
	dns.on <void (const unit::dns_t::id_t, const string &, const uint8_t)> ("attempts", [&log](const unit::dns_t::id_t did, const string & domain, const uint8_t attempts) noexcept -> void {
		// Возвращаем количество попыток резолвинга доменного имени
		log.print("Количество попыток резолвинга доменного имени '%s': attempts=%d (ID: %u)", log_t::flag_t::WARNING, domain.c_str(), attempts, did);
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие DNS-резолвера
	dns.on <void (const event::status_t)> ("status", [&dns, &log](const event::status_t status) noexcept -> void {
		/**
		 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
		 */
		switch(static_cast <uint8_t> (status)){
			// Если событие DNS-резолвера запущено
			case static_cast <uint8_t> (event::status_t::LAUNCHED): {
				// Записываем в лог сообщение о запуске события DNS-резолвера
				log.print("Событие DNS-резолвера было запущено", log_t::flag_t::INFO);
				// Кодируем доменное имя в Punycode
				log.print("Доменное имя закодированно: %s", log_t::flag_t::INFO, dns.encode("ремпрофи.рф").c_str());
				// Декодируем доменное имя из Punycode
				log.print("Доменное имя декодированно: %s", log_t::flag_t::INFO, dns.decode("xn--e1agliedd7a.xn--p1ai").c_str());
				// Выполняем пересортировку адресов в кэше для доменного имени
				dns.shuffle(event::family_t::IPV4, "www.google.com");
				// Выполняем поиск доменного имени соответствующему IP-адресу
				if(!dns.search(dns.issue(), event::family_t::IPV4, "77.88.44.242", 3000))
					// Записываем ошибку в лог
					log.print("Не удалось выполнить поиск доменного имени", log_t::flag_t::CRITICAL);
				// Выполняем резолвинг доменного имени
				if(!dns.resolve(dns.issue(), event::family_t::IPV4, "www.google.com", 3000))
					// Записываем ошибку в лог
					log.print("Не удалось выполнить резолвинг доменного имени", log_t::flag_t::CRITICAL);
				// Выполняем резолвинг доменного имени
				if(!dns.resolve(dns.issue(), event::family_t::IPV4, "gitlab.pgr.local", 3000))
					// Записываем ошибку в лог
					log.print("Не удалось выполнить резолвинг доменного имени", log_t::flag_t::CRITICAL);
				// Выполняем запрос на получение записей CNAME
				if(!dns.request(dns.issue(), unit::dns_t::record_t::CNAME, "dns.rambler-co.ru", 3000))
					// Записываем ошибку в лог
					log.print("Не удалось выполнить запрос на получение CNAME доменного имени", log_t::flag_t::CRITICAL);
				// Выполняем запрос на получение всех записей доменного имени
				if(!dns.request(dns.issue(), unit::dns_t::record_t::ANY, "ya.ru", 3000))
					// Записываем ошибку в лог
					log.print("Не удалось выполнить запрос на получение всех записей доменного имени", log_t::flag_t::CRITICAL);
				// Выполняем запрос на получение записей CNAME
				if(!dns.request(dns.issue(), unit::dns_t::record_t::CNAME, "www.google.com", 3000))
					// Записываем ошибку в лог
					log.print("Не удалось выполнить запрос на получение CNAME доменного имени", log_t::flag_t::CRITICAL);
			} break;
			// Если событие DNS-резолвера остановлено
			case static_cast <uint8_t> (event::status_t::DESTROYED):
				// Записываем в лог сообщение об остановке события DNS-резолвера
				log.print("Событие DNS-резолвера было остановлено", log_t::flag_t::INFO);
			break;
		}
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на событие получения ошибок DNS-резолвера
	dns.on <void (const event::id_t, const event::error_t, const string &)> ("error", [&log](const event::id_t, const event::error_t error, const string & description) noexcept -> void {
		// Записываем в лог информацию об ошибке
		log.print("DNS error: %s (code: %d)", log_t::flag_t::CRITICAL, description.c_str(), static_cast <uint16_t> (error));
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Запускаем DNS-резолвер
	dns.start();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
