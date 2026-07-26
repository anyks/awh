/**
 * @file: sender.cpp
 * @date: 2026-07-20
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
#include <memory>
#include <vector>
#include <utility>
#include <cstdint>
#include <cstring>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http1.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Внутренние вспомогательные функции
 *
 */
namespace {
	/**
	 * @brief Функция перекачки исходящих байтов отправителя в принимающий парсер (pull-модель)
	 *
	 * @param sender   объект парсера-отправителя
	 * @param receiver объект парсера-приёмника
	 */
	void drain(parser_http_t & sender, parser_http_t & receiver) noexcept {
		/**
		 * Выкачиваем исходящие байты, пока они есть: consumePending может
		 * дозагрузить буфер из pull-источника данных отправителя
		 */
		while(!sender.pending().empty()){
			// Получаем ещё не отправленные исходящие байты
			const std::string_view out = sender.pending();
			// Передаём исходящие байты принимающему парсеру
			receiver.parse(out.data(), out.size());
			// Освобождаем отданные байты из исходящего буфера
			sender.consumePending(out.size());
		}
	}
}

/**
 * @brief Метод проверки отправки запроса без тела (endStream на заголовках)
 *
 */
TEST_F(ParserFixture, SendRequestHeadersOnlyTest){
	// Создаём объект парсера-отправителя запроса
	auto sender = this->make(direct_t::REQUEST);
	// Создаём объект парсера-приёмника запроса
	auto receiver = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/index.html")));
	// Дописываем заголовок Host
	request.emplace("Host", "anyks.com");
	// Дописываем заголовок User-Agent
	request.emplace("User-Agent", "awh");
	// Отправляем заголовки запроса с завершением сообщения (тела не будет)
	sender->sendHeaders(request, true);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Получаем объект провайдера заголовков запроса клиента
	const request_t * provider = static_cast <const request_t *> (receiver->message().provider.get());
	// Проверяем что метод запроса передан корректно
	ASSERT_EQ(provider->method, method_t::GET);
	// Проверяем что URI-адрес запроса передан корректно
	ASSERT_EQ(provider->uri, "/index.html");
	// Проверяем что версия протокола передана корректно
	ASSERT_EQ(provider->version, version_t::HTTP1_1);
	// Проверяем что переданы ровно два заголовка (Transfer-Encoding не дописан)
	ASSERT_EQ(events.headers.size(), 2u);
	// Проверяем название первого заголовка
	ASSERT_EQ(events.headers[0].first, "Host");
	// Проверяем значение первого заголовка
	ASSERT_EQ(events.headers[0].second, "anyks.com");
	// Проверяем что тело сообщения отсутствует
	ASSERT_TRUE(events.body.empty());
}

/**
 * @brief Метод проверки отправки тела фиксированного размера (Content-Length)
 *
 */
TEST_F(ParserFixture, SendIdentityBodyTest){
	// Создаём объект парсера-отправителя запроса
	auto sender = this->make(direct_t::REQUEST);
	// Создаём объект парсера-приёмника запроса
	auto receiver = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::POST, std::string("/api")));
	// Дописываем заголовок Host
	request.emplace("Host", "anyks.com");
	// Дописываем заголовок фиксированного размера тела
	request.emplace("Content-Length", "11");
	// Отправляем заголовки запроса (тело последует)
	sender->sendHeaders(request, false);
	// Отправляем тело запроса с завершением сообщения
	ASSERT_EQ(sender->sendData("hello world", 11, true), 11u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения передано корректно
	ASSERT_EQ(events.body, "hello world");
	// Проверяем что тело не кадрировалось chunked
	ASSERT_FALSE(receiver->message().flags.chunked);
	// Проверяем что размер тела передан через Content-Length
	ASSERT_EQ(receiver->message().bodySize, 11);
}

/**
 * @brief Метод проверки автоматического кадрирования chunked (без Content-Length)
 *
 */
