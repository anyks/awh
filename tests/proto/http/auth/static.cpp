/**
 * @file: static.cpp
 * @date: 2026-07-14
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

/**
 * @brief Метод проверки Digest для двух пользователей с одним nonce и nc=00000001
 *
 */
TEST_F(AuthFixture, DigestMultiUserSameNonceTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		// Возвращаем пароль только для известных логинов
		if(user == "alice")
			return "secret-a";
		if(user == "bob")
			return "secret-b";
		return "";
	});
	// Формируем общий вызов авторизации сервера
	const std::string challenge = server->header();
	// Создаём клиента alice
	std::unique_ptr <auth_t> alice = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	alice->type(type_t::DIGEST);
	// Устанавливаем учётные данные alice
	alice->user("alice");
	alice->pass("secret-a");
	alice->uri("/api/resource");
	// Разбираем вызов сервера
	ASSERT_TRUE(alice->parse(challenge));
	// Формируем учётные данные alice
	const std::string aliceCredentials = alice->header();
	// Создаём клиента bob
	std::unique_ptr <auth_t> bob = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	bob->type(type_t::DIGEST);
	// Устанавливаем учётные данные bob
	bob->user("bob");
	bob->pass("secret-b");
	bob->uri("/api/resource");
	// Разбираем тот же вызов сервера
	ASSERT_TRUE(bob->parse(challenge));
	// Формируем учётные данные bob
	const std::string bobCredentials = bob->header();
	// Оба клиента должны начинать с nc=00000001
	ASSERT_NE(aliceCredentials.find("nc=00000001"), std::string::npos);
	ASSERT_NE(bobCredentials.find("nc=00000001"), std::string::npos);
	// Сервер принимает alice
	ASSERT_TRUE(server->parse(aliceCredentials));
	ASSERT_TRUE(server->check());
	// Сервер принимает bob с тем же nonce и тем же nc
	ASSERT_TRUE(server->parse(bobCredentials));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки legacy Digest без параметра qop (RFC 2069)
 *
 */
