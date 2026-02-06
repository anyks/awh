/**
 * @file: iface.cpp
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
 * @brief Тест получения доступных интерфейсов
 *
 */
TEST_F(EthFixture, IfaceAvailableTest){
	auto interfaces = this->_eth->iface.available();
	ASSERT_FALSE(interfaces.empty()); // В системе обычно есть хотя бы loopback
}

/**
 * @brief Тест проверки доступности конкретного интерфейса
 *
 */
TEST_F(EthFixture, IfaceIsAvailableTest){
	auto interfaces = this->_eth->iface.available();
	ASSERT_FALSE(interfaces.empty());
	std::string ifname = *interfaces.begin();
	ASSERT_TRUE(this->_eth->iface.isAvailable(ifname));
	ASSERT_FALSE(this->_eth->iface.isAvailable("non_existent_iface_123"));
}

/**
 * @brief Тест проверки на туннель и виртуальный интерфейс
 *
 */
TEST_F(EthFixture, IfaceTypeTest){
	auto interfaces = this->_eth->iface.available();
	if (!interfaces.empty()) {
		std::string ifname = *interfaces.begin();
		// Просто вызываем методы, результат зависит от системы
		this->_eth->iface.isTunnel(ifname);
		this->_eth->iface.isVirtual(ifname);
	}
}

/**
 * @brief Тест проверки интерфейса по адресу
 *
 */
TEST_F(EthFixture, IfaceTypeByAddrTest){
	// Loopback address
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = htonl(INADDR_LOOPBACK);
	
	this->_eth->iface.isTunnel(addr);
	this->_eth->iface.isVirtual(addr);
}

/**
 * @brief Тест получения имени интерфейса по адресу
 *
 */
TEST_F(EthFixture, IfaceNameTest){
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес мультикаст-группы (как в static.cpp) или loopback
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = 0;
	// Получаем имя сетевого интерфейса по IP-адресу (ожидаем пустое для 0.0.0.0 или соответствующее)
	this->_eth->iface.name(addr);
}

/**
 * @brief Тест создания/удаления интерфейса
 *
 */
TEST_F(EthFixture, IfaceCreateDestroyTest){
	std::string name = "tun0";
	// Попытка создания TAP/TUN интерфейса (требует прав)
	auto sock = this->_eth->iface.create(awh::event::eth_t::TUN, name);
	if (sock != -1) {
		// Если создался, закрываем и уничтожаем
		::close(sock);
		// Уничтожение может требовать persistent режима, здесь просто проверяем вызов
		this->_eth->iface.destroy(name);
	}
}

/**
 * @brief Тест MTU
 *
 */
TEST_F(EthFixture, IfaceMtuTest){
	auto interfaces = this->_eth->iface.available();
	if (!interfaces.empty()) {
		std::string ifname = *interfaces.begin();
		uint16_t mtu = this->_eth->iface.mtu(ifname);
		ASSERT_GT(mtu, 0);
		
		// Попытка установить тот же MTU (безопасно)
		this->_eth->iface.mtu(ifname, mtu);
	}
}

/**
 * @brief Тест флагов интерфейса
 *
 */
TEST_F(EthFixture, IfaceFlagsTest){
	auto interfaces = this->_eth->iface.available();
	if (!interfaces.empty()) {
		std::string ifname = *interfaces.begin();
		auto flags = this->_eth->iface.flags(ifname);
		// Проверяем наличие флагов (например UP)
		bool up = flags.count(awh::event::eth_flag_t::UP);
		
		// Попытка изменить флаг (UP -> UP)
		if (up) {
			this->_eth->iface.flag(ifname, awh::event::eth_flag_t::UP, awh::event::mode_t::ENABLED);
		}
	}
}

/**
 * @brief Тест получения/установки адреса
 *
 */
TEST_F(EthFixture, IfaceAddressTest){
	auto interfaces = this->_eth->iface.available();
	if (!interfaces.empty()) {
		std::string ifname = *interfaces.begin();
		// Получение адреса IPv4
		auto ip = this->_eth->iface.getAddress(ifname, awh::event::family_t::IPV4);
		if (ip) {
			// Если адрес есть, пробуем сеттер (нужны права, но проверяем АПИ)
			// this->_eth->iface.setAddress(ifname, ip, 24); 
		}

		std::unique_ptr <awh::net::addr_t> ip_ptr;
		std::unique_ptr <awh::net::addr_t> peer_ptr;
		uint8_t prefix = 0;
		// Получение P2P параметров
		this->_eth->iface.getAddress(ifname, ip_ptr, peer_ptr, prefix);
	}
}
