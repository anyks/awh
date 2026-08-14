/**
 * @file ip.cpp
 * @date 2025-10-31
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
 * @brief Пример работы с сетевым уровнем Ethernet — демонстрация перечисления сетевых интерфейсов машины,
 *        получения их адресов и параметров, определения шлюза по умолчанию и проброса портов на маршрутизаторе
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем заголовочные файлы стандартной библиотеки C++
 */
#include <iostream>

/**
 * Подключаем заголовочный файл проекта
 */
// #include <net/eth.hpp>
#include <net/addr.hpp>
#include <net/eth/iface.hpp>
#include <net/eth/gateway.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

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
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Создаём объект Ethernet
	eth::iface_t iface(&fmk, &log);
	// Создаём объект для работы с шлюзами
	eth::gateway_t gateway(&fmk, &log);
	// Структура маршрута
	eth::gateway_t::route_t route{};
	// Инициализируем объект адреса назначения в маршруте
	// route.destination = make_unique <net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = make_unique <net::addr_net_ipv4_t> ();
	// Если получаем маршрут для указанного адреса
	if(gateway.get(route)){
		// Записываем в лог информацию о найденном маршруте
		cout << "Gateway found:" << endl;
		// Записываем в лог информацию о маршруте
		cout << " Interface: " << route.ifname << endl;
		// Устанавливаем полученный IP-адрес
		addr.v4(awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address, net_addr_t::endian_t::LITTLE);
		// Возвращаем адрес шлюза по умолчанию
		cout << "Default Gateway: " << static_cast <string> (addr) << endl;
		// Устанавливаем полученный IP-адрес
		addr.v4(awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address, net_addr_t::endian_t::LITTLE);
		// Возвращаем адрес назначения
		cout << "Destination: " << static_cast <string> (addr) << "/" << static_cast <uint32_t> (route.prefix) << endl;
		// Удаляем маршрут по указанному адресу
		if(gateway.remove(route)){
			/**
			 * sudo route add default 192.168.7.1
			 * sudo route delete default 0.0.0.0
			 */
			// Выполняем парсинг адреса нового шлюза
			addr = "192.168.7.131";
			// Устанавливаем адрес шлюза в маршрут
			awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = addr.v4(net_addr_t::endian_t::LITTLE);
			// Устанавливаем имя сетевого интерфейса
			// route.ifname = "en0";
			// Сбрасываем адрес назначения
			// awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = 0;
			// Добавляем маршрут с новым шлюзом
			if(gateway.add(route)){
				// Сбрасываем имя сетевого интерфейса
				// route.ifname = "";
				// Если получаем маршрут для указанного адреса
				if(gateway.get(route)){
					// Записываем в лог информацию о маршруте
					cout << " Interface: " << route.ifname << endl;
					// Устанавливаем полученный IP-адрес
					addr.v4(awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address, net_addr_t::endian_t::LITTLE);
					// Возвращаем адрес шлюза по умолчанию
					cout << "Default Gateway: " << static_cast <string> (addr) << endl;
					// Удаляем маршрут по указанному адресу
					if(gateway.remove(route)){
						// Выполняем парсинг адреса нового шлюза
						addr = "192.168.7.1"; 
						// Устанавливаем адрес шлюза в маршрут
						awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = addr.v4(net_addr_t::endian_t::LITTLE);
						// Добавляем маршрут с новым шлюзом
						if(gateway.add(route))
							// Записываем в лог сообщение об успешном восстановлении шлюза
							cout << "Gateway restored successfully." << endl;
					}
				}
			}
		}
	// Иначе выводим сообщение об ошибке
	} else cout << "Gateway not found." << endl;
	// Печатаем заголовок в отладочный вывод примера списка сетевых интерфейсов
	cout << " --- Ifaces Example --- " << endl;
	/**
	 * Получаем список сетевых интерфейсов системы
	 */
	for(auto & iface : iface.available())
		// Возвращаем список сетевых интерфейсов системы
		cout << "Interface: " << iface << endl;
	// Название туннельного интерфейса
	string tunnel = "";
	// Создаём туннельный интерфейс
	cout << " Tunnel interface: " << iface.create(event::eth_t::TUN, tunnel) << " === " << tunnel << ", isVirtual=" << iface.isVirtual(tunnel) << ", isTunnel=" << iface.isTunnel(tunnel) << endl;
	// Получаем IP-адрес туннельного интерфейса
	auto ip = iface.getAddress("utun4", event::family_t::IPV4);
	// Если IP-адрес получен
	if(ip != nullptr){
		// Устанавливаем полученный IP-адрес
		addr.v4(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address, net_addr_t::endian_t::LITTLE);
		// Возвращаем адрес шлюза по умолчанию
		cout << "IPv4 address: " << static_cast <string> (addr) << ", iface=" << iface.name(ip.get()) << ", isVirtual=" << iface.isVirtual("utun4") << ", isTunnel=" << iface.isTunnel("utun4") << endl;
		// Если устанавливаем IP-адрес туннельного интерфейса
		if(iface.setAddress(tunnel, ip.get(), ip.get(), 24))
			// Записываем в лог сообщение об успешной установке IP-адреса
			cout << " Assigned IPv4 address to " << tunnel << endl;
		// Иначе выводим сообщение об ошибке
		else cout << " Failed to assign IPv4 address to " << tunnel << endl;
	}
	// Устанавливаем MTU туннельного интерфейса
	if(iface.mtu(tunnel, 1800))
		// Записываем в лог сообщение об успешной установке MTU
		cout << " Set MTU to " << iface.mtu(tunnel) << " on " << tunnel << endl;
	// Устанавливаем туннельный интерфейс в состояние UP
	if(iface.flag(tunnel, event::eth_flag_t::UP, event::mode_t::ENABLED)){
		// Записываем в лог сообщение об успешной установке флага интерфейса
		cout << " Set interface " << tunnel << " UP" << endl;
		/**
		 * Перебираем все установленные флаги туннельного интерфейса
		 */
		for(auto & flag : iface.flags(tunnel))
			// Возвращаем флаг туннельного интерфейса
			cout << "  Flag: " << static_cast <uint16_t> (flag) << endl;
		// Снимаем туннельный интерфейс в состояние UP
		if(iface.flag(tunnel, event::eth_flag_t::UP, event::mode_t::DISABLED)){
			// Записываем в лог сообщение об успешной установке флага интерфейса
			cout << " Set interface " << tunnel << " DOWN" << endl;
			/**
			 * Перебираем все установленные флаги туннельного интерфейса
			 */
			for(auto & flag : iface.flags(tunnel))
				// Возвращаем флаг туннельного интерфейса
				cout << "  Flag: " << static_cast <uint16_t> (flag) << endl;
		}
	}
	// Получаем IP-адрес туннельного интерфейса
	ip = iface.getAddress("utun3", event::family_t::IPV6);
	// Если IP-адрес получен
	if(ip != nullptr){
		// Устанавливаем полученный IP-адрес
		addr.v6(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address, net_addr_t::endian_t::LITTLE);
		// Возвращаем адрес шлюза по умолчанию
		cout << "IPv6 address: " << static_cast <string> (addr) << ", iface=" << iface.name(ip.get()) << endl;
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
