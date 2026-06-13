/**
 * @file http_test.cpp
 * @brief Регрессионные тесты парсера HTTP/1.1 (awh::http) — самодостаточный раннер.
 *
 *  В отличие от http_demo.cpp (наглядные примеры), здесь — проверки корректности
 *  с авто-вердиктом и ненулевым кодом возврата при провале. Внешних фреймворков
 *  нет: крошечный набор макросов CHECK/CHECK_EQ, процедурный стиль как у парсера.
 *
 *  Сборка и запуск:
 *      c++ -std=c++17 -O2 -Wall -Wextra http.cpp http_test.cpp -o http_test && ./http_test
 *
 *  Рекомендуется прогонять под санитайзерами:
 *      c++ -std=c++17 -O1 -g -fsanitize=address,undefined -Wall -Wextra \
 *          http.cpp http_test.cpp -o http_test_dbg && ./http_test_dbg
 *
 *  Код возврата: 0 — все тесты прошли, иначе число провалившихся проверок.
 */

#include "http.hpp"

#include <cstdio>
#include <cstdint>
#include <new>
#include <cstdlib>
#include <string>
#include <vector>

namespace ah = awh::http;

// ============================================================================
//  Микро-харнесс проверок.
// ============================================================================
namespace {
	int g_checks = 0;
	int g_failed = 0;
	const char * g_group = "";

	void group(const char * name) {
		g_group = name;
		std::printf("\n\033[1;36m# %s\033[0m\n", name);
	}

	void checkImpl(bool ok, const char * expr, const char * file, int line) {
		++g_checks;
		if (ok) {
			std::printf("  \033[32mok\033[0m   %s\n", expr);
		} else {
			++g_failed;
			std::printf("  \033[31mFAIL\033[0m %s\n       (%s:%d, группа \"%s\")\n",
			            expr, file, line, g_group);
		}
	}

	#define CHECK(cond) checkImpl((cond), #cond, __FILE__, __LINE__)

	// --- удобные обёртки разбора ---

	struct Parsed {
		ah::Status   status = ah::Status::OK;
		ah::Error    error  = ah::Error::NONE;
		size_t       used   = 0;
	};

	/// Разобрать весь буфер как одно сообщение (опц. сигнал EOF при OK).
	Parsed parseAll(ah::Parser & p, const std::string & raw, bool eof = true) {
		Parsed r;
		r.used = ah::execute(p, raw.data(), raw.size(), r.status);
		if (eof && r.status == ah::Status::OK)
			ah::execute(p, nullptr, 0, r.status);
		r.error = p.error;
		return r;
	}

	/// Скормить буфер парсеру кусками фиксированного размера (проверка инкрементальности).
	Parsed parseChunked(ah::Parser & p, const std::string & raw, size_t step) {
		Parsed r;
		size_t off = 0;
		while (off < raw.size()) {
			const size_t n = (raw.size() - off < step) ? (raw.size() - off) : step;
			const size_t used = ah::execute(p, raw.data() + off, n, r.status);
			off += used;
			r.used += used;
			if (r.status == ah::Status::ERROR || r.status == ah::Status::COMPLETE) break;
		}
		r.error = p.error;
		return r;
	}
}

// ============================================================================
//  1. Базовый разбор запросов.
// ============================================================================
static void testRequests() {
	group("Запросы: базовый разбор");
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET /index.html?x=1 HTTP/1.1\r\nHost: anyks.com\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.method == ah::Method::GET);
		CHECK(p.message.methodName == "GET");
		CHECK(p.message.target == "/index.html?x=1");
		CHECK(p.message.versionMajor == 1 && p.message.versionMinor == 1);
		CHECK(p.message.headers.size() == 1);
		CHECK(p.message.keepAlive == true);
		CHECK(p.message.body.empty());
	}
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		const std::string body = "name=anyks&project=awh";
		Parsed r = parseAll(p, "POST /api HTTP/1.1\r\nContent-Length: " +
		                    std::to_string(body.size()) + "\r\n\r\n" + body);
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.method == ah::Method::POST);
		CHECK(p.message.hasContentLength && p.message.contentLength == body.size());
		CHECK(p.message.body == body);
	}
}

