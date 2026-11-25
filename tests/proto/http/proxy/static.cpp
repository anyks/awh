/**
 * @file static.cpp
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
 * @brief Статические тесты клиента HTTP-прокси — проверка сборки запросов CONNECT и absolute-form,
 *        разбора ответов прокси, установки туннеля, early data, авторизации Basic/Digest/Bearer
 *        и пересылки обычных HTTP-запросов через прокси
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
// Псевдоним режима работы через прокси
using proxy_mode_t = proxy::client_t::mode_t;

/**
 * @brief Метод проверки сборки запроса CONNECT для FQDN-адреса
 *
 */
TEST_F(HttpProxyFixture, ConnectRequestFqdnTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост с доменным именем
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что запрос CONNECT собран корректно
	ASSERT_EQ(wire, "CONNECT example.com:443 HTTP/1.1\r\nHost: example.com:443\r\n\r\n");
	// Проверяем что контекст перешёл в состояние ожидания ответа
	ASSERT_EQ(ctx.state, state_t::REQUEST);
}

/**
 * @brief Метод проверки сборки запроса CONNECT для IPv4-адреса
 *
 */
TEST_F(HttpProxyFixture, ConnectRequestIpv4Test){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост с IPv4-адресом
	this->host(ctx, "192.0.2.1", 8080);
	// Извлекаем сформированный запрос CONNECT
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что запрос CONNECT собран корректно
	ASSERT_EQ(wire, "CONNECT 192.0.2.1:8080 HTTP/1.1\r\nHost: 192.0.2.1:8080\r\n\r\n");
}

/**
 * @brief Метод проверки сборки запроса CONNECT для IPv6-адреса (скобки)
 *
 */
TEST_F(HttpProxyFixture, ConnectRequestIpv6Test){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост с IPv6-адресом
	this->host(ctx, "2001:db8::1", 443);
	// Извлекаем сформированный запрос CONNECT
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что запрос CONNECT собран с IPv6-адресом в скобках
	ASSERT_EQ(wire, "CONNECT [2001:db8::1]:443 HTTP/1.1\r\nHost: [2001:db8::1]:443\r\n\r\n");
}

/**
 * @brief Метод проверки успешной установки туннеля по ответу 200
 *
 */
TEST_F(HttpProxyFixture, ConnectSuccessTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем успешный ответ прокси на запрос CONNECT
	const std::string answer = "HTTP/1.1 200 Connection established\r\n\r\n";
	// Разбираем ответ прокси
	const size_t consumed = proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что ответ потреблён полностью
	ASSERT_EQ(consumed, answer.size());
	// Проверяем что код состояния ответа сохранён
	ASSERT_EQ(ctx.code, 200);
	// Проверяем что туннель установлен
	ASSERT_EQ(ctx.state, state_t::HANDSHAKE);
}

/**
 * @brief Метод проверки сохранности early data за успешным ответом CONNECT
 *
 */
TEST_F(HttpProxyFixture, ConnectEarlyDataTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем успешный ответ прокси с данными туннеля в хвосте
	const std::string answer = "HTTP/1.1 200 Connection established\r\n\r\nTUNNELDATA";
	// Разбираем ответ прокси
	const size_t consumed = proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что парсер остановился ровно на границе заголовков
	ASSERT_EQ(consumed, answer.size() - 10u);
	// Проверяем что туннель установлен
	ASSERT_EQ(ctx.state, state_t::HANDSHAKE);
	// Проверяем что хвост буфера принадлежит туннелю
	ASSERT_EQ(answer.substr(consumed), "TUNNELDATA");
}

/**
 * @brief Метод проверки разбора фрагментированного ответа прокси
 *
 */
TEST_F(HttpProxyFixture, ConnectFragmentedTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем успешный ответ прокси на запрос CONNECT
	const std::string answer = "HTTP/1.1 200 Connection established\r\nProxy-Agent: test\r\n\r\n";
	/**
	 * Подаём ответ прокси по одному октету
	 */
	for(size_t i = 0; i < answer.size(); i++){
		// Разбираем очередной октет ответа
		proxy->parse(answer.data() + i, 1, ctx);
		// Если туннель ещё не установлен
		if(ctx.state != state_t::HANDSHAKE)
			// Проверяем что обмен остаётся в состоянии ожидания ответа
			ASSERT_EQ(ctx.state, state_t::REQUEST);
	}
	// Проверяем что туннель установлен после последнего октета
	ASSERT_EQ(ctx.state, state_t::HANDSHAKE);
	// Проверяем что заголовок ответа собран
	ASSERT_EQ(ctx.headers.size(), 1u);
	// Проверяем имя собранного заголовка
	ASSERT_EQ(ctx.headers[0].first, "Proxy-Agent");
	// Проверяем значение собранного заголовка
	ASSERT_EQ(ctx.headers[0].second, "test");
}

