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
 * @brief Генерация случайного порта в диапазоне 49152-65535
 *
 * @return случайный порт
 */
static uint16_t port() noexcept {
	/**
	 * Инициализация генератора случайных чисел
	 */
	::srandom(::time(nullptr) ^ ::getpid());
	/**
	 * Возвращаем случайный порт из диапазона 49152-65535
	 */
	return (49152 + (::random() % (65535 - 49152 + 1)));
}

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
			// Извлекаем информационные метаданные SCTP сообщения
			const awh::net::sctp_minfo_t minfo = this->_io->sctpMessageInfo(eid1);
			// Устанавливаем информационные метаданные SCTP сообщения
			this->_io->sctpMessageInfo(eid1, minfo);
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

/**
 * @brief Тест проверки работы TCP-соединения
 *
 */
TEST_F(IoFixture, IoTCPTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->options(cid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY | awh::event::options::KEEPALIVE));
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(events[1], 100, true));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-событий асинхронного ввода-вывода
 *
 */
TEST_F(IoFixture, IoUDPTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Текст исходящего сообщения
		const std::string message("Hello from async client!");
		// Отправляем данные обратно клиенту
		ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.size()));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-соединения
 *
 */
TEST_F(IoFixture, IoUDPConnectTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDS-соединения
 *
 */
TEST_F(IoFixture, IoUDSTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие клиента TCP
	awh::event::id_t cid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Добавляем новое событие клиента TCP
	awh::event::id_t sid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::UDS, awh::event::type_t::STREAM);
	// Проверяем корректность создания событий
	ASSERT_GT(cid, 0);
	ASSERT_GT(sid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->options(cid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	ASSERT_TRUE(this->_io->options(sid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->target(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->address(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(sid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(sid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(sid, static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->options(cid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY | awh::event::options::KEEPALIVE));
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(sid, awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(sid));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(sid, 100, true));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid, true));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP UDS-соединения
 *
 */
TEST_F(IoFixture, IoUDPUDSTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие клиента TCP
	awh::event::id_t cid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::DATAGRAM);
	// Добавляем новое событие клиента TCP
	awh::event::id_t sid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::UDS, awh::event::type_t::DATAGRAM);
	// Проверяем корректность создания событий
	ASSERT_GT(cid, 0);
	ASSERT_GT(sid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->options(cid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	ASSERT_TRUE(this->_io->options(sid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->target(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->address(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(sid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(sid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(sid, static_cast <awh::event::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->options(cid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY | awh::event::options::KEEPALIVE));
			// Выводим сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Выводим сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Выводим сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Выводим сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Выводим сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Выводим сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Выводим сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Выводим сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Выводим сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Выводим сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Выводим сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Выводим сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Выводим сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Выводим сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(sid, awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(sid));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::event::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid, true));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы широковещательного события
 *
 */
TEST_F(IoFixture, IoBroadcastTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY | awh::event::options::BROADCAST));
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "192.168.7.255"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Текст исходящего сообщения
		const std::string message("Hello from async client!");
		// Отправляем данные обратно клиенту
		ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.size()));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы многоадресной передачи UDP
 *
 */
TEST_F(IoFixture, IoMulticastTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[0], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT));
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->cast(events[0], awh::event::cast_t::MULTICAST));
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->membership(events[0], awh::event::mode_t::ENABLED, "239.255.1.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
	}
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->cast(events[1], awh::event::cast_t::MULTICAST));
		// Устанавливаем TTL для мультикастового события
		ASSERT_TRUE(this->_io->hops(events[1], awh::event::family_t::IPV4, awh::event::hops_t::NETWORK));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[1], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT | awh::event::options::MULTICASTLOOP));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "239.255.1.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Формируем отправляемое сообщение
		const std::string & message = "Hello from async multicast server!";
		// Выполняем отправку сообщения в мультикастовую группу
		ASSERT_TRUE(this->_io->send(events[1], message.c_str(), message.size()));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-соединения с перехватом данных из файла
 *
 */
TEST_F(IoFixture, IoUDPSpliceConnectTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие отслеживания файла
	awh::event::id_t fid = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->port(events[i], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->port(events[i]));
	}
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем адрес текстового файла для чтения
	ASSERT_TRUE(this->_io->address(fid, awh::event::address_t::FS, "../README.md"));
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->options(events[i], awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR | awh::event::options::REUSEPORT | awh::event::options::NOIOBLOCK | awh::event::options::CLOSEONEXEC | awh::event::options::TCPNODELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->address(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->address(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->target(events[0], "127.0.0.1"));
		// Выполняем объединение двух сокетов
		ASSERT_TRUE(this->_io->splice(fid, events[0]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::event::callback::connect_t> ([fid, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Выводим сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Выполняем фиксацию события файла
				ASSERT_TRUE(this->_io->commit(fid));
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->options(fid, awh::event::options::KEEPALIVE));
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->timeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->timeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->timeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0], true));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы файловой системой I/O событий
 *
 */
TEST_F(IoFixture, IoFsTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие отслеживания каталога
	awh::event::id_t did = this->_io->event(awh::event::node_t::DIR, awh::event::family_t::FSYS);
	// Добавляем новое событие отслеживания файла
	awh::event::id_t fid = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
	// Проверяем идентификаторы созданных событий
	ASSERT_GT(did, 0);
	ASSERT_GT(fid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Событие каталога
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на изменение события
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action, const awh::event::vnode_t vnode, const std::string & path) noexcept -> void {
			/**
			 * Обрабатываем тип узла события
			 */
			switch(static_cast <uint8_t> (vnode)){
				// Если тип узла не определён
				case static_cast <uint8_t> (awh::event::vnode_t::NONE):
					// Выводим сообщение о типе узла события
					this->_log->print("Тип узла события: Не определён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
				break;
				case static_cast <uint8_t> (awh::event::vnode_t::CHR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Символьный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Символьный узел устройства удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				case static_cast <uint8_t> (awh::event::vnode_t::BLK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Блочный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Блочный узел устройства удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каналом FIFO
				case static_cast <uint8_t> (awh::event::vnode_t::FIFO): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Канал FIFO добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Канал FIFO удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является сокетом
				case static_cast <uint8_t> (awh::event::vnode_t::SOCK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Сокет добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Сокет удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является файлом
				case static_cast <uint8_t> (awh::event::vnode_t::FILE): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Файл добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение об удалении события
							this->_log->print("Тип узла события: Файл удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каталогом
				case static_cast <uint8_t> (awh::event::vnode_t::DIR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Каталог добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение о типе узла события
							this->_log->print("Тип узла события: Каталог удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является символической ссылкой
				case static_cast <uint8_t> (awh::event::vnode_t::LINK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							this->_log->print("Тип узла события: Символическая ссылка добавлена, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Выводим сообщение о типе узла события
							this->_log->print("Тип узла события: Символическая ссылка удалена, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому каталогу
		ASSERT_TRUE(this->_io->address(did, awh::event::address_t::FS, "./"));
		// Выполняем фиксацию настроек события каталога
		ASSERT_TRUE(this->_io->commit(did));
	}
	/**
	 * Событие файла
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(fid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Выводим сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Выводим сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Выводим сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::RUNNING):
					// Выводим сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус остановки события
				case static_cast <uint8_t> (awh::event::status_t::STOPPED):
					// Выводим сообщение о остановке события
					this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Выводим сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Выводим сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Выводим сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Выводим сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING): {
					// Выводим сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					// Устанавливаем смещение в файле
					// this->_io->seek(eid, 1024);
					// Отправляем тестовое сообщение в файл
					this->_io->send(eid, "Hello World!!!", 14);
				} break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Выводим сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Выводим сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(fid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Выводим сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(fid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Выводим сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Удаляем файл
				::remove("tmp.txt");
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Удаляем файл
				::unlink("./tmp.txt");
			#endif
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(fid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Выводим сообщение об ошибке неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Выводим сообщение об ошибке недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Выводим сообщение об ошибке доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Выводим сообщение об ошибке уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Выводим сообщение об ошибке доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Выводим сообщение об ошибке некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Выводим сообщение об ошибке подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Выводим сообщение об ошибке недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Выводим сообщение об ошибке события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Выводим сообщение об ошибке события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Выводим сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Выводим сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Выводим сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Выводим сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Выводим сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Выводим сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Выводим сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Выводим сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Выводим сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Выводим сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Выводим сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Выводим сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому файлу
		ASSERT_TRUE(this->_io->address(fid, awh::event::address_t::FS, "./tmp.txt"));
		// Выполняем фиксацию настроек события файла
		ASSERT_TRUE(this->_io->commit(fid));
		// Устанавливаем опции события
		ASSERT_TRUE(this->_io->options(fid, awh::event::options::KEEPALIVE));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы пользовательских событий
 *
 */
TEST_F(IoFixture, IoEventsTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие отслеживания каталога
	awh::event::id_t eid = this->_io->event(awh::event::node_t::NOTIFY, awh::event::family_t::USER);
	// Проверяем инициализацию
	ASSERT_GT(eid, 0);
	// Проверяем, что идентификатор события больше нуля
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (status)){
			// Если статус принятия
			case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
				// Выводим сообщение о принятии события
				this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус уничтожения
			case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
				// Выводим сообщение об уничтожении события
				this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус инициализации
			case static_cast <uint8_t> (awh::event::status_t::INITIAL):
				// Выводим сообщение об инициализации события
				this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус запуска события
			case static_cast <uint8_t> (awh::event::status_t::RUNNING):
				// Выводим сообщение о запуске события
				this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус остановки события
			case static_cast <uint8_t> (awh::event::status_t::STOPPED):
				// Выводим сообщение о остановке события
				this->_log->print("Событие остановлено: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус паузы события
			case static_cast <uint8_t> (awh::event::status_t::PAUSED):
				// Выводим сообщение о паузе события
				this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус возобновления события
			case static_cast <uint8_t> (awh::event::status_t::RESUMED):
				// Выводим сообщение о возобновлении события
				this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус успешного выполнения события
			case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
				// Выводим сообщение о успешном выполнении события
				this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус неудачного выполнения события
			case static_cast <uint8_t> (awh::event::status_t::FAILURE):
				// Выводим сообщение о неудачном выполнении события
				this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			break;
			// Если статус выполнения события в ожидании
			case static_cast <uint8_t> (awh::event::status_t::PENDING):
				// Выводим сообщение о выполнении события в ожидании
				this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус подключения события
			case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
				// Выводим сообщение о подключении события
				this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус отмены события
			case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
				// Выводим сообщение об отмене события
				this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если статус переподключения события
			case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
				// Выводим сообщение о переподключении события
				this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
		}
	});
	// Устанавливаем функцию обратного вызова на запись в событие
	this->_io->on(eid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
		// Выводим сообщение о переподключении события
		this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
	}));
	// Устанавливаем функцию обратного вызова на чтение из события
	this->_io->on(eid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const std::string message(reinterpret_cast <const char *> (data), size);
		// Выводим сообщение о переподключении события
		this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
		// Останавливаем тест
		stop = true;
	});
	// Устанавливаем функцию обратного вызова на ошибку события
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (error)){
			// Если ошибка неизвестного события
			case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
				// Выводим сообщение об ошибке неизвестного события
				this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недопустимой операции
			case static_cast <uint8_t> (awh::event::error_t::INVALID):
				// Выводим сообщение об ошибке недопустимой операции
				this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа запрещёния
			case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
				// Выводим сообщение об ошибке доступа запрещёния
				this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка уже существующего объекта
			case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
				// Выводим сообщение об ошибке уже существующего объекта
				this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа к сокету
			case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
				// Выводим сообщение об ошибке доступа к сокету
				this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка некорректного адреса
			case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
				// Выводим сообщение об ошибке некорректного адреса
				this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка ошибки подключения
			case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
				// Выводим сообщение об ошибке подключения
				this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недостаточно ресурсов
			case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
				// Выводим сообщение об ошибке недостаточно ресурсов
				this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка события
			case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
				// Выводим сообщение об ошибке события
				this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если объект не найден
			case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
				// Выводим сообщение об ошибке события
				this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
		}
	});
	// Устанавливаем функцию обратного вызова на общее событие
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
		/**
		 * Обрабатываем действие события
		 */
		switch(static_cast <uint8_t> (action)){
			// Если действие является чтением
			case static_cast <uint8_t> (awh::event::action_t::READ):
				// Выводим сообщение о чтении события
				this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является записью
			case static_cast <uint8_t> (awh::event::action_t::WRITE):
				// Выводим сообщение о записи события
				this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является подключением
			case static_cast <uint8_t> (awh::event::action_t::CONNECT):
				// Выводим сообщение о подключении события
				this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отключением
			case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
				// Выводим сообщение об отключении события
				this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переподключением
			case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
				// Выводим сообщение о переподключении события
				this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является закрытием
			case static_cast <uint8_t> (awh::event::action_t::CLOSE):
				// Выводим сообщение о закрытии события
				this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением
			case static_cast <uint8_t> (awh::event::action_t::CHANGE):
				// Выводим сообщение об изменении события
				this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является удалением
			case static_cast <uint8_t> (awh::event::action_t::DELETE):
				// Выводим сообщение об удалении события
				this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переименованием
			case static_cast <uint8_t> (awh::event::action_t::RENAME):
				// Выводим сообщение о переименовании события
				this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением атрибутов
			case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
				// Выводим сообщение об изменении атрибутов события
				this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отзывом доступа
			case static_cast <uint8_t> (awh::event::action_t::REVOKE):
				// Выводим сообщение об отзыве доступа события
				this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением счётчика жёстких ссылок
			case static_cast <uint8_t> (awh::event::action_t::HDLINK):
				// Выводим сообщение о изменении счётчика жёстких ссылок события
				this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
		}
	});
	// Выполняем фиксацию настроек события сервера
	ASSERT_TRUE(this->_io->commit(eid));
	// Запускаем дочерний поток для уведомления события
	std::thread([this](const awh::event::id_t eid) noexcept -> void {
		// Текст сообщения
		const std::string message = "Hello AWH IO Event!";
		// Уведомляем событие
		this->_io->send(eid, reinterpret_cast <const char *> (message.c_str()), message.length());
	}, eid).detach();
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}
