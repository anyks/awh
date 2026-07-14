/**
 * @file: static.cpp
 * @date: 2026-07-14
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "auth.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * Псевдонимы вложенных типов модуля авторизации
 */
using owner_t = auth_t::owner_t;
using type_t  = auth_t::type_t;
using hash_t  = auth_t::hash_t;

/**
 * @brief Метод проверки создания модуля и значений по умолчанию
 *
 */
TEST_F(AuthFixture, CreateDefaultsTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Проверяем что модуль создан
	ASSERT_TRUE(client != nullptr);
	// Проверяем что сторона работы соответствует клиенту
	ASSERT_EQ(client->owner(), owner_t::CLIENT);
	// Проверяем что тип авторизации по умолчанию не установлен
	ASSERT_EQ(client->type(), type_t::NONE);
	// Проверяем что без установленной схемы формирование заголовка возвращает пустоту
	ASSERT_TRUE(client->header().empty());
	// Проверяем что без установленной схемы разбор заголовка неуспешен
	ASSERT_FALSE(client->parse("Basic dXNlcjpwYXNz"));
	// Проверяем что без установленной схемы проверка неуспешна
	ASSERT_FALSE(client->check());
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Проверяем что сторона работы соответствует серверу
	ASSERT_EQ(server->owner(), owner_t::SERVER);
	// Устанавливаем тип авторизации BASIC
	server->type(type_t::BASIC);
	// Проверяем что тип авторизации установлен
	ASSERT_EQ(server->type(), type_t::BASIC);
}

/**
 * @brief Метод проверки формирования учётных данных BASIC на стороне клиента
 *
 */
TEST_F(AuthFixture, BasicClientHeaderTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему BASIC-авторизации
	client->type(type_t::BASIC);
	// Устанавливаем логин пользователя
	client->user("Aladdin");
	// Устанавливаем пароль пользователя
	client->pass("open sesame");
	// Формируем значение заголовка учётных данных
	const std::string header = client->header();
	// Проверяем что заголовок начинается со схемы BASIC
	ASSERT_EQ(header.compare(0, 6, "Basic "), 0);
	// Создаём объект криптографии для декодирования учётных данных
	awh::crypto_t crypto(this->_fmk.get(), this->_log.get());
	// Декодируем полезную нагрузку заголовка из BASE64
	const std::string credentials = crypto.decrypt <std::string> (header.substr(6), awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::BASE64);
	// Проверяем что декодированные учётные данные соответствуют паре «логин:пароль»
	ASSERT_EQ(credentials, "Aladdin:open sesame");
}

/**
 * @brief Метод проверки полного цикла BASIC-авторизации между клиентом и сервером
 *
 */