TEST_F(AuthFixture, DigestLegacyNoQopTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("legacy.example");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем учётные данные клиента
	client->user("login");
	client->pass("secret");
	client->uri("/legacy");
	// Разбираем legacy-вызов без qop
	ASSERT_TRUE(client->parse("Digest realm=\"legacy.example\", nonce=\"legacy-nonce\", algorithm=MD5"));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Legacy-ответ не должен содержать nc=
	ASSERT_EQ(credentials.find("nc="), std::string::npos);
	// Сервер вручную принимает legacy-параметры
	server->nonce("legacy-nonce");
	ASSERT_TRUE(server->parse(credentials));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки Digest с realm, содержащим запятую
 *
 */
TEST_F(AuthFixture, DigestRealmWithCommaTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем учётные данные клиента
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	// Разбираем вызов с запятой внутри realm
	ASSERT_TRUE(client->parse("Digest realm=\"foo, bar\", qop=\"auth\", nonce=\"comma-nonce\", algorithm=MD5"));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Проверяем что realm с запятой сохранён
	ASSERT_NE(credentials.find("realm=\"foo, bar\""), std::string::npos);
}

/**
 * @brief Метод проверки отклонения заголовка с ложным префиксом схемы
 *
 */
TEST_F(AuthFixture, ParseInvalidSchemePrefixTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему BASIC-авторизации
	server->type(type_t::BASIC);
	// Проверяем что ложный префикс NotBasic отклоняется
	ASSERT_FALSE(server->parse("NotBasic dXNlcjpwYXNz"));
	// Переключаемся на BEARER
	server->type(type_t::BEARER);
	// Проверяем что ложный префикс NotBearer отклоняется
	ASSERT_FALSE(server->parse("NotBearer token"));
}

/**
 * @brief Метод проверки отклонения Digest с неверным opaque
 *
 */
TEST_F(AuthFixture, DigestOpaqueMismatchTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов авторизации сервера
	const std::string challenge = server->header();
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем учётные данные клиента
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	// Разбираем вызов сервера
	ASSERT_TRUE(client->parse(challenge));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Подменяем opaque в учётных данных
	std::string tampered = credentials;
	const std::string badOpaque = "opaque=\"wrong-opaque-value\"";
	const size_t pos = tampered.find("opaque=\"");
	ASSERT_NE(pos, std::string::npos);
	const size_t end = tampered.find('"', pos + 8);
	ASSERT_NE(end, std::string::npos);
	tampered.replace(pos, (end - pos + 1), badOpaque);
	// Сервер отклоняет учётные данные с чужим opaque
	ASSERT_TRUE(server->parse(tampered));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки полного цикла Digest с qop=auth-int
 *
 */
TEST_F(AuthFixture, DigestAuthIntRoundTripTest){
	// Тело запроса для auth-int
	const std::string body = "{\"status\":\"ok\"}";
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST, hash_t::SHA256);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Устанавливаем тело запроса на сервере
	server->entity(body);
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов с auth-int
	std::string challenge = server->header();
	// Подменяем qop на auth-int в вызове сервера
	const size_t qopPos = challenge.find("qop=\"auth\"");
	ASSERT_NE(qopPos, std::string::npos);
	challenge.replace(qopPos, 10, "qop=\"auth-int\"");
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST, hash_t::SHA256);
	// Устанавливаем учётные данные и тело запроса
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	client->entity(body);
	// Разбираем вызов сервера
	ASSERT_TRUE(client->parse(challenge));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Сервер проверяет auth-int учётные данные
	ASSERT_TRUE(server->parse(credentials));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения просроченной HMAC-подписи
 *
 */
TEST_F(AuthFixture, HmacExpiredSignatureTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему подписи HMAC
	client->type(type_t::HMAC, hash_t::SHA256);
	// Устанавливаем секретный ключ подписи
	client->key("shared-secret-key");
	// Устанавливаем идентификатор ключа подписи
	client->keyId("test-key");
	// Добавляем покрываемый компонент
	client->component("@method", "GET");
	client->component("@path", "/data");
	// Контейнер для набора заголовков подписи
	std::vector <std::pair <std::string, std::string>> headers;
	// Формируем набор заголовков подписи
	client->headers(headers);
	ASSERT_EQ(headers.size(), 2u);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему подписи HMAC
	server->type(type_t::HMAC, hash_t::SHA256);
	// Восстанавливаем компоненты запроса
	server->component("@method", "GET");
	server->component("@path", "/data");
	// Регистрируем функцию извлечения секретного ключа
	server->callbackExtractKey([](const std::string &) -> std::string {
		return "shared-secret-key";
	});
	// Разбираем заголовки подписи
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	// Подменяем expires на прошедшее время
	std::string input = headers.at(0).second;
	const std::string expired = ";expires=1";
	if(input.find(";expires=") == std::string::npos)
		input.append(expired);
	else {
		const size_t pos = input.find(";expires=");
		const size_t end = input.find(';', pos + 1);
		if(end == std::string::npos)
			input.replace(pos, input.size() - pos, expired);
		else input.replace(pos, end - pos, expired);
	}
	ASSERT_TRUE(server->parse("Signature-Input", input));
	// Просроченная подпись должна быть отклонена
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки обновления HMAC-компонента без дублирования
 *
 */
TEST_F(AuthFixture, HmacComponentDedupTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему подписи HMAC
	client->type(type_t::HMAC, hash_t::SHA256);
	// Устанавливаем секретный ключ подписи
	client->key("shared-secret-key");
	// Дважды добавляем один и тот же компонент с разными значениями
	client->component("@method", "GET");
	client->component("@method", "POST");
	// Контейнер для набора заголовков подписи
	std::vector <std::pair <std::string, std::string>> headers;
	// Формируем набор заголовков подписи
	client->headers(headers);
	ASSERT_EQ(headers.size(), 2u);
	// В Signature-Input должен быть только один @method
	ASSERT_NE(headers.at(0).second.find("\"@method\""), std::string::npos);
	const size_t first = headers.at(0).second.find("\"@method\"");
	const size_t second = headers.at(0).second.find("\"@method\"", first + 1);
	ASSERT_EQ(second, std::string::npos);
	// Сервер с POST должен принять подпись
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "POST");
	server->key("shared-secret-key");
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки Digest с параметрами в смешанном регистре
 *
 */
TEST_F(AuthFixture, DigestMixedCaseParamsTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем учётные данные клиента
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	// Разбираем вызов с параметрами в нестандартном регистре
	ASSERT_TRUE(client->parse("Digest Realm=\"anyks.com\", Qop=\"auth\", Nonce=\"case-nonce\", algorithm=MD5, Username=\"login\""));
	// Формируем учётные данные клиента
	const std::string credentials = client->header();
	// Проверяем что логин и realm разобраны корректно
	ASSERT_NE(credentials.find("username=\"login\""), std::string::npos);
	ASSERT_NE(credentials.find("realm=\"anyks.com\""), std::string::npos);
}

/**
 * @brief Метод проверки отклонения Digest с некорректным nc
 *
 */
TEST_F(AuthFixture, DigestInvalidNcTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов авторизации сервера
	const std::string challenge = server->header();
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем учётные данные клиента
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	// Разбираем вызов сервера
	ASSERT_TRUE(client->parse(challenge));
	// Формируем корректные учётные данные
	std::string credentials = client->header();
	// Подменяем nc на некорректное значение
	const size_t pos = credentials.find("nc=");
	ASSERT_NE(pos, std::string::npos);
	credentials.replace(pos, 11, "nc=0000000G");
	// Сервер отклоняет учётные данные с некорректным nc
	ASSERT_TRUE(server->parse(credentials));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки сброса digest-состояния при смене type()
 *
 */
TEST_F(AuthFixture, DigestTypeSwitchClearsStateTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации
	server->type(type_t::DIGEST);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию изvлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов и принимаем первый запрос
	const std::string challenge = server->header();
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse(challenge));
	const std::string first = client->header();
	ASSERT_TRUE(server->parse(first));
	ASSERT_TRUE(server->check());
	// Переключаем схему и возвращаемся к DIGEST — replay-таблица должна сброситься
	server->type(type_t::BASIC);
	server->type(type_t::DIGEST);
	server->realm("anyks.com");
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	const std::string challenge2 = server->header();
	ASSERT_TRUE(client->parse(challenge2));
	const std::string replay = client->header();
	// Повтор nc=00000001 после сброса lncs должен быть принят
	ASSERT_TRUE(server->parse(replay));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки допуска clock skew для HMAC
 *
 */
TEST_F(AuthFixture, HmacClockSkewTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->keyId("test-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	// Подпись создаётся с created на 30 секунд в будущем
	const uint64_t future = this->_fmk->timestamp <uint64_t> (awh::fmk_t::chrono_t::SECONDS) + 30;
	client->signCreated(future);
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	ASSERT_EQ(headers.size(), 2u);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "GET");
	server->component("@path", "/data");
	server->callbackExtractKey([](const std::string &) -> std::string {
		return "shared-secret-key";
	});
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	// С допуском по умолчанию (60 с) подпись должна быть принята
	ASSERT_TRUE(server->check());
	// Без допуска подпись должна быть отклонена
	server->clockSkew(0);
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки защиты HMAC от повторного использования nonce
 *
 */
TEST_F(AuthFixture, HmacNonceReplayTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	client->signNonce("unique-nonce-42");
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	ASSERT_EQ(headers.size(), 2u);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "GET");
	server->component("@path", "/data");
	server->key("shared-secret-key");
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	// Первый запрос с nonce принимается
	ASSERT_TRUE(server->check());
	// Повторная проверка того же nonce отклоняется
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки сохранения digest.sess после reset()
 *
 */
TEST_F(AuthFixture, DigestResetPreservesSessionTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::DIGEST, hash_t::SHA256);
	server->session(true);
	server->realm("anyks.com");
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	const std::string challenge = server->header();
	ASSERT_NE(challenge.find("SHA-256-sess"), std::string::npos);
	// Создаём клиента, включаем -sess и сбрасываем временное состояние
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST, hash_t::SHA256);
	client->session(true);
	client->reset();
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse(challenge));
	const std::string credentials = client->header();
	ASSERT_NE(credentials.find("SHA-256-sess"), std::string::npos);
	ASSERT_TRUE(server->parse(credentials));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки очистки учётных данных при неудачном parse() на сервере
 *
 */
TEST_F(AuthFixture, ServerParseClearsStaleCredentialsTest){
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::BASIC);
	server->callbackCheckUser([](const std::string & user, const std::string &) -> bool {
		return (user == "alice");
	});
	ASSERT_TRUE(server->parse("Basic YWxpY2U6c2VjcmV0"));
	ASSERT_TRUE(server->check());
	ASSERT_FALSE(server->parse("Basic !!!invalid!!!"));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки отклонения Digest с nc=00000000
 *
 */
TEST_F(AuthFixture, DigestNcZeroRejectedTest){
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::DIGEST);
	server->realm("anyks.com");
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	const std::string challenge = server->header();
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse(challenge));
	std::string credentials = client->header();
	const size_t pos = credentials.find("nc=");
	ASSERT_NE(pos, std::string::npos);
	credentials.replace(pos, 11, "nc=00000000");
	ASSERT_TRUE(server->parse(credentials));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки отклонения HMAC без параметра created
 *
 */
TEST_F(AuthFixture, HmacMissingCreatedRejectedTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	ASSERT_EQ(headers.size(), 2u);
	std::string input = headers.at(0).second;
	const size_t createdPos = input.find(";created=");
	ASSERT_NE(createdPos, std::string::npos);
	const size_t nextSemi = input.find(';', createdPos + 1);
	input.erase(createdPos, (nextSemi == std::string::npos ? input.size() - createdPos : nextSemi - createdPos));
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "GET");
	server->component("@path", "/data");
	server->key("shared-secret-key");
	ASSERT_TRUE(server->parse("Signature-Input", input));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки разбора HMAC с параметрами в верхнем регистре
 *
 */
TEST_F(AuthFixture, HmacUppercaseParamKeysTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->keyId("test-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	std::string input = headers.at(0).second;
	input.replace(input.find("created="), 8, "Created=");
	input.replace(input.find("keyid="), 6, "KeyId=");
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "GET");
	server->component("@path", "/data");
	server->callbackExtractKey([](const std::string &) -> std::string {
		return "shared-secret-key";
	});
	ASSERT_TRUE(server->parse("Signature-Input", input));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения Digest с чужим realm на сервере
 *
 */
TEST_F(AuthFixture, DigestWrongRealmRejectedTest){
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::DIGEST);
	server->realm("anyks.com");
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	const std::string challenge = server->header();
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse(challenge));
	std::string credentials = client->header();
	const size_t pos = credentials.find("realm=\"anyks.com\"");
	ASSERT_NE(pos, std::string::npos);
	credentials.replace(pos, 17, "realm=\"evil.com\"");
	ASSERT_FALSE(server->parse(credentials));
}

