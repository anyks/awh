/**
 * @file: parser.cpp
 * @date: 2026-07-18
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
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <proto/http/parser/http1/http.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;
/**
 * Используем пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Демонстрация разбора простого HTTP-запроса клиента
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleRequest(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== REQUEST ======== " << endl;
	// Создаём объект парсера запросов клиента
	parser_http_t parser(direct_t::REQUEST, fmk, log);
	// Устанавливаем функцию обратного вызова для обработки заголовков сообщения
	parser.on(parser_http_t::header_callback_t([](const string_view name, const string_view value, const parser_t::part_t) noexcept -> bool {
		// Выводим название и значение очередного заголовка
		cout << "Header: [" << name << "] = [" << value << "]" << endl;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки провайдера заголовков сообщения
	parser.on(parser_http_t::provider_callback_t([](const provider_t * provider) noexcept -> bool {
		// Получаем объект провайдера заголовков запроса клиента
		const request_t * request = static_cast <const request_t *> (provider);
		// Выводим разобранный URI-адрес запроса
		cout << "URI: " << request->uri << endl;
		// Выводим версию протокола запроса
		cout << "Version: HTTP/" << (request->version == version_t::HTTP1_0 ? "1.0" : "1.1") << endl;
		// Продолжаем разбор
		return true;
	}));
	// Формируем данные HTTP-запроса
	const string message = "GET /index.html?q=awh HTTP/1.1\r\nHost: anyks.com\r\nAccept: text/html\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	const size_t bytes = parser.parse(message.data(), message.size());
	// Выводим количество обработанных байт
	cout << "Parsed bytes: " << bytes << " of " << message.size() << endl;
	// Выводим итоговый статус разбора
	cout << "Complete: " << (parser.status() == parser_t::status_t::COMPLETE ? "yes" : "no") << endl;
	// Выводим флаг переиспользования соединения
	cout << "Keep-Alive: " << (parser.message().flags.keepAlive ? "yes" : "no") << endl << endl;
}
/**
 * @brief Демонстрация инкрементального (потокового) разбора HTTP-ответа сервера
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleStreaming(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== STREAMING ======== " << endl;
	// Создаём объект парсера ответов сервера
	parser_http_t parser(direct_t::RESPONSE, fmk, log);
	// Устанавливаем функцию обратного вызова для обработки тела сообщения
	parser.on(parser_http_t::body_callback_t([](const void * buffer, const size_t size) noexcept -> bool {
		// Выводим очередной принятый фрагмент тела сообщения (zero-copy)
		cout << "Body fragment (" << size << " bytes): [" << string(static_cast <const char *> (buffer), size) << "]" << endl;
		// Продолжаем разбор
		return true;
	}));
	// Формируем данные HTTP-ответа с телом фиксированного размера
	const string message = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello, World";
	/**
	 * Данные из сети приходят произвольными кусками — подаём сообщение фрагментами по 10 байт,
	 * парсер сам восстанавливает границы строк, заголовков и тела между вызовами parse()
	 */
	for(size_t i = 0; i < message.size(); i += 10){
		// Выполняем разбор очередного фрагмента данных
		parser.parse(message.data() + i, ((message.size() - i) < 10 ? (message.size() - i) : 10));
		// Выводим текущий статус разбора после обработки фрагмента
		cout << "Status: " << (parser.status() == parser_t::status_t::COMPLETE ? "COMPLETE" : "PARTIAL") << endl;
	}
	// Выводим ожидаемый размер тела сообщения из заголовка Content-Length
	cout << "Body size: " << parser.message().bodySize << endl << endl;
}
/**
 * @brief Демонстрация разбора chunked-ответа с расширениями чанков и трейлерами
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleChunked(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== CHUNKED ======== " << endl;
	// Создаём объект парсера ответов сервера
	parser_http_t parser(direct_t::RESPONSE, fmk, log);
	// Собранное тело сообщения
	string body = "";
	// Устанавливаем функцию обратного вызова для обработки тела сообщения
	parser.on(parser_http_t::body_callback_t([&body](const void * buffer, const size_t size) noexcept -> bool {
		// Собираем фрагмент тела сообщения
		body.append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	/**
	 * События границ чанков нужны для прозрачного проксирования: они позволяют
	 * ретранслировать chunked-поток байт-в-байт, включая размеры и расширения чанков
	 */
	parser.on(parser_http_t::chunk_callback_t([](const parser_t::phase_t phase, const uint64_t size, const string_view extension) noexcept -> bool {
		// Если разобран заголовок очередного чанка
		if(phase == parser_t::phase_t::BEGIN){
			// Выводим размер очередного чанка
			cout << "Chunk begin: size=" << size;
			// Если расширения чанка присутствуют - выводим их
			if(!extension.empty())
				// Выводим расширения чанка
				cout << ", extension=[" << extension << "]";
			// Завершаем строку вывода
			cout << endl;
		// Если данные чанка дочитаны - выводим событие завершения чанка
		} else cout << "Chunk end" << endl;
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки заголовков или трейлеров сообщения
	parser.on(parser_http_t::header_callback_t([](const string_view name, const string_view value, const parser_t::part_t part) noexcept -> bool {
		// Если получен трейлер сообщения - выводим его
		if(part == parser_t::part_t::TRAILER)
			// Выводим название и значение трейлера
			cout << "Trailer: [" << name << "] = [" << value << "]" << endl;
		// Продолжаем разбор
		return true;
	}));
	// Формируем данные HTTP-ответа с телом в кодировке chunked
	const string message =
		"HTTP/1.1 200 OK\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5;sig=abc\r\nHello\r\n"
		"7\r\n, World\r\n"
		"0\r\n"
		"X-Checksum: 42\r\n"
		"\r\n";
	// Выполняем разбор данных HTTP-ответа
	parser.parse(message.data(), message.size());
	// Выводим собранное тело сообщения
	cout << "Body: [" << body << "]" << endl << endl;
}
/**
 * @brief Демонстрация разбора конвейерных (pipelined) запросов в одном буфере
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void samplePipelining(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== PIPELINING ======== " << endl;
	// Создаём объект парсера запросов клиента
	parser_http_t parser(direct_t::REQUEST, fmk, log);
	// Формируем данные трёх конвейерных HTTP-запросов в одном буфере
	const string message =
		"GET /first HTTP/1.1\r\nHost: x\r\n\r\n"
		"GET /second HTTP/1.1\r\nHost: x\r\n\r\n"
		"GET /third HTTP/1.1\r\nHost: x\r\n\r\n";
	// Текущее смещение в буфере данных
	size_t offset = 0;
	/**
	 * Разбираем сообщения по одному: parse() останавливается на границе сообщения,
	 * а дешёвый reset() готовит парсер к следующему сообщению сохраняя настройки
	 */
	while(offset < message.size()){
		// Выполняем разбор очередного HTTP-запроса
		offset += parser.parse(message.data() + offset, message.size() - offset);
		// Если сообщение полностью разобрано
		if(parser.status() == parser_t::status_t::COMPLETE){
			// Выводим URI-адрес разобранного запроса
			cout << "Request URI: " << static_cast <const request_t *> (parser.message().provider.get())->uri << endl;
			// Выполняем сброс парсера для разбора следующего сообщения
			parser.reset();
		// Если разбор прерван по другой причине - выходим из цикла
		} else break;
	}
	// Завершаем блок демонстрации
	cout << endl;
}
/**
 * @brief Демонстрация кадрирования ответа на запрос методом HEAD
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleHead(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== HEAD RESPONSE ======== " << endl;
	// Создаём объект парсера ответов сервера
	parser_http_t parser(direct_t::RESPONSE, fmk, log);
	/**
	 * Сообщаем парсеру метод запроса, которому соответствует ожидаемый ответ:
	 * ответ на HEAD содержит Content-Length, но тело при этом не передаётся
	 */
	parser.method(method_t::HEAD);
	// Формируем данные HTTP-ответа на запрос методом HEAD
	const string message = "HTTP/1.1 200 OK\r\nContent-Length: 1048576\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	parser.parse(message.data(), message.size());
	// Выводим итоговый статус разбора (сообщение завершено без чтения тела)
	cout << "Complete: " << (parser.status() == parser_t::status_t::COMPLETE ? "yes" : "no") << endl;
	// Выводим ожидаемый размер тела сообщения из заголовка Content-Length
	cout << "Declared body size: " << parser.message().bodySize << endl << endl;
}
/**
 * @brief Демонстрация чтения тела до закрытия соединения (HTTP/1.0)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleUntilClose(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== BODY UNTIL CLOSE ======== " << endl;
	// Создаём объект парсера ответов сервера
	parser_http_t parser(direct_t::RESPONSE, fmk, log);
	// Собранное тело сообщения
	string body = "";
	// Устанавливаем функцию обратного вызова для обработки тела сообщения
	parser.on(parser_http_t::body_callback_t([&body](const void * buffer, const size_t size) noexcept -> bool {
		// Собираем фрагмент тела сообщения
		body.append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Формируем данные HTTP-ответа без Content-Length и без chunked
	const string message = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nlegacy stream data";
	// Выполняем разбор данных HTTP-ответа
	parser.parse(message.data(), message.size());
	// Выводим текущий статус разбора (конец тела определяется закрытием соединения)
	cout << "Status before EOF: " << (parser.status() == parser_t::status_t::PARTIAL ? "PARTIAL" : "?") << endl;
	// Уведомляем парсер о закрытии соединения удалённой стороной
	parser.eof();
	// Выводим итоговый статус разбора после закрытия соединения
	cout << "Status after EOF: " << (parser.status() == parser_t::status_t::COMPLETE ? "COMPLETE" : "?") << endl;
	// Выводим собранное тело сообщения
	cout << "Body: [" << body << "]" << endl << endl;
}
/**
 * @brief Демонстрация детектирования ошибок разбора и защиты от request smuggling
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void sampleErrors(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== ERRORS ======== " << endl;
	// Создаём объект парсера запросов клиента
	parser_http_t parser(direct_t::REQUEST, fmk, log);
	// Формируем данные HTTP-запроса с конфликтом кадрирования (попытка request smuggling)
	const string message = "POST / HTTP/1.1\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser.parse(message.data(), message.size());
	// Выводим итоговый статус разбора
	cout << "Status: " << (parser.status() == parser_t::status_t::ERROR ? "ERROR" : "?") << endl;
	// Выводим человекочитаемое название кода ошибки
	cout << "Error: " << parser_t::errorName(parser.error()) << endl << endl;
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
	// Демонстрируем разбор простого HTTP-запроса клиента
	sampleRequest(&fmk, &log);
	// Демонстрируем инкрементальный (потоковый) разбор HTTP-ответа сервера
	sampleStreaming(&fmk, &log);
	// Демонстрируем разбор chunked-ответа с расширениями чанков и трейлерами
	sampleChunked(&fmk, &log);
	// Демонстрируем разбор конвейерных (pipelined) запросов
	samplePipelining(&fmk, &log);
	// Демонстрируем кадрирование ответа на запрос методом HEAD
	sampleHead(&fmk, &log);
	// Демонстрируем чтение тела до закрытия соединения (HTTP/1.0)
	sampleUntilClose(&fmk, &log);
	// Демонстрируем детектирование ошибок разбора
	sampleErrors(&fmk, &log);
	// Возвращаем результат
	return EXIT_SUCCESS;
}
