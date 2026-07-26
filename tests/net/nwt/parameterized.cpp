/**
 * @file: parameterized.cpp
 * @date: 2025-12-14
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
#include "nwt.hpp"

/**
 * @brief Параметры теста выполнения работы со списком параметров URL
 *
 */
struct NwtTestParameter {
	// Тип URL-адреса
	awh::nwt_t::types_t type = awh::nwt_t::types_t::NONE;
	// Порт URL-адреса
	uint32_t port = 0;
	// Хост URL-адреса
	std::string host = "";
	// Путь URL-адреса
	std::string path = "";
	// Ник пользователя (для электронной почты)
	std::string user = "";
	// Пароль пользователя
	std::string pass = "";
	// Якорь URL-адреса
	std::string anchor = "";
	// Домен верхнего уровня
	std::string domain = "";
	// Параметры URL-адреса
	std::string params = "";
	// Протокол URL-адреса
	std::string schema = "";
	// Пользовательская зона
	std::string zone = "";
	// Список пользовательских зон
	std::unordered_set <std::string> zones;
	// Адрес для проверки
	std::string address = "";
};

/**
 * @brief Класс параметризованной тестовой фикстуры
 *
 */
class NwtTestParameterizedFixture : public NwtFixture, public ::testing::WithParamInterface <NwtTestParameter> {
	public:
		// Параметры теста
		NwtTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения работы со списком параметров URL
 *
 */
TEST_P(NwtTestParameterizedFixture, NwtTestingTest){
	// Выполняем очистку объекта работы со списком параметров URL
	this->_nwt->clear();
	// Устанавливаем логер для объекта работы со списком параметров URL
	this->_nwt->setLogger(this->_log.get());
	// Если пользовательская зона указана, устанавливаем её
	if(!this->_parameter.zone.empty()){
		// Устанавливаем пользовательскую зону для объекта работы со списком параметров URL
		this->_nwt->zone(this->_parameter.zone);
		// Выполняем извлечение списка установленных зон
		auto zones = this->_nwt->zones();
		// Проверяем наличие зоны в списке установленных зон
		ASSERT_TRUE(zones.find(this->_parameter.zone) != zones.end());
	}
	// Если список пользовательских зон указан, устанавливаем его
	if(!this->_parameter.zones.empty()){
		// Устанавливаем список пользовательских зон для объекта работы со списком параметров URL
		this->_nwt->zones(this->_parameter.zones);
		// Выполняем извлечение списка установленных зон
		auto zones = this->_nwt->zones();
		/**
		 * Выполняем перебор всех установленных зон
		 */
		for(const auto & zone : this->_parameter.zones)
			// Проверяем наличие зоны в списке установленных зон
			ASSERT_TRUE(zones.find(zone) != zones.end());
	}
	// Выполняем парсинг адреса
	auto result = this->_nwt->parse(this->_parameter.address);
	// Проверяем тип URL-адреса
	ASSERT_EQ(this->_parameter.type, result.type);
	// Проверяем порт URL-адреса
	ASSERT_EQ(this->_parameter.port, result.port);
	// Проверяем полный URI-параметры
	ASSERT_EQ(this->_parameter.address, result.uri);
	// Проверяем хост URL-адреса
	ASSERT_EQ(this->_parameter.host, result.host);
	// Проверяем путь URL-адреса
	ASSERT_EQ(this->_parameter.path, result.path);
	// Проверяем ник пользователя (для электронной почты)
	ASSERT_EQ(this->_parameter.user, result.user);
	// Проверяем пароль пользователя
	ASSERT_EQ(this->_parameter.pass, result.pass);
	// Проверяем якорь URL-адреса
	ASSERT_EQ(this->_parameter.anchor, result.anchor);
	// Проверяем домен верхнего уровня
	ASSERT_EQ(this->_parameter.domain, result.domain);
	// Проверяем параметры URL-адреса
	ASSERT_EQ(this->_parameter.params, result.params);
	// Проверяем протокол URL-адреса
	ASSERT_EQ(this->_parameter.schema, result.schema);
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, NwtTestParameterizedFixture,
	::testing::Values(
		NwtTestParameter({
			awh::nwt_t::types_t::URL,
			443,
			"anyks.com",
			"/path/to/resource",
			"user",
			"password",
			"anchor",
			"com",
			"query=param",
			"https",
			"test",
			{"awh", "anyks"},
			"https://user:password@anyks.com:443/path/to/resource?query=param#anchor"
		}),
		NwtTestParameter({
			awh::nwt_t::types_t::URL,
			0,
			"anyks.ru",
			"/path/to/resource",
			"",
			"",
			"anchor",
			"ru",
			"query=param&id=154",
			"http",
			"test",
			{"awh", "anyks"},
			"http://anyks.ru/path/to/resource?query=param&id=154#anchor"
		}),
		NwtTestParameter({
			awh::nwt_t::types_t::URL,
			0,
			"anyks.ru",
			"/path/to/resource",
			"",
			"",
			"",
			"ru",
			"",
			"http",
			"test",
			{"awh", "anyks"},
			"http://anyks.ru/path/to/resource"
		}),
		NwtTestParameter({
			awh::nwt_t::types_t::MAC,
			0,
			"73:0b:04:0d:db:79",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			{},
			"73:0b:04:0d:db:79"
		}),
		NwtTestParameter({
			awh::nwt_t::types_t::IPV4,
			0,
			"127.0.0.1",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			{},
			"127.0.0.1"
		}),
		NwtTestParameter({
			awh::nwt_t::types_t::IPV6,
			0,
			"2001:0db8:0000:0000:0000:0000:ae21:ad12",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			{},
			"2001:0db8:0000:0000:0000:0000:ae21:ad12"
		}),
		NwtTestParameter({
			awh::nwt_t::types_t::IPV6,
			0,
			"2001:0db8:0000:0000:0000:0000:ae21:ad12",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			{},
			"2001:0db8:0000:0000:0000:0000:ae21:ad12"
		}),
		NwtTestParameter({
			awh::nwt_t::types_t::EMAIL,
			0,
			"anyks.com",
			"",
			"info",
			"",
			"",
			"com",
			"",
			"",
			"",
			{},
			"info@anyks.com"
		})
	)
);
