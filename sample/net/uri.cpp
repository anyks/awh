/**
 * @file: uri.cpp
 * @date: 2026-03-29
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с универсальными идентификаторами ресурсов — демонстрация разбора URI на составные части,
 *        сборки адреса, нормализации пути и работы с параметрами запроса
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/uri.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
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
	// Устанавливаем функцию обратного вызова для генерации параметра URI (например, для генерации контрольной суммы)
	uri.callback([&fmk](const uri_t * uri) -> string {
		// Генерируем контрольную сумму для строки URI и возвращаем её в виде параметра "checksum"
		return fmk.format("%s=%s", "checksum", uri->etag(uri->print(uri_t::item_t::QUERY)).c_str());
	});

	cout << endl << endl;

	string address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "http://www.example.com/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://www.example.com?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", ORIGIN: " << uri.print(uri_t::item_t::ORIGIN) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://localhost/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "http://localhost:80/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://192.168.0.1/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://192.168.0.1:80/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://[2001:db8::1]/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://[2001:db8::1]:80/path/to/resource?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@www.example.com:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@www.example.com/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@www.example.com:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@www.example.com/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "www.example.com/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "www.example.com:443/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@localhost:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@localhost/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@localhost:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@localhost/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "localhost/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "localhost:443/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@192.168.0.1:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@192.168.0.1/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@192.168.0.1:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@192.168.0.1/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "192.168.0.1/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "192.168.0.1:443/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@[2001:db8::1]:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "https://user:pass@[2001:db8::1]/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@[2001:db8::1]:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user@[2001:db8::1]/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "[2001:db8::1]/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "[2001:db8::1]:443/api/v1/?query=1&id=123#frag";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "https://user:pass@[2001:db8::1]:8080/api/v1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "mailto:user@example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "user@example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "user:password@example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "mailto:user:password@example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "/relative/path?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "ftp://ftp.example.com/file.txt";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "http://example.com:80";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "http://example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "http://example.com/";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "example.com";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "example.com/";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://example.com/";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << endl;

	cout << endl << endl;

	address = "http://example.com:80/";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "http://example.com/?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "http://example.com:80/?query=1";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "scheme:path-only";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "unix:///var/run/socket.sock";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "file:///path/to/file.txt";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "file://c:/path/to/file.txt";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "file://c:\\path\\to\\file.txt";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "c:\\path\\to\\file.txt";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "socks5://rfbPbd:XcCuZH@127.0.0.1:8000";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "wss://stream.testnet.binance.vision:9443";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	address = "https://www.example.com/%D0%B3%D1%80%D0%B8%D0%B3%D0%BE%D1%80%D0%B8%D0%B9/%D0%BB%D0%B8%D1%87%D0%BD%D1%8B%D0%B9%20%D0%BA%D0%B0%D0%B1%D0%B8%D0%BD%D0%B5%D1%82/%D0%B1%D0%B0%D0%BB%D0%B0%D0%BD%D1%81?%D0%B7%D0%B0%D0%BF%D1%80%D0%BE%D1%81=%D0%BF%D1%8F%D1%82%D1%8C&%D0%B8%D0%B4%D0%B5%D0%BD%D1%82%D0%B8%D1%84%D0%B8%D0%BA%D0%B0%D1%82%D0%BE%D1%80=%D0%B3%D0%BE%D0%B3%D0%B0#%D0%BF%D0%B5%D1%80%D0%B5%D0%B9%D1%82%D0%B8%20%D0%B2%20%D0%BD%D0%B8%D0%B7";
	cout << "Parsing URI: " << address << endl;

	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << " URI Scheme: " << uri.scheme() << endl;
	cout << " URI Host: " << uri.host() << endl;
	cout << " URI Port: " << uri.port() << endl;
	cout << " URI Fragment: " << uri.fragment() << endl;

	/**
	 * Выводим сегменты пути и параметры запроса URI
	 */
	for(const auto & segment : uri.path())
		cout << " URI Path Segment: " << segment << endl;
	cout << endl;

	/**
	 * Выводим параметры запроса URI
	 */
	for(const auto & [key, value] : uri.query())
		cout << " URI Query Parameter: " << key << " = " << value << endl;
	cout << endl;

	cout << endl << endl;

	address = "//cdn.jsdelivr.net/npm/prismjs@1/components/prism-json.min.js";
	cout << "Parsing URI: " << address << endl;

	uri.clear();
	uri.parse(address);

	cout << "SMART: " << uri.print(uri_t::item_t::URI, uri_t::format_t::SMART) << ", FULL: " << uri.print(uri_t::item_t::URI, uri_t::format_t::FULL) << ", REQUEST: " << uri.print(uri_t::item_t::REQUEST) << endl;

	cout << endl << endl;

	// Возвращаем результат
	return EXIT_SUCCESS;
}
