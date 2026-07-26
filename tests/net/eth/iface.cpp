/**
 * @file: iface.cpp
 * @date: 2026-02-06
 * @license: LicenseRef-AWH-1.0
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
 * @brief Вспомогательная функция поиска петлевого (loopback) сетевого интерфейса
 *
 * @param eth объект работы с Ethernet
 * @return    имя петлевого интерфейса либо пустая строка
 */
static std::string findLoopback(const awh::eth_t * eth) noexcept {
	/**
	 * Перебираем все доступные сетевые интерфейсы
	 */
	for(auto & name : eth->iface.available()){
		// Получаем флаги сетевого интерфейса
		auto flags = eth->iface.flags(name);
		// Если интерфейс является петлевым
		if(flags.find(awh::event::eth_flag_t::LOOPBACK) != flags.end())
			// Возвращаем найденное имя
			return name;
	}
	// Возвращаем пустое имя
	return std::string{};
}

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

/**
 * @brief Тест безопасной обработки нулевого адреса
 *
 */
TEST_F(EthFixture, IfaceNullAddrTest){
	// Нулевой адрес сетевого подключения
	const awh::net::addr_t * nullAddr = nullptr;
	// Получение имени по нулевому адресу должно вернуть пустую строку без падения
	ASSERT_TRUE(this->_eth->iface.name(nullAddr).empty());
	// Проверки по нулевому адресу должны вернуть false без падения
	ASSERT_FALSE(this->_eth->iface.isTunnel(nullAddr));
	ASSERT_FALSE(this->_eth->iface.isVirtual(nullAddr));
}

/**
 * @brief Тест безопасной обработки пустого имени интерфейса
 *
 */
TEST_F(EthFixture, IfaceEmptyNameTest){
	// Пустое имя сетевого интерфейса
	const std::string empty = "";
	// Все методы должны безопасно обрабатывать пустое имя
	ASSERT_FALSE(this->_eth->iface.isAvailable(empty));
	ASSERT_FALSE(this->_eth->iface.isTunnel(empty));
	ASSERT_FALSE(this->_eth->iface.isVirtual(empty));
	ASSERT_EQ(this->_eth->iface.mtu(empty), 0);
	ASSERT_FALSE(this->_eth->iface.mtu(empty, 1500));
	ASSERT_TRUE(this->_eth->iface.flags(empty).empty());
	ASSERT_FALSE(this->_eth->iface.flag(empty, awh::event::eth_flag_t::UP, awh::event::mode_t::ENABLED));
	ASSERT_FALSE(this->_eth->iface.destroy(empty));
	ASSERT_EQ(this->_eth->iface.getAddress(empty, awh::event::family_t::IPV4), nullptr);
	// Создаём объект IPv4 адреса для проверки сеттеров
	auto ip = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес петлевого сетевого интерфейса
	ip->address = htonl(INADDR_LOOPBACK);
	// Установка адреса и комплексная настройка с пустым именем должны вернуть false
	ASSERT_FALSE(this->_eth->iface.setAddress(empty, ip.get(), 24));
	ASSERT_FALSE(this->_eth->iface.configure(empty, ip.get(), 24, 1500));
}

/**
 * @brief Тест безопасной обработки несуществующего интерфейса
 *
 */
TEST_F(EthFixture, IfaceNonExistentTest){
	// Заведомо несуществующее имя сетевого интерфейса
	const std::string fake = "non_existent_iface_123";
	// Несуществующий интерфейс недоступен
	ASSERT_FALSE(this->_eth->iface.isAvailable(fake));
	// MTU несуществующего интерфейса равен нулю
	ASSERT_EQ(this->_eth->iface.mtu(fake), 0);
	// Список флагов несуществующего интерфейса пуст
	ASSERT_TRUE(this->_eth->iface.flags(fake).empty());
	// Классификация несуществующего интерфейса возвращает false
	ASSERT_FALSE(this->_eth->iface.isTunnel(fake));
	ASSERT_FALSE(this->_eth->iface.isVirtual(fake));
}

/**
 * @brief Тест классификации петлевого интерфейса как виртуального
 *
 */
TEST_F(EthFixture, IfaceLoopbackVirtualTest){
	// Ищем петлевой интерфейс
	const std::string lo = findLoopback(this->_eth.get());
	// Если петлевой интерфейс найден
	if(!lo.empty()){
		// Петлевой интерфейс должен быть доступен
		ASSERT_TRUE(this->_eth->iface.isAvailable(lo));
		// Петлевой интерфейс всегда виртуальный
		ASSERT_TRUE(this->_eth->iface.isVirtual(lo));
		// MTU петлевого интерфейса больше нуля
		ASSERT_GT(this->_eth->iface.mtu(lo), 0);
		// В флагах петлевого интерфейса присутствует LOOPBACK
		auto flags = this->_eth->iface.flags(lo);
		ASSERT_NE(flags.find(awh::event::eth_flag_t::LOOPBACK), flags.end());
	}
}

