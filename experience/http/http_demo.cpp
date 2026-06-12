/**
 * @file http_demo.cpp
 * @brief Демонстрационные примеры использования парсера HTTP/1.1 (НЕ unit-тесты).
 *
 *  Цель файла — наглядно показать человеку, как работает парсер: что подаётся
 *  на вход и что получается на выходе. Каждый пример печатает разобранную
 *  структуру сообщения в читаемом виде.
 *
 *  Сборка (см. также шапку http.cpp):
 *      c++ -std=c++17 -O3 -march=native -Wall -Wextra \
 *          http.cpp http_demo.cpp -o http_demo
 *      ./http_demo
 */

#include "http.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace ah = awh::http;

namespace {
	// --- маленькие хелперы печати (тоже без классов) ---

	void hr(const char * title) {
		std::printf("\n\033[1;36m======== %s ========\033[0m\n", title);
	}

	void note(const char * text) {
		std::printf("\033[2m%s\033[0m\n", text);
	}

	/// Печать «сырого» сообщения с подсветкой CRLF, чтобы видеть структуру.
	void printRaw(const std::string & raw) {
		std::printf("\033[33mВХОД:\033[0m\n");
		std::string out;
		for (char c : raw) {
			if (c == '\r') out += "\\r";
			else if (c == '\n') out += "\\n\n";
			else out += c;
		}
		std::printf("%s\n", out.c_str());
	}

	/// Печать разобранного сообщения.
	void printMessage(const ah::Message & m) {
		std::printf("\033[32mРАЗОБРАНО:\033[0m\n");
		if (m.type == ah::Type::REQUEST) {
			std::printf("  тип            : ЗАПРОС\n");
			std::printf("  метод          : %s (%s)\n",
			            m.methodName.c_str(), ah::methodName(m.method));
			std::printf("  target         : %s\n", m.target.c_str());
		} else {
			std::printf("  тип            : ОТВЕТ\n");
			std::printf("  статус         : %u %s\n", m.statusCode, m.reason.c_str());
		}
		std::printf("  версия         : HTTP/%u.%u\n", m.versionMajor, m.versionMinor);
		std::printf("  keep-alive     : %s\n", m.keepAlive ? "да" : "нет");
		std::printf("  chunked        : %s\n", m.chunked ? "да" : "нет");
		if (m.hasContentLength)
			std::printf("  Content-Length : %llu\n", (unsigned long long) m.contentLength);

		std::printf("  заголовки (%zu):\n", m.headers.size());
		for (const auto & h : m.headers)
			std::printf("      %s: %s\n", h.name.c_str(), h.value.c_str());

		if (!m.trailers.empty()) {
			std::printf("  трейлеры (%zu):\n", m.trailers.size());
			for (const auto & h : m.trailers)
				std::printf("      %s: %s\n", h.name.c_str(), h.value.c_str());
		}

		std::printf("  тело (%zu байт) : ", m.body.size());
		if (m.body.size() <= 200) std::printf("\"%s\"\n", m.body.c_str());
		else std::printf("\"%.*s...\"\n", 200, m.body.c_str());
	}

	/// Разобрать целиком переданный буфер как одно сообщение.
	void parseWhole(ah::Type type, const std::string & raw, bool responseToHead = false) {
		printRaw(raw);
		ah::Parser p;
		ah::init(p, type);
		p.responseToHead = responseToHead;

		ah::Status st = ah::Status::OK;
		const size_t used = ah::execute(p, raw.data(), raw.size(), st);

		// Для ответов «до закрытия» нужен сигнал EOF.
		if (st == ah::Status::OK)
			ah::execute(p, nullptr, 0, st);

		if (st == ah::Status::ERROR) {
			std::printf("\033[31mОШИБКА: %s (потреблено %zu байт)\033[0m\n",
			            ah::errorName(p.error), used);
			return;
		}
		if (st != ah::Status::COMPLETE) {
			std::printf("\033[31mНеполное сообщение (нужно больше данных)\033[0m\n");
			return;
		}
		printMessage(p.message);
	}
}

// ============================================================================
//  Пример 1: простой GET-запрос
// ============================================================================
static void demoSimpleRequest() {
	hr("Пример 1: простой GET-запрос");
	const std::string raw =
		"GET /index.html?lang=ru HTTP/1.1\r\n"
		"Host: anyks.com\r\n"
		"User-Agent: awh-demo/1.0\r\n"
		"Accept: text/html\r\n"
		"\r\n";
	parseWhole(ah::Type::REQUEST, raw);
}