/**
 * @brief Метод проверки переключения клиента с qop на legacy без qop
 *
 */
TEST_F(AuthFixture, DigestQopToLegacySwitchTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"nonce-qop\", algorithm=MD5"));
	const std::string withQop = client->header();
	ASSERT_NE(withQop.find("nc="), std::string::npos);
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", nonce=\"nonce-legacy\", algorithm=MD5"));
	const std::string legacy = client->header();
	ASSERT_EQ(legacy.find("nc="), std::string::npos);
	ASSERT_EQ(legacy.find("cnonce="), std::string::npos);
}

/**
 * @brief Метод проверки отклонения учётных данных без nonce после успешного запроса
 *
 */
TEST_F(AuthFixture, DigestMissingNonceAfterClearTest){
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::DIGEST);
	server->realm("anyks.com");
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	const std::string challenge = server->header();
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse(challenge));
	const std::string credentials = client->header();
	ASSERT_TRUE(server->parse(credentials));
	ASSERT_TRUE(server->check());
	// Учётные данные без nonce не должны проходить проверку
	std::string noNonce = credentials;
	const size_t pos = noNonce.find("nonce=\"");
	ASSERT_NE(pos, std::string::npos);
	const size_t end = noNonce.find('"', pos + 7);
	ASSERT_NE(end, std::string::npos);
	noNonce.erase(pos, end - pos + 1);
	ASSERT_TRUE(server->parse(noNonce));
	ASSERT_FALSE(server->check());
}