TEST_F(ParserFixture, SendChunkedBodyTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Дописываем заголовок типа контента
	response.emplace("Content-Type", "text/plain");
	// Отправляем заголовки ответа (тело последует, Transfer-Encoding: chunked дописывается автоматически)
	sender->sendHeaders(response, false);
	// Отправляем первую порцию тела ответа
	ASSERT_EQ(sender->sendData("hello ", 6, false), 6u);
	// Отправляем вторую порцию тела ответа с завершением сообщения
	ASSERT_EQ(sender->sendData("world", 5, true), 5u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело кадрировалось chunked
	ASSERT_TRUE(receiver->message().flags.chunked);
	// Проверяем что тело сообщения передано корректно
	ASSERT_EQ(events.body, "hello world");
	// Проверяем что переданы два чанка данных и последний (нулевой) чанк
	ASSERT_EQ(events.chunks.size(), 5u);
}

/**
 * @brief Метод проверки отправки трейлеров (контейнер без провайдера в режиме chunked)
 *
 */
TEST_F(ParserFixture, SendTrailersTest){
	// Создаём объект парсера-отправителя запроса
	auto sender = this->make(direct_t::REQUEST);
	// Создаём объект парсера-приёмника запроса
	auto receiver = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::POST, std::string("/upload")));
	// Дописываем заголовок Host
	request.emplace("Host", "anyks.com");
	// Отправляем заголовки запроса (тело и трейлеры последуют)
	sender->sendHeaders(request, false);
	// Отправляем тело запроса (сообщение остаётся открытым для трейлеров)
	ASSERT_EQ(sender->sendData("hello", 5, false), 5u);
	// Формируем контейнер трейлеров (без провайдера - стартовая строка не формируется)
	headers_t trailers;
	// Дописываем трейлер контрольной суммы
	trailers.emplace("X-Checksum", "5d41402a");
	// Отправляем трейлеры с завершением сообщения
	sender->sendHeaders(trailers, true);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения передано корректно
	ASSERT_EQ(events.body, "hello");
	// Проверяем что передан ровно один трейлер
	ASSERT_EQ(events.trailers.size(), 1u);
	// Проверяем название трейлера
	ASSERT_EQ(events.trailers[0].first, "X-Checksum");
	// Проверяем значение трейлера
	ASSERT_EQ(events.trailers[0].second, "5d41402a");
}

/**
 * @brief Метод проверки pull-источника данных тела с кадрированием chunked
 *
 */
TEST_F(ParserFixture, SendDataSourceChunkedTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Уменьшаем пороги выходного буфера для многократной дозагрузки источника
	sender->sendWaterMarks(16 * 1024, 4 * 1024);
	// Формируем эталонное тело большого размера
	std::string expected(100000, '\0');
	/**
	 * Заполняем эталонное тело псевдослучайными данными
	 */
	for(size_t i = 0; i < expected.size(); ++i)
		// Формируем байт эталонного тела
		expected[i] = static_cast <char> ('A' + (i % 26));
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело последует из pull-источника данных)
	sender->sendHeaders(response, false);
	// Позиция чтения эталонного тела источником данных
	size_t position = 0;
	// Назначаем pull-источник данных тела сообщения
	sender->dataSource(parser_http_t::data_source_callback_t([&expected, &position](const uint32_t sid, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
		// Проверяем что идентификатор потока соответствует константе HTTP/1.x
		EXPECT_EQ(sid, parser_http_t::STREAM_ID);
		// Вычисляем размер выдаваемой порции данных
		const size_t size = std::min(cap, expected.size() - position);
		// Копируем порцию эталонного тела в буфер парсера
		std::memcpy(buffer, expected.data() + position, size);
		// Сдвигаем позицию чтения эталонного тела
		position += size;
		// Выставляем флаг достижения конца тела
		eof = (position == expected.size());
		// Возвращаем число записанных байт
		return static_cast <int64_t> (size);
	}));
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что источник данных выдал всё эталонное тело
	ASSERT_EQ(position, expected.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело кадрировалось chunked
	ASSERT_TRUE(receiver->message().flags.chunked);
	// Проверяем что тело сообщения передано без искажений
	ASSERT_EQ(events.body, expected);
}

/**
 * @brief Метод проверки pull-источника данных тела с кадрированием Content-Length
 *
 */
