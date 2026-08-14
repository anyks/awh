/**
 * @file parameterized.cpp
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
 * @brief Параметризованные тесты парсера протокола HTTP/1.x —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой разбора стартовой строки,
 *        заголовков и тела, кадрирования chunked и контроля лимитов
 *
 * @copyright Copyright © 2026
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
#include <cstring>
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
		MethodTestParameter({"DELETE", method_t::DELETE}),
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
 * @brief Метод проверки отказов при разборе непригодного блока трейлеров
 *
 * @details Блок трейлеров разбирается собственным набором состояний конечного автомата,
 *          и проверки в них повторяют проверки блока заголовков: символы имени и значения,
 *          лимиты длины, окончание строки. Повторение означает, что каждая из них может
 *          разойтись с оригиналом независимо, поэтому проверяется отдельно - тем более что
 *          блок трейлеров приходит после тела, то есть после того, как получатель уже начал
 *          считать сообщение доставленным
 *
 */
TEST_F(ParserFixture, TrailerErrorPathsTest){
	/**
	 * @brief Структура проверяемого отказа разбора блока трейлеров
	 *
	 */
	struct probe_t {
		// Название проверяемого отказа
		const char * name;
		// Разбираемый блок трейлеров (дописывается к завершённому телу)
		std::string trailers;
		// Ожидаемый код ошибки разбора
		parser_http_t::error_t error;
		// Требование строгого окончания строк
		bool strictEOL;
		// Предельная длина имени заголовка (ноль - значение по умолчанию)
		size_t maxHeaderName;
		// Предельная длина значения заголовка (ноль - значение по умолчанию)
		size_t maxHeaderValue;
		// Предельный суммарный размер заголовков (ноль - значение по умолчанию)
		size_t maxHeadersTotal;
	};
	// Формируем перечень проверяемых отказов разбора блока трейлеров
	const std::vector <probe_t> probes = {
		// Недопустимый символ в имени трейлера
		{"недопустимый символ имени", "X-Bad\x01Name: value\r\n\r\n", parser_http_t::error_t::INVALID_HEADER_TOKEN, false, 0, 0, 0},
		// Недопустимый символ в значении трейлера
		{"недопустимый символ значения", "X-Check: va\x01lue\r\n\r\n", parser_http_t::error_t::INVALID_HEADER_VALUE, false, 0, 0, 0},
		// Недопустимый символ в значении трейлера, начинающемся с OWS
		{"недопустимый символ после OWS", "X-Check: \x01value\r\n\r\n", parser_http_t::error_t::INVALID_HEADER_VALUE, false, 0, 0, 0},
		/**
		 * Пределы подобраны так, чтобы блок основных заголовков в них укладывался:
		 * иначе разбор отказал бы ещё до тела, и проверялась бы не та ветка
		 */
		// Превышение предельной длины имени трейлера
		{"превышение длины имени", "X-Very-Long-Trailer-Name-Here: value\r\n\r\n", parser_http_t::error_t::HEADER_OVERFLOW, false, 20, 0, 0},
		// Превышение предельной длины значения трейлера
		{"превышение длины значения", "X-Check: 0123456789\r\n\r\n", parser_http_t::error_t::HEADER_OVERFLOW, false, 0, 8, 0},
		/**
		 * Блок трейлеров получает собственный бюджет суммарного размера, поэтому предел
		 * должен вмещать блок основных заголовков, а поток OWS - его превышать
		 */
		{"превышение размера на OWS", ("X-Check:" + std::string(64, ' ') + "value\r\n\r\n"), parser_http_t::error_t::HEADER_OVERFLOW, false, 0, 0, 40},
		// Одиночный LF в роли окончания строки трейлера при строгом режиме
		{"голый LF после значения", "X-Check: done\n\r\n", parser_http_t::error_t::INVALID_EOL, true, 0, 0, 0},
		// Одиночный LF в роли окончания строки пустого значения при строгом режиме
		{"голый LF при пустом значении", "X-Check:\n\r\n", parser_http_t::error_t::INVALID_EOL, true, 0, 0, 0},
		// Одиночный LF в роли завершающей блок пустой строки при строгом режиме
		{"голый LF в конце блока", "X-Check: done\r\n\n", parser_http_t::error_t::INVALID_EOL, true, 0, 0, 0},
		// Пробел в начале строки трейлера (obs-fold запрещён RFC 7230 §3.2.4)
		{"obs-fold в начале строки", " X-Check: done\r\n\r\n", parser_http_t::error_t::INVALID_HEADER_TOKEN, false, 0, 0, 0},
		// Табуляция в начале строки трейлера
		{"табуляция в начале строки", "\tX-Check: done\r\n\r\n", parser_http_t::error_t::INVALID_HEADER_TOKEN, false, 0, 0, 0},
		// Символ, не являющийся символом токена, в начале строки трейлера
		{"не-токен в начале строки", "(X-Check: done\r\n\r\n", parser_http_t::error_t::INVALID_HEADER_TOKEN, false, 0, 0, 0},
		// Отсутствие LF после CR в конце строки трейлера
		{"CR без LF после значения", "X-Check: done\rX", parser_http_t::error_t::INVALID_EOL, false, 0, 0, 0},
		// Отсутствие LF после CR при пустом значении трейлера
		{"CR без LF при пустом значении", "X-Check:\rX", parser_http_t::error_t::INVALID_EOL, false, 0, 0, 0},
		// Отсутствие LF после CR завершающей блок пустой строки
		{"CR без LF в конце блока", "X-Check: done\r\n\rX", parser_http_t::error_t::INVALID_EOL, false, 0, 0, 0}
	};
	/**
	 * Каждый отказ проверяется двумя способами подачи
	 *
	 * Пределы длины имени и значения проверяются дважды: крупноблочным сканером,
	 * который принимает непрерывный участок целиком, и посимвольным разбором. При
	 * подаче сообщения одним куском срабатывает первый, и вторая защита остаётся
	 * недостижимой - до неё доходит только подача по одному октету, при которой
	 * непрерывного участка не возникает вовсе
	 */
	for(auto & probe : probes)
	for(const bool bytewise : {false, true}){
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Создаём объект сборщика событий парсера
		events_t events;
		// Подписываем сборщик событий на все функции обратного вызова парсера
		this->attach(* parser, events);
		// Получаем текущий набор ограничений парсера
		parser_http_t::limits_t limits = parser->limits();
		// Устанавливаем требование строгого окончания строк
		limits.strictEOL = probe.strictEOL;
		// Если предельная длина имени заголовка задана
		if(probe.maxHeaderName > 0)
			// Устанавливаем предельную длину имени заголовка
			limits.maxHeaderName = probe.maxHeaderName;
		// Если предельная длина значения заголовка задана
		if(probe.maxHeaderValue > 0)
			// Устанавливаем предельную длину значения заголовка
			limits.maxHeaderValue = probe.maxHeaderValue;
		// Если предельный суммарный размер заголовков задан
		if(probe.maxHeadersTotal > 0)
			// Устанавливаем предельный суммарный размер заголовков
			limits.maxHeadersTotal = probe.maxHeadersTotal;
		// Устанавливаем сформированный набор ограничений
		parser->limits(limits);
		// Формируем сообщение с телом и проверяемым блоком трейлеров
		const std::string message = (
			"HTTP/1.1 200 OK\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5\r\nhello\r\n"
			"0\r\n" + probe.trailers
		);
		// Формируем описание проверяемого отказа для диагностики
		const std::string details = (std::string("отказ: ") + probe.name + (bytewise ? " (по октету)" : " (целиком)"));
		// Если сообщение подаётся по одному октету
		if(bytewise){
			/**
			 * Подаём сообщение по одному октету до фиксации отказа
			 */
			for(size_t i = 0; i < message.size(); i++){
				// Выполняем разбор очередного октета сообщения
				parser->parse(message.data() + i, 1);
				// Если отказ зафиксирован
				if(parser->error() != parser_http_t::error_t::NONE)
					// Прекращаем подачу
					break;
			}
		// Выполняем разбор сформированного сообщения целиком
		} else parser->parse(message.data(), message.size());
		// Проверяем что зафиксирована ожидаемая ошибка разбора
		ASSERT_EQ(parser->error(), probe.error) << details;
		// Проверяем что сообщение не признано полностью разобранным
		ASSERT_NE(parser->status(), parser_t::status_t::COMPLETE) << details;
		/**
		 * Проверяем что тело, принятое до блока трейлеров, не искажено отказом:
		 * непригодный трейлер обязан отменить сообщение, а не переписать доставленное
		 */
		ASSERT_EQ(events.body, "hello") << details;
	}
}