// ============================================================================
//  Пример 2: POST с телом фиксированной длины (Content-Length)
// ============================================================================
static void demoPostContentLength() {
	hr("Пример 2: POST с Content-Length");
	const std::string body = "name=anyks&project=awh";
	const std::string raw =
		"POST /api/v1/submit HTTP/1.1\r\n"
		"Host: anyks.com\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Content-Length: " + std::to_string(body.size()) + "\r\n"
		"\r\n" + body;
	parseWhole(ah::Type::REQUEST, raw);
}

// ============================================================================
//  Пример 3: ответ с chunked-кодированием и трейлерами
// ============================================================================
static void demoChunkedWithTrailers() {
	hr("Пример 3: ответ chunked + трейлеры");
	note("Тело собирается из чанков 5+6 байт, в конце — трейлер.");
	const std::string raw =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Trailer: X-Checksum\r\n"
		"\r\n"
		"5\r\n"
		"Hello\r\n"
		"6\r\n"
		" world\r\n"
		"0\r\n"
		"X-Checksum: 0xCAFEBABE\r\n"
		"\r\n";
	parseWhole(ah::Type::RESPONSE, raw);
}

// ============================================================================
//  Пример 4: инкрементальный разбор (данные приходят по 1 байту)
// ============================================================================
static void demoIncremental() {
	hr("Пример 4: потоковый разбор по 1 байту");
	note("Тот же запрос подаётся парсеру побайтно — состояние сохраняется между вызовами.");
	const std::string raw =
		"PUT /resource/42 HTTP/1.1\r\n"
		"Host: anyks.com\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"hello world";
	printRaw(raw);

	ah::Parser p;
	ah::init(p, ah::Type::REQUEST);

	ah::Status st = ah::Status::OK;
	size_t fed = 0;
	for (size_t i = 0; i < raw.size(); ++i) {
		ah::execute(p, raw.data() + i, 1, st);
		++fed;
		if (st == ah::Status::ERROR) {
			std::printf("\033[31mОШИБКА на байте %zu: %s\033[0m\n", fed, ah::errorName(p.error));
			return;
		}
		if (st == ah::Status::COMPLETE) break;
	}
	std::printf("Сообщение собрано после %zu отдельных вызовов execute().\n", fed);
	printMessage(p.message);
}

// ============================================================================
//  Пример 5: конвейер (pipelining) — несколько запросов в одном буфере
// ============================================================================
static void demoPipelining() {
	hr("Пример 5: конвейер из 3 запросов");
	note("Один буфер -> reset() между сообщениями -> разбор по очереди.");
	const std::string raw =
		"GET /a HTTP/1.1\r\nHost: x\r\n\r\n"
		"GET /b HTTP/1.1\r\nHost: y\r\n\r\n"
		"DELETE /c HTTP/1.1\r\nHost: z\r\n\r\n";

	ah::Parser p;
	ah::init(p, ah::Type::REQUEST);

	const char * data = raw.data();
	size_t left = raw.size();
	int idx = 0;
	while (left > 0) {
		ah::Status st = ah::Status::OK;
		const size_t used = ah::execute(p, data, left, st);
		if (st == ah::Status::ERROR) {
			std::printf("\033[31mОШИБКА: %s\033[0m\n", ah::errorName(p.error));
			return;
		}
		if (st == ah::Status::COMPLETE) {
			std::printf("  [%d] %s %s\n", ++idx,
			            p.message.methodName.c_str(), p.message.target.c_str());
			data += used;
			left -= used;
			ah::reset(p); // готовим к следующему сообщению
			continue;
		}
		break; // нужно больше данных
	}
	std::printf("Всего разобрано сообщений: %d\n", idx);
}

// ============================================================================
//  Пример 6: безопасность — защита от request smuggling (CL + TE)
// ============================================================================
static void demoSecuritySmuggling() {
	hr("Пример 6: защита от smuggling (Content-Length + Transfer-Encoding)");
	note("Одновременное присутствие CL и TE должно отвергаться (RFC 7230).");
	const std::string raw =
		"POST / HTTP/1.1\r\n"
		"Host: anyks.com\r\n"
		"Content-Length: 6\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"0\r\n\r\n";
	parseWhole(ah::Type::REQUEST, raw);
}

// ============================================================================
//  Пример 7: безопасность — недопустимые символы в имени заголовка
// ============================================================================
static void demoSecurityBadHeader() {
	hr("Пример 7: отклонение некорректного заголовка");
	const std::string raw =
		"GET / HTTP/1.1\r\n"
		"Bad Header: value\r\n"   // пробел в имени недопустим
		"\r\n";
	parseWhole(ah::Type::REQUEST, raw);
}

