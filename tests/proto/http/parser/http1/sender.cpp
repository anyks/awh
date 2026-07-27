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
 * @brief Тесты сборки исходящих сообщений HTTP/1.x — проверка формирования стартовой строки,
 *        заголовков и тела с кадрированием chunked и Content-Length
 *
 * @copyright: Copyright © 2026
 *
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
 * @brief Внутренние вспомогательные функции
 *
 */
namespace {
	/**
	 * @brief Функция перекачки исходящих байтов отправителя в принимающий парсер (pull-модель)
	 *
	 * @param sender   объект парсера-отправителя
	 * @param receiver объект парсера-приёмника
	 *
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

/**
 * @brief Метод проверки сигнала writable в push-модели (установлена функция обратного вызова записи)
 *
 */
TEST_F(ParserFixture, SendPushModeWritableSignalTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Собранный сетевым слоем поток исходящих байтов
	std::string wire;
	// Счётчик срабатываний сигнала writable
	size_t writables = 0;
	// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
	sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
		// Накапливаем исходящие байты сетевого слоя
		wire.append(static_cast <const char *> (buffer), size);
	}));
	// Устанавливаем функцию обратного вызова о готовности принимать данные тела
	sender->on(parser_http_t::writable_callback_t([&writables](const uint32_t sid) noexcept {
		// Проверяем что идентификатор потока соответствует константе HTTP/1.x
		EXPECT_EQ(sid, parser_http_t::STREAM_ID);
		// Учитываем срабатывание сигнала writable
		++writables;
	}));
	// Уменьшаем пороги выходного буфера, чтобы приём тела оказался частичным
	sender->sendWaterMarks(1024, 512);
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело последует)
	sender->sendHeaders(response, false);
	// Формируем тело заведомо большего размера, чем ёмкость выходного буфера
	const std::string body(4096, 'x');
	// Передаём тело для отправки (приём заведомо частичный)
	const size_t accepted = sender->sendData(body.data(), body.size(), true);
	// Проверяем что приём оказался частичным
	ASSERT_LT(accepted, body.size());
	/**
	 * Проверяем что сигнал готовности принимать данные подан: в push-модели выборка
	 * consumePending не выполняется, и без сигнала отправка встала бы навсегда
	 */
	ASSERT_GT(writables, 0u);
	// Объём тела, принятый отправителем
	size_t sent = accepted;
	/**
	 * Досылаем остаток тела порциями: приём за один вызов ограничен ёмкостью
	 * выходного буфера, а продолжение выдачи разрешает именно сигнал writable
	 */
	while(sent < body.size()){
		// Передаём очередную порцию остатка тела
		const size_t portion = sender->sendData((body.data() + sent), (body.size() - sent), true);
		// Проверяем что выдача продвигается
		ASSERT_GT(portion, 0u);
		// Наращиваем объём принятого тела
		sent += portion;
	}
	// Проверяем что сигнал готовности подавался на каждый провал выходного буфера
	ASSERT_GT(writables, 1u);
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Разбираем собранный сетевым слоем поток исходящих байтов
	receiver->parse(wire.data(), wire.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело целиком ушло в сетевой слой без искажений
	ASSERT_EQ(events.body, body);
}

/**
 * @brief Метод проверки запрета досрочного завершения тела фиксированного размера
 *
 */
TEST_F(ParserFixture, SendIdentityShortBodyTest){
	// Создаём объект парсера-отправителя запроса
	auto sender = this->make(direct_t::REQUEST);
	// Формируем контейнер заголовков запроса с провайдером
	headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::POST, std::string("/api")));
	// Дописываем заголовок Host
	request.emplace("Host", "anyks.com");
	// Дописываем заголовок фиксированного размера тела
	request.emplace("Content-Length", "10");
	// Отправляем заголовки запроса (тело последует)
	sender->sendHeaders(request, false);
	// Отправляем часть тела с преждевременным признаком завершения сообщения
	ASSERT_EQ(sender->sendData("abcde", 5, true), 5u);
	/**
	 * Проверяем что отправитель не считает сообщение завершённым: анонсировано
	 * десять байт, а выдано пять - досрочное завершение отправило бы усечённое тело
	 */
	ASSERT_EQ(sender->sendData("fghij", 5, true), 5u);
	// Получаем сформированный отправителем поток исходящих байтов
	const std::string wire(sender->pending());
	// Проверяем что тело ушло целиком
	ASSERT_NE(wire.find("abcdefghij"), std::string::npos);
	// Проверяем что дальнейшая выдача тела уже не принимается
	ASSERT_EQ(sender->sendData("xyz", 3, true), 0u);
}

/**
 * @brief Метод проверки исключения одновременной отправки Content-Length и Transfer-Encoding
 *
 */
TEST_F(ParserFixture, SendFramingConflictTest){
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
	request.emplace("Content-Length", "5");
	// Дописываем конфликтующий заголовок транспортного кодирования
	request.emplace("Transfer-Encoding", "chunked");
	// Отправляем заголовки запроса (тело последует)
	sender->sendHeaders(request, false);
	// Отправляем тело запроса с завершением сообщения
	ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	/**
	 * Проверяем что принимающая сторона не отвергла сообщение: конфликтующий
	 * заголовок вычищен, а кадрирование соответствует объявленному
	 */
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело передано корректно
	ASSERT_EQ(events.body, "hello");
	// Проверяем что тело кадрировано фиксированным размером
	ASSERT_EQ(receiver->message().bodySize, 5);
}

/**
 * @brief Метод проверки дополнения Transfer-Encoding токеном chunked
 *
 */