/**
 * @brief Метод проверки пропуска промежуточного ответа 1xx
 *
 */
TEST_F(HttpProxyFixture, ConnectInterimTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем промежуточный ответ прокси
	const std::string interim = "HTTP/1.1 100 Continue\r\n\r\n";
	// Разбираем промежуточный ответ прокси
	const size_t consumed = proxy->parse(interim.data(), interim.size(), ctx);
	// Проверяем что промежуточный ответ потреблён полностью
	ASSERT_EQ(consumed, interim.size());
	// Проверяем что обмен остаётся в состоянии ожидания финального ответа
	ASSERT_EQ(ctx.state, state_t::REQUEST);
	// Формируем финальный успешный ответ прокси
	const std::string answer = "HTTP/1.1 200 Connection established\r\n\r\n";
	// Разбираем финальный ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что туннель установлен
	ASSERT_EQ(ctx.state, state_t::HANDSHAKE);
	// Проверяем что код состояния финального ответа сохранён
	ASSERT_EQ(ctx.code, 200);
}

/**
 * @brief Метод проверки авторизации Basic по ответу 407
 *
 */
TEST_F(HttpProxyFixture, ConnectAuthBasicTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем ответ прокси с требованием авторизации Basic
	const std::string answer = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что контекст перешёл в состояние повторной авторизации
	ASSERT_EQ(ctx.state, state_t::AUTH);
	// Проверяем что счётчик повторов увеличен
	ASSERT_EQ(ctx.retries, 1);
	// Извлекаем повторный запрос CONNECT с учётными данными
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что повторный запрос содержит заголовок авторизации Basic
	ASSERT_NE(wire.find("Proxy-Authorization: Basic dXNlcjpwYXNz\r\n"), std::string::npos);
	// Проверяем что контекст перешёл в состояние ожидания ответа
	ASSERT_EQ(ctx.state, state_t::REQUEST);
	// Формируем успешный ответ прокси на повторный запрос
	const std::string success = "HTTP/1.1 200 Connection established\r\n\r\n";
	// Разбираем успешный ответ прокси
	proxy->parse(success.data(), success.size(), ctx);
	// Проверяем что туннель установлен
	ASSERT_EQ(ctx.state, state_t::HANDSHAKE);
}

/**
 * @brief Метод проверки отказа при 407 без учётных данных
 *
 */
TEST_F(HttpProxyFixture, ConnectAuthNoCredentialsTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем ответ прокси с требованием авторизации
	const std::string answer = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что обмен завершён ошибкой
	ASSERT_EQ(ctx.state, state_t::BROKEN);
	// Проверяем что голова ответа 407 выдана вызывающей стороне
	ASSERT_TRUE(events.headFired);
	// Проверяем код состояния выданной головы ответа
	ASSERT_EQ(events.code, 407);
}

/**
 * @brief Метод проверки лимита повторов авторизации
 *
 */
TEST_F(HttpProxyFixture, ConnectAuthRetryLimitTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем ответ прокси с требованием авторизации
	const std::string answer = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Извлекаем повторный запрос CONNECT с учётными данными
	this->request(* proxy, ctx);
	// Разбираем повторный ответ 407
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что обмен завершён ошибкой после исчерпания повторов
	ASSERT_EQ(ctx.state, state_t::BROKEN);
}

/**
 * @brief Метод проверки авторизации Digest по ответу 407
 *
 */
