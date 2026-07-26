/**
 * @file: parameterized.cpp
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
 * @brief Структура параметров теста алгоритмов DIGEST-авторизации
 *
 */
struct DigestAlgorithmParameter {
	// Алгоритм хэширования
	hash_t hash;
	// Флаг использования сессионного режима алгоритма (-sess)
	bool sess;
	// Ожидаемое имя алгоритма в заголовке
	std::string name;
};

/**
 * @brief Класс фикстуры теста алгоритмов DIGEST-авторизации
 *
 */
class DigestAlgorithmParameterizedFixture : public AuthFixture, public ::testing::WithParamInterface <DigestAlgorithmParameter> {
	public:
		// Параметры теста алгоритмов DIGEST-авторизации
		DigestAlgorithmParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования полного цикла DIGEST-авторизации для разных алгоритмов
 *
 */
TEST_P(DigestAlgorithmParameterizedFixture, DigestAlgorithmRoundTripTest){
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему DIGEST-авторизации с проверяемым алгоритмом
	server->type(type_t::DIGEST, this->_parameter.hash);
	// Устанавливаем сессионный режим алгоритма согласно параметрам теста
	server->session(this->_parameter.sess);
	// Устанавливаем название сервера
	server->realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server->callbackExtractPass([](const std::string & user) -> std::string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? std::string("secret") : std::string(""));
	});
	// Формируем вызов авторизации сервера
	const std::string challenge = server->header();
	// Проверяем что вызов содержит ожидаемое имя алгоритма
	ASSERT_NE(challenge.find(this->_parameter.name), std::string::npos);
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему DIGEST-авторизации с проверяемым алгоритмом
	client->type(type_t::DIGEST, this->_parameter.hash);
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
	// Проверяем что учётные данные содержат ожидаемое имя алгоритма
	ASSERT_NE(credentials.find(this->_parameter.name), std::string::npos);
	// Сервер разбирает учётные данные клиента
	ASSERT_TRUE(server->parse(credentials));
	// Проверяем успешную авторизацию для проверяемого алгоритма
	ASSERT_TRUE(server->check());
}

/**
 * @brief Инициализация параметров теста алгоритмов DIGEST-авторизации
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, DigestAlgorithmParameterizedFixture,
	::testing::Values(
		DigestAlgorithmParameter({hash_t::MD5, false, "algorithm=MD5"}),
		DigestAlgorithmParameter({hash_t::SHA1, false, "algorithm=SHA1"}),
		DigestAlgorithmParameter({hash_t::SHA224, false, "algorithm=SHA-224"}),
		DigestAlgorithmParameter({hash_t::SHA256, false, "algorithm=SHA-256"}),
		DigestAlgorithmParameter({hash_t::SHA384, false, "algorithm=SHA-384"}),
		DigestAlgorithmParameter({hash_t::SHA512, false, "algorithm=SHA-512"}),
		DigestAlgorithmParameter({hash_t::MD5, true, "algorithm=MD5-sess"}),
		DigestAlgorithmParameter({hash_t::SHA256, true, "algorithm=SHA-256-sess"}),
		DigestAlgorithmParameter({hash_t::SHA512, true, "algorithm=SHA-512-sess"})
	)
);

/**
 * @brief Структура параметров теста алгоритмов авторизации подписью HMAC
 *
 */
struct HmacAlgorithmParameter {
	// Алгоритм хэширования
	hash_t hash;
	// Ожидаемое имя алгоритма подписи
	std::string name;
};

/**
 * @brief Класс фикстуры теста алгоритмов авторизации подписью HMAC
 *
 */
class HmacAlgorithmParameterizedFixture : public AuthFixture, public ::testing::WithParamInterface <HmacAlgorithmParameter> {
	public:
		// Параметры теста алгоритмов авторизации подписью HMAC
		HmacAlgorithmParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования полного цикла HMAC-подписи для разных алгоритмов
 *
 */
TEST_P(HmacAlgorithmParameterizedFixture, HmacAlgorithmRoundTripTest){
	// Создаём модуль авторизации на стороне клиента
	std::unique_ptr <auth_t> client = this->make(owner_t::CLIENT);
	// Устанавливаем схему подписи HMAC с проверяемым алгоритмом
	client->type(type_t::HMAC, this->_parameter.hash);
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
	// Проверяем что заголовок Signature-Input содержит ожидаемое имя алгоритма
	ASSERT_NE(headers.at(0).second.find(this->_parameter.name), std::string::npos);
	// Создаём модуль авторизации на стороне сервера
	std::unique_ptr <auth_t> server = this->make(owner_t::SERVER);
	// Устанавливаем схему подписи HMAC с проверяемым алгоритмом
	server->type(type_t::HMAC, this->_parameter.hash);
	// Восстанавливаем значение компонента метода запроса
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
	// Проверяем успешную проверку подписи для проверяемого алгоритма
	ASSERT_TRUE(server->check());
}

/**
 * @brief Инициализация параметров теста алгоритмов авторизации подписью HMAC
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, HmacAlgorithmParameterizedFixture,
	::testing::Values(
		HmacAlgorithmParameter({hash_t::MD5, "alg=\"hmac-md5\""}),
		HmacAlgorithmParameter({hash_t::SHA1, "alg=\"hmac-sha1\""}),
		HmacAlgorithmParameter({hash_t::SHA224, "alg=\"hmac-sha224\""}),
		HmacAlgorithmParameter({hash_t::SHA256, "alg=\"hmac-sha256\""}),
		HmacAlgorithmParameter({hash_t::SHA384, "alg=\"hmac-sha384\""}),
		HmacAlgorithmParameter({hash_t::SHA512, "alg=\"hmac-sha512\""})
	)
);
