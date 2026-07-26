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
#include <string>
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