TEST_F(ParserFixture, SendEncodingWithoutChunkedTest){
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
	// Дописываем транспортное кодирование без завершающего токена chunked
	response.emplace("Transfer-Encoding", "gzip");
	// Отправляем заголовки ответа (тело последует)
	sender->sendHeaders(response, false);
	// Отправляем тело ответа с завершением сообщения
	ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	/**
	 * Проверяем что принимающая сторона разобрала сообщение: тело кадрировано
	 * chunked, и объявление транспортного кодирования этому соответствует
	 */
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело кадрировалось chunked
	ASSERT_TRUE(receiver->message().flags.chunked);
	// Проверяем что тело передано корректно
	ASSERT_EQ(events.body, "hello");
}

/**
 * @brief Метод проверки назначения pull-источника данных до отправки заголовков
 *
 */
TEST_F(ParserFixture, SendDataSourceBeforeHeadersTest){
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
	// Формируем эталонное тело сообщения
	const std::string expected = "AWH is awesome!";
	// Позиция чтения эталонного тела источником данных
	size_t position = 0;
	// Назначаем pull-источник данных тела ДО отправки заголовков
	sender->dataSource(parser_http_t::data_source_callback_t([&expected, &position](const uint32_t sid, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
		// Вычисляем размер выдаваемой порции данных
		const size_t size = std::min(cap, (expected.size() - position));
		// Копируем порцию эталонного тела в буфер парсера
		std::memcpy(buffer, (expected.data() + position), size);
		// Сдвигаем позицию чтения эталонного тела
		position += size;
		// Выставляем флаг достижения конца тела
		eof = (position == expected.size());
		// Возвращаем число записанных байт
		return static_cast <int64_t> (size);
	}));
	/**
	 * До отправки заголовков прокачка продвинуться не может, и признак незавершённой
	 * отправки обязан быть ложным: иначе документированный цикл дозагрузки, запущенный
	 * сетевым слоем до отправки заголовков, крутился бы вхолостую
	 */
	ASSERT_FALSE(sender->sourcePending());
	// Проверяем что возобновление прокачки тоже сообщает о невозможности продвинуться
	ASSERT_FALSE(sender->resumeSource());
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело последует из ранее назначенного источника)
	sender->sendHeaders(response, false);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что источник данных не был потерян при отправке заголовков
	ASSERT_EQ(position, expected.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения передано без искажений
	ASSERT_EQ(events.body, expected);
}

/**
 * @brief Метод проверки устойчивости отправителя к некорректному Content-Length
 *
 */
TEST_F(ParserFixture, SendInvalidContentLengthTest){
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
	// Дописываем некорректный заголовок фиксированного размера тела
	request.emplace("Content-Length", "abc");
	// Отправляем заголовки запроса (тело последует)
	sender->sendHeaders(request, false);
	// Отправляем тело запроса с завершением сообщения
	ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	/**
	 * Проверяем что принимающая сторона разобрала сообщение: некорректный заголовок
	 * вычищен, а тело кадрировано способом, не требующим заранее известной длины
	 */
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело кадрировалось chunked
	ASSERT_TRUE(receiver->message().flags.chunked);
	// Проверяем что тело передано корректно
	ASSERT_EQ(events.body, "hello");
	/**
	 * Выполняем перебор всех разобранных заголовков сообщения
	 */
	for(const auto & header : events.headers)
		// Проверяем что некорректный заголовок на провод не ушёл
		ASSERT_STRNE(header.first.c_str(), "Content-Length");
}

/**
 * @brief Метод проверки определения кадрирования по последнему заголовку Transfer-Encoding
 *
 */
TEST_F(ParserFixture, SendMultipleEncodingHeadersTest){
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
	// Дописываем первое транспортное кодирование
	response.emplace("Transfer-Encoding", "gzip");
	// Дописываем завершающее транспортное кодирование
	response.emplace("Transfer-Encoding", "chunked");
	// Отправляем заголовки ответа (тело последует)
	sender->sendHeaders(response, false);
	// Отправляем тело ответа с завершением сообщения
	ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	/**
	 * Проверяем что дублирующий токен chunked не дописан: он сделал бы кадрирование
	 * некорректным, и принимающая сторона отвергла бы сообщение
	 */
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело передано корректно
	ASSERT_EQ(events.body, "hello");
}

/**
 * @brief Метод проверки возобновляемой прокачки pull-источника данных в push-модели
 *
 */
TEST_F(ParserFixture, SendDataSourceResumeTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Собранный сетевым слоем поток исходящих байтов
	std::string wire;
	// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
	sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
		// Накапливаем исходящие байты сетевого слоя
		wire.append(static_cast <const char *> (buffer), size);
	}));
	// Уменьшаем пороги выходного буфера отправки
	sender->sendWaterMarks(8 * 1024, 4 * 1024);
	// Уменьшаем объём одной прокачки pull-источника данных
	sender->pumpLimit(4 * 1024);
	// Формируем эталонное тело заведомо большего размера, чем объём одной прокачки
	std::string expected(100000, '\0');
	/**
	 * Заполняем эталонное тело псевдослучайными данными
	 */
	for(size_t i = 0; i < expected.size(); ++i)
		// Формируем байт эталонного тела
		expected[i] = static_cast <char> ('A' + (i % 26));
	// Позиция чтения эталонного тела источником данных
	size_t position = 0;
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело последует из pull-источника данных)
	sender->sendHeaders(response, false);
	// Назначаем pull-источник данных тела сообщения
	sender->dataSource(parser_http_t::data_source_callback_t([&expected, &position](const uint32_t sid, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
		// Вычисляем размер выдаваемой порции данных
		const size_t size = std::min(cap, (expected.size() - position));
		// Копируем порцию эталонного тела в буфер парсера
		std::memcpy(buffer, (expected.data() + position), size);
		// Сдвигаем позицию чтения эталонного тела
		position += size;
		// Выставляем флаг достижения конца тела
		eof = (position == expected.size());
		// Возвращаем число записанных байт
		return static_cast <int64_t> (size);
	}));
	/**
	 * Проверяем что первая прокачка ограничена лимитом: тело целиком не выкачано
	 * и управление возвращено сетевому слою
	 */
	ASSERT_LT(position, expected.size());
	// Проверяем что отправка тела помечена незавершённой
	ASSERT_TRUE(sender->sourcePending());
	// Счётчик прокачек тела из источника данных
	size_t rounds = 0;
	/**
	 * Продолжаем отправку тела по готовности сокета к записи
	 */
	while(sender->resumeSource()){
		// Учитываем очередную прокачку тела
		++rounds;
		// Проверяем что прокачка не зациклилась
		ASSERT_LT(rounds, 1000u);
	}
	// Проверяем что потребовалось несколько прокачек
	ASSERT_GT(rounds, 1u);
	// Проверяем что отправка тела завершена
	ASSERT_FALSE(sender->sourcePending());
	// Проверяем что источник данных выдал всё эталонное тело
	ASSERT_EQ(position, expected.size());
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Разбираем собранный сетевым слоем поток исходящих байтов
	receiver->parse(wire.data(), wire.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения передано без искажений
	ASSERT_EQ(events.body, expected);
}

