/**
 * @file: portmap.cpp
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
 * Подключаем стандартные заголовочные файлы
 */
#include <atomic>
#include <thread>
#include <vector>
#include <cstdlib>
#include <cstring>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Внутренние служебные функции тестов
 *
 */
namespace {
	/**
	 * @brief Функция проверки необходимости запуска сетевых (live) тестов
	 *
	 * @note Сетевые тесты требуют наличия маршрутизатора с поддержкой UPnP/NAT-PMP/PCP
	 *       и включаются установкой переменной окружения AWH_PORTMAP_LIVE
	 *
	 * @return флаг необходимости запуска сетевых тестов
	 */
	bool liveTestsEnabled() noexcept {
		// Получаем значение переменной окружения
		const char * value = ::getenv("AWH_PORTMAP_LIVE");
		// Тесты включены, если переменная установлена и не равна "0"
		return ((value != nullptr) && (value[0] != '\0') && (::strcmp(value, "0") != 0));
	}
	/**
	 * @brief Функция формирования структуры проброса порта
	 *
	 * @param addr       объект работы с сетевым адресом
	 * @param type       тип проброса порта
	 * @param internalIp внутренний IPv4-адрес в строковом виде
	 * @return           заполненная структура проброса порта
	 */
	awh::eth::portmap_t::fwd_t makeForwarding(awh::net_addr_t * addr, const awh::eth::portmap_t::type_t type, const char * internalIp) noexcept {
		// Создаём структуру проброса порта
		awh::eth::portmap_t::fwd_t fwd{};
		// Устанавливаем тип проброса порта
		fwd.type = type;
		// Устанавливаем протокол проброса порта
		fwd.proto = awh::eth::portmap_t::proto_t::TCP;
		// Устанавливаем время жизни проброса порта
		fwd.lifeTime = 3600;
		// Устанавливаем внутренний порт
		fwd.internalPort = 8081;
		// Устанавливаем внешний порт
		fwd.externalPort = 8080;
		// Инициализируем объект внутреннего IPv4-адреса
		fwd.internalAddress = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Выполняем парсинг внутреннего IPv4-адреса
		(* addr) = internalIp;
		// Устанавливаем внутренний IPv4-адрес в пробросе порта
		awh_cast <awh::net::addr_net_ipv4_t *> (fwd.internalAddress.get())->address = addr->v4(awh::net_addr_t::endian_t::LITTLE);
		// Устанавливаем описание проброса порта
		::memcpy(fwd.description, "AWH Web Server", ::strlen("AWH Web Server") + 1);
		// Возвращаем сформированную структуру
		return fwd;
	}
}

/**
 * @brief Тест значений по умолчанию структуры проброса порта
 *
 */
TEST_F(EthFixture, PortMapForwardingDefaults){
	// Создаём структуру проброса порта со значениями по умолчанию
	awh::eth::portmap_t::fwd_t fwd{};
	// Тип проброса порта по умолчанию не определён
	ASSERT_EQ(fwd.type, awh::eth::portmap_t::type_t::NONE);
	// Протокол проброса порта по умолчанию не определён
	ASSERT_EQ(fwd.proto, awh::eth::portmap_t::proto_t::NONE);
	// Время жизни проброса порта по умолчанию нулевое
	ASSERT_EQ(fwd.lifeTime, 0U);
	// Внутренний порт по умолчанию нулевой
	ASSERT_EQ(fwd.internalPort, 0U);
	// Внешний порт по умолчанию нулевой
	ASSERT_EQ(fwd.externalPort, 0U);
	// Описание проброса порта по умолчанию пустое
	ASSERT_EQ(fwd.description[0], '\0');
	// Внутренний IP-адрес по умолчанию не инициализирован
	ASSERT_EQ(fwd.internalAddress, nullptr);
	// Внешний IP-адрес по умолчанию не инициализирован
	ASSERT_EQ(fwd.externalAddress, nullptr);
}
/**
 * @brief Тест получения списка пробросов и проверки инвариантов
 *
 */
TEST_F(EthFixture, PortMapMappingsInvariants){
	// Получаем список проброшенных портов (может быть пустым при отсутствии UPnP-устройств)
	const auto mappings = this->_eth->portmap.mappings();
	/**
	 * Перебираем полученные записи проброса портов
	 */
	for(const auto & fwd : mappings){
		// Внутренний IP-адрес записи всегда должен быть инициализирован
		ASSERT_NE(fwd.internalAddress, nullptr);
		// Внешний IP-адрес записи всегда должен быть инициализирован
		ASSERT_NE(fwd.externalAddress, nullptr);
		// Тип записи, полученной через mappings(), всегда UPnP
		ASSERT_EQ(fwd.type, awh::eth::portmap_t::type_t::UPNP);
		// Протокол записи должен быть TCP или UDP
		ASSERT_TRUE((fwd.proto == awh::eth::portmap_t::proto_t::TCP) || (fwd.proto == awh::eth::portmap_t::proto_t::UDP));
	}
}
/**
 * @brief Тест безопасного завершения проброса для неопределённого типа
 *
 * @note Для типа NONE метод mapping() не выполняет сетевых операций и должен детерминированно вернуть false
 */