/**
 * @brief Метод проверки отклонения просроченной HMAC-подписи без expires
 *
 */
TEST_F(AuthFixture, HmacSignMaxAgeTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	const uint64_t now = this->_fmk->timestamp <uint64_t> (awh::fmk_t::chrono_t::SECONDS);
	client->signCreated(now - 600);
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "GET");
	server->component("@path", "/data");
	server->key("shared-secret-key");
	server->clockSkew(0);
	server->signMaxAge(300);
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	ASSERT_FALSE(server->check());
	server->signMaxAge(0);
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(server->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения HMAC с несовпадающими метками заголовков
 *
 */
TEST_F(AuthFixture, HmacLabelMismatchTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	std::string signature = headers.at(1).second;
	const size_t pos = signature.find('=');
	ASSERT_NE(pos, std::string::npos);
	signature.replace(0, pos, "sig2");
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "GET");
	server->component("@path", "/data");
	server->key("shared-secret-key");
	ASSERT_TRUE(server->parse("Signature-Input", headers.at(0).second));
	ASSERT_FALSE(server->parse("Signature", signature));
}

/**
 * @brief Метод проверки сброса sess при переключении клиента с -sess на legacy без algorithm
 *
 */
TEST_F(AuthFixture, DigestSessToLegacySwitchTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST, hash_t::SHA256);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"nonce-sess\", algorithm=SHA-256-sess"));
	const std::string withSess = client->header();
	ASSERT_NE(withSess.find("SHA-256-sess"), std::string::npos);
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"nonce-legacy\""));
	const std::string legacy = client->header();
	ASSERT_EQ(legacy.find("-sess"), std::string::npos);
}

