/**
 * @file: parameterized.cpp
 * @date: 2026-07-18
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты парсера протокола HTTP/1.x —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой разбора стартовой строки,
 *        заголовков и тела, кадрирования chunked и контроля лимитов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <tuple>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "http1.hpp"

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
	ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
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
	awh::http::parser_http_t::error_t error;
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
		ErrorTestParameter({"GE[T / HTTP/1.1\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_METHOD}),
		// Одиночный CR без последующего LF в конце строки
		ErrorTestParameter({"GET / HTTP/1.1\rX", direct_t::REQUEST, parser_http_t::error_t::INVALID_EOL}),
		// Неподдерживаемая версия протокола в запросе
		ErrorTestParameter({"GET / HTTP/3.0\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_VERSION}),
		// Некорректный литерал префикса версии протокола
		ErrorTestParameter({"GET / HTTX/1.1\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_VERSION}),
		// Недопустимый управляющий символ в request-target
		ErrorTestParameter({std::string("GET /pa\x01th HTTP/1.1\r\n\r\n"), direct_t::REQUEST, parser_http_t::error_t::INVALID_TARGET}),
		// Пробел перед двоеточием в имени заголовка
		ErrorTestParameter({"GET / HTTP/1.1\r\nHost : x\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_HEADER_TOKEN}),
		// Устаревший перенос строки заголовка (obs-fold)
		ErrorTestParameter({"GET / HTTP/1.1\r\nHost: x\r\n y\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_HEADER_TOKEN}),
		// Нечисловое значение заголовка Content-Length
		ErrorTestParameter({"POST / HTTP/1.1\r\nContent-Length: abc\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_CONTENT_LENGTH}),
		// Одновременные заголовки Content-Length и Transfer-Encoding (request smuggling)
		ErrorTestParameter({"POST / HTTP/1.1\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::CONTENT_LENGTH_CONFLICT}),
		// Различающиеся значения двух заголовков Content-Length (request smuggling)
		ErrorTestParameter({"POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::CONTENT_LENGTH_CONFLICT}),
		// Кодирование chunked не последнее в списке Transfer-Encoding
		ErrorTestParameter({"POST / HTTP/1.1\r\nTransfer-Encoding: chunked, gzip\r\n\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_TRANSFER_ENCODING}),
		// Недопустимый символ в размере чанка
		ErrorTestParameter({"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nXYZ\r\n", direct_t::REQUEST, parser_http_t::error_t::INVALID_CHUNK_SIZE}),
		// Отсутствие CRLF после данных чанка
		ErrorTestParameter({"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabcXX", direct_t::REQUEST, parser_http_t::error_t::INVALID_CHUNK_TERMINATOR}),
		// Статус-код ответа из четырёх цифр
		ErrorTestParameter({"HTTP/1.1 2000 OK\r\n\r\n", direct_t::RESPONSE, parser_http_t::error_t::INVALID_STATUS}),
		// Статус-код ответа содержащий буквы
		ErrorTestParameter({"HTTP/1.1 2A0 OK\r\n\r\n", direct_t::RESPONSE, parser_http_t::error_t::INVALID_STATUS}),
		// Неподдерживаемая версия протокола в ответе
		ErrorTestParameter({"HTTP/2.0 200 OK\r\n\r\n", direct_t::RESPONSE, parser_http_t::error_t::INVALID_VERSION})
	)
);

/**
 * @brief Метод тестирования отсутствия удержания входного буфера парсером
 *
 * @details Каждый фрагмент подаётся из отдельного одноразового буфера, который
 *          сразу после возврата из parse затирается и освобождается. Если парсер
 *          удерживал бы указатель во входные данные (вместо копирования в свои
 *          накопители и отдачи наружу только на время вызова), результат разбора
 *          оказался бы искажён
 *
 */
TEST_P(FragmentParameterizedFixture, PoisonedInputParsingTest){
	// Создаём объект парсера запросов клиента
	auto parser = this->make(direct_t::REQUEST);
	// Создаём объект сборщика событий парсера
	events_t events;
	// Подписываем сборщик событий на все функции обратного вызова парсера
	this->attach(* parser, events);
	/**
	 * Формируем запрос, задействующий все накопители парсера: метод, URI-адрес,
	 * имена и значения заголовков, расширения чанков, данные тела и трейлеры
	 */
	const std::string message =
		"POST /path/to/resource?query=value HTTP/1.1\r\n"
		"Host: anyks.com\r\n"
		"X-Long-Header-Name: value with spaces\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"6;name=first\r\nAWH is\r\n"
		"9;name=second\r\n awesome!\r\n"
		"0\r\n"
		"X-Check: done\r\n"
		"\r\n";
	// Общее количество обработанных байт
	size_t total = 0;
	/**
	 * Выполняем подачу данных фрагментами заданного размера из одноразовых буферов
	 */
	for(size_t i = 0; i < message.size(); i += this->_fragment){
		// Определяем размер очередного фрагмента данных
		const size_t size = ((message.size() - i) < this->_fragment ? (message.size() - i) : this->_fragment);
		// Формируем одноразовый буфер под очередной фрагмент данных
		std::unique_ptr <char []> scratch(new char[size]);
		// Копируем очередной фрагмент данных в одноразовый буфер
		std::memcpy(scratch.get(), (message.data() + i), size);
		// Выполняем разбор очередного фрагмента данных и считаем обработанные байты
		const size_t bytes = parser->parse(scratch.get(), size);
		// Проверяем что фрагмент обработан целиком (парсер не требует повторной подачи хвоста)
		ASSERT_EQ(bytes, size);
		// Учитываем обработанные байты
		total += bytes;
		// Затираем отданный фрагмент до следующего вызова парсера
		std::memset(scratch.get(), 0xEE, size);
	}
	// Проверяем что все данные обработаны
	ASSERT_EQ(total, message.size());
	// Проверяем что сообщение полностью разобрано
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Получаем объект провайдера заголовков запроса клиента
	const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
	// Проверяем что метод запроса разобран без искажений
	ASSERT_EQ(request->method, method_t::POST);
	// Проверяем что URI-адрес запроса разобран без искажений
	ASSERT_EQ(request->uri, "/path/to/resource?query=value");
	// Проверяем что разобраны все три заголовка
	ASSERT_EQ(events.headers.size(), 3u);
	// Проверяем что имя длинного заголовка разобрано без искажений
	ASSERT_EQ(events.headers[1].first, "X-Long-Header-Name");
	// Проверяем что значение длинного заголовка разобрано без искажений
	ASSERT_EQ(events.headers[1].second, "value with spaces");
	// Проверяем что тело сообщения собрано без искажений
	ASSERT_EQ(events.body, "AWH is awesome!");
	// Проверяем что расширения первого чанка разобраны без искажений
	ASSERT_EQ(std::get <2> (events.chunks.front()), "name=first");
	// Проверяем что разобран один трейлер
	ASSERT_EQ(events.trailers.size(), 1u);
	// Проверяем что имя трейлера разобрано без искажений
	ASSERT_EQ(events.trailers[0].first, "X-Check");
	// Проверяем что значение трейлера разобрано без искажений
	ASSERT_EQ(events.trailers[0].second, "done");
}

/**
 * @brief Метод тестирования эквивалентности быстрого и посимвольного путей разбора заголовков
 *
 * @details Строка заголовка разбирается быстрым путём только когда присутствует
 *          во входном буфере целиком, поэтому размер фрагмента подачи сам по себе
 *          переключает пути: подача по одному октету не даёт быстрому пути
 *          сработать ни разу, подача целиком задействует его на каждой строке.
 *          Поток событий обязан совпадать при любом размере фрагмента - иначе
 *          быстрый путь меняет наблюдаемое поведение разбора
 *
 */
