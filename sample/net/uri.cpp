/**
 * @file: uri.cpp
 * @date: 2026-03-29
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
 * Подключаем заголовочный файл проекта
 */
#include <net/uri.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

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
	// Выполняем создание объекта URI
	uri_t uri(&fmk, &log);
	// Генерируем ETag для строки "Hello, World!" и выводим его
	cout << " ETag: " << "W/" << uri.etag("Hello, World!", 8) << endl;

	cout << endl << endl;

	string address = "http://www.example.com/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://localhost/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://localhost:80/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://127.0.0.1/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://127.0.0.1:80/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user:pass@www.example.com:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user:pass@www.example.com/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user@www.example.com:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user@www.example.com/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "www.example.com/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "www.example.com:443/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user:pass@localhost:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user:pass@localhost/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user@localhost:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user@localhost/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "localhost/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "localhost:443/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user:pass@127.0.0.1:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user:pass@127.0.0.1/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user@127.0.0.1:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user@127.0.0.1/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "127.0.0.1/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "127.0.0.1:443/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "https://user:pass@[2001:db8::1]:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "mailto:user@example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "user@example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "ftp://ftp.example.com/file.txt";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://example.com:80";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://example.com/";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://example.com:80/";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://example.com/?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "http://example.com:80/?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << endl << endl;

	address = "scheme:path-only";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	// Выводим результат
	return EXIT_SUCCESS;
}
