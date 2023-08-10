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

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём биндинг
	dns_t dns(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("DNS");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Устанавливаем префикс переменной окружения
	dns.setPrefix("AWH");
	// Выполняем установку серверов имён
	dns.servers({"77.88.8.88", "77.88.8.2"});
	// Выполняем запрос на получение первого IP-адреса
	log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve(AF_INET, "localhost").c_str());
	// Выполняем запрос на получение второго IP-адреса
	log.print("IP2: %s", log_t::flag_t::INFO, dns.resolve(AF_INET, "yandex.ru").c_str());
	// Выполняем запрос на получение третьего IP-адреса
	log.print("IP3: %s", log_t::flag_t::INFO, dns.resolve(AF_INET, "google.com").c_str());
	/**
	 * Выполняем запрос на получение четвертого IP-адреса
	 * Пример переменной окружения: $ export AWH_DNS_IPV4_STALIN_INFO=255.255.255.222
	 */
	log.print("IP4: %s", log_t::flag_t::INFO, dns.resolve(AF_INET, "stalin.info").c_str());
	// Выполняем кодирование кирилического доменного имени
	log.print("Encode domain \"ремпрофи.рф\" == \"%s\"", log_t::flag_t::INFO, dns.encode("ремпрофи.рф").c_str());
	// Выполняем декодирование кирилического доменного имени
	log.print("Decode domain \"xn--e1agliedd7a.xn--p1ai\" == \"%s\"", log_t::flag_t::INFO, dns.decode("xn--e1agliedd7a.xn--p1ai").c_str());
	// Выводим результат
	return 0;
}