TEST_F(AuthFixture, BasicRoundTripTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему BASIC-авторизации
	client->type(type_t::BASIC);
	// Устанавливаем логин пользователя
	client->user("user");
	// Устанавливаем пароль пользователя
	client->pass("pass");
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BASIC-авторизации
	server->type(type_t::BASIC);
	// Регистрируем функцию проверки пары «логин/пароль»
	server->callbackCheckUser([](const std::string & user, const std::string & pass) -> bool {
		// Подтверждаем корректность только для известной пары
		return ((user == "user") && (pass == "pass"));
	});
	// Разбираем учётные данные клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем успешную авторизацию
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения неверных учётных данных BASIC
 *
 */
TEST_F(AuthFixture, BasicWrongCredentialsTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему BASIC-авторизации
	client->type(type_t::BASIC);
	// Устанавливаем логин пользователя
	client->user("user");
	// Устанавливаем неверный пароль пользователя
	client->pass("wrong");
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BASIC-авторизации
	server->type(type_t::BASIC);
	// Регистрируем функцию проверки пары «логин/пароль»
	server->callbackCheckUser([](const std::string & user, const std::string & pass) -> bool {
		// Подтверждаем корректность только для известной пары
		return ((user == "user") && (pass == "pass"));
	});
	// Разбираем учётные данные клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем что авторизация с неверным паролем отклонена
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки формирования вызова BASIC на стороне сервера
 *
 */
TEST_F(AuthFixture, BasicServerChallengeTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BASIC-авторизации
	server->type(type_t::BASIC);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Формируем значение вызова авторизации
	const std::string challenge = server->header();
	// Проверяем что вызов содержит схему и название сервера
	ASSERT_EQ(challenge, "Basic realm=\"anyks.com\"");
}

/**
 * @brief Метод проверки полного цикла BEARER-авторизации между клиентом и сервером
 *
 */
TEST_F(AuthFixture, BearerRoundTripTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему BEARER-авторизации
	client->type(type_t::BEARER);
	// Устанавливаем токен доступа
	client->token("mF_9.B5f-4.1JqM");
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Проверяем что заголовок содержит схему и токен доступа
	ASSERT_EQ(credentials, "Bearer mF_9.B5f-4.1JqM");
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BEARER-авторизации
	server->type(type_t::BEARER);
	// Регистрируем функцию проверки токена доступа
	server->callbackCheckToken([](const std::string & token) -> bool {
		// Подтверждаем корректность только для известного токена
		return (token == "mF_9.B5f-4.1JqM");
	});
	// Разбираем токен доступа клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем успешную авторизацию
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения неверного токена BEARER
 *
 */
TEST_F(AuthFixture, BearerWrongTokenTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BEARER-авторизации
	server->type(type_t::BEARER);
	// Регистрируем функцию проверки токена доступа
	server->callbackCheckToken([](const std::string & token) -> bool {
		// Подтверждаем корректность только для известного токена
		return (token == "valid-token");
	});
	// Разбираем неизвестный токен доступа
	ASSERT_TRUE(server->parse("Bearer invalid-token"));
	// Проверяем что авторизация с неверным токеном отклонена
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки разбора вызова DIGEST на стороне клиента
 *
 */
TEST_F(AuthFixture, DigestClientParseChallengeTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем логин пользователя
	client->user("Mufasa");
	// Устанавливаем пароль пользователя
	client->pass("Circle Of Life");
	// Устанавливаем параметры HTTP-запроса
	client->uri("/dir/index.html");
	// Разбираем вызов сервера с фиксированными параметрами
	ASSERT_TRUE(client->parse("Digest realm=\"test-realm\", qop=\"auth\", nonce=\"abc123def\", opaque=\"op4que\", algorithm=MD5"));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Проверяем что учётные данные содержат схему DIGEST
	ASSERT_NE(credentials.find("Digest"), std::string::npos);
	// Проверяем что разобранный логин пользователя отражён в учётных данных
	ASSERT_NE(credentials.find("username=\"Mufasa\""), std::string::npos);
	// Проверяем что разобранное название сервера отражено в учётных данных
	ASSERT_NE(credentials.find("realm=\"test-realm\""), std::string::npos);
	// Проверяем что разобранный ключ сервера отражён в учётных данных
	ASSERT_NE(credentials.find("nonce=\"abc123def\""), std::string::npos);
	// Проверяем что разобранный временный ключ сессии отражён в учётных данных
	ASSERT_NE(credentials.find("opaque=\"op4que\""), std::string::npos);
	// Проверяем что параметры HTTP-запроса отражены в учётных данных
	ASSERT_NE(credentials.find("uri=\"/dir/index.html\""), std::string::npos);
	// Проверяем что рассчитан ответ клиента
	ASSERT_NE(credentials.find("response=\""), std::string::npos);
}

/**
 * @brief Метод проверки полного цикла DIGEST-авторизации между клиентом и сервером
 *
 */
TEST_F(AuthFixture, DigestRoundTripTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов авторизации сервера
	const std::string challenge = server->header();
	// Проверяем что вызов сервера сформирован
	ASSERT_NE(challenge.find("Digest"), std::string::npos);
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем логин пользователя
	client->user("login");
	// Устанавливаем пароль пользователя
	client->pass("secret");
	// Устанавливаем параметры HTTP-запроса
	client->uri("/api/resource");
	// Разбираем вызов сервера
	ASSERT_TRUE(client->parse(challenge));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Проверяем что учётные данные содержат ответ клиента
	ASSERT_NE(credentials.find("response=\""), std::string::npos);
	// Сервер разбирает учётные данные клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем успешную авторизацию
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения неверного пароля DIGEST
 *
 */
TEST_F(AuthFixture, DigestWrongPasswordTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину (пароль на сервере отличается)
	server->callbackExtractPass([](const std::string &) -> std::string {
		// Возвращаем пароль, отличный от клиентского
		return "server-side-password";
	});
	// Формируем вызов авторизации сервера
	const std::string challenge = server->header();
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем логин пользователя
	client->user("login");
	// Устанавливаем пароль пользователя (не совпадает с серверным)
	client->pass("client-side-password");
	// Устанавливаем параметры HTTP-запроса
	client->uri("/api/resource");
	// Разбираем вызов сервера
	ASSERT_TRUE(client->parse(challenge));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Сервер разбирает учётные данные клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем что авторизация с неверным паролем отклонена
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки полного цикла DIGEST-авторизации в сессионном режиме (-sess)
 *
 */
TEST_F(AuthFixture, DigestSessionRoundTripTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации с алгоритмом SHA-256
	server->type(type_t::DIGEST, hash_t::SHA256);
	// Включаем сессионный режим алгоритма
	server->session(true);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов авторизации сервера
	const std::string challenge = server->header();
	// Проверяем что вызов содержит сессионный вариант алгоритма
	ASSERT_NE(challenge.find("SHA-256-sess"), std::string::npos);
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации с алгоритмом SHA-256
	client->type(type_t::DIGEST, hash_t::SHA256);
	// Устанавливаем логин пользователя
	client->user("login");
	// Устанавливаем пароль пользователя
	client->pass("secret");
	// Устанавливаем параметры HTTP-запроса
	client->uri("/api/resource");
	// Разбираем вызов сервера (клиент должен подхватить сессионный режим)
	ASSERT_TRUE(client->parse(challenge));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Проверяем что учётные данные содержат сессионный вариант алгоритма
	ASSERT_NE(credentials.find("SHA-256-sess"), std::string::npos);
	// Сервер разбирает учётные данные клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем успешную авторизацию в сессионном режиме
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки инкремента счётчика запросов DIGEST (nonce count)
 *
 */
TEST_F(AuthFixture, DigestNonceCountIncrementTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем логин пользователя
	client->user("login");
	// Устанавливаем пароль пользователя
	client->pass("secret");
	// Устанавливаем параметры HTTP-запроса
	client->uri("/api/resource");
	// Разбираем вызов сервера с фиксированным ключом сервера
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"nonce-A\", algorithm=MD5"));
	// Проверяем что первый запрос использует счётчик 00000001
	ASSERT_NE(client->header().find("nc=00000001"), std::string::npos);
	// Проверяем что второй запрос увеличивает счётчик до 00000002
	ASSERT_NE(client->header().find("nc=00000002"), std::string::npos);
	// Проверяем что третий запрос увеличивает счётчик до 00000003
	ASSERT_NE(client->header().find("nc=00000003"), std::string::npos);
	// Разбираем новый вызов сервера с изменённым ключом сервера
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"nonce-B\", algorithm=MD5"));
	// Проверяем что смена ключа сервера сбрасывает счётчик к 00000001
	ASSERT_NE(client->header().find("nc=00000001"), std::string::npos);
}

/**
 * @brief Метод проверки защиты DIGEST от повторного воспроизведения запроса (replay)
 *
 */
TEST_F(AuthFixture, DigestReplayDetectionTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов авторизации сервера
	const std::string challenge = server->header();
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем логин пользователя
	client->user("login");
	// Устанавливаем пароль пользователя
	client->pass("secret");
	// Устанавливаем параметры HTTP-запроса
	client->uri("/api/resource");
	// Разбираем вызов сервера
	ASSERT_TRUE(client->parse(challenge));
	// Формируем учётные данные первого запроса (nc=00000001)
	const std::string first = client->header();
	// Формируем учётные данные второго запроса (nc=00000002)
	const std::string second = client->header();
	// Сервер разбирает учётные данные первого запроса
	ASSERT_TRUE(server->parse(first));
	// Проверяем что первый запрос принят
	ASSERT_TRUE(server->check());
	// Сервер разбирает учётные данные второго запроса
	ASSERT_TRUE(server->parse(second));
	// Проверяем что второй запрос с возросшим счётчиком принят
	ASSERT_TRUE(server->check());
	// Сервер повторно разбирает учётные данные первого запроса (атака повторного воспроизведения)
	ASSERT_TRUE(server->parse(first));
	// Проверяем что повтор с уже использованным счётчиком отклонён
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки отклонения учётных данных DIGEST с чужим ключом сервера (nonce)
 *
 */
TEST_F(AuthFixture, DigestNonceMismatchTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов авторизации сервера (сервер запоминает выданный ключ)
	const std::string challenge = server->header();
	// Проверяем что сервер выдал ключ
	ASSERT_NE(challenge.find("nonce=\""), std::string::npos);
	// Разбираем учётные данные с посторонним ключом сервера (воспроизведение со старым nonce)
	ASSERT_TRUE(server->parse("Digest username=\"login\", realm=\"anyks.com\", nonce=\"deadbeefdeadbeef\", uri=\"/api/resource\", qop=auth, nc=00000001, cnonce=\"abc123\", response=\"00000000000000000000000000000000\", algorithm=MD5"));
	// Проверяем что учётные данные с чужим ключом сервера отклонены
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки сверки ключа сервера при ручной установке nonce
 *
 */
TEST_F(AuthFixture, DigestManualNonceValidationTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Вручную устанавливаем ключ сервера (сервер фиксирует его как выданный)
	server->nonce("fixednonce123456");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем логин пользователя
	client->user("login");
	// Устанавливаем пароль пользователя
	client->pass("secret");
	// Устанавливаем параметры HTTP-запроса
	client->uri("/api/resource");
	// Разбираем вызов сервера с тем же ключом, что установлен вручную
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"fixednonce123456\", algorithm=MD5"));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Сервер разбирает учётные данные клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем что учётные данные с корректным ключом сервера приняты
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки полного цикла авторизации подписью HMAC (RFC 9421)
 *
 */
TEST_F(AuthFixture, HmacRoundTripTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему подписи HMAC с алгоритмом SHA-256
	client->type(type_t::HMAC, hash_t::SHA256);
	// Устанавливаем секретный ключ подписи
	client->key("shared-secret-key");
	// Устанавливаем идентификатор ключа подписи
	client->keyId("test-key");
	// Добавляем покрываемый подписью компонент метода запроса
	client->component("@method", "POST");
	// Добавляем покрываемый подписью компонент авторитета запроса
	client->component("@authority", "example.com");
	// Добавляем покрываемый подписью компонент пути запроса
	client->component("@path", "/foo");
	// Контейнер для набора заголовков подписи
	std::vector <std::pair <std::string, std::string>> headers;
	// Формируем набор заголовков подписи
	client->headers(headers);
	// Проверяем что сформированы оба заголовка подписи
	ASSERT_EQ(headers.size(), 2u);
	// Проверяем что первый заголовок является Signature-Input
	ASSERT_EQ(headers.at(0).first, "Signature-Input");
	// Проверяем что второй заголовок является Signature
	ASSERT_EQ(headers.at(1).first, "Signature");
	// Проверяем что заголовок Signature-Input содержит идентификатор ключа
	ASSERT_NE(headers.at(0).second.find("keyid=\"test-key\""), std::string::npos);
	// Проверяем что заголовок Signature-Input содержит имя алгоритма подписи
	ASSERT_NE(headers.at(0).second.find("alg=\"hmac-sha256\""), std::string::npos);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему подписи HMAC с алгоритмом SHA-256
	server->type(type_t::HMAC, hash_t::SHA256);
	// Восстанавливаем значения покрываемых компонентов из принятого запроса
	server->component("@method", "POST");
	// Восстанавливаем значение компонента авторитета запроса
	server->component("@authority", "example.com");
	// Восстанавливаем значение компонента пути запроса
	server->component("@path", "/foo");
	// Регистрируем функцию извлечения секретного ключа по идентификатору
	server->callbackExtractKey([](const std::string & keyId) -> std::string {
		// Возвращаем секретный ключ только для известного идентификатора
		return (keyId == "test-key" ? std::string("shared-secret-key") : std::string(""));
	});
	// Разбираем заголовок параметров подписи
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	// Разбираем заголовок значения подписи
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	// Проверяем успешную проверку подписи
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения подписи HMAC при неверном ключе
 *
 */
TEST_F(AuthFixture, HmacWrongKeyTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему подписи HMAC с алгоритмом SHA-256
	client->type(type_t::HMAC, hash_t::SHA256);
	// Устанавливаем секретный ключ подписи
	client->key("client-secret-key");
	// Устанавливаем идентификатор ключа подписи
	client->keyId("test-key");
	// Добавляем покрываемый подписью компонент метода запроса
	client->component("@method", "GET");
	// Добавляем покрываемый подписью компонент пути запроса
	client->component("@path", "/data");
	// Контейнер для набора заголовков подписи
	std::vector <std::pair <std::string, std::string>> headers;
	// Формируем набор заголовков подписи
	client->headers(headers);
	// Проверяем что сформированы оба заголовка подписи
	ASSERT_EQ(headers.size(), 2u);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему подписи HMAC с алгоритмом SHA-256
	server->type(type_t::HMAC, hash_t::SHA256);
	// Восстанавливаем значения покрываемых компонентов из принятого запроса
	server->component("@method", "GET");
	// Восстанавливаем значение компонента пути запроса
	server->component("@path", "/data");
	// Регистрируем функцию извлечения секретного ключа (ключ на сервере отличается)
	server->callbackExtractKey([](const std::string &) -> std::string {
		// Возвращаем ключ, отличный от клиентского
		return "server-secret-key";
	});
	// Разбираем заголовок параметров подписи
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	// Разбираем заголовок значения подписи
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	// Проверяем что подпись с неверным ключом отклонена
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки отклонения подписи HMAC при подмене компонента запроса
 *
 */
TEST_F(AuthFixture, HmacTamperedComponentTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему подписи HMAC с алгоритмом SHA-256
	client->type(type_t::HMAC, hash_t::SHA256);
	// Устанавливаем секретный ключ подписи
	client->key("shared-secret-key");
	// Устанавливаем идентификатор ключа подписи
	client->keyId("test-key");
	// Добавляем покрываемый подписью компонент метода запроса
	client->component("@method", "GET");
	// Добавляем покрываемый подписью компонент пути запроса
	client->component("@path", "/private");
	// Контейнер для набора заголовков подписи
	std::vector <std::pair <std::string, std::string>> headers;
	// Формируем набор заголовков подписи
	client->headers(headers);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему подписи HMAC с алгоритмом SHA-256
	server->type(type_t::HMAC, hash_t::SHA256);
	// Восстанавливаем значение компонента метода запроса
	server->component("@method", "GET");
	// Восстанавливаем ПОДМЕНЁННОЕ значение компонента пути запроса
	server->component("@path", "/public");
	// Регистрируем функцию извлечения секретного ключа (ключ совпадает с клиентским)
	server->callbackExtractKey([](const std::string &) -> std::string {
		// Возвращаем корректный секретный ключ
		return "shared-secret-key";
	});
	// Разбираем заголовок параметров подписи
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	// Разбираем заголовок значения подписи
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	// Проверяем что подпись при подмене компонента запроса отклонена
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки, что сервер не формирует заголовки подписи HMAC
 *
 */
TEST_F(AuthFixture, HmacServerNoHeadersTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему подписи HMAC
	server->type(type_t::HMAC, hash_t::SHA256);
	// Устанавливаем секретный ключ подписи
	server->key("shared-secret-key");
	// Добавляем покрываемый подписью компонент метода запроса
	server->component("@method", "GET");
	// Контейнер для набора заголовков подписи
	std::vector <std::pair <std::string, std::string>> headers;
	// Пытаемся сформировать набор заголовков подписи на стороне сервера
	server->headers(headers);
	// Проверяем что сервер не формирует заголовки подписи
	ASSERT_TRUE(headers.empty());
}

/**
 * @brief Метод проверки имён заголовков при работе через прокси
 *
 */
TEST_F(AuthFixture, ProxyHeaderNamesTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему BASIC-авторизации
	client->type(type_t::BASIC);
	// Включаем режим работы через прокси
	client->proxy(true);
	// Устанавливаем логин пользователя
	client->user("user");
	// Устанавливаем пароль пользователя
	client->pass("pass");
	// Формируем заголовок учётных данных вместе с его именем
	const std::string header = client->header(true);
	// Проверяем что имя заголовка соответствует прокси-авторизации
	ASSERT_EQ(header.compare(0, 20, "Proxy-Authorization:"), 0);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BASIC-авторизации
	server->type(type_t::BASIC);
	// Включаем режим работы через прокси
	server->proxy(true);
	// Устанавливаем название сервера
	server->realm("proxy.anyks.com");
	// Формируем вызов авторизации вместе с его именем
	const std::string challenge = server->header(true);
	// Проверяем что имя заголовка соответствует прокси-вызову
	ASSERT_EQ(challenge.compare(0, 18, "Proxy-Authenticate"), 0);
}

/**
 * @brief Метод проверки неудачного разбора некорректных заголовков
 *
 */
TEST_F(AuthFixture, ParseInvalidHeadersTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BASIC-авторизации
	server->type(type_t::BASIC);
	// Проверяем что разбор пустого заголовка неуспешен
	ASSERT_FALSE(server->parse(""));
	// Проверяем что разбор заголовка чужой схемы неуспешен
	ASSERT_FALSE(server->parse("Bearer some-token"));
	// Проверяем что разбор заголовка без полезной нагрузки неуспешен
	ASSERT_FALSE(server->parse("Basic"));
	// Переключаемся на схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Проверяем что разбор заголовка без параметров DIGEST неуспешен
	ASSERT_FALSE(server->parse("Digest"));
}

/**
 * @brief Метод проверки смены схемы авторизации на одном объекте
 *
 */
TEST_F(AuthFixture, SwitchSchemeTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем токен доступа
	client->token("token-123");
	// Устанавливаем логин пользователя
	client->user("user");
	// Устанавливаем пароль пользователя
	client->pass("pass");
	// Выбираем схему BEARER-авторизации
	client->type(type_t::BEARER);
	// Проверяем что формируется заголовок схемы BEARER
	ASSERT_EQ(client->header(), "Bearer token-123");
	// Переключаемся на схему BASIC-авторизации на том же объекте
	client->type(type_t::BASIC);
	// Проверяем что формируется заголовок схемы BASIC
	ASSERT_EQ(client->header().compare(0, 6, "Basic "), 0);
}
