/**
 * @file: parameterized.cpp
 * @date: 2025-12-13
 * @license: LicenseRef-AWH-1.0
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
#include "addr.hpp"

/**
 * @brief Структура параметров тестов типов сетевых адресов
 *
 */
struct NetTypeTestParameter {
	// Тестовый хост
	std::string host = "";
	// Ожидаемый результат парсинга
	bool expectedParseResult = false;
	// Ожидаемый тип хоста
	awh::net_addr_t::type_t hostType = awh::net_addr_t::type_t::NONE;
};

/**
 * @brief Класс фикстуры для параметризованных тестов типов сетевых адресов
 *
 */
class NetTypeParameterizedFixture : public NetFixture, public ::testing::WithParamInterface <NetTypeTestParameter> {
	public:
		// Параметр теста
		NetTypeTestParameter _parameter = GetParam();
};

/**
 * @brief Тест определения типа сетевого адреса
 *
 */
TEST_P(NetTypeParameterizedFixture, NetHostTypeTest){
	// Определяем тип хоста
	ASSERT_EQ(this->_parameter.hostType, this->_addr->host(this->_parameter.host));
}

/**
 * @brief Тест парсинга сетевого адреса по заданному типу
 *
 */
TEST_P(NetTypeParameterizedFixture, NetParseByTypeTest){
	// Парсим хост по заданному типу и сравниваем результат с ожидаемым
	ASSERT_EQ(this->_parameter.expectedParseResult, this->_addr->parse(this->_parameter.host, this->_parameter.hostType));
}

/**
 * @brief Инициализация параметров тестов типов сетевых адресов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, NetTypeParameterizedFixture,
	::testing::Values(
		NetTypeTestParameter({
			"02:42:e8:0f:5a:b9",
			true,
			awh::net_addr_t::type_t::MAC
		}),
		NetTypeTestParameter({
			"192.168.0.1",
			true,
			awh::net_addr_t::type_t::IPV4
		}),
		NetTypeTestParameter({
			"255.255.255.255",
			true,
			awh::net_addr_t::type_t::IPV4
		}),
		NetTypeTestParameter({
			"fe80::377b:7b76:9698:44c1",
			true,
			awh::net_addr_t::type_t::IPV6
		}),
		NetTypeTestParameter({
			"192.168.0.0/24",
			false,
			awh::net_addr_t::type_t::NETV4
		}),
		NetTypeTestParameter({
			"192.168.0.0/255.255.254.0",
			false,
			awh::net_addr_t::type_t::NETV4
		}),
		NetTypeTestParameter({
			"192.168.0.1/32",
			false,
			awh::net_addr_t::type_t::NETV4
		}),
		NetTypeTestParameter({
			"192.168.0.1/255.255.255.255",
			false,
			awh::net_addr_t::type_t::NETV4
		}),
		NetTypeTestParameter({
			"192.168.0.0/255.255.254.0",
			false,
			awh::net_addr_t::type_t::NETV4
		}),
		NetTypeTestParameter({
			"pangeoradar.ru",
			false,
			awh::net_addr_t::type_t::FQDN
		}),
		NetTypeTestParameter({
			"https://www.pangeoradar.ru",
			false,
			awh::net_addr_t::type_t::URL
		}),
		NetTypeTestParameter({
			"/tmp/.socket",
			false,
			awh::net_addr_t::type_t::FS
		})
	)
);

/**
 * @brief Структура параметров тестов режимов сетевых адресов
 *
 */
struct NetModeTestParameter {
	// Тестовый адрес
	std::string addr = "";
	// Режим дислокации IP-адреса
	awh::net_addr_t::own_t own = awh::net_addr_t::own_t::NONE;
};

/**
 * @brief Класс фикстуры для параметризованных тестов режимов сетевых адресов
 *
 */
class NetModeParameterizedFixture : public NetFixture, public ::testing::WithParamInterface <NetModeTestParameter> {
	public:
		// Параметр теста
		NetModeTestParameter _parameter = GetParam();
};

/**
 * @brief Тест режима дислокации сетевого адреса
 *
 */
TEST_P(NetModeParameterizedFixture, NetAddrModeTest){
	// Парсим адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Проверяем режим дислокации адреса
	ASSERT_EQ(this->_parameter.own, this->_addr->own());
}

