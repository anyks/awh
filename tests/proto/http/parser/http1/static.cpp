/**
 * @file static.cpp
 * @date 2026-07-18
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
 * @brief Статические тесты парсера протокола HTTP/1.x — проверка создания и сброса объекта модуля,
 *        а также корректности разбора стартовой строки, заголовков и тела, кадрирования chunked и контроля лимитов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <utility>
#include <cstdint>
#include <cstdio>
#include <algorithm>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http1.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Метод проверки разбора простого GET-запроса без тела
 *
 */
TEST_F(ParserFixture, SimpleRequestTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-запроса
	const std::string message = "GET /index.html?q=1 HTTP/1.1\r\nHost: anyks.com\r\nUser-Agent: awh\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что ошибок разбора нет
	ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
	// Получаем объект провайдера заголовков запроса клиента
	const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
	// Проверяем что метод запроса разобран корректно
	ASSERT_EQ(request->method, method_t::GET);
	// Проверяем что URI-адрес запроса разобран корректно
	ASSERT_EQ(request->uri, "/index.html?q=1");
	// Проверяем что версия протокола разобрана корректно
	ASSERT_EQ(request->version, version_t::HTTP1_1);
	// Проверяем что функция обратного вызова обработки провайдера вызвана
	ASSERT_TRUE(events.providerFired);
	// Проверяем что разобраны оба заголовка
	ASSERT_EQ(events.headers.size(), 2u);
	// Проверяем название первого заголовка
	ASSERT_EQ(events.headers[0].first, "Host");
	// Проверяем значение первого заголовка
	ASSERT_EQ(events.headers[0].second, "anyks.com");
	// Проверяем что соединение переиспользуемое (HTTP/1.1 по умолчанию)
	ASSERT_TRUE(parser->message().flags.keepAlive);
	// Проверяем что тело не передаётся chunked
	ASSERT_FALSE(parser->message().flags.chunked);
	// Проверяем что флаг завершения сообщения установлен
	ASSERT_TRUE(parser->message().flags.complete);
	// Проверяем что размер тела не определён (Content-Length не передавался)
	ASSERT_EQ(parser->message().bodySize, -1);
}

/**
 * @brief Метод проверки инкрементального разбора запроса с телом (побайтная подача)
 *
 */
TEST_F(ParserFixture, IncrementalRequestTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-запроса с телом фиксированного размера
	const std::string message = "POST /api HTTP/1.1\r\nHost: x\r\nContent-Length: 11\r\n\r\nhello world";
	/**
	 * Выполняем подачу данных HTTP-запроса по одному байту
	 */
	for(size_t i = 0; i < message.size(); ++i)
		// Проверяем что каждый байт данных обработан
		ASSERT_EQ(parser->parse(message.data() + i, 1), 1u);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения собрано корректно
	ASSERT_EQ(events.body, "hello world");
	// Проверяем что размер тела соответствует заголовку Content-Length
	ASSERT_EQ(parser->message().bodySize, 11);
}

/**
 * @brief Метод проверки разбора chunked-ответа с расширениями чанков и трейлерами
 *
 */
TEST_F(ParserFixture, ChunkedResponseWithTrailersTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-ответа с телом в кодировке chunked
	const std::string message =
		"HTTP/1.1 200 OK\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\nHello\r\n"
		"7;ext=val\r\n, World\r\n"
		"0\r\n"
		"X-Sum: abc\r\n"
		"\r\n";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения собрано корректно
	ASSERT_EQ(events.body, "Hello, World");
	// Проверяем что флаг передачи тела chunked установлен
	ASSERT_TRUE(parser->message().flags.chunked);
	// Проверяем что разобран один трейлер
	ASSERT_EQ(events.trailers.size(), 1u);
	// Проверяем название трейлера
	ASSERT_EQ(events.trailers[0].first, "X-Sum");
	// Проверяем значение трейлера
	ASSERT_EQ(events.trailers[0].second, "abc");
	// Получаем объект провайдера заголовков ответа сервера
	const response_t * response = static_cast <const response_t *> (parser->message().provider.get());
	// Проверяем что код ответа разобран корректно
	ASSERT_EQ(response->code, 200u);
	// Проверяем что сообщение сервера разобрано корректно
	ASSERT_EQ(response->message, "OK");
	// Проверяем количество событий границ чанков (BEGIN для трёх чанков + END для двух чанков с данными)
	ASSERT_EQ(events.chunks.size(), 5u);
	// Проверяем что расширения второго чанка переданы корректно
	ASSERT_EQ(std::get <2> (events.chunks[2]), "ext=val");
}

/**
 * @brief Метод проверки разбора конвейерных (pipelined) запросов через reset
 *
 */