/**
 * @brief Метод проверки значимой семантики разобранного сообщения
 *
 * @details Разобранное сообщение отдаётся вызывающей стороне как значение: его копируют,
 *          перемещают и сравнивают, чтобы отличить одно сообщение от другого. Внутри
 *          лежит провайдер стартовой строки, владение которым исключительное, поэтому
 *          копия обязана создавать собственный провайдер, а не разделять чужой - иначе
 *          два сообщения освободили бы одну и ту же память
 *
 */
TEST_F(ParserFixture, MessageValueSemanticsTest){
	/**
	 * @brief Функция разбора сообщения и получения его значения
	 *
	 * @param fixture набор проверок парсера
	 * @param data    разбираемое сообщение
	 * @return        разобранное сообщение
	 *
	 */
	auto parse = [this](const std::string & data) -> parser_http_t::message_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Выполняем разбор переданного сообщения
		parser->parse(data.data(), data.size());
		// Выводим копию разобранного сообщения
		return parser_http_t::message_t(parser->message());
	};
	// Разбираем сообщение запроса
	const parser_http_t::message_t first = parse("GET /index.html HTTP/1.1\r\nHost: anyks.com\r\n\r\n");
	// Разбираем такое же сообщение запроса
	const parser_http_t::message_t same = parse("GET /index.html HTTP/1.1\r\nHost: anyks.com\r\n\r\n");
	// Разбираем сообщение запроса с другим адресом
	const parser_http_t::message_t other = parse("GET /other.html HTTP/1.1\r\nHost: anyks.com\r\n\r\n");
	// Разбираем сообщение запроса с телом фиксированного размера
	const parser_http_t::message_t sized = parse("POST /index.html HTTP/1.1\r\nHost: anyks.com\r\nContent-Length: 5\r\n\r\nhello");
	// Создаём копию разобранного сообщения
	parser_http_t::message_t copy(first);
	// Проверяем что копия равна оригиналу
	ASSERT_TRUE(copy == first);
	// Проверяем что копия получила собственный провайдер стартовой строки
	ASSERT_NE(copy.provider.get(), first.provider.get());
	// Проверяем что одинаковые сообщения признаны равными
	ASSERT_TRUE(first == same);
	// Проверяем что сообщения с разными адресами признаны различными
	ASSERT_TRUE(first != other);
	// Проверяем что сообщения с разным кадрированием тела признаны различными
	ASSERT_TRUE(first != sized);
	// Выполняем присваивание копированием другого сообщения
	copy = other;
	// Проверяем что присвоенное значение равно источнику
	ASSERT_TRUE(copy == other);
	// Проверяем что присвоенное значение получило собственный провайдер стартовой строки
	ASSERT_NE(copy.provider.get(), other.provider.get());
	// Создаём перемещаемое сообщение
	parser_http_t::message_t source(sized);
	// Запоминаем провайдер перемещаемого сообщения
	const provider_t * origin = source.provider.get();
	// Выполняем перемещение сообщения
	parser_http_t::message_t moved(::std::move(source));
	// Проверяем что провайдер перешёл к перемещённому сообщению без копирования
	ASSERT_EQ(moved.provider.get(), origin);
	// Проверяем что перемещённое сообщение равно исходному значению
	ASSERT_TRUE(moved == sized);
	// Выполняем присваивание перемещением
	copy = ::std::move(moved);
	// Проверяем что присвоенное перемещением значение равно исходному
	ASSERT_TRUE(copy == sized);
}

/**
 * @brief Метод проверки названий кодов ошибок разбора
 *
 * @details Название кода ошибки уходит в журнал и в диагностику вызывающей стороны:
 *          пропущенный в разборе названий код печатался бы как неизвестный, и разбор
 *          отказа по журналу стал бы невозможен
 *
 */
TEST_F(ParserFixture, ErrorNameCoverageTest){
	// Создаём объект парсера-приёмника запроса
	auto parser = this->make(direct_t::REQUEST);
	// Проверяем название кода ошибки текущего состояния парсера
	ASSERT_EQ(parser->errorName(), "NONE");
	// Формируем сообщение с недопустимым символом в имени заголовка
	const std::string message = "GET / HTTP/1.1\r\nHost: anyks.com\r\nX-Bad\x01Name: value\r\n\r\n";
	// Выполняем разбор сформированного сообщения
	parser->parse(message.data(), message.size());
	// Проверяем что название кода ошибки соответствует зафиксированной ошибке
	ASSERT_EQ(parser->errorName(), parser_http_t::errorName(parser->error()));
	// Проверяем что название кода ошибки не пустое
	ASSERT_FALSE(parser->errorName().empty());
	/**
	 * Выполняем перебор всех кодов ошибок разбора
	 */
	for(uint32_t code = 0; code <= 0xFF; code++){
		// Получаем название проверяемого кода ошибки
		const std::string_view name = parser_http_t::errorName(static_cast <parser_http_t::error_t> (code));
		// Проверяем что название кода ошибки не пустое
		ASSERT_FALSE(name.empty()) << ("код ошибки: " + std::to_string(code));
	}
}

/**
 * @brief Метод проверки пределов длины строк на всех состояниях их разбора
 *
 * @details Предел длины проверяется в каждом состоянии разбора строки по отдельности,
 *          и состояний этих больше десятка: метод запроса, request-target, версия
 *          протокола, разделительные пробелы, код и текст состояния ответа, строка размера
 *          чанка. Проверка, пропущенная в одном из них, оставляет ровно один способ
 *          подать строку неограниченной длины, поэтому проверяется каждое состояние
 *
 */
TEST_F(ParserFixture, LineLimitsTest){
	/**
	 * @brief Структура проверяемого превышения предела длины строки
	 *
	 */
	struct probe_t {
		// Название проверяемого превышения
		const char * name;
		// Направление разбираемого трафика
		direct_t direct;
		// Разбираемое сообщение
		std::string message;
		// Ожидаемый код ошибки разбора
		parser_http_t::error_t error;
	};
	// Предел длины стартовой строки для всех проверок
	static constexpr size_t LINE = 32;
	// Формируем перечень проверяемых превышений предела длины строки
	const std::vector <probe_t> probes = {
		// Превышение предела длины методом запроса
		{"длинный метод запроса", direct_t::REQUEST, (std::string(64, 'G') + " / HTTP/1.1\r\n\r\n"), parser_http_t::error_t::URL_OVERFLOW},
		// Превышение предела длины адресом запроса
		{"длинный адрес запроса", direct_t::REQUEST, ("GET /" + std::string(64, 'a') + " HTTP/1.1\r\n\r\n"), parser_http_t::error_t::URL_OVERFLOW},
		// Превышение предела длины разделительными пробелами перед кодом ответа
		{"длинный разделитель ответа", direct_t::RESPONSE, ("HTTP/1.1" + std::string(64, ' ') + "200 OK\r\n\r\n"), parser_http_t::error_t::URL_OVERFLOW},
		// Превышение предела длины текстом состояния ответа
		{"длинный текст состояния", direct_t::RESPONSE, ("HTTP/1.1 200 " + std::string(64, 'O') + "\r\n\r\n"), parser_http_t::error_t::URL_OVERFLOW},
		// Превышение предела длины расширениями строки размера чанка
		{"длинные расширения чанка", direct_t::RESPONSE,
		 ("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5;" + std::string(64, 'a') + "\r\nhello\r\n0\r\n\r\n"),
		 parser_http_t::error_t::CHUNK_OVERFLOW}
	};
	/**
	 * Каждое превышение проверяется двумя способами подачи: крупноблочный сканер и
	 * посимвольный разбор считают длину строки по отдельности
	 */
	for(auto & probe : probes)
	for(const bool bytewise : {false, true}){
		// Создаём объект парсера-приёмника
		auto parser = this->make(probe.direct);
		// Получаем текущий набор ограничений парсера
		parser_http_t::limits_t limits = parser->limits();
		// Устанавливаем предел длины стартовой строки
		limits.maxRequestLine = LINE;
		// Устанавливаем предел длины строки размера чанка
		limits.maxChunkLine = LINE;
		// Устанавливаем сформированный набор ограничений
		parser->limits(limits);
		// Формируем описание проверяемого превышения для диагностики
		const std::string details = (std::string("предел: ") + probe.name + (bytewise ? " (по октету)" : " (целиком)"));
		// Если сообщение подаётся по одному октету
		if(bytewise){
			/**
			 * Подаём сообщение по одному октету до фиксации отказа
			 */
			for(size_t i = 0; i < probe.message.size(); i++){
				// Выполняем разбор очередного октета сообщения
				parser->parse(probe.message.data() + i, 1);
				// Если отказ зафиксирован
				if(parser->error() != parser_http_t::error_t::NONE)
					// Прекращаем подачу
					break;
			}
		// Выполняем разбор сформированного сообщения целиком
		} else parser->parse(probe.message.data(), probe.message.size());
		// Проверяем что зафиксирована ожидаемая ошибка разбора
		ASSERT_EQ(parser->error(), probe.error) << details;
		// Проверяем что сообщение не признано полностью разобранным
		ASSERT_NE(parser->status(), parser_t::status_t::COMPLETE) << details;
	}
}

