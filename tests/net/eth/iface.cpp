/**
 * @file iface.cpp
 * @date 2026-02-06
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Тесты работы с сетевыми интерфейсами — проверка перечисления интерфейсов машины, получения их адресов,
 *        флагов, MTU и состояния
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы
 */
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют:
 *       соответствующие им объявления приходят там из winsock2.h,
 *       подключаемого через единую точку sys/win32.hpp
 *
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <arpa/inet.h>
	#include <netinet/in.h>
#endif
#include <sys/types.h>

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Вспомогательная функция поиска петлевого (loopback) сетевого интерфейса
 *
 * @param eth объект работы с Ethernet
 * @return    имя петлевого интерфейса либо пустая строка
 *
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
		/**
		 * @note Значения признаков зависят от системы, а их согласованность - нет:
		 *       устройство туннеля обязано быть виртуальным всегда, обратное неверно.
		 *       Прежде проверка звала оба опроса и исхода не смотрела
		 */
		// Если устройство опознано туннелем
		if(this->_eth->iface.isTunnel(ifname))
			// Устройство туннеля обязано быть виртуальным
			ASSERT_TRUE(this->_eth->iface.isVirtual(ifname))
			 << "Устройство " << ifname << " опознано туннелем, но не виртуальным";
		// Признак туннеля обязан читаться одинаково при повторном опросе
		ASSERT_EQ(this->_eth->iface.isTunnel(ifname), this->_eth->iface.isTunnel(ifname));
		// Признак виртуальности обязан читаться одинаково при повторном опросе
		ASSERT_EQ(this->_eth->iface.isVirtual(ifname), this->_eth->iface.isVirtual(ifname));
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
	/**
	 * @note Петлевое устройство туннелем не является ни у одной системы, а виртуальным
	 *       является у всех: признаки от него определены однозначно и годны утверждению
	 */
	// Петлевой адрес туннелю принадлежать не может
	ASSERT_FALSE(this->_eth->iface.isTunnel(addr.get()))
	 << "Петлевой адрес опознан принадлежащим туннелю";
	// Петлевое устройство обязано быть виртуальным
	ASSERT_TRUE(this->_eth->iface.isVirtual(addr.get()))
	 << "Петлевое устройство не опознано виртуальным";
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
	/**
	 * @note Нулевой адрес - это INADDR_ANY, и система выдаёт его связям без адреса.
	 *       Устройству он принадлежать не может: розыск по нему обязан отвечать пусто.
	 *       Прежде проверка звала розыск и исхода не смотрела вовсе
	 */
	// Розыск устройства по нулевому адресу обязан отвечать пусто
	ASSERT_TRUE(this->_eth->iface.name(addr.get()).empty())
	 << "Нулевой адрес опознан принадлежащим устройству";
	/**
	 * Заслон стоит на ТРЁХ видах адреса, и проверяется каждый
	 *
	 * @details Нулевой адрес значит «любой»: искать по нему нечего, и ответом обязана
	 *          быть пустота. Прежде здесь проверялся лишь нулевой IPv4, а ветви IPv6 и
	 *          MAC заслона не проверялись ничем - выломать их можно было, не уронив ни
	 *          одной проверки
	 *
	 * @note Заведён заслон по находке на Solaris и OpenIndiana: перебор выдавал там
	 *       связь без адреса с нулевым адресом, и розыск возвращал её имя. Договор
	 *       общий на все движки, оттого и проверка общая
	 */
	// Создаём объект IPv6 адреса
	std::unique_ptr <awh::net::addr_t> addr6 = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Зануляем адрес целиком
	::memset(&static_cast <awh::net::addr_net_ipv6_t *> (addr6.get())->address[0], 0, 16);
	// Розыск устройства по нулевому адресу IPv6 обязан отвечать пусто
	ASSERT_TRUE(this->_eth->iface.name(addr6.get()).empty())
	 << "Нулевой адрес IPv6 опознан принадлежащим устройству";
	// Создаём объект аппаратного адреса
	std::unique_ptr <awh::net::addr_t> mac = std::make_unique <awh::net::addr_mac_t> ();
	// Зануляем аппаратный адрес целиком
	::memset(&static_cast <awh::net::addr_mac_t *> (mac.get())->address[0], 0, 6);
	// Розыск устройства по нулевому аппаратному адресу обязан отвечать пусто
	ASSERT_TRUE(this->_eth->iface.name(mac.get()).empty())
	 << "Нулевой аппаратный адрес опознан принадлежащим устройству";
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
	/**
	 * @note Заведение требует надзорных прав, и отказ без них законен. А вот заведённое
	 *       устройство обязано быть НАЗВАННЫМ и опознаваемым: имя не смеет остаться
	 *       пустым, а перечень устройств обязан его показывать. Прежде проверка звала
	 *       заведение с уничтожением и ни того ни другого не смотрела
	 */
	// Если пользователь является привилигированным
	if(::getuid() == 0)
		// Заведение устройства туннеля обязано удаваться
		ASSERT_NE(sock, awh::net::invalid_socket_t) << "Устройство туннеля завести не удалось";
	// Если сокет создан успешно
	if(sock != awh::net::invalid_socket_t){
		// Название заведённого устройства обязано быть определено
		ASSERT_FALSE(name.empty()) << "Устройство заведено, а имени у него нет";
		// Заведённое устройство обязано числиться доступным
		ASSERT_TRUE(this->_eth->iface.isAvailable(name))
		 << "Заведённое устройство " << name << " не числится доступным";
		// Заведённое устройство туннеля обязано опознаваться виртуальным
		ASSERT_TRUE(this->_eth->iface.isVirtual(name))
		 << "Устройство туннеля " << name << " не опознано виртуальным";
		// Если создался, закрываем и уничтожаем
		::closesocket(sock);
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
	/**
	 * Перебираем все перечисленные сетевые интерфейсы
	 *
	 * @note Прежде проверка брала первый интерфейс из `unordered_set`, порядок в
	 *       котором не определён: на большинстве систем первым выпадал обычный
	 *       интерфейс, а на OpenIndiana - связь канального уровня `etherstub` без
	 *       поднятого уровня IP, у которой размер не читался вовсе. Отказ выглядел
	 *       свойством системы, хотя был расхождением перечисления с опросом
	 *
	 * @note Спрашивать размер у каждого имени - это и есть договор: имя, названное
	 *       `available()`, обязано отвечать и у `mtu()`
	 */
	for(auto & ifname : interfaces){
		// Получаем значение MTU для сетевого интерфейса
		const uint32_t mtu = this->_eth->iface.mtu(ifname);
		/**
		 * Спрашиваем размер лишь у интерфейсов, которые уровню IP известны
		 *
		 * @note Пустой набор признаков означает, что интерфейса IP у имени нет вовсе,
		 *       а перечислен оно записью канального уровня. Размера у такого имени
		 *       может не быть по устройству системы: OpenBSD заводит `enc0` для
		 *       упаковки IPsec, и ядро не называет ему MTU вовсе - `ifconfig` тоже
		 *       печатает его без размера. Требовать размер у такого имени значило бы
		 *       требовать от системы того, чего у неё нет
		 *
		 * @note Обратный случай тоже есть: у illumos связь `etherstub` уровня IP не
		 *       имеет, а размер у неё есть, и модуль обязан его назвать - берётся он
		 *       у канального уровня. Проверяется это замером на стенде,
		 *       см. src/net/backend/sun/iface.cpp
		 */
		if(this->_eth->iface.flags(ifname).empty())
			// Переходим к следующему интерфейсу
			continue;
		// Проверяем, что размер получен
		ASSERT_GT(mtu, 0U) << "Интерфейс \"" << ifname << "\" перечислен, но размер MTU у него не читается";
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
		/**
		 * @note Перечень признаков обязан быть согласован с самим собой: устройство,
		 *       названное поднятым, обязано остаться поднятым и после повторного
		 *       поднятия. Прежде проверка звала установку и исхода не смотрела
		 */
		if(flags.find(awh::event::eth_flag_t::UP) != flags.end()){
			// Выполняем повторное поднятие устройства
			const bool result = this->_eth->iface.flag(ifname, awh::event::eth_flag_t::UP, awh::event::mode_t::ENABLED);
			/**
			 * @note Правка признаков требует надзорных прав, и отказ без них законен -
			 *       утверждается он только под ними. А вот СОСТОЯНИЕ устройства обязано
			 *       уцелеть при любом исходе: поднятое не смеет опуститься оттого, что
			 *       его попросили подняться ещё раз
			 */
			// Если пользователь является привилигированным
			if(::getuid() == 0)
				// Поднятие уже поднятого устройства обязано удаваться
				ASSERT_TRUE(result) << "Поднять уже поднятое устройство " << ifname << " не удалось";
			// Получаем перечень признаков устройства заново
			const auto actual = this->_eth->iface.flags(ifname);
			// Устройство обязано остаться поднятым
			ASSERT_NE(actual.find(awh::event::eth_flag_t::UP), actual.end())
			 << "Устройство " << ifname << " перестало числиться поднятым после поднятия";
		}
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
 * @brief Тест полного круга адреса на своём устройстве: постановка, чтение, снятие
 *
 * @details `IfaceAddressTest` рядом зовёт `setAddress` и `getAddress`, но исхода их не
 *          утверждает: она проходит и при движке, который молчит. Здесь круг ставится на
 *          СВОЁМ устройстве - туннеле, заведённом тут же, - и каждый шаг проверяется:
 *          адрес обязан читаться после постановки и пропадать после снятия. Действующие
 *          устройства стенда при этом не трогаются
 *
 */
TEST_F(EthFixture, IfaceAddressLifecycleTest){
	// Если пользователь не является привилигированным
	if(::getuid() != 0)
		// Пропускаем проверку
		GTEST_SKIP() << "Для заведения устройства нужны права суперпользователя";
	// Название созданного тоннеля
	std::string name = "";
	// Выполняем создание устройства туннеля
	const awh::net::socket_t sock = this->_eth->iface.create(awh::event::eth_t::TUN, name);
	// Если устройство туннеля создать не удалось
	if(sock == awh::net::invalid_socket_t)
		// Пропускаем проверку
		GTEST_SKIP() << "Устройство туннеля завести не удалось";
	// Название устройства обязано быть определено
	ASSERT_FALSE(name.empty()) << "Устройство заведено, а имени у него нет";
	// Объект адреса устройства
	std::unique_ptr <awh::net::addr_t> ip = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Выполняем парсинг адреса испытательной сети
	awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
	// Устанавливаем адрес испытательной сети
	addr = "192.0.2.17";
	// Устанавливаем адрес устройства
	awh_cast <awh::net::addr_net_ipv4_t *> (ip.get())->address = addr.v4(awh::net_addr_t::endian_t::LITTLE);
	// Выполняем постановку адреса на устройство
	const bool result = this->_eth->iface.setAddress(name, ip.get(), 24);
	// Если адрес поставлен
	if(result){
		/**
		 * @note Устройство поднимается до чтения намеренно: `getifaddrs` не показывает
		 *       адреса устройства, пока оно не поднято. Проверено щупом на Debian -
		 *       система при этом адрес уже числит (`ip addr` его показывает), а перечень
		 *       устройств его ещё не отдаёт. Без поднятия проверка обвиняла бы движок в
		 *       том, что делает система
		 */
		// Поднимаем устройство туннеля
		ASSERT_TRUE(this->_eth->iface.flag(name, awh::event::eth_flag_t::UP, awh::event::mode_t::ENABLED))
		 << "Поднять устройство " << name << " не удалось";
		// Читаем адрес устройства обратно
		auto actual = this->_eth->iface.getAddress(name, awh::event::family_t::IPV4);
		// Прочитанный адрес обязан существовать
		ASSERT_NE(actual, nullptr) << "Адрес поставлен на " << name << ", а читается пустым";
		// Прочитанный адрес обязан совпадать с поставленным
		ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (actual.get())->address,
		          awh_cast <awh::net::addr_net_ipv4_t *> (ip.get())->address)
		 << "Прочитанный адрес не совпадает с поставленным";
		// Название устройства по его же адресу обязано определяться
		ASSERT_EQ(this->_eth->iface.name(actual.get()), name)
		 << "Адрес не опознаётся принадлежащим устройству " << name;
		/**
		 * @note Снятие адреса объявлено не у всех систем: у Linux и BSD устройство
		 *       туннеля исчезает вместе с процессом, и отдельного снятия там нет.
		 *       Где метод есть, круг проверяется целиком; где его нет - исчезновение
		 *       адреса проверяется уничтожением самого устройства ниже
		 */
		#if _WIN32 || _WIN64 || __sun__ || __APPLE__
			// Выполняем снятие адреса с устройства
			ASSERT_TRUE(this->_eth->iface.delAddress(name, ip.get(), nullptr))
			 << "Снять поставленный адрес с устройства " << name << " не удалось";
			// Читаем адрес устройства после снятия
			auto removed = this->_eth->iface.getAddress(name, awh::event::family_t::IPV4);
			// Снятый адрес читаться больше не должен
			ASSERT_TRUE((removed == nullptr) ||
			            (awh_cast <awh::net::addr_net_ipv4_t *> (removed.get())->address !=
			             awh_cast <awh::net::addr_net_ipv4_t *> (ip.get())->address))
			 << "Адрес читается с устройства " << name << " после снятия";
		#endif
	}
	// Закрываем сокет устройства туннеля
	::closesocket(sock);
	// Выполняем уничтожение устройства туннеля
	this->_eth->iface.destroy(name);
	// Если адрес был поставлен
	if(result){
		// Читаем адрес уничтоженного устройства
		auto orphan = this->_eth->iface.getAddress(name, awh::event::family_t::IPV4);
		// Адрес уничтоженного устройства читаться больше не должен
		ASSERT_TRUE((orphan == nullptr) ||
		            (awh_cast <awh::net::addr_net_ipv4_t *> (orphan.get())->address !=
		             awh_cast <awh::net::addr_net_ipv4_t *> (ip.get())->address))
		 << "Адрес читается с устройства " << name << " после его уничтожения";
	}
	/**
	 * @note Отступления здесь больше НЕТ, и это намеренно: постановка адреса на СВОЁ
	 *       устройство, заведённое тут же и под надзорными правами, обязана удаваться.
	 *       Прежде проверка при отказе отступала - и этим прятала настоящий дефект: у
	 *       систем Sun итог постановки решал необязательный широковещательный адрес,
	 *       которого у устройства точка-точка нет вовсе, и `setAddress` отвечал отказом
	 *       при уже поставленном адресе
	 */
	// Постановка адреса на своё устройство обязана удаваться
	ASSERT_TRUE(result) << "Поставить адрес на своё устройство " << name << " не удалось";
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
	/**
	 * @note Петлевой адрес принадлежит петлевому устройству у всякой системы: розыск по
	 *       нему обязан отвечать именем. Прежде пустой ответ проходил молча, не утвердив
	 *       ничего
	 */
	// Розыск устройства по петлевому адресу обязан отвечать именем
	ASSERT_FALSE(name.empty()) << "Петлевой адрес не опознан принадлежащим устройству";
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
		auto v4 = this->_eth->iface.getAddress(name, awh::event::family_t::IPV4);
		// Получаем IPv6-адрес интерфейса
		auto v6 = this->_eth->iface.getAddress(name, awh::event::family_t::IPV6);
		/**
		 * @note Наличие адреса зависит от устройства, а вид выданного - нет: спросили
		 *       IPv4 - обязан прийти IPv4. Прежде проверка звала оба опроса и на
		 *       выданное не смотрела вовсе
		 */
		// Если адрес IPv4 получен
		if(v4 != nullptr)
			// Размер выданного адреса обязан отвечать запрошенному виду
			ASSERT_EQ(v4->size, 4) << "У устройства " << name << " вместо адреса IPv4 выдан адрес иного вида";
		// Если адрес IPv6 получен
		if(v6 != nullptr)
			// Размер выданного адреса обязан отвечать запрошенному виду
			ASSERT_EQ(v6->size, 16) << "У устройства " << name << " вместо адреса IPv6 выдан адрес иного вида";
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
		if(ip != nullptr){
			// Вызов комплексной настройки не должен приводить к падению (MTU не меняем)
			this->_eth->iface.configure(lo, ip.get(), 8, 0);
			/**
			 * @note Настройка требует надзорных прав, и отказ без них законен: утверждается
			 *       не её исход, а неприкосновенность петлевого устройства. Настройка тем же
			 *       адресом, каким оно уже настроено, не смеет ни отнять у него адрес, ни
			 *       опустить его. Прежде проверка звала настройку и не смотрела вовсе,
			 *       чем она кончилась
			 */
			// Получаем адрес петлевого устройства заново
			auto actual = this->_eth->iface.getAddress(lo, awh::event::family_t::IPV4);
			// Адрес петлевого устройства обязан уцелеть
			ASSERT_NE(actual, nullptr) << "Петлевое устройство " << lo << " осталось без адреса после настройки";
			// Адрес петлевого устройства обязан остаться прежним
			ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (actual.get())->address,
			          awh_cast <awh::net::addr_net_ipv4_t *> (ip.get())->address)
			 << "Адрес петлевого устройства " << lo << " изменился после настройки тем же адресом";
			// Получаем перечень признаков петлевого устройства
			const auto flags = this->_eth->iface.flags(lo);
			// Петлевое устройство обязано остаться поднятым
			ASSERT_NE(flags.find(awh::event::eth_flag_t::UP), flags.end())
			 << "Петлевое устройство " << lo << " опустилось после настройки";
		}
	}
}