// ============================================================================
//  2. Расширенные методы (WebDAV и пр.).
// ============================================================================
static void testMethods() {
	group("Методы: классификация");
	struct { const char * name; ah::Method m; } cases[] = {
		{"GET", ah::Method::GET}, {"DELETE", ah::Method::DELETE_},
		{"PATCH", ah::Method::PATCH}, {"PROPFIND", ah::Method::PROPFIND},
		{"MKCALENDAR", ah::Method::MKCALENDAR}, {"M-SEARCH", ah::Method::MSEARCH},
		{"UNSUBSCRIBE", ah::Method::UNSUBSCRIBE}, {"ACL", ah::Method::ACL},
	};
	for (auto & c : cases) {
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, std::string(c.name) + " / HTTP/1.1\r\nHost: x\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.method == c.m);
		CHECK(p.message.methodName == c.name);
	}
	// Нераспознанный, но синтаксически корректный метод => UNKNOWN, имя сохранено.
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "WAT / HTTP/1.1\r\nHost: x\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.method == ah::Method::UNKNOWN);
		CHECK(p.message.methodName == "WAT");
	}
}

// ============================================================================
//  3. Ответы и статус-строка.
// ============================================================================
static void testResponses() {
	group("Ответы: статус-строка");
	{
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nhi");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.statusCode == 200);
		CHECK(p.message.reason == "OK");
		CHECK(p.message.body == "hi");
	}
	{	// Без reason-фразы.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1 204\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.statusCode == 204);
	}
	{	// Дополнительные пробелы перед кодом допустимы.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1   200   OK\r\nContent-Length: 0\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE && p.message.statusCode == 200);
	}
	group("Ответы: невалидная статус-строка");
	{	// Нет SP после версии (FIX #7).
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1200 OK\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_VERSION);
	}
	{	// 4 цифры кода.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1 2000 OK\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_STATUS);
	}
	{	// 2 цифры кода.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1 20 OK\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_STATUS);
	}
}

// ============================================================================
//  4. Chunked + трейлеры.
// ============================================================================
static void testChunked() {
	group("Chunked: тело и трейлеры");
	{
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nTrailer: X-Sum\r\n\r\n"
			"5\r\nHello\r\n6\r\n world\r\n0\r\nX-Sum: abc\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.chunked == true);
		CHECK(p.message.body == "Hello world");
		CHECK(p.message.trailers.size() == 1);
		CHECK(p.message.trailers.size() == 1 && p.message.trailers[0].name == "X-Sum" &&
		      p.message.trailers[0].value == "abc");
	}
	{	// chunk-ext игнорируется, тело собирается верно.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
			"A;name=value\r\n0123456789\r\n0\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body == "0123456789");
	}
	{	// chunked без трейлеров.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body == "abc" &&
		      p.message.trailers.empty());
	}
}

// ============================================================================
//  5. Transfer-Encoding: порядок кодирований и кадрирование.
// ============================================================================
static void testTransferEncoding() {
	group("Transfer-Encoding: порядок кодирований");
	{	// gzip, chunked — chunked последний => валидно.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip, chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE && p.message.chunked && p.message.body == "abc");
	}
	{	// chunked, gzip — chunked не последний; для ответа = читаем до закрытия.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked, gzip\r\n\r\nraw");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_TRANSFER_ENCODING);
	}
	{	// chunked для запроса без финального chunked-кодирования — ошибка.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p,
			"POST / HTTP/1.1\r\nTransfer-Encoding: gzip\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_TRANSFER_ENCODING);
	}
}

// ============================================================================
//  6. Content-Length: списки, дубликаты, мусор.
// ============================================================================
static void testContentLength() {
	group("Content-Length: валидация");
	{	// Одинаковые дубликаты допустимы.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: 3\r\nContent-Length: 3\r\n\r\nabc");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body == "abc");
	}
	{	// Разные дубликаты => конфликт.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: 3\r\nContent-Length: 4\r\n\r\nabc");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::CONTENT_LENGTH_CONFLICT);
	}
	{	// Список "3, 3" — одинаковые элементы допустимы.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: 3, 3\r\n\r\nabc");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body == "abc");
	}
	{	// Знак "+" недопустим.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: +5\r\n\r\nhello");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_CONTENT_LENGTH);
	}
	{	// Переполнение uint64.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: 99999999999999999999999\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_CONTENT_LENGTH);
	}
}

// ============================================================================
//  7. Защита от request smuggling.
// ============================================================================
static void testSmuggling() {
	group("Безопасность: request smuggling");
	{	// CL + TE одновременно => отказ.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p,
			"POST / HTTP/1.1\r\nContent-Length: 6\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::CONTENT_LENGTH_CONFLICT);
	}
}