/**
 * @brief Инициализация параметров тестов режимов сетевых адресов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, NetModeParameterizedFixture,
	::testing::Values(
		NetModeTestParameter({
			"0.0.0.0",
			awh::net_addr_t::own_t::SYS
		}),
		NetModeTestParameter({
			"192.168.31.12",
			awh::net_addr_t::own_t::LAN
		}),
		NetModeTestParameter({
			"10.0.0.1",
			awh::net_addr_t::own_t::LAN
		}),
		NetModeTestParameter({
			"172.16.0.1",
			awh::net_addr_t::own_t::LAN
		}),
		NetModeTestParameter({
			"46.39.230.51",
			awh::net_addr_t::own_t::WAN
		}),
		NetModeTestParameter({
			"::0",
			awh::net_addr_t::own_t::SYS
		}),
		NetModeTestParameter({
			"::1",
			awh::net_addr_t::own_t::LAN
		}),
		NetModeTestParameter({
			"fe80::a2",
			awh::net_addr_t::own_t::LAN
		}),
		NetModeTestParameter({
			"2a01:230:5:7:1:9a2:0:1",
			awh::net_addr_t::own_t::WAN
		})
	)
);

/**
 * @brief Структура параметров тестов префиксов сетевых адресов
 *
 */
struct RangeParam {
	std::string begin = "";
	std::string end = "";
};

/**
 * @brief Класс фикстуры для параметризованных тестов префиксов сетевых адресов
 *
 */
struct NetPrefixTestParameter {
	// Тестовый адрес
	std::string addr = "";
	// Тестовый длинный адрес
	std::string addrLong = "";
	// Тестовая маска
	std::string mask = "";
	// Тестовый префикс
	uint8_t prefix = 0;
	// Ожидаемый результат наложения сетевого префикса
	RangeParam range;
	// Ожидаемый результат наложения сетевого префикса
	std::string expectedImposeNetworkResult = "";
	// Ожидаемый результат наложения хостового префикса
	std::string expectedImposeHostResult = "";
};

/**
 * @brief Класс фикстуры для параметризованных тестов префиксов сетевых адресов
 *
 */
class NetPrefixParameterizedFixture : public NetFixture, public ::testing::WithParamInterface <NetPrefixTestParameter> {
	public:
		// Параметр теста
		NetPrefixTestParameter _parameter = GetParam();
};

/**
 * @brief Тест преобразования маски в префикс сетевого адреса
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetMask2PrefixTest){
	/**
	 * Временное решение так как нельзя вызвать mask2Prefix отдельно,
	 * mask2Prefix требует наличе проинициализированного поля _type
	 * Сейчас нет способа установить его или автоматически определить внутри mask2Prefix
	 */
	ASSERT_TRUE(this->_addr->parse(this->_parameter.mask));
	// Проверяем преобразование маски в префикс
	ASSERT_EQ(this->_parameter.prefix, this->_addr->mask2Prefix(this->_parameter.mask));
}

/**
 * @brief Тест преобразования префикса в маску сетевого адреса
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetPrefix2MaskTest){
	/**
	 * Временное решение так как нельзя вызвать prefix2Mask отдельно,
	 * prefix2Mask требует наличе проинициализированного поля _type
	 * Сейчас нет способа установить его или автоматически определить внутри prefix2Mask
	 */
	ASSERT_TRUE(this->_addr->parse(this->_parameter.mask));
	// Проверяем преобразование префикса в маску
	ASSERT_EQ(this->_parameter.mask, this->_addr->prefix2Mask(this->_parameter.prefix));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetImposeNetworkPrefixTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем префикс
	this->_addr->impose(this->_parameter.prefix, awh::net_addr_t::addr_t::NETWORK);
	// Сравниваем результат с ожидаемым
	ASSERT_EQ(this->_parameter.expectedImposeNetworkResult, this->_addr->print());
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetImposeNetworkMaskTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем маску
	this->_addr->impose(this->_parameter.mask, awh::net_addr_t::addr_t::NETWORK);
	// Сравниваем результат с ожидаемым
	ASSERT_EQ(this->_parameter.expectedImposeNetworkResult, this->_addr->print());
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetImposeHostPrefixTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем префикс
	this->_addr->impose(this->_parameter.prefix, awh::net_addr_t::addr_t::HOST);
	// Сравниваем результат с ожидаемым
	ASSERT_EQ(this->_parameter.expectedImposeHostResult, this->_addr->print());
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetImposeHostMaskTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем маску
	this->_addr->impose(this->_parameter.mask, awh::net_addr_t::addr_t::HOST);
	// Сравниваем результат с ожидаемым
	ASSERT_EQ(this->_parameter.expectedImposeHostResult, this->_addr->print());
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessMappingTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем маску и проверяем результат
	ASSERT_TRUE(this->_addr->mapping(this->_parameter.expectedImposeNetworkResult));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessMappingNetworkMaskTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем маску и проверяем результат
	ASSERT_TRUE(this->_addr->mapping(this->_parameter.expectedImposeNetworkResult, this->_parameter.mask, awh::net_addr_t::addr_t::NETWORK));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessMappingNetworkPrefixTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем префикс и проверяем результат
	ASSERT_TRUE(this->_addr->mapping(this->_parameter.expectedImposeNetworkResult, this->_parameter.prefix, awh::net_addr_t::addr_t::NETWORK));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetFailedMappingNetworkMaskTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем маску и проверяем результат
	ASSERT_FALSE(this->_addr->mapping(this->_parameter.expectedImposeHostResult, this->_parameter.mask, awh::net_addr_t::addr_t::NETWORK));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetFailedMappingNetworkPrefixTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем префикс и проверяем результат
	ASSERT_FALSE(this->_addr->mapping(this->_parameter.expectedImposeHostResult, this->_parameter.prefix, awh::net_addr_t::addr_t::NETWORK));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessMappingHostMaskTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем маску и проверяем результат
	ASSERT_TRUE(this->_addr->mapping(this->_parameter.expectedImposeHostResult, this->_parameter.mask, awh::net_addr_t::addr_t::HOST));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessMappingHostPrefixTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем префикс и проверяем результат
	ASSERT_TRUE(this->_addr->mapping(this->_parameter.expectedImposeHostResult, this->_parameter.prefix, awh::net_addr_t::addr_t::HOST));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetFailedMappingHostMaskTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем маску и проверяем результат
	ASSERT_TRUE(this->_addr->mapping(this->_parameter.expectedImposeHostResult, this->_parameter.mask, awh::net_addr_t::addr_t::HOST));
}