/**
 * @brief Метод проверки вычистки Content-Length у запроса без тела
 *
 */
TEST_F(ParserFixture, SendBodylessContentLengthTest){
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
	// Дописываем заголовок размера тела, которого не будет
	request.emplace("Content-Length", "5");
	// Отправляем заголовки запроса с завершением сообщения (тела не будет)
	sender->sendHeaders(request, true);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	/**
	 * Проверяем что принимающая сторона считает сообщение завершённым: объявленный
	 * размер тела остался бы на проводе и получатель ждал бы недостающие байты
	 */
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что размер тела не объявлен
	ASSERT_EQ(receiver->message().bodySize, -1);
	/**
	 * Выполняем перебор всех разобранных заголовков сообщения
	 */
	for(const auto & header : events.headers)
		// Проверяем что заголовок размера тела на провод не ушёл
		ASSERT_STRNE(header.first.c_str(), "Content-Length");
}

/**
 * @brief Метод проверки вычистки запрещённых полей из исходящих трейлеров
 *
 */
TEST_F(ParserFixture, SendForbiddenTrailersTest){
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
	// Отправляем заголовки запроса (тело последует в кодировке chunked)
	sender->sendHeaders(request, false);
	// Отправляем тело запроса без завершения сообщения
	ASSERT_EQ(sender->sendData("hello", 5, false), 5u);
	// Формируем контейнер трейлеров без провайдера
	headers_t trailers;
	// Дописываем запрещённое в трейлерах поле кадрирования
	trailers.emplace("Content-Length", "100");
	// Дописываем запрещённое в трейлерах поле управления соединением
	trailers.emplace("Keep-Alive", "timeout=5");
	// Дописываем разрешённое поле трейлера
	trailers.emplace("X-Check", "done");
	// Отправляем блок трейлеров (завершает тело последним чанком)
	sender->sendHeaders(trailers, false);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело передано корректно
	ASSERT_EQ(events.body, "hello");
	// Проверяем что до потребителя дошёл единственный разрешённый трейлер
	ASSERT_EQ(events.trailers.size(), 1u);
	// Проверяем что разрешённый трейлер передан без искажений
	ASSERT_EQ(events.trailers.front().first, "X-Check");
}

/**
 * @brief Метод проверки вычистки всех категорий непригодных полей из исходящих трейлеров
 *
 * @details RFC 9110 §6.5.1 запрещает отправителю формировать трейлер, если определение
 *          соответствующего заголовка не разрешает его передачу в трейлерах. Проверить
 *          такое разрешение по определению поля библиотека не может, поэтому применяется
 *          отбраковка по категориям, названным стандартом непригодными. Проверяется, что
 *          на провод не уходит ни одно поле из этих категорий и что пригодные поля
 *          доходят до получателя без искажений
 *
 */
TEST_F(ParserFixture, SendForbiddenTrailerCategoriesTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело последует в кодировке chunked)
	sender->sendHeaders(response, false);
	// Отправляем тело ответа без завершения сообщения
	ASSERT_EQ(sender->sendData("hello", 5, false), 5u);
	// Формируем перечень непригодных для трейлеров полей по одному представителю каждой категории RFC
	const std::vector <std::string> forbidden = {
		// Поля кадрирования сообщения
		"Content-Length", "Transfer-Encoding",
		// Поля маршрутизации и управления соединением
		"Host", "Connection", "Keep-Alive", "Proxy-Connection", "Upgrade", "TE", "Trailer",
		// Модификаторы запроса: управляющие поля и условные
		"Cache-Control", "Expect", "Max-Forwards", "Pragma", "Range",
		"If-Match", "If-None-Match", "If-Modified-Since", "If-Unmodified-Since", "If-Range",
		// Поля аутентификации
		"Authorization", "Proxy-Authorization", "WWW-Authenticate", "Proxy-Authenticate",
		"Authentication-Info", "Proxy-Authentication-Info", "Cookie", "Set-Cookie",
		// Управляющие данные ответа
		"Age", "Date", "Expires", "Location", "Retry-After", "Vary", "Warning",
		// Поля, определяющие способ обработки содержимого
		"Content-Type", "Content-Encoding", "Content-Range"
	};
	// Формируем контейнер трейлеров без провайдера
	headers_t trailers;
	/**
	 * Выполняем перебор всех непригодных для трейлеров полей
	 */
	for(auto & name : forbidden)
		// Дописываем непригодное для трейлеров поле в контейнер
		trailers.emplace(name, "value");
	// Дописываем пригодное для трейлеров поле контрольной суммы
	trailers.emplace("X-Checksum", "abc");
	// Дописываем пригодное для трейлеров поле статуса постобработки
	trailers.emplace("X-Status", "ok");
	// Отправляем блок трейлеров (завершает тело последним чанком)
	sender->sendHeaders(trailers, false);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело передано корректно
	ASSERT_EQ(events.body, "hello");
	// Проверяем что до получателя дошли оба пригодных трейлера и только они
	ASSERT_EQ(events.trailers.size(), 2u);
	// Проверяем что первый пригодный трейлер передан без искажений
	ASSERT_EQ(events.trailers[0].first, "X-Checksum");
	// Проверяем что второй пригодный трейлер передан без искажений
	ASSERT_EQ(events.trailers[1].first, "X-Status");
}

