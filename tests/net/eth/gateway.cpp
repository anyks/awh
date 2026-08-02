/**
 * @file: gateway.cpp
 * @date: 2026-02-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты работы со шлюзами — проверка чтения таблицы маршрутизации,
 *        определения шлюза по умолчанию и разбора параметров маршрута
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы
 */
#include <arpa/inet.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Тест получения маршрута
 *
 */
TEST_F(EthFixture, GatewayGetTest){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Если получаем маршрут для указанного адреса
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Записываем в лог информацию о найденном маршруте
	std::cout << "Gateway found:" << std::endl;
	// Записываем в лог информацию о маршруте
	std::cout << " Interface: " << route.ifname << std::endl;
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Получаем IP-адрес текущего шлюза по умолчанию
	const std::string gateway = static_cast <std::string> (* this->_addr.get());
	// Возвращаем адрес шлюза по умолчанию
	std::cout << "Default Gateway: " << gateway << std::endl;
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Возвращаем адрес назначения
	std::cout << "Destination: " << static_cast <std::string> (* this->_addr.get()) << "/" << static_cast <uint32_t> (route.prefix) << std::endl;
	// Если пользователь является привилигированным
	if(::getuid() == 0)
		// Удаляем маршрут по указанному адресу
		ASSERT_TRUE(this->_eth->gateway.remove(route));
	/**
	 * sudo route add default 192.168.7.1
	 * sudo route delete default 0.0.0.0
	 */
	// Выполняем парсинг адреса нового шлюза
	(* this->_addr.get()) = "192.168.7.131";
	// Устанавливаем адрес шлюза в маршрут
	awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Устанавливаем имя сетевого интерфейса
	// route.ifname = "en0";
	// Сбрасываем адрес назначения
	// awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = 0;
	// Если пользователь является привилигированным
	if(::getuid() == 0)
		// Добавляем маршрут с новым шлюзом
		ASSERT_TRUE(this->_eth->gateway.add(route));
	// Сбрасываем имя сетевого интерфейса
	// route.ifname = "";
	// Если пользователь является привилигированным
	if(::getuid() == 0)
		// Если получаем маршрут для указанного адреса
		ASSERT_TRUE(this->_eth->gateway.get(route));
	// Записываем в лог информацию о маршруте
	std::cout << " Interface: " << route.ifname << std::endl;
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Возвращаем адрес шлюза по умолчанию
	std::cout << "Default Gateway: " << static_cast <std::string> (* this->_addr.get()) << std::endl;
	// Если пользователь является привилигированным
	if(::getuid() == 0)
		// Удаляем маршрут по указанному адресу
		ASSERT_TRUE(this->_eth->gateway.remove(route));
	// Выполняем парсинг адреса нового шлюза
	(* this->_addr.get()) = gateway;
	// Устанавливаем адрес шлюза в маршрут
	awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Если пользователь является привилигированным
	if(::getuid() == 0)
		// Добавляем маршрут с новым шлюзом
		ASSERT_TRUE(this->_eth->gateway.add(route));
}
/**
 * @brief Тест получения маршрута по умолчанию IPv4 и разрешения имени интерфейса
 *
 */
TEST_F(EthFixture, GatewayGetDefaultIPv4){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Адрес шлюза по умолчанию должен быть ненулевым
	ASSERT_NE(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, 0U);
	// Адрес назначения маршрута по умолчанию должен быть нулевым
	ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address, 0U);
	// Префикс маршрута по умолчанию должен быть нулевым
	ASSERT_EQ(static_cast <uint32_t> (route.prefix), 0U);
	// Имя сетевого интерфейса должно быть определено (регрессия на разрешение через if_indextoname)
	ASSERT_FALSE(route.ifname.empty());
}
/**
 * @brief Тест получения маршрута по умолчанию IPv4 без инициализации адреса назначения
 *
 */
TEST_F(EthFixture, GatewayGetDefaultIPv4NullDestination){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем только объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Объект адреса назначения намеренно не инициализируем (должен создаться автоматически)
	ASSERT_EQ(route.destination, nullptr);
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Объект адреса назначения должен быть создан автоматически
	ASSERT_NE(route.destination, nullptr);
	// Адрес шлюза по умолчанию должен быть ненулевым
	ASSERT_NE(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, 0U);
}
/**
 * @brief Тест поиска маршрута по адресу шлюза IPv4 (без привилегий)
 *
 */
TEST_F(EthFixture, GatewayGetByGatewayIPv4){
	// Структура маршрута для поиска шлюза по умолчанию
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Запоминаем найденный адрес шлюза по умолчанию
	const uint32_t gateway = awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address;
	// Структура маршрута для поиска по адресу шлюза
	awh::eth::gateway_t::route_t search{};
	// Инициализируем объект адреса шлюза в маршруте
	search.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	search.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес шлюза для поиска
	awh_cast <awh::net::addr_net_ipv4_t *> (search.gateway.get())->address = gateway;
	// Выполняем поиск маршрута по адресу шлюза
	ASSERT_TRUE(this->_eth->gateway.get(search));
	// Найденный адрес шлюза должен совпадать с искомым
	ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (search.gateway.get())->address, gateway);
}
/**
 * @brief Тест поиска маршрута по имени сетевого интерфейса IPv4 (без привилегий)
 *
 */
