/**
 * @file: portmap.cpp
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

/**
 * @brief Тест получения списка пробросов
 *
 */
TEST_F(EthFixture, PortMapMappingsTest){
	// Получаем список открытых портов
	auto mappings = this->_eth->portmap.mappings();
	// Список может быть пустым, это нормально
	ASSERT_TRUE(mappings.empty() || !mappings.empty());
}

/**
 * @brief Тест установки проброса
 *
 */
TEST_F(EthFixture, PortMapMappingTest){
	// Создаём структуру проброса порта на маршрутизаторе
	awh::eth::portmap_t::fwd_t fwd{};
	// Заполняем параметры проброса порта
	fwd.lifeTime = 3600;
	// Устанавливаем внутренний порт
	fwd.internalPort = 8081;
	// Устанавливаем внешний порт
	fwd.externalPort = 8080;
	// Устанавливаем протокол проброса порта
	fwd.proto = awh::eth::portmap_t::proto_t::TCP;
	// Устанавливаем тип проброса порта
	fwd.type = awh::eth::portmap_t::type_t::NAT_PMP;
	// Инициализируем объект внутреннего IPv4-адреса
	fwd.internalAddress = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект внешнего IPv4-адреса
	fwd.externalAddress = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Выполняем парсинг внутреннего IPv4-адреса
	(* this->_addr.get()) = "192.168.7.215";
	// Устанавливаем внутренний IPv4-адрес в пробросе порта
	awh_cast <awh::net::addr_net_ipv4_t *> (fwd.internalAddress.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Выполняем парсинг внешнего IPv4-адреса
	(* this->_addr.get()) = "123.456.78.90";
	// Устанавливаем внешний IPv4-адрес в пробросе порта
	awh_cast <awh::net::addr_net_ipv4_t *> (fwd.externalAddress.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Устанавливаем описание проброса порта
	::memcpy(fwd.description, "AWH Web Server", ::strlen("AWH Web Server") + 1);
	// Устанавливаем завершение строки описания проброса порта
	fwd.description[::strlen("AWH Web Server")] = '\0';
	// Выполняем проброс порта на маршрутизаторе
	ASSERT_TRUE(this->_eth->portmap.mapping(fwd, awh::event::mode_t::ENABLED));
	/**
	 * Выводим информацию о проброшенных портах на маршрутизаторе
	 */
	for(auto & fwd : this->_eth->portmap.mappings()){
		// Если внутренний адрес является IPv4
		if(fwd.internalAddress->size == 4){
			// Устанавливаем полученный внутренний IPv4-адрес
			this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (fwd.internalAddress.get())->address, awh::net_addr_t::endian_t::LITTLE);
			// Выводим адрес внутреннего IPv4-адреса
			std::cout << "Internal IP: " << static_cast <std::string> (* this->_addr.get()) << ":" << fwd.internalPort << std::endl;
		}
		// Если внешний адрес является IPv4
		if(fwd.externalAddress->size == 4){
			// Устанавливаем полученный внешний IPv4-адрес
			this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (fwd.externalAddress.get())->address, awh::net_addr_t::endian_t::LITTLE);
			// Выводим адрес внешнего IPv4-адреса
			std::cout << "External IP: " << static_cast <std::string> (* this->_addr.get()) << ":" << fwd.externalPort << std::endl;
		}
		// Выводим информацию о проброшенных портах
		std::cout << " Protocol: " << static_cast <uint16_t> (fwd.proto) << " Type: " << static_cast <uint16_t> (fwd.type)
			<< " Description: " << fwd.description << ", TTL: " << fwd.lifeTime << " seconds" << std::endl;
	}
	// Удаляем проброс порта на маршрутизаторе
	fwd = awh::eth::portmap_t::fwd_t();
	// Устанавливаем внутренний порт
	fwd.internalPort = 8081;
	// Устанавливаем внешний порт
	fwd.externalPort = 8080;
	// Устанавливаем описание проброса порта
	::memcpy(fwd.description, "AWH Web Server", ::strlen("AWH Web Server") + 1);
	// Устанавливаем завершение строки описания проброса порта
	fwd.description[::strlen("AWH Web Server")] = '\0';
	// Устанавливаем протокол проброса порта
	fwd.proto = awh::eth::portmap_t::proto_t::TCP;
	// Устанавливаем тип проброса порта
	fwd.type = awh::eth::portmap_t::type_t::NAT_PMP;
	// Выполняем удаление проброса порта на маршрутизаторе
	ASSERT_TRUE(this->_eth->portmap.mapping(fwd, awh::event::mode_t::DISABLED));
}