/**
 * @brief Метод проверки согласованности лимита памяти буферов при клонировании
 *
 */
TEST_F(ParserFixture, CloneWaterMarksTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Устанавливаем пороги выходного буфера заведомо выше лимита памяти по умолчанию
	sender->sendWaterMarks(32 * 1024 * 1024, 16 * 1024 * 1024);
	// Выполняем клонирование объекта парсера
	auto clone = sender->clone();
	// Проверяем что клон создан
	ASSERT_NE(clone, nullptr);
	// Получаем объект клонированного парсера-отправителя
	auto * parser = static_cast <parser_http_t *> (clone.get());
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Отправляем заголовки ответа (тело последует)
	parser->sendHeaders(response, false);
	// Формируем тело заведомо большего размера, чем лимит памяти буфера по умолчанию
	const std::string body(12 * 1024 * 1024, 'x');
	/**
	 * Проверяем что клон принимает тело целиком: без согласования лимита памяти
	 * с порогами буфер клона упёрся бы в значение по умолчанию
	 */
	ASSERT_EQ(parser->sendData(body.data(), body.size(), true), body.size());
}

/**
 * @brief Метод проверки отбрасывания трейлеров после завершения сообщения
 *
 * @details Контейнер без провайдера является блоком трейлеров, и стартовую строку
 *          формировать из него не из чего. Если сообщение уже завершено - флагом
 *          endStream, исчерпанием Content-Length либо концом тела pull-источника -
 *          такой блок обязан отбрасываться. Иначе он уходит на провод голыми
 *          полями, а получатель читает их как начало следующего сообщения и
 *          рассинхронизирует кадрирование
 *
 */
TEST_F(ParserFixture, SendTrailersAfterMessageEndTest){
	/**
	 * @brief Функция сборки сообщения с попыткой дослать трейлеры после его завершения
	 *
	 * @param sender объект парсера-отправителя
	 * @param wire   собранные байты исходящего сообщения
	 * @param source признак подачи тела pull-источником вместо sendData
	 * @return       количество октетов, дописанных после завершения сообщения
	 *
	 */
	auto attempt = [](parser_http_t & sender, std::string & wire, const bool source) noexcept -> size_t {
		// Формируем контейнер заголовков ответа с провайдером
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Дописываем заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "chunked");
		// Отправляем заголовки ответа (тело последует)
		sender.sendHeaders(response, false);
		// Тело отправляемого сообщения
		static const std::string body = "hello";
		// Если тело подаётся pull-источником
		if(source){
			// Позиция чтения тела сообщения источником
			static size_t position = 0;
			// Сбрасываем позицию чтения тела сообщения
			position = 0;
			// Устанавливаем pull-источник данных тела сообщения
			sender.dataSource(parser_http_t::data_source_callback_t([](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
				// Определяем размер выдаваемой источником порции тела
				const size_t size = std::min(cap, (body.size() - position));
				// Копируем очередную порцию тела сообщения
				std::memcpy(buffer, (body.data() + position), size);
				// Смещаем позицию чтения тела сообщения
				position += size;
				// Устанавливаем признак достижения конца тела сообщения
				eof = (position >= body.size());
				// Выводим размер выданной порции тела
				return static_cast <int64_t> (size);
			}));
			/**
			 * Прокачиваем pull-источник до исчерпания тела сообщения
			 */
			while(sender.sourcePending()){
				// Если прокачка источника не возобновилась
				if(!sender.resumeSource())
					// Прекращаем прокачку источника
					break;
			}
		// Если тело подаётся напрямую с завершением сообщения
		} else sender.sendData(body.data(), body.size(), true);
		// Запоминаем объём провода до попытки дослать трейлеры
		const size_t before = wire.size();
		// Формируем контейнер трейлеров сообщения
		headers_t trailers;
		// Дописываем трейлер контрольной суммы
		trailers.emplace("X-Checksum", "abc123");
		// Пытаемся отправить трейлеры уже завершённому сообщению
		sender.sendHeaders(trailers, true);
		// Выводим количество октетов, дописанных после завершения сообщения
		return (wire.size() - before);
	};
	/**
	 * Проверяем оба способа завершения тела сообщения
	 */
	for(const bool source : {false, true}){
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Выполняем сборку сообщения с попыткой дослать трейлеры
		const size_t appended = attempt(* sender, wire, source);
		// Проверяем что после завершения сообщения на провод не ушло ни одного октета
		ASSERT_EQ(appended, 0u) << (source ? "pull-источник" : "sendData");
		// Создаём объект парсера-приёмника собранного сообщения
		auto receiver = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		receiver->method(method_t::GET);
		// Выполняем разбор собранного сообщения
		receiver->parse(wire.data(), wire.size());
		// Проверяем что собранное сообщение разбирается целиком и без остатка
		ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE) << (source ? "pull-источник" : "sendData");
	}
}