TEST_P(FragmentParameterizedFixture, HeaderFastPathEquivalenceTest){
	/**
	 * @brief Функция разбора сообщения с заданным размером фрагмента подачи
	 *
	 * @param parser   объект парсера
	 * @param events   объект сборщика событий парсера
	 * @param message  разбираемое сообщение
	 * @param fragment размер фрагмента подачи
	 *
	 */
	auto feed = [](parser_http_t & parser, const std::string & message, const size_t fragment) noexcept -> void {
		/**
		 * Выполняем подачу данных фрагментами заданного размера
		 */
		for(size_t i = 0; i < message.size(); i += fragment)
			// Выполняем разбор очередного фрагмента данных
			parser.parse(message.data() + i, std::min(fragment, (message.size() - i)));
	};
	/**
	 * Набор сообщений, покрывающих как безусловно корректные строки заголовков,
	 * так и отклонения, которые быстрый путь обязан передавать посимвольному:
	 * пустые значения, OWS по краям, obs-fold, пробел перед двоеточием,
	 * недопустимые символы значения и трейлеры
	 */
	const std::vector <std::string> messages = {
		"GET / HTTP/1.1\r\nHost: anyks.com\r\n\r\n",
		"GET / HTTP/1.1\r\nHost: anyks.com\r\nX-Empty:\r\nX-Spaces:   value   \r\nX-Tab:\tvalue\t\r\n\r\n",
		"POST / HTTP/1.1\r\nHost: anyks.com\r\nContent-Length: 5\r\n\r\nhello",
		"POST / HTTP/1.1\r\nHost: anyks.com\r\nTransfer-Encoding: chunked\r\n\r\n3;x=1\r\nabc\r\n0\r\nX-Check: done\r\n\r\n",
		"GET / HTTP/1.1\r\nHost : anyks.com\r\n\r\n",
		"GET / HTTP/1.1\r\nHost: anyks.com\r\n obsfold\r\n\r\n",
		std::string("GET / HTTP/1.1\r\nHost: any\x01ks.com\r\n\r\n"),
		"POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n",
		"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\n",
		"GET / HTTP/1.1\r\nHost: anyks.com\nX-Mixed: value\r\nX-Bare: value\n\r\n",
		"GET / HTTP/1.1\nHost: anyks.com\n\n",
		"GET / HTTP/1.1\r\nHost: anyks.com\r\nX-Trail-OWS: value \t \r\nX-Colon-Value: a:b:c\r\n\r\n"
	};
	/**
	 * Выполняем перебор всех проверяемых сообщений
	 */
	for(const auto & message : messages){
		// Создаём объект парсера эталонного разбора (подача по одному октету)
		auto reference = this->make(direct_t::REQUEST);
		// Создаём объект сборщика событий эталонного разбора
		events_t expected;
		// Подписываем сборщик событий эталонного разбора
		this->attach(* reference, expected);
		// Выполняем эталонный разбор посимвольной подачей
		feed(* reference, message, 1);
		// Создаём объект парсера проверяемого разбора
		auto parser = this->make(direct_t::REQUEST);
		// Создаём объект сборщика событий проверяемого разбора
		events_t actual;
		// Подписываем сборщик событий проверяемого разбора
		this->attach(* parser, actual);
		// Выполняем проверяемый разбор подачей фрагментами заданного размера
		feed(* parser, message, this->_fragment);
		// Проверяем что итоговый статус разбора совпадает
		ASSERT_EQ(parser->status(), reference->status()) << message;
		// Проверяем что код ошибки разбора совпадает
		ASSERT_EQ(parser->error(), reference->error()) << message;
		// Проверяем что собранное тело сообщения совпадает
		ASSERT_EQ(actual.body, expected.body) << message;
		// Проверяем что набор разобранных заголовков совпадает
		ASSERT_EQ(actual.headers, expected.headers) << message;
		// Проверяем что набор разобранных трейлеров совпадает
		ASSERT_EQ(actual.trailers, expected.trailers) << message;
		// Проверяем что последовательность фазовых событий совпадает
		ASSERT_EQ(actual.phases, expected.phases) << message;
		// Проверяем что последовательность событий границ чанков совпадает
		ASSERT_EQ(actual.chunks, expected.chunks) << message;
		// Проверяем что кадрирование тела определено одинаково
		ASSERT_EQ(parser->message().flags.chunked, reference->message().flags.chunked) << message;
		// Проверяем что размер тела определён одинаково
		ASSERT_EQ(parser->message().bodySize, reference->message().bodySize) << message;
	}
}

/**
 * @brief Метод тестирования эквивалентности быстрого и посимвольного путей разбора метода запроса
 *
 * @details Метод запроса разбирается быстрым путём только когда присутствует во
 *          входном буфере целиком вместе с завершающим разделителем, поэтому
 *          размер фрагмента подачи сам по себе переключает пути: подача по одному
 *          октету не даёт быстрому пути сработать ни разу. Проверяются как
 *          распознаваемые методы всех длин, так и то, что быстрый путь обязан
 *          передавать посимвольному: нераспознанные написания, недопустимые
 *          символы, отсутствие разделителя и превышение лимита длины стартовой строки
 *
 */
TEST_P(FragmentParameterizedFixture, MethodFastPathEquivalenceTest){
	/**
	 * @brief Структура проверяемого случая разбора метода запроса
	 *
	 */
	typedef struct Sample {
		// Разбираемое сообщение
		std::string message;
		// Максимальная длина стартовой строки (0 - лимит по умолчанию)
		size_t limit;
	} sample_t;
	/**
	 * @brief Функция разбора сообщения с заданным размером фрагмента подачи
	 *
	 * @param parser   объект парсера
	 * @param message  разбираемое сообщение
	 * @param fragment размер фрагмента подачи
	 *
	 */
	auto feed = [](parser_http_t & parser, const std::string & message, const size_t fragment) noexcept -> void {
		/**
		 * Выполняем подачу данных фрагментами заданного размера
		 */
		for(size_t i = 0; i < message.size(); i += fragment)
			// Выполняем разбор очередного фрагмента данных
			parser.parse(message.data() + i, std::min(fragment, (message.size() - i)));
	};
	/**
	 * Набор проверяемых случаев разбора метода запроса
	 */
	const std::vector <sample_t> samples = {
		// Распознаваемые методы всех встречающихся длин
		sample_t({"GET / HTTP/1.1\r\n\r\n", 0}),
		sample_t({"PUT / HTTP/1.1\r\nContent-Length: 0\r\n\r\n", 0}),
		sample_t({"POST /x HTTP/1.1\r\nContent-Length: 0\r\n\r\n", 0}),
		sample_t({"TRACE / HTTP/1.1\r\n\r\n", 0}),
		sample_t({"DELETE / HTTP/1.1\r\n\r\n", 0}),
		sample_t({"OPTIONS * HTTP/1.1\r\n\r\n", 0}),
		sample_t({"PROPPATCH / HTTP/1.1\r\n\r\n", 0}),
		// Регистрозависимость: строчное написание известным методом не является
		sample_t({"get / HTTP/1.1\r\n\r\n", 0}),
		// Нераспознанные, но синтаксически корректные методы
		sample_t({"PURGE / HTTP/1.1\r\n\r\n", 0}),
		sample_t({"X / HTTP/1.1\r\n\r\n", 0}),
		// Лишние пробелы между методом и request-target
		sample_t({"GET  / HTTP/1.1\r\n\r\n", 0}),
		// Недопустимые написания метода запроса
		sample_t({std::string("GE\x01T / HTTP/1.1\r\n\r\n"), 0}),
		sample_t({"GET/ HTTP/1.1\r\n\r\n", 0}),
		sample_t({" GET / HTTP/1.1\r\n\r\n", 0}),
		// Превышение лимита длины стартовой строки внутри метода запроса
		sample_t({"PROPPATCH / HTTP/1.1\r\n\r\n", 4}),
		// Превышение лимита длины стартовой строки на разделителе после метода
		sample_t({"POST / HTTP/1.1\r\n\r\n", 4})
	};
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(const auto & sample : samples){
		// Создаём объект парсера эталонного разбора (подача по одному октету)
		auto reference = this->make(direct_t::REQUEST);
		// Создаём объект парсера проверяемого разбора
		auto parser = this->make(direct_t::REQUEST);
		// Если лимит длины стартовой строки задан явно
		if(sample.limit > 0){
			// Получаем текущие лимиты безопасности
			parser_http_t::limits_t limits = reference->limits();
			// Устанавливаем максимальную длину стартовой строки
			limits.maxRequestLine = sample.limit;
			// Применяем изменённые лимиты безопасности эталонному разбору
			reference->limits(limits);
			// Применяем изменённые лимиты безопасности проверяемому разбору
			parser->limits(limits);
		}
		// Создаём объект сборщика событий эталонного разбора
		events_t expected;
		// Подписываем сборщик событий эталонного разбора
		this->attach(* reference, expected);
		// Выполняем эталонный разбор посимвольной подачей
		feed(* reference, sample.message, 1);
		// Создаём объект сборщика событий проверяемого разбора
		events_t actual;
		// Подписываем сборщик событий проверяемого разбора
		this->attach(* parser, actual);
		// Выполняем проверяемый разбор подачей фрагментами заданного размера
		feed(* parser, sample.message, this->_fragment);
		// Проверяем что итоговый статус разбора совпадает
		ASSERT_EQ(parser->status(), reference->status()) << sample.message;
		// Проверяем что код ошибки разбора совпадает
		ASSERT_EQ(parser->error(), reference->error()) << sample.message;
		// Проверяем что последовательность фазовых событий совпадает
		ASSERT_EQ(actual.phases, expected.phases) << sample.message;
		// Получаем объект провайдера заголовков проверяемого разбора
		const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
		// Получаем объект провайдера заголовков эталонного разбора
		const request_t * origin = static_cast <const request_t *> (reference->message().provider.get());
		// Проверяем что метод запроса классифицирован одинаково
		ASSERT_EQ(request->method, origin->method) << sample.message;
		// Проверяем что оригинальное написание метода сохранено одинаково
		ASSERT_EQ(request->methodName, origin->methodName) << sample.message;
		// Проверяем что URI-адрес запроса разобран одинаково
		ASSERT_EQ(request->uri, origin->uri) << sample.message;
		// Проверяем что версия протокола разобрана одинаково
		ASSERT_EQ(request->version, origin->version) << sample.message;
	}
}

