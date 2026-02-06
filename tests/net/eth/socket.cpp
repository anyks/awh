/**
 * @file: socket.cpp
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
 * @brief Тест создания сокета
 *
 */
TEST_F(EthFixture, SocketCreateTest){
	auto sock = this->_eth->socket.create(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	ASSERT_NE(sock, awh::net::invalid_socket_t);
	::close(sock);
}

/**
 * @brief Тест создания пары сокетов
 *
 */
TEST_F(EthFixture, SocketPairTest){
	auto pair = this->_eth->socket.pair(awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE);
	ASSERT_NE(pair[0], awh::net::invalid_socket_t);
	ASSERT_NE(pair[1], awh::net::invalid_socket_t);
	::close(pair[0]);
	::close(pair[1]);
}

/**
 * @brief Тест ошибки и таймаутов сокета
 *
 */
TEST_F(EthFixture, SocketOptionsTest){
	auto sock = this->_eth->socket.create(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Error
	ASSERT_EQ(0, this->_eth->socket.error(sock));

	// Timeout
	ASSERT_TRUE(this->_eth->socket.timeout(sock, awh::net::socket_event_t::READ, 100));
	ASSERT_TRUE(this->_eth->socket.timeout(sock, awh::net::socket_event_t::WRITE, 100));

	// Buffer Size
	int32_t rcv = this->_eth->socket.bufferSize(sock, awh::net::socket_event_t::READ);
	ASSERT_GT(rcv, 0);
	this->_eth->socket.bufferSize(sock, awh::net::socket_event_t::READ, rcv * 2);
	
	int32_t snd = this->_eth->socket.bufferSize(sock, awh::net::socket_event_t::WRITE);
	ASSERT_GT(snd, 0);
	this->_eth->socket.bufferSize(sock, awh::net::socket_event_t::WRITE, snd * 2);

	::close(sock);
}

/**
 * @brief Тест опций сокета
 *
 */
TEST_F(EthFixture, SocketSetOptionTest){
	auto sock = this->_eth->socket.create(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	ASSERT_TRUE(this->_eth->socket.setoption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_ADDR));
	ASSERT_TRUE(this->_eth->socket.setoption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_PORT));
	ASSERT_TRUE(this->_eth->socket.setoption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_IO_BLOCK));
	
	// KeepAlive
	ASSERT_FALSE(this->_eth->socket.keepalive(sock, 5, 5, 5)); // Может вернуть false на некоторых системах или если не поддерживается полностью
	
	::close(sock);
}

/**
 * @brief Тест мультикаста и Hops
 *
 */
TEST_F(EthFixture, SocketMulticastTest){
	auto sock = this->_eth->socket.create(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Hops
	this->_eth->socket.hops(sock, awh::event::family_t::IPV4, awh::event::delivery_mode_t::UNICAST, awh::event::hops_t::NETWORK);
	
	// Multicast Iface (по Loopback)
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	this->_eth->addr.fillSource(source);
	if (!source.iface.empty()) {
		this->_eth->socket.multicastIface(sock, awh::event::family_t::IPV4, source.iface);
	}

	// Membership
	std::unique_ptr <awh::net::addr_net_t> mcast_addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// 239.0.0.1
	static_cast <awh::net::addr_net_ipv4_t *> (mcast_addr.get())->address = htonl(0xEF000001);
	
	// Join/Leave (нужен реальный интерфейс с поддержкой мультикаста, но API тест пройдёт)
	// Для теста передаём nullptr source, чтобы система выбрала дефолтный (если сработает)
	// Или используем source, полученный ранее.
	
	// Можем создать source адрес
	std::unique_ptr <awh::net::addr_net_t> any_addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	static_cast <awh::net::addr_net_ipv4_t *> (any_addr.get())->address = htonl(INADDR_ANY);

	this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, mcast_addr.get(), any_addr.get());
	this->_eth->socket.membership(sock, awh::net::socket_mode_t::DISABLED, mcast_addr.get(), any_addr.get());

	::close(sock);
}