/**
 * @brief Метод проверки восстановления hash схемы на сервере при отсутствии algorithm в credentials
 *
 */
TEST_F(AuthFixture, DigestStaleHashAfterClearTest){
	const auto extractPass = [](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	};
	std::unique_ptr <auth_t> serverMd5 = this->make(owner_t::SERVER);
	serverMd5->type(type_t::DIGEST, hash_t::MD5);
	serverMd5->realm("anyks.com");
	serverMd5->callbackExtractPass(extractPass);
	const std::string challengeMd5 = serverMd5->header();
	std::unique_ptr <auth_t> clientMd5 = this->make(owner_t::CLIENT);
	clientMd5->type(type_t::DIGEST, hash_t::MD5);
	clientMd5->user("login");
	clientMd5->pass("secret");
	clientMd5->uri("/api/resource");
	ASSERT_TRUE(clientMd5->parse(challengeMd5));
	const std::string md5Credentials = clientMd5->header();
	std::unique_ptr <auth_t> serverSha256 = this->make(owner_t::SERVER);
	serverSha256->type(type_t::DIGEST, hash_t::SHA256);
	serverSha256->realm("anyks.com");
	serverSha256->callbackExtractPass(extractPass);
	ASSERT_TRUE(serverSha256->parse(md5Credentials));
	ASSERT_TRUE(serverSha256->check());
	std::string noAlgorithm = md5Credentials;
	const size_t pos = noAlgorithm.find(", algorithm=");
	ASSERT_NE(pos, std::string::npos);
	noAlgorithm.erase(pos);
	ASSERT_TRUE(serverSha256->parse(noAlgorithm));
	ASSERT_FALSE(serverSha256->check());
}

