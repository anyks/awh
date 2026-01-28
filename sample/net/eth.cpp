/**
 * @file: ip.cpp
 * @date: 2025-10-31
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
 * Подключаем заголовочные файлы стандартной библиотеки C++
 */
#include <iostream>
#include <arpa/inet.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth.hpp>
#include <net/addr.hpp>

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
	// Создаём объект Ethernet
	eth_t eth(&fmk, &log);
	// Объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Получаем список сетевых интерфейсов системы
	for(auto & iface : eth.ifaces())
		// Выводим список сетевых интерфейсов системы
		cout << "Interface: " << iface << endl;
	// Выводим заголовок примера проброса порта
	cout << " --- Gateway Example --- " << endl;
	// Временный объект для извлечения сетевого интерфейса
	net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
	// Получаем шлюз по умолчанию
	eth.gateway(source);
	// Устанавливаем полученный IP-адрес
	addr.v4(awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address, net_addr_t::endian_t::LITTLE);
	// Выводим адрес шлюза по умолчанию
	cout << "Default Gateway: " << static_cast <string> (addr) << endl;
	// Устанавливаем полученный MAC-адрес в объект события
	addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
	// Выводим адрес MAC-адрес шлюза по умолчанию
	cout << "Default MAC Gateway: " << static_cast <string> (addr) << endl;

	awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = inet_addr("192.168.7.1"); // 192.168.7.131

	eth.gateway(source.ip);

	// Получаем шлюз по умолчанию
	eth.gateway(source);
	// Устанавливаем полученный IP-адрес
	addr.v4(awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address, net_addr_t::endian_t::LITTLE);
	// Выводим адрес шлюза по умолчанию
	cout << "Default Gateway: " << static_cast <string> (addr) << endl;
	// Устанавливаем полученный MAC-адрес в объект события
	addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
	// Выводим адрес MAC-адрес шлюза по умолчанию
	cout << "Default MAC Gateway: " << static_cast <string> (addr) << endl;

	// Выводим заголовок примера проброса порта
	cout << " --- Portmapping Example --- " << endl;
	// Создаём структуру проброса порта на маршрутизаторе
	net::portmap_t portmap;
	// Заполняем параметры проброса порта
	portmap.ttl = 3600;
	portmap.internalPort = 8081;
	portmap.externalPort = 8080;
	portmap.type = net::portmap_type_t::UPNP;
	portmap.protocol = event::protocol_t::TCP;
	portmap.internalAddr = "192.168.1.100";
	portmap.externalAddr = "123.456.78.90";
	portmap.description = "AWH Web Server";
	// Выполняем проброс порта на маршрутизаторе
	if(eth.mapping(portmap, event::mode_t::ENABLED)){
		// Получаем список проброшенных портов на маршрутизаторе
		const vector <net::portmap_t> & map = eth.mappings();
		/**
		 * Выводим информацию о проброшенных портах на маршрутизаторе
		 */
		for(auto & item : map){
			// Выводим информацию о проброшенных портах
			cout << "Portmapping: " << item.internalAddr << ":" << item.internalPort << " -> " << item.externalAddr << ":" << item.externalPort
				<< " Protocol: " << static_cast <uint16_t> (item.protocol) << " Type: " << static_cast <uint16_t> (item.type)
				<< " Description: " << item.description << " TTL: " << item.ttl << " seconds" << endl;
		}
		// Удаляем проброс порта на маршрутизаторе
		portmap = net::portmap_t();
		portmap.internalPort = 8081;
		portmap.externalPort = 8080;
		portmap.type = net::portmap_type_t::UPNP;
		portmap.protocol = event::protocol_t::TCP;
		// Выполняем удаление проброса порта на маршрутизаторе
		if(eth.mapping(portmap, event::mode_t::DISABLED))
			// Выводим сообщение об успешном удалении проброса порта
			cout << "Portmapping removed successfully." << endl;
	}
	// Выводим результат
	return EXIT_SUCCESS;
}