// ============================================================================
//  Пример 8: ответ на HEAD (тело не читается, даже если есть Content-Length)
// ============================================================================
static void demoHeadResponse() {
	hr("Пример 8: ответ на HEAD-запрос");
	note("Content-Length присутствует, но тела в ответе на HEAD нет.");
	const std::string raw =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/pdf\r\n"
		"Content-Length: 1048576\r\n"
		"\r\n";
	parseWhole(ah::Type::RESPONSE, raw, /*responseToHead=*/true);
}

// ============================================================================
//  Пример 9: микро-бенчмарк пропускной способности
// ============================================================================
static void demoBenchmark() {
	hr("Пример 9: микро-бенчмарк производительности");
	const std::string one =
		"GET /index.html HTTP/1.1\r\n"
		"Host: anyks.com\r\n"
		"User-Agent: awh-benchmark/1.0\r\n"
		"Accept: text/html,application/xhtml+xml\r\n"
		"Accept-Encoding: gzip, deflate, br\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";

	const int iters = 2'000'000;
	const auto t0 = std::chrono::steady_clock::now();

	ah::Parser p;
	ah::init(p, ah::Type::REQUEST);
	size_t completed = 0;
	for (int i = 0; i < iters; ++i) {
		ah::reset(p);
		ah::Status st = ah::Status::OK;
		ah::execute(p, one.data(), one.size(), st);
		if (st == ah::Status::COMPLETE) ++completed;
	}

	const auto t1 = std::chrono::steady_clock::now();
	const double sec = std::chrono::duration<double>(t1 - t0).count();
	const double mb = (double(one.size()) * iters) / (1024.0 * 1024.0);

	std::printf("Размер запроса      : %zu байт\n", one.size());
	std::printf("Итераций            : %d (успешно: %zu)\n", iters, completed);
	std::printf("Время               : %.3f c\n", sec);
	std::printf("Скорость (запросы)  : %.2f млн req/s\n", iters / sec / 1e6);
	std::printf("Пропускная способность: %.1f МБ/с\n", mb / sec);
}

// ============================================================================
//  Пример 10: расширенный набор методов (WebDAV и пр.)
// ============================================================================
static void demoExtendedMethods() {
	hr("Пример 10: распознавание расширенных методов (WebDAV/RFC 5789)");
	const char * methods[] = {
		"GET", "PATCH", "PROPFIND", "PROPPATCH", "MKCOL", "MKCALENDAR",
		"REPORT", "M-SEARCH", "SUBSCRIBE", "UNSUBSCRIBE", "ACL", "WAT"
	};
	for (const char * m : methods) {
		const std::string raw = std::string(m) + " /dav/resource HTTP/1.1\r\nHost: x\r\n\r\n";
		ah::Parser p;
		ah::init(p, ah::Type::REQUEST);
		ah::Status st = ah::Status::OK;
		ah::execute(p, raw.data(), raw.size(), st);
		if (st == ah::Status::COMPLETE) {
			std::printf("  %-12s -> enum: %-12s (исходная строка сохранена: \"%s\")\n",
			            m, ah::methodName(p.message.method), p.message.methodName.c_str());
		} else {
			std::printf("  %-12s -> ОШИБКА %s\n", m, ah::errorName(p.error));
		}
	}
}

// ----- глобальные аккумуляторы для потоковых callback'ов (обычные функции) -----
static uint64_t g_streamBytes = 0;
static uint32_t g_streamHash  = 2166136261u; // FNV-1a seed
static int      g_chunkCount  = 0;

static bool cb_headers_complete(ah::Parser & p, void *) {
	const char * m = (p.message.type == ah::Type::REQUEST)
	                 ? ah::methodName(p.message.method) : "RESPONSE";
	std::printf("  [callback] заголовки получены (%s), тело пойдёт потоком\n", m);
	return true; // продолжаем
}
static bool cb_body(ah::Parser &, void *, const char * data, size_t len) {
	// Zero-copy: data указывает прямо во входной буфер, тело нигде не копится.
	g_streamBytes += len;
	for (size_t i = 0; i < len; ++i) {            // FNV-1a по содержимому
		g_streamHash ^= static_cast<unsigned char>(data[i]);
		g_streamHash *= 16777619u;
	}
	return true;
}
static bool cb_chunk_header(ah::Parser &, void *, uint64_t size) {
	if (size > 0) ++g_chunkCount;
	return true;
}
static bool cb_message_complete(ah::Parser &, void *) {
	std::printf("  [callback] сообщение завершено\n");
	return true;
}

