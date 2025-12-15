/**
 * @file: static.cpp
 * @date: 2025-12-15
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
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"

/**
 * @brief Тест создания объекта работы со списком параметров URL
 *
 */
TEST_F(IoFixture, CreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Сбрасываем объект асинхронного движка ввода-вывода
	this->_io.reset();
	// Проверяем, что объект асинхронного движка ввода-вывода сброшен
	ASSERT_TRUE(this->_io == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта асинхронного движка ввода-вывода
 *
 */
TEST_F(IoFixture, ResetAndCreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Сбрасываем объект асинхронного движка ввода-вывода
	this->_io.reset();
	// Проверяем, что объект асинхронного движка ввода-вывода сброшен
	ASSERT_TRUE(this->_io == nullptr);
	// Создаём объект асинхронного движка ввода-вывода заново
	this->_io = std::make_unique <awh::io_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
}

/**
 * @brief Тест повторного создания объекта асинхронного движка ввода-вывода
 *
 */
TEST_F(IoFixture, ReCreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Создаём объект асинхронного движка ввода-вывода заново
	this->_io = std::make_unique <awh::io_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
}

/**
 * @brief Тест набора сетевых тестов
 *
 */
TEST_F(IoFixture, IoSuiteTest){
	// Устанавливаем логгер
	this->_fmk->setLogger(this->_log.get());
	// Создаём объект работы с Ethernet
	awh::eth_t eth(this->_fmk.get(), this->_log.get());
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	eth.fillsource(source);
	// Проверяем, что название сетевого интерфейса получено
	ASSERT_FALSE(source.iface.empty());
	// Если сетевой интерфейс не принадлежит к VPN
	if(::memcmp("ut", source.iface.c_str(), 2) != 0){
		// MAC-адрес и IP-адрес сетевого интерфейса
		std::string mac = "", ip = "";
		/**
		 * IPv4 событие
		 */
		{
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid1 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid1, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->iface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid1));
			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->address(eid1, awh::event::address_t::IPV4);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->address(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid2, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::IPV4, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid3, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::NETWORK, ip + "/255.255.255.0"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid4).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid4));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid4, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid4, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid5 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid5, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->port(eid5, 8080));
			// Устанавливаем UDS-адрес события
			ASSERT_TRUE(this->_io->address(eid5, awh::event::address_t::UDS, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->iface(eid5).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->address(eid5, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->address(eid5, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid4).empty());
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->address(eid5, awh::event::address_t::UDS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid6 = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid6, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->port(eid6, 8080));
			// Устанавливаем сетевой адрес события
			ASSERT_TRUE(this->_io->address(eid6, awh::event::address_t::FS, "/tmp/awh.txt"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->iface(eid6).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->address(eid6, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->address(eid6, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid6).empty());
			// Проверяем, что адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.txt", this->_io->address(eid6, awh::event::address_t::FS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->target(eid7, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid7).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid7));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid7, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid7, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->target(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid7, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid8 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid8, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->port(eid8, 8080));
			// Устанавливаем UDS-адрес назначения для события
			ASSERT_TRUE(this->_io->target(eid8, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->iface(eid8).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->address(eid8, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->address(eid8, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_EQ("/tmp/awh.sock", this->_io->target(eid8));
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->address(eid8, awh::event::address_t::UDS));
		}
		/**
		 * IPv6 событие
		 */
		{
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid1 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid1, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->iface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid1));
			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->address(eid1, awh::event::address_t::IPV6);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->address(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid2, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::IPV6, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->iface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->address(eid3, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->address(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::NETWORK, ip + "/64"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->iface(eid4).empty());
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->address(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->target(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->port(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->port(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->target(eid7, ip));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->address(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->target(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->address(eid7, awh::event::address_t::UDS).empty());
		}
	}
}