/**
 * @brief Тесты наложения префиксов и масок на сетевой адрес с возвратом результата
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetFailedMappingHostPrefixTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Накладываем префикс и проверяем результат
	ASSERT_TRUE(this->_addr->mapping(this->_parameter.expectedImposeHostResult, this->_parameter.prefix, awh::net_addr_t::addr_t::HOST));
}

/**
 * @brief Тесты проверки принадлежности адреса к заданному диапазону
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessRangePrefixTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Проверяем принадлежность к диапазону с префиксом
	ASSERT_TRUE(this->_addr->range(this->_parameter.range.begin, this->_parameter.range.end, this->_parameter.prefix));
}

/**
 * @brief Тесты проверки принадлежности адреса к заданному диапазону
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessRangeMaskTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Проверяем принадлежность к диапазону с маской
	ASSERT_TRUE(this->_addr->range(this->_parameter.range.begin, this->_parameter.range.end, this->_parameter.mask));
}

/**
 * @brief Тесты проверки принадлежности адреса к заданному диапазону
 *
 */
TEST_P(NetPrefixParameterizedFixture, NetSuccessGetLongTest){
	// Парсим тестовый адрес
	ASSERT_TRUE(this->_addr->parse(this->_parameter.addr));
	// Проверяем получение длинного формата адреса
	ASSERT_EQ(this->_parameter.addrLong, this->_addr->print(awh::net_addr_t::format_size_t::LONG));
}

/**
 * @brief Инициализация параметров тестов префиксов сетевых адресов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, NetPrefixParameterizedFixture,
	::testing::Values(
		NetPrefixTestParameter({
			"192.168.0.1",
			"192.168.000.001",
			"255.255.255.255",
			32,
			RangeParam({"192.168.0.1", "192.168.0.1"}),
			"192.168.0.1",
			"0.0.0.0"
		}),
		NetPrefixTestParameter({
			"192.168.1.1",
			"192.168.001.001",
			"255.255.255.0",
			24,
			RangeParam({"192.168.1.0", "192.168.1.100"}),
			"192.168.1.0",
			"0.0.0.1"
		}),
		NetPrefixTestParameter({
			"192.168.1.10",
			"192.168.001.010",
			"255.255.254.0",
			23,
			RangeParam({"192.168.0.0", "192.168.1.100"}),
			"192.168.0.0",
			"0.0.1.10"
		}),
		NetPrefixTestParameter({
			"192.168.3.192",
			"192.168.003.192",
			"255.255.0.0",
			16,
			RangeParam({"192.148.0.0", "192.168.3.200"}),
			"192.168.0.0",
			"0.0.3.192"
		}),
		NetPrefixTestParameter({
			"192.168.0.2",
			"192.168.000.002",
			"255.0.0.0",
			8,
			RangeParam({"192.168.0.1", "192.168.1.0"}),
			"192.0.0.0",
			"0.168.0.2"
		}),
		NetPrefixTestParameter({
			"2001:1234:abcd:5678:9877:3322:5541:aabb",
			"2001:1234:ABCD:5678:9877:3322:5541:AABB",
			"FFFF:FFFF:FFFF:FEFF::",
			63,
			RangeParam({"2001:1234:abcd:5678:9877:3322:5541:1111", "2001:1234:abcd:5678:9877:3322:5541:ffff"}),
			"2001:1234:ABCD:5678::",
			"::9877:3322:5541:AABB"
		})
	)
);

/**
 * @brief Структура параметров тестов сравнения сетевых адресов
 *
 */
