/**
 * @file: parameterized.cpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты парсера протокола HTTP/2 —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой разбора фреймов,
 *        управления состояниями потоков, окнами flow control и кодирования HPACK
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

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http2.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Класс фикстуры теста инкрементального разбора при разных размерах фрагментов
 *
 */
class Http2FragmentParameterizedFixture : public ParserHttp2Fixture, public ::testing::WithParamInterface <size_t> {
	public:
		// Размер фрагмента подачи данных
		size_t _fragment = GetParam();
};

/**
 * @brief Метод тестирования идентичности результата разбора при любом размере фрагментов
 *
 */
TEST_P(Http2FragmentParameterizedFixture, FragmentedParsingTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента (без канала записи - pull-модель)
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера сервера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера сервера
	this->attach(* server, events);
	// Формируем тело запроса (два DATA-фрейма при лимите фрейма 16384)
	std::string body(30000, '\0');
	/**
	 * Выполняем заполнение тела псевдослучайным паттерном
	 */
	for(size_t i = 0; i < body.size(); ++i)
		// Заполняем очередной байт тела
		body[i] = static_cast <char> ((i * 13 + 5) & 0xFF);
	// Клиент отправляет magic-строку и свой SETTINGS
	client->sendPreface();
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок адресата запроса
	fields.emplace_back(":authority", "example.com");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/fragmented");
	// Дописываем обычный заголовок
	fields.emplace_back("user-agent", "awh");
	// Отправляем заголовки запроса (тело последует отдельно)
	client->sendHeaders(sid, fields, false);
	// Отправляем тело запроса с завершением потока
	client->sendData(sid, body.data(), body.size(), true);
	// Забираем сырые исходящие байты клиента (полный трафик соединения)
	const std::string raw(client->pending());
	// Освобождаем исходящий буфер клиента
	client->consumePending(raw.size());
	/**
	 * Выполняем подачу сырых байтов серверу фрагментами заданного размера,
	 * границы фреймов при этом рвутся в произвольных местах
	 */
	for(size_t i = 0; i < raw.size(); i += this->_fragment)
		// Выполняем разбор очередного фрагмента данных
		server->parse(raw.data() + i, ((raw.size() - i) < this->_fragment ? (raw.size() - i) : this->_fragment));
	// Проверяем что соединение сервера живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
	// Проверяем что ошибок уровня соединения нет
	ASSERT_EQ(server->error(), parser_http2_t::error_t::NO_ERROR);
	// Проверяем что все заголовки запроса получены
	ASSERT_EQ(events.headers.size(), 5u);
	// Проверяем метод запроса из провайдера
	ASSERT_EQ(events.method, method_t::POST);
	// Проверяем параметры URI-запроса из провайдера
	ASSERT_EQ(events.uri, "/fragmented");
	// Проверяем что тело запроса собрано без искажений вне зависимости от фрагментации
	ASSERT_EQ(events.bodies[sid], body);
	// Формируем заголовки ответа сервера (для полного закрытия потока)
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "200");
	// Отправляем заголовки ответа с завершением потока (исходящие байты копятся в буфере)
	server->sendHeaders(sid, response, true);
	// Проверяем что поток закрыт штатно (обе половины завершены)
	ASSERT_EQ(events.closes.size(), 1u);
	// Проверяем код закрытия потока
	ASSERT_EQ(events.closes.front().second, parser_http2_t::error_t::NO_ERROR);
}

/**
 * @brief Инициализация параметров теста инкрементального разбора
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, Http2FragmentParameterizedFixture,
	::testing::Values(1, 2, 3, 5, 7, 9, 13, 64, 1024, 4096)
);

/**
 * @brief Структура параметров теста классификации методов запроса
 *
 */
struct Http2MethodTestParameter {
	// Название метода запроса
	std::string name;
	// Ожидаемый метод запроса
	method_t method;
};

/**
 * @brief Класс фикстуры теста классификации методов запроса
 *
 */