TEST_F(EthFixture, PortMapMappingTypeNone){
	// Формируем структуру проброса порта с неопределённым типом
	awh::eth::portmap_t::fwd_t fwd = ::makeForwarding(this->_addr.get(), awh::eth::portmap_t::type_t::NONE, "192.168.7.215");
	// Проброс с неопределённым типом должен завершиться неудачей
	ASSERT_FALSE(this->_eth->portmap.mapping(fwd, awh::event::mode_t::ENABLED));
	// Удаление проброса с неопределённым типом также должно завершиться неудачей
	ASSERT_FALSE(this->_eth->portmap.mapping(fwd, awh::event::mode_t::DISABLED));
}
/**
 * @brief Тест переключения режима потокобезопасности
 *
 * @note Переключение режима не должно приводить к падению и должно быть идемпотентным
 */
TEST_F(EthFixture, PortMapThreadSafetyToggle){
	/**
	 * Многократно переключаем режим потокобезопасности
	 */
	for(uint8_t i = 0; i < 3; ++i){
		// Включаем потокобезопасность
		this->_eth->portmap.threadSafety(true);
		// Выключаем потокобезопасность
		this->_eth->portmap.threadSafety(false);
	}
	// Повторное включение потокобезопасности должно быть безопасным
	this->_eth->portmap.threadSafety(true);
	// Получение списка пробросов при включённой потокобезопасности не должно падать
	const auto mappings = this->_eth->portmap.mappings();
	// Список может быть пустым - это нормально
	ASSERT_GE(mappings.size(), 0U);
	// Возвращаем режим потокобезопасности в исходное состояние
	this->_eth->portmap.threadSafety(false);
}
/**
 * @brief Тест конкурентного доступа к общему кешу при включённой потокобезопасности
 *
 * @note Проверяет отсутствие гонок при одновременном обращении нескольких потоков
 *       к статическому кешу IGD/шлюза (по умолчанию выполняется параллельное обнаружение)
 */
TEST_F(EthFixture, PortMapConcurrentMappings){
	// Включаем потокобезопасность для защиты доступа к общему кешу
	this->_eth->portmap.threadSafety(true);
	// Количество конкурентных потоков
	constexpr uint8_t threadsCount = 4;
	// Счётчик успешно завершённых потоков
	std::atomic <uint32_t> completed{0};
	// Контейнер потоков
	std::vector <std::thread> threads;
	// Резервируем память под потоки
	threads.reserve(threadsCount);
	/**
	 * Запускаем конкурентные потоки
	 */
	for(uint8_t i = 0; i < threadsCount; ++i){
		// Создаём поток, многократно обращающийся к общему кешу
		threads.emplace_back([this, &completed]() noexcept {
			/**
			 * Выполняем несколько итераций обращения к кешу
			 */
			for(uint8_t j = 0; j < 2; ++j)
				// Получаем список пробросов (обращается к статическому кешу IGD)
				(void) this->_eth->portmap.mappings();
			// Увеличиваем счётчик завершённых потоков
			++completed;
		});
	}
	/**
	 * Дожидаемся завершения всех потоков
	 */
	for(auto & thread : threads){
		// Если поток можно присоединить
		if(thread.joinable())
			// Дожидаемся завершения потока
			thread.join();
	}
	// Все потоки должны успешно завершиться без падений и зависаний
	ASSERT_EQ(completed.load(), static_cast <uint32_t> (threadsCount));
	// Возвращаем режим потокобезопасности в исходное состояние
	this->_eth->portmap.threadSafety(false);
}
/**
 * @brief Сетевой тест проброса/удаления порта по протоколу NAT-PMP (best-effort)
 *
 * @note Требует маршрутизатор с поддержкой NAT-PMP. Включается переменной окружения AWH_PORTMAP_LIVE.
 *       Результат проброса зависит от внешней инфраструктуры, поэтому проверяются только инварианты при успехе.
 */