/**
 * @brief Метод тестирования эквивалентности быстрого и посимвольного путей разбора версии протокола
 *
 * @details Литерал версии разбирается быстрым путём только когда присутствует во
 *          входном буфере целиком вместе с окончанием строки CRLF, поэтому размер
 *          фрагмента подачи сам по себе переключает пути. Проверяются оба
 *          допустимых написания версии и всё, что быстрый путь обязан передавать
 *          посимвольному: голое окончание строки, лишние пробелы, неподдерживаемые
 *          версии, обрыв литерала и превышение лимита длины стартовой строки
 *
 */
TEST_P(FragmentParameterizedFixture, VersionFastPathEquivalenceTest){
	/**
	 * @brief Структура проверяемого случая разбора версии протокола
	 *
	 */
	typedef struct Sample {
		// Разбираемое сообщение
		std::string message;
		// Максимальная длина стартовой строки (0 - лимит по умолчанию)
		size_t limit;
		// Режим строгой трактовки окончаний строк
		bool strictEOL;
		// Режим строгой трактовки лишних пробелов
		bool strictSpaces;
	} sample_t;
	/**
	 * @brief Функция разбора сообщения с заданным размером фрагмента подачи
	 *
	 * @param parser   объект парсера
	 * @param message  разбираемое сообщение
	 * @param fragment размер фрагмента подачи
	 *
	 */
	auto feed = [](parser_http_t & parser, const std::string & message, const size_t fragment) noexcept -> void {
		/**
		 * Выполняем подачу данных фрагментами заданного размера
		 */
		for(size_t i = 0; i < message.size(); i += fragment)
			// Выполняем разбор очередного фрагмента данных
			parser.parse(message.data() + i, std::min(fragment, (message.size() - i)));
	};
	/**
	 * Набор проверяемых случаев разбора версии протокола
	 */
	const std::vector <sample_t> samples = {
		// Оба допустимых написания версии протокола
		sample_t({"GET / HTTP/1.1\r\n\r\n", 0, false, false}),
		sample_t({"GET / HTTP/1.0\r\n\r\n", 0, false, false}),
		// Голое окончание строки после версии в толерантном и строгом режимах
		sample_t({"GET / HTTP/1.1\n\r\n", 0, false, false}),
		sample_t({"GET / HTTP/1.1\n\r\n", 0, true, false}),
		// Строгий режим окончаний строк не должен влиять на корректное CRLF
		sample_t({"GET / HTTP/1.1\r\n\r\n", 0, true, false}),
		// Лишние пробелы перед литералом версии в толерантном и строгом режимах
		sample_t({"GET /  HTTP/1.1\r\n\r\n", 0, false, false}),
		sample_t({"GET /  HTTP/1.1\r\n\r\n", 0, false, true}),
		// Пробел между литералом версии и окончанием строки
		sample_t({"GET / HTTP/1.1 \r\n\r\n", 0, false, false}),
		// Неподдерживаемые версии протокола
		sample_t({"GET / HTTP/2.0\r\n\r\n", 0, false, false}),
		sample_t({"GET / HTTP/1.2\r\n\r\n", 0, false, false}),
		sample_t({"GET / HTTP/0.9\r\n\r\n", 0, false, false}),
		// Искажённые написания литерала версии
		sample_t({"GET / HTTP1.1\r\n\r\n", 0, false, false}),
		sample_t({"GET / HTTP/1.\r\n\r\n", 0, false, false}),
		sample_t({"GET / HTTP/1.1\r\r\n\r\n", 0, false, false}),
		sample_t({"GET / http/1.1\r\n\r\n", 0, false, false}),
		// Превышение лимита длины стартовой строки на литерале версии
		sample_t({"GET / HTTP/1.1\r\n\r\n", 12, false, false})
	};
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(const auto & sample : samples){
		// Создаём объект парсера эталонного разбора (подача по одному октету)
		auto reference = this->make(direct_t::REQUEST);
		// Создаём объект парсера проверяемого разбора
		auto parser = this->make(direct_t::REQUEST);
		// Получаем текущие лимиты безопасности
		parser_http_t::limits_t limits = reference->limits();
		// Устанавливаем режим строгой трактовки окончаний строк
		limits.strictEOL = sample.strictEOL;
		// Устанавливаем режим строгой трактовки лишних пробелов
		limits.strictSpaces = sample.strictSpaces;
		// Если лимит длины стартовой строки задан явно
		if(sample.limit > 0)
			// Устанавливаем максимальную длину стартовой строки
			limits.maxRequestLine = sample.limit;
		// Применяем лимиты безопасности эталонному разбору
		reference->limits(limits);
		// Применяем лимиты безопасности проверяемому разбору
		parser->limits(limits);
		// Создаём объект сборщика событий эталонного разбора
		events_t expected;
		// Подписываем сборщик событий эталонного разбора
		this->attach(* reference, expected);
		// Выполняем эталонный разбор посимвольной подачей
		feed(* reference, sample.message, 1);
		// Создаём объект сборщика событий проверяемого разбора
		events_t actual;
		// Подписываем сборщик событий проверяемого разбора
		this->attach(* parser, actual);
		// Выполняем проверяемый разбор подачей фрагментами заданного размера
		feed(* parser, sample.message, this->_fragment);
		// Проверяем что итоговый статус разбора совпадает
		ASSERT_EQ(parser->status(), reference->status()) << sample.message;
		// Проверяем что код ошибки разбора совпадает
		ASSERT_EQ(parser->error(), reference->error()) << sample.message;
		// Проверяем что последовательность фазовых событий совпадает
		ASSERT_EQ(actual.phases, expected.phases) << sample.message;
		// Проверяем что набор разобранных заголовков совпадает
		ASSERT_EQ(actual.headers, expected.headers) << sample.message;
		// Получаем объект провайдера заголовков проверяемого разбора
		const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
		// Получаем объект провайдера заголовков эталонного разбора
		const request_t * origin = static_cast <const request_t *> (reference->message().provider.get());
		// Проверяем что версия протокола разобрана одинаково
		ASSERT_EQ(request->version, origin->version) << sample.message;
		// Проверяем что метод запроса классифицирован одинаково
		ASSERT_EQ(request->method, origin->method) << sample.message;
		// Проверяем что URI-адрес запроса разобран одинаково
		ASSERT_EQ(request->uri, origin->uri) << sample.message;
	}
}