TEST_F(HttpProxyFixture, ConnectAuthDigestTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем ответ прокси с требованием авторизации Digest
	const std::string answer = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Digest realm=\"proxy\", nonce=\"abc123\", qop=\"auth\", algorithm=MD5\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что контекст перешёл в состояние повторной авторизации
	ASSERT_EQ(ctx.state, state_t::AUTH);
	// Извлекаем повторный запрос CONNECT с учётными данными Digest
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что повторный запрос содержит заголовок авторизации Digest
	ASSERT_NE(wire.find("Proxy-Authorization: Digest "), std::string::npos);
	// Проверяем что заголовок содержит имя пользователя
	ASSERT_NE(wire.find("username=\"user\""), std::string::npos);
	// Проверяем что заголовок содержит область авторизации
	ASSERT_NE(wire.find("realm=\"proxy\""), std::string::npos);
	// Проверяем что заголовок содержит одноразовое значение сервера
	ASSERT_NE(wire.find("nonce=\"abc123\""), std::string::npos);
	// Проверяем что заголовок содержит цель запроса
	ASSERT_NE(wire.find("uri=\"example.com:443\""), std::string::npos);
	// Проверяем что заголовок содержит рассчитанный ответ
	ASSERT_NE(wire.find("response=\""), std::string::npos);
}

/**
 * @brief Метод проверки предпочтения схемы Digest перед Basic
 *
 */
TEST_F(HttpProxyFixture, ConnectAuthPreferDigestTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем ответ прокси с двумя вызовами авторизации
	const std::string answer = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nProxy-Authenticate: Digest realm=\"proxy\", nonce=\"abc123\", qop=\"auth\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Извлекаем повторный запрос CONNECT с учётными данными
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что выбрана схема Digest
	ASSERT_NE(wire.find("Proxy-Authorization: Digest "), std::string::npos);
}

/**
 * @brief Метод проверки превентивной авторизации Basic
 *
 */