/**
 * @brief Метод проверки завершения объявленного кадрирования chunked при пустом теле
 *
 * @details Если вызывающая сторона объявила Transfer-Encoding: chunked и сразу
 *          завершила сообщение флагом endStream, тело обязано быть завершено нулевым
 *          чанком. Блок заголовков сам по себе конца сообщения не обозначает, и без
 *          нулевого чанка получатель ждал бы тело до закрытия соединения
 *
 */
TEST_F(ParserFixture, SendChunkedWithoutBodyTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Создаём объект парсера-приёмника ответа
	auto receiver = this->make(direct_t::RESPONSE);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Создаём объект сборщика событий парсера-приёмника
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера-приёмника
	this->attach(* receiver, events);
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Дописываем заголовок кодирования тела сообщения
	response.emplace("Transfer-Encoding", "chunked");
	// Отправляем заголовки ответа с завершением сообщения (тела не будет)
	sender->sendHeaders(response, true);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что сообщение полностью разобрано, а не осталось в ожидании тела
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело сообщения пустое
	ASSERT_TRUE(events.body.empty());
	// Проверяем что кадрирование тела определено получателем как chunked
	ASSERT_TRUE(receiver->message().flags.chunked);
}

/**
 * @brief Метод проверки разрешения конфликта кадрирования при завершении заголовками
 *
 * @details Одновременная отправка Content-Length и Transfer-Encoding запрещена
 *          (RFC 9112 §6.1) независимо от наличия тела: получатель обязан отвергнуть
 *          такой кадр как попытку request smuggling. Путь с телом конфликт разрешал,
 *          путь с завершением заголовками - нет, и на провод уходили оба заголовка
 *
 */
TEST_F(ParserFixture, SendConflictingFramingWithoutBodyTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Собранные байты исходящего сообщения
	std::string wire;
	// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
	sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
		// Собираем отданные сетевому слою байты
		wire.append(static_cast <const char *> (buffer), size);
	}));
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Дописываем заголовок размера тела сообщения
	response.emplace("Content-Length", "10");
	// Дописываем конфликтующий заголовок кодирования тела сообщения
	response.emplace("Transfer-Encoding", "chunked");
	// Отправляем заголовки ответа с завершением сообщения
	sender->sendHeaders(response, true);
	// Проверяем что конфликтующий заголовок кадрирования вычищен из блока
	ASSERT_EQ(wire.find("Transfer-Encoding"), std::string::npos);
	// Проверяем что заголовок размера тела на проводе сохранён
	ASSERT_NE(wire.find("Content-Length: 10"), std::string::npos);
	// Определяем позицию конца блока заголовков
	const size_t block = wire.find("\r\n\r\n");
	// Проверяем что блок заголовков на проводе завершён
	ASSERT_NE(block, std::string::npos);
	/**
	 * Проверяем что за блоком заголовков на проводе нет ничего: искать подстроку
	 * нулевого чанка бессмысленно - она совпадает с хвостом "Content-Length: 10"
	 * вместе с завершающей блок пустой строкой
	 */
	ASSERT_EQ((block + 4), wire.size());
	// Создаём объект парсера-приёмника собранного сообщения
	auto receiver = this->make(direct_t::RESPONSE);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Выполняем разбор собранного сообщения
	receiver->parse(wire.data(), wire.size());
	// Проверяем что собранный кадр не отвергается как конфликт кадрирования
	ASSERT_NE(receiver->error(), parser_http_t::error_t::CONTENT_LENGTH_CONFLICT);
}

/**
 * @brief Метод проверки отсутствия нулевого чанка в HTTP/1.0
 *
 * @details Кодирования chunked в HTTP/1.0 не существует, и нулевой чанк оказался бы
 *          для получателя частью тела, а не признаком его конца
 *
 */
TEST_F(ParserFixture, SendChunkedWithoutBodyLegacyTest){
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Собранные байты исходящего сообщения
	std::string wire;
	// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
	sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
		// Собираем отданные сетевому слою байты
		wire.append(static_cast <const char *> (buffer), size);
	}));
	// Формируем контейнер заголовков ответа версии HTTP/1.0
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_0, static_cast <uint16_t> (200)));
	// Дописываем заголовок кодирования тела сообщения
	response.emplace("Transfer-Encoding", "chunked");
	// Отправляем заголовки ответа с завершением сообщения
	sender->sendHeaders(response, true);
	// Определяем позицию конца блока заголовков
	const size_t block = wire.find("\r\n\r\n");
	// Проверяем что блок заголовков на проводе завершён
	ASSERT_NE(block, std::string::npos);
	// Проверяем что за блоком заголовков на проводе нет нулевого чанка
	ASSERT_EQ((block + 4), wire.size());
}

/**
 * @brief Метод проверки вычистки заголовка Transfer-Encoding из исходящего HTTP/1.0
 *
 * @details Заголовок Transfer-Encoding появился в HTTP/1.1, и сообщение HTTP/1.0 с этим
 *          заголовком получатель обязан считать сообщением с неисправным кадрированием
 *          даже при наличии Content-Length (RFC 9112 §6.1). Тело такого сообщения
 *          кадрируется закрытием соединения, поэтому объявление кодирования обязано
 *          сниматься с провода - иначе оно противоречит тому, чем тело кадрировано,
 *          и собственный приёмник уходит разбирать сырые байты как размеры чанков
 *
 */