/**
 * @brief Метод тестирования совпадения состояния стартовой строки после отказа по лимиту
 *
 * @details Крупноблочные пути проверяют лимит длины стартовой строки один раз на
 *          весь участок, а посимвольный - на каждом октете, поэтому недособранное
 *          состояние после отказа легко расходится: адрес запроса у одного пути
 *          заполнен до предела, у другого пуст, а разобранная версия протокола у
 *          одного установлена, у другого осталась значением по умолчанию. Состояние
 *          отвергнутого сообщения обязано совпадать: иначе то, что видит потребитель
 *          после отказа, зависело бы от разбиения входа сетевым слоем
 *
 */
TEST_P(FragmentParameterizedFixture, StartLineOverflowStateEquivalenceTest){
	/**
	 * @brief Функция разбора сообщения с заданным размером фрагмента подачи
	 *
	 * @param parser   объект парсера
	 * @param message  разбираемое сообщение
	 * @param fragment размер фрагмента подачи
	 *
	 */
	auto feed = [](parser_http_t & parser, const std::string & message, const size_t fragment) noexcept -> void {
		/**
		 * Выполняем подачу данных фрагментами заданного размера
		 */
		for(size_t i = 0; i < message.size(); i += fragment)
			// Выполняем разбор очередного фрагмента данных
			parser.parse(message.data() + i, std::min(fragment, (message.size() - i)));
	};
	/**
	 * Набор сообщений, на которых лимит выбирается в разных частях стартовой строки:
	 * внутри метода, внутри адреса запроса и на литерале версии протокола
	 */
	const std::vector <std::string> messages = {
		"GET /aaaaaaaa HTTP/1.1\r\n\r\n",
		"POST /ab HTTP/1.0\r\n\r\n",
		"OPTIONS /x HTTP/1.1\r\n\r\n",
		"PROPPATCH /path/to/resource HTTP/1.1\r\n\r\n"
	};
	/**
	 * Выполняем перебор всех проверяемых сообщений
	 */
	for(const auto & message : messages){
		/**
		 * Выполняем перебор лимита длины стартовой строки, проходя точку отказа
		 * в каждой её части
		 */
		for(size_t limit = 1; limit <= 40; limit++){
			// Создаём объект парсера эталонного разбора (подача по одному октету)
			auto reference = this->make(direct_t::REQUEST);
			// Создаём объект парсера проверяемого разбора
			auto parser = this->make(direct_t::REQUEST);
			// Получаем текущие лимиты безопасности
			parser_http_t::limits_t limits = reference->limits();
			// Устанавливаем максимальную длину стартовой строки
			limits.maxRequestLine = limit;
			// Применяем лимиты безопасности эталонному разбору
			reference->limits(limits);
			// Применяем лимиты безопасности проверяемому разбору
			parser->limits(limits);
			// Выполняем эталонный разбор посимвольной подачей
			feed(* reference, message, 1);
			// Выполняем проверяемый разбор подачей фрагментами заданного размера
			feed(* parser, message, this->_fragment);
			// Формируем сведения о проверяемом случае
			const std::string details = (message + " (лимит " + std::to_string(limit) + ")");
			// Проверяем что итоговый статус разбора совпадает
			ASSERT_EQ(parser->status(), reference->status()) << details;
			// Проверяем что код ошибки разбора совпадает
			ASSERT_EQ(parser->error(), reference->error()) << details;
			// Получаем объект провайдера заголовков проверяемого разбора
			const request_t * request = static_cast <const request_t *> (parser->message().provider.get());
			// Получаем объект провайдера заголовков эталонного разбора
			const request_t * origin = static_cast <const request_t *> (reference->message().provider.get());
			// Проверяем что состояние адреса запроса после отказа совпадает
			ASSERT_EQ(request->uri, origin->uri) << details;
			// Проверяем что состояние версии протокола после отказа совпадает
			ASSERT_EQ(request->version, origin->version) << details;
			// Проверяем что состояние метода запроса после отказа совпадает
			ASSERT_EQ(request->method, origin->method) << details;
			// Проверяем что оригинальное написание метода после отказа совпадает
			ASSERT_EQ(request->methodName, origin->methodName) << details;
		}
	}
}

/**
 * @brief Метод тестирования отбраковки непригодных полей блока трейлеров
 *
 * @details RFC 9110 §6.5.1 не допускает передачу в трейлерах полей, вычисление
 *          которых обязано предшествовать получению тела: кадрирования сообщения,
 *          маршрутизации, модификаторов запроса, аутентификации, управляющих данных
 *          ответа и полей, определяющих способ обработки содержимого. RFC 9112 §7.1.2
 *          разрешает получателю такие трейлеры отбрасывать - иначе поле, принятое
 *          задним числом, переопределило бы трактовку уже разобранного тела в обход
 *          внешних фильтров безопасности. Проверяется, что до потребителя доходят
 *          только пригодные поля, и по одному представителю каждой категории
 *
 */
TEST_F(ParserFixture, ForbiddenTrailerCategoriesTest){
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
	/**
	 * Формируем перечень пригодных для трейлеров полей: Content-Digest определён
	 * RFC 9530 именно для передачи в трейлерах и заодно страхует ветку полей на
	 * "C" от чрезмерно широкого совпадения по префиксу
	 */
	const std::vector <std::string> allowed = {"X-Checksum", "Content-Digest", "Server-Timing"};
	/**
	 * Выполняем перебор всех непригодных для трейлеров полей
	 */
	for(auto & name : forbidden){
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Создаём объект сборщика событий парсера
		events_t events;
		// Подписываем сборщик событий на все функции обратного вызова парсера
		this->attach(* parser, events);
		// Формируем сообщение с блоком трейлеров из проверяемого поля и одного пригодного
		const std::string message = (
			"HTTP/1.1 200 OK\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5\r\nhello\r\n"
			"0\r\n" + name + ": value\r\n"
			"X-Checksum: abc\r\n"
			"\r\n"
		);
		// Формируем описание проверяемого поля для диагностики
		const std::string details = ("trailer field: " + name);
		// Выполняем разбор сформированного сообщения
		ASSERT_EQ(parser->parse(message.data(), message.size()), message.size()) << details;
		// Проверяем что сообщение полностью разобрано
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE) << details;
		// Проверяем что тело сообщения собрано без искажений
		ASSERT_EQ(events.body, "hello") << details;
		// Проверяем что до потребителя дошёл единственный пригодный трейлер
		ASSERT_EQ(events.trailers.size(), 1u) << details;
		// Проверяем что дошедший трейлер является пригодным
		ASSERT_EQ(events.trailers.front().first, "X-Checksum") << details;
	}
	/**
	 * Выполняем перебор всех пригодных для трейлеров полей
	 */
	for(auto & name : allowed){
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Создаём объект сборщика событий парсера
		events_t events;
		// Подписываем сборщик событий на все функции обратного вызова парсера
		this->attach(* parser, events);
		// Формируем сообщение с блоком трейлеров из проверяемого поля
		const std::string message = (
			"HTTP/1.1 200 OK\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5\r\nhello\r\n"
			"0\r\n" + name + ": value\r\n"
			"\r\n"
		);
		// Формируем описание проверяемого поля для диагностики
		const std::string details = ("trailer field: " + name);
		// Выполняем разбор сформированного сообщения
		ASSERT_EQ(parser->parse(message.data(), message.size()), message.size()) << details;
		// Проверяем что сообщение полностью разобрано
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE) << details;
		// Проверяем что пригодный трейлер дошёл до потребителя
		ASSERT_EQ(events.trailers.size(), 1u) << details;
		// Проверяем что имя пригодного трейлера передано без искажений
		ASSERT_EQ(events.trailers.front().first, name) << details;
	}
}

