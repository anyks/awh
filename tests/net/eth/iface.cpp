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
 * Подключаем системные заголовочные файлы
 */
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Тест получения доступных интерфейсов
 *
 */
TEST_F(EthFixture, IfaceAvailableTest){
	// Получаем список доступных сетевых интерфейсов
	auto interfaces = this->_eth->iface.available();
	// В системе обычно есть хотя бы loopback
	ASSERT_FALSE(interfaces.empty());
}

/**
 * @brief Тест проверки доступности конкретного интерфейса
 *
 */
TEST_F(EthFixture, IfaceIsAvailableTest){
	// Получаем список доступных сетевых интерфейсов
	auto interfaces = this->_eth->iface.available();
	// В системе обычно есть хотя бы loopback
	ASSERT_FALSE(interfaces.empty());
	// Получаем название первого сетевого интерфейса
	std::string ifname = * interfaces.begin();
	// Проверяем что сетевой интерфейс доступен в системе
	ASSERT_TRUE(this->_eth->iface.isAvailable(ifname));
	// Проверяем что такого фейкового сетевого интерфейса в системе нет
	ASSERT_FALSE(this->_eth->iface.isAvailable("non_existent_iface_123"));
}

/**
 * @brief Тест проверки на туннель и виртуальный интерфейс
 *
 */
TEST_F(EthFixture, IfaceTypeTest){
	// Получаем список доступных сетевых интерфейсов
	auto interfaces = this->_eth->iface.available();
	// Если список сетевых интерфейсов получен
	if(!interfaces.empty()){
		// Получаем первый сетевой интерфейс
		std::string ifname = * interfaces.begin();
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
	// Создаём объект IPv4 адреса
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес петлевого сетевого интерфейса
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = htonl(INADDR_LOOPBACK);
	// Просто вызываем методы, результат зависит от системы
	this->_eth->iface.isTunnel(addr.get());
	this->_eth->iface.isVirtual(addr.get());
}

/**
 * @brief Тест получения имени интерфейса по адресу
 *
 */
TEST_F(EthFixture, IfaceNameTest){
	// Создаём объект IPv4 адреса
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес мультикаст-группы (как в static.cpp) или loopback
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = 0;
	// Получаем имя сетевого интерфейса по IP-адресу (ожидаем пустое для 0.0.0.0 или соответствующее)
	this->_eth->iface.name(addr.get());
}

/**
 * @brief Тест создания/удаления интерфейса
 *
 */
TEST_F(EthFixture, IfaceCreateDestroyTest){
	// Название созданного тоннеля
	std::string name = "";
	// Попытка создания TAP/TUN интерфейса (требует прав)
	auto sock = this->_eth->iface.create(awh::event::eth_t::TUN, name);
	// Если сокет создан успешно
	if(sock != awh::net::invalid_socket_t){
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
	// Получаем список доступных сетевых интерфейсов
	auto interfaces = this->_eth->iface.available();
	// Если список сетевых интерфейсов получен
	if(!interfaces.empty()){
		// Получаем первый сетевой интерфейс
		std::string ifname = * interfaces.begin();
		// Получаем значение MTU для сетевого интерфейса
		const uint16_t mtu = this->_eth->iface.mtu(ifname);
		// Проверяем, что мы извлекли все удачно
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
	// Получаем список доступных сетевых интерфейсов
	auto interfaces = this->_eth->iface.available();
	// Если список сетевых интерфейсов получен
	if(!interfaces.empty()){
		// Получаем первый сетевой интерфейс
		std::string ifname = * interfaces.begin();
		// Получаем список флагов сетевого интерфейса
		auto flags = this->_eth->iface.flags(ifname);
		// Проверяем наличие флагов (например UP)
		if(flags.find(awh::event::eth_flag_t::UP) != flags.end())
			// Устанавливаем флаг обратно
			this->_eth->iface.flag(ifname, awh::event::eth_flag_t::UP, awh::event::mode_t::ENABLED);
	}
}

/**
 * @brief Тест получения/установки адреса
 *
 */
TEST_F(EthFixture, IfaceAddressTest){
	// Получаем список доступных сетевых интерфейсов
	auto interfaces = this->_eth->iface.available();
	// Если список сетевых интерфейсов получен
	if(!interfaces.empty()){
		// Получаем первый сетевой интерфейс
		std::string ifname = * interfaces.begin();
		// Получаем адрес IPv4
		auto ip = this->_eth->iface.getAddress(ifname, awh::event::family_t::IPV4);
		// Если IP-адрес получен успешно
		if(ip != nullptr)
			// Если адрес есть, пробуем сеттер (нужны права, но проверяем АПИ)
			this->_eth->iface.setAddress(ifname, ip.get(), 24); 
		// Префикс адреса назначения сетевого интерфейса
		uint8_t prefix = 0;
		// IP-адрес маршрутизатора сетевого интерфейса
		std::unique_ptr <awh::net::addr_t> ip_ptr = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// IP-адрес назначения сетевого интерфейса
		std::unique_ptr <awh::net::addr_t> peer_ptr = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Извлекаем P2P параметры
		this->_eth->iface.getAddress(ifname, ip_ptr, peer_ptr, prefix);
	}
}
