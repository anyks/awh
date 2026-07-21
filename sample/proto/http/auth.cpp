/**
 * @file: auth.cpp
 * @date: 2026-07-15
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
 * Стандартные модули
 */
#include <string>
#include <vector>
#include <utility>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <proto/http/auth/auth.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;
/**
 * Используем пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Демонстрация схемы BASIC-авторизации (клиент/сервер)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleBasic(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== BASIC ======== " << endl;
	// Создаём модуль авторизации на стороне клиента
	auth_t client(auth_t::owner_t::CLIENT, fmk, log);
	// Выбираем схему BASIC-авторизации
	client.type(auth_t::type_t::BASIC);
	// Устанавливаем логин пользователя
	client.user("Aladdin");
	// Устанавливаем пароль пользователя
	client.pass("open sesame");
	// Формируем учётные данные клиента (заголовок Authorization)
	const string credentials = client.header();
	// Выводим сформированные учётные данные клиента
	cout << "Client Authorization: " << credentials << endl;
	// Создаём модуль авторизации на стороне сервера
	auth_t server(auth_t::owner_t::SERVER, fmk, log);
	// Выбираем схему BASIC-авторизации
	server.type(auth_t::type_t::BASIC);
	// Регистрируем функцию проверки пары «логин/пароль»
	server.callbackCheckUser([](const string & user, const string & pass) -> bool {
		// Подтверждаем корректность только для известной пары
		return ((user == "Aladdin") && (pass == "open sesame"));
	});
	// Сервер разбирает учётные данные клиента
	server.parse(credentials);
	// Выводим результат проверки учётных данных
	cout << "Server check: " << (server.check() ? "OK" : "FAIL") << endl << endl;
}
/**
 * @brief Демонстрация схемы BEARER-авторизации (клиент/сервер)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleBearer(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== BEARER ======== " << endl;
	// Создаём модуль авторизации на стороне клиента
	auth_t client(auth_t::owner_t::CLIENT, fmk, log);
	// Выбираем схему BEARER-авторизации
	client.type(auth_t::type_t::BEARER);
	// Устанавливаем токен доступа
	client.token("mF_9.B5f-4.1JqM");
	// Формируем учётные данные клиента (заголовок Authorization)
	const string credentials = client.header();
	// Выводим сформированные учётные данные клиента
	cout << "Client Authorization: " << credentials << endl;
	// Создаём модуль авторизации на стороне сервера
	auth_t server(auth_t::owner_t::SERVER, fmk, log);
	// Выбираем схему BEARER-авторизации
	server.type(auth_t::type_t::BEARER);
	// Регистрируем функцию проверки токена доступа
	server.callbackCheckToken([](const string & token) -> bool {
		// Подтверждаем корректность только для известного токена
		return (token == "mF_9.B5f-4.1JqM");
	});
	// Сервер разбирает токен доступа клиента
	server.parse(credentials);
	// Выводим результат проверки токена доступа
	cout << "Server check: " << (server.check() ? "OK" : "FAIL") << endl << endl;
}
/**
 * @brief Демонстрация схемы DIGEST-авторизации (клиент/сервер)
 *
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 * @param hash алгоритм хэширования
 * @param name название алгоритма для вывода
 * @param sess флаг сессионного режима алгоритма (-sess)
 */