/**
 * @brief Метод тестирования отказа при получении сообщения HTTP/1.0 с Transfer-Encoding
 *
 * @details RFC 9112 §6.1 требует считать сообщение HTTP/1.0, несущее заголовок
 *          Transfer-Encoding, сообщением с неисправным кадрированием - даже при
 *          наличии Content-Length - и закрывать соединение после его обработки.
 *          Такое сообщение почти наверняка прошло через звено, не обработавшее
 *          кодирование chunked, и часть тела осталась в его буфере: продолжение
 *          работы по соединению прочитало бы этот остаток как следующее сообщение.
 *          Проверяется, что отказ наступает в обоих направлениях трафика и что
 *          то же сообщение версии HTTP/1.1 разбирается штатно
 *
 */
TEST_F(ParserFixture, LegacyTransferEncodingTest){
	// Формируем перечень проверяемых сообщений: направление, версия, признак отказа и само сообщение
	const std::vector <std::tuple <direct_t, std::string, bool, std::string>> samples = {
		// Ответ сервера версии HTTP/1.0 с объявленным кодированием - кадрирование неисправно
		{direct_t::RESPONSE, "HTTP/1.0 response", true,
			"HTTP/1.0 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n"},
		// Ответ сервера версии HTTP/1.0 с объявленным кодированием и размером тела - отказ обязан наступить и здесь
		{direct_t::RESPONSE, "HTTP/1.0 response with Content-Length", true,
			"HTTP/1.0 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\nhello"},
		// Запрос клиента версии HTTP/1.0 с объявленным кодированием - кадрирование неисправно
		{direct_t::REQUEST, "HTTP/1.0 request", true,
			"POST /upload HTTP/1.0\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n"},
		// То же сообщение версии HTTP/1.1 - кодирование законно и сообщение обязано разобраться
		{direct_t::RESPONSE, "HTTP/1.1 response", false,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n"}
	};
	/**
	 * Выполняем перебор всех проверяемых сообщений
	 */
	for(auto & sample : samples){
		// Создаём объект парсера-приёмника заданного направления
		auto parser = this->make(std::get <0> (sample));
		// Создаём объект сборщика событий парсера
		events_t events;
		// Подписываем сборщик событий на все функции обратного вызова парсера
		this->attach(* parser, events);
		// Получаем описание проверяемого сообщения для диагностики
		const std::string & details = std::get <1> (sample);
		// Получаем разбираемое сообщение
		const std::string & message = std::get <3> (sample);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Если сообщение обязано быть отвергнуто
		if(std::get <2> (sample)){
			// Проверяем что разбор завершился ошибкой
			ASSERT_EQ(parser->status(), parser_t::status_t::ERROR) << details;
			// Проверяем что зафиксирована ошибка некорректного транспортного кодирования
			ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_TRANSFER_ENCODING) << details;
			// Проверяем что тело сообщения до потребителя не дошло
			ASSERT_TRUE(events.body.empty()) << details;
		// Если сообщение обязано быть разобрано
		} else {
			// Проверяем что сообщение полностью разобрано
			ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE) << details;
			// Проверяем что тело сообщения собрано без искажений
			ASSERT_EQ(events.body, "hello") << details;
		}
	}
}

/**
 * @brief Метод тестирования симметрии лимита стартовой строки запроса и ответа
 *
 * @details Настройка ограничивает длину стартовой строки одним значением, и стартовая
 *          строка ответа обязана укладываться в тот же бюджет, что и стартовая строка
 *          запроса. Раньше у ответа не учитывались обязательный пробел после версии
 *          и цифры кода состояния, а reason-phrase ограничивался отдельным лимитом
 *          той же величины - в сумме стартовая строка ответа занимала вдвое больше
 *          канала, чем разрешено запросу при том же значении настройки
 *
 */
TEST_F(ParserFixture, StartLineLimitSymmetryTest){
	/**
	 * @brief Функция разбора сообщения с заданным лимитом длины стартовой строки
	 *
	 * @param direct  направление разбираемого трафика
	 * @param limit   лимит длины стартовой строки
	 * @param message разбираемое сообщение
	 * @return        код ошибки разбора
	 *
	 */
	auto probe = [this](const direct_t direct, const size_t limit, const std::string & message) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника заданного направления
		auto parser = this->make(direct);
		// Получаем текущие лимиты безопасности разбора
		parser_http_t::limits_t limits = parser->limits();
		// Устанавливаем проверяемый лимит длины стартовой строки
		limits.maxRequestLine = limit;
		// Применяем изменённые лимиты безопасности разбора
		parser->limits(limits);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Устанавливаем лимит длины стартовой строки для проверки
	const size_t limit = 32;
	/**
	 * Проверяем стартовую строку ответа, укладывающуюся в лимит целиком
	 */
	{
		// Формируем стартовую строку длиной ровно в лимит: "HTTP/1.1 200 " (13) и reason-phrase
		const std::string message = ("HTTP/1.1 200 " + std::string(limit - 13, 'A') + "\r\nContent-Length: 0\r\n\r\n");
		// Проверяем что сообщение разобрано без ошибок
		ASSERT_EQ(probe(direct_t::RESPONSE, limit, message), parser_http_t::error_t::NONE);
	}
	/**
	 * Проверяем стартовую строку ответа, превышающую лимит на один октет
	 */
	{
		// Формируем стартовую строку длиной на октет больше лимита
		const std::string message = ("HTTP/1.1 200 " + std::string((limit - 13) + 1, 'A') + "\r\nContent-Length: 0\r\n\r\n");
		// Проверяем что зафиксировано превышение длины стартовой строки
		ASSERT_EQ(probe(direct_t::RESPONSE, limit, message), parser_http_t::error_t::URL_OVERFLOW);
	}
	/**
	 * Проверяем что бюджет стартовой строки запроса остался прежним
	 */
	{
		// Формируем стартовую строку запроса длиной ровно в лимит: "GET " (4) и " HTTP/1.1" (9)
		const std::string message = ("GET /" + std::string((limit - 13) - 1, 'a') + " HTTP/1.1\r\n\r\n");
		// Проверяем что сообщение разобрано без ошибок
		ASSERT_EQ(probe(direct_t::REQUEST, limit, message), parser_http_t::error_t::NONE);
	}
	{
		// Формируем стартовую строку запроса длиной на октет больше лимита
		const std::string message = ("GET /" + std::string(limit - 13, 'a') + " HTTP/1.1\r\n\r\n");
		// Проверяем что зафиксировано превышение длины стартовой строки
		ASSERT_EQ(probe(direct_t::REQUEST, limit, message), parser_http_t::error_t::URL_OVERFLOW);
	}
}

/**
 * @brief Метод тестирования прерывания разбора сбросом парсера из функции обратного вызова
 *
 * @details Сброс парсера из функции обратного вызова обнуляет конечный автомат прямо
 *          посреди активного разбора. Без прерывания цикл разбора продолжил бы читать
 *          оставшиеся байты уже обнулённым автоматом и принял бы середину текущего
 *          сообщения за начало следующего - молчаливая рассинхронизация потока.
 *          Проверяется, что разбор возвращает управление ровно на месте сброса,
 *          не превращает это в ошибку и что сброшенный парсер пригоден для
 *          следующего сообщения, поданного с его начала. Хвост прерванного
 *          сообщения при этом отбрасывается: границу следующего после сброса
 *          определить нечем
 *
 */