/**
 * @brief Тест инварианта: туннельный интерфейс обязан быть виртуальным
 *
 */
TEST_F(EthFixture, IfaceTunnelIsVirtualTest){
	/**
	 * Перебираем все доступные сетевые интерфейсы
	 */
	for(auto & name : this->_eth->iface.available()){
		// Если интерфейс является туннельным
		if(this->_eth->iface.isTunnel(name))
			// Туннель обязан классифицироваться и как виртуальный (туннель ⊂ виртуальный)
			ASSERT_TRUE(this->_eth->iface.isVirtual(name));
	}
}

/**
 * @brief Тест согласованности проверок по адресу и по имени (единый проход getifaddrs)
 *
 */
TEST_F(EthFixture, IfaceNameByLoopbackAddrTest){
	// Создаём объект IPv4 адреса
	auto addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес петлевого сетевого интерфейса
	addr->address = htonl(INADDR_LOOPBACK);
	// Получаем имя интерфейса по адресу
	const std::string name = this->_eth->iface.name(addr.get());
	// Если имя найдено
	if(!name.empty()){
		// Найденный интерфейс должен быть доступен
		ASSERT_TRUE(this->_eth->iface.isAvailable(name));
		// Проверки по адресу и по имени должны давать одинаковый результат
		ASSERT_EQ(this->_eth->iface.isVirtual(addr.get()), this->_eth->iface.isVirtual(name));
		ASSERT_EQ(this->_eth->iface.isTunnel(addr.get()), this->_eth->iface.isTunnel(name));
	}
}

/**
 * @brief Тест получения адресов разных семейств без падения
 *
 */
TEST_F(EthFixture, IfaceGetAddressFamiliesTest){
	/**
	 * Перебираем все доступные сетевые интерфейсы
	 */
	for(auto & name : this->_eth->iface.available()){
		// Получаем IPv4-адрес интерфейса
		this->_eth->iface.getAddress(name, awh::event::family_t::IPV4);
		// Получаем IPv6-адрес интерфейса
		this->_eth->iface.getAddress(name, awh::event::family_t::IPV6);
	}
}

/**
 * @brief Тест извлечения префикса подсети петлевого интерфейса
 *
 */
TEST_F(EthFixture, IfacePrefixTest){
	// Ищем петлевой интерфейс
	const std::string lo = findLoopback(this->_eth.get());
	// Если петлевой интерфейс найден
	if(!lo.empty()){
		// IP-адрес сетевого интерфейса
		std::unique_ptr <awh::net::addr_t> ip = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// IP-адрес удалённого пира
		std::unique_ptr <awh::net::addr_t> peer = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Префикс подсети
		uint8_t prefix = 0;
		// Если параметры IPv4 успешно извлечены
		if(this->_eth->iface.getAddress(lo, ip, peer, prefix))
			// Префикс должен быть в допустимом диапазоне
			ASSERT_LE(prefix, 32);
	}
}

/**
 * @brief Тест безопасной обработки некорректных аргументов комплексной настройки
 *
 */
TEST_F(EthFixture, IfaceConfigureGuardTest){
	// Создаём объект IPv4 адреса
	auto ip = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес петлевого сетевого интерфейса
	ip->address = htonl(INADDR_LOOPBACK);
	// Комплексная настройка с пустым именем недопустима
	ASSERT_FALSE(this->_eth->iface.configure("", ip.get(), 24, 1500));
	// Комплексная настройка с нулевым адресом недопустима
	ASSERT_FALSE(this->_eth->iface.configure("lo0", static_cast <const awh::net::addr_t *> (nullptr), 24, 1500));
	// Создаём объект IPv6 адреса для проверки несовпадения типов
	auto peer6 = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Комплексная настройка точка-точка с разными типами адреса и пира недопустима
	ASSERT_FALSE(this->_eth->iface.configure("lo0", ip.get(), peer6.get(), 24, 1500));
}

/**
 * @brief Тест вызова комплексной настройки на реальном интерфейсе
 *
 */
TEST_F(EthFixture, IfaceConfigureCallTest){
	// Ищем петлевой интерфейс
	const std::string lo = findLoopback(this->_eth.get());
	// Если петлевой интерфейс найден
	if(!lo.empty()){
		// Получаем текущий IPv4-адрес петлевого интерфейса
		auto ip = this->_eth->iface.getAddress(lo, awh::event::family_t::IPV4);
		// Если адрес получен (фактическое применение требует прав суперпользователя)
		if(ip != nullptr)
			// Вызов комплексной настройки не должен приводить к падению (MTU не меняем)
			this->_eth->iface.configure(lo, ip.get(), 8, 0);
	}
}
