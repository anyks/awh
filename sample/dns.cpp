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
	// Создаём объекты нейм-сервера
	dns_t::serv_t serv1, serv2;
	// Устанавливаем хост первого нейм-сервера
	serv1.host = "77.88.8.88";
	// Устанавливаем хост второго нейм-сервера
	serv2.host = "77.88.8.2";
	// Выполняем установку серверов имён
	dns.servers(AF_INET, {std::move(serv1), std::move(serv2)});
	// Выполняем запрос на получение первого IP-адресов
	log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("localhost", AF_INET).c_str());
	// Выполняем запрос на получение второго IP-адресов
	log.print("IP2: %s", log_t::flag_t::INFO, dns.resolve("yandex.ru", AF_INET).c_str());
	// Выполняем запрос на получение третьего IP-адресов
	log.print("IP3: %s", log_t::flag_t::INFO, dns.resolve("google.com", AF_INET).c_str());
	// Выводим результат
	return 0;
}