TEST_F(EthFixture, GatewayGetByInterfaceIPv4){
	// Структура маршрута для поиска шлюза по умолчанию
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Если имя сетевого интерфейса не определено, пропускаем тест
	if(route.ifname.empty())
		// Завершаем тест успешно
		return;
	// Запоминаем имя сетевого интерфейса маршрута по умолчанию
	const std::string ifname = route.ifname;
	// Структура маршрута для поиска по имени интерфейса
	awh::eth::gateway_t::route_t search{};
	// Инициализируем объект адреса шлюза в маршруте
	search.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	search.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем имя сетевого интерфейса для поиска
	search.ifname = ifname;
	// Выполняем поиск маршрута по имени сетевого интерфейса
	ASSERT_TRUE(this->_eth->gateway.get(search));
	// Найденное имя сетевого интерфейса должно совпадать с искомым
	ASSERT_EQ(search.ifname, ifname);
}
/**
 * @brief Тест поиска несуществующего конкретного маршрута IPv4 (ветка точного совпадения)
 *
 */
TEST_F(EthFixture, GatewayGetSpecificDestinationNotFoundIPv4){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Выполняем парсинг тестового адреса назначения (TEST-NET-3, RFC 5737)
	(* this->_addr.get()) = "203.0.113.123";
	// Устанавливаем конкретный адрес назначения для поиска
	awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Устанавливаем префикс маршрута на хостовый
	route.prefix = 32;
	/**
	 * Адрес, у которого своей записи в таблице нет, разрешается маршрутом по
	 * умолчанию: метод отвечает на вопрос «каким путём система отправит пакет»,
	 * а не «есть ли запись ровно с таким назначением». Прежде здесь закреплялось
	 * обратное - точное сличение поля назначения, - отчего недостижимым
	 * объявлялся и адрес своей же сети
	 */
	const bool result = this->_eth->gateway.get(route);
	// Если маршрут до тестового адреса найден
	if(result){
		// Устройство, которым система отправит пакет, обязано быть названо
		ASSERT_FALSE(route.ifname.empty());
		// Запрошенный адрес назначения ответом затираться не должен
		ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address, this->_addr->v4(awh::net_addr_t::endian_t::LITTLE));
	// Если маршрута нет вовсе, у машины нет и выхода наружу
	} else std::cout << "Маршрут до тестового адреса отсутствует - выхода наружу у машины нет" << std::endl;
}
/**
 * @brief Тест получения маршрута по умолчанию IPv6 (толерантный к отсутствию IPv6)
 *
 */
TEST_F(EthFixture, GatewayGetDefaultIPv6){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Выполняем получение маршрута по умолчанию IPv6 (может отсутствовать)
	const bool result = this->_eth->gateway.get(route);
	// Если маршрут по умолчанию IPv6 найден
	if(result){
		// Нулевой IPv6 адрес для сравнения
		const std::array <uint8_t, 16> zero{0};
		// Адрес шлюза по умолчанию IPv6 должен быть ненулевым
		ASSERT_NE(awh_cast <awh::net::addr_net_ipv6_t *> (route.gateway.get())->address, zero);
		// Имя сетевого интерфейса должно быть определено
		ASSERT_FALSE(route.ifname.empty());
	}
}
/**
 * @brief Тест получения маршрута с неинициализированным адресом шлюза
 *
 */
TEST_F(EthFixture, GatewayGetUninitialized){
	// Структура маршрута без инициализированного адреса шлюза
	awh::eth::gateway_t::route_t route{};
	// Получение маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.get(route));
}
/**
 * @brief Тест получения маршрута с неподдерживаемым семейством адресов
 *
 */
TEST_F(EthFixture, GatewayGetUnsupportedFamily){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Устанавливаем неподдерживаемый тип адреса шлюза (MAC, размер 6)
	route.gateway = std::make_unique <awh::net::addr_mac_t> ();
	// Получение маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.get(route));
}
/**
 * @brief Тест добавления маршрута с неинициализированным адресом шлюза
 *
 */
TEST_F(EthFixture, GatewayAddUninitialized){
	// Структура маршрута без инициализированного адреса шлюза
	awh::eth::gateway_t::route_t route{};
	// Добавление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.add(route));
}
/**
 * @brief Тест добавления маршрута с неподдерживаемым семейством адресов
 *
 */
TEST_F(EthFixture, GatewayAddUnsupportedFamily){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Устанавливаем неподдерживаемый тип адреса шлюза (MAC, размер 6)
	route.gateway = std::make_unique <awh::net::addr_mac_t> ();
	// Добавление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.add(route));
}
/**
 * @brief Тест удаления маршрута с неинициализированным адресом шлюза
 *
 */
TEST_F(EthFixture, GatewayRemoveUninitialized){
	// Структура маршрута без инициализированного адреса шлюза
	awh::eth::gateway_t::route_t route{};
	// Удаление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.remove(route));
}
/**
 * @brief Тест удаления маршрута с неподдерживаемым семейством адресов
 *
 */
TEST_F(EthFixture, GatewayRemoveUnsupportedFamily){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Устанавливаем неподдерживаемый тип адреса шлюза (MAC, размер 6)
	route.gateway = std::make_unique <awh::net::addr_mac_t> ();
	// Удаление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.remove(route));
}
