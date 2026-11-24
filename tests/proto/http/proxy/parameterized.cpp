/**
 * @file parameterized.cpp
 * @date 2026-11-23
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
 * @brief Параметризованные тесты клиента HTTP-прокси — прогон подготовленных наборов входных данных
 *        через методы модуля с проверкой сборки authority целевого хоста и обработки кодов состояния ответа прокси
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include "proxy.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * Псевдонимы вложенных типов клиента HTTP-прокси
 */
using proxy_t = proxy::client_t;
// Псевдоним контекста обмена
using ctx_t   = proxy::client_t::ctx_t;
// Псевдоним состояния обмена
using state_t = proxy::client_t::state_t;

/**
 * @brief Структура параметров теста сборки authority целевого хоста
 *
 */
struct AuthorityParameter {
	// Адрес целевого хоста
	std::string addr;
	// Порт целевого хоста
	uint16_t port;
	// Ожидаемая строка authority
	std::string authority;
};

/**
 * @brief Класс фикстуры теста сборки authority целевого хоста
 *
 */
class AuthorityParameterizedFixture : public HttpProxyFixture, public ::testing::WithParamInterface <AuthorityParameter> {
	public:
		// Параметры теста сборки authority
		AuthorityParameter _parameter = GetParam();
};

/**
 * @brief Метод проверки сборки authority целевого хоста для разных типов адресов
 *
 */
TEST_P(AuthorityParameterizedFixture, ConnectAuthorityTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост согласно параметрам теста
	this->host(ctx, this->_parameter.addr, this->_parameter.port);
	// Извлекаем сформированный запрос CONNECT
	const std::string wire = this->request(* proxy, ctx);
	// Формируем ожидаемую стартовую строку запроса
	const std::string line = "CONNECT " + this->_parameter.authority + " HTTP/1.1\r\n";
	// Проверяем что стартовая строка запроса собрана корректно
	ASSERT_EQ(wire.compare(0, line.size(), line), 0);
	// Формируем ожидаемый заголовок Host
	const std::string host = "Host: " + this->_parameter.authority + "\r\n";
	// Проверяем что заголовок Host собран корректно
	ASSERT_NE(wire.find(host), std::string::npos);
}

/**
 * Набор параметров теста сборки authority целевого хоста
 */
INSTANTIATE_TEST_SUITE_P(HttpProxyAuthority, AuthorityParameterizedFixture, ::testing::Values(
	AuthorityParameter{"example.com", 443, "example.com:443"},
	AuthorityParameter{"example.com", 8080, "example.com:8080"},
	AuthorityParameter{"192.0.2.1", 443, "192.0.2.1:443"},
	AuthorityParameter{"192.0.2.1", 3128, "192.0.2.1:3128"},
	AuthorityParameter{"2001:db8::1", 443, "[2001:db8::1]:443"},
	AuthorityParameter{"2001:db8:0:0:0:0:0:1", 8080, "[2001:db8::1]:8080"},
	AuthorityParameter{"::1", 443, "[::1]:443"}
));

/**
 * @brief Структура параметров теста обработки кода состояния ответа прокси
 *
 */
struct StatusCodeParameter {
	// Код состояния ответа прокси
	uint16_t code;
	// Ожидаемое состояние обмена после разбора ответа
	state_t state;
};

/**
 * @brief Класс фикстуры теста обработки кода состояния ответа прокси
 *
 */
class StatusCodeParameterizedFixture : public HttpProxyFixture, public ::testing::WithParamInterface <StatusCodeParameter> {
	public:
		// Параметры теста обработки кода состояния
		StatusCodeParameter _parameter = GetParam();
};

/**
 * @brief Метод проверки обработки кодов состояния ответа прокси на запрос CONNECT
 *
 */
TEST_P(StatusCodeParameterizedFixture, ConnectStatusCodeTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем ответ прокси с кодом состояния согласно параметрам теста
	std::string answer = "HTTP/1.1 " + std::to_string(this->_parameter.code) + " Test\r\n";
	// Ответы 1xx и 204 не объявляют кадрирование (PROXY1 отвергает его)
	if((this->_parameter.code >= 200) && (this->_parameter.code != 204))
		// Объявляем нулевой размер тела ответа
		answer += "Content-Length: 0\r\n";
	// Завершаем блок заголовков ответа
	answer += "\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что код состояния ответа сохранён
	ASSERT_EQ(ctx.code, this->_parameter.code);
	// Проверяем что состояние обмена соответствует ожидаемому
	ASSERT_EQ(ctx.state, this->_parameter.state);
}

/**
 * Набор параметров теста успешных кодов состояния ответа прокси
 */
INSTANTIATE_TEST_SUITE_P(HttpProxySuccessCodes, StatusCodeParameterizedFixture, ::testing::Values(
	StatusCodeParameter{200, state_t::HANDSHAKE},
	StatusCodeParameter{201, state_t::HANDSHAKE},
	StatusCodeParameter{202, state_t::HANDSHAKE},
	StatusCodeParameter{204, state_t::HANDSHAKE},
	StatusCodeParameter{299, state_t::HANDSHAKE}
));

/**
 * Набор параметров теста ошибочных кодов состояния ответа прокси
 */
INSTANTIATE_TEST_SUITE_P(HttpProxyFailureCodes, StatusCodeParameterizedFixture, ::testing::Values(
	StatusCodeParameter{301, state_t::BROKEN},
	StatusCodeParameter{400, state_t::BROKEN},
	StatusCodeParameter{403, state_t::BROKEN},
	StatusCodeParameter{404, state_t::BROKEN},
	StatusCodeParameter{405, state_t::BROKEN},
	StatusCodeParameter{500, state_t::BROKEN},
	StatusCodeParameter{502, state_t::BROKEN},
	StatusCodeParameter{503, state_t::BROKEN}
));