TEST_F(ParserFixture, ResetFromCallbackAbortsParseTest){
	/**
	 * Проверяем сброс из функции обратного вызова заголовка (посимвольный путь)
	 */
	{
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Формируем два конвейерных запроса в одном буфере
		const std::string first = "GET /first HTTP/1.1\r\nHost: anyks.com\r\nX-Mark: one\r\n";
		// Формируем второй конвейерный запрос
		const std::string second = "GET /second HTTP/1.1\r\nHost: anyks.com\r\n\r\n";
		// Формируем общий буфер подачи
		const std::string message = (first + "\r\n" + second);
		// Признак выполненного сброса парсера
		bool fired = false;
		// Устанавливаем функцию обратного вызова обработки заголовков сообщения
		parser->on(parser_http_t::header_callback_t([&parser, &fired](const uint32_t, const std::string_view name, const std::string_view, const parser_t::part_t) noexcept -> bool {
			// Если получен заголовок, на котором выполняется сброс
			if(!fired && (name == "X-Mark")){
				// Помечаем сброс выполненным
				fired = true;
				// Сбрасываем парсер прямо из функции обратного вызова
				parser->reset();
			}
			// Продолжаем разбор
			return true;
		}));
		// Выполняем разбор сформированного буфера
		const size_t consumed = parser->parse(message.data(), message.size());
		// Проверяем что сброс действительно выполнялся
		ASSERT_TRUE(fired);
		// Проверяем что разбор остановлен ровно на месте сброса
		ASSERT_EQ(consumed, first.size());
		// Проверяем что прерывание не превращено в ошибку разбора
		ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
		// Проверяем что итоговый статус сообщает о незавершённом разборе
		ASSERT_EQ(parser->status(), parser_t::status_t::PARTIAL);
		/**
		 * Хвост прерванного сообщения парсеру более не принадлежит: границу
		 * следующего сообщения после сброса определить нечем, и вызывающая сторона
		 * этот хвост отбрасывает. Проверяется, что сброшенный парсер пригоден для
		 * следующего сообщения, поданного с его начала
		 */
		ASSERT_EQ(parser->parse(second.data(), second.size()), second.size());
		// Проверяем что следующее сообщение полностью разобрано
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
		// Проверяем что разобрана стартовая строка именно следующего запроса
		ASSERT_EQ(static_cast <const request_t *> (parser->message().provider.get())->uri, "/second");
	}
	/**
	 * Проверяем сброс из функции обратного вызова тела (крупноблочный путь)
	 *
	 * Крупноблочные пути возвращаются к началу цикла через continue и его хвоста
	 * не проходят, поэтому прерывание проверяется для них отдельно
	 */
	{
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Формируем заголовки ответа с телом фиксированного размера
		const std::string head = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n";
		// Формируем общий буфер подачи: тело подаётся двумя частями
		const std::string message = (head + "01234" + "56789");
		// Признак выполненного сброса парсера
		bool fired = false;
		// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		parser->on(parser_http_t::data_callback_t([&parser, &fired](const uint32_t, const void *, const size_t, const bool) noexcept -> bool {
			// Если сброс ещё не выполнялся
			if(!fired){
				// Помечаем сброс выполненным
				fired = true;
				// Сбрасываем парсер прямо из функции обратного вызова
				parser->reset();
			}
			// Продолжаем разбор
			return true;
		}));
		// Выполняем разбор заголовков и первой части тела
		const size_t consumed = parser->parse(message.data(), (head.size() + 5));
		// Проверяем что сброс действительно выполнялся
		ASSERT_TRUE(fired);
		// Проверяем что разбор остановлен сразу после отданной части тела
		ASSERT_EQ(consumed, (head.size() + 5));
		// Проверяем что прерывание не превращено в ошибку разбора
		ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
		// Проверяем что итоговый статус сообщает о незавершённом разборе
		ASSERT_EQ(parser->status(), parser_t::status_t::PARTIAL);
		/**
		 * Проверяем чистоту состояния после сброса: счётчики тела обнулены сбросом,
		 * и учёт уже отданной порции увёл бы остаток тела в переполнение. Наблюдаемо
		 * это по следующему сообщению - при испорченных счётчиках его тело не сошлось
		 * бы либо упёрлось в лимит размера
		 */
		// Собранное тело следующего сообщения
		std::string next;
		// Устанавливаем безобидную функцию обратного вызова обработки фрагмента тела
		parser->on(parser_http_t::data_callback_t([&next](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Собираем фрагмент принятого тела сообщения
			next.append(static_cast <const char *> (buffer), size);
			// Продолжаем разбор
			return true;
		}));
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Формируем следующее сообщение с телом того же размера
		const std::string second = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nabcdefghij";
		// Выполняем разбор следующего сообщения
		ASSERT_EQ(parser->parse(second.data(), second.size()), second.size());
		// Проверяем что следующее сообщение полностью разобрано
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
		// Проверяем что тело следующего сообщения собрано без искажений
		ASSERT_EQ(next, "abcdefghij");
	}
	/**
	 * Проверяем сброс из функции обратного вызова фазы разбора
	 *
	 * Фазовые обработчики вызываются из мест, которые следом записывают в автомат -
	 * выбирают кадрирование тела, взводят состояние трейлеров, помечают сообщение
	 * завершённым. Проверка в начале цикла разбора это не ловит: она срабатывает
	 * лишь на следующей итерации, уже после такой записи
	 */
	{
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Признак выполненного сброса парсера
		bool fired = false;
		// Устанавливаем функцию обратного вызова обработки фазы разбора сообщения
		parser->on(parser_http_t::phase_callback_t([&parser, &fired](const uint32_t, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
			// Если получено завершение приёма тела сообщения и сброс ещё не выполнялся
			if(!fired && (phase == parser_t::phase_t::END) && (part == parser_t::part_t::BODY)){
				// Помечаем сброс выполненным
				fired = true;
				// Сбрасываем парсер прямо из функции обратного вызова
				parser->reset();
			}
			// Продолжаем разбор
			return true;
		}));
		// Формируем сообщение с телом фиксированного размера
		const std::string message = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Проверяем что сброс действительно выполнялся
		ASSERT_TRUE(fired);
		// Проверяем что прерывание не превращено в ошибку разбора
		ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
		/**
		 * Сброшенный парсер не вправе объявлять сообщение полностью разобранным:
		 * состояние обнулено, и потребитель принял бы за готовое то, чего нет
		 */
		ASSERT_NE(parser->status(), parser_t::status_t::COMPLETE);
	}
}

/**
 * @brief Метод тестирования пропуска пустых строк перед стартовой строкой запроса
 *
 * @details RFC 9112 §2.2 требует от сервера игнорировать хотя бы одну пустую строку
 *          перед request-line: устаревшие клиенты дописывают лишний CRLF после тела,
 *          и без пропуска соединение keep-alive обрывалось бы на ровном месте.
 *          Правило адресовано серверу, поэтому к ответам не применяется. Пропуск
 *          выключается строгим режимом окончаний строк - расхождение в трактовке
 *          пустой строки с соседним звеном цепочки смещает границы сообщений
 *          в конвейере, и это тот же вектор, что и у одиночного LF
 *
 */
