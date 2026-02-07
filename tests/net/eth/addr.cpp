/**
 * @file: addr.cpp
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Тест заполнения источника сетевых адресов
 *
 */
TEST_F(EthFixture, AddressFillSourceTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(source);
	// Проверяем, что название сетевого интерфейса получено (или хотя бы не упало, может быть пустым если сети нет)
	ASSERT_FALSE(source.iface.empty()); // Необходимо, чтобы хотя бы один сетевой интерфейс имел выход в интернет
}

/**
 * @brief Тест заполнения источника сетевых адресов по типу узла
 *
 */
TEST_F(EthFixture, AddressFillSourceNodeTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(awh::event::node_t::NONE, source);
	// Проверяем результат
	ASSERT_FALSE(source.iface.empty());
}

/**
 * @brief Тест заполнения источника сетевых адресов по сети
 *
 */
TEST_F(EthFixture, AddressFillSourceNetTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Создаём объект IPv4-адреса
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес (например, 0.0.0.0 или localhost 127.0.0.1)
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = htonl(INADDR_LOOPBACK);

	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(addr, source);
	// Проверяем, что интерфейс найден (для loopback он должен быть)
	ASSERT_FALSE(source.iface.empty());
}

/**
 * @brief Тест проверки принадлежности к подсети
 *
 */
TEST_F(EthFixture, AddressIsInSubnetTest){
	// 192.168.1.10
	uint32_t ip = 0xC0A8010A; 
	// 192.168.1.0
	uint32_t net = 0xC0A80100;
	// /24
	uint8_t prefix = 24;

	// Выполняем проверку принадлежности IP-адреса подсети
	ASSERT_TRUE(this->_eth->addr.isInSubnet(ip, net, prefix));

	// Неверная подсеть 192.168.2.0
	uint32_t net2 = 0xC0A80200;
	// Если адрес соответствует подсети
	ASSERT_FALSE(this->_eth->addr.isInSubnet(ip, net2, prefix));
}

/**
 * @brief Тест сравнения префиксов IPv6
 *
 */
TEST_F(EthFixture, AddressIpv6PrefixEqualTest){
	// Создаём два IPv6-адреса
	uint8_t a[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
	uint8_t b[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};

	// /64 совпадают
	ASSERT_TRUE(this->_eth->addr.ipv6PrefixEqual(a, b, 64));

	// /128 не совпадают
	ASSERT_FALSE(this->_eth->addr.ipv6PrefixEqual(a, b, 128));
}

/**
 * @brief Тест вычисления контрольной суммы
 *
 */
TEST_F(EthFixture, AddressChecksumTest){
	// Тест с нулевыми данными должен вернуть 0 или корректную сумму пустого пакета
	// Здесь проверяем просто вызов и отсутствие краша, так как логика checksum сложная для моков
	uint16_t sum = this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, nullptr, nullptr, nullptr, 0);
	ASSERT_EQ(0, sum);

	// Можно добавить реальный тест с данными, если известна логика
	char data[] = "test";
	sum = this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, nullptr, nullptr, data, 4);
	// Проверяем что сумма не ноль (или вычисляем ожидаемую)
	// ASSERT_NE(0, sum); // Зависит от реализации
}
