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
 * @brief Тест создания сокета
 *
 */
TEST_F(EthFixture, SocketCreateTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);
	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест создания пары сокетов
 *
 */
TEST_F(EthFixture, SocketPairTest){
	// Создаём пару сокетов Unix Domain Stream
	auto ipc = this->_eth->socket.ipc(awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE);
	// Проверяем что первый сокет создан успешно
	ASSERT_NE(ipc[0], awh::net::invalid_socket_t);
	// Проверяем что второй сокет создан успешно
	ASSERT_NE(ipc[1], awh::net::invalid_socket_t);
	// Закрываем первый сокет
	::close(ipc[0]);
	// Закрываем второй сокет
	::close(ipc[1]);
}

/**
 * @brief Тест ошибки и таймаутов сокета
 *
 */
TEST_F(EthFixture, SocketOptionsTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Проверяем что нет ошибки на сокете
	ASSERT_EQ(0, this->_eth->socket.getError(sock));

	// Проверяем что таймаут для чтения установлен успешно
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::READ, 100));
	// Проверяем что таймаут для записи установлен успешно
	ASSERT_TRUE(this->_eth->socket.setTimeout(sock, awh::net::socket_event_t::WRITE, 100));

	// Получаем таймаут на чтение сокета
	ASSERT_EQ(100, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::READ));
	// Получаем таймаут на запись сокета
	ASSERT_EQ(100, this->_eth->socket.getTimeout(sock, awh::net::socket_event_t::WRITE));

	// Получаем размер буфера для чтения
	const int32_t rcv = this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::READ);
	// Проверяем что размер буфера для чтения больше 0
	ASSERT_GT(rcv, 0);
	// Устанавливаем размер буфера для чтения в 2 раза больше
	this->_eth->socket.setBufferSize(sock, awh::net::socket_event_t::READ, rcv * 2);
	
	// Получаем размер буфера для записи
	const int32_t snd = this->_eth->socket.getBufferSize(sock, awh::net::socket_event_t::WRITE);
	// Проверяем что размер буфера для записи больше 0
	ASSERT_GT(snd, 0);
	// Устанавливаем размер буфера для записи в 2 раза больше
	this->_eth->socket.setBufferSize(sock, awh::net::socket_event_t::WRITE, snd * 2);

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест опций сокета
 *
 */
TEST_F(EthFixture, SocketSetOptionTest){
	// Создаём TCP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Устанавливаем опции сокета
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_ADDR));
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::REUSE_PORT));
	ASSERT_TRUE(this->_eth->socket.switchOption(sock, awh::event::family_t::IPV4, awh::net::socket_mode_t::ENABLED, awh::event::options::NO_IO_BLOCK));
	
	// Устанавливаем keepalive (может вернуть false на некоторых системах или если не поддерживается полностью)
	ASSERT_TRUE(this->_eth->socket.setKeepalive(sock, 5, 5, 5)); // Может вернуть false на некоторых системах или если не поддерживается полностью

	// Закрываем сокет
	::close(sock);
}

/**
 * @brief Тест мультикаста и Hops
 *
 */
TEST_F(EthFixture, SocketMulticastTest){
	// Создаём UDP сокет IPv4
	auto sock = this->_eth->socket.issue(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что сокет создан успешно
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Устанавливаем максимальное количество хопов для unicast пакетов в 1 (локальная сеть)
	ASSERT_TRUE(this->_eth->socket.setHops(sock, awh::event::family_t::IPV4, awh::event::delivery_mode_t::UNICAST, static_cast <uint8_t> (awh::event::hops_t::NETWORK)));
	// Получаем максимальное количество хопов для unicast пакетов
	ASSERT_EQ(static_cast <uint8_t> (awh::event::hops_t::NETWORK), this->_eth->socket.getHops(sock, awh::event::family_t::IPV4, awh::event::delivery_mode_t::UNICAST));

	// Устанавливаем дифференцированные услуги (DSCP) для сокета
	ASSERT_TRUE(this->_eth->socket.setDifferentiatedServicesCodePoint(sock, awh::event::family_t::IPV4, awh::event::dscp_t::CS0));
	// Получаем дифференцированные услуги (DSCP) для сокета
	ASSERT_EQ(awh::event::dscp_t::CS0, this->_eth->socket.getDifferentiatedServicesCodePoint(sock, awh::event::family_t::IPV4));

	// Устанавливаем обнаружение максимального размера пакета (MTU) для сокета
	ASSERT_TRUE(this->_eth->socket.setMaximumTransmissionUnitDiscover(sock, awh::event::family_t::IPV4, awh::event::mtu_discover_t::DO));
	// Получаем обнаружение максимального размера пакета (MTU) для сокета
	ASSERT_EQ(awh::event::mtu_discover_t::DO, this->_eth->socket.getMaximumTransmissionUnitDiscover(sock, awh::event::family_t::IPV4));
	
	// Создаём адрес источника для мультикаста
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Заполняем адрес источника по имени сетевого интерфейса (может не сработать если нет подходящего интерфейса)
	this->_eth->addr.fillSource(source);
	// Если интерфейс найден, устанавливаем его для мультикаста
	if(!source.iface.empty())
		// Устанавливаем интерфейс для мультикаста (может не сработать если интерфейс не поддерживает мультикаст)
		this->_eth->socket.setMulticastIface(sock, awh::event::family_t::IPV4, source.iface);

	// Создаём адрес мультикаст группы
	std::unique_ptr <awh::net::addr_net_t> mcast_addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес мультикаст группы в 239.0.0.1
	static_cast <awh::net::addr_net_ipv4_t *> (mcast_addr.get())->address = htonl(0xEF000001);
	
	/**
	 * Join/Leave (нужен реальный интерфейс с поддержкой мультикаста, но API тест пройдёт)
	 * Для теста передаём nullptr source, чтобы система выбрала дефолтный (если сработает)
	 * Или используем source, полученный ранее.
	 */ 
	
	// Создаём адрес any для отключения от мультикаста
	std::unique_ptr <awh::net::addr_net_t> any_addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес INADDR_ANY в объект адреса IPv4
	static_cast <awh::net::addr_net_ipv4_t *> (any_addr.get())->address = htonl(INADDR_ANY);
	// Подписываемся на мультикаст группу (может не сработать если нет подходящего интерфейса или если интерфейс не поддерживает мультикаст)
	this->_eth->socket.membership(sock, awh::net::socket_mode_t::ENABLED, mcast_addr.get(), any_addr.get());
	// Отписываемся от мультикаст группы (может не сработать если нет подходящего интерфейса или если интерфейс не поддерживает мультикаст)
	this->_eth->socket.membership(sock, awh::net::socket_mode_t::DISABLED, mcast_addr.get(), any_addr.get());

	// Закрываем сокет
	::close(sock);
}