// ============================================================================
//  8. Невалидные заголовки.
// ============================================================================
static void testBadHeaders() {
	group("Заголовки: невалидные");
	{	// Пробел в имени.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET / HTTP/1.1\r\nBad Header: v\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_HEADER_TOKEN);
	}
	{	// Пустое имя.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET / HTTP/1.1\r\n: v\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_HEADER_TOKEN);
	}
	{	// Пробел перед двоеточием недопустим.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET / HTTP/1.1\r\nHost : x\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_HEADER_TOKEN);
	}
	{	// obs-fold (продолжение значения с отступа) отвергается.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET / HTTP/1.1\r\nX: a\r\n b\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_HEADER_TOKEN);
	}
	{	// OWS вокруг значения тримится.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET / HTTP/1.1\r\nX:   value here   \r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.headers.size() == 1 && p.message.headers[0].value == "value here");
	}
}

// ============================================================================
//  9. Лимиты / DoS.
// ============================================================================
static void testLimits() {
	group("Лимиты: число заголовков");
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		p.limits.maxHeaderCount = 4;
		std::string raw = "GET / HTTP/1.1\r\n";
		for (int i = 0; i < 5; ++i) raw += "X-H" + std::to_string(i) + ": v\r\n";
		raw += "\r\n";
		Parsed r = parseAll(p, raw);
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::TOO_MANY_HEADERS);
	}
	group("Лимиты: размер тела");
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		p.limits.maxBodySize = 4;
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::BODY_OVERFLOW);
	}
	group("Лимиты: длина строки заголовка чанка (FIX #4 DoS)");
	{
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.limits.maxChunkLine = 64;
		std::string raw = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5;";
		raw += std::string(200, 'a'); // длинный chunk-ext без CRLF
		Parsed r = parseAll(p, raw, /*eof=*/false);
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::CHUNK_OVERFLOW);
	}
	group("Лимиты: размер чанка");
	{
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.limits.maxChunkSize = 4;
		Parsed r = parseAll(p, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n10\r\n", false);
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::CHUNK_OVERFLOW);
	}
	group("Лимиты: предвыделение тела ограничено maxBodyPrealloc (FIX #4)");
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		// Анонсируем огромный Content-Length, но не шлём тело.
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: 50000000\r\n\r\n", false);
		CHECK(r.status == ah::Status::OK);
		// capacity не должен раздуваться до 50 МБ (с небольшим запасом на округление аллокатора).
		CHECK(p.message.body.capacity() < p.limits.maxBodyPrealloc + 4096);
	}
}

// ============================================================================
//  10. keep-alive семантика (FIX #1).
// ============================================================================
static void testKeepAlive() {
	group("keep-alive: семантика");
	{	// HTTP/1.1 по умолчанию keep-alive.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		parseAll(p, "GET / HTTP/1.1\r\nHost: x\r\n\r\n");
		CHECK(p.message.keepAlive == true);
	}
	{	// HTTP/1.1 + Connection: close.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		parseAll(p, "GET / HTTP/1.1\r\nConnection: close\r\n\r\n");
		CHECK(p.message.keepAlive == false);
	}
	{	// HTTP/1.0 по умолчанию НЕ keep-alive.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		parseAll(p, "HTTP/1.0 200 OK\r\nContent-Length: 2\r\n\r\nhi");
		CHECK(p.message.keepAlive == false);
	}
	{	// HTTP/1.0 + Connection: keep-alive.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		parseAll(p, "HTTP/1.0 200 OK\r\nConnection: keep-alive\r\nContent-Length: 2\r\n\r\nhi");
		CHECK(p.message.keepAlive == true);
	}
	{	// Тело "до закрытия" => keepAlive ОБЯЗАН быть false (FIX #1).
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello world");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.keepAlive == false);
		CHECK(p.message.body == "Hello world");
	}
}

// ============================================================================
//  11. Ответы без тела: 1xx / 204 / 304 / HEAD / CONNECT.
// ============================================================================
static void testNoBodyResponses() {
	group("Ответы без тела");
	{	// 1xx.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1 100 Continue\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body.empty());
	}
	{	// 304 с Content-Length, но тела нет.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseAll(p, "HTTP/1.1 304 Not Modified\r\nContent-Length: 100\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body.empty());
	}
	{	// HEAD: Content-Length есть, тело не читается.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.responseToHead = true;
		Parsed r = parseAll(p, "HTTP/1.1 200 OK\r\nContent-Length: 1048576\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body.empty());
	}
	group("CONNECT-туннель (FIX #6)");
	{	// 2xx-ответ на CONNECT — тела нет, разбор завершается до данных туннеля.
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.responseToConnect = true;
		const std::string raw = "HTTP/1.1 200 Connection Established\r\n\r\nTUNNELDATA";
		Parsed r = parseAll(p, raw, /*eof=*/false);
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.body.empty());
		CHECK(r.used == raw.find("TUNNELDATA")); // данные туннеля не потреблены
	}
	{	// 4xx-ответ на CONNECT — обычная семантика тела (здесь до закрытия).
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.responseToConnect = true;
		Parsed r = parseAll(p, "HTTP/1.1 407 Proxy Auth Required\r\nContent-Length: 3\r\n\r\nerr");
		CHECK(r.status == ah::Status::COMPLETE && p.message.body == "err");
	}
}