class Http2MethodParameterizedFixture : public ParserHttp2Fixture, public ::testing::WithParamInterface <Http2MethodTestParameter> {
	public:
		// Параметры теста классификации методов запроса
		Http2MethodTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования классификации методов запроса из псевдо-заголовка [:method]
 *
 */
TEST_P(Http2MethodParameterizedFixture, MethodClassificationTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", this->_parameter.name);
	// Если запрос выполняется методом CONNECT - [:scheme]/[:path] запрещены (RFC 9113 §8.5)
	if(this->_parameter.method == method_t::CONNECT)
		// Дописываем псевдо-заголовок авторитета запроса
		fields.emplace_back(":authority", "anyks.com:443");
	// Для остальных методов обязательны [:scheme] и [:path]
	else {
		// Дописываем псевдо-заголовок схемы запроса
		fields.emplace_back(":scheme", "https");
		// Дописываем псевдо-заголовок адресата запроса
		fields.emplace_back(":authority", "anyks.com");
		// Дописываем псевдо-заголовок пути запроса
		fields.emplace_back(":path", "/");
	}
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Проверяем что провайдер заголовков собран
	ASSERT_EQ(serverEvents.providers.size(), 1u);
	// Проверяем что метод запроса классифицирован корректно
	ASSERT_EQ(serverEvents.method, this->_parameter.method);
	// Проверяем что соединение сервера живо
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Инициализация параметров теста классификации методов запроса
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, Http2MethodParameterizedFixture,
	::testing::Values(
		Http2MethodTestParameter{"GET", method_t::GET},
		Http2MethodTestParameter{"PUT", method_t::PUT},
		Http2MethodTestParameter{"DELETE", method_t::DEL},
		Http2MethodTestParameter{"POST", method_t::POST},
		Http2MethodTestParameter{"HEAD", method_t::HEAD},
		Http2MethodTestParameter{"PATCH", method_t::PATCH},
		Http2MethodTestParameter{"TRACE", method_t::TRACE},
		Http2MethodTestParameter{"OPTIONS", method_t::OPTIONS},
		Http2MethodTestParameter{"CONNECT", method_t::CONNECT},
		Http2MethodTestParameter{"PROPFIND", method_t::PROPFIND},
		Http2MethodTestParameter{"MKCALENDAR", method_t::MKCALENDAR},
		Http2MethodTestParameter{"M-SEARCH", method_t::MSEARCH},
		Http2MethodTestParameter{"FOOBAR", method_t::UNKNOWN}
	)
);

/**
 * @brief Класс фикстуры теста передачи тел разных размеров
 *
 */
class Http2BodySizeParameterizedFixture : public ParserHttp2Fixture, public ::testing::WithParamInterface <size_t> {
	public:
		// Размер передаваемого тела
		size_t _size = GetParam();
};

/**
 * @brief Метод тестирования передачи тела произвольного размера (нарезка на фреймы + flow control)
 *
 */
TEST_P(Http2BodySizeParameterizedFixture, BodyRoundtripTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Формируем тело запроса заданного размера
	std::string body(this->_size, '\0');
	/**
	 * Выполняем заполнение тела псевдослучайным паттерном
	 */
	for(size_t i = 0; i < body.size(); ++i)
		// Заполняем очередной байт тела
		body[i] = static_cast <char> ((i * 7 + 11) & 0xFF);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "POST");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок адресата запроса
	fields.emplace_back(":authority", "example.com");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/body");
	// Если тело пустое - завершаем поток сразу заголовками
	if(body.empty())
		// Отправляем заголовки запроса с завершением потока
		client->sendHeaders(sid, fields, true);
	// Если тело передаётся
	else {
		// Отправляем заголовки запроса (тело последует отдельно)
		client->sendHeaders(sid, fields, false);
		// Смещение отправки тела
		size_t offset = 0;
		// Ограничитель количества попыток отправки (защита теста от зависания)
		size_t attempts = 0;
		/**
		 * Отправляем тело порциями с учётом заполнения буфера отправки
		 */
		while((offset < body.size()) && (attempts++ < 100))
			// Отправляем очередную порцию тела с завершением потока на последнем фрагменте
			offset += client->sendData(sid, body.data() + offset, body.size() - offset, true);
		// Проверяем что всё тело принято парсером
		ASSERT_EQ(offset, body.size());
	}
	// Проверяем что сервер получил тело без искажений
	ASSERT_EQ(serverEvents.bodies[sid], body);
	// Формируем заголовки ответа сервера (для полного закрытия потока)
	std::vector <h2::hpack::field_t> response;
	// Дописываем псевдо-заголовок статуса ответа
	response.emplace_back(":status", "204");
	// Отправляем заголовки ответа с завершением потока
	server->sendHeaders(sid, response, true);
	// Проверяем что поток сервера закрыт штатно (обе половины завершены)
	ASSERT_EQ(serverEvents.closes.size(), 1u);
	// Проверяем код закрытия потока сервера
	ASSERT_EQ(serverEvents.closes.front().second, parser_http2_t::error_t::NO_ERROR);
	// Проверяем что соединения живы
	ASSERT_EQ(server->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Инициализация параметров теста передачи тел разных размеров
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, Http2BodySizeParameterizedFixture,
	::testing::Values(0, 1, 100, 16383, 16384, 16385, 65535, 65536, 200000)
);

/**
 * @brief Класс фикстуры теста передачи статус-кодов ответа сервера
 *
 */
class Http2StatusCodeParameterizedFixture : public ParserHttp2Fixture, public ::testing::WithParamInterface <uint16_t> {
	public:
		// Статус-код ответа сервера
		uint16_t _code = GetParam();
};

/**
 * @brief Метод тестирования передачи статус-кода ответа через псевдо-заголовок [:status]
 *
 */
TEST_P(Http2StatusCodeParameterizedFixture, StatusCodeTest){
	// Создаём объект парсера сервера
	auto server = this->make(direct_t::REQUEST);
	// Создаём объект парсера клиента
	auto client = this->make(direct_t::RESPONSE);
	// Создаём объекты сборщиков событий парсеров
	events_t serverEvents, clientEvents;
	// Подписываем сборщики событий на все функции обратного вызова парсеров
	this->attach(* server, serverEvents);
	// Подписываем сборщик событий клиента
	this->attach(* client, clientEvents);
	// Соединяем парсеры каналами записи
	this->connect(* client, * server);
	// Выполняем рукопожатие соединения
	this->handshake(* client, * server);
	// Выделяем идентификатор нового потока клиента
	const uint32_t sid = client->nextStreamId();
	// Формируем заголовки запроса
	std::vector <h2::hpack::field_t> fields;
	// Дописываем псевдо-заголовок метода запроса
	fields.emplace_back(":method", "GET");
	// Дописываем псевдо-заголовок схемы запроса
	fields.emplace_back(":scheme", "https");
	// Дописываем псевдо-заголовок адресата запроса
	fields.emplace_back(":authority", "example.com");
	// Дописываем псевдо-заголовок пути запроса
	fields.emplace_back(":path", "/status");
	// Отправляем заголовки запроса с завершением потока
	client->sendHeaders(sid, fields, true);
	// Формируем контейнер заголовков ответа с провайдером проверяемого статус-кода
	headers_t response(std::make_unique <response_t> (version_t::HTTP2, this->_code));
	// Отправляем заголовки ответа из контейнера с завершением потока
	server->sendHeaders(sid, response, true);
	// Проверяем что провайдер ответа собран
	ASSERT_FALSE(clientEvents.providers.empty());
	// Проверяем что статус-код ответа передан без искажений
	ASSERT_EQ(clientEvents.code, this->_code);
	// Проверяем что соединения живы
	ASSERT_EQ(client->status(), parser_t::status_t::PARTIAL);
}

/**
 * @brief Инициализация параметров теста передачи статус-кодов ответа сервера
 *
 * @note Информационные коды 1xx финальным ответом не являются и завершать поток не могут
 *       (RFC 9113 §8.1) - они проверяются отдельно тестами InformationalResponseTest
 *       и InformationalEndStreamTest
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, Http2StatusCodeParameterizedFixture,
	::testing::Values(200, 201, 204, 206, 301, 304, 400, 401, 403, 404, 418, 500, 502, 503, 599)
);