TEST_F(ParserFixture, SendTransferEncodingLegacyTest){
	/**
	 * @brief Функция сборки исходящего сообщения HTTP/1.0 с объявленным кодированием
	 *
	 * @param sender    объект парсера-отправителя
	 * @param wire      собранные байты исходящего сообщения
	 * @param endStream флаг завершения сообщения блоком заголовков
	 *
	 */
	auto build = [](parser_http_t & sender, std::string & wire, const bool endStream) noexcept -> void {
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender.on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Формируем контейнер заголовков ответа версии HTTP/1.0
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_0, static_cast <uint16_t> (200)));
		// Дописываем заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "chunked");
		// Отправляем заголовки ответа
		sender.sendHeaders(response, endStream);
		// Если сообщение телом не завершается
		if(!endStream)
			// Отправляем тело ответа с завершением сообщения
			sender.sendData("hello", 5, true);
	};
	/**
	 * Проверяем сообщение, завершаемое блоком заголовков
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Собираем исходящее сообщение без тела
		build(* sender, wire, true);
		// Проверяем что объявление кодирования на провод не ушло
		ASSERT_EQ(wire.find("Transfer-Encoding"), std::string::npos) << wire;
		// Определяем позицию конца блока заголовков
		const size_t block = wire.find("\r\n\r\n");
		// Проверяем что блок заголовков на проводе завершён
		ASSERT_NE(block, std::string::npos) << wire;
		// Проверяем что за блоком заголовков на проводе нет нулевого чанка
		ASSERT_EQ((block + 4), wire.size()) << wire;
	}
	/**
	 * Проверяем сообщение с телом: тело HTTP/1.0 кадрируется закрытием соединения
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Собираем исходящее сообщение с телом
		build(* sender, wire, false);
		// Проверяем что объявление кодирования на провод не ушло
		ASSERT_EQ(wire.find("Transfer-Encoding"), std::string::npos) << wire;
		// Определяем позицию конца блока заголовков
		const size_t block = wire.find("\r\n\r\n");
		// Проверяем что блок заголовков на проводе завершён
		ASSERT_NE(block, std::string::npos) << wire;
		// Проверяем что тело ушло сырыми байтами, без разметки чанков
		ASSERT_EQ(wire.substr(block + 4), "hello") << wire;
	}
}

/**
 * @brief Метод проверки отказа кадрировать тело запроса HTTP/1.0 без Content-Length
 *
 * @details Кодирования chunked в HTTP/1.0 не существует, а тело запроса, в отличие от
 *          тела ответа, закрытием соединения не ограничивается: запрос без Content-Length
 *          и без Transfer-Encoding получатель обязан считать запросом без тела
 *          (RFC 9112 §6.3). Отданные следом байты он прочитает как начало следующего
 *          запроса - классическая рассинхронизация кадрирования. Кадрировать такое тело
 *          нечем, поэтому оно не принимается вовсе, а вызывающая сторона узнаёт об отказе
 *          по нулю, возвращённому методом отправки тела. Тело ответа сервера при тех же
 *          условиях кадрируется закрытием соединения и отправляется штатно
 *
 */
