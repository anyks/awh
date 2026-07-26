/**
 * @file: headers.cpp
 * @date: 2026-07-15
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с контейнером HTTP-заголовков — демонстрация добавления, поиска и удаления полей,
 *        обхода итераторами и работы с множественными значениями одного заголовка
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <string>
#include <vector>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <proto/http/headers.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;
/**
 * Используем пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Демонстрация базовых операций с контейнером HTTP-заголовков
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleBasic(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== BASIC ======== " << endl;
	// Создаём контейнер HTTP-заголовков
	headers_t headers(fmk, log);
	// Добавляем заголовок хоста запроса
	headers.emplace("Host", "example.com");
	// Добавляем заголовок типа содержимого
	headers.emplace("Content-Type", "application/json");
	// Добавляем заголовок агента пользователя
	headers.emplace("User-Agent", "AWH/1.0");
	// Выводим общее количество добавленных заголовков
	cout << "Size: " << headers.size() << endl;
	// Проверяем наличие заголовка без учёта регистра названия
	cout << "Has 'host': " << (headers.has("host") ? "yes" : "no") << endl;
	// Извлекаем значение заголовка по его названию
	cout << "Content-Type: " << headers.at("Content-Type") << endl;
	// Извлекаем значение заголовка через оператор доступа
	cout << "User-Agent: " << headers["User-Agent"] << endl;
	// Заменяем значение уже существующего заголовка
	headers.emplace("Content-Type", "text/plain");
	// Выводим обновлённое значение заголовка (старое вхождение заменено)
	cout << "Content-Type (updated): " << headers.at("Content-Type") << endl;
	// Удаляем заголовок агента пользователя
	headers.erase("User-Agent");
	// Выводим количество заголовков после удаления
	cout << "Size after erase: " << headers.size() << endl << endl;
}
/**
 * @brief Демонстрация работы с несколькими заголовками с одним названием
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleMulti(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== MULTI-VALUE ======== " << endl;
	// Создаём контейнер HTTP-заголовков
	headers_t headers(fmk, log);
	// Добавляем заголовок принимаемых типов содержимого
	headers.emplace("Accept", "text/html");
	/**
	 * По умолчанию emplace() работает в режиме REPLACE и заменяет прежние одноимённые заголовки.
	 * Для нескольких заголовков с одним названием (например, Set-Cookie) используем режим APPEND,
	 * который добавляет новый заголовок, сохраняя уже существующие.
	 */
	headers.emplace("Set-Cookie", "sid=abc123; Path=/", headers_t::mode_t::APPEND);
	// Добавляем второй заголовок установки cookie в режиме APPEND
	headers.emplace("Set-Cookie", "theme=dark; Path=/", headers_t::mode_t::APPEND);
	// Выводим количество заголовков с указанным названием
	cout << "Set-Cookie count: " << headers.count("Set-Cookie") << endl;
	/**
	 * Получаем список всех значений заголовков с одним названием
	 */
	for(const auto & value : headers.range("Set-Cookie"))
		// Выводим очередное значение заголовка установки cookie
		cout << "Set-Cookie: " << value << endl;
	// Выводим список названий всех заголовков контейнера
	cout << "Names:";
	/**
	 * Проходим по всем названиям заголовков
	 */
	for(const auto & name : headers.names())
		// Выводим очередное название заголовка
		cout << " [" << name << "]";
	// Завершаем строку вывода названий заголовков
	cout << endl << endl;
}
/**
 * @brief Демонстрация обхода заголовков через итераторы
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleIterate(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== ITERATE ======== " << endl;
	// Создаём контейнер HTTP-заголовков
	headers_t headers(fmk, log);
	// Добавляем заголовок соединения
	headers.emplace("Connection", "keep-alive");
	// Добавляем заголовок длины содержимого
	headers.emplace("Content-Length", "1024");
	// Добавляем заголовок кодировки передачи
	headers.emplace("Transfer-Encoding", "chunked");
	/**
	 * Проходим по всем заголовкам контейнера при помощи диапазонного цикла
	 */
	for(const auto & header : headers)
		// Выводим название и значение очередного заголовка
		cout << header.name << ": " << header.value << endl;
	// Ищем конкретный заголовок по его названию
	auto it = headers.find("Content-Length");
	// Если заголовок найден - выводим его значение
	if(it != headers.end())
		// Выводим значение найденного заголовка
		cout << "Found Content-Length: " << it->value << endl;
	// Завершаем блок демонстрации
	cout << endl;
}
/**
 * @brief Демонстрация печати HTTP-запроса клиента (стартовая строка + заголовки)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleRequest(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== REQUEST (HTTP/1.1) ======== " << endl;
	// Создаём контейнер HTTP-заголовков
	headers_t headers(fmk, log);
	// Формируем параметры запроса клиента (метод, версия протокола и URI)
	request_t request(version_t::HTTP1_1, method_t::GET, "/index.html");
	// Устанавливаем объект провайдера запроса в контейнер заголовков
	headers.provider(&request);
	// Добавляем заголовок хоста запроса
	headers.emplace("Host", "anyks.com");
	// Добавляем заголовок принимаемых типов содержимого
	headers.emplace("Accept", "text/html");
	// Выводим сформированную стартовую строку запроса
	cout << "Startline: " << headers.startline() << endl;
	// Печатаем полный HTTP-запрос в формате протокола HTTP/1.1
	cout << "----" << endl << headers.print(proto_t::HTTP1);
}
/**
 * @brief Демонстрация печати HTTP-ответа сервера (стартовая строка + заголовки)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleResponse(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== RESPONSE (HTTP/1.1) ======== " << endl;
	// Создаём контейнер HTTP-заголовков
	headers_t headers(fmk, log);
	// Формируем параметры ответа сервера (версия протокола и код ответа)
	response_t response(version_t::HTTP1_1, static_cast <uint16_t> (200));
	// Устанавливаем объект провайдера ответа в контейнер заголовков
	headers.provider(&response);
	// Добавляем заголовок сервера
	headers.emplace("Server", "AWH");
	// Добавляем заголовок типа содержимого ответа
	headers.emplace("Content-Type", "application/json");
	// Добавляем заголовок длины содержимого ответа
	headers.emplace("Content-Length", "27");
	// Выводим сформированную стартовую строку ответа (сообщение подставлено по коду)
	cout << "Startline: " << headers.startline() << endl;
	// Печатаем полный HTTP-ответ в формате протокола HTTP/1.1
	cout << "----" << endl << headers.print(proto_t::HTTP1);
}
/**
 * @brief Демонстрация печати запроса в формате HTTP/2 (псевдозаголовки)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleHttp2(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== REQUEST (HTTP/2) ======== " << endl;
	// Создаём контейнер HTTP-заголовков с протоколом HTTP/2
	headers_t headers(proto_t::HTTP2, fmk, log);
	// Формируем параметры запроса клиента для протокола HTTP/2
	request_t request(version_t::HTTP2, method_t::GET, "https://example.com/path");
	// Устанавливаем объект провайдера запроса в контейнер заголовков
	headers.provider(&request);
	// Добавляем пользовательский заголовок принимаемых типов содержимого
	headers.emplace("Accept", "application/json");
	// Печатаем запрос в формате HTTP/2 (стартовая строка заменяется псевдозаголовками)
	cout << headers.print(proto_t::HTTP2) << endl;
}
/**
 * @brief Демонстрация слияния и обмена контейнеров заголовков
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleMerge(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== MERGE / SWAP ======== " << endl;
	// Создаём первый контейнер с базовыми заголовками
	headers_t base(fmk, log);
	// Добавляем заголовок хоста в первый контейнер
	base.emplace("Host", "anyks.com");
	// Добавляем заголовок соединения в первый контейнер
	base.emplace("Connection", "keep-alive");
	// Создаём второй контейнер с дополнительными заголовками
	headers_t extra(fmk, log);
	// Добавляем заголовок кэширования во второй контейнер
	extra.emplace("Cache-Control", "no-cache");
	// Добавляем заголовок принимаемых кодировок во второй контейнер
	extra.emplace("Accept-Encoding", "gzip, deflate");
	// Выполняем слияние второго контейнера в первый через оператор
	base += extra;
	// Выводим количество заголовков после слияния
	cout << "After merge: " << base.size() << " headers" << endl;
	// Создаём пустой контейнер для обмена
	headers_t target(fmk, log);
	// Обмениваемся содержимым контейнеров
	target.swap(base);
	// Выводим количество заголовков после обмена
	cout << "After swap: target=" << target.size() << ", base=" << base.size() << endl << endl;
}
/**
 * @brief Демонстрация преобразования контейнера в стандартные коллекции
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleConvert(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== CONVERT ======== " << endl;
	// Создаём контейнер HTTP-заголовков сразу с несколькими одноимёнными заголовками
	headers_t headers({
		headers_t::header_t{}.from("Host", "example.com"),
		headers_t::header_t{}.from("Set-Cookie", "a=1"),
		headers_t::header_t{}.from("Set-Cookie", "b=2")
	}, fmk, log);
	// Преобразуем контейнер в мультикарту заголовков (сохраняет все вхождения)
	const headers_t::multimap_t multi = static_cast <headers_t::multimap_t> (headers);
	// Выводим количество записей в мультикарте
	cout << "Multimap entries: " << multi.size() << endl;
	/**
	 * Проходим по всем записям мультикарты заголовков
	 */
	for(const auto & item : multi)
		// Выводим название и значение очередной записи мультикарты
		cout << item.first << " => " << item.second << endl;
	// Завершаем блок демонстрации
	cout << endl;
}
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
	// Демонстрируем базовые операции с заголовками
	sampleBasic(&fmk, &log);
	// Демонстрируем работу с заголовками с одним названием
	sampleMulti(&fmk, &log);
	// Демонстрируем обход заголовков через итераторы
	sampleIterate(&fmk, &log);
	// Демонстрируем печать HTTP-запроса клиента
	sampleRequest(&fmk, &log);
	// Демонстрируем печать HTTP-ответа сервера
	sampleResponse(&fmk, &log);
	// Демонстрируем печать запроса в формате HTTP/2
	sampleHttp2(&fmk, &log);
	// Демонстрируем слияние и обмен контейнеров заголовков
	sampleMerge(&fmk, &log);
	// Демонстрируем преобразование контейнера в стандартные коллекции
	sampleConvert(&fmk, &log);
	// Возвращаем результат
	return EXIT_SUCCESS;
}