TEST_F(ParserFixture, PipeliningResetTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Формируем данные двух конвейерных HTTP-запросов в одном буфере
	const std::string message = "GET /a HTTP/1.1\r\n\r\nGET /b HTTP/1.1\r\n\r\n";
	// Выполняем разбор данных первого HTTP-запроса
	const size_t first = parser->parse(message.data(), message.size());
	// Проверяем что первое сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что URI-адрес первого запроса разобран корректно
	ASSERT_EQ(static_cast <const request_t *> (parser->message().provider.get())->uri, "/a");
	// Выполняем сброс парсера для разбора следующего сообщения
	parser->reset();
	// Выполняем разбор данных второго HTTP-запроса на оставшемся хвосте буфера
	const size_t second = parser->parse(message.data() + first, message.size() - first);
	// Проверяем что второе сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что все данные буфера обработаны за два вызова
	ASSERT_EQ(first + second, message.size());
	// Проверяем что URI-адрес второго запроса разобран корректно
	ASSERT_EQ(static_cast <const request_t *> (parser->message().provider.get())->uri, "/b");
}

/**
 * @brief Метод проверки кадрирования ответа на запрос методом HEAD (Content-Length есть, тела нет)
 *
 */
TEST_F(ParserFixture, HeadResponseTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser->method(method_t::HEAD);
	// Формируем данные HTTP-ответа на запрос методом HEAD
	const std::string message = "HTTP/1.1 200 OK\r\nContent-Length: 12345\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано (тело не читается)
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения отсутствует
	ASSERT_TRUE(events.body.empty());
}

/**
 * @brief Метод проверки кадрирования успешного ответа на запрос методом CONNECT (туннель)
 *
 */
TEST_F(ParserFixture, ConnectTunnelTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser->method(method_t::CONNECT);
	// Формируем данные HTTP-ответа на запрос методом CONNECT с данными туннеля в хвосте
	const std::string message = "HTTP/1.1 200 Connection Established\r\n\r\nTUNNELDATA";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что парсер остановился ровно на границе заголовков (данные туннеля не тронуты)
	ASSERT_EQ(bytes, message.size() - 10u);
	// Проверяем что флаг переключения протокола установлен
	ASSERT_TRUE(parser->message().flags.upgrade);
	// Проверяем что тело сообщения отсутствует
	ASSERT_TRUE(events.body.empty());
}

/**
 * @brief Метод проверки чтения тела до закрытия соединения (HTTP/1.0) и метода eof
 *
 */
TEST_F(ParserFixture, BodyUntilCloseEofTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-ответа без Content-Length и без chunked
	const std::string message = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nstream-until-close";
	// Выполняем разбор данных HTTP-ответа
	parser->parse(message.data(), message.size());
	// Проверяем что сообщение ещё не завершено (конец тела определяется закрытием соединения)
	ASSERT_EQ(parser->status(), parser_t::status_t::PARTIAL);
	// Проверяем что соединение не переиспользуемое
	ASSERT_FALSE(parser->message().flags.keepAlive);
	// Уведомляем парсер о закрытии соединения
	parser->eof();
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения собрано корректно
	ASSERT_EQ(events.body, "stream-until-close");
	// Проверяем что версия протокола разобрана корректно
	ASSERT_EQ(static_cast <const response_t *> (parser->message().provider.get())->version, version_t::HTTP1_0);
}

/**
 * @brief Метод проверки поведения метода eof между сообщениями и посреди сообщения
 *
 */
TEST_F(ParserFixture, EofHandlingTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Уведомляем парсер о закрытии соединения между сообщениями
	parser->eof();
	// Проверяем что ошибки нет (нормальное закрытие keep-alive соединения)
	ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
	// Формируем данные частично принятого HTTP-запроса
	const std::string message = "GET / HTTP/1.1\r\nHo";
	// Выполняем разбор частично принятых данных
	parser->parse(message.data(), message.size());
	// Уведомляем парсер о закрытии соединения посреди сообщения
	parser->eof();
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует обрыву соединения
	ASSERT_EQ(parser->error(), parser_http_t::error_t::PREMATURE_EOF);
}

/**
 * @brief Метод проверки защиты от HTTP request smuggling
 *
 */
TEST_F(ParserFixture, SmugglingProtectionTest){
	// Создаём объект парсера запросов клиента для проверки конфликта CL+TE
	auto parser1 = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса с одновременными Content-Length и Transfer-Encoding
	const std::string message1 = "POST / HTTP/1.1\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser1->parse(message1.data(), message1.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser1->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует конфликту кадрирования
	ASSERT_EQ(parser1->error(), parser_http_t::error_t::CONTENT_LENGTH_CONFLICT);
	// Создаём объект парсера запросов клиента для проверки различающихся Content-Length
	auto parser2 = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса с двумя различающимися заголовками Content-Length
	const std::string message2 = "POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser2->parse(message2.data(), message2.size());
	// Проверяем что ошибка соответствует конфликту кадрирования
	ASSERT_EQ(parser2->error(), parser_http_t::error_t::CONTENT_LENGTH_CONFLICT);
}