TEST_F(ParserFixture, LeadingBlankLinesTest){
	/**
	 * @brief Функция разбора запроса с заданным префиксом и режимом строгости
	 *
	 * @param prefix префикс перед стартовой строкой запроса
	 * @param strict признак строгой трактовки окончаний строк
	 * @return       код ошибки разбора
	 *
	 */
	auto probe = [this](const std::string & prefix, const bool strict) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Получаем текущие лимиты безопасности разбора
		parser_http_t::limits_t limits = parser->limits();
		// Устанавливаем проверяемый режим трактовки окончаний строк
		limits.strictEOL = strict;
		// Применяем изменённые лимиты безопасности разбора
		parser->limits(limits);
		// Формируем разбираемое сообщение
		const std::string message = (prefix + "GET /path HTTP/1.1\r\nHost: anyks.com\r\n\r\n");
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Проверяем что одна пустая строка перед запросом пропускается
	ASSERT_EQ(probe("\r\n", false), parser_http_t::error_t::NONE);
	// Проверяем что несколько пустых строк перед запросом пропускаются
	ASSERT_EQ(probe("\r\n\r\n\r\n", false), parser_http_t::error_t::NONE);
	// Проверяем что пустая строка из одиночного LF перед запросом пропускается
	ASSERT_EQ(probe("\n", false), parser_http_t::error_t::NONE);
	// Проверяем что поток пустых строк не удерживает соединение без продвижения
	ASSERT_EQ(probe(std::string(64, '\n'), false), parser_http_t::error_t::INVALID_METHOD);
	// Проверяем что предельно допустимое число пустых октетов ещё пропускается
	ASSERT_EQ(probe(std::string(16, '\n'), false), parser_http_t::error_t::NONE);
	// Проверяем что превышение предела на один октет уже отвергается
	ASSERT_EQ(probe(std::string(17, '\n'), false), parser_http_t::error_t::INVALID_METHOD);
	// Проверяем что строгий режим окончаний строк пустую строку перед запросом отвергает
	ASSERT_EQ(probe("\r\n", true), parser_http_t::error_t::INVALID_EOL);
	/**
	 * Проверяем что фаза начала сообщения не выдаётся на пропускаемые пустые строки
	 */
	{
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Создаём объект сборщика событий парсера
		events_t events;
		// Подписываем сборщик событий на все функции обратного вызова парсера
		this->attach(* parser, events);
		// Формируем сообщение с пустыми строками перед стартовой строкой
		const std::string message = "\r\n\r\nGET /path HTTP/1.1\r\nHost: anyks.com\r\n\r\n";
		// Выполняем разбор сформированного сообщения
		ASSERT_EQ(parser->parse(message.data(), message.size()), message.size());
		// Проверяем что сообщение полностью разобрано
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
		// Количество полученных фаз начала разбора сообщения
		size_t begins = 0;
		/**
		 * Выполняем перебор всех собранных фазовых событий
		 */
		for(auto & phase : events.phases){
			// Если получена фаза начала разбора сообщения
			if((phase.first == parser_t::phase_t::BEGIN) && (phase.second == parser_t::part_t::NONE))
				// Учитываем полученную фазу начала разбора сообщения
				++begins;
		}
		// Проверяем что фаза начала сообщения выдана ровно один раз
		ASSERT_EQ(begins, 1u);
	}
	/**
	 * Проверяем что закрытие соединения после одних лишь пустых строк остаётся штатным
	 *
	 * Ради этого пропущенные октеты и не входят в бюджет стартовой строки: его нулевое
	 * значение служит признаком "между сообщениями", и закрытие keep-alive соединения
	 * после случайного CRLF иначе стало бы обрывом посреди сообщения
	 */
	{
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Подаём только пустые строки, ничего кроме них
		const std::string message = "\r\n\r\n";
		// Выполняем разбор поданных пустых строк
		ASSERT_EQ(parser->parse(message.data(), message.size()), message.size());
		// Уведомляем парсер о закрытии соединения
		parser->eof();
		// Проверяем что закрытие соединения не признано обрывом сообщения
		ASSERT_EQ(parser->error(), parser_http_t::error_t::NONE);
	}
	/**
	 * Проверяем что к ответу сервера правило не применяется
	 */
	{
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Формируем ответ с пустой строкой перед стартовой строкой
		const std::string message = "\r\nHTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Проверяем что ведущая пустая строка перед ответом остаётся ошибкой версии
		ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_VERSION);
	}
}

/**
 * @brief Метод тестирования допустимости октетов в расширениях чанка
 *
 * @details Структурно расширения чанка не разбираются, но по RFC 9112 §7.1.1 они
 *          состоят из token и token либо quoted-string. DEL не входит ни в token,
 *          ни в qdtext и обязан отвергаться - как и в значении заголовка, - а
 *          obs-text законен внутри quoted-string и обязан приниматься
 *
 */
TEST_F(ParserFixture, ChunkExtensionOctetsTest){
	/**
	 * @brief Функция разбора ответа с заданным октетом внутри расширения чанка
	 *
	 * @param letter октет, помещаемый в расширение чанка
	 * @return       код ошибки разбора
	 *
	 */
	auto probe = [this](const char letter) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Формируем ответ с телом в кодировке chunked и расширением у первого чанка
		const std::string message = (
			"HTTP/1.1 200 OK\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"3;a=" + std::string(1, letter) + "\r\nAWH\r\n"
			"0\r\n\r\n"
		);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Проверяем что обычный символ token в расширении чанка принимается
	ASSERT_EQ(probe('b'), parser_http_t::error_t::NONE);
	// Проверяем что пробел в расширении чанка принимается (BWS)
	ASSERT_EQ(probe(' '), parser_http_t::error_t::NONE);
	// Проверяем что obs-text в расширении чанка принимается (законен внутри quoted-string)
	ASSERT_EQ(probe(static_cast <char> (0x80)), parser_http_t::error_t::NONE);
	// Проверяем что DEL в расширении чанка отвергается
	ASSERT_EQ(probe(static_cast <char> (0x7F)), parser_http_t::error_t::INVALID_CHUNK_SIZE);
	// Проверяем что управляющий символ в расширении чанка отвергается
	ASSERT_EQ(probe(static_cast <char> (0x01)), parser_http_t::error_t::INVALID_CHUNK_SIZE);
}

/**
 * @brief Метод тестирования пустых элементов в списке транспортного кодирования
 *
 * @details RFC 9110 §5.6.1.2 обязывает получателя разбирать и игнорировать пустые
 *          элементы списка, поэтому завершающая запятая не отменяет того, что
 *          последним кодированием объявлен chunked. Отвергать такое значение
 *          означало бы нарушить прямое требование стандарта
 *
 */
