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
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"
#include <arpa/inet.h>

/**
 * @brief Тест получения маршрута
 *
 */
TEST_F(EthFixture, GatewayGetTest){
	awh::eth::gateway_t::route_t route;
	// Попытка получить маршрут по умолчанию (пустой destination может интерпретироваться как default)
	// Или устанавливаем конкретный destination
	route.destination = std::make_unique<awh::net::addr_net_ipv4_t>();
	static_cast<awh::net::addr_net_ipv4_t*>(route.destination.get())->address = 0x08080808; // 8.8.8.8

	// Вызываем метод. Может вернуть false если маршрута нет или нет прав/сети
	// Главное - метод не должен падать
	bool res = this->_eth->gateway.get(route);
	if (res) {
		ASSERT_FALSE(route.ifname.empty());
	}
}

/**
 * @brief Тест добавления и удаления маршрута
 *
 */
TEST_F(EthFixture, GatewayAddRemoveTest){
	// Этот тест скорее всего требует привилегий root.
	// Мы пишем код, который пытается добавить фиктивный маршрут.
	
	awh::eth::gateway_t::route_t route;
	route.ifname = "lo0"; // или lo
	route.prefix = 32;
	
	// Destination 127.0.0.2
	route.destination = std::make_unique<awh::net::addr_net_ipv4_t>();
	static_cast<awh::net::addr_net_ipv4_t*>(route.destination.get())->address = htonl(0x7F000002);
	
	// Gateway 127.0.0.1
	route.gateway = std::make_unique<awh::net::addr_net_ipv4_t>();
	static_cast<awh::net::addr_net_ipv4_t*>(route.gateway.get())->address = htonl(0x7F000001);

	// Пробуем добавить
	if (this->_eth->gateway.add(route)) {
		// Если добавилось - пробуем удалить
		ASSERT_TRUE(this->_eth->gateway.remove(route));
	} else {
		// Если не добавилось (нет прав), то удаления тоже не должно быть или оно вернёт false
		this->_eth->gateway.remove(route);
	}
}