TEST_F(ParserFixture, SendDataSourceIdentityTest){
	// Создаём объект парсера-отправителя запроса
	auto sender = this->make(direct_t::REQUEST);
	// Создаём объект парсера-приёмника запроса
	auto receiver = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Формируем эталонное тело сообщения
	const std::string expected(50000, 'x');
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::PUT, std::string("/file")));
	// Дописываем заголовок Host
	request.emplace("Host", "anyks.com");
	// Дописываем заголовок фиксированного размера тела
	request.emplace("Content-Length", std::to_string(expected.size()));
	// Отправляем заголовки запроса (тело последует из pull-источника данных)
	sender->sendHeaders(request, false);
	// Позиция чтения эталонного тела источником данных
	size_t position = 0;
	// Назначаем pull-источник данных тела сообщения (eof не выставляется - конец определяется Content-Length)
	sender->dataSource(parser_http_t::data_source_callback_t([&expected, &position](const uint32_t, uint8_t * buffer, const size_t cap, bool &) noexcept -> int64_t {
		// Вычисляем размер выдаваемой порции данных
		const size_t size = std::min(cap, expected.size() - position);
		// Копируем порцию эталонного тела в буфер парсера
		std::memcpy(buffer, expected.data() + position, size);
		// Сдвигаем позицию чтения эталонного тела
		position += size;
		// Возвращаем число записанных байт
		return static_cast <int64_t> (size);
	}));
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело не кадрировалось chunked
	ASSERT_FALSE(receiver->message().flags.chunked);
	// Проверяем что тело сообщения передано без искажений
	ASSERT_EQ(events.body, expected);
}

/**
 * @brief Метод проверки push-режима записи (write callback) с pull-источником данных
 *
 */
TEST_F(ParserFixture, SendPushModeWriterTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Устанавливаем функцию обратного вызова записи исходящих байтов (push-режим)
	sender->on(parser_http_t::write_callback_t([&receiver](const void * buffer, const size_t size) noexcept {
		// Передаём исходящие байты принимающему парсеру напрямую
		receiver->parse(buffer, size);
	}));
	// Формируем эталонное тело сообщения
	const std::string expected(30000, 'z');
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело последует из pull-источника данных)
	sender->sendHeaders(response, false);
	// Позиция чтения эталонного тела источником данных
	size_t position = 0;
	// Назначаем pull-источник данных тела сообщения (в push-режиме тело качается до конца сразу)
	sender->dataSource(parser_http_t::data_source_callback_t([&expected, &position](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
		// Вычисляем размер выдаваемой порции данных
		const size_t size = std::min(cap, expected.size() - position);
		// Копируем порцию эталонного тела в буфер парсера
		std::memcpy(buffer, expected.data() + position, size);
		// Сдвигаем позицию чтения эталонного тела
		position += size;
		// Выставляем флаг достижения конца тела
		eof = (position == expected.size());
		// Возвращаем число записанных байт
		return static_cast <int64_t> (size);
	}));
	// Проверяем что в push-режиме исходящий буфер полностью опустошён
	ASSERT_TRUE(sender->pending().empty());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения передано без искажений
	ASSERT_EQ(events.body, expected);
}

/**
 * @brief Метод проверки частичного приёма sendData и сигнала writable (bounded buffer)
 *
 */
