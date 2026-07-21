/**
 * @file: parameterized.cpp
 * @date: 2026-07-12
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "headers.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Структура параметров теста разбора стартовой строки
 *
 */
struct StartlineTestParameter {
	// Исходная стартовая строка HTTP-запроса/ответа
	std::string input;
	// Ожидаемое направление трафика (запрос/ответ)
	direct_t direct;
	// Ожидаемая каноническая стартовая строка после разбора
	std::string expected;
};

/**
 * @brief Класс фикстуры теста разбора стартовой строки
 *
 */
class StartlineParameterizedFixture : public HeadersFixture, public ::testing::WithParamInterface <StartlineTestParameter> {
	public:
		// Параметры теста разбора стартовой строки
		StartlineTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования разбора и обратной сборки стартовой строки
 *
 */
TEST_P(StartlineParameterizedFixture, StartlineRoundTripTest){
	// Устанавливаем исходную стартовую строку в контейнер заголовков
	this->_headers->startline(this->_parameter.input);
	// Получаем сформированный объект провайдера
	const provider_t * provider = this->_headers->provider();
	// Проверяем что провайдер сформирован по стартовой строке
	ASSERT_TRUE(provider != nullptr);
	// Проверяем что направление трафика определено корректно
	ASSERT_EQ(provider->direct, this->_parameter.direct);
	// Проверяем что обратная сборка стартовой строки соответствует канонической форме
	ASSERT_EQ(this->_headers->startline(), this->_parameter.expected);
}

/**
 * @brief Инициализация параметров теста разбора стартовой строки
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, StartlineParameterizedFixture,
	::testing::Values(
		StartlineTestParameter({"GET /index.html HTTP/1.1", direct_t::REQUEST, "GET /index.html HTTP/1.1"}),
		StartlineTestParameter({"POST /submit HTTP/1.1", direct_t::REQUEST, "POST /submit HTTP/1.1"}),
		StartlineTestParameter({"PUT /upload HTTP/1.0", direct_t::REQUEST, "PUT /upload HTTP/1.0"}),
		StartlineTestParameter({"DELETE /item/42 HTTP/1.1", direct_t::REQUEST, "DELETE /item/42 HTTP/1.1"}),
		StartlineTestParameter({"OPTIONS * HTTP/1.1", direct_t::REQUEST, "OPTIONS * HTTP/1.1"}),
		StartlineTestParameter({"HEAD / HTTP/1.1", direct_t::REQUEST, "HEAD / HTTP/1.1"}),
		StartlineTestParameter({"HTTP/1.1 200 OK", direct_t::RESPONSE, "HTTP/1.1 200 OK"}),
		StartlineTestParameter({"HTTP/1.1 404 Not Found", direct_t::RESPONSE, "HTTP/1.1 404 Not Found"}),
		StartlineTestParameter({"HTTP/1.1 500 Internal Server Error", direct_t::RESPONSE, "HTTP/1.1 500 Internal Server Error"}),
		StartlineTestParameter({"HTTP/1.0 301 Moved Permanently", direct_t::RESPONSE, "HTTP/1.0 301 Moved Permanently"})
	)
);

/**
 * @brief Структура параметров теста набора заголовков
 *
 */
struct HeadersTestParameter {
	// Версия протокола HTTP-запроса/ответа
	proto_t proto;
	// Список пар «название - значение» заголовков
	std::vector <std::pair <std::string, std::string>> items;
};

/**
 * @brief Класс фикстуры теста набора заголовков
 *
 */
class HeadersParameterizedFixture : public HeadersFixture, public ::testing::WithParamInterface <HeadersTestParameter> {
	public:
		// Параметры теста набора заголовков
		HeadersTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования наполнения, учёта и печати набора заголовков
 *
 */
TEST_P(HeadersParameterizedFixture, HeadersFillPrintTest){
	// Ожидаемый суммарный объём полезной нагрузки заголовков
	size_t expectedMemory = 0;
	/**
	 * Наполняем контейнер заголовками из параметров теста
	 */
	for(auto & item : this->_parameter.items){
		// Добавляем заголовок в контейнер
		this->_headers->emplace(item.first, item.second);
		// Дополняем ожидаемый объём полезной нагрузки объёмом добавленного заголовка
		expectedMemory += (item.first.size() + item.second.size());
	}
	// Проверяем что все заголовки добавлены
	ASSERT_EQ(this->_headers->size(), this->_parameter.items.size());
	// Проверяем что учёт потребляемой памяти совпадает с ожидаемым объёмом
	ASSERT_EQ(this->_headers->memory(), expectedMemory);
	// Устанавливаем протокол контейнера
	this->_headers->proto(this->_parameter.proto);
	// Формируем текстовое представление заголовков
	std::string output = this->_headers->print(this->_parameter.proto);
	/**
	 * Проверяем что каждый добавленный заголовок присутствует в выводе
	 */
	for(auto & item : this->_parameter.items){
		// Проверяем что значение заголовка присутствует в выводе
		ASSERT_NE(output.find(item.second), std::string::npos);
		// Проверяем что заголовок доступен по названию
		ASSERT_TRUE(this->_headers->has(item.first));
	}
	// Проверяем что вывод завершается пустой строкой, отделяющей тело сообщения
	ASSERT_EQ(output.compare(output.size() - 4, 4, "\r\n\r\n"), 0);
}

/**
 * @brief Метод тестирования копирования, сравнения и очистки набора заголовков
 *
 */
TEST_P(HeadersParameterizedFixture, HeadersCopyCompareTest){
	/**
	 * Наполняем контейнер заголовками из параметров теста
	 */
	for(auto & item : this->_parameter.items)
		// Добавляем заголовок в контейнер
		this->_headers->emplace(item.first, item.second);
	// Копируем контейнер через конструктор копирования
	headers_t copy(* this->_headers);
	// Проверяем что копия равна исходному контейнеру
	ASSERT_TRUE(copy == (* this->_headers));
	// Проверяем что размеры контейнеров совпадают
	ASSERT_EQ(copy.size(), this->_headers->size());
	// Проверяем что учёт памяти контейнеров совпадает
	ASSERT_EQ(copy.memory(), this->_headers->memory());
	// Выполняем очистку исходного контейнера
	this->_headers->clear();
	// Проверяем что исходный контейнер опустел
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что копия сохранила заголовки
	ASSERT_EQ(copy.size(), this->_parameter.items.size());
	// Проверяем что контейнеры перестали быть равными (кроме случая пустого набора)
	if(!this->_parameter.items.empty())
		// Контейнеры разного размера не равны
		ASSERT_TRUE(copy != (* this->_headers));
}

/**
 * @brief Инициализация параметров теста набора заголовков
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, HeadersParameterizedFixture,
	::testing::Values(
		HeadersTestParameter({proto_t::HTTP1, {{"Host", "example.com"}, {"Accept", "text/html"}, {"User-Agent", "awh"}}}),
		HeadersTestParameter({proto_t::HTTP1, {{"Content-Type", "application/json"}, {"Content-Length", "128"}}}),
		HeadersTestParameter({proto_t::HTTP2, {{"Host", "anyks.com"}, {"Accept-Encoding", "gzip, deflate"}}}),
		HeadersTestParameter({proto_t::HTTP2, {{"Cache-Control", "no-cache"}, {"Connection", "keep-alive"}, {"Pragma", "no-cache"}}}),
		HeadersTestParameter({proto_t::HTTP1, {{"X-Custom-Header", "custom-value"}}})
	)
);