/**
 * @brief Метод проверки валидации заголовка Transfer-Encoding
 *
 */
TEST_F(ParserFixture, TransferEncodingValidationTest){
	// Создаём объект парсера ответов сервера для проверки валидного списка кодирований
	auto parser1 = this->make(direct_t::RESPONSE);
	// Формируем данные HTTP-ответа со списком кодирований где chunked последний
	const std::string message1 = "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip, chunked\r\n\r\n0\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser1->parse(message1.data(), message1.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message1.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser1->status(), parser_t::status_t::COMPLETE);
	// Проверяем что флаг передачи тела chunked установлен
	ASSERT_TRUE(parser1->message().flags.chunked);
	// Создаём объект парсера запросов клиента для проверки некорректного списка кодирований
	auto parser2 = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса со списком кодирований где chunked не последний
	const std::string message2 = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked, gzip\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser2->parse(message2.data(), message2.size());
	// Проверяем что ошибка соответствует некорректному Transfer-Encoding
	ASSERT_EQ(parser2->error(), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
}

/**
 * @brief Метод проверки детектирования переключения протокола (Upgrade)
 *
 */
TEST_F(ParserFixture, UpgradeDetectionTest){
	// Создаём объект парсера запросов клиента
	auto parser1 = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса на переключение протокола (WebSocket)
	const std::string message1 =
		"GET /ws HTTP/1.1\r\n"
		"Host: x\r\n"
		"Connection: Upgrade\r\n"
		"Upgrade: websocket\r\n"
		"\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser1->parse(message1.data(), message1.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser1->status(), parser_t::status_t::COMPLETE);
	// Проверяем что флаг переключения протокола установлен
	ASSERT_TRUE(parser1->message().flags.upgrade);
	// Создаём объект парсера ответов сервера
	auto parser2 = this->make(direct_t::RESPONSE);
	// Формируем данные HTTP-ответа с подтверждением переключения протокола
	const std::string message2 = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser2->parse(message2.data(), message2.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message2.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser2->status(), parser_t::status_t::COMPLETE);
	// Проверяем что флаг переключения протокола установлен
	ASSERT_TRUE(parser2->message().flags.upgrade);
}

/**
 * @brief Метод проверки детектирования заголовка [Expect: 100-continue]
 *
 */
TEST_F(ParserFixture, ExpectContinueTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-запроса с заголовком [Expect: 100-continue]
	const std::string message = "PUT /file HTTP/1.1\r\nExpect: 100-continue\r\nContent-Length: 3\r\n\r\nabc";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что флаг ожидания промежуточного ответа установлен
	ASSERT_TRUE(parser->message().flags.expectContinue);
	// Проверяем что тело сообщения собрано корректно
	ASSERT_EQ(events.body, "abc");
}

/**
 * @brief Метод проверки разбора синтаксически корректного, но нераспознанного метода запроса
 *
 */
TEST_F(ParserFixture, UnknownMethodTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса с экзотическим методом
	const std::string message = "FOOBAR /exotic HTTP/1.1\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано (прозрачное проксирование экзотических методов)
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Получаем объект провайдера заголовков запроса клиента
	const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
	// Проверяем что метод запроса помечен как нераспознанный
	ASSERT_EQ(request->method, method_t::UNKNOWN);
	// Проверяем что оригинальное написание метода сохранено
	ASSERT_EQ(request->methodName, "FOOBAR");
	// Проверяем что URI-адрес запроса разобран корректно
	ASSERT_EQ(request->uri, "/exotic");
	// Выполняем сброс парсера для разбора следующего сообщения
	parser->reset();
	// Проверяем что оригинальное написание метода очищено
	ASSERT_TRUE(request->methodName.empty());
	// Формируем данные HTTP-запроса с распознаваемым методом
	const std::string message2 = "GET / HTTP/1.1\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message2.data(), message2.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что метод запроса распознан
	ASSERT_EQ(request->method, method_t::GET);
	// Проверяем что оригинальное написание метода не заполняется для распознанных методов
	ASSERT_TRUE(request->methodName.empty());
}

/**
 * @brief Метод проверки контракта версий протокола (принимаются только HTTP/1.0 и HTTP/1.1)
 *
 */
TEST_F(ParserFixture, VersionContractTest){
	// Создаём объект парсера запросов клиента
	auto parser1 = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса с неподдерживаемой минорной версией
	const std::string message1 = "GET / HTTP/1.2\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser1->parse(message1.data(), message1.size());
	// Проверяем что ошибка соответствует некорректной версии протокола
	ASSERT_EQ(parser1->error(), parser_http_t::error_t::INVALID_VERSION);
	// Создаём объект парсера ответов сервера
	auto parser2 = this->make(direct_t::RESPONSE);
	// Формируем данные HTTP-ответа с неподдерживаемой мажорной версией
	const std::string message2 = "HTTP/2.0 200 OK\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	parser2->parse(message2.data(), message2.size());
	// Проверяем что ошибка соответствует некорректной версии протокола
	ASSERT_EQ(parser2->error(), parser_http_t::error_t::INVALID_VERSION);
}

/**
 * @brief Метод проверки правил keep-alive для протокола HTTP/1.0
 *
 */
TEST_F(ParserFixture, Http10KeepAliveTest){
	// Создаём объект парсера запросов клиента с явным заголовком keep-alive
	auto parser1 = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса версии 1.0 с явным заголовком соединения
	const std::string message1 = "GET / HTTP/1.0\r\nConnection: keep-alive\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser1->parse(message1.data(), message1.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser1->status(), parser_t::status_t::COMPLETE);
	// Проверяем что соединение переиспользуемое (явный keep-alive)
	ASSERT_TRUE(parser1->message().flags.keepAlive);
	// Создаём объект парсера запросов клиента без заголовка соединения
	auto parser2 = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса версии 1.0 без заголовка соединения
	const std::string message2 = "GET / HTTP/1.0\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser2->parse(message2.data(), message2.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser2->status(), parser_t::status_t::COMPLETE);
	// Проверяем что соединение не переиспользуемое (для HTTP/1.0 по умолчанию close)
	ASSERT_FALSE(parser2->message().flags.keepAlive);
}

/**
 * @brief Метод проверки прерывания разбора пользовательской функцией обратного вызова
 *
 */
TEST_F(ParserFixture, AbortByCallbackTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Устанавливаем функцию обратного вызова прерывающую разбор на первом заголовке
	parser->on(parser_http_t::header_callback_t([](const uint32_t, const std::string_view, const std::string_view, const parser_t::part_t) noexcept -> bool {
		// Прерываем разбор
		return false;
	}));
	// Формируем данные HTTP-запроса
	const std::string message = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует прерыванию пользовательским callback'ом
	ASSERT_EQ(parser->error(), parser_http_t::error_t::ABORTED);
}

/**
 * @brief Метод проверки отсутствия тела у ответов со статус-кодами 204 и 304
 *
 */
TEST_F(ParserFixture, NoBodyStatusesTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-ответа со статус-кодом 204 и заголовком Content-Length
	const std::string message = "HTTP/1.1 204 No Content\r\nContent-Length: 10\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано (тело не читается)
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения отсутствует
	ASSERT_TRUE(events.body.empty());
}

/**
 * @brief Метод проверки лимитов безопасности парсера
 *
 */
TEST_F(ParserFixture, SecurityLimitsTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Устанавливаем максимальное число заголовков
	limits.maxHeaderCount = 2;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	// Формируем данные HTTP-запроса с превышением числа заголовков
	const std::string message = "GET / HTTP/1.1\r\nA: 1\r\nB: 2\r\nC: 3\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что ошибка соответствует превышению числа заголовков
	ASSERT_EQ(parser->error(), parser_http_t::error_t::TOO_MANY_HEADERS);
}

/**
 * @brief Метод проверки разбора статус-кодов превышающих 255 (проверка отсутствия усечения)
 *
 */
TEST_F(ParserFixture, LargeStatusCodeTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Формируем данные HTTP-ответа со статус-кодом больше 255
	const std::string message = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Получаем объект провайдера заголовков ответа сервера
	const response_t * response = static_cast <const response_t *> (parser->message().provider.get());
	// Проверяем что статус-код разобран без усечения
	ASSERT_EQ(response->code, 404u);
	// Проверяем что сообщение сервера разобрано корректно
	ASSERT_EQ(response->message, "Not Found");
}

/**
 * @brief Метод проверки учёта пробелов стартовой строки в лимите длины (защита от space-flood DoS)
 *
 */
TEST_F(ParserFixture, RequestLineSpaceFloodTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Устанавливаем максимальную длину request-line
	limits.maxRequestLine = 32;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	// Формируем данные HTTP-запроса с потоком пробелов перед request-target
	const std::string message = ("GET " + std::string(100, ' ') + "/ HTTP/1.1\r\n\r\n");
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует превышению длины request-line
	ASSERT_EQ(parser->error(), parser_http_t::error_t::URL_OVERFLOW);
}

/**
 * @brief Метод проверки разбора значения Content-Length переданного списком
 *
 */
TEST_F(ParserFixture, ContentLengthListTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-запроса со списком одинаковых значений Content-Length
	const std::string message = "POST / HTTP/1.1\r\nContent-Length: 5, 5\r\n\r\nabcde";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения собрано корректно
	ASSERT_EQ(events.body, "abcde");
}

/**
 * @brief Метод проверки порядка фазовых событий разбора сообщения
 *
 */
TEST_F(ParserFixture, PhaseOrderTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-запроса с телом фиксированного размера
	const std::string message = "POST / HTTP/1.1\r\nContent-Length: 1\r\n\r\nZ";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Формируем ожидаемую последовательность фазовых событий
	const std::vector <std::pair <parser_t::phase_t, parser_t::part_t>> expected = {
		{parser_t::phase_t::BEGIN, parser_t::part_t::NONE},
		{parser_t::phase_t::END,   parser_t::part_t::HEADERS},
		{parser_t::phase_t::BEGIN, parser_t::part_t::BODY},
		{parser_t::phase_t::END,   parser_t::part_t::BODY},
		{parser_t::phase_t::END,   parser_t::part_t::NONE}
	};
	// Проверяем что последовательность фазовых событий соответствует ожидаемой
	ASSERT_EQ(events.phases, expected);
}

/**
 * @brief Метод проверки получения человекочитаемых названий кодов ошибок
 *
 */
TEST_F(ParserFixture, ErrorNameTest){
	// Проверяем название кода ошибки обрыва соединения
	ASSERT_EQ(parser_http_t::errorName(parser_http_t::error_t::PREMATURE_EOF), "PREMATURE_EOF");
	// Проверяем название кода ошибки конфликта кадрирования
	ASSERT_EQ(parser_http_t::errorName(parser_http_t::error_t::CONTENT_LENGTH_CONFLICT), "CONTENT_LENGTH_CONFLICT");
	// Проверяем название кода отсутствия ошибки
	ASSERT_EQ(parser_http_t::errorName(parser_http_t::error_t::NONE), "NONE");
}

/**
 * @brief Метод проверки клонирования объекта парсера
 *
 */
TEST_F(ParserFixture, CloneTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Устанавливаем максимальное число заголовков
	limits.maxHeaderCount = 3;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	// Клонируем объект парсера через базовый интерфейс
	std::unique_ptr <parser_t> clone = parser->clone();
	// Проверяем что клон создан
	ASSERT_TRUE(clone != nullptr);
	// Безопасно приводим клон к типу парсера HTTP/1.1
	parser_http_t * clonePtr = static_cast <parser_http_t *> (clone.get());
	// Проверяем что лимиты безопасности клонированы корректно
	ASSERT_EQ(clonePtr->limits().maxHeaderCount, 3u);
	// Формируем данные HTTP-запроса
	const std::string message = "GET /clone HTTP/1.1\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса клонированным парсером
	clonePtr->parse(message.data(), message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(clonePtr->status(), parser_t::status_t::COMPLETE);
}

/**
 * @brief Метод проверки прозрачной ретрансляции chunked-кадрирования (сценарий прокси)
 *
 */
TEST_F(ParserFixture, ChunkTransparentRelayTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Реконструированный chunked-поток
	std::string relay;
	// Устанавливаем функцию обратного вызова для обработки границ чанков
	parser->on(parser_http_t::chunk_callback_t([&relay](const parser_t::phase_t phase, const uint64_t size, const std::string_view extension) noexcept -> bool {
		// Если разобран заголовок очередного чанка
		if(phase == parser_t::phase_t::BEGIN){
			// Буфер для форматирования hex-размера чанка
			char buffer[32];
			// Форматируем hex-размер чанка
			::snprintf(buffer, sizeof(buffer), "%llx", static_cast <unsigned long long> (size));
			// Дописываем hex-размер чанка в реконструированный поток
			relay.append(buffer);
			// Если расширения чанка присутствуют
			if(!extension.empty()){
				// Дописываем разделитель расширений чанка
				relay.push_back(';');
				// Дописываем расширения чанка в реконструированный поток
				relay.append(extension);
			}
			// Дописываем окончание строки заголовка чанка
			relay.append("\r\n");
		// Если данные чанка дочитаны - дописываем завершающий CRLF
		} else relay.append("\r\n");
		// Продолжаем разбор
		return true;
	}));
	// Устанавливаем функцию обратного вызова для обработки фрагмента тела сообщения
	parser->on(parser_http_t::data_callback_t([&relay](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Дописываем фрагмент данных чанка в реконструированный поток
		relay.append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Формируем заголовки HTTP-ответа
	const std::string head = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
	// Формируем исходный chunked-поток с расширением чанка (подпись в стиле aws-chunked)
	const std::string chunked =
		"5;chunk-signature=af6a9\r\nHello\r\n"
		"7\r\n, World\r\n"
		"0\r\n";
	// Формируем полные данные HTTP-ответа
	const std::string message = (head + chunked + "\r\n");
	/**
	 * Выполняем подачу данных мелкими кусками по 3 байта (проверка инкрементальности расширений)
	 */
	for(size_t i = 0; i < message.size(); i += 3)
		// Выполняем разбор очередного куска данных
		parser->parse(message.data() + i, ((message.size() - i) < 3 ? (message.size() - i) : 3));
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что реконструированный поток байт-в-байт совпадает с исходным
	ASSERT_EQ(relay, chunked);
}

/**
 * @brief Метод проверки регистрозависимости метода запроса (RFC 9110 §9.1)
 *
 */
TEST_F(ParserFixture, MethodCaseSensitivityTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса с методом в неканоническом регистре
	const std::string message = "get / HTTP/1.1\r\nHost: anyks.com\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что сообщение полностью разобрано (синтаксис метода корректен)
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Получаем объект провайдера заголовков запроса клиента
	const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
	// Проверяем что метод в неканоническом регистре не приравнен к GET
	ASSERT_EQ(request->method, method_t::UNKNOWN);
	// Проверяем что оригинальное написание метода сохранено
	ASSERT_EQ(request->methodName, "get");
}

/**
 * @brief Метод проверки строгого режима окончания строк (запрет одиночного LF)
 *
 */
TEST_F(ParserFixture, StrictEndOfLineTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса с одиночным LF в качестве окончания строк
	const std::string message = "GET / HTTP/1.1\nHost: anyks.com\n\n";
	// Выполняем разбор данных HTTP-запроса в толерантном режиме
	parser->parse(message.data(), message.size());
	// Проверяем что в толерантном режиме сообщение разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Создаём объект парсера запросов клиента для строгого режима
	auto strict = this->make(direct_t::REQUEST);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = strict->limits();
	// Включаем требование строгого окончания строк
	limits.strictEOL = true;
	// Применяем изменённые лимиты безопасности
	strict->limits(limits);
	// Выполняем разбор данных HTTP-запроса в строгом режиме
	strict->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(strict->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует некорректному окончанию строки
	ASSERT_EQ(strict->error(), parser_http_t::error_t::INVALID_EOL);
}

/**
 * @brief Метод проверки строгого режима пробелов стартовой строки
 *
 */
TEST_F(ParserFixture, StrictSpacesTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Включаем запрет лишних пробелов внутри стартовой строки
	limits.strictSpaces = true;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	// Формируем данные HTTP-запроса с удвоенным разделителем стартовой строки
	const std::string message = "GET  / HTTP/1.1\r\nHost: anyks.com\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует некорректному request-target
	ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_TARGET);
}

/**
 * @brief Метод проверки требования обязательного заголовка Host у запросов HTTP/1.1
 *
 */
TEST_F(ParserFixture, RequireHostTest){
	/**
	 * Выполняем перебор проверяемых наборов заголовков запроса
	 */
	for(const auto & item : {std::string(""), std::string("Host: a\r\nHost: b\r\n")}) {
		// Создаём объект парсера запросов клиента
		auto parser = this->make(direct_t::REQUEST);
		// Получаем текущие лимиты безопасности
		parser_http_t::limits_t limits = parser->limits();
		// Включаем требование обязательного заголовка Host
		limits.requireHost = true;
		// Применяем изменённые лимиты безопасности
		parser->limits(limits);
		// Формируем данные HTTP-запроса с проверяемым набором заголовков
		const std::string message = ("GET / HTTP/1.1\r\n" + item + "\r\n");
		// Выполняем разбор данных HTTP-запроса
		parser->parse(message.data(), message.size());
		// Проверяем что зафиксирована ошибка разбора
		ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
		// Проверяем что ошибка соответствует отсутствию либо дублированию заголовка Host
		ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_HOST);
	}
	// Создаём объект парсера запросов клиента для корректного запроса
	auto parser = this->make(direct_t::REQUEST);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Включаем требование обязательного заголовка Host
	limits.requireHost = true;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	// Формируем данные корректного HTTP-запроса с единственным заголовком Host
	const std::string message = "GET / HTTP/1.1\r\nHost: anyks.com\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что корректный запрос полностью разобран
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
}

/**
 * @brief Метод проверки отказа от chunked с параметрами транспортного кодирования
 *
 * @details Кодирование chunked параметров не определяет, и их присутствие RFC 9112 §7.1
 *          предписывает считать ошибкой. Послабление здесь недопустимо именно потому,
 *          что кодирование кадрирующее: звено цепочки, не признавшее "chunked;ext=1"
 *          за chunked, будет кадрировать то же тело иначе - это тот же механизм
 *          рассинхронизации, что и chunked не последним в списке. У прочих кодирований
 *          параметры на кадрирование не влияют и отсекаются от имени как прежде
 *
 */
TEST_F(ParserFixture, TransferEncodingParametersTest){
	/**
	 * Параметры у кодирования chunked делают объявление некорректным
	 */
	{
		// Создаём объект парсера запросов клиента
		auto parser = this->make(direct_t::REQUEST);
		// Формируем данные HTTP-запроса с параметрами транспортного кодирования
		const std::string message = "POST / HTTP/1.1\r\nHost: anyks.com\r\nTransfer-Encoding: chunked;ext=1\r\n\r\n3\r\nabc\r\n0\r\n\r\n";
		// Выполняем разбор данных HTTP-запроса
		parser->parse(message.data(), message.size());
		// Проверяем что объявление кодирования отвергнуто
		ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
	}
	/**
	 * Параметры у прочих кодирований отсекаются от имени и кадрирование не ломают
	 */
	{
		// Создаём объект парсера запросов клиента
		auto parser = this->make(direct_t::REQUEST);
		// Создаём объект сборщика событий парсера
		events_t events;
		// Подписываем сборщик событий на все функции обратного вызова парсера
		this->attach(* parser, events);
		// Формируем данные HTTP-запроса с параметрами у некадрирующего кодирования
		const std::string message = "POST / HTTP/1.1\r\nHost: anyks.com\r\nTransfer-Encoding: gzip;q=1, chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n";
		// Выполняем разбор данных HTTP-запроса
		parser->parse(message.data(), message.size());
		// Проверяем что сообщение полностью разобрано
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
		// Проверяем что тело кадрировалось chunked
		ASSERT_TRUE(parser->message().flags.chunked);
		// Проверяем что тело сообщения собрано корректно
		ASSERT_EQ(events.body, "abc");
	}
}

/**
 * @brief Метод проверки отбрасывания запрещённых трейлеров (RFC 9112 §6.5)
 *
 */
TEST_F(ParserFixture, ForbiddenTrailersTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-запроса с запрещённым и разрешённым трейлерами
	const std::string message =
		"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
		"3\r\nabc\r\n0\r\nContent-Length: 100\r\nX-Check: done\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что до потребителя дошёл единственный разрешённый трейлер
	ASSERT_EQ(events.trailers.size(), 1u);
	// Проверяем что разрешённый трейлер передан без искажений
	ASSERT_EQ(events.trailers.front().first, "X-Check");
	// Проверяем что кадрирование тела не переопределено трейлером
	ASSERT_EQ(parser->message().bodySize, -1);
}

/**
 * @brief Метод проверки разбора заголовка Expect переданного списком
 *
 */
TEST_F(ParserFixture, ExpectContinueListTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса со списком ожиданий клиента
	const std::string message = "POST / HTTP/1.1\r\nContent-Length: 0\r\nExpect: other, 100-Continue;v=1\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что ожидание промежуточного ответа распознано
	ASSERT_TRUE(parser->message().flags.expectContinue);
}

/**
 * @brief Метод проверки отклонения Content-Length превышающего диапазон размера тела
 *
 */
TEST_F(ParserFixture, ContentLengthRangeTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Снимаем лимит размера тела, чтобы проверить именно контроль диапазона
	limits.maxBodySize = UINT64_MAX;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	// Формируем данные HTTP-запроса с Content-Length выше диапазона знакового типа
	const std::string message = "POST / HTTP/1.1\r\nContent-Length: 18446744073709551000\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует некорректному Content-Length
	ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем что размер тела наружу отрицательным не отдан
	ASSERT_EQ(parser->message().bodySize, -1);
}

/**
 * @brief Метод проверки порядка фиксации превышения размера тела (до уведомления потребителя)
 *
 */
TEST_F(ParserFixture, BodyOverflowOrderTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Устанавливаем максимальный размер тела
	limits.maxBodySize = 10;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	// Формируем данные HTTP-запроса с телом превышающим лимит
	const std::string message = "POST / HTTP/1.1\r\nContent-Length: 100\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что ошибка соответствует превышению размера тела
	ASSERT_EQ(parser->error(), parser_http_t::error_t::BODY_OVERFLOW);
	// Проверяем что потребитель не был уведомлён о готовности провайдера заголовков
	ASSERT_FALSE(events.providerFired);
}

/**
 * @brief Метод проверки неприменимости лимита размера тела к ответам без тела
 *
 */
TEST_F(ParserFixture, NoBodyResponseLimitTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser->method(method_t::HEAD);
	// Получаем текущие лимиты безопасности
	parser_http_t::limits_t limits = parser->limits();
	// Устанавливаем максимальный размер тела
	limits.maxBodySize = 10;
	// Применяем изменённые лимиты безопасности
	parser->limits(limits);
	/**
	 * Формируем данные ответа на HEAD с заголовком Content-Length выше лимита:
	 * заголовок описывает гипотетическое тело, которого в сообщении нет
	 */
	const std::string message = "HTTP/1.1 200 OK\r\nContent-Length: 1000000\r\n\r\n";
	// Выполняем разбор данных HTTP-ответа
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано без ошибки лимита
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что кадрирование chunked у ответа без тела не выставлено
	ASSERT_FALSE(parser->message().flags.chunked);
}

/**
 * @brief Метод проверки порядка фиксации некадрируемого Transfer-Encoding у запроса
 *
 */
TEST_F(ParserFixture, TransferEncodingOrderTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-запроса с транспортным кодированием без завершающего chunked
	const std::string message = "POST / HTTP/1.1\r\nHost: anyks.com\r\nTransfer-Encoding: gzip\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует некорректному Transfer-Encoding
	ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
	/**
	 * Проверяем что потребитель не был уведомлён о готовности провайдера заголовков:
	 * иначе он завёл бы приём тела, которого у некадрируемого запроса не будет
	 */
	ASSERT_FALSE(events.providerFired);
}

/**
 * @brief Метод проверки признака полноты сообщения при прерывании разбора
 *
 */
TEST_F(ParserFixture, AbortOnCompletePhaseTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Подписываемся на фазовые события с прерыванием на завершении сообщения
	parser->on(parser_http_t::phase_callback_t([](const uint32_t, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
		// Прерываем разбор на событии завершения всего сообщения
		return !((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE));
	}));
	// Формируем данные корректного HTTP-запроса
	const std::string message = "GET / HTTP/1.1\r\nHost: anyks.com\r\n\r\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка прерывания разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что ошибка соответствует прерыванию разбора
	ASSERT_EQ(parser->error(), parser_http_t::error_t::ABORTED);
	/**
	 * Проверяем что сообщение не помечено полностью разобранным: признак полноты
	 * вместе с состоянием ошибки давал бы потребителю противоречивую картину
	 */
	ASSERT_FALSE(parser->message().flags.complete);
}

/**
 * @brief Метод проверки строгого набора ограничений безопасности
 *
 */
TEST_F(ParserFixture, StrictLimitsPresetTest){
	// Получаем строгий набор ограничений безопасности
	const parser_http_t::limits_t limits = parser_http_t::limits_t::strict();
	// Проверяем что требование строгого окончания строк включено
	ASSERT_TRUE(limits.strictEOL);
	// Проверяем что запрет лишних пробелов включён
	ASSERT_TRUE(limits.strictSpaces);
	// Проверяем что требование заголовка Host включено
	ASSERT_TRUE(limits.requireHost);
	// Проверяем что числовые лимиты остались значениями по умолчанию
	ASSERT_EQ(limits.maxRequestLine, parser_http_t::MAX_REQUEST_LINE);
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Применяем строгий набор ограничений безопасности
	parser->limits(limits);
	// Формируем данные HTTP-запроса с одиночным LF в качестве окончания строк
	const std::string message = "GET / HTTP/1.1\nHost: anyks.com\n\n";
	// Выполняем разбор данных HTTP-запроса
	parser->parse(message.data(), message.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
}

/**
 * @brief Метод проверки различимости усечённого сообщения по фазе и части
 *
 * @details Фаза END относится к части, названной рядом идущим значением part_t, а не
 *          к сообщению. Усечённый ответ, у которого принят лишь блок заголовков, выдаёт
 *          END точно так же, как дочитанный до конца, - разница целиком в части.
 *          Признак завершённости, собранный по одной лишь фазе, объявил бы принятым
 *          сообщение, у которого не хватает тела, и потребитель начал бы разбор
 *          неполных данных. Тест закрепляет обе приметы, по которым усечение
 *          отличается от дочитанного сообщения: отсутствие END вместе с NONE
 *          и статус PARTIAL
 *
 */
TEST_F(ParserFixture, TruncatedBodyIsNotCompleteTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем блок заголовков ответа с объявленной длиной тела
	const std::string head = "HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: 64\r\n\r\n";
	// Выполняем разбор блока заголовков и половины объявленного тела
	parser->parse((head + std::string(32, 'x')).data(), head.size() + 32);
	// Проверяем что конец блока заголовков объявлен
	ASSERT_NE(std::find(events.phases.begin(), events.phases.end(),
		std::make_pair(parser_t::phase_t::END, parser_t::part_t::HEADERS)), events.phases.end());
	// Проверяем что конец самого сообщения не объявлен
	ASSERT_EQ(std::find(events.phases.begin(), events.phases.end(),
		std::make_pair(parser_t::phase_t::END, parser_t::part_t::NONE)), events.phases.end());
	// Проверяем что сообщение считается незавершённым
	ASSERT_EQ(parser->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибка при этом не зафиксирована - данных просто не хватает
	ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
	// Выполняем разбор оставшейся половины объявленного тела
	parser->parse(std::string(32, 'x').data(), 32);
	// Проверяем что конец самого сообщения объявлен
	ASSERT_NE(std::find(events.phases.begin(), events.phases.end(),
		std::make_pair(parser_t::phase_t::END, parser_t::part_t::NONE)), events.phases.end());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения собрано целиком
	ASSERT_EQ(events.body, std::string(64, 'x'));
}