TEST_F(EthFixture, PortMapNatPmpLive){
	// Если сетевые тесты не включены
	if(!::liveTestsEnabled())
		// Пропускаем тест
		GTEST_SKIP() << "Set AWH_PORTMAP_LIVE=1 to run live NAT-PMP port mapping test";
	// Формируем структуру проброса порта по протоколу NAT-PMP
	awh::eth::portmap_t::fwd_t fwd = ::makeForwarding(this->_addr.get(), awh::eth::portmap_t::type_t::NAT_PMP, "192.168.7.215");
	// Выполняем проброс порта на маршрутизаторе
	const bool enabled = this->_eth->portmap.mapping(fwd, awh::event::mode_t::ENABLED);
	// Записываем в лог результат проброса
	std::cout << "NAT-PMP mapping result: " << (enabled ? "success" : "failure") << std::endl;
	// Если проброс выполнен успешно
	if(enabled){
		// Назначенный маршрутизатором внешний порт должен быть ненулевым
		ASSERT_NE(fwd.externalPort, 0U);
		// Если назначенный внешний IP-адрес получен
		if(fwd.externalAddress != nullptr){
			// Назначенный внешний IP-адрес должен быть IPv4
			ASSERT_EQ(fwd.externalAddress->size, 4U);
			// Устанавливаем полученный внешний IPv4-адрес
			this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (fwd.externalAddress.get())->address, awh::net_addr_t::endian_t::LITTLE);
			// Записываем в лог назначенный внешний IP-адрес и порт
			std::cout << "External IP: " << static_cast <std::string> (* this->_addr.get()) << ":" << fwd.externalPort << std::endl;
		}
		// Формируем структуру для удаления проброса порта
		awh::eth::portmap_t::fwd_t remove = ::makeForwarding(this->_addr.get(), awh::eth::portmap_t::type_t::NAT_PMP, "192.168.7.215");
		// Удаляем проброс порта на маршрутизаторе
		ASSERT_TRUE(this->_eth->portmap.mapping(remove, awh::event::mode_t::DISABLED));
	}
}
/**
 * @brief Сетевой тест проброса/удаления порта по протоколу UPnP (best-effort)
 *
 * @note Требует маршрутизатор с поддержкой UPnP IGD. Включается переменной окружения AWH_PORTMAP_LIVE.
 */
TEST_F(EthFixture, PortMapUpnpLive){
	// Если сетевые тесты не включены
	if(!::liveTestsEnabled())
		// Пропускаем тест
		GTEST_SKIP() << "Set AWH_PORTMAP_LIVE=1 to run live UPnP port mapping test";
	// Формируем структуру проброса порта по протоколу UPnP
	awh::eth::portmap_t::fwd_t fwd = ::makeForwarding(this->_addr.get(), awh::eth::portmap_t::type_t::UPNP, "192.168.7.215");
	// Выполняем проброс порта на маршрутизаторе
	const bool enabled = this->_eth->portmap.mapping(fwd, awh::event::mode_t::ENABLED);
	// Записываем в лог результат проброса
	std::cout << "UPnP mapping result: " << (enabled ? "success" : "failure") << std::endl;
	// Если проброс выполнен успешно
	if(enabled){
		// Формируем структуру для удаления проброса порта
		awh::eth::portmap_t::fwd_t remove = ::makeForwarding(this->_addr.get(), awh::eth::portmap_t::type_t::UPNP, "192.168.7.215");
		// Удаляем проброс порта на маршрутизаторе
		ASSERT_TRUE(this->_eth->portmap.mapping(remove, awh::event::mode_t::DISABLED));
	}
}
/**
 * @brief Сетевой тест проброса/удаления порта по протоколу PCP (best-effort)
 *
 * @note Требует маршрутизатор с поддержкой PCP. Включается переменной окружения AWH_PORTMAP_LIVE.
 */
TEST_F(EthFixture, PortMapPcpLive){
	// Если сетевые тесты не включены
	if(!::liveTestsEnabled())
		// Пропускаем тест
		GTEST_SKIP() << "Set AWH_PORTMAP_LIVE=1 to run live PCP port mapping test";
	// Формируем структуру проброса порта по протоколу PCP
	awh::eth::portmap_t::fwd_t fwd = ::makeForwarding(this->_addr.get(), awh::eth::portmap_t::type_t::PCP, "192.168.7.215");
	// Выполняем проброс порта на маршрутизаторе
	const bool enabled = this->_eth->portmap.mapping(fwd, awh::event::mode_t::ENABLED);
	// Записываем в лог результат проброса
	std::cout << "PCP mapping result: " << (enabled ? "success" : "failure") << std::endl;
	// Если проброс выполнен успешно
	if(enabled){
		// Назначенный маршрутизатором внешний порт должен быть ненулевым
		ASSERT_NE(fwd.externalPort, 0U);
		// Если назначенный внешний IP-адрес получен
		if(fwd.externalAddress != nullptr)
			// Назначенный внешний IP-адрес должен быть IPv4 или IPv6
			ASSERT_TRUE((fwd.externalAddress->size == 4U) || (fwd.externalAddress->size == 16U));
		// Формируем структуру для удаления проброса порта
		awh::eth::portmap_t::fwd_t remove = ::makeForwarding(this->_addr.get(), awh::eth::portmap_t::type_t::PCP, "192.168.7.215");
		// Удаляем проброс порта на маршрутизаторе
		ASSERT_TRUE(this->_eth->portmap.mapping(remove, awh::event::mode_t::DISABLED));
	}
}