// ============================================================================
//  Пример 11: zero-copy потоковый разбор большого тела (без буферизации)
// ============================================================================
static void demoStreamingZeroCopy() {
	hr("Пример 11: zero-copy стриминг тела 16 МБ (storeBody = false)");
	note("Тело НЕ копится в Message::body; onBody отдаёт срез входного буфера.");

	const size_t bodySize = 16u * 1024 * 1024;
	std::string body(bodySize, '\0');
	for (size_t i = 0; i < bodySize; ++i) body[i] = static_cast<char>('A' + (i % 26));

	const std::string head =
		"POST /upload HTTP/1.1\r\n"
		"Host: anyks.com\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Length: " + std::to_string(bodySize) + "\r\n"
		"\r\n";
	std::string raw = head + body;

	ah::Handler handler{};
	handler.onHeadersComplete = cb_headers_complete;
	handler.onBody            = cb_body;
	handler.onMessageComplete = cb_message_complete;

	ah::Parser p;
	ah::init(p, ah::Type::REQUEST);
	p.handler   = &handler;
	p.storeBody = false; // ключевое: тело не буферизуется

	g_streamBytes = 0;
	g_streamHash  = 2166136261u;

	// Эмуляция чтения из сокета порциями по 64 КБ.
	ah::Status st = ah::Status::OK;
	size_t off = 0;
	const size_t sock = 64 * 1024;
	const auto t0 = std::chrono::steady_clock::now();
	while (off < raw.size()) {
		const size_t n = std::min(sock, raw.size() - off);
		const size_t used = ah::execute(p, raw.data() + off, n, st);
		off += used;
		if (st == ah::Status::ERROR) {
			std::printf("\033[31mОШИБКА: %s\033[0m\n", ah::errorName(p.error));
			return;
		}
		if (st == ah::Status::COMPLETE) break;
	}
	const auto t1 = std::chrono::steady_clock::now();
	const double sec = std::chrono::duration<double>(t1 - t0).count();

	std::printf("Принято тела через onBody : %llu байт\n", (unsigned long long) g_streamBytes);
	std::printf("Хранится в Message::body  : %zu байт (буферизации нет!)\n", p.message.body.size());
	std::printf("FNV-1a хэш тела           : 0x%08X\n", g_streamHash);
	std::printf("Время / скорость          : %.3f c / %.1f МБ/с\n",
	            sec, (double(bodySize) / (1024.0 * 1024.0)) / sec);
}

// ============================================================================
//  Пример 12: потоковый разбор chunked-тела через callback'и
// ============================================================================
static void demoStreamingChunked() {
	hr("Пример 12: потоковый chunked через callback'и (счётчик чанков)");
	const std::string raw =
		"HTTP/1.1 200 OK\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"4\r\nWiki\r\n"
		"5\r\npedia\r\n"
		"E\r\n in\r\n\r\nchunks.\r\n"
		"0\r\n\r\n";

	ah::Handler handler{};
	handler.onChunkHeader     = cb_chunk_header;
	handler.onBody            = cb_body;
	handler.onMessageComplete = cb_message_complete;

	ah::Parser p;
	ah::init(p, ah::Type::RESPONSE);
	p.handler   = &handler;
	p.storeBody = false;

	g_streamBytes = 0;
	g_streamHash  = 2166136261u;
	g_chunkCount  = 0;

	ah::Status st = ah::Status::OK;
	ah::execute(p, raw.data(), raw.size(), st);

	if (st == ah::Status::COMPLETE)
		std::printf("Чанков с данными: %d, всего тела: %llu байт (через onBody)\n",
		            g_chunkCount, (unsigned long long) g_streamBytes);
	else
		std::printf("\033[31mОШИБКА/неполно: %s\033[0m\n", ah::errorName(p.error));
}

int main() {
	std::printf("\033[1mДемонстрация парсера HTTP/1.1 (awh::http)\033[0m\n");

	demoSimpleRequest();
	demoPostContentLength();
	demoChunkedWithTrailers();
	demoIncremental();
	demoPipelining();
	demoSecuritySmuggling();
	demoSecurityBadHeader();
	demoHeadResponse();
	demoExtendedMethods();
	demoStreamingZeroCopy();
	demoStreamingChunked();
	demoBenchmark();

	std::printf("\n\033[1;32mГотово.\033[0m\n");
	return 0;
}
