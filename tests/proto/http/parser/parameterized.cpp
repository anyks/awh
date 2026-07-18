/**
 * @file: parameterized.cpp
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "parser.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Класс фикстуры теста инкрементального разбора при разных размерах фрагментов
 *
 */
class FragmentParameterizedFixture : public ParserFixture, public ::testing::WithParamInterface <size_t> {
	public:
		// Размер фрагмента подачи данных
		size_t _fragment = GetParam();
};

/**
 * @brief Метод тестирования идентичности результата разбора при любом размере фрагментов
 *
 */
TEST_P(FragmentParameterizedFixture, FragmentedParsingTest){
	// Создаём объект парсера ответов сервера
	auto parser = this->make(direct_t::RESPONSE);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	// Формируем данные HTTP-ответа с телом в кодировке chunked и трейлером
	const std::string message =
		"HTTP/1.1 200 OK\r\n"
		"Server: AWH\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"6\r\nAWH is\r\n"
		"9\r\n awesome!\r\n"
		"0\r\n"
		"X-Check: done\r\n"
		"\r\n";
	// Общее количество обработанных байт
	size_t total = 0;
	/**
	 * Выполняем подачу данных фрагментами заданного размера
	 */
	for(size_t i = 0; i < message.size(); i += this->_fragment)
		// Выполняем разбор очередного фрагмента данных и считаем обработанные байты
		total += parser->parse(message.data() + i, ((message.size() - i) < this->_fragment ? (message.size() - i) : this->_fragment));
	// Проверяем что все данные обработаны
	ASSERT_EQ(total, message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что ошибок разбора нет
	ASSERT_EQ(parser->error(), parser_t::error_t::NONE);
	// Проверяем что тело сообщения собрано корректно вне зависимости от фрагментации
	ASSERT_EQ(events.body, "AWH is awesome!");
	// Проверяем что разобраны оба заголовка
	ASSERT_EQ(events.headers.size(), 2u);
	// Проверяем что разобран один трейлер
	ASSERT_EQ(events.trailers.size(), 1u);
	// Проверяем значение трейлера
	ASSERT_EQ(events.trailers[0].second, "done");
	// Проверяем что код ответа разобран корректно
	ASSERT_EQ(static_cast <const response_t *> (parser->message().provider.get())->code, 200u);
}

/**
 * @brief Инициализация параметров теста инкрементального разбора
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, FragmentParameterizedFixture,
	::testing::Values(1, 2, 3, 5, 7, 13, 16, 64, 1024)
);

/**
 * @brief Структура параметров теста классификации методов запроса
 *
 */
struct MethodTestParameter {
	// Название метода запроса
	std::string name;
	// Ожидаемый метод запроса
	method_t method;
};

/**
 * @brief Класс фикстуры теста классификации методов запроса
 *
 */
class MethodParameterizedFixture : public ParserFixture, public ::testing::WithParamInterface <MethodTestParameter> {
	public:
		// Параметры теста классификации методов запроса
		MethodTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования классификации всех поддерживаемых методов запроса
 *
 */
TEST_P(MethodParameterizedFixture, MethodClassificationTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Формируем данные HTTP-запроса с проверяемым методом
	const std::string message = (this->_parameter.name + " / HTTP/1.1\r\n\r\n");
	// Выполняем разбор данных HTTP-запроса
	const size_t bytes = parser->parse(message.data(), message.size());
	// Проверяем что все данные обработаны
	ASSERT_EQ(bytes, message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Получаем объект провайдера заголовков запроса клиента
	const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
	// Проверяем что метод запроса классифицирован корректно
	ASSERT_EQ(request->method, this->_parameter.method);
	// Если метод запроса не распознан
	if(this->_parameter.method == method_t::UNKNOWN)
		// Проверяем что оригинальное написание метода сохранено
		ASSERT_EQ(request->methodName, this->_parameter.name);
	// Если метод запроса распознан - оригинальное написание не заполняется
	else ASSERT_TRUE(request->methodName.empty());
}

/**
 * @brief Инициализация параметров теста классификации методов запроса
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, MethodParameterizedFixture,
	::testing::Values(
		MethodTestParameter({"GET", method_t::GET}),
		MethodTestParameter({"PUT", method_t::PUT}),
		MethodTestParameter({"DELETE", method_t::DEL}),
		MethodTestParameter({"POST", method_t::POST}),
		MethodTestParameter({"HEAD", method_t::HEAD}),
		MethodTestParameter({"PATCH", method_t::PATCH}),
		MethodTestParameter({"TRACE", method_t::TRACE}),
		MethodTestParameter({"OPTIONS", method_t::OPTIONS}),
		MethodTestParameter({"ACL", method_t::ACL}),
		MethodTestParameter({"COPY", method_t::COPY}),
		MethodTestParameter({"LOCK", method_t::LOCK}),
		MethodTestParameter({"MOVE", method_t::MOVE}),
		MethodTestParameter({"BIND", method_t::BIND}),
		MethodTestParameter({"MKCOL", method_t::MKCOL}),
		MethodTestParameter({"MERGE", method_t::MERGE}),
		MethodTestParameter({"REPORT", method_t::REPORT}),
		MethodTestParameter({"SEARCH", method_t::SEARCH}),
		MethodTestParameter({"UNLOCK", method_t::UNLOCK}),
		MethodTestParameter({"REBIND", method_t::REBIND}),
		MethodTestParameter({"UNBIND", method_t::UNBIND}),
		MethodTestParameter({"CHECKOUT", method_t::CHECKOUT}),
		MethodTestParameter({"PROPFIND", method_t::PROPFIND}),
		MethodTestParameter({"PROPPATCH", method_t::PROPPATCH}),
		MethodTestParameter({"MKACTIVITY", method_t::MKACTIVITY}),
		MethodTestParameter({"PRI", method_t::PRI}),
		MethodTestParameter({"LINK", method_t::LINK}),
		MethodTestParameter({"PURGE", method_t::PURGE}),
		MethodTestParameter({"NOTIFY", method_t::NOTIFY}),
		MethodTestParameter({"UNLINK", method_t::UNLINK}),
		MethodTestParameter({"SOURCE", method_t::SOURCE}),
		MethodTestParameter({"M-SEARCH", method_t::MSEARCH}),
		MethodTestParameter({"SUBSCRIBE", method_t::SUBSCRIBE}),
		MethodTestParameter({"MKCALENDAR", method_t::MKCALENDAR}),
		MethodTestParameter({"UNSUBSCRIBE", method_t::UNSUBSCRIBE}),
		MethodTestParameter({"FOOBAR", method_t::UNKNOWN}),
		MethodTestParameter({"GETX", method_t::UNKNOWN})
	)
);

/**
 * @brief Структура параметров теста обработки некорректных сообщений
 *
 */
struct ErrorTestParameter {
	// Данные некорректного HTTP-сообщения
	std::string payload;
	// Направление трафика (запрос/ответ)
	direct_t direct;
	// Ожидаемый код ошибки разбора
	awh::http::parser_t::error_t error;
};

/**
 * @brief Класс фикстуры теста обработки некорректных сообщений
 *
 */
class ErrorParameterizedFixture : public ParserFixture, public ::testing::WithParamInterface <ErrorTestParameter> {
	public:
		// Параметры теста обработки некорректных сообщений
		ErrorTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования детектирования ошибок разбора некорректных сообщений
 *
 */
TEST_P(ErrorParameterizedFixture, ErrorDetectionTest){
	// Создаём объект парсера с заданным направлением трафика
	auto parser = this->make(this->_parameter.direct);
	// Выполняем разбор данных некорректного HTTP-сообщения
	parser->parse(this->_parameter.payload.data(), this->_parameter.payload.size());
	// Проверяем что зафиксирована ошибка разбора
	ASSERT_EQ(parser->status(), parser_t::status_t::ERROR);
	// Проверяем что код ошибки соответствует ожидаемому
	ASSERT_EQ(parser->error(), this->_parameter.error);
}

/**
 * @brief Инициализация параметров теста обработки некорректных сообщений
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ErrorParameterizedFixture,
	::testing::Values(
		// Недопустимый символ в методе запроса
		ErrorTestParameter({"GE[T / HTTP/1.1\r\n\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_METHOD}),
		// Одиночный CR без последующего LF в конце строки
		ErrorTestParameter({"GET / HTTP/1.1\rX", direct_t::REQUEST, parser_t::error_t::INVALID_EOL}),
		// Неподдерживаемая версия протокола в запросе
		ErrorTestParameter({"GET / HTTP/3.0\r\n\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_VERSION}),
		// Некорректный литерал префикса версии протокола
		ErrorTestParameter({"GET / HTTX/1.1\r\n\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_VERSION}),
		// Недопустимый управляющий символ в request-target
		ErrorTestParameter({std::string("GET /pa\x01th HTTP/1.1\r\n\r\n"), direct_t::REQUEST, parser_t::error_t::INVALID_TARGET}),
		// Пробел перед двоеточием в имени заголовка
		ErrorTestParameter({"GET / HTTP/1.1\r\nHost : x\r\n\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_HEADER_TOKEN}),
		// Устаревший перенос строки заголовка (obs-fold)
		ErrorTestParameter({"GET / HTTP/1.1\r\nHost: x\r\n y\r\n\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_HEADER_TOKEN}),
		// Нечисловое значение заголовка Content-Length
		ErrorTestParameter({"POST / HTTP/1.1\r\nContent-Length: abc\r\n\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_CONTENT_LENGTH}),
		// Одновременные заголовки Content-Length и Transfer-Encoding (request smuggling)
		ErrorTestParameter({"POST / HTTP/1.1\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n", direct_t::REQUEST, parser_t::error_t::CONTENT_LENGTH_CONFLICT}),
		// Различающиеся значения двух заголовков Content-Length (request smuggling)
		ErrorTestParameter({"POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n", direct_t::REQUEST, parser_t::error_t::CONTENT_LENGTH_CONFLICT}),
		// Кодирование chunked не последнее в списке Transfer-Encoding
		ErrorTestParameter({"POST / HTTP/1.1\r\nTransfer-Encoding: chunked, gzip\r\n\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_TRANSFER_ENCODING}),
		// Недопустимый символ в размере чанка
		ErrorTestParameter({"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nXYZ\r\n", direct_t::REQUEST, parser_t::error_t::INVALID_CHUNK_SIZE}),
		// Отсутствие CRLF после данных чанка
		ErrorTestParameter({"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabcXX", direct_t::REQUEST, parser_t::error_t::INVALID_CHUNK_TERMINATOR}),
		// Статус-код ответа из четырёх цифр
		ErrorTestParameter({"HTTP/1.1 2000 OK\r\n\r\n", direct_t::RESPONSE, parser_t::error_t::INVALID_STATUS}),
		// Статус-код ответа содержащий буквы
		ErrorTestParameter({"HTTP/1.1 2A0 OK\r\n\r\n", direct_t::RESPONSE, parser_t::error_t::INVALID_STATUS}),
		// Неподдерживаемая версия протокола в ответе
		ErrorTestParameter({"HTTP/2.0 200 OK\r\n\r\n", direct_t::RESPONSE, parser_t::error_t::INVALID_VERSION})
	)
);