TEST_F(ParserFixture, SendLegacyRequestWithoutLengthTest){
	/**
	 * Проверяем запрос: кадрировать тело нечем
	 */
	{
		// Создаём объект парсера-отправителя запроса
		auto sender = this->make(direct_t::REQUEST);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Формируем контейнер заголовков запроса версии HTTP/1.0 без объявления размера тела
		headers_t request(std::make_unique <request_t> (version_t::HTTP1_0, method_t::POST, std::string("/upload")));
		// Дописываем заголовок Host
		request.emplace("Host", "anyks.com");
		// Отправляем заголовки запроса (тело последует)
		sender->sendHeaders(request, false);
		// Проверяем что тело запроса к отправке не принято
		ASSERT_EQ(sender->sendData("hello", 5, true), 0u);
		// Определяем позицию конца блока заголовков
		const size_t block = wire.find("\r\n\r\n");
		// Проверяем что блок заголовков на проводе завершён
		ASSERT_NE(block, std::string::npos) << wire;
		// Проверяем что за блоком заголовков на проводе нет некадрированных байт тела
		ASSERT_EQ((block + 4), wire.size()) << wire;
		/**
		 * Проверяем что отправитель не залип: на проводе уже лежит законченный запрос
		 * без тела, поэтому сообщение считается завершённым и следующее обязано уходить.
		 * Иначе отправитель ждал бы тела, которое принять не может, а вызывающая сторона
		 * не получала бы об этом никакого признака
		 */
		headers_t next(std::make_unique <request_t> (version_t::HTTP1_0, method_t::POST, std::string("/second")));
		// Дописываем заголовок Host следующего запроса
		next.emplace("Host", "anyks.com");
		// Объявляем размер тела следующего запроса
		next.emplace("Content-Length", "5");
		// Отправляем заголовки следующего запроса
		sender->sendHeaders(next, false);
		// Проверяем что тело следующего запроса принято к отправке целиком
		ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
		// Проверяем что следующий запрос ушёл на провод вместе с телом
		ASSERT_NE(wire.find("POST /second HTTP/1.0\r\n"), std::string::npos) << wire;
		// Проверяем что тело следующего запроса ушло на провод
		ASSERT_EQ(wire.compare((wire.size() - 5), 5, "hello"), 0) << wire;
	}
	/**
	 * Проверяем запрос с pull-источником: источник обязан отвергаться так же,
	 * как отвергается тело, поданное методом отправки - иначе он остаётся
	 * обходом запрета, выдающим на провод некадрированные байты
	 */
	{
		// Создаём объект парсера-отправителя запроса
		auto sender = this->make(direct_t::REQUEST);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Формируем контейнер заголовков запроса версии HTTP/1.0 без объявления размера тела
		headers_t request(std::make_unique <request_t> (version_t::HTTP1_0, method_t::POST, std::string("/upload")));
		// Дописываем заголовок Host
		request.emplace("Host", "anyks.com");
		// Отправляем заголовки запроса (тело последует)
		sender->sendHeaders(request, false);
		// Отдаваемое источником тело сообщения
		const std::string expected = "hello";
		// Позиция чтения тела сообщения источником
		size_t position = 0;
		// Устанавливаем pull-источник данных тела сообщения
		sender->dataSource(parser_http_t::data_source_callback_t([&expected, &position](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
			// Определяем размер отдаваемой порции тела
			const size_t size = std::min(cap, (expected.size() - position));
			// Копируем порцию тела в буфер источника
			std::memcpy(buffer, expected.data() + position, size);
			// Смещаем позицию чтения тела
			position += size;
			// Определяем достижение конца тела
			eof = (position >= expected.size());
			// Выводим размер отданной порции тела
			return static_cast <int64_t> (size);
		}));
		/**
		 * Источник на провод ничего не выдаёт: сообщение завершено отказом, и прокачка
		 * продвинуться не может. Признак незавершённой отправки обязан быть ложным -
		 * иначе документированный цикл дозагрузки крутился бы вхолостую, а это хуже
		 * тихого отказа: сетевой слой занимал бы процессор без всякого продвижения
		 */
		ASSERT_FALSE(sender->sourcePending());
		// Проверяем что возобновление прокачки тоже сообщает о невозможности продвинуться
		ASSERT_FALSE(sender->resumeSource());
		// Определяем позицию конца блока заголовков
		const size_t block = wire.find("\r\n\r\n");
		// Проверяем что блок заголовков на проводе завершён
		ASSERT_NE(block, std::string::npos) << wire;
		// Проверяем что за блоком заголовков на проводе нет некадрированных байт тела
		ASSERT_EQ((block + 4), wire.size()) << wire;
	}
	/**
	 * Проверяем ответ: тело кадрируется закрытием соединения
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Формируем контейнер заголовков ответа версии HTTP/1.0 без объявления размера тела
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_0, static_cast <uint16_t> (200)));
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Проверяем что тело ответа принято к отправке целиком
		ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
		// Определяем позицию конца блока заголовков
		const size_t block = wire.find("\r\n\r\n");
		// Проверяем что блок заголовков на проводе завершён
		ASSERT_NE(block, std::string::npos) << wire;
		// Проверяем что тело ушло сырыми байтами до закрытия соединения
		ASSERT_EQ(wire.substr(block + 4), "hello") << wire;
	}
}

/**
 * @brief Метод проверки границы превышения верхнего порога выходного буфера
 *
 * @details Верхний порог управляет обратным давлением, а не является жёсткой ёмкостью:
 *          до порога отмеряются байты тела, а разметка кадрирования chunked дописывается
 *          поверх отмеренного. Превышение обязано оставаться размером этой разметки -
 *          заголовок чанка и два CRLF - и не зависеть от размера тела, а также не
 *          накапливаться от вызова к вызову
 *
 */
TEST_F(ParserFixture, SendWaterMarkChunkedOvershootTest){
	// Устанавливаем верхний порог выходного буфера
	const size_t high = 4096;
	// Устанавливаем предел превышения порога: заголовок чанка и два CRLF
	const size_t tolerance = 16;
	/**
	 * Проверяем push-модель: тело подаётся методом отправки
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Устанавливаем пороги выходного буфера
		sender->sendWaterMarks(high, (high / 4));
		// Формируем контейнер заголовков ответа с кадрированием chunked
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Дописываем заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "chunked");
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Формируем тело заведомо большего размера, чем верхний порог
		const std::string body((16 * high), 'x');
		// Позиция выдачи тела сообщения
		size_t offset = 0;
		/**
		 * Подаём тело до отказа выходного буфера
		 */
		while(offset < body.size()){
			// Передаём очередную часть тела сообщения
			const size_t taken = sender->sendData((body.data() + offset), (body.size() - offset), false);
			// Если буфер заполнен - прекращаем подачу
			if(taken == 0)
				// Прекращаем подачу тела
				break;
			// Смещаем позицию выдачи тела
			offset += taken;
			// Проверяем что заполнение буфера не ушло за порог дальше размера разметки
			ASSERT_LE(sender->pending().size(), (high + tolerance)) << "принято байт тела: " << offset;
		}
		// Проверяем что буфер действительно наполнился до порога
		ASSERT_GT(sender->pending().size(), (high / 2));
	}
	/**
	 * Проверяем pull-модель: тело подаётся источником данных
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Устанавливаем пороги выходного буфера
		sender->sendWaterMarks(high, (high / 4));
		// Формируем контейнер заголовков ответа с кадрированием chunked
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Дописываем заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "chunked");
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Формируем тело заведомо большего размера, чем верхний порог
		const std::string body((16 * high), 'y');
		// Позиция чтения тела сообщения источником
		size_t position = 0;
		// Устанавливаем pull-источник данных тела сообщения
		sender->dataSource(parser_http_t::data_source_callback_t([&body, &position](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
			// Определяем размер отдаваемой порции тела
			const size_t size = std::min(cap, (body.size() - position));
			// Копируем порцию тела в буфер источника
			std::memcpy(buffer, (body.data() + position), size);
			// Смещаем позицию чтения тела
			position += size;
			// Определяем достижение конца тела
			eof = (position >= body.size());
			// Выводим размер отданной порции тела
			return static_cast <int64_t> (size);
		}));
		// Проверяем что заполнение буфера не ушло за порог дальше размера разметки
		ASSERT_LE(sender->pending().size(), (high + tolerance));
		// Проверяем что буфер действительно наполнился до порога
		ASSERT_GT(sender->pending().size(), (high / 2));
		/**
		 * Гоняем цикл выборки и дозагрузки: в pull-модели буфер досыпается по мере
		 * выборки, и превышение обязано оставаться в тех же пределах на каждом круге.
		 * Однократного наполнения для этого мало - именно повторные круги показывают,
		 * накапливается превышение от порции к порции или нет
		 */
		size_t rounds = 0;
		/**
		 * Выбираем накопленные байты, пока источник их досыпает
		 */
		while(!sender->pending().empty()){
			// Выбираем часть накопленных байт, освобождая место под дозагрузку
			sender->consumePending(std::min(sender->pending().size(), (high / 4)));
			// Проверяем что заполнение буфера не ушло за порог дальше размера разметки
			ASSERT_LE(sender->pending().size(), (high + tolerance)) << "круг выборки: " << rounds;
			// Учитываем выполненный круг выборки
			++rounds;
			// Если тело источника исчерпано и буфер опустошён - прекращаем выборку
			if((position >= body.size()) && (sender->pending().size() <= (high / 4)))
				// Прекращаем выборку накопленных байт
				break;
		}
		// Проверяем что кругов выборки было достаточно для проверки накопления
		ASSERT_GT(rounds, 4u);
		// Проверяем что тело источника действительно было выдано целиком
		ASSERT_EQ(position, body.size());
	}
}

