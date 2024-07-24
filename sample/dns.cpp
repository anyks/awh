/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/dns.hpp>
#include <core/core.hpp>

// Подключаем пространство имён
using namespace std;
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
	// Создаём биндинг
	dns_t dns(&fmk, &log);
	// Создаём объект сетевого ядра
	core_t core(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("DNS");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Устанавливаем префикс переменной окружения
	dns.prefix("AWH");
	// Выполняем установку серверов имён
	dns.servers({"77.88.8.88", "77.88.8.2"});
	/**
	 * sudo tcpdump host 172.30.253.105 -A
	 * sudo tcpdump host 77.88.8.88 or 77.88.8.2 -XX
	 */
	// Выполняем установку серверов имён
	// dns.servers({"172.30.253.105"});
	// Список локальных доменов для проведения сложного тестирования
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("GITLAB.pgr.local").c_str());
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("gitlab.pgr.LOCAL").c_str());
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("GITLAB.pgr.LOCAL").c_str());
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("gitlab.PGR.local").c_str());
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("GITLAB.PGR.local").c_str());
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("gitlab.PGR.LOCAL").c_str());
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("GITLAB.PGR.LOCAL").c_str());
	// log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("gitlab.pgr.local").c_str());
	// Выполняем запрос на получение первого IP-адреса
	log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("localhost").c_str());
	// Выполняем запрос на получение второго IP-адреса
	log.print("IP2: %s", log_t::flag_t::INFO, dns.resolve("yandex.ru").c_str());
	// Выполняем запрос на получение третьего IP-адреса
	log.print("IP3: %s", log_t::flag_t::INFO, dns.resolve("google.com").c_str());
	/**
	 * Выполняем запрос на получение четвертого IP-адреса
	 * Пример переменной окружения: $ export AWH_DNS_IPV4_STALIN_INFO=255.255.255.222
	 */
	log.print("IP4: %s", log_t::flag_t::INFO, dns.resolve("stalin.info").c_str());
	// Выполняем очистку кэша
	dns.flush();
	// Выполняем поиск доменных имён принадлежащим IP-адресу yandex.ru
	const auto & yandex = dns.search("77.88.55.60");
	// Если список доменных имён получен
	if(!yandex.empty()){
		// Выполняем перебор всего списка доменных имён
		for(auto & domain : yandex)
			// Выводим доменное имя
			log.print("Domain: %s => %s", log_t::flag_t::INFO, domain.c_str(), "77.88.55.60");
	}
	// Выполняем поиск доменных имён принадлежащим IP-адресу google.com
	const auto & google = dns.search("74.125.131.139");
	// Если список доменных имён получен
	if(!google.empty()){
		// Выполняем перебор всего списка доменных имён
		for(auto & domain : google)
			// Выводим доменное имя
			log.print("Domain: %s => %s", log_t::flag_t::INFO, domain.c_str(), "74.125.131.139");
	}
	// Выполняем поиск доменных имён принадлежащим IP-адресу anyks.com
	const auto & anyks = dns.search("193.42.110.185");
	// Если список доменных имён получен
	if(!anyks.empty()){
		// Выполняем перебор всего списка доменных имён
		for(auto & domain : anyks)
			// Выводим доменное имя
			log.print("Domain: %s => %s", log_t::flag_t::INFO, domain.c_str(), "193.42.110.185");
	}
	// Выполняем кодирование кирилического доменного имени
	log.print("Encode domain \"ремпрофи.рф\" == \"%s\"", log_t::flag_t::INFO, dns.encode("ремпрофи.рф").c_str());
	// Выполняем декодирование кирилического доменного имени
	log.print("Decode domain \"xn--e1agliedd7a.xn--p1ai\" == \"%s\"", log_t::flag_t::INFO, dns.decode("xn--e1agliedd7a.xn--p1ai").c_str());
	// Выводим результат
	return 0;
}