TEST_F(HttpProxyFixture, ConnectPreemptiveBasicTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Включаем режим превентивной авторизации
	proxy->preemptive(true);
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что первый запрос уже содержит заголовок авторизации Basic
	ASSERT_NE(wire.find("Proxy-Authorization: Basic dXNlcjpwYXNz\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки превентивной авторизации Bearer
 *
 */
TEST_F(HttpProxyFixture, ConnectPreemptiveBearerTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем токен доступа
	proxy->setToken("secret-token");
	// Включаем режим превентивной авторизации
	proxy->preemptive(true);
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что первый запрос содержит заголовок авторизации Bearer
	ASSERT_NE(wire.find("Proxy-Authorization: Bearer secret-token\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки ошибки разбора бракованного ответа прокси
 *
 */
TEST_F(HttpProxyFixture, ConnectGarbageTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем бракованный ответ прокси
	const std::string answer = "NOT-HTTP GARBAGE\r\n\r\n";
	// Разбираем бракованный ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что обмен завершён ошибкой
	ASSERT_EQ(ctx.state, state_t::BROKEN);
	// Проверяем что название ошибки разбора доступно
	ASSERT_FALSE(proxy->error().empty());
}

/**
 * @brief Метод проверки лимита суммарного размера заголовков ответа
 *
 */
TEST_F(HttpProxyFixture, ConnectHeadersLimitTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем ответ прокси с заголовком сверх лимита
	std::string answer = "HTTP/1.1 200 Connection established\r\nX-Large: ";
	// Добавляем значение заголовка размером больше лимита
	answer.append(20 * 1024, 'a');
	// Завершаем блок заголовков ответа
	answer += "\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что обмен завершён ошибкой превышения лимита
	ASSERT_EQ(ctx.state, state_t::BROKEN);
}

/**
 * @brief Метод проверки пересылки простого GET-запроса в absolute-form
 *
 */
TEST_F(HttpProxyFixture, ForwardSimpleGetTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя в origin-form
	const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\nUser-Agent: test\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Извлекаем сформированный запрос к прокси
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что запрос пересобран в absolute-form
	ASSERT_EQ(wire, "GET http://example.com/path HTTP/1.1\r\nHost: example.com\r\nUser-Agent: test\r\n\r\n");
	// Проверяем что контекст перешёл в состояние ожидания ответа
	ASSERT_EQ(ctx.state, state_t::REQUEST);
}

/**
 * @brief Метод проверки пересылки POST-запроса с телом и вычисткой hop-by-hop
 *
 */
TEST_F(HttpProxyFixture, ForwardPostBodyTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя с телом и заголовками hop-by-hop
	const std::string user = "POST /api HTTP/1.1\r\nHost: example.com\r\nConnection: keep-alive\r\nContent-Length: 5\r\n\r\nhello";
	// Передаём запрос пользователя на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Извлекаем сформированный запрос к прокси
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что запрос пересобран в absolute-form с телом
	ASSERT_EQ(wire, "POST http://example.com/api HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello");
}

/**
 * @brief Метод проверки перекадрирования chunked-тела запроса пользователя
 *
 */
TEST_F(HttpProxyFixture, ForwardChunkedBodyTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя с телом chunked
	const std::string user = "POST /api HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Извлекаем сформированный запрос к прокси
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что тело декодировано и перекадрировано фиксированным размером
	ASSERT_EQ(wire, "POST http://example.com/api HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello");
}

/**
 * @brief Метод проверки вычистки заголовков, названных токенами Connection
 *
 */
TEST_F(HttpProxyFixture, ForwardConnectionTokensTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя с заголовками, названными токенами Connection
	const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\nConnection: X-Custom , x-other\t, keep-alive\r\nX-Custom: secret\r\nX-Other: hidden\r\nX-Keep: visible\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Извлекаем сформированный запрос к прокси
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что названные токенами заголовки вырезаны, а остальные сохранены
	ASSERT_EQ(wire, "GET http://example.com/path HTTP/1.1\r\nHost: example.com\r\nX-Keep: visible\r\n\r\n");
}

/**
 * @brief Метод проверки сохранения хвоста запроса при частичном освобождении байтов
 *
 */
TEST_F(HttpProxyFixture, ForwardPartialConsumeTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя
	const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Буфер сформированного запроса
	uint8_t * buffer = nullptr;
	// Размер сформированного запроса
	size_t size = 0;
	// Извлекаем сформированный запрос к прокси
	ASSERT_TRUE(proxy->buffer(&buffer, size, ctx));
	// Копируем сформированный запрос в строку
	const std::string wire(reinterpret_cast <const char *> (buffer), size);
	// Освобождаем все байты запроса кроме последних трёх
	proxy->consume(size - 3);
	// Буфер остатка запроса
	uint8_t * rest = nullptr;
	// Размер остатка запроса
	size_t restSize = 0;
	// Извлекаем остаток запроса к прокси
	ASSERT_TRUE(proxy->buffer(&rest, restSize, ctx));
	// Проверяем что остаток равен трём последним байтам запроса
	ASSERT_EQ(restSize, 3u);
	// Проверяем побайтовое совпадение остатка с хвостом запроса
	ASSERT_EQ(std::string(reinterpret_cast <const char *> (rest), restSize), wire.substr(size - 3));
	// Освобождаем остаток байтов запроса
	proxy->consume(restSize);
	// Проверяем что неотправленных запросов больше нет
	ASSERT_TRUE(this->request(* proxy, ctx).empty());
}

/**
 * @brief Метод проверки сохранения absolute-form цели запроса пользователя
 *
 */
TEST_F(HttpProxyFixture, ForwardAbsoluteFormTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя уже в absolute-form
	const std::string user = "GET http://example.com/path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Извлекаем сформированный запрос к прокси
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что цель запроса сохранена
	ASSERT_EQ(wire, "GET http://example.com/path HTTP/1.1\r\nHost: example.com\r\n\r\n");
}

/**
 * @brief Метод проверки отказа пересылки https-цели без туннеля
 *
 */
TEST_F(HttpProxyFixture, ForwardHttpsTargetTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя с https-целью
	const std::string user = "GET https://example.com/path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	ASSERT_FALSE(proxy->forward(user.data(), user.size(), ctx));
	// Проверяем что обмен завершён ошибкой
	ASSERT_EQ(ctx.state, state_t::BROKEN);
}

/**
 * @brief Метод проверки отказа пересылки запроса без заголовка Host
 *
 */
TEST_F(HttpProxyFixture, ForwardNoHostTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя без заголовка Host
	const std::string user = "GET /path HTTP/1.1\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	ASSERT_FALSE(proxy->forward(user.data(), user.size(), ctx));
}

/**
 * @brief Метод проверки приёма фрагментированного запроса пользователя
 *
 */
TEST_F(HttpProxyFixture, ForwardPartialRequestTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем первую часть запроса пользователя
	const std::string first = "GET /path HTTP/1.1\r\nHost: example";
	// Передаём первую часть запроса пользователя
	ASSERT_TRUE(proxy->forward(first.data(), first.size(), ctx));
	// Проверяем что запрос к прокси ещё не собран
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Формируем вторую часть запроса пользователя
	const std::string second = ".com\r\n\r\n";
	// Передаём вторую часть запроса пользователя
	ASSERT_TRUE(proxy->forward(second.data(), second.size(), ctx));
	// Извлекаем сформированный запрос к прокси
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что запрос собран из обеих частей
	ASSERT_EQ(wire, "GET http://example.com/path HTTP/1.1\r\nHost: example.com\r\n\r\n");
}

/**
 * @brief Метод проверки доставки ответа прокси в режиме пересылки
 *
 */
TEST_F(HttpProxyFixture, ForwardResponseTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя
	const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ прокси с телом
	const std::string answer = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nhello";
	// Разбираем ответ прокси
	const size_t consumed = proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что ответ потреблён полностью
	ASSERT_EQ(consumed, answer.size());
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тело ответа доставлено
	ASSERT_EQ(events.body, "hello");
	// Проверяем что голова ответа выдана
	ASSERT_TRUE(events.headFired);
	// Проверяем код состояния выданной головы ответа
	ASSERT_EQ(events.code, 200);
	// Проверяем что заголовки ответа собраны в контексте
	ASSERT_EQ(ctx.headers.size(), 2u);
}

/**
 * @brief Метод проверки доставки chunked-тела ответа в режиме пересылки
 *
 */
TEST_F(HttpProxyFixture, ForwardResponseChunkedTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя
	const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ прокси с телом chunked
	const std::string answer = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тело ответа декодировано и доставлено
	ASSERT_EQ(events.body, "hello world");
}

/**
 * @brief Метод проверки доставки трейлеров ответа в режиме пересылки
 *
 */
TEST_F(HttpProxyFixture, ForwardResponseTrailersTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя
	const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ прокси с телом chunked и трейлером
	const std::string answer = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nTrailer: X-Checksum\r\n\r\n5\r\nhello\r\n0\r\nX-Checksum: abc123\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тело ответа декодировано и доставлено
	ASSERT_EQ(events.body, "hello");
	// Проверяем что трейлер ответа собран в контексте
	ASSERT_EQ(ctx.trailers.size(), 1u);
	// Проверяем что трейлер ответа доставлен вызывающей стороне
	ASSERT_EQ(events.trailers.size(), 1u);
	// Проверяем имя доставленного трейлера
	ASSERT_EQ(events.trailers[0].first, "X-Checksum");
	// Проверяем значение доставленного трейлера
	ASSERT_EQ(events.trailers[0].second, "abc123");
}

/**
 * @brief Метод проверки авторизации в режиме пересылки по ответу 407
 *
 */
TEST_F(HttpProxyFixture, ForwardAuthTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем запрос пользователя
	const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ прокси с требованием авторизации
	const std::string answer = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что повторный запрос поставлен в очередь (состояние ожидания ответа)
	ASSERT_EQ(ctx.state, state_t::REQUEST);
	// Извлекаем повторный запрос к прокси с учётными данными
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что повторный запрос содержит заголовок авторизации
	ASSERT_NE(wire.find("Proxy-Authorization: Basic dXNlcjpwYXNz\r\n"), std::string::npos);
	// Проверяем что цель повторного запроса сохранена
	ASSERT_NE(wire.find("GET http://example.com/path HTTP/1.1\r\n"), std::string::npos);
	// Формируем успешный ответ прокси на повторный запрос
	const std::string success = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
	// Разбираем успешный ответ прокси
	proxy->parse(success.data(), success.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
}

/**
 * @brief Метод проверки keep-alive в режиме пересылки
 *
 */
TEST_F(HttpProxyFixture, ForwardKeepAliveTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	/**
	 * Проводим два последовательных обмена в одном соединении
	 */
	for(int i = 0; i < 2; i++){
		// Формируем запрос пользователя
		const std::string user = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
		// Передаём запрос пользователя на пересылку
		ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
		// Извлекаем сформированный запрос к прокси
		ASSERT_FALSE(this->request(* proxy, ctx).empty());
		// Формируем ответ прокси с телом
		const std::string answer = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
		// Разбираем ответ прокси
		proxy->parse(answer.data(), answer.size(), ctx);
		// Проверяем что обмен завершён и очередь пуста
		ASSERT_EQ(ctx.state, state_t::NONE);
		// Готовим модуль к следующему обмену
		proxy->reset(ctx);
		// Проверяем что контекст сброшен
		ASSERT_EQ(ctx.state, state_t::NONE);
	}
	// Проверяем что тела обоих ответов доставлены
	ASSERT_EQ(events.body, "okok");
}

/**
 * @brief Метод проверки ответа на HEAD-запрос без тела
 *
 */
TEST_F(HttpProxyFixture, ForwardHeadResponseTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем HEAD-запрос пользователя
	const std::string user = "HEAD /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём запрос пользователя на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ прокси с Content-Length, но без тела (ответ на HEAD)
	const std::string answer = "HTTP/1.1 200 OK\r\nContent-Length: 12345\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тело не доставлено
	ASSERT_TRUE(events.body.empty());
}

/**
 * @brief Метод проверки полной очистки модуля для нового соединения
 *
 */
TEST_F(HttpProxyFixture, ClearTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена
	ctx_t ctx;
	// Устанавливаем целевой хост
	this->host(ctx, "example.com", 443);
	// Извлекаем сформированный запрос CONNECT
	this->request(* proxy, ctx);
	// Формируем успешный ответ прокси
	const std::string answer = "HTTP/1.1 200 Connection established\r\n\r\n";
	// Разбираем ответ прокси
	proxy->parse(answer.data(), answer.size(), ctx);
	// Очищаем модуль полностью
	proxy->clear(ctx);
	// Проверяем что контекст сброшен
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Устанавливаем целевой хост нового соединения
	this->host(ctx, "other.com", 80);
	// Извлекаем сформированный запрос CONNECT нового соединения
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что запрос нового соединения собран корректно
	ASSERT_EQ(wire, "CONNECT other.com:80 HTTP/1.1\r\nHost: other.com:80\r\n\r\n");
}

/**
 * @brief Метод проверки конвейера запросов в режиме пересылки (RFC 9112 §9.3.2)
 *
 */
TEST_F(HttpProxyFixture, ForwardPipelineTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем два запроса пользователя в одном буфере (конвейер)
	const std::string user =
		"GET /a HTTP/1.1\r\nHost: example.com\r\n\r\n"
		"GET /b HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём конвейер запросов на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Извлекаем первый сформированный запрос к прокси
	const std::string first = this->request(* proxy, ctx);
	// Проверяем что первый запрос пересобран в absolute-form
	ASSERT_EQ(first, "GET http://example.com/a HTTP/1.1\r\nHost: example.com\r\n\r\n");
	// Извлекаем второй сформированный запрос к прокси
	const std::string second = this->request(* proxy, ctx);
	// Проверяем что второй запрос пересобран в absolute-form
	ASSERT_EQ(second, "GET http://example.com/b HTTP/1.1\r\nHost: example.com\r\n\r\n");
	// Проверяем что больше неотправленных запросов нет
	ASSERT_TRUE(this->request(* proxy, ctx).empty());
	// Формируем ответ прокси на первый запрос
	const std::string answerA = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\na";
	// Разбираем ответ прокси на первый запрос
	proxy->parse(answerA.data(), answerA.size(), ctx);
	// Проверяем что очередь ещё ждёт второй ответ
	ASSERT_EQ(ctx.state, state_t::REQUEST);
	// Формируем ответ прокси на второй запрос
	const std::string answerB = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nb";
	// Разбираем ответ прокси на второй запрос
	proxy->parse(answerB.data(), answerB.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тела обоих ответов доставлены в порядке FIFO
	ASSERT_EQ(events.body, "ab");
}

/**
 * @brief Метод проверки кадрирования ответов конвейера с HEAD-запросом
 *
 */
TEST_F(HttpProxyFixture, ForwardPipelineHeadTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем конвейер из HEAD- и GET-запроса
	const std::string user =
		"HEAD /a HTTP/1.1\r\nHost: example.com\r\n\r\n"
		"GET /b HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём конвейер запросов на пересылку
	ASSERT_TRUE(proxy->forward(user.data(), user.size(), ctx));
	// Извлекаем первый сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Извлекаем второй сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ на HEAD с Content-Length, но без тела
	const std::string answerHead = "HTTP/1.1 200 OK\r\nContent-Length: 12345\r\n\r\n";
	// Разбираем ответ прокси на HEAD-запрос
	proxy->parse(answerHead.data(), answerHead.size(), ctx);
	// Формируем ответ на GET с телом
	const std::string answerGet = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nb";
	// Разбираем ответ прокси на GET-запрос
	proxy->parse(answerGet.data(), answerGet.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тело HEAD не доставлено, а тело GET доставлено
	ASSERT_EQ(events.body, "b");
}

/**
 * @brief Метод проверки фрагментированного конвейера запросов
 *
 */
TEST_F(HttpProxyFixture, ForwardPipelineFragmentedTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем первую часть конвейера (первый запрос и начало второго)
	const std::string first = "GET /a HTTP/1.1\r\nHost: example.com\r\n\r\nGET /b HTTP/1.1\r\nHost: example";
	// Передаём первую часть конвейера
	ASSERT_TRUE(proxy->forward(first.data(), first.size(), ctx));
	// Извлекаем первый сформированный запрос к прокси
	ASSERT_EQ(this->request(* proxy, ctx), "GET http://example.com/a HTTP/1.1\r\nHost: example.com\r\n\r\n");
	// Проверяем что второй запрос ещё не собран
	ASSERT_TRUE(this->request(* proxy, ctx).empty());
	// Формируем вторую часть конвейера (окончание второго запроса)
	const std::string second = ".com\r\n\r\n";
	// Передаём вторую часть конвейера
	ASSERT_TRUE(proxy->forward(second.data(), second.size(), ctx));
	// Извлекаем второй сформированный запрос к прокси
	ASSERT_EQ(this->request(* proxy, ctx), "GET http://example.com/b HTTP/1.1\r\nHost: example.com\r\n\r\n");
}

/**
 * @brief Метод проверки повторной авторизации в конвейере (повтор уходит в хвост)
 *
 */
TEST_F(HttpProxyFixture, ForwardPipelineAuthTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем конвейер из двух запросов
	const std::string user =
		"GET /a HTTP/1.1\r\nHost: example.com\r\n\r\n"
		"GET /b HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём конвейер запросов на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем первый сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Извлекаем второй сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ 407 на первый запрос
	const std::string answer407 = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ 407 на первый запрос
	proxy->parse(answer407.data(), answer407.size(), ctx);
	// Извлекаем повторный запрос первого запроса с учётными данными (ушёл в хвост)
	const std::string retry = this->request(* proxy, ctx);
	// Проверяем что повторный запрос содержит заголовок авторизации
	ASSERT_NE(retry.find("Proxy-Authorization: Basic dXNlcjpwYXNz\r\n"), std::string::npos);
	// Проверяем что повторный запрос сохраняет цель первого запроса
	ASSERT_NE(retry.find("GET http://example.com/a HTTP/1.1\r\n"), std::string::npos);
	// Формируем ответ на второй запрос (пришёл раньше повтора)
	const std::string answerB = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nb";
	// Разбираем ответ прокси на второй запрос
	proxy->parse(answerB.data(), answerB.size(), ctx);
	// Формируем ответ на повторный первый запрос
	const std::string answerA = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\na";
	// Разбираем ответ прокси на повторный первый запрос
	proxy->parse(answerA.data(), answerA.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тела доставлены в порядке ответов прокси
	ASSERT_EQ(events.body, "ba");
}

/**
 * @brief Метод проверки повторной авторизации каждого запроса конвейера по ответу 407
 *
 */
TEST_F(HttpProxyFixture, ForwardPipelineAuthBothTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем конвейер из двух запросов
	const std::string user =
		"GET /a HTTP/1.1\r\nHost: example.com\r\n\r\n"
		"GET /b HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём конвейер запросов на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем первый сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Извлекаем второй сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ 407 на первый запрос
	const std::string answer407 = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ 407 на первый запрос
	proxy->parse(answer407.data(), answer407.size(), ctx);
	// Извлекаем повторный первый запрос с учётными данными (ушёл в хвост)
	const std::string retryA = this->request(* proxy, ctx);
	// Проверяем что повторный первый запрос содержит заголовок авторизации
	ASSERT_NE(retryA.find("Proxy-Authorization: Basic dXNlcjpwYXNz\r\n"), std::string::npos);
	// Разбираем ответ 407 на второй запрос
	proxy->parse(answer407.data(), answer407.size(), ctx);
	// Проверяем что обмен не сломан: второй запрос получил собственный повтор
	ASSERT_EQ(ctx.state, state_t::REQUEST);
	// Извлекаем повторный второй запрос с учётными данными
	const std::string retryB = this->request(* proxy, ctx);
	// Проверяем что повторный второй запрос содержит заголовок авторизации
	ASSERT_NE(retryB.find("Proxy-Authorization: Basic dXNlcjpwYXNz\r\n"), std::string::npos);
	// Проверяем что повторный второй запрос сохраняет цель второго запроса
	ASSERT_NE(retryB.find("GET http://example.com/b HTTP/1.1\r\n"), std::string::npos);
	// Формируем ответ на повторный первый запрос
	const std::string answerA = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\na";
	// Разбираем ответ прокси на повторный первый запрос
	proxy->parse(answerA.data(), answerA.size(), ctx);
	// Формируем ответ на повторный второй запрос
	const std::string answerB = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nb";
	// Разбираем ответ прокси на повторный второй запрос
	proxy->parse(answerB.data(), answerB.size(), ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тела обоих повторных запросов доставлены в порядке FIFO
	ASSERT_EQ(events.body, "ab");
}

/**
 * @brief Метод проверки превентивных учётных данных после состоявшегося вызова авторизации
 *
 */
TEST_F(HttpProxyFixture, ForwardAuthChallengedPreemptiveTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Устанавливаем учётные данные пользователя
	proxy->setUser("user", "pass");
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем первый запрос пользователя
	const std::string user = "GET /a HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём первый запрос пользователя на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем ответ прокси с требованием авторизации
	const std::string answer407 = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"proxy\"\r\nContent-Length: 0\r\n\r\n";
	// Разбираем ответ прокси с требованием авторизации
	proxy->parse(answer407.data(), answer407.size(), ctx);
	// Извлекаем повторный запрос с учётными данными
	this->request(* proxy, ctx);
	// Формируем успешный ответ прокси на повторный запрос
	const std::string success = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
	// Разбираем успешный ответ прокси
	proxy->parse(success.data(), success.size(), ctx);
	// Формируем второй запрос пользователя в том же соединении
	const std::string next = "GET /b HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём второй запрос пользователя на пересылку
	ASSERT_TRUE(proxy->forward(next.data(), next.size(), ctx));
	// Извлекаем сформированный второй запрос к прокси
	const std::string wire = this->request(* proxy, ctx);
	// Проверяем что второй запрос сразу несёт учётные данные без нового вызова
	ASSERT_NE(wire.find("Proxy-Authorization: Basic dXNlcjpwYXNz\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки разбора конвейера ответов в одном буфере
 *
 */
TEST_F(HttpProxyFixture, ForwardPipelineResponseSegmentTest){
	// Создаём объект клиента HTTP-прокси
	std::unique_ptr <proxy_t> proxy = this->make();
	// Создаём объект сборщика событий
	events_t events;
	// Подписываем сборщик событий на функции обратного вызова клиента
	this->attach(* proxy, events);
	// Создаём контекст обмена в режиме пересылки
	ctx_t ctx;
	// Устанавливаем режим пересылки
	ctx.mode = proxy_mode_t::FORWARD;
	// Формируем конвейер из двух запросов
	const std::string user =
		"GET /a HTTP/1.1\r\nHost: example.com\r\n\r\n"
		"GET /b HTTP/1.1\r\nHost: example.com\r\n\r\n";
	// Передаём конвейер запросов на пересылку
	proxy->forward(user.data(), user.size(), ctx);
	// Извлекаем первый сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Извлекаем второй сформированный запрос к прокси
	this->request(* proxy, ctx);
	// Формируем первый ответ прокси
	const std::string answerA = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\na";
	// Формируем второй ответ прокси
	const std::string answerB = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nb";
	// Формируем оба ответа прокси в одном буфере
	const std::string answers = answerA + answerB;
	// Разбираем первый ответ из буфера
	const size_t consumed = proxy->parse(answers.data(), answers.size(), ctx);
	// Проверяем что потреблён ровно первый ответ
	ASSERT_EQ(consumed, answerA.size());
	// Проверяем что очередь ещё ждёт второй ответ
	ASSERT_EQ(ctx.state, state_t::REQUEST);
	// Разбираем второй ответ из хвоста буфера
	proxy->parse(answers.data() + consumed, answers.size() - consumed, ctx);
	// Проверяем что обмен завершён и очередь пуста
	ASSERT_EQ(ctx.state, state_t::NONE);
	// Проверяем что тела обоих ответов доставлены в порядке FIFO
	ASSERT_EQ(events.body, "ab");
}