TEST_F(ParserFixture, SendWritableSignalTest){
	// Создаём объект парсера-отправителя запроса
	auto sender = this->make(direct_t::REQUEST);
	// Создаём объект парсера-приёмника запроса
	auto receiver = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Счётчик срабатываний сигнала writable
	size_t writables = 0;
	// Устанавливаем функцию обратного вызова о готовности принимать данные тела
	sender->on(parser_http_t::writable_callback_t([&writables](const uint32_t sid) noexcept {
		// Проверяем что идентификатор потока соответствует константе HTTP/1.x
		EXPECT_EQ(sid, parser_http_t::STREAM_ID);
		// Учитываем срабатывание сигнала writable
		++writables;
	}));
	// Уменьшаем пороги выходного буфера для проверки частичного приёма
	sender->sendWaterMarks(256, 64);
	// Формируем эталонное тело сообщения (больше ёмкости выходного буфера)
	const std::string expected(2000, 'q');
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::POST, std::string("/api")));
	// Дописываем заголовок фиксированного размера тела
	request.emplace("Content-Length", std::to_string(expected.size()));
	// Отправляем заголовки запроса (тело последует)
	sender->sendHeaders(request, false);
	// Позиция отправки эталонного тела
	size_t position = 0;
	/**
	 * Отправляем тело порциями: буфер ограничен, поэтому приём частичный,
	 * а продолжение отправки происходит по мере выкачивания байтов
	 */
	while(position < expected.size()){
		// Передаём остаток тела для отправки
		const size_t accepted = sender->sendData(expected.data() + position, expected.size() - position, true);
		// Сдвигаем позицию отправки на число принятых байт
		position += accepted;
		// Если буфер заполнен - выкачиваем накопленные исходящие байты
		if(accepted == 0){
			// Получаем ещё не отправленные исходящие байты
			const std::string_view out = sender->pending();
			// Проверяем что исходящие байты действительно накоплены
			ASSERT_FALSE(out.empty());
			// Передаём исходящие байты принимающему парсеру
			receiver->parse(out.data(), out.size());
			// Освобождаем отданные байты из исходящего буфера (сработает сигнал writable)
			sender->consumePending(out.size());
		}
	}
	// Выкачиваем остаток исходящих байтов
	::drain(* sender, * receiver);
	// Проверяем что сигнал writable срабатывал
	ASSERT_GT(writables, 0u);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения передано без искажений
	ASSERT_EQ(events.body, expected);
}

/**
 * @brief Метод проверки сырого тела до закрытия соединения (HTTP/1.0 без Content-Length)
 *
 */
TEST_F(ParserFixture, SendHttp10RawBodyTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Формируем контейнер заголовков ответа HTTP/1.0 без Content-Length
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_0, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело кадрируется закрытием соединения, chunked в HTTP/1.0 не существует)
	sender->sendHeaders(response, false);
	// Отправляем тело ответа с завершением сообщения
	ASSERT_EQ(sender->sendData("raw body", 8, true), 8u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Уведомляем принимающий парсер о закрытии соединения (конец тела "до закрытия")
	receiver->eof();
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело не кадрировалось chunked
	ASSERT_FALSE(receiver->message().flags.chunked);
	// Проверяем что тело сообщения передано корректно
	ASSERT_EQ(events.body, "raw body");
}

/**
 * @brief Метод проверки отправки двух сообщений подряд в одном соединении (keep-alive)
 *
 */
TEST_F(ParserFixture, SendKeepAliveSequenceTest){
	// Создаём объект парсера-отправителя запроса
	auto sender = this->make(direct_t::REQUEST);
	// Создаём объект парсера-приёмника запроса
	auto receiver = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Формируем контейнер заголовков первого запроса с провайдером
	headers_t first(std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/first")));
	// Дописываем заголовок Host
	first.emplace("Host", "anyks.com");
	// Отправляем заголовки первого запроса с завершением сообщения
	sender->sendHeaders(first, true);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что первое сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что URI-адрес первого запроса передан корректно
	ASSERT_EQ(static_cast <const request_t *> (receiver->message().provider.get())->uri, "/first");
	// Сбрасываем принимающий парсер для разбора следующего сообщения
	receiver->reset();
	// Формируем контейнер заголовков второго запроса с провайдером
	headers_t second(std::make_unique <request_t> (version_t::HTTP1_1, method_t::POST, std::string("/second")));
	// Дописываем заголовок Host
	second.emplace("Host", "anyks.com");
	// Дописываем заголовок фиксированного размера тела
	second.emplace("Content-Length", "3");
	// Отправляем заголовки второго запроса (отправитель сбрасывается автоматически - первое сообщение завершено)
	sender->sendHeaders(second, false);
	// Отправляем тело второго запроса с завершением сообщения
	ASSERT_EQ(sender->sendData("abc", 3, true), 3u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что второе сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что URI-адрес второго запроса передан корректно
	ASSERT_EQ(static_cast <const request_t *> (receiver->message().provider.get())->uri, "/second");
	// Проверяем что тело второго запроса передано корректно
	ASSERT_EQ(events.body, "abc");
}
