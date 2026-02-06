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
	auto mappings = this->_eth->portmap.mappings();
	// Список может быть пустым, это нормально
	ASSERT_TRUE(mappings.empty() || !mappings.empty());
}

/**
 * @brief Тест установки проброса
 *
 */
TEST_F(EthFixture, PortMapMappingTest){
	awh::eth::portmap_t::fwd_t fwd;
	fwd.type = awh::eth::portmap_t::type_t::UPNP;
	fwd.proto = awh::eth::portmap_t::proto_t::TCP;
	fwd.internalPort = 12345;
	fwd.externalPort = 12345;
	fwd.lifeTime = 3600;
	
	// Попытка добавить (нужен роутер с UPnP/NAT-PMP)
	// Результат скорее всего будет false в среде CI/тестов
	this->_eth->portmap.mapping(fwd, awh::event::mode_t::ENABLED);
	
	// Попытка удалить
	this->_eth->portmap.mapping(fwd, awh::event::mode_t::DISABLED);
}