TEST_F(ParserFixture, TransferEncodingEmptyListElementTest){
	/**
	 * @brief Функция разбора ответа с заданным значением заголовка Transfer-Encoding
	 *
	 * @param value  значение заголовка транспортного кодирования
	 * @param events сборщик событий разбора
	 * @return       код ошибки разбора
	 *
	 */
	auto probe = [this](const std::string & value, events_t & events) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Подписываем сборщик событий на все функции обратного вызова парсера
		this->attach(* parser, events);
		// Формируем ответ с телом в кодировке chunked
		const std::string message = (
			"HTTP/1.1 200 OK\r\n"
			"Transfer-Encoding: " + value + "\r\n"
			"\r\n"
			"3\r\nAWH\r\n"
			"0\r\n\r\n"
		);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	/**
	 * Завершающая запятая: пустой элемент игнорируется, кадрирование остаётся chunked
	 */
	{
		// Создаём объект сборщика событий парсера
		events_t events;
		// Проверяем что значение с завершающей запятой принимается
		ASSERT_EQ(probe("chunked,", events), parser_http_t::error_t::NONE);
		// Проверяем что тело разобрано кодировкой chunked
		ASSERT_EQ(events.body, "AWH");
	}
	/**
	 * Пустые элементы вокруг кодирования игнорируются точно так же
	 */
	{
		// Создаём объект сборщика событий парсера
		events_t events;
		// Проверяем что значение с пустыми элементами по краям принимается
		ASSERT_EQ(probe(" , chunked , ", events), parser_http_t::error_t::NONE);
		// Проверяем что тело разобрано кодировкой chunked
		ASSERT_EQ(events.body, "AWH");
	}
	/**
	 * Пустые элементы не превращают непоследний chunked в последний
	 */
	{
		// Создаём объект сборщика событий парсера
		events_t events;
		// Проверяем что chunked перед другим кодированием остаётся ошибкой кадрирования
		ASSERT_EQ(probe("chunked, , gzip", events), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
	}
}

/**
 * @brief Метод тестирования отсечения параметров у токенов заголовка Connection
 *
 * @details Элементом списка Connection обязан быть голый токен (RFC 9110 §7.6.1),
 *          и точка с запятой в нём недопустима. Расхождение в трактовке такого значения
 *          с соседним звеном цепочки решается в пользу закрытия соединения: удержать
 *          открытым то, что пир считает закрытым, опаснее обратного - следующее
 *          сообщение ушло бы в соединение, которого уже нет
 *
 */
TEST_F(ParserFixture, ConnectionTokenParametersTest){
	/**
	 * @brief Функция разбора ответа с заданным значением заголовка Connection
	 *
	 * @param value значение заголовка Connection
	 * @return      признак переиспользуемости соединения
	 *
	 */
	auto probe = [this](const std::string & value) noexcept -> bool {
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Формируем разбираемое сообщение
		const std::string message = ("HTTP/1.1 200 OK\r\nConnection: " + value + "\r\nContent-Length: 0\r\n\r\n");
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим признак переиспользуемости соединения
		return parser->message().flags.keepAlive;
	};
	// Проверяем что голый токен закрытия соединения распознаётся
	ASSERT_FALSE(probe("close"));
	// Проверяем что токен закрытия соединения с параметром распознаётся так же
	ASSERT_FALSE(probe("close;foo"));
	// Проверяем что токен закрытия соединения с параметром распознаётся и внутри списка
	ASSERT_FALSE(probe("keep-alive, close;foo=bar"));
	// Проверяем что отсечение параметров не превращает посторонний токен в закрытие
	ASSERT_TRUE(probe("closely;foo"));
	// Проверяем что соединение без токена закрытия остаётся переиспользуемым
	ASSERT_TRUE(probe("keep-alive"));
}

/**
 * @brief Метод тестирования эквивалентности быстрого и посимвольного путей разбора стартовой строки ответа
 *
 * @details Стартовая строка ответа разбирается быстрым путём только когда присутствует
 *          во входном буфере целиком вместе с окончанием строки CRLF, поэтому размер
 *          фрагмента подачи сам по себе переключает пути. Проверяются оба допустимых
 *          написания версии, пустое и непустое пояснение к коду состояния и всё, что
 *          быстрый путь обязан передавать посимвольному: голое окончание строки, лишние
 *          пробелы, неподдерживаемые версии, некорректный код состояния, недопустимый
 *          символ пояснения и превышение лимита длины стартовой строки
 *
 */
TEST_P(FragmentParameterizedFixture, StatusLineFastPathEquivalenceTest){
	/**
	 * @brief Структура проверяемого случая разбора стартовой строки ответа
	 *
	 */
	typedef struct Sample {
		// Разбираемое сообщение
		std::string message;
		// Максимальная длина стартовой строки (0 - лимит по умолчанию)
		size_t limit;
		// Режим строгой трактовки окончаний строк
		bool strictEOL;
		// Режим строгой трактовки лишних пробелов
		bool strictSpaces;
	} sample_t;
	/**
	 * @brief Функция разбора сообщения с заданным размером фрагмента подачи
	 *
	 * @param parser   объект парсера
	 * @param message  разбираемое сообщение
	 * @param fragment размер фрагмента подачи
	 *
	 */
	auto feed = [](parser_http_t & parser, const std::string & message, const size_t fragment) noexcept -> void {
		/**
		 * Выполняем подачу данных фрагментами заданного размера
		 */
		for(size_t i = 0; i < message.size(); i += fragment)
			// Выполняем разбор очередного фрагмента данных
			parser.parse(message.data() + i, std::min(fragment, (message.size() - i)));
	};
	/**
	 * Набор проверяемых случаев разбора стартовой строки ответа
	 */
	const std::vector <sample_t> samples = {
		// Оба допустимых написания версии протокола
		sample_t({"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.0 200 OK\r\nContent-Length: 0\r\n\r\n", 0, false, false}),
		// Пустое пояснение к коду состояния с разделителем и без него
		sample_t({"HTTP/1.1 204\r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.1 204 \r\n\r\n", 0, false, false}),
		// Пояснение из нескольких слов и с пробелами
		sample_t({"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n", 0, false, false}),
		// Голое окончание стартовой строки в толерантном и строгом режимах
		sample_t({"HTTP/1.1 200 OK\nContent-Length: 0\r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.1 200 OK\nContent-Length: 0\r\n\r\n", 0, true, false}),
		// Строгий режим окончаний строк не должен влиять на корректное CRLF
		sample_t({"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", 0, true, false}),
		// Лишние пробелы перед кодом состояния в толерантном и строгом режимах
		sample_t({"HTTP/1.1  200 OK\r\nContent-Length: 0\r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.1  200 OK\r\nContent-Length: 0\r\n\r\n", 0, false, true}),
		// Неподдерживаемые и искажённые написания версии протокола
		sample_t({"HTTP/2.0 200 OK\r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.2 200 OK\r\n\r\n", 0, false, false}),
		sample_t({"HTTP1.1 200 OK\r\n\r\n", 0, false, false}),
		sample_t({"http/1.1 200 OK\r\n\r\n", 0, false, false}),
		// Некорректный код состояния: не число, лишняя цифра, недостаток цифр
		sample_t({"HTTP/1.1 2A0 OK\r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.1 2000 OK\r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.1 20 OK\r\n\r\n", 0, false, false}),
		// Отсутствие кода состояния вовсе
		sample_t({"HTTP/1.1 \r\n\r\n", 0, false, false}),
		sample_t({"HTTP/1.1\r\n\r\n", 0, false, false}),
		// Недопустимый символ в пояснении к коду состояния
		sample_t({"HTTP/1.1 200 O\x01K\r\n\r\n", 0, false, false}),
		// Превышение лимита длины стартовой строки на пояснении
		sample_t({"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", 13, false, false}),
		// Стартовая строка ровно по лимиту длины
		sample_t({"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", 15, false, false})
	};
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(const auto & sample : samples){
		// Создаём объект парсера эталонного разбора (подача по одному октету)
		auto reference = this->make(direct_t::RESPONSE);
		// Создаём объект парсера проверяемого разбора
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		reference->method(method_t::GET);
		// Устанавливаем метод запроса проверяемому разбору
		parser->method(method_t::GET);
		// Получаем текущие лимиты безопасности
		parser_http_t::limits_t limits = reference->limits();
		// Устанавливаем режим строгой трактовки окончаний строк
		limits.strictEOL = sample.strictEOL;
		// Устанавливаем режим строгой трактовки лишних пробелов
		limits.strictSpaces = sample.strictSpaces;
		// Если лимит длины стартовой строки задан явно
		if(sample.limit > 0)
			// Устанавливаем максимальную длину стартовой строки
			limits.maxRequestLine = sample.limit;
		// Применяем лимиты безопасности эталонному разбору
		reference->limits(limits);
		// Применяем лимиты безопасности проверяемому разбору
		parser->limits(limits);
		// Создаём объект сборщика событий эталонного разбора
		events_t expected;
		// Подписываем сборщик событий эталонного разбора
		this->attach(* reference, expected);
		// Выполняем эталонный разбор подачей по одному октету
		feed(* reference, sample.message, 1);
		// Создаём объект сборщика событий проверяемого разбора
		events_t actual;
		// Подписываем сборщик событий проверяемого разбора
		this->attach(* parser, actual);
		// Выполняем проверяемый разбор подачей фрагментами заданного размера
		feed(* parser, sample.message, this->_fragment);
		// Проверяем что итоговый статус разбора совпадает
		ASSERT_EQ(parser->status(), reference->status()) << sample.message;
		// Проверяем что код ошибки разбора совпадает
		ASSERT_EQ(parser->error(), reference->error()) << sample.message;
		// Проверяем что последовательность фазовых событий совпадает
		ASSERT_EQ(actual.phases, expected.phases) << sample.message;
		// Проверяем что собранные заголовки совпадают
		ASSERT_EQ(actual.headers, expected.headers) << sample.message;
		// Получаем объект провайдера заголовков проверяемого разбора
		const response_t * response = static_cast <const response_t *> (parser->message().provider.get());
		// Получаем объект провайдера заголовков эталонного разбора
		const response_t * origin = static_cast <const response_t *> (reference->message().provider.get());
		// Проверяем что код состояния разобран одинаково
		ASSERT_EQ(response->code, origin->code) << sample.message;
		// Проверяем что пояснение к коду состояния разобрано одинаково
		ASSERT_EQ(response->message, origin->message) << sample.message;
		// Проверяем что версия протокола разобрана одинаково
		ASSERT_EQ(response->version, origin->version) << sample.message;
	}
}