/**
 * @brief Метод проверки завершения исходящего сообщения объявленным нулевым размером тела
 *
 * @details Блок заголовков с Content-Length: 0 на проводе уже является законченным
 *          сообщением без тела, и состояние отправителя обязано этому соответствовать
 *          независимо от флага завершения. Иначе отправитель ждёт тела, которого по
 *          объявленному размеру быть не может, а следующее сообщение отбрасывается
 *          как поданное поверх незавершённого - соединение залипает
 *
 */
TEST_F(ParserFixture, SendZeroContentLengthFinishesTest){
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
	// Формируем контейнер заголовков ответа с объявленным нулевым размером тела
	headers_t first(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Объявляем нулевой размер тела ответа
	first.emplace("Content-Length", "0");
	// Отправляем заголовки ответа без флага завершения сообщения
	sender->sendHeaders(first, false);
	/**
	 * Отправляем следующее сообщение по тому же соединению: предыдущее завершено
	 * объявленным нулевым размером тела, и отправитель обязан его принять
	 */
	headers_t second(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Объявляем размер тела следующего ответа
	second.emplace("Content-Length", "5");
	// Отправляем заголовки следующего ответа
	sender->sendHeaders(second, false);
	// Проверяем что тело следующего ответа принято к отправке целиком
	ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
	// Перекачиваем исходящие байты отправителя в принимающий парсер
	::drain(* sender, * receiver);
	// Проверяем что первое сообщение разобрано полностью
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело первого сообщения пустое
	ASSERT_TRUE(events.body.empty());
}

/**
 * @brief Метод проверки отказа собрать сообщение с неисправимым объявлением кодирования
 *
 * @details Если объявление транспортного кодирования уже содержит chunked, но не
 *          последним, дописать его нельзя: к телу оно применялось бы дважды, что
 *          запрещено RFC 9112 §6.1, а получатель отвергнет такой кадр. Изменить порядок
 *          кодирований библиотека тоже не вправе - ей неизвестно, какие из них вызывающая
 *          сторона к телу действительно применила. Сообщение не собирается вовсе
 *
 */
TEST_F(ParserFixture, SendUnfixableTransferEncodingTest){
	/**
	 * Проверяем отказ при неисправимом объявлении кодирования
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Формируем контейнер заголовков ответа с кодированием, где chunked не последний
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Дописываем заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "chunked, gzip");
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Проверяем что на провод не ушло ничего
		ASSERT_TRUE(wire.empty()) << wire;
		// Проверяем что тело к отправке не принято - заголовки не отправлялись
		ASSERT_EQ(sender->sendData("hello", 5, true), 0u);
	}
	/**
	 * Проверяем отказ и при завершении сообщения заголовками
	 *
	 * Кадрирование тела в этом случае не выбирается вовсе, но заголовок уходит
	 * на провод так же - и получатель отвергнет его так же
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Формируем контейнер заголовков ответа с кодированием, где chunked не последний
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Дописываем заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "chunked, gzip");
		// Отправляем заголовки ответа с завершением сообщения
		sender->sendHeaders(response, true);
		// Проверяем что на провод не ушло ничего
		ASSERT_TRUE(wire.empty()) << wire;
	}
	/**
	 * Проверяем что вычищенный из блока заголовок отказа не вызывает
	 *
	 * При конфликте с Content-Length объявление кодирования снимается с провода,
	 * и его неисправимость получателю уже не видна - отказывать не в чем
	 */
	{
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Собранные байты исходящего сообщения
		std::string wire;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
		}));
		// Формируем контейнер заголовков ответа с конфликтующим кадрированием
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Объявляем размер тела ответа
		response.emplace("Content-Length", "5");
		// Дописываем конфликтующий заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "chunked, gzip");
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Проверяем что тело принято к отправке целиком
		ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
		// Проверяем что объявление кодирования на провод не ушло
		ASSERT_EQ(wire.find("Transfer-Encoding"), std::string::npos) << wire;
		// Проверяем что сообщение собрано с кадрированием по объявленному размеру тела
		ASSERT_NE(wire.find("Content-Length: 5"), std::string::npos) << wire;
	}
	/**
	 * Проверяем что исправимое объявление по-прежнему дополняется
	 *
	 * Кодирование без chunked дополняется отдельным заголовком: значения нескольких
	 * заголовков склеиваются по порядку следования, и chunked оказывается последним
	 */
	{
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
		// Формируем контейнер заголовков ответа с кодированием без chunked
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Дописываем заголовок кодирования тела сообщения
		response.emplace("Transfer-Encoding", "gzip");
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Отправляем тело ответа с завершением сообщения
		ASSERT_EQ(sender->sendData("hello", 5, true), 5u);
		// Перекачиваем исходящие байты отправителя в принимающий парсер
		::drain(* sender, * receiver);
		// Проверяем что собранное сообщение разбирается собственным приёмником
		ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
		// Проверяем что тело передано без искажений
		ASSERT_EQ(events.body, "hello");
	}
}
