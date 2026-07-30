/**
 * @file: parameterized.cpp
 * @date: 2026-03-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты модуля работы с универсальными идентификаторами ресурсов —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой разбора и сборки URI,
 *        нормализации пути, процентного кодирования и работы с параметрами запроса
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "uri.hpp"
#include <net/uri.hpp>

/**
 * @brief Параметры теста генерации ETag для URI
 *
 */
struct UriTestETagParameter {
	// Размер ETag
	uint8_t size = 0;
	// Входная строка для генерации ETag
	std::string input = "";
	// Ожидаемый результат генерации ETag
	std::string result = "";
};

/**
 * @brief Класс параметризованной тестовой фикстуры для тестирования генерации ETag для URI
 *
 */
class UriTestParameterizedFixture : public UriFixture, public ::testing::WithParamInterface <UriTestETagParameter> {
	public:
		// Параметры теста
		UriTestETagParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения работы с генерацией ETag для URI
 *
 */
TEST_P(UriTestParameterizedFixture, UriETagTest){
	// Выполняем генерацию ETag для входной строки
	auto etag = this->_uri->etag(this->_parameter.input, this->_parameter.size);
	// Проверяем результат генерации ETag
	ASSERT_EQ(this->_parameter.result, etag);
}

/**
 * @brief Инициализация параметров теста работы с генерацией ETag для URI
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, UriTestParameterizedFixture,
	::testing::Values(
		UriTestETagParameter({
			8,
			"anyks.com",
			"\"27248efc\""
		}),
		UriTestETagParameter({
			16,
			"anyks.ru",
			"\"d05f16eec02303ee\""
		}),
		UriTestETagParameter({
			32,
			"anyks.net",
			"\"cbb1a5b03fd0323a\""
		})
	)
);

/**
 * @brief Параметры теста выполнения работы с URI
 *
 */
struct UriTestParsingParameter {
	// Порт URI для проверки
	uint16_t port = 0;
	// Хост URI для проверки
	std::string host = "";
	// Имя пользователя URI
	std::string user = "";
	// Пароль пользователя
	std::string password = "";
	// Схема URI для проверки
	std::string scheme = "";
	// Якорь URI для проверки
	std::string fragment = "";
	// Входная строка для генерации URI
	std::string input = "";
	// Ожидаемый результат генерации URI
	std::string result = "";
	// Тип извлекаемого URI
	awh::uri_t::type_t type = awh::uri_t::type_t::NONE;
	// Режим элемента URI для генерации
	awh::uri_t::item_t item = awh::uri_t::item_t::NONE;
	// Режим формата URI для извлечения
	awh::uri_t::format_t format = awh::uri_t::format_t::NONE;
	// Путь URI для проверки
	std::vector <std::string> path;
	// Параметры URI для проверки
	std::unordered_multimap <std::string, std::string> query;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для тестирования работы с URI
 *
 */
class UriTestParsingParameterizedFixture : public UriFixture, public ::testing::WithParamInterface <UriTestParsingParameter> {
	public:
		// Параметры теста
		UriTestParsingParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения работы парсинга URI
 *
 */
TEST_P(UriTestParsingParameterizedFixture, UriParsingTest){
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Устанавливаем функцию обратного вызова для генерации параметра URI (например, для генерации контрольной суммы)
	this->_uri->callback([this](const awh::uri_t * uri) -> std::string {
		// Если параметры URI не пустые, то генерируем контрольную сумму для строки URI и возвращаем её в виде параметра "checksum"
		if(!this->_parameter.query.empty())
			// Генерируем контрольную сумму для строки URI и возвращаем её в виде параметра "checksum"
			return this->_fmk->format("%s=%s", "checksum", uri->etag(uri->print(awh::uri_t::item_t::QUERY)).c_str());
		// Иначе возвращаем пустую строку
		return "";
	});
	// Выполняем проверку, что контейнер URI пустой
	ASSERT_TRUE(this->_uri->empty());
	// Выполняем парсинг входной строки URI
	ASSERT_EQ(this->_parameter.type, this->_uri->parse(this->_parameter.input));
	// Выполняем проверку, что контейнер URI уже не пустой
	ASSERT_FALSE(this->_uri->empty());
	// Проверяем результат генерации URI
	ASSERT_EQ(this->_parameter.result, this->_uri->print(this->_parameter.item, this->_parameter.format));
	// Проверяем хост URI
	ASSERT_EQ(this->_parameter.host, this->_uri->host());
	// Проверяем порт URI
	ASSERT_EQ(this->_parameter.port, this->_uri->port());
	// Проверяем тип извлекаемого URI
	ASSERT_EQ(this->_parameter.type, this->_uri->type());
	// Проверяем схему URI
	ASSERT_EQ(this->_parameter.scheme, this->_uri->scheme());
	// Извлекаем параметры пользователя URI
	const auto & user = this->_uri->user();
	// Проверяем имя пользователя URI
	ASSERT_EQ(this->_parameter.user, user.username);
	// Проверяем пароль пользователя URI
	ASSERT_EQ(this->_parameter.password, user.password);
	// Проверяем якорь URI
	ASSERT_EQ(this->_parameter.fragment, this->_uri->fragment());
	// Проверяем путь URI
	ASSERT_EQ(this->_parameter.path, this->_uri->path());
	// Проверяем параметры URI
	ASSERT_EQ(this->_parameter.query, this->_uri->query());
}

/**
 * @brief Инициализация параметров теста работы с парсингом URI
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, UriTestParsingParameterizedFixture,
	::testing::Values(
		UriTestParsingParameter({
			0,
			"",
			"",
			"",
			"",
			"",
			"/relative/path?query=1",
			"/relative/path?query=1&checksum=\"c83823d23a048591\"",
			awh::uri_t::type_t::NONE,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"relative", "path"},
			{{"query", "1"}}
		}),
		UriTestParsingParameter({
			80,
			"www.example.com",
			"",
			"",
			"http",
			"frag",
			"http://www.example.com:80/path/to/resource?query=1&id=123#frag",
			"http://www.example.com/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"www.example.com",
			"",
			"",
			"http",
			"frag",
			"http://www.example.com/path/to/resource?query=1&id=123#frag",
			"http://www.example.com:80/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"www.example.com",
			"",
			"",
			"http",
			"frag",
			"http://www.example.com?query=1&id=123#frag",
			"http://www.example.com:80/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"localhost",
			"",
			"",
			"http",
			"frag",
			"http://localhost/path/to/resource?query=1&id=123#frag",
			"http://localhost/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"localhost",
			"",
			"",
			"http",
			"frag",
			"http://localhost:80/path/to/resource?query=1&id=123#frag",
			"http://localhost/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"192.168.0.1",
			"",
			"",
			"http",
			"frag",
			"http://192.168.0.1/path/to/resource?query=1&id=123#frag",
			"http://192.168.0.1:80/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"192.168.0.1",
			"",
			"",
			"http",
			"frag",
			"http://192.168.0.1:80/path/to/resource?query=1&id=123#frag",
			"http://192.168.0.1/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"2001:DB8::1",
			"",
			"",
			"http",
			"frag",
			"http://[2001:db8::1]:80/path/to/resource?query=1&id=123#frag",
			"http://[2001:DB8::1]/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"2001:DB8::1",
			"",
			"",
			"http",
			"frag",
			"http://[2001:db8::1]/path/to/resource?query=1&id=123#frag",
			"http://[2001:DB8::1]:80/path/to/resource?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"path", "to", "resource"},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			8080,
			"www.example.com",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@www.example.com:8080/api/v1",
			"https://user:pass@www.example.com:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"www.example.com",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@www.example.com/api/v1",
			"https://user:pass@www.example.com:443/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			8080,
			"www.example.com",
			"user",
			"",
			"https",
			"",
			"https://user@www.example.com:8080/api/v1",
			"https://user@www.example.com:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"www.example.com",
			"user",
			"",
			"https",
			"frag",
			"https://user@www.example.com/api/v1/?query=1&id=123#frag",
			"https://user@www.example.com:443/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"www.example.com",
			"",
			"",
			"http",
			"frag",
			"www.example.com:80/api/v1/?query=1&id=123#frag",
			"http://www.example.com:80/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"www.example.com",
			"",
			"",
			"https",
			"frag",
			"www.example.com:443/api/v1/?query=1&id=123#frag",
			"https://www.example.com/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			8080,
			"localhost",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@localhost:8080/api/v1",
			"https://user:pass@localhost:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"localhost",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@localhost/api/v1",
			"https://user:pass@localhost/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			8080,
			"localhost",
			"user",
			"",
			"https",
			"",
			"https://user@localhost:8080/api/v1",
			"https://user@localhost:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"localhost",
			"user",
			"",
			"https",
			"frag",
			"https://user@localhost/api/v1/?query=1&id=123#frag",
			"https://user@localhost/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			8080,
			"192.168.0.1",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@192.168.0.1:8080/api/v1",
			"https://user:pass@192.168.0.1:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"192.168.0.1",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@192.168.0.1/api/v1",
			"https://user:pass@192.168.0.1:443/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			8080,
			"192.168.0.1",
			"user",
			"",
			"https",
			"",
			"https://user@192.168.0.1:8080/api/v1",
			"https://user@192.168.0.1:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::FULL,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"192.168.0.1",
			"user",
			"",
			"https",
			"frag",
			"https://user@192.168.0.1/api/v1/?query=1&id=123#frag",
			"https://user@192.168.0.1/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"192.168.0.1",
			"",
			"",
			"http",
			"frag",
			"192.168.0.1:80/api/v1/?query=1&id=123#frag",
			"http://192.168.0.1/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"192.168.0.1",
			"",
			"",
			"https",
			"frag",
			"192.168.0.1:443/api/v1/?query=1&id=123#frag",
			"https://192.168.0.1/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			8080,
			"2001:DB8::1",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@[2001:db8::1]:8080/api/v1",
			"https://user:pass@[2001:DB8::1]:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@[2001:db8::1]/api/v1",
			"https://user:pass@[2001:DB8::1]/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			8080,
			"2001:DB8::1",
			"user",
			"",
			"https",
			"",
			"https://user@[2001:db8::1]:8080/api/v1",
			"https://user@[2001:DB8::1]:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"",
			"https",
			"frag",
			"https://user@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"https://user@[2001:DB8::1]/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			80,
			"2001:DB8::1",
			"",
			"",
			"http",
			"frag",
			"[2001:db8::1]:80/api/v1/?query=1&id=123#frag",
			"http://[2001:DB8::1]/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"",
			"",
			"https",
			"frag",
			"[2001:db8::1]:443/api/v1/?query=1&id=123#frag",
			"https://[2001:DB8::1]/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			8080,
			"2001:DB8::1",
			"user",
			"pass",
			"https",
			"",
			"https://user:pass@[2001:db8::1]:8080/api/v1",
			"https://user:pass@[2001:DB8::1]:8080/api/v1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"api", "v1"},
			{}
		}),
		UriTestParsingParameter({
			25,
			"example.com",
			"user",
			"",
			"mailto",
			"",
			"mailto:user@example.com",
			"mailto:user@example.com",
			awh::uri_t::type_t::EMAIL,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			25,
			"example.com",
			"user",
			"",
			"mailto",
			"",
			"user@example.com:25",
			"mailto:user@example.com",
			awh::uri_t::type_t::EMAIL,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			25,
			"example.com",
			"user",
			"password",
			"mailto",
			"",
			"mailto:user:password@example.com",
			"mailto:user:password@example.com",
			awh::uri_t::type_t::EMAIL,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			0,
			"example.com",
			"user",
			"",
			"",
			"",
			"user@example.com",
			"//user@example.com",
			awh::uri_t::type_t::NONE,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			21,
			"ftp.example.com",
			"",
			"",
			"ftp",
			"",
			"ftp://ftp.example.com/file.txt",
			"ftp://ftp.example.com/file.txt",
			awh::uri_t::type_t::FTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"file.txt"},
			{}
		}),
		UriTestParsingParameter({
			80,
			"example.com",
			"",
			"",
			"http",
			"",
			"http://example.com:80",
			"http://example.com",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			80,
			"example.com",
			"",
			"",
			"http",
			"",
			"http://example.com",
			"http://example.com",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			80,
			"example.com",
			"",
			"",
			"http",
			"",
			"http://example.com/",
			"http://example.com/",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{""},
			{}
		}),
		UriTestParsingParameter({
			80,
			"example.com",
			"",
			"",
			"http",
			"",
			"http://example.com:80/",
			"http://example.com/",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{""},
			{}
		}),
		UriTestParsingParameter({
			80,
			"example.com",
			"",
			"",
			"http",
			"",
			"http://example.com/?query=1",
			"http://example.com/?query=1&checksum=\"c83823d23a048591\"",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{""},
			{{"query", "1"}}
		}),
		UriTestParsingParameter({
			80,
			"example.com",
			"",
			"",
			"http",
			"",
			"http://example.com:80/?query=1",
			"http://example.com/?query=1&checksum=\"c83823d23a048591\"",
			awh::uri_t::type_t::HTTP,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{""},
			{{"query", "1"}}
		}),
		UriTestParsingParameter({
			0,
			"",
			"",
			"",
			"scheme",
			"",
			"scheme:path-only",
			"scheme:path-only",
			awh::uri_t::type_t::SCHEME,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path-only"},
			{}
		}),
		UriTestParsingParameter({
			0,
			"",
			"",
			"",
			"unix",
			"",
			"unix:///var/run/socket.sock",
			"unix:///var/run/socket.sock",
			awh::uri_t::type_t::UDS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"var", "run", "socket.sock"},
			{}
		}),
		UriTestParsingParameter({
			0,
			"",
			"",
			"",
			"file",
			"",
			"file:///path/to/file.txt",
			"file:///path/to/file.txt",
			awh::uri_t::type_t::FILE,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path", "to", "file.txt"},
			{}
		}),
		UriTestParsingParameter({
			0,
			"c",
			"",
			"",
			"file",
			"",
			"file://c:/path/to/file.txt",
			"file://c/path/to/file.txt",
			awh::uri_t::type_t::FILE,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"path", "to", "file.txt"},
			{}
		}),
		UriTestParsingParameter({
			8000,
			"127.0.0.1",
			"rfbPbd",
			"XcCuZH",
			"socks5",
			"",
			"socks5://rfbPbd:XcCuZH@127.0.0.1:8000",
			"socks5://rfbPbd:XcCuZH@127.0.0.1:8000",
			awh::uri_t::type_t::SOCKS5,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			9443,
			"stream.testnet.binance.vision",
			"",
			"",
			"wss",
			"",
			"wss://stream.testnet.binance.vision:9443",
			"wss://stream.testnet.binance.vision:9443",
			awh::uri_t::type_t::WSS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{},
			{}
		}),
		UriTestParsingParameter({
			443,
			"www.example.com",
			"",
			"",
			"https",
			"перейти в низ",
			"https://www.example.com/%D0%B3%D1%80%D0%B8%D0%B3%D0%BE%D1%80%D0%B8%D0%B9/%D0%BB%D0%B8%D1%87%D0%BD%D1%8B%D0%B9%20%D0%BA%D0%B0%D0%B1%D0%B8%D0%BD%D0%B5%D1%82/%D0%B1%D0%B0%D0%BB%D0%B0%D0%BD%D1%81?%D0%B7%D0%B0%D0%BF%D1%80%D0%BE%D1%81=%D0%BF%D1%8F%D1%82%D1%8C&%D0%B8%D0%B4%D0%B5%D0%BD%D1%82%D0%B8%D1%84%D0%B8%D0%BA%D0%B0%D1%82%D0%BE%D1%80=%D0%B3%D0%BE%D0%B3%D0%B0#%D0%BF%D0%B5%D1%80%D0%B5%D0%B9%D1%82%D0%B8%20%D0%B2%20%D0%BD%D0%B8%D0%B7",
			"https://www.example.com/%D0%B3%D1%80%D0%B8%D0%B3%D0%BE%D1%80%D0%B8%D0%B9/%D0%BB%D0%B8%D1%87%D0%BD%D1%8B%D0%B9%20%D0%BA%D0%B0%D0%B1%D0%B8%D0%BD%D0%B5%D1%82/%D0%B1%D0%B0%D0%BB%D0%B0%D0%BD%D1%81?%D0%B7%D0%B0%D0%BF%D1%80%D0%BE%D1%81=%D0%BF%D1%8F%D1%82%D1%8C&%D0%B8%D0%B4%D0%B5%D0%BD%D1%82%D0%B8%D1%84%D0%B8%D0%BA%D0%B0%D1%82%D0%BE%D1%80=%D0%B3%D0%BE%D0%B3%D0%B0&checksum=\"781c3e0db24eb0c6\"#%D0%BF%D0%B5%D1%80%D0%B5%D0%B9%D1%82%D0%B8%20%D0%B2%20%D0%BD%D0%B8%D0%B7",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::URI,
			awh::uri_t::format_t::SMART,
			{"григорий","личный кабинет", "баланс"},
			{{"идентификатор", "гога"}, {"запрос", "пять"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"",
			"https",
			"frag",
			"https://user@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"https://",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::SCHEME,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"user:password",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::USER,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"[2001:DB8::1]",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::HOST,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"/api/v1/",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::PATH,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"id=123&query=1",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::QUERY,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::FRAGMENT,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"https://[2001:DB8::1]",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::ORIGIN,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"https://[2001:DB8::1]:443",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::ORIGIN,
			awh::uri_t::format_t::FULL,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		}),
		UriTestParsingParameter({
			443,
			"2001:DB8::1",
			"user",
			"password",
			"https",
			"frag",
			"https://user:password@[2001:db8::1]/api/v1/?query=1&id=123#frag",
			"/api/v1/?id=123&query=1&checksum=\"6f0947b4f44cdfe9\"#frag",
			awh::uri_t::type_t::HTTPS,
			awh::uri_t::item_t::REQUEST,
			awh::uri_t::format_t::SMART,
			{"api", "v1", ""},
			{{"query", "1"}, {"id", "123"}}
		})
	)
);

/**
 * @brief Параметры теста выполнения сравнения URI
 *
 */
struct UriTestMatchParameter {
	// Порт URI для проверки
	uint16_t port = 0;
	// IPv4-адрес URI для проверки
	uint32_t ipv4 = 0;
	// IPv6-адрес URI для проверки
	std::string ipv6 = "";
	// Домен URI для проверки
	std::string domain = "";
	// Входная строка для генерации URI
	std::string input = "";
	// Тип извлекаемого URI
	awh::uri_t::type_t type = awh::uri_t::type_t::NONE;
	// Адрес запроса для проверки
	std::vector <std::string> request;
	// Ожидаемый результат генерации URI
	std::vector <std::string> result;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для тестирования сравнения URI
 *
 */
class UriTestMatchParameterizedFixture : public UriFixture, public ::testing::WithParamInterface <UriTestMatchParameter> {
	public:
		// Параметры теста
		UriTestMatchParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения работы парсинга URI
 *
 */
TEST_P(UriTestMatchParameterizedFixture, UriMatchTest){
	// Выполняем очистку объекта работы с URI
	this->_uri->clear();
	// Выполняем проверку, что контейнер URI пустой
	ASSERT_TRUE(this->_uri->empty());
	// Выполняем парсинг входной строки URI
	ASSERT_EQ(this->_parameter.type, this->_uri->parse(this->_parameter.input));
	// Выполняем проверку, что контейнер URI уже не пустой
	ASSERT_FALSE(this->_uri->empty());
	// Получаем объект атрибутов URI
	const auto * attr = this->_uri->attr();
	// Если атрибуты URI получены успешно, выполняем проверку их значений
	if(attr != nullptr){
		/**
		 * Определяем тип атрибутов URI адреса
		 */
		switch(static_cast <uint8_t> (attr->type)){
			// Если атрибуты URI адреса являются адресом файловой системы
			case static_cast <uint8_t> (awh::net::type_t::FS):
				// Выполняем проверку, что unix-сокет в атрибутах URI адреса совпадает с ожидаемым адресом в параметрах теста
				ASSERT_EQ(this->_parameter.domain, awh_cast <const awh::net::addr_fs_t *> (awh_cast <const awh::net::attr_uds_t *> (attr)->path.get())->address);
			break;
			// Если атрибуты URI адреса являются FQDN-адресом
			case static_cast <uint8_t> (awh::net::type_t::FQDN): {
				// Выполняем проверку, что порт хоста в атрибутах URI адреса совпадает с ожидаемым портом в параметрах теста
				ASSERT_EQ(this->_parameter.port, awh_cast <const awh::net::attr_fqdn_t *> (attr)->port);
				// Выполняем проверку, что доменное имя хоста в атрибутах URI адреса совпадает с ожидаемым доменным именем в параметрах теста
				ASSERT_EQ(this->_parameter.domain, awh_cast <const awh::net::attr_fqdn_t *> (attr)->domain);
			} break;
			// Если атрибуты URI адреса являются IPv4-адресом
			case static_cast <uint8_t> (awh::net::type_t::IPV4): {
				// Выполняем проверку, что порт хоста в атрибутах URI адреса совпадает с ожидаемым портом в параметрах теста
				ASSERT_EQ(this->_parameter.port, awh_cast <const awh::net::attr_net_t *> (attr)->port);
				// Выполняем проверку, что IPv4-адрес хоста в атрибутах URI адреса совпадает с ожидаемым IPv4-адресом в параметрах теста
				ASSERT_EQ(this->_parameter.ipv4, awh_cast <const awh::net::addr_net_ipv4_t *> (awh_cast <const awh::net::attr_net_t *> (attr)->ip.get())->address);
			} break;
			// Если атрибуты URI адреса являются IPv6-адресом
			case static_cast <uint8_t> (awh::net::type_t::IPV6): {
				// Создаем объект сетевого адреса на основе атрибутов URI адреса
				awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
				// Выполняем установку IPv6-адреса хоста в объект сетевого адреса на основе атрибутов URI адреса
				addr.source(awh_cast <const awh::net::attr_net_t *> (attr)->ip.get(), awh::net_addr_t::endian_t::LITTLE);
				// Выполняем проверку, что порт хоста в атрибутах URI адреса совпадает с ожидаемым портом в параметрах теста
				ASSERT_EQ(this->_parameter.port, awh_cast <const awh::net::attr_net_t *> (attr)->port);
				// Выполняем проверку, что IPv6-адрес хоста в атрибутах URI адреса совпадает с ожидаемым IPv6-адресом в параметрах теста
				ASSERT_EQ(this->_parameter.ipv6, static_cast <std::string> (addr));
			} break;
		}
	}
	/**
	 * Выполняем перебор всех запросов в параметрах теста
	 */
	for(size_t i = 0; i < this->_parameter.request.size(); i++){
		// Выполняем установку запроса в объект работы с URI
		(* this->_uri.get()) = this->_parameter.request[i];
		// Выполняем проверку, что результат генерации URI совпадает с ожидаемым результатом в параметрах теста
		ASSERT_EQ(this->_parameter.result[i], static_cast <std::string> (* this->_uri.get()));
	}
}

/**
 * @brief Инициализация параметров теста выполнения сравнения URI
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, UriTestMatchParameterizedFixture,
	::testing::Values(
		UriTestMatchParameter({
			80,
			0,
			"",
			"www.example.com",
			"http://www.example.com/path/to/resource?query=1&id=123#frag",
			awh::uri_t::type_t::HTTP,
			{
				"/addr/data/test?anyks=com&id=222#best",
				"/?anyks=com&id=222#bestTop",
				"/path/to/data",
				"/user/goga#bad",
				"#goga",
				"?anyks=com&id=222",
				"https://anyks.com/api/v1/resource?query=1&id=123#frag"
			},
			{
				"http://www.example.com/addr/data/test?anyks=com&id=222#best",
				"http://www.example.com/?anyks=com&id=222#bestTop",
				"http://www.example.com/path/to/data",
				"http://www.example.com/user/goga#bad",
				"http://www.example.com/user/goga#goga",
				"http://www.example.com/user/goga?anyks=com&id=222",
				"https://anyks.com/api/v1/resource?id=123&query=1#frag"
			}
		}),
		UriTestMatchParameter({
			443,
			16777343,
			"",
			"",
			"https://127.0.0.1/path/to/resource?query=1&id=123#frag",
			awh::uri_t::type_t::HTTPS,
			{
				"/addr/data/test?anyks=com&id=222#best",
				"/?anyks=com&id=222#bestTop",
				"/path/to/data",
				"/user/goga#bad",
				"https://anyks.com/api/v1/resource?query=1&id=123#frag"
			},
			{
				"https://127.0.0.1/addr/data/test?anyks=com&id=222#best",
				"https://127.0.0.1/?anyks=com&id=222#bestTop",
				"https://127.0.0.1/path/to/data",
				"https://127.0.0.1/user/goga#bad",
				"https://anyks.com/api/v1/resource?id=123&query=1#frag"
			}
		}),
		UriTestMatchParameter({
			443,
			0,
			"2001:DB8::1",
			"",
			"https://[2001:DB8::1]/path/to/resource?query=1&id=123#frag",
			awh::uri_t::type_t::HTTPS,
			{
				"/addr/data/test?anyks=com&id=222#best",
				"/?anyks=com&id=222#bestTop",
				"/path/to/data",
				"/user/goga#bad",
				"https://anyks.com/api/v1/resource?query=1&id=123#frag"
			},
			{
				"https://[2001:DB8::1]/addr/data/test?anyks=com&id=222#best",
				"https://[2001:DB8::1]/?anyks=com&id=222#bestTop",
				"https://[2001:DB8::1]/path/to/data",
				"https://[2001:DB8::1]/user/goga#bad",
				"https://anyks.com/api/v1/resource?id=123&query=1#frag"
			}
		})
	)
);