static void sampleDigest(const fmk_t * fmk, const log_t * log, const auth_t::hash_t hash, const string & name, const bool sess) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== DIGEST " << name << (sess ? "-sess" : "") << " ======== " << endl;
	// Создаём модуль авторизации на стороне сервера
	auth_t server(auth_t::owner_t::SERVER, fmk, log);
	// Выбираем схему DIGEST-авторизации с указанным алгоритмом
	server.type(auth_t::type_t::DIGEST, hash);
	// Устанавливаем сессионный режим алгоритма при необходимости
	server.session(sess);
	// Устанавливаем название сервера (realm)
	server.realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server.callbackExtractPass([](const string & user) -> string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? string("secret") : string(""));
	});
	// Формируем вызов авторизации сервера (заголовок WWW-Authenticate)
	const string challenge = server.header();
	// Выводим сформированный вызов сервера
	cout << "Server WWW-Authenticate: " << challenge << endl;
	// Создаём модуль авторизации на стороне клиента
	auth_t client(auth_t::owner_t::CLIENT, fmk, log);
	// Выбираем схему DIGEST-авторизации с указанным алгоритмом
	client.type(auth_t::type_t::DIGEST, hash);
	// Устанавливаем логин пользователя
	client.user("login");
	// Устанавливаем пароль пользователя
	client.pass("secret");
	// Устанавливаем параметры HTTP-запроса
	client.uri("/api/resource");
	// Разбираем вызов сервера
	client.parse(challenge);
	// Формируем учётные данные клиента (заголовок Authorization)
	const string credentials = client.header();
	// Выводим сформированные учётные данные клиента
	cout << "Client Authorization: " << credentials << endl;
	// Сервер разбирает учётные данные клиента
	server.parse(credentials);
	// Выводим результат проверки учётных данных
	cout << "Server check: " << (server.check() ? "OK" : "FAIL") << endl << endl;
}
/**
 * @brief Демонстрация защиты DIGEST от повторного воспроизведения (replay)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleDigestReplay(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== DIGEST REPLAY PROTECTION ======== " << endl;
	// Создаём модуль авторизации на стороне сервера
	auth_t server(auth_t::owner_t::SERVER, fmk, log);
	// Выбираем схему DIGEST-авторизации
	server.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
	// Устанавливаем название сервера (realm)
	server.realm("anyks.com");
	// Регистрируем функцию извлечения пароля по логину
	server.callbackExtractPass([](const string & user) -> string {
		// Возвращаем пароль только для известного логина
		return (user == "login" ? string("secret") : string(""));
	});
	// Формируем вызов авторизации сервера
	const string challenge = server.header();
	// Создаём модуль авторизации на стороне клиента
	auth_t client(auth_t::owner_t::CLIENT, fmk, log);
	// Выбираем схему DIGEST-авторизации
	client.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
	// Устанавливаем логин пользователя
	client.user("login");
	// Устанавливаем пароль пользователя
	client.pass("secret");
	// Устанавливаем параметры HTTP-запроса
	client.uri("/api/resource");
	// Разбираем вызов сервера
	client.parse(challenge);
	// Формируем учётные данные первого запроса (nc=00000001)
	const string first = client.header();
	// Формируем учётные данные второго запроса (nc=00000002)
	const string second = client.header();
	// Сервер разбирает и проверяет первый запрос
	server.parse(first);
	// Выводим результат проверки первого запроса
	cout << "Request #1 (nc=00000001): " << (server.check() ? "OK" : "FAIL") << endl;
	// Сервер разбирает и проверяет второй запрос
	server.parse(second);
	// Выводим результат проверки второго запроса
	cout << "Request #2 (nc=00000002): " << (server.check() ? "OK" : "FAIL") << endl;
	// Поясняем, что предупреждение в логе на этом шаге — ожидаемое поведение защиты
	cout << "Request #1 replay (expecting rejection, warning below is normal): " << endl;
	// Сервер повторно разбирает первый запрос (атака повторного воспроизведения)
	server.parse(first);
	// Выводим результат проверки повторного запроса (ожидается отказ)
	cout << (server.check() ? "OK" : "REJECTED") << endl << endl;
}
/**
 * @brief Демонстрация авторизации подписью HMAC (RFC 9421, клиент/сервер)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleHmac(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== HMAC (RFC 9421) ======== " << endl;
	// Создаём модуль авторизации на стороне клиента
	auth_t client(auth_t::owner_t::CLIENT, fmk, log);
	// Выбираем схему подписи HMAC с алгоритмом SHA-256
	client.type(auth_t::type_t::HMAC, auth_t::hash_t::SHA256);
	// Устанавливаем секретный ключ подписи
	client.key("shared-secret-key");
	// Устанавливаем идентификатор ключа подписи
	client.keyId("test-key");
	// Добавляем покрываемый подписью компонент метода запроса
	client.component("@method", "POST");
	// Добавляем покрываемый подписью компонент авторитета запроса
	client.component("@authority", "example.com");
	// Добавляем покрываемый подписью компонент пути запроса
	client.component("@path", "/foo");
	// Контейнер для набора заголовков подписи
	vector <pair <string, string>> headers;
	// Формируем набор заголовков подписи (Signature-Input и Signature)
	client.headers(headers);
	/**
	 * Выводим сформированные заголовки подписи
	 */
	for(auto & header : headers)
		// Выводим название и значение заголовка подписи
		cout << "Client " << header.first << ": " << header.second << endl;
	// Создаём модуль авторизации на стороне сервера
	auth_t server(auth_t::owner_t::SERVER, fmk, log);
	// Выбираем схему подписи HMAC с алгоритмом SHA-256
	server.type(auth_t::type_t::HMAC, auth_t::hash_t::SHA256);
	// Восстанавливаем значения покрываемых компонентов из принятого запроса
	server.component("@method", "POST");
	// Восстанавливаем значение компонента авторитета запроса
	server.component("@authority", "example.com");
	// Восстанавливаем значение компонента пути запроса
	server.component("@path", "/foo");
	// Регистрируем функцию извлечения секретного ключа по идентификатору
	server.callbackExtractKey([](const string & keyId) -> string {
		// Возвращаем секретный ключ только для известного идентификатора
		return (keyId == "test-key" ? string("shared-secret-key") : string(""));
	});
	/**
	 * Сервер разбирает заголовки подписи
	 */
	for(auto & header : headers)
		// Разбираем очередной заголовок подписи с указанием его имени
		server.parse(header.first, header.second);
	// Выводим результат проверки подписи
	cout << "Server check: " << (server.check() ? "OK" : "FAIL") << endl << endl;
}
/**
 * @brief Демонстрация авторизации через прокси (Proxy-Authorization)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleProxy(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== PROXY (BASIC) ======== " << endl;
	// Создаём модуль авторизации на стороне клиента
	auth_t client(auth_t::owner_t::CLIENT, fmk, log);
	// Выбираем схему BASIC-авторизации
	client.type(auth_t::type_t::BASIC);
	// Включаем режим работы через прокси
	client.proxy(true);
	// Устанавливаем логин пользователя
	client.user("user");
	// Устанавливаем пароль пользователя
	client.pass("pass");
	// Формируем заголовок учётных данных вместе с его именем
	cout << "Client " << client.header(true);
	// Создаём модуль авторизации на стороне сервера (прокси)
	auth_t server(auth_t::owner_t::SERVER, fmk, log);
	// Выбираем схему BASIC-авторизации
	server.type(auth_t::type_t::BASIC);
	// Включаем режим работы через прокси
	server.proxy(true);
	// Устанавливаем название сервера (realm)
	server.realm("proxy.anyks.com");
	// Формируем вызов авторизации вместе с его именем
	cout << "Server " << server.header(true) << endl;
}
/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Демонстрируем схему BASIC-авторизации
	sampleBasic(&fmk, &log);
	// Демонстрируем схему BEARER-авторизации
	sampleBearer(&fmk, &log);
	// Демонстрируем схему DIGEST-авторизации для алгоритма MD5
	sampleDigest(&fmk, &log, auth_t::hash_t::MD5, "MD5", false);
	// Демонстрируем схему DIGEST-авторизации для алгоритма SHA-256
	sampleDigest(&fmk, &log, auth_t::hash_t::SHA256, "SHA-256", false);
	// Демонстрируем схему DIGEST-авторизации для алгоритма SHA-512
	sampleDigest(&fmk, &log, auth_t::hash_t::SHA512, "SHA-512", false);
	// Демонстрируем схему DIGEST-авторизации в сессионном режиме (SHA-256-sess)
	sampleDigest(&fmk, &log, auth_t::hash_t::SHA256, "SHA-256", true);
	// Демонстрируем защиту DIGEST от повторного воспроизведения
	sampleDigestReplay(&fmk, &log);
	// Демонстрируем авторизацию подписью HMAC
	sampleHmac(&fmk, &log);
	// Демонстрируем авторизацию через прокси
	sampleProxy(&fmk, &log);
	// Возвращаем результат
	return EXIT_SUCCESS;
}