/**
 * @brief Тест розыска сетевого устройства по его аппаратному адресу
 *
 * @details Аппаратный адрес метод `name` принимает наравне с сетевым, и проверка на
 *          пустоту такого адреса у него стоит - то есть довод такого рода он разбирает.
 *          Закрепляется здесь то, что разбор этот доводит до ответа: под MS Windows
 *          ветви розыска по аппаратному адресу не было НИ ОДНОЙ, и метод молча отвечал
 *          пустым названием при живом устройстве
 *
 * @note Аппаратный адрес берётся у самой библиотеки - связкой `fillSource`, - а не
 *       задаётся числом: свой у всякой машины, и вписать его в проверку нельзя
 *
 */
TEST_F(EthFixture, IfaceNameByHardwareAddrTest){
	/**
	 * Связка сведений об устройстве, каким машина выходит наружу
	 *
	 * @note Заводится она сетевым адресом: пустого конструктора у связки нет вовсе -
	 *       адрес сети приходит извне и хранится ею с самого заведения
	 */
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем заполнение связки сведений об устройстве
	this->_eth->addr.fillSource(source);
	// Если аппаратного адреса у машины не нашлось - проверять нечего
	if((source.mac == nullptr) || source.iface.empty()){
		// Сообщаем о пропуске проверки с доводом
		GTEST_SKIP() << "Аппаратный адрес устройства выхода наружу машиной не выдан";
		// Выходим из проверки
		return;
	}
	// Признак того, что аппаратный адрес пуст
	bool empty = true;
	/**
	 * Выполняем разбор аппаратного адреса на пустоту
	 */
	for(uint8_t i = 0; (i < 6) && empty; i++)
		// Разбираем очередной октет аппаратного адреса
		empty = (awh_cast <awh::net::addr_mac_t *> (source.mac.get())->address[i] == 0);
	// Если аппаратный адрес пуст - проверять нечего
	if(empty){
		// Сообщаем о пропуске проверки с доводом
		GTEST_SKIP() << "Аппаратный адрес устройства выхода наружу выдан пустым";
		// Выходим из проверки
		return;
	}
	// Собираемое описание аппаратного адреса устройства
	std::string mac;
	/**
	 * Собираем описание аппаратного адреса устройства
	 */
	for(uint8_t i = 0; i < 6; i++){
		// Место под очередной октет аппаратного адреса
		char octet[4];
		// Переводим очередной октет аппаратного адреса в запись
		::snprintf(octet, sizeof(octet), "%02X", awh_cast <awh::net::addr_mac_t *> (source.mac.get())->address[i]);
		// Дополняем описание разделителем октетов
		if(i > 0)
			// Дополняем описание разделителем октетов
			mac.append(":");
		// Дополняем описание очередным октетом
		mac.append(octet);
	}
	// Признак того, что устройство числится среди доступных
	bool listed = false;
	// Собираемый перечень доступных устройств машины
	std::string listing;
	/**
	 * Выполняем перебор всех доступных устройств машины
	 */
	for(const std::string & item : this->_eth->iface.available()){
		// Дополняем перечень разделителем
		listing.append(listing.empty() ? "" : ", ").append(item);
		// Если устройство выхода наружу среди доступных найдено
		if(item == source.iface)
			// Отмечаем устройство числящимся среди доступных
			listed = true;
	}
	// Розыск устройства по его аппаратному адресу обязан отвечать именем
	ASSERT_EQ(this->_eth->iface.name(source.mac.get()), source.iface)
	 << "Аппаратный адрес " << mac << " не опознан принадлежащим устройству " << source.iface
	 << "; среди доступных устройство " << (listed ? "числится" : "НЕ ЧИСЛИТСЯ") << ", доступны: " << listing;
}