/**
 * @brief Метод проверки прерывания разбора потребителем внутри блока трейлеров
 *
 * @details Потребитель вправе прервать разбор из любой функции обратного вызова, и блок
 *          трейлеров - последнее место, где это ещё возможно: тело уже доставлено, а
 *          сообщение ещё не завершено. Отказ обязан фиксироваться как прерывание разбора,
 *          а не как успешное завершение сообщения
 *
 */
TEST_F(ParserFixture, TrailerAbortedByConsumerTest){
	// Создаём объект парсера-приёмника ответа
	auto parser = this->make(direct_t::RESPONSE);
	// Устанавливаем функцию обратного вызова получения заголовка, прерывающую разбор на трейлере
	parser->on(static_cast <parser_http_t::header_callback_t> (
		[](const uint32_t, const std::string_view, const std::string_view, const parser_http_t::part_t part) -> bool {
			// Прерываем разбор при получении поля блока трейлеров
			return (part != parser_http_t::part_t::TRAILER);
		}
	));
	// Формируем сообщение с телом и блоком трейлеров
	const std::string message = (
		"HTTP/1.1 200 OK\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\nhello\r\n"
		"0\r\nX-Check: done\r\n\r\n"
	);
	// Выполняем разбор сформированного сообщения
	parser->parse(message.data(), message.size());
	// Проверяем что разбор прерван потребителем
	ASSERT_EQ(parser->error(), parser_http_t::error_t::ABORTED);
	// Проверяем что сообщение не признано полностью разобранным
	ASSERT_NE(parser->status(), parser_t::status_t::COMPLETE);
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
 *          состоят из token и token либо quoted-string. DELETE не входит ни в token,
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
	// Проверяем что DELETE в расширении чанка отвергается
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
 * @brief Метод тестирования кадрирования тела при транспортном кодировании без финального chunked
 *
 * @details RFC 9112 §6.3 п.4 разводит два направления: у ответа сервера с заголовком
 *          Transfer-Encoding, где chunked не является последним кодированием, длина
 *          тела определяется чтением до закрытия соединения, а у запроса клиента такое
 *          сообщение надёжно кадрировать нечем и получатель обязан отвергнуть его.
 *          Пустое значение заголовка - частный случай того же правила: кодирований в
 *          нём нет, а значит chunked последним не является. Ужесточать поведение ответа
 *          до ошибки нельзя - это прямое расхождение с нормативным текстом
 *
 */
TEST_F(ParserFixture, TransferEncodingWithoutFinalChunkedTest){
	/**
	 * @brief Функция разбора сообщения с заданным значением заголовка Transfer-Encoding
	 *
	 * @param direct направление трафика
	 * @param value  значение заголовка транспортного кодирования
	 * @param body   собранное тело сообщения
	 * @return       код ошибки разбора
	 *
	 */
	auto probe = [this](const direct_t direct, const std::string & value, std::string & body) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера заданного направления
		auto parser = this->make(direct);
		// Если выполняется разбор ответа сервера
		if(direct == direct_t::RESPONSE)
			// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
			parser->method(method_t::GET);
		// Подписываем сборщик тела сообщения
		parser->on(parser_http_t::data_callback_t([&body](const int32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Накапливаем полученную порцию тела сообщения
			body.append(static_cast <const char *> (buffer), size);
			// Продолжаем разбор
			return true;
		}));
		// Формируем разбираемое сообщение
		const std::string message = ((direct == direct_t::RESPONSE)
		 ? ("HTTP/1.1 200 OK\r\nTransfer-Encoding:" + value + "\r\n\r\nAWH")
		 : ("POST / HTTP/1.1\r\nHost: anyks.com\r\nTransfer-Encoding:" + value + "\r\n\r\nAWH"));
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Если выполняется разбор ответа сервера - сообщаем о закрытии соединения
		if(direct == direct_t::RESPONSE)
			// Уведомляем парсер о закрытии соединения
			parser->eof();
		// Выводим код ошибки разбора
		return parser->error();
	};
	/**
	 * Ответ сервера с кодированием без финального chunked кадрируется закрытием соединения
	 */
	{
		// Собранное тело сообщения
		std::string body;
		// Проверяем что ответ принимается
		ASSERT_EQ(probe(direct_t::RESPONSE, " gzip", body), parser_http_t::error_t::NONE);
		// Проверяем что тело прочитано до закрытия соединения
		ASSERT_EQ(body, "AWH");
	}
	/**
	 * Пустое значение заголовка у ответа - тот же случай отсутствия финального chunked
	 */
	{
		// Собранное тело сообщения
		std::string body;
		// Проверяем что ответ принимается
		ASSERT_EQ(probe(direct_t::RESPONSE, "", body), parser_http_t::error_t::NONE);
		// Проверяем что тело прочитано до закрытия соединения
		ASSERT_EQ(body, "AWH");
	}
	/**
	 * Запрос клиента с тем же кодированием надёжно кадрировать нечем
	 */
	{
		// Собранное тело сообщения
		std::string body;
		// Проверяем что запрос отвергается
		ASSERT_EQ(probe(direct_t::REQUEST, " gzip", body), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
		// Проверяем что тело отвергнутого запроса не собиралось
		ASSERT_TRUE(body.empty());
	}
	/**
	 * Пустое значение заголовка у запроса отвергается точно так же
	 */
	{
		// Собранное тело сообщения
		std::string body;
		// Проверяем что запрос отвергается
		ASSERT_EQ(probe(direct_t::REQUEST, "", body), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
		// Проверяем что тело отвергнутого запроса не собиралось
		ASSERT_TRUE(body.empty());
	}
}

/**
 * @brief Метод тестирования кодов состояния вне диапазона 100..599
 *
 * @details Стартовая строка ответа определяет код состояния как три десятичные цифры
 *          (RFC 9112 §4), а RFC 9110 §15 объявляет коды вне диапазона 100..599
 *          недопустимыми и предписывает обрабатывать такой ответ так, как если бы код
 *          принадлежал классу 5xx. Отвергать сообщение парсер не вправе: предписание
 *          адресовано получателю уже принятого ответа, и тело у класса 5xx кадрируется
 *          по обычным правилам
 *
 */
TEST_F(ParserFixture, StatusCodeOutOfRangeTest){
	/**
	 * @brief Функция разбора ответа с заданным кодом состояния
	 *
	 * @param code код состояния ответа сервера
	 * @param body собранное тело сообщения
	 * @return     разобранный код состояния
	 *
	 */
	auto probe = [this](const std::string & code, std::string & body) noexcept -> uint16_t {
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Подписываем сборщик тела сообщения
		parser->on(parser_http_t::data_callback_t([&body](const int32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Накапливаем полученную порцию тела сообщения
			body.append(static_cast <const char *> (buffer), size);
			// Продолжаем разбор
			return true;
		}));
		// Формируем разбираемое сообщение
		const std::string message = ("HTTP/1.1 " + code + " Weird\r\nContent-Length: 3\r\n\r\nAWH");
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Проверяем что сообщение полностью разобрано
		EXPECT_EQ(parser->status(), parser_t::status_t::COMPLETE) << code;
		// Проверяем что ошибок разбора нет
		EXPECT_EQ(parser->error(), parser_http_t::error_t::NONE) << code;
		// Выводим разобранный код состояния
		return static_cast <const response_t *> (parser->message().provider.get())->code;
	};
	/**
	 * Выполняем перебор кодов состояния вне допустимого диапазона
	 */
	for(const std::string & code : {std::string("000"), std::string("099"), std::string("600"), std::string("999")}){
		// Собранное тело сообщения
		std::string body;
		// Проверяем что код состояния разобран как есть
		ASSERT_EQ(probe(code, body), static_cast <uint16_t> (std::stoi(code))) << code;
		// Проверяем что тело кадрировано по обычным правилам
		ASSERT_EQ(body, "AWH") << code;
	}
}

/**
 * @brief Метод тестирования учёта отброшенных OWS в бюджете блока заголовков
 *
 * @details Ведущие OWS значения заголовка при разборе отбрасываются, но канал и
 *          процессорное время занимают наравне с сохранёнными октетами. Без их учёта
 *          одна строка вида "X:" с потоком пробелов растягивалась бы неограниченно,
 *          не приближая разбор к завершению и не расходуя ни одного лимита. Оба пути
 *          разбора обязаны считать их одинаково
 *
 */
TEST_P(FragmentParameterizedFixture, HeaderValueWhitespaceBudgetTest){
	/**
	 * @brief Функция разбора сообщения с заданным числом ведущих OWS значения заголовка
	 *
	 * @param spaces число октетов ведущих OWS значения заголовка
	 * @return       код ошибки разбора
	 *
	 */
	auto probe = [this](const size_t spaces) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Формируем пониженные лимиты безопасности
		parser_http_t::limits_t limits;
		// Понижаем суммарный размер блока заголовков
		limits.maxHeadersTotal = 128;
		// Устанавливаем пониженные лимиты безопасности
		parser->limits(limits);
		// Формируем разбираемое сообщение
		const std::string message = ("GET / HTTP/1.1\r\nHost: a\r\nX-Pad:" + std::string(spaces, ' ') + "v\r\n\r\n");
		// Выполняем подачу данных фрагментами заданного размера
		for(size_t i = 0; i < message.size(); i += this->_fragment){
			// Выполняем разбор очередного фрагмента данных
			parser->parse(message.data() + i, ((message.size() - i) < this->_fragment ? (message.size() - i) : this->_fragment));
			// Если разбор завершён - дальнейшая подача не нужна
			if(parser->status() != parser_t::status_t::PARTIAL)
				// Прекращаем подачу данных
				break;
		}
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Проверяем что умеренное число OWS укладывается в бюджет
	ASSERT_EQ(probe(16), parser_http_t::error_t::NONE);
	// Проверяем что поток OWS упирается в бюджет блока заголовков
	ASSERT_EQ(probe(4096), parser_http_t::error_t::HEADER_OVERFLOW);
}

/**
 * @brief Метод тестирования BWS между размером чанка и его расширениями
 *
 * @details По RFC 9112 §7.1 расширения чанка имеют вид *( BWS ";" BWS ... ), поэтому
 *          запись "3 ;a=b" грамматически допустима. BWS входит в грамматику только
 *          вместе с точкой с запятой, поэтому запись "3 " без расширений остаётся
 *          ошибкой: звено цепочки, читающее размер чанка до пробела, а не до конца
 *          строки, увидело бы иное кадрирование
 *
 */
TEST_F(ParserFixture, ChunkSizeWhitespaceTest){
	/**
	 * @brief Функция разбора ответа с заданной строкой размера чанка
	 *
	 * @param line строка размера чанка вместе с её окончанием
	 * @param body собранное тело сообщения
	 * @return     код ошибки разбора
	 *
	 */
	auto probe = [this](const std::string & line, std::string & body) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Подписываем сборщик тела сообщения
		parser->on(parser_http_t::data_callback_t([&body](const int32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Накапливаем полученную порцию тела сообщения
			body.append(static_cast <const char *> (buffer), size);
			// Продолжаем разбор
			return true;
		}));
		// Формируем разбираемое сообщение
		const std::string message = ("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + line + "AWH\r\n0\r\n\r\n");
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	/**
	 * @brief Функция проверки принимаемой строки размера чанка
	 *
	 * @param line строка размера чанка вместе с её окончанием
	 *
	 */
	auto accepted = [&probe](const std::string & line) noexcept -> void {
		// Собранное тело сообщения
		std::string body;
		// Проверяем что строка размера чанка принимается
		ASSERT_EQ(probe(line, body), parser_http_t::error_t::NONE) << line;
		// Проверяем что тело чанка разобрано
		ASSERT_EQ(body, "AWH") << line;
	};
	/**
	 * @brief Функция проверки отвергаемой строки размера чанка
	 *
	 * @param line строка размера чанка вместе с её окончанием
	 *
	 */
	auto rejected = [&probe](const std::string & line) noexcept -> void {
		// Собранное тело сообщения
		std::string body;
		// Проверяем что строка размера чанка отвергается
		ASSERT_EQ(probe(line, body), parser_http_t::error_t::INVALID_CHUNK_SIZE) << line;
	};
	// Проверяем что расширение без BWS принимается
	accepted("3;a=b\r\n");
	// Проверяем что пробел перед точкой с запятой принимается
	accepted("3 ;a=b\r\n");
	// Проверяем что табуляция перед точкой с запятой принимается
	accepted("3\t;a=b\r\n");
	// Проверяем что несколько октетов BWS принимаются
	accepted("3   ;a=b\r\n");
	// Проверяем что строка размера чанка без расширений принимается
	accepted("3\r\n");
	// Проверяем что пробел без расширения отвергается
	rejected("3 \r\n");
	// Проверяем что посторонний символ после пробела отвергается
	rejected("3 x\r\n");
}

/**
 * @brief Метод тестирования требований к заголовку Host
 *
 * @details RFC 9112 §3.2 требует отвергать три случая, но податливость к ним разная.
 *          Отсутствие заголовка адресовано только запросу HTTP/1.1 и встречается у
 *          простого инструментария, поэтому оставлено под переключателем строгости.
 *          Дублирование и недопустимое значение адресованы любому запросу и
 *          податливости не заслуживают: законного отправителя, выдающего два
 *          противоречащих Host, не существует, а выбор звеньями цепочки разных из
 *          них - тот же механизм рассинхронизации, что и конфликт кадрирования
 *
 */
TEST_F(ParserFixture, HostRequirementsTest){
	/**
	 * @brief Функция разбора запроса с заданным набором строк заголовка Host
	 *
	 * @param lines  строки заголовков запроса
	 * @param strict признак включения строгого требования заголовка Host
	 * @return       код ошибки разбора
	 *
	 */
	auto probe = [this](const std::string & lines, const bool strict) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Если включается строгое требование заголовка Host
		if(strict){
			// Формируем лимиты безопасности
			parser_http_t::limits_t limits;
			// Включаем строгое требование заголовка Host
			limits.requireHost = true;
			// Устанавливаем лимиты безопасности
			parser->limits(limits);
		}
		// Формируем разбираемое сообщение
		const std::string message = ("GET / HTTP/1.1\r\n" + lines + "\r\n");
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Проверяем что единственный корректный заголовок Host принимается
	ASSERT_EQ(probe("Host: anyks.com\r\n", false), parser_http_t::error_t::NONE);
	// Проверяем что заголовок Host с портом принимается
	ASSERT_EQ(probe("Host: anyks.com:8080\r\n", false), parser_http_t::error_t::NONE);
	// Проверяем что пустое значение заголовка Host принимается (absolute-form у OPTIONS)
	ASSERT_EQ(probe("Host:\r\n", false), parser_http_t::error_t::NONE);
	// Проверяем что два заголовка Host отвергаются независимо от строгости
	ASSERT_EQ(probe("Host: anyks.com\r\nHost: evil.com\r\n", false), parser_http_t::error_t::INVALID_HOST);
	// Проверяем что пробел внутри значения заголовка Host отвергается
	ASSERT_EQ(probe("Host: anyks.com evil.com\r\n", false), parser_http_t::error_t::INVALID_HOST);
	// Проверяем что табуляция внутри значения заголовка Host отвергается
	ASSERT_EQ(probe("Host: anyks.com\tevil.com\r\n", false), parser_http_t::error_t::INVALID_HOST);
	// Проверяем что отсутствие заголовка Host в толерантном режиме принимается
	ASSERT_EQ(probe("", false), parser_http_t::error_t::NONE);
	// Проверяем что отсутствие заголовка Host в строгом режиме отвергается
	ASSERT_EQ(probe("", true), parser_http_t::error_t::INVALID_HOST);
	/**
	 * Проверяем что негодный заголовок Host не доходит до функции обратного вызова
	 *
	 * Обработка поля прерывается до уведомления - тем же порядком, что и у негодного
	 * Content-Length. Иначе вызывающая сторона получала бы заведомо битый заголовок и
	 * узнавала о его негодности лишь по итогу разбора: прокси успел бы передать его
	 * дальше по цепочке
	 */
	{
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Число доставленных функции обратного вызова полей
		size_t delivered = 0;
		// Устанавливаем функцию обратного вызова обработки заголовков сообщения
		parser->on(parser_http_t::header_callback_t([&delivered](const int32_t, const std::string_view, const std::string_view, const parser_t::part_t) noexcept -> bool {
			// Считаем доставленное поле
			++delivered;
			// Продолжаем разбор
			return true;
		}));
		// Формируем запрос с недопустимым значением заголовка Host
		const std::string message = "GET / HTTP/1.1\r\nHost: anyks.com evil.com\r\n\r\n";
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Проверяем что разбор остановлен по недопустимому значению заголовка Host
		ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_HOST);
		// Проверяем что негодный заголовок наружу не отдан
		ASSERT_EQ(delivered, 0u);
	}
}

/**
 * @brief Метод тестирования ужесточённого кадрирования в режиме прокси
 *
 * @details Ответы с кодом 1xx и [204 No Content] заканчиваются первой пустой строкой
 *          после блока заголовков независимо от присутствующих в нём полей
 *          (RFC 9112 §6.3 п.1), а отправлять в них Content-Length и Transfer-Encoding
 *          запрещено (§6.1, §6.2). При прямом соединении объявление просто
 *          игнорируется, но узел, передающий сообщение дальше по цепочке, обязан
 *          отвергнуть то, что следующее звено может истолковать иначе: звено,
 *          уважившее объявленный размер, прочитает следующий ответ как тело этого.
 *          По той же причине отвергается кодирование, не заканчивающееся токеном
 *          chunked: такое тело ограничено только закрытием соединения и само себя
 *          не размечает. Ответу [304 Not Modified] и ответу на HEAD объявления
 *          разрешены - они описывают тело, которое ушло бы в ответ на такой же GET
 *
 */
TEST_F(ParserFixture, ProxyFramingStrictnessTest){
	/**
	 * @brief Функция разбора ответа сервера с заданным протоколом работы
	 *
	 * @param message разбираемое сообщение
	 * @param proto   протокол работы парсера
	 * @return        код ошибки разбора
	 *
	 */
	auto probe = [this](const std::string & message, const proto_t proto) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем протокол работы парсера
		parser->proto(proto);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Формируем ответ [204 No Content] с объявленным размером тела
	const std::string lengthy = "HTTP/1.1 204 No Content\r\nContent-Length: 5\r\n\r\nhello";
	// Формируем ответ [204 No Content] с объявленным кодированием тела
	const std::string chunked = "HTTP/1.1 204 No Content\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n";
	// Формируем информационный ответ с объявленным размером тела
	const std::string interim = "HTTP/1.1 100 Continue\r\nContent-Length: 5\r\n\r\nhello";
	// Формируем ответ с кодированием, не заканчивающимся токеном chunked
	const std::string unframed = "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\nbody";
	/**
	 * Проверяем что при прямом соединении объявления игнорируются, а не отвергаются
	 *
	 * Это буквальное следование RFC 9112 §6.3, и ужесточать его вне цепочки узлов
	 * означало бы ломать законный трафик
	 */
	ASSERT_EQ(probe(lengthy, proto_t::HTTP1), parser_http_t::error_t::NONE);
	// Проверяем что объявленное кодирование при прямом соединении также игнорируется
	ASSERT_EQ(probe(chunked, proto_t::HTTP1), parser_http_t::error_t::NONE);
	// Проверяем что информационный ответ при прямом соединении принимается
	ASSERT_EQ(probe(interim, proto_t::HTTP1), parser_http_t::error_t::NONE);
	// Проверяем что кодирование без завершающего chunked при прямом соединении принимается
	ASSERT_EQ(probe(unframed, proto_t::HTTP1), parser_http_t::error_t::NONE);
	// Проверяем что в режиме прокси объявленный размер тела у ответа 204 отвергается
	ASSERT_EQ(probe(lengthy, proto_t::PROXY1), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем что в режиме прокси объявленное кодирование у ответа 204 отвергается
	ASSERT_EQ(probe(chunked, proto_t::PROXY1), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
	// Проверяем что в режиме прокси объявленный размер тела у информационного ответа отвергается
	ASSERT_EQ(probe(interim, proto_t::PROXY1), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем что в режиме прокси кодирование без завершающего chunked отвергается
	ASSERT_EQ(probe(unframed, proto_t::PROXY1), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
	/**
	 * Проверяем что ответу [304 Not Modified] объявление размера тела разрешено
	 *
	 * Заголовок описывает тело, которое было бы отправлено в ответ на такой же
	 * запрос GET, и отвергать его недопустимо даже на границе сети
	 */
	ASSERT_EQ(probe("HTTP/1.1 304 Not Modified\r\nContent-Length: 5\r\n\r\n", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что обычный ответ с размером тела в режиме прокси принимается
	ASSERT_EQ(probe("HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что обычный ответ с кадрированием chunked в режиме прокси принимается
	ASSERT_EQ(probe("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n", proto_t::PROXY1), parser_http_t::error_t::NONE);
	/**
	 * Проверяем что клон наследует протокол работы
	 *
	 * Протокол описывает соединение и настраивается наравне с лимитами: фабрика,
	 * настроенная режимом прокси, обязана выпускать парсеры в том же режиме. Иначе
	 * все проверки кадрирования, которые режим включает, у клонов молча выключены -
	 * а именно клоны и обслуживают соединения
	 */
	{
		// Создаём объект парсера-приёмника ответа, играющий роль фабрики
		auto factory = this->make(direct_t::RESPONSE);
		// Устанавливаем протокол работы через прокси-сервер
		factory->proto(proto_t::PROXY1);
		// Создаём клон настроенной фабрики
		const auto clone = factory->clone();
		// Приводим клон к типу парсера HTTP/1.x
		auto parser = static_cast <parser_http_t *> (clone.get());
		// Проверяем что клон унаследовал протокол работы
		ASSERT_EQ(parser->proto(), proto_t::PROXY1);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Формируем ответ, объявленное кадрирование которого в режиме прокси недопустимо
		const std::string message = "HTTP/1.1 204 No Content\r\nContent-Length: 5\r\n\r\nhello";
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Проверяем что клон применяет ужесточение режима, а не только помнит о нём
		ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	}
	/**
	 * Проверяем что клон наследует и режим переключения протокола
	 */
	{
		// Создаём объект парсера-приёмника запроса, играющий роль фабрики
		auto factory = this->make(direct_t::REQUEST);
		// Устанавливаем протокол работы с переключением на WebSocket
		factory->proto(proto_t::WEBSOCKET1);
		// Создаём клон настроенной фабрики
		const auto clone = factory->clone();
		// Приводим клон к типу парсера HTTP/1.x
		auto parser = static_cast <parser_http_t *> (clone.get());
		// Проверяем что клон унаследовал протокол работы
		ASSERT_EQ(parser->proto(), proto_t::WEBSOCKET1);
		// Формируем рукопожатие с объявленным телом
		const std::string message =
			"GET /chat HTTP/1.1\r\nHost: anyks.com\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
			"Content-Length: 5\r\n\r\nhello";
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Проверяем что клон применяет ужесточение режима
		ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	}
	/**
	 * Проверяем что полная очистка возвращает протокол к прямому соединению
	 *
	 * Очистка готовит объект к повторному использованию и уже возвращает к умолчанию
	 * лимиты: оставить режим означало бы отдать объект под новое соединение с
	 * настройками предыдущего
	 */
	{
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем протокол работы через прокси-сервер
		parser->proto(proto_t::PROXY1);
		// Выполняем полную очистку объекта парсера
		parser->clear();
		// Проверяем что протокол работы возвращён к прямому соединению
		ASSERT_EQ(parser->proto(), proto_t::HTTP1);
	}
	/**
	 * Проверяем что протокол чужого семейства не принимается
	 *
	 * Разбирать HTTP/2 этот парсер не умеет, и молчаливое принятие такого указания
	 * создало бы у вызывающей стороны ложное представление о происходящем
	 */
	{
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Проверяем что по умолчанию установлено прямое соединение
		ASSERT_EQ(parser->proto(), proto_t::HTTP1);
		// Пытаемся установить протокол чужого семейства
		parser->proto(proto_t::HTTP2);
		// Проверяем что протокол работы парсера не изменился
		ASSERT_EQ(parser->proto(), proto_t::HTTP1);
		// Устанавливаем протокол работы через прокси-сервер
		parser->proto(proto_t::PROXY1);
		// Проверяем что протокол работы парсера установлен
		ASSERT_EQ(parser->proto(), proto_t::PROXY1);
	}
}

/**
 * @brief Метод тестирования сверки адресата цели запроса с заголовком Host
 *
 * @details Цель в absolute-form несёт адресата сама, и тогда в запросе их оказывается
 *          два. Получателю предписано брать адресата из цели и заголовок игнорировать
 *          (RFC 9112 §3.2.2), а клиенту - присылать заголовок, совпадающий с адресатом
 *          цели (RFC 9110 §7.2). Соблюдают это не все звенья цепочки: одно
 *          маршрутизирует по цели, другое смотрит на заголовок - и они расходятся в
 *          том, кому адресован один и тот же запрос. Проверка выполняется только в
 *          режиме прокси: конечному получателю расходиться не с кем.
 *          Адресат "anyks.com" и адресат "anyks.com:80" при схеме http обозначают один
 *          узел, поэтому порт сверяется с подстановкой стандартного для схемы
 *
 */
TEST_F(ParserFixture, ProxyTargetHostTest){
	/**
	 * @brief Функция разбора запроса с заданной целью и заголовком Host
	 *
	 * @param target цель запроса
	 * @param host   значение заголовка Host
	 * @param proto  протокол работы парсера
	 * @return       код ошибки разбора
	 *
	 */
	auto probe = [this](const std::string & target, const std::string & host, const proto_t proto) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Устанавливаем протокол работы парсера
		parser->proto(proto);
		// Формируем разбираемое сообщение
		const std::string message = ("GET " + target + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n");
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Проверяем что совпадающий адресат принимается
	ASSERT_EQ(probe("http://anyks.com/x", "anyks.com", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что явно указанный стандартный порт схемы расхождением не считается
	ASSERT_EQ(probe("http://anyks.com/x", "anyks.com:80", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что стандартный порт схемы в цели расхождением не считается
	ASSERT_EQ(probe("http://anyks.com:80/x", "anyks.com", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что стандартный порт защищённой схемы также подставляется
	ASSERT_EQ(probe("https://anyks.com/x", "anyks.com:443", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что регистр имени узла на сверку не влияет
	ASSERT_EQ(probe("http://ANYKS.COM/x", "anyks.com", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что совпадающий нестандартный порт принимается
	ASSERT_EQ(probe("http://anyks.com:8080/x", "anyks.com:8080", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что параметры пользователя в цели на сверку не влияют
	ASSERT_EQ(probe("http://user:pass@anyks.com/x", "anyks.com", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что литерал IPv6 разбирается вместе с портом
	ASSERT_EQ(probe("http://[::1]:8080/x", "[::1]:8080", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что литерал IPv6 без порта также принимается
	ASSERT_EQ(probe("http://[::1]/x", "[::1]", proto_t::PROXY1), parser_http_t::error_t::NONE);
	/**
	 * Проверяем что цель без адресата сверку проходит
	 *
	 * Ни origin-form, ни asterisk-form адресата не содержат, и сравнивать не с чем
	 */
	ASSERT_EQ(probe("/x", "anyks.com", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что цель вида asterisk-form сверку также проходит
	ASSERT_EQ(probe("*", "anyks.com", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что подмена имени узла отвергается
	ASSERT_EQ(probe("http://anyks.com/x", "evil.com", proto_t::PROXY1), parser_http_t::error_t::INVALID_HOST);
	/**
	 * Проверяем что подмена имени узла той же длины отвергается
	 *
	 * Случай отделён намеренно: подмену другой длины отсекает сравнение длин, и без
	 * равной длины посимвольное сравнение имён оставалось бы непроверенным
	 */
	ASSERT_EQ(probe("http://anyks.com/x", "anyks.net", proto_t::PROXY1), parser_http_t::error_t::INVALID_HOST);
	// Проверяем что подмена имени узла регистронезависимо отличающимся именем отвергается
	ASSERT_EQ(probe("http://anyks.com/x", "ANYKS.NET", proto_t::PROXY1), parser_http_t::error_t::INVALID_HOST);
	// Проверяем что подмена имени узла поддоменом отвергается
	ASSERT_EQ(probe("http://anyks.com/x", "anyks.com.evil.com", proto_t::PROXY1), parser_http_t::error_t::INVALID_HOST);
	// Проверяем что расхождение по порту отвергается
	ASSERT_EQ(probe("http://anyks.com/x", "anyks.com:8080", proto_t::PROXY1), parser_http_t::error_t::INVALID_HOST);
	// Проверяем что расхождение по порту отвергается и в обратную сторону
	ASSERT_EQ(probe("http://anyks.com:8080/x", "anyks.com", proto_t::PROXY1), parser_http_t::error_t::INVALID_HOST);
	/**
	 * Проверяем что при прямом соединении сверка не выполняется
	 *
	 * Получателю предписано брать адресата из цели и заголовок игнорировать, и
	 * отвергать по расхождению вне цепочки узлов означало бы ужесточать RFC там,
	 * где расходиться не с кем
	 */
	ASSERT_EQ(probe("http://anyks.com/x", "evil.com", proto_t::HTTP1), parser_http_t::error_t::NONE);
	// Проверяем что расхождение по порту при прямом соединении также принимается
	ASSERT_EQ(probe("http://anyks.com:8080/x", "anyks.com", proto_t::HTTP1), parser_http_t::error_t::NONE);
}

/**
 * @brief Метод тестирования кадрирования при переключении протокола
 *
 * @details Запрос с заголовком Upgrade получает в ответ [101 Switching Protocols], за
 *          пустой строкой которого начинается поток нового протокола. Объявленное этим
 *          же запросом тело делает точку переключения предметом догадки: звено,
 *          уважившее объявленный размер, съест байты как тело, а звено, признавшее
 *          переключение состоявшимся, прочитает их как начало нового протокола -
 *          и атакующая сторона выберет первый кадр чужого соединения. Рукопожатие
 *          WebSocket тела не определяет вовсе (RFC 6455 §4.1).
 *          Прямому соединению это не опасно: оно переключаться не собирается, читает
 *          обычный запрос с телом, а заголовок Upgrade игнорирует
 *
 */
TEST_F(ParserFixture, UpgradeFramingTest){
	/**
	 * @brief Функция разбора запроса рукопожатия с заданным кадрированием тела
	 *
	 * @param framing строки заголовков кадрирования и тело сообщения
	 * @param proto   протокол работы парсера
	 * @return        код ошибки разбора
	 *
	 */
	auto handshake = [this](const std::string & framing, const proto_t proto) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Устанавливаем протокол работы парсера
		parser->proto(proto);
		// Формируем разбираемое рукопожатие клиента
		const std::string message = (
			"GET /chat HTTP/1.1\r\nHost: anyks.com\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
			"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n" + framing
		);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	/**
	 * @brief Функция разбора ответа переключения протокола с заданным кадрированием
	 *
	 * @param framing строка заголовка кадрирования
	 * @param proto   протокол работы парсера
	 * @return        код ошибки разбора
	 *
	 */
	auto accept = [this](const std::string & framing, const proto_t proto) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Устанавливаем протокол работы парсера
		parser->proto(proto);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Формируем разбираемый ответ переключения протокола
		const std::string message = (
			"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n" + framing + "\r\n"
		);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Проверяем что чистое рукопожатие принимается во всех режимах
	ASSERT_EQ(handshake("\r\n", proto_t::HTTP1), parser_http_t::error_t::NONE);
	// Проверяем что чистое рукопожатие принимается при переключении протокола
	ASSERT_EQ(handshake("\r\n", proto_t::WEBSOCKET1), parser_http_t::error_t::NONE);
	// Проверяем что чистое рукопожатие принимается через прокси
	ASSERT_EQ(handshake("\r\n", proto_t::PROXY1), parser_http_t::error_t::NONE);
	// Проверяем что нулевой размер тела рукопожатию не мешает
	ASSERT_EQ(handshake("Content-Length: 0\r\n\r\n", proto_t::WEBSOCKET1), parser_http_t::error_t::NONE);
	// Проверяем что объявленный размер тела рукопожатия отвергается при переключении протокола
	ASSERT_EQ(handshake("Content-Length: 5\r\n\r\nhello", proto_t::WEBSOCKET1), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем что объявленное кодирование тела рукопожатия отвергается при переключении протокола
	ASSERT_EQ(handshake("Transfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n", proto_t::WEBSOCKET1), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
	// Проверяем что объявленный размер тела рукопожатия отвергается и через прокси
	ASSERT_EQ(handshake("Content-Length: 5\r\n\r\nhello", proto_t::PROXY1), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	/**
	 * Проверяем что при прямом соединении тело рукопожатия принимается
	 *
	 * Переключаться такое соединение не собирается: заголовок Upgrade оно игнорирует,
	 * а запрос читает как обычный запрос с телом
	 */
	ASSERT_EQ(handshake("Content-Length: 5\r\n\r\nhello", proto_t::HTTP1), parser_http_t::error_t::NONE);
	// Проверяем что чистый ответ переключения протокола принимается
	ASSERT_EQ(accept("", proto_t::WEBSOCKET1), parser_http_t::error_t::NONE);
	// Проверяем что объявленный размер тела в ответе переключения отвергается
	ASSERT_EQ(accept("Content-Length: 5\r\n", proto_t::WEBSOCKET1), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем что объявленное кодирование тела в ответе переключения отвергается
	ASSERT_EQ(accept("Transfer-Encoding: chunked\r\n", proto_t::WEBSOCKET1), parser_http_t::error_t::INVALID_TRANSFER_ENCODING);
	// Проверяем что при прямом соединении объявление в ответе переключения игнорируется
	ASSERT_EQ(accept("Content-Length: 5\r\n", proto_t::HTTP1), parser_http_t::error_t::NONE);
	/**
	 * Проверяем что собрать рукопожатие с телом также нельзя
	 *
	 * Собранное сообщение обязано быть принято собственным приёмником, а он такой
	 * запрос отвергает
	 */
	{
		// Создаём объект парсера-отправителя запроса
		auto sender = this->make(direct_t::REQUEST);
		// Устанавливаем протокол работы парсера
		sender->proto(proto_t::WEBSOCKET1);
		// Формируем контейнер заголовков запроса с провайдером
		headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/chat")));
		// Дописываем обязательный для запроса HTTP/1.1 заголовок Host
		request.emplace("Host", "anyks.com");
		// Дописываем заголовок запроса переключения протокола
		request.emplace("Upgrade", "websocket");
		// Объявляем размер тела запроса
		request.emplace("Content-Length", "5");
		// Отправляем заголовки запроса (тело последует)
		sender->sendHeaders(request, false);
		// Проверяем что рукопожатие с телом не собирается
		ASSERT_TRUE(sender->pending().empty());
	}
	/**
	 * Проверяем что рукопожатие без тела собирается
	 */
	{
		// Создаём объект парсера-отправителя запроса
		auto sender = this->make(direct_t::REQUEST);
		// Устанавливаем протокол работы парсера
		sender->proto(proto_t::WEBSOCKET1);
		// Формируем контейнер заголовков запроса с провайдером
		headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/chat")));
		// Дописываем обязательный для запроса HTTP/1.1 заголовок Host
		request.emplace("Host", "anyks.com");
		// Дописываем заголовок запроса переключения протокола
		request.emplace("Upgrade", "websocket");
		// Отправляем заголовки запроса с завершением сообщения
		sender->sendHeaders(request, true);
		// Проверяем что рукопожатие собрано
		ASSERT_NE(std::string(sender->pending()).find("Upgrade: websocket"), std::string::npos);
	}
	/**
	 * Проверяем что рукопожатие с объявленным chunked не собирается и при пустом теле
	 *
	 * Объявленное вызывающей стороной кодирование уходит на провод и без тела -
	 * завершённым нулевым чанком, а собственного кадрирования отправитель при этом
	 * не выбирает вовсе. Проверять выбранный способ кадрирования здесь недостаточно:
	 * приёмник смотрит на объявленные заголовки и такой запрос отвергает
	 */
	{
		// Создаём объект парсера-отправителя запроса
		auto sender = this->make(direct_t::REQUEST);
		// Устанавливаем протокол работы парсера
		sender->proto(proto_t::PROXY1);
		// Формируем контейнер заголовков запроса с провайдером
		headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/chat")));
		// Дописываем обязательный для запроса HTTP/1.1 заголовок Host
		request.emplace("Host", "anyks.com");
		// Дописываем заголовок запроса переключения протокола
		request.emplace("Upgrade", "websocket");
		// Объявляем кодирование тела запроса
		request.emplace("Transfer-Encoding", "chunked");
		// Отправляем заголовки запроса с завершением сообщения
		sender->sendHeaders(request, true);
		// Проверяем что рукопожатие с объявленным кодированием не собирается
		ASSERT_TRUE(sender->pending().empty());
	}
	/**
	 * Проверяем что объявленный нулевой размер тела рукопожатию не мешает
	 *
	 * Тела он не описывает, и собственный приёмник такой запрос принимает: отказ
	 * в сборке был бы строже приёма, то есть отбрасывал бы законное рукопожатие
	 */
	{
		// Создаём объект парсера-отправителя запроса
		auto sender = this->make(direct_t::REQUEST);
		// Устанавливаем протокол работы парсера
		sender->proto(proto_t::PROXY1);
		// Формируем контейнер заголовков запроса с провайдером
		headers_t request(std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/chat")));
		// Дописываем обязательный для запроса HTTP/1.1 заголовок Host
		request.emplace("Host", "anyks.com");
		// Дописываем заголовок запроса переключения протокола
		request.emplace("Upgrade", "websocket");
		// Объявляем нулевой размер тела запроса
		request.emplace("Content-Length", "0");
		// Отправляем заголовки запроса с завершением сообщения
		sender->sendHeaders(request, true);
		// Получаем собранные байты исходящего запроса
		const std::string wire(sender->pending());
		// Проверяем что рукопожатие собрано
		ASSERT_NE(wire.find("Upgrade: websocket"), std::string::npos);
		// Проверяем что собранное рукопожатие принимает собственный приёмник
		ASSERT_EQ(handshake("Content-Length: 0\r\n\r\n", proto_t::PROXY1), parser_http_t::error_t::NONE);
	}
}

/**
 * @brief Метод тестирования переноса настроек клонированием
 *
 * @details Соединения обслуживают клоны, а настраивают фабрику: настройка, забытая
 *          в clone, оставляет обслуживающий парсер в умолчаниях, и всё, что она
 *          включает, у него молча выключено. Каждая настройка проверяется по
 *          поведению клона, а не по значению геттера: совпадение значений не
 *          означает, что настройка применена - именно так дефект переноса протокола
 *          пережил первую версию собственной проверки
 *
 */
TEST_F(ParserFixture, CloneCarriesSettingsTest){
	/**
	 * Проверяем перенос протокола работы
	 */
	{
		// Создаём фабрику парсеров ответов
		auto factory = this->make(direct_t::RESPONSE);
		// Настраиваем фабрику на работу через прокси
		factory->proto(proto_t::PROXY1);
		// Клонируем настроенную фабрику
		auto cloned = factory->clone();
		// Проверяем что клон получен
		ASSERT_NE(cloned, nullptr);
		// Получаем объект клонированного парсера
		auto parser = static_cast <parser_http_t *> (cloned.get());
		// Формируем ответ, объявляющий кадрирование при невозможности нести тело
		const std::string message = "HTTP/1.1 204 No Content\r\nContent-Length: 5\r\n\r\n";
		// Выполняем разбор сформированного ответа
		parser->parse(message.data(), message.size());
		// Проверяем что клон отверг объявленное кадрирование как работающий через прокси
		ASSERT_EQ(parser->error(), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	}
	/**
	 * Проверяем перенос метода запроса, которому соответствует ожидаемый ответ,
	 * и перенос установленных функций обратного вызова
	 */
	{
		// Количество принятых клоном заголовков
		size_t count = 0;
		// Размер принятого клоном тела
		size_t bytes = 0;
		// Создаём фабрику парсеров ответов
		auto factory = this->make(direct_t::RESPONSE);
		// Настраиваем фабрику на ответ, соответствующий запросу методом HEAD
		factory->method(method_t::HEAD);
		// Устанавливаем функцию обратного вызова обработки заголовков сообщения
		factory->on(parser_http_t::header_callback_t([&count](const uint32_t, const std::string_view, const std::string_view, const parser_t::part_t) noexcept -> bool {
			// Считаем принятый заголовок сообщения
			count++;
			// Продолжаем разбор
			return true;
		}));
		// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		factory->on(parser_http_t::data_callback_t([&bytes](const uint32_t, const void *, const size_t size, const bool) noexcept -> bool {
			// Считаем принятые байты тела сообщения
			bytes += size;
			// Продолжаем разбор
			return true;
		}));
		// Клонируем настроенную фабрику
		auto cloned = factory->clone();
		// Проверяем что клон получен
		ASSERT_NE(cloned, nullptr);
		// Получаем объект клонированного парсера
		auto parser = static_cast <parser_http_t *> (cloned.get());
		/**
		 * Формируем ответ с объявленным размером тела: ответ на запрос HEAD его не
		 * несёт, и объявленный размер описывает тело, которое было бы отправлено
		 * в ответ на такой же запрос GET (RFC 9110 §9.3.2)
		 */
		const std::string message = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n";
		// Выполняем разбор сформированного ответа
		parser->parse(message.data(), message.size());
		// Проверяем что ответ разобран целиком без ожидания тела
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
		// Проверяем что тело до потребителя не дошло
		ASSERT_EQ(bytes, 0u);
		// Проверяем что функция обратного вызова обработки заголовков перенесена
		ASSERT_EQ(count, 1u);
	}
	/**
	 * Проверяем перенос лимитов безопасности разбора
	 */
	{
		// Создаём фабрику парсеров запросов
		auto factory = this->make(direct_t::REQUEST);
		// Формируем лимиты безопасности с пониженным пределом названия заголовка
		parser_http_t::limits_t limits;
		// Понижаем предел размера названия заголовка
		limits.maxHeaderName = 8;
		// Настраиваем фабрику пониженными лимитами
		factory->limits(limits);
		// Клонируем настроенную фабрику
		auto cloned = factory->clone();
		// Проверяем что клон получен
		ASSERT_NE(cloned, nullptr);
		// Получаем объект клонированного парсера
		auto parser = static_cast <parser_http_t *> (cloned.get());
		// Формируем запрос с названием заголовка длиннее установленного предела
		const std::string message = "GET / HTTP/1.1\r\nHost: anyks.com\r\nX-Very-Long-Header-Name: value\r\n\r\n";
		// Выполняем разбор сформированного запроса
		parser->parse(message.data(), message.size());
		// Проверяем что клон отверг заголовок по пониженному пределу
		ASSERT_EQ(parser->error(), parser_http_t::error_t::HEADER_OVERFLOW);
	}
	/**
	 * Проверяем перенос порогов выходного буфера
	 */
	{
		// Создаём фабрику парсеров ответов
		auto factory = this->make(direct_t::RESPONSE);
		// Понижаем пороги выходного буфера отправки
		factory->sendWaterMarks(64, 32);
		// Клонируем настроенную фабрику
		auto cloned = factory->clone();
		// Проверяем что клон получен
		ASSERT_NE(cloned, nullptr);
		// Получаем объект клонированного парсера
		auto parser = static_cast <parser_http_t *> (cloned.get());
		// Формируем контейнер заголовков ответа с провайдером
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Объявляем размер тела ответа
		response.emplace("Content-Length", "10000");
		// Отправляем заголовки ответа (тело последует)
		parser->sendHeaders(response, false);
		// Формируем тело ответа заведомо большего размера, чем понижённый порог
		const std::string body(10000, 'A');
		/**
		 * Проверяем что выходной буфер принял не всё тело: при пороге по умолчанию
		 * тело такого размера уместилось бы целиком, и перенос порога остался бы
		 * непроверенным
		 */
		ASSERT_LT(parser->sendData(body.data(), body.size(), true), body.size());
	}
	/**
	 * Проверяем перенос объёма одной прокачки pull-источника данных
	 */
	{
		// Позиция чтения тела источником данных
		size_t position = 0;
		// Создаём фабрику парсеров ответов
		auto factory = this->make(direct_t::RESPONSE);
		/**
		 * Устанавливаем функцию обратного вызова записи исходящих байтов
		 *
		 * Объём одной прокачки применяется только в push-модели: в pull-модели
		 * выходной буфер наполняется до верхнего порога однократно, и настройка
		 * не участвует в работе вовсе
		 */
		factory->on(parser_http_t::write_callback_t([](const void *, const size_t) noexcept {}));
		/**
		 * Понижаем пороги выходного буфера вместе с объёмом одной прокачки
		 *
		 * Одна дозагрузка ограничена свободным местом буфера, а прокачка прекращается
		 * по накоплении заданного объёма, поэтому связывает их пара: с объёмом по
		 * умолчанию прокачка выкачала бы тело целиком за один заход, опустошая буфер
		 * записью после каждой дозагрузки
		 */
		factory->sendWaterMarks(256, 128);
		// Понижаем объём одной прокачки pull-источника данных
		factory->pumpLimit(128);
		// Клонируем настроенную фабрику
		auto cloned = factory->clone();
		// Проверяем что клон получен
		ASSERT_NE(cloned, nullptr);
		// Получаем объект клонированного парсера
		auto parser = static_cast <parser_http_t *> (cloned.get());
		// Формируем контейнер заголовков ответа с провайдером
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Объявляем размер тела ответа
		response.emplace("Content-Length", "10000");
		// Отправляем заголовки ответа (тело последует из pull-источника данных)
		parser->sendHeaders(response, false);
		// Назначаем pull-источник данных тела сообщения
		parser->dataSource(parser_http_t::data_source_callback_t([&position](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
			// Вычисляем размер выдаваемой порции тела
			const size_t size = std::min(cap, (static_cast <size_t> (10000) - position));
			// Заполняем выдаваемую порцию тела
			std::memset(buffer, 'A', size);
			// Сдвигаем позицию чтения тела источником
			position += size;
			// Выставляем признак достижения конца тела
			eof = (position == 10000);
			// Выводим размер выданной порции тела
			return static_cast <int64_t> (size);
		}));
		// Проверяем что прокачка всё же состоялась
		ASSERT_GT(position, 0u);
		// Проверяем что первая прокачка ограничена перенесённым объёмом
		ASSERT_LT(position, 1024u);
		// Проверяем что отправка тела осталась незавершённой
		ASSERT_TRUE(parser->sourcePending());
	}
}

/**
 * @brief Метод тестирования игнорирования полей HTTP/1.1 в запросе HTTP/1.0
 *
 * @details Заголовок Upgrade в запросе HTTP/1.0 предписано игнорировать (RFC 9110 §7.8),
 *          как и ожидание 100-continue (RFC 9110 §10.1.1). Обе поблажки нужны по одной
 *          причине: отправитель HTTP/1.0 не умеет обрабатывать то, к чему эти поля ведут.
 *          Промежуточный ответ [100 Continue] он прочитает как окончательный и уйдёт в
 *          рассинхронизацию, а смену протокола на соединении, где о ней не договаривались,
 *          не поймёт вовсе
 *
 */
TEST_F(ParserFixture, LegacyRequestFieldsTest){
	/**
	 * @brief Функция разбора запроса заданной версии протокола
	 *
	 * @param version версия протокола запроса
	 * @param lines   строки заголовков запроса
	 * @return        флаги состояния разобранного сообщения
	 *
	 */
	auto probe = [this](const std::string & version, const std::string & lines) noexcept -> parser_http_t::message_t::flags_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Формируем разбираемое сообщение
		const std::string message = ("POST / " + version + "\r\nHost: anyks.com\r\n" + lines + "Content-Length: 0\r\n\r\n");
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Проверяем что сообщение полностью разобрано
		EXPECT_EQ(parser->status(), parser_t::status_t::COMPLETE) << version;
		// Выводим флаги состояния разобранного сообщения
		return parser->message().flags;
	};
	// Проверяем что ожидание промежуточного ответа в запросе HTTP/1.1 распознаётся
	ASSERT_TRUE(probe("HTTP/1.1", "Expect: 100-continue\r\n").expectContinue);
	// Проверяем что ожидание промежуточного ответа в запросе HTTP/1.0 игнорируется
	ASSERT_FALSE(probe("HTTP/1.0", "Expect: 100-continue\r\n").expectContinue);
	// Проверяем что запрос переключения протокола в запросе HTTP/1.1 распознаётся
	ASSERT_TRUE(probe("HTTP/1.1", "Upgrade: websocket\r\nConnection: upgrade\r\n").upgrade);
	// Проверяем что запрос переключения протокола в запросе HTTP/1.0 игнорируется
	ASSERT_FALSE(probe("HTTP/1.0", "Upgrade: websocket\r\nConnection: upgrade\r\n").upgrade);
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