// ============================================================================
//  12. Инкрементальный разбор (разрыв в любом байте).
// ============================================================================
static void testIncremental() {
	group("Инкрементальность: подача по 1 байту");
	const std::string raw =
		"PUT /r/42 HTTP/1.1\r\nHost: x\r\nContent-Length: 11\r\n\r\nhello world";
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseChunked(p, raw, 1);
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(p.message.method == ah::Method::PUT);
		CHECK(p.message.body == "hello world");
	}
	// Разные размеры кусков должны давать одинаковый результат.
	for (size_t step : {2u, 3u, 5u, 7u, 13u}) {
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseChunked(p, raw, step);
		CHECK(r.status == ah::Status::COMPLETE && p.message.body == "hello world");
	}
	group("Инкрементальность: chunked по 1 байту");
	{
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		Parsed r = parseChunked(p,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n0\r\n\r\n", 1);
		CHECK(r.status == ah::Status::COMPLETE && p.message.body == "Hello");
	}
}

// ============================================================================
//  13. Конвейер (pipelining) + reset(): сброс per-message флагов (FIX #2).
// ============================================================================
static void testPipelining() {
	group("Конвейер: 3 запроса через reset()");
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		const std::string raw =
			"GET /a HTTP/1.1\r\nHost: x\r\n\r\n"
			"GET /b HTTP/1.1\r\nHost: y\r\n\r\n"
			"DELETE /c HTTP/1.1\r\nHost: z\r\n\r\n";
		const char * d = raw.data(); size_t left = raw.size();
		std::vector<std::string> targets;
		ah::Status st = ah::Status::OK;
		while (left > 0) {
			const size_t used = ah::execute(p, d, left, st);
			if (st != ah::Status::COMPLETE) break;
			targets.push_back(p.message.target);
			d += used; left -= used;
			ah::reset(p);
		}
		CHECK(targets.size() == 3);
		CHECK(targets.size() == 3 && targets[0] == "/a" && targets[1] == "/b" && targets[2] == "/c");
	}
	group("reset() сбрасывает responseToHead (FIX #2)");
	{	// 1-й ответ — на HEAD (тело пропускается), 2-й — обычный (тело должно прочитаться).
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.responseToHead = true;
		const std::string buf =
			"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n"           // HEAD: тела нет
			"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHELLO";    // GET: тело HELLO
		const char * d = buf.data(); size_t left = buf.size();
		ah::Status st = ah::Status::OK;
		const size_t u1 = ah::execute(p, d, left, st);
		CHECK(st == ah::Status::COMPLETE && p.message.body.empty());
		d += u1; left -= u1;
		ah::reset(p);
		CHECK(p.responseToHead == false); // флаг сброшен
		const size_t u2 = ah::execute(p, d, left, st);
		CHECK(st == ah::Status::COMPLETE && p.message.body == "HELLO");
		CHECK(u2 == left); // второй ответ потреблён полностью вместе с телом
	}
}

// ============================================================================
//  14. Трейлерам выдаётся собственный бюджет лимитов (FIX #8).
// ============================================================================
static void testTrailerBudget() {
	group("Трейлеры: собственный бюджет лимитов (FIX #8)");
	{
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.limits.maxHeaderCount = 128;
		std::string raw = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n";
		for (int i = 0; i < 120; ++i) raw += "X-H" + std::to_string(i) + ": v\r\n";
		raw += "\r\n0\r\nX-Trailer: ok\r\n\r\n";
		Parsed r = parseAll(p, raw);
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(!p.message.trailers.empty() && p.message.trailers[0].name == "X-Trailer");
	}
}

// ============================================================================
//  15. Потоковый (streaming) API: zero-copy тело и chunked через callback'и.
// ============================================================================
namespace {
	uint64_t s_bytes = 0;
	int s_chunks = 0;
	bool s_headersDone = false;
	bool s_msgDone = false;

	bool cbHeaders(ah::Parser &, void *) { s_headersDone = true; return true; }
	bool cbBody(ah::Parser &, void *, const char *, size_t n) { s_bytes += n; return true; }
	bool cbChunkHdr(ah::Parser &, void *, uint64_t sz) { if (sz) ++s_chunks; return true; }
	bool cbDone(ah::Parser &, void *) { s_msgDone = true; return true; }
	bool cbAbortBody(ah::Parser &, void *, const char *, size_t) { return false; } // прервать
}