struct NetCompareTestParameter {
	// Первый тестовый хост
	std::string host1 = "";
	// Второй тестовый хост
	std::string host2 = "";
	// Ожидаемый результат сравнения
	bool expectedResult = false;
};

/**
 * @brief Класс фикстуры для параметризованных тестов сравнения сетевых адресов
 *
 */
class NetCompareParameterizedFixture : public NetFixture, public ::testing::WithParamInterface <NetCompareTestParameter> {
	public:
		// Параметр теста
		NetCompareTestParameter _parameter = GetParam();
};

/**
 * @brief Тест сравнения двух IPv4 сетевых адресов
 *
 */
TEST_P(NetCompareParameterizedFixture, NetCompareIPv4Test){
	// Создаём первый объект сетевого адреса
	awh::net_addr_t addr1(this->_fmk.get(), this->_log.get());
	// Создаём второй объект сетевого адреса
	awh::net_addr_t addr2(this->_fmk.get(), this->_log.get());
	// Выполняем парсинг первого хоста
	ASSERT_TRUE(addr1.parse(this->_parameter.host1));
	// Выполняем парсинг второго хоста
	ASSERT_TRUE(addr2.parse(this->_parameter.host2));
	// Сравниваем результат с ожидаемым
	ASSERT_EQ(this->_parameter.expectedResult, addr1 < addr2);
}

/**
 * @brief Инициализация параметров тестов сравнения сетевых адресов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, NetCompareParameterizedFixture,
	::testing::Values(
		NetCompareTestParameter({
			"192.168.0.1",
			"192.168.0.2",
			true
		}),
		NetCompareTestParameter({
			"192.168.0.1",
			"193.168.0.1",
			true
		}),
		NetCompareTestParameter({
			"192.168.0.1",
			"192.168.1.0",
			true
		}),
		NetCompareTestParameter({
			"169.192.0.1",
			"192.168.0.1",
			true
		}),
		NetCompareTestParameter({
			"192.168.0.2",
			"192.168.0.1",
			false
		}),
		NetCompareTestParameter({
			"193.168.0.1",
			"192.168.0.1",
			false
		}),
		NetCompareTestParameter({
			"192.168.1.0",
			"192.168.0.1",
			false
		}),
		NetCompareTestParameter({
			"192.168.0.1",
			"169.192.0.1",
			false
		})
	)
);

/**
 * @brief Структура параметров тестов обратного преобразования сетевых адресов
 *
 */
struct NetArpaTestParameter {
	std::string host = "";
	std::string arpa = "";
};

/**
 * @brief Класс фикстуры для параметризованных тестов обратного преобразования сетевых адресов
 *
 */
class NetArpaParameterizedFixture : public NetFixture, public ::testing::WithParamInterface <NetArpaTestParameter> {
	public:
		// Параметр теста
		NetArpaTestParameter _parameter = GetParam();
};

/**
 * @brief Тест обратного преобразования сетевого адреса в ARPA формат
 *
 */
TEST_P(NetArpaParameterizedFixture, NetArpaTest){
	// Парсим тестовый хост
	ASSERT_TRUE(this->_addr->parse(this->_parameter.host));
	// Проверяем обратное преобразование в ARPA формат
	ASSERT_EQ(this->_parameter.arpa, this->_addr->arpa());
}

/**
 * @brief Тест парсинга сетевого адреса из ARPA формата
 *
 */
TEST_P(NetArpaParameterizedFixture, NetParseArpaTest){
	// Парсим тестовый ARPA хост
	ASSERT_TRUE(this->_addr->arpa(this->_parameter.arpa));
	// Проверяем результат парсинга
	ASSERT_EQ(this->_parameter.host, this->_addr->print());
}

/**
 * @brief Инициализация параметров тестов обратного преобразования сетевых адресов
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, NetArpaParameterizedFixture,
	::testing::Values(
		NetArpaTestParameter({
			"192.168.0.1",
			"1.0.168.192.in-addr.arpa"
		}),
		NetArpaTestParameter({
			"5.255.255.70",
			"70.255.255.5.in-addr.arpa"
		}),
		NetArpaTestParameter({
			"2001:DB8::567:89AB",
			"b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa"
		})
	)
);