/**
 * @brief Метод проверки восстановления hash схемы на клиенте при отсутствии algorithm в challenge
 *
 */
TEST_F(AuthFixture, DigestClientStaleHashTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST, hash_t::SHA256);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"nonce-md5\", algorithm=MD5"));
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"nonce-sha256\""));
	const std::string credentials = client->header();
	ASSERT_NE(credentials.find("SHA-256"), std::string::npos);
	ASSERT_EQ(credentials.find("algorithm=MD5"), std::string::npos);
}

/**
 * @brief Метод проверки отклонения Digest-учётных данных без username и response на сервере
 *
 */
TEST_F(AuthFixture, DigestInvalidCredentialsRejectedTest){
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::DIGEST, hash_t::SHA256);
	server->realm("anyks.com");
	ASSERT_FALSE(server->parse("Digest foo=\"bar\", nonce=\"test-nonce\""));
}

/**
 * @brief Метод проверки двухшагового разбора HMAC через одноаргументный parse()
 *
 */
TEST_F(AuthFixture, HmacSingleArgParseTest){
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->keyId("test-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::HMAC, hash_t::SHA256);
	server->component("@method", "GET");
	server->component("@path", "/data");
	server->key("shared-secret-key");
	ASSERT_TRUE(server->parse(headers.at(0).second));
	ASSERT_TRUE(server->parse(headers.at(1).second));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки отклонения Digest legacy (без qop) в строгом режиме
 *
 */
TEST_F(AuthFixture, DigestStrictModeRejectsLegacyTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации
	client->type(type_t::DIGEST);
	// Устанавливаем учётные данные клиента
	client->user("login");
	client->pass("secret");
	client->uri("/legacy");
	// Разбираем legacy-вызов без qop
	ASSERT_TRUE(client->parse("Digest realm=\"legacy.example\", nonce=\"legacy-nonce\", algorithm=MD5"));
	// Формируем учётные данные клиента (legacy, без nc)
	const std::string credentials = client->header();
	ASSERT_EQ(credentials.find("nc="), std::string::npos);
	// Создаём модуль авторизации на стороне сервера в простом режиме
	std::unique_ptr <auth_t> simple = this->make(owner_t::SERVER);
	simple->type(type_t::DIGEST);
	simple->realm("legacy.example");
	simple->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	simple->nonce("legacy-nonce");
	// В простом режиме legacy-учётные данные принимаются
	ASSERT_TRUE(simple->parse(credentials));
	ASSERT_TRUE(simple->check());
	// Создаём модуль авторизации на стороне сервера в строгом режиме
	std::unique_ptr <auth_t> strict = this->make(owner_t::SERVER);
	strict->type(type_t::DIGEST);
	strict->mode(auth_t::mode_t::STRICT);
	strict->realm("legacy.example");
	strict->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	strict->nonce("legacy-nonce");
	// В строгом режиме legacy-учётные данные без qop отклоняются
	ASSERT_TRUE(strict->parse(credentials));
	ASSERT_FALSE(strict->check());
}

/**
 * @brief Метод проверки байт-точной сверки параметров HMAC в строгом режиме
 *
 */
TEST_F(AuthFixture, HmacStrictModeParamCaseTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему подписи HMAC с алгоритмом SHA-256
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->keyId("test-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	// Формируем набор заголовков подписи
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	// Подменяем ключи параметров на верхний регистр
	std::string input = headers.at(0).second;
	input.replace(input.find("created="), 8, "Created=");
	input.replace(input.find("keyid="), 6, "KeyId=");
	// Создаём функцию извлечения секретного ключа
	const auto extractKey = [](const std::string &) -> std::string {
		return "shared-secret-key";
	};
	// В простом режиме сверка регистронезависимая — подпись принимается
	std::unique_ptr <auth_t> simple = this->make(owner_t::SERVER);
	simple->type(type_t::HMAC, hash_t::SHA256);
	simple->component("@method", "GET");
	simple->component("@path", "/data");
	simple->callbackExtractKey(extractKey);
	ASSERT_TRUE(simple->parse("Signature-Input", input));
	ASSERT_TRUE(simple->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(simple->check());
	// В строгом режиме сверка байт-точная — параметры в верхнем регистре отклоняются
	std::unique_ptr <auth_t> strict = this->make(owner_t::SERVER);
	strict->type(type_t::HMAC, hash_t::SHA256);
	strict->mode(auth_t::mode_t::STRICT);
	strict->component("@method", "GET");
	strict->component("@path", "/data");
	strict->callbackExtractKey(extractKey);
	ASSERT_TRUE(strict->parse("Signature-Input", input));
	ASSERT_TRUE(strict->parse("Signature", headers.at(1).second));
	ASSERT_FALSE(strict->check());
	// В строгом режиме корректные (нижний регистр) параметры принимаются
	std::unique_ptr <auth_t> strictOk = this->make(owner_t::SERVER);
	strictOk->type(type_t::HMAC, hash_t::SHA256);
	strictOk->mode(auth_t::mode_t::STRICT);
	strictOk->component("@method", "GET");
	strictOk->component("@path", "/data");
	strictOk->callbackExtractKey(extractKey);
	ASSERT_TRUE(strictOk->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(strictOk->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(strictOk->check());
}

/**
 * @brief Метод проверки полного цикла Digest в строгом режиме (qop+cnonce+opaque)
 *
 */
TEST_F(AuthFixture, DigestStrictModeRoundTripTest){
	// Создаём модуль авторизации на стороне сервера в строгом режиме
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	server->type(type_t::DIGEST);
	server->mode(auth_t::mode_t::STRICT);
	server->realm("anyks.com");
	server->callbackExtractPass([](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Сервер формирует вызов (nonce + opaque)
	const std::string challenge = server->header();
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	ASSERT_TRUE(client->parse(challenge));
	const std::string credentials = client->header();
	// Строгий сервер принимает корректные учётные данные с qop, cnonce и opaque
	ASSERT_TRUE(server->parse(credentials));
	ASSERT_TRUE(server->check());
}

/**
 * @brief Метод проверки требования opaque в строгом режиме Digest
 *
 */
TEST_F(AuthFixture, DigestStrictModeRequiresOpaqueTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::DIGEST);
	client->user("login");
	client->pass("secret");
	client->uri("/api/resource");
	// Разбираем вызов с qop, но без opaque
	ASSERT_TRUE(client->parse("Digest realm=\"anyks.com\", qop=\"auth\", nonce=\"manual-nonce-abcd\", algorithm=MD5"));
	// Формируем учётные данные клиента
	std::string credentials = client->header();
	// Полностью удаляем поле opaque из учётных данных (opaque не участвует в расчёте response)
	const size_t op = credentials.find("opaque=");
	ASSERT_NE(op, std::string::npos);
	const size_t pre = credentials.rfind(", ", op);
	ASSERT_NE(pre, std::string::npos);
	const size_t opEnd = credentials.find(',', op);
	if(opEnd == std::string::npos)
		credentials.erase(pre);
	else credentials.erase(pre, opEnd - pre);
	// Убеждаемся, что поле opaque отсутствует
	ASSERT_EQ(credentials.find("opaque="), std::string::npos);
	// Функция извлечения пароля
	const auto extractPass = [](const std::string & user) -> std::string {
		return (user == "login" ? std::string("secret") : std::string(""));
	};
	// В простом режиме отсутствие opaque допускается
	std::unique_ptr <auth_t> simple = this->make(owner_t::SERVER);
	simple->type(type_t::DIGEST);
	simple->realm("anyks.com");
	simple->callbackExtractPass(extractPass);
	simple->nonce("manual-nonce-abcd");
	ASSERT_TRUE(simple->parse(credentials));
	ASSERT_TRUE(simple->check());
	// В строгом режиме отсутствие opaque отклоняется
	std::unique_ptr <auth_t> strict = this->make(owner_t::SERVER);
	strict->type(type_t::DIGEST);
	strict->mode(auth_t::mode_t::STRICT);
	strict->realm("anyks.com");
	strict->callbackExtractPass(extractPass);
	strict->nonce("manual-nonce-abcd");
	ASSERT_TRUE(strict->parse(credentials));
	ASSERT_FALSE(strict->check());
}

/**
 * @brief Метод проверки ограниченного срока жизни подписи HMAC по умолчанию в строгом режиме
 *
 */
TEST_F(AuthFixture, HmacStrictDefaultMaxAgeTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	client->type(type_t::HMAC, hash_t::SHA256);
	client->key("shared-secret-key");
	client->component("@method", "GET");
	client->component("@path", "/data");
	// Подпись создана 10 минут назад, без expires
	const uint64_t now = this->_fmk->timestamp <uint64_t> (awh::fmk_t::chrono_t::SECONDS);
	client->signCreated(now - 600);
	std::vector <std::pair <std::string, std::string>> headers;
	client->headers(headers);
	// Функция извлечения секретного ключа
	const auto extractKey = [](const std::string &) -> std::string {
		return "shared-secret-key";
	};
	// В простом режиме без signMaxAge подпись без expires принимается
	std::unique_ptr <auth_t> simple = this->make(owner_t::SERVER);
	simple->type(type_t::HMAC, hash_t::SHA256);
	simple->component("@method", "GET");
	simple->component("@path", "/data");
	simple->callbackExtractKey(extractKey);
	ASSERT_TRUE(simple->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(simple->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(simple->check());
	// В строгом режиме применяется ограниченный срок жизни по умолчанию — старая подпись отклоняется
	std::unique_ptr <auth_t> strict = this->make(owner_t::SERVER);
	strict->type(type_t::HMAC, hash_t::SHA256);
	strict->mode(auth_t::mode_t::STRICT);
	strict->component("@method", "GET");
	strict->component("@path", "/data");
	strict->callbackExtractKey(extractKey);
	ASSERT_TRUE(strict->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(strict->parse("Signature", headers.at(1).second));
	ASSERT_FALSE(strict->check());
	// С увеличенным настраиваемым лимитом строгого режима та же подпись принимается
	std::unique_ptr <auth_t> relaxed = this->make(owner_t::SERVER);
	relaxed->type(type_t::HMAC, hash_t::SHA256);
	relaxed->mode(auth_t::mode_t::STRICT);
	relaxed->signStrictMaxAge(1200);
	relaxed->component("@method", "GET");
	relaxed->component("@path", "/data");
	relaxed->callbackExtractKey(extractKey);
	ASSERT_EQ(relaxed->signStrictMaxAge(), static_cast <uint64_t> (1200));
	ASSERT_TRUE(relaxed->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(relaxed->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(relaxed->check());
	// С отключённым лимитом строгого режима (0) старая подпись без expires также принимается
	std::unique_ptr <auth_t> unlimited = this->make(owner_t::SERVER);
	unlimited->type(type_t::HMAC, hash_t::SHA256);
	unlimited->mode(auth_t::mode_t::STRICT);
	unlimited->signStrictMaxAge(0);
	unlimited->component("@method", "GET");
	unlimited->component("@path", "/data");
	unlimited->callbackExtractKey(extractKey);
	ASSERT_TRUE(unlimited->parse("Signature-Input", headers.at(0).second));
	ASSERT_TRUE(unlimited->parse("Signature", headers.at(1).second));
	ASSERT_TRUE(unlimited->check());
}