static void testStreaming() {
	group("Streaming: zero-copy тело (storeBody=false)");
	{
		const size_t bodySize = 1u * 1024 * 1024;
		std::string raw = "POST /u HTTP/1.1\r\nContent-Length: " +
		                  std::to_string(bodySize) + "\r\n\r\n" + std::string(bodySize, 'Z');
		ah::Handler h{};
		h.onHeadersComplete = cbHeaders; h.onBody = cbBody; h.onMessageComplete = cbDone;
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		p.handler = &h; p.storeBody = false;
		s_bytes = 0; s_headersDone = s_msgDone = false;
		Parsed r = parseAll(p, raw);
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(s_headersDone && s_msgDone);
		CHECK(s_bytes == bodySize);
		CHECK(p.message.body.empty()); // тело не буферизовалось
	}
	group("Streaming: chunked через callback'и");
	{
		ah::Handler h{};
		h.onChunkHeader = cbChunkHdr; h.onBody = cbBody; h.onMessageComplete = cbDone;
		ah::Parser p; ah::init(p, ah::Type::RESPONSE);
		p.handler = &h; p.storeBody = false;
		s_bytes = 0; s_chunks = 0; s_msgDone = false;
		Parsed r = parseAll(p,
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
			"4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n");
		CHECK(r.status == ah::Status::COMPLETE);
		CHECK(s_chunks == 2 && s_bytes == 9 && s_msgDone);
	}
	group("Streaming: прерывание из callback => ABORTED");
	{
		ah::Handler h{};
		h.onBody = cbAbortBody;
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		p.handler = &h;
		Parsed r = parseAll(p, "POST / HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::ABORTED);
	}
}

// ============================================================================
//  16. EOL: голый LF принимается, одинокий CR — ошибка.
// ============================================================================
static void testEol() {
	group("EOL: переносы строк");
	{	// Голый LF (без CR) — толерантно принимается.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET / HTTP/1.1\nHost: x\n\n");
		CHECK(r.status == ah::Status::COMPLETE);
	}
	{	// CR без LF внутри строки заголовка — ошибка EOL.
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		Parsed r = parseAll(p, "GET / HTTP/1.1\r\nHost: x\rZ\r\n\r\n");
		CHECK(r.status == ah::Status::ERROR && r.error == ah::Error::INVALID_EOL);
	}
}

// ============================================================================
//  17. noexcept-устойчивость к сбою аллокации (FIX #5).
//      Глобально перехватываем operator new и включаем сбой только на время теста.
// ============================================================================
namespace {
	bool g_oom = false;
}
void * operator new(std::size_t n) {
	if (g_oom) throw std::bad_alloc();
	void * p = std::malloc(n ? n : 1);
	if (!p) throw std::bad_alloc();
	return p;
}
void operator delete(void * p) noexcept { std::free(p); }
void operator delete(void * p, std::size_t) noexcept { std::free(p); }

static void testNoexceptOom() {
	group("noexcept: сбой аллокации => Error::INTERNAL без краха (FIX #5)");
	{
		ah::Parser p; ah::init(p, ah::Type::REQUEST);
		std::string raw = "POST / HTTP/1.1\r\nX-Big: " + std::string(8192, 'a') + "\r\n\r\n";
		ah::Status st = ah::Status::OK;
		g_oom = true;                 // любая аллокация теперь бросает bad_alloc
		ah::execute(p, raw.data(), raw.size(), st);
		g_oom = false;
		CHECK(st == ah::Status::ERROR && p.error == ah::Error::INTERNAL);
	}
}

// ============================================================================
int main() {
	std::printf("\033[1mРегрессионные тесты парсера HTTP/1.1 (awh::http)\033[0m\n");

	testRequests();
	testMethods();
	testResponses();
	testChunked();
	testTransferEncoding();
	testContentLength();
	testSmuggling();
	testBadHeaders();
	testLimits();
	testKeepAlive();
	testNoBodyResponses();
	testIncremental();
	testPipelining();
	testTrailerBudget();
	testStreaming();
	testEol();
	testNoexceptOom();

	std::printf("\n========================================\n");
	if (g_failed == 0)
		std::printf("\033[1;32mВСЕ ТЕСТЫ ПРОЙДЕНЫ\033[0m: %d проверок.\n", g_checks);
	else
		std::printf("\033[1;31mЕСТЬ ПРОВАЛЫ\033[0m: %d из %d проверок не прошли.\n", g_failed, g_checks);
	return g_failed;
}
