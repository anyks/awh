/**
 * @file: gateway.cpp
 * @date: 2026-02-06
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
 * Подключаем системные заголовочные файлы
 */
#include <arpa/inet.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Тест получения маршрута
 *
 */
TEST_F(EthFixture, GatewayGetTest){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Если получаем маршрут для указанного адреса
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Выводим информацию о найденном маршруте
	std::cout << "Gateway found:" << std::endl;
	// Выводим информацию о маршруте
	std::cout << " Interface: " << route.ifname << std::endl;
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Получаем IP-адрес текущего шлюза по умолчанию
	const std::string gateway = static_cast <std::string> (* this->_addr.get());
	// Выводим адрес шлюза по умолчанию
	std::cout << "Default Gateway: " << gateway << std::endl;
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Выводим адрес назначения
	std::cout << "Destination: " << static_cast <std::string> (* this->_addr.get()) << "/" << static_cast <uint32_t> (route.prefix) << std::endl;
	// Удаляем маршрут по указанному адресу
	ASSERT_TRUE(this->_eth->gateway.remove(route));
	/**
	 * sudo route add default 192.168.7.1
	 * sudo route delete default 0.0.0.0
	 */
	// Выполняем парсинг адреса нового шлюза
	(* this->_addr.get()) = "192.168.7.131";
	// Устанавливаем адрес шлюза в маршрут
	awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Устанавливаем имя сетевого интерфейса
	// route.ifname = "en0";
	// Сбрасываем адрес назначения
	// awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = 0;
	// Добавляем маршрут с новым шлюзом
	ASSERT_TRUE(this->_eth->gateway.add(route));
	// Сбрасываем имя сетевого интерфейса
	// route.ifname = "";
	// Если получаем маршрут для указанного адреса
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Выводим информацию о маршруте
	std::cout << " Interface: " << route.ifname << std::endl;
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Выводим адрес шлюза по умолчанию
	std::cout << "Default Gateway: " << static_cast <std::string> (* this->_addr.get()) << std::endl;
	// Удаляем маршрут по указанному адресу
	ASSERT_TRUE(this->_eth->gateway.remove(route));
	// Выполняем парсинг адреса нового шлюза
	(* this->_addr.get()) = gateway;
	// Устанавливаем адрес шлюза в маршрут
	awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Добавляем маршрут с новым шлюзом
	ASSERT_TRUE(this->_eth->gateway.add(route));
}
