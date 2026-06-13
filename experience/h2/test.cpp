/**
 * @file test.cpp
 * @brief Round-trip и эталонные тесты для framing-слоя и HPACK.
 *
 * Проверяются:
 *   - целочисленный кодек HPACK на примерах RFC 7541 Appendix C.1;
 *   - Huffman на эталонном векторе RFC 7541 C.4.1 ("www.example.com");
 *   - round-trip Huffman на произвольных строках;
 *   - round-trip HPACK-блока (Encoder -> Decoder);
 *   - round-trip framing (serialize -> parse) для HEADERS / SETTINGS / DATA.
 *
 * Сборка:
 *   g++ -std=c++17 -O2 -Wall -Wextra h2.cpp frame.cpp hpack.cpp session.cpp test.cpp -o h2_test
 *   ./h2_test
 */

#include "frame.hpp"
#include "hpack.hpp"
#include "session.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace awh::http2;

static int g_failed = 0;
static int g_total  = 0;

static void check(bool cond, const char * name) {
	++g_total;
	if(cond) std::printf("  ok   %s\n", name);
	else { std::printf("  FAIL %s\n", name); ++g_failed; }
}

/// Сравнить байтовую строку с массивом ожидаемых байт.
static bool bytesEq(const std::string & got, const uint8_t * exp, size_t n) {
	if(got.size() != n) return false;
	for(size_t i = 0; i < n; ++i) if(static_cast <uint8_t> (got[i]) != exp[i]) return false;
	return true;
}

static void testInteger() {
	std::printf("[HPACK integer codec — RFC 7541 C.1]\n");
	// C.1.1: 10 с 5-битным префиксом => 0x0a
	{
		std::string out; hpack::encodeInteger(out, 10, 5, 0x00);
		const uint8_t exp[] = { 0x0a };
		check(bytesEq(out, exp, 1), "encode 10 (prefix 5)");
		uint64_t v = 0; size_t used = 0;
		const auto st = hpack::decodeInteger(reinterpret_cast <const uint8_t *> (out.data()), out.size(), 5, v, used);
		check((st == status_t::OK) && (v == 10) && (used == 1), "decode 10 (prefix 5)");
	}
	// C.1.2: 1337 с 5-битным префиксом => 0x1f 0x9a 0x0a
	{
		std::string out; hpack::encodeInteger(out, 1337, 5, 0x00);
		const uint8_t exp[] = { 0x1f, 0x9a, 0x0a };
		check(bytesEq(out, exp, 3), "encode 1337 (prefix 5)");
		uint64_t v = 0; size_t used = 0;
		const auto st = hpack::decodeInteger(reinterpret_cast <const uint8_t *> (out.data()), out.size(), 5, v, used);
		check((st == status_t::OK) && (v == 1337) && (used == 3), "decode 1337 (prefix 5)");
	}
	// C.1.3: 42 с 8-битным префиксом => 0x2a
	{
		std::string out; hpack::encodeInteger(out, 42, 8, 0x00);
		const uint8_t exp[] = { 0x2a };
		check(bytesEq(out, exp, 1), "encode 42 (prefix 8)");
		uint64_t v = 0; size_t used = 0;
		const auto st = hpack::decodeInteger(reinterpret_cast <const uint8_t *> (out.data()), out.size(), 8, v, used);
		check((st == status_t::OK) && (v == 42) && (used == 1), "decode 42 (prefix 8)");
	}
	// Недостаток данных => INCOMPLETE
	{
		const uint8_t in[] = { 0x1f, 0x9a }; // продолжение оборвано
		uint64_t v = 0; size_t used = 0;
		const auto st = hpack::decodeInteger(in, sizeof(in), 5, v, used);
		check(st == status_t::INCOMPLETE, "decode truncated => INCOMPLETE");
	}
}

static void testHuffman() {
	std::printf("[HPACK Huffman — RFC 7541 C.4.1]\n");
	// "www.example.com" => f1e3 c2e5 f23a 6ba0 ab90 f4ff
	{
		std::string enc; hpack::huffmanEncode("www.example.com", enc);
		const uint8_t exp[] = { 0xf1,0xe3,0xc2,0xe5,0xf2,0x3a,0x6b,0xa0,0xab,0x90,0xf4,0xff };
		check(bytesEq(enc, exp, sizeof(exp)), "encode \"www.example.com\"");
		check(hpack::huffmanLength("www.example.com") == sizeof(exp), "huffmanLength matches");
		std::string dec;
		check(hpack::huffmanDecode(reinterpret_cast <const uint8_t *> (enc.data()), enc.size(), dec), "decode returns true");
		check(dec == "www.example.com", "decode round-trip value");
	}
	// Декодирование эталонных байт из RFC напрямую.
	{
		const uint8_t in[] = { 0xf1,0xe3,0xc2,0xe5,0xf2,0x3a,0x6b,0xa0,0xab,0x90,0xf4,0xff };
		std::string dec;
		check(hpack::huffmanDecode(in, sizeof(in), dec) && (dec == "www.example.com"), "decode RFC bytes directly");
	}
	// Round-trip на наборе строк (включая все байты 0..255).
	{
		std::vector <std::string> samples = {
			"", "/", "/index.html", "https", "GET", "no-cache",
			"Mozilla/5.0 (X11; Linux) AppleWebKit/537.36",
			"\x00\x01\x02\x7f\x80\xff текст-юникод"
		};
		std::string all; for(int i = 0; i < 256; ++i) all.push_back(static_cast <char> (i));
		samples.push_back(all);
		bool ok = true;
		for(const auto & s : samples){
			std::string enc; hpack::huffmanEncode(s, enc);
			std::string dec;
			if(!hpack::huffmanDecode(reinterpret_cast <const uint8_t *> (enc.data()), enc.size(), dec) || (dec != s)) ok = false;
		}
		check(ok, "round-trip набора строк (вкл. все байты 0..255)");
	}
}

static void testHpackBlock() {
	std::printf("[HPACK block — Encoder -> Decoder round-trip]\n");
	std::vector <hpack::field_t> req = {
		{ ":method",    "GET" },
		{ ":path",      "/index.html" },
		{ ":scheme",    "https" },
		{ ":authority", "example.com" },
		{ "accept",     "*/*" },
		{ "user-agent", "awh-h2/0.1" }
	};
	for(bool huff : { false, true }){
		hpack::Encoder enc;
		std::string block; enc.encode(req, block, huff);
		hpack::Decoder dec;
		std::vector <hpack::field_t> out; error_t err = error_t::NO_ERROR;
		const auto st = dec.decode(block, out, /* maxListSize */ 0, err);
		bool eq = (st == status_t::OK) && (out.size() == req.size());
		for(size_t i = 0; eq && (i < req.size()); ++i)
			eq = (out[i].name == req[i].name) && (out[i].value == req[i].value);
		check(eq, huff ? "round-trip (Huffman)" : "round-trip (literal)");
	}
	// Декодирование одного индексированного поля из статической таблицы: index 2 => :method GET.
	{
		const uint8_t block[] = { 0x82 }; // 1000 0010
		hpack::Decoder dec;
		std::vector <hpack::field_t> out; error_t err = error_t::NO_ERROR;
		const auto st = dec.decode(std::string_view(reinterpret_cast <const char *> (block), 1), out, 0, err);
		check((st == status_t::OK) && (out.size() == 1) && (out[0].name == ":method") && (out[0].value == "GET"),
			"indexed static field (idx 2 => :method GET)");
	}
	// Индексация кодера: полное совпадение в статической таблице => один байт 0x82.
	{
		hpack::Encoder enc;
		std::string block; enc.encode({ { ":method", "GET" } }, block, true);
		check((block.size() == 1) && (static_cast <uint8_t> (block[0]) == 0x82),
			"encoder: :method GET => indexed (1 байт 0x82)");
	}
	// Индексация кодера: повтор пользовательского заголовка во втором блоке => 1 байт (динамическая таблица).
	{
		hpack::Encoder enc;
		const std::vector <hpack::field_t> f = { { "x-custom", "hello-value" } };
		std::string b1; enc.encode(f, b1, true); // literal + incremental indexing
		std::string b2; enc.encode(f, b2, true); // должен стать индексированным
		// Первая запись динамической таблицы => индекс 62 => encodeInteger(62,7,0x80) => 0xBE.
		check((b1.size() > 1) && (b2.size() == 1) && (static_cast <uint8_t> (b2[0]) == 0xBE),
			"encoder: повтор => индекс из динамической таблицы (1 байт)");
	}
	// Синхронность кодера/декодера через несколько блоков с одной парой объектов.
	{
		hpack::Encoder enc; hpack::Decoder dec;
		const std::vector <hpack::field_t> f1 = { { ":method", "POST" }, { "x-trace", "abc123" }, { "content-type", "application/json" } };
		const std::vector <hpack::field_t> f2 = { { ":method", "POST" }, { "x-trace", "abc123" }, { "content-type", "application/json" } };
		bool ok = true;
		for(const auto * f : { &f1, &f2 }){
			std::string block; enc.encode(*f, block, true);
			std::vector <hpack::field_t> out; error_t err = error_t::NO_ERROR;
			ok = ok && (dec.decode(block, out, 0, err) == status_t::OK) && (out.size() == f->size());
			for(size_t i = 0; ok && (i < f->size()); ++i)
				ok = (out[i].name == (*f)[i].name) && (out[i].value == (*f)[i].value);
		}
		check(ok, "кодер/декодер синхронны через 2 блока (динамическая таблица)");
	}
}

static void testFraming() {
	std::printf("[Framing — serialize -> parse round-trip]\n");
	// HEADERS
	{
		const std::string block = "\x82\x86\x84"; // произвольный псевдо-блок
		std::string buf; frame::serializeHeaders(buf, 1, block, /* endStream */ true, /* endHeaders */ true);
		frame::header_t h;
		check(frame::parseHeader(reinterpret_cast <const uint8_t *> (buf.data()), buf.size(), h), "parseHeader");
		check((h.type == frame_t::HEADERS) && (h.streamId == 1) && (h.length == block.size()), "HEADERS header fields");
		frame::headers_t hd; error_t err = error_t::NO_ERROR;
		const auto st = frame::parseHeaders(h, reinterpret_cast <const uint8_t *> (buf.data()) + 9, hd, err);
		check((st == status_t::OK) && hd.endStream && hd.endHeaders && (hd.block == block), "HEADERS payload");
	}
	// SETTINGS
	{
		const frame::setting_entry_t items[] = {
			{ setting_t::MAX_CONCURRENT_STREAMS, 100 },
			{ setting_t::INITIAL_WINDOW_SIZE, 65535 }
		};
		std::string buf; frame::serializeSettings(buf, items, 2, false);
		frame::header_t h; frame::parseHeader(reinterpret_cast <const uint8_t *> (buf.data()), buf.size(), h);
		std::vector <frame::setting_entry_t> out; error_t err = error_t::NO_ERROR;
		const auto st = frame::parseSettings(h, reinterpret_cast <const uint8_t *> (buf.data()) + 9, out, err);
		bool eq = (st == status_t::OK) && (out.size() == 2)
			&& (out[0].id == setting_t::MAX_CONCURRENT_STREAMS) && (out[0].value == 100)
			&& (out[1].id == setting_t::INITIAL_WINDOW_SIZE) && (out[1].value == 65535);
		check(eq, "SETTINGS round-trip");
	}
	// DATA с END_STREAM
	{
		const std::string payload = "hello body";
		std::string buf; frame::serializeData(buf, 3, payload, /* endStream */ true);
		frame::header_t h; frame::parseHeader(reinterpret_cast <const uint8_t *> (buf.data()), buf.size(), h);
		frame::data_t d; error_t err = error_t::NO_ERROR;
		const auto st = frame::parseData(h, reinterpret_cast <const uint8_t *> (buf.data()) + 9, d, err);
		check((st == status_t::OK) && d.endStream && (d.data == payload), "DATA round-trip");
	}
	// WINDOW_UPDATE с нулевым инкрементом => PROTOCOL_ERROR
	{
		std::string buf; frame::serializeWindowUpdate(buf, 0, 0);
		frame::header_t h; frame::parseHeader(reinterpret_cast <const uint8_t *> (buf.data()), buf.size(), h);
		uint32_t inc = 0; error_t err = error_t::NO_ERROR;
		const auto st = frame::parseWindowUpdate(h, reinterpret_cast <const uint8_t *> (buf.data()) + 9, inc, err);
		check((st == status_t::ERROR) && (err == error_t::PROTOCOL_ERROR), "WINDOW_UPDATE zero => PROTOCOL_ERROR");
	}
	// HEADERS + CONTINUATION при отправке большого блока (нарезка по maxFramePayload).
	{
		const std::string big(50000, 'x');
		std::string buf;
		frame::serializeHeaderBlock(buf, 1, big, /* endStream */ false, /* maxFramePayload */ 16384);
		size_t off = 0;
		int headers = 0, cont = 0;
		std::string reassembled;
		while(off + 9 <= buf.size()){
			frame::header_t h;
			if(!frame::parseHeader(reinterpret_cast <const uint8_t *> (buf.data()) + off, buf.size() - off, h)) break;
			const uint8_t * pay = reinterpret_cast <const uint8_t *> (buf.data()) + off + 9;
			if(h.type == frame_t::HEADERS){
				headers++;
				frame::headers_t hd; error_t err = error_t::NO_ERROR;
				check(frame::parseHeaders(h, pay, hd, err) == status_t::OK, "split HEADERS parse");
				reassembled.append(hd.block.data(), hd.block.size());
			} else if(h.type == frame_t::CONTINUATION){
				cont++;
				std::string_view frag; bool endH = false; error_t err = error_t::NO_ERROR;
				check(frame::parseContinuation(h, pay, frag, endH, err) == status_t::OK, "split CONTINUATION parse");
				reassembled.append(frag.data(), frag.size());
			} else break;
			off += 9 + h.length;
		}
		check((headers == 1) && (cont == 3) && (reassembled == big), "serializeHeaderBlock => HEADERS+CONTINUATION, payload восстановлен");
	}
	// PUSH_PROMISE + CONTINUATION при большом блоке (4 октета под promised id в первом фрейме).
	{
		const std::string big(40000, 'y');
		std::string buf;
		frame::serializePushPromiseBlock(buf, 1, 2, big, 16384);
		size_t off = 0;
		int push = 0, cont = 0;
		std::string reassembled;
		while(off + 9 <= buf.size()){
			frame::header_t h;
			if(!frame::parseHeader(reinterpret_cast <const uint8_t *> (buf.data()) + off, buf.size() - off, h)) break;
			const uint8_t * pay = reinterpret_cast <const uint8_t *> (buf.data()) + off + 9;
			if(h.type == frame_t::PUSH_PROMISE){
				push++;
				frame::push_promise_t pp; error_t err = error_t::NO_ERROR;
				check(frame::parsePushPromise(h, pay, pp, err) == status_t::OK, "split PUSH_PROMISE parse");
				check(pp.promisedStreamId == 2, "split PUSH_PROMISE promised id");
				reassembled.append(pp.block.data(), pp.block.size());
			} else if(h.type == frame_t::CONTINUATION){
				cont++;
				std::string_view frag; bool endH = false; error_t err = error_t::NO_ERROR;
				check(frame::parseContinuation(h, pay, frag, endH, err) == status_t::OK, "split PUSH CONTINUATION parse");
				reassembled.append(frag.data(), frag.size());
			} else break;
			off += 9 + h.length;
		}
		check((push == 1) && (cont == 2) && (reassembled == big), "serializePushPromiseBlock => PUSH_PROMISE+CONTINUATION");
	}
}

// ───────────────────── Конечный автомат состояний потоков ─────────────────────

namespace {
	struct sm_ctx_t {
		int     begins = 0;
		int     headers = 0;            // число вызовов onHeader
		int     headersComplete = 0;
		bool    lastEndStream = false;
		int     closes = 0;
		error_t lastClose = error_t::NO_ERROR;
		bool    errored = false;
		error_t err = error_t::NO_ERROR;
	};
	void smBegin(void * u, uint32_t) { static_cast <sm_ctx_t *> (u)->begins++; }
	void smHeader(void * u, uint32_t, std::string_view, std::string_view) { static_cast <sm_ctx_t *> (u)->headers++; }
	void smHeadersComplete(void * u, uint32_t, bool es) { auto * c = static_cast <sm_ctx_t *> (u); c->headersComplete++; c->lastEndStream = es; }
	void smClose(void * u, uint32_t, error_t code) { auto * c = static_cast <sm_ctx_t *> (u); c->closes++; c->lastClose = code; }
	void smError(void * u, error_t e, const char *) { auto * c = static_cast <sm_ctx_t *> (u); c->errored = true; c->err = e; }

	/// Закодировать произвольный набор заголовков в HEADERS-фрейм потока.
	void appendFields(std::string & buf, uint32_t streamId, const std::vector <hpack::field_t> & f, bool endStream) {
		hpack::Encoder enc;
		std::string block; enc.encode(f, block, false);
		frame::serializeHeaders(buf, streamId, block, endStream, true);
	}

	/// Прогнать серверную сессию на запросе из заданных заголовков.
	status_t runRequest(const std::vector <hpack::field_t> & f, sm_ctx_t & c) {
		callbacks_t cb;
		cb.onStreamBegin = smBegin; cb.onHeader = smHeader; cb.onHeadersComplete = smHeadersComplete;
		cb.onStreamClose = smClose; cb.onError = smError;
		Session s(endpoint_t::SERVER, cb, &c);
		s.submitPreface();
		std::string in(proto::PREFACE); appendFields(in, 1, f, true);
		return s.feed(reinterpret_cast <const uint8_t *> (in.data()), in.size());
	}

	/// Сформировать клиентский HEADERS-фрейм с минимальным валидным блоком.
	void appendRequest(std::string & buf, uint32_t streamId, bool endStream) {
		hpack::Encoder enc;
		std::vector <hpack::field_t> f = {
			{ ":method", "GET" }, { ":path", "/" }, { ":scheme", "https" }, { ":authority", "x" }
		};
		std::string block; enc.encode(f, block, false);
		frame::serializeHeaders(buf, streamId, block, endStream, true);
	}

	/// Прогнать серверную сессию на готовом входном буфере.
	status_t runServer(const std::string & input, sm_ctx_t & ctx) {
		callbacks_t cb;
		cb.onStreamBegin     = smBegin;
		cb.onHeadersComplete = smHeadersComplete;
		cb.onStreamClose     = smClose;
		cb.onError           = smError;
		Session s(endpoint_t::SERVER, cb, &ctx);
		s.submitPreface();
		return s.feed(reinterpret_cast <const uint8_t *> (input.data()), input.size());
	}
}

static void testStateMachine() {
	std::printf("[Stream state machine — RFC 9113 §5.1]\n");
	const std::string PREFACE(proto::PREFACE);

	// 1. Happy path: HEADERS со стрима 1 с END_STREAM.
	{
		std::string in = PREFACE; appendRequest(in, 1, true);
		sm_ctx_t c; const auto st = runServer(in, c);
		check((st == status_t::OK) && (c.begins == 1) && (c.headersComplete == 1) && c.lastEndStream && !c.errored,
			"idle -> open -> half-closed(remote) (END_STREAM)");
	}
	// 2. DATA на ещё не открытом (idle) потоке => PROTOCOL_ERROR.
	{
		std::string in = PREFACE; frame::serializeData(in, 5, "x", false);
		sm_ctx_t c; const auto st = runServer(in, c);
		check((st == status_t::ERROR) && c.errored && (c.err == error_t::PROTOCOL_ERROR), "DATA on idle stream => PROTOCOL_ERROR");
	}
	// 3. Чётный stream id от клиента => PROTOCOL_ERROR.
	{
		std::string in = PREFACE; appendRequest(in, 2, true);
		sm_ctx_t c; const auto st = runServer(in, c);
		check((st == status_t::ERROR) && (c.err == error_t::PROTOCOL_ERROR), "even client stream id => PROTOCOL_ERROR");
	}
	// 4. Немонотонный id: после потока 3 открыть поток 1 => PROTOCOL_ERROR.
	{
		std::string in = PREFACE; appendRequest(in, 3, true); appendRequest(in, 1, true);
		sm_ctx_t c; const auto st = runServer(in, c);
		check((st == status_t::ERROR) && (c.err == error_t::PROTOCOL_ERROR) && (c.begins == 1),
			"non-monotonic stream id => PROTOCOL_ERROR");
	}
	// 5. DATA после END_STREAM (поток half-closed remote) => STREAM_CLOSED.
	{
		std::string in = PREFACE; appendRequest(in, 1, true); frame::serializeData(in, 1, "x", false);
		sm_ctx_t c; const auto st = runServer(in, c);
		check((st == status_t::ERROR) && (c.err == error_t::STREAM_CLOSED), "DATA after END_STREAM => STREAM_CLOSED");
	}
}

// ───────────────────── Flow control (этап 4) ─────────────────────

namespace {
	struct data_scan_t { size_t bytes = 0; bool endStream = false; int frames = 0; };

	/// Просканировать буфер и собрать статистику DATA-фреймов указанного потока.
	data_scan_t scanData(std::string_view buf, uint32_t streamId) {
		data_scan_t r;
		size_t pos = 0;
		while((buf.size() - pos) >= proto::FRAME_HEADER_SIZE){
			frame::header_t h;
			frame::parseHeader(reinterpret_cast <const uint8_t *> (buf.data()) + pos, buf.size() - pos, h);
			if((buf.size() - pos) < (proto::FRAME_HEADER_SIZE + h.length)) break;
			if((h.type == frame_t::DATA) && (h.streamId == streamId)){
				r.bytes += h.length; r.frames++;
				if(h.flags & flag::END_STREAM) r.endStream = true;
			}
			pos += proto::FRAME_HEADER_SIZE + h.length;
		}
		return r;
	}
}

static void testFlowControl() {
	std::printf("[Flow control — RFC 9113 §6.9]\n");
	const std::string PREFACE(proto::PREFACE);

	// Главный сценарий: окно=100, тело=300 -> уходит 100, остаток оседает,
	// после WINDOW_UPDATE(+200) автоматически дослыается с END_STREAM.
	{
		std::string in = PREFACE;
		const frame::setting_entry_t cs[] = { { setting_t::INITIAL_WINDOW_SIZE, 100 } };
		frame::serializeSettings(in, cs, 1, false);
		appendRequest(in, 1, true); // клиент завершил свою половину

		sm_ctx_t c; callbacks_t cb; cb.onStreamClose = smClose; cb.onError = smError;
		Session srv(endpoint_t::SERVER, cb, &c);
		srv.submitPreface();
		check(srv.feed(reinterpret_cast <const uint8_t *> (in.data()), in.size()) == status_t::OK, "feed preface+SETTINGS+HEADERS");

		const std::string body(300, 'A');
		const size_t taken = srv.submitData(1, body.data(), body.size(), true);
		check(taken == 300, "submitData принял всё тело (буфер большой)");

		const data_scan_t d1 = scanData(srv.pending(), 1);
		check((d1.bytes == 100) && !d1.endStream, "ушло только 100 байт (окно=100), поток не завершён");
		check(c.closes == 0, "поток ещё открыт");

		srv.consumePending(srv.pending().size()); // эмулируем слив в сокет

		std::string wu; frame::serializeWindowUpdate(wu, 1, 200);
		srv.feed(reinterpret_cast <const uint8_t *> (wu.data()), wu.size());
		const data_scan_t d2 = scanData(srv.pending(), 1);
		check((d2.bytes == 200) && d2.endStream, "после WINDOW_UPDATE дослано 200 байт с END_STREAM");
		check(c.closes == 1, "поток закрыт после финального DATA");
	}

	// Backpressure: маленький буфер => частичный приём (возврат < len).
	{
		std::string in = PREFACE; appendRequest(in, 1, false); // поток остаётся открытым
		sm_ctx_t c; callbacks_t cb; cb.onError = smError;
		Session srv(endpoint_t::SERVER, cb, &c);
		srv.submitPreface();
		srv.feed(reinterpret_cast <const uint8_t *> (in.data()), in.size());
		srv.setOutputHighWater(80); // ограничиваем и выходной буфер, чтобы данные осели
		srv.setSendWaterMarks(50, 10);
		const std::string body(300, 'B');
		const size_t taken = srv.submitData(1, body.data(), body.size(), false);
		check(taken == 50, "bounded buffer принял только high-water (50)");
	}
}

// ───────────────────── DoS-защиты (этап 5) ─────────────────────

static void testSecurity() {
	std::printf("[DoS-защиты — этап 5]\n");
	const std::string PREFACE(proto::PREFACE);

	auto feed = [](Session & s, const std::string & b) {
		return s.feed(reinterpret_cast <const uint8_t *> (b.data()), b.size());
	};

	// Rapid Reset (CVE-2023-44487): поток HEADERS+RST много раз => ENHANCE_YOUR_CALM.
	{
		sm_ctx_t c; callbacks_t cb; cb.onError = smError; cb.onStreamClose = smClose;
		Session srv(endpoint_t::SERVER, cb, &c); srv.submitPreface();
		srv.setResetRateLimit(3, 0); // burst 3, без пополнения
		std::string in = PREFACE;
		for(uint32_t id : { 1u, 3u, 5u, 7u }){ appendRequest(in, id, false); frame::serializeRstStream(in, id, error_t::CANCEL); }
		const auto st = feed(srv, in);
		check((st == status_t::ERROR) && (c.err == error_t::ENHANCE_YOUR_CALM), "Rapid Reset: RST_STREAM flood => ENHANCE_YOUR_CALM");
	}

	// CONTINUATION flood (2024) по числу фреймов.
	{
		sm_ctx_t c; callbacks_t cb; cb.onError = smError;
		Session srv(endpoint_t::SERVER, cb, &c); srv.submitPreface();
		srv.setHeaderBlockLimits(1u << 20, 3); // максимум 3 фрейма в блоке
		const std::string frag(2, '\0');
		std::string in = PREFACE;
		frame::serializeHeaders(in, 1, frag, false, false);    // фрейм 1
		frame::serializeContinuation(in, 1, frag, false);      // 2
		frame::serializeContinuation(in, 1, frag, false);      // 3
		frame::serializeContinuation(in, 1, frag, false);      // 4 > 3
		const auto st = feed(srv, in);
		check((st == status_t::ERROR) && (c.err == error_t::ENHANCE_YOUR_CALM), "CONTINUATION flood (число фреймов) => ENHANCE_YOUR_CALM");
	}

	// CONTINUATION flood по суммарному размеру блока заголовков.
	{
		sm_ctx_t c; callbacks_t cb; cb.onError = smError;
		Session srv(endpoint_t::SERVER, cb, &c); srv.submitPreface();
		srv.setHeaderBlockLimits(100, 1000); // максимум 100 байт
		const std::string big(200, '\0');
		std::string in = PREFACE;
		frame::serializeHeaders(in, 1, big, false, false);
		const auto st = feed(srv, in);
		check((st == status_t::ERROR) && (c.err == error_t::ENHANCE_YOUR_CALM), "header block too large => ENHANCE_YOUR_CALM");
	}

	// SETTINGS flood.
	{
		sm_ctx_t c; callbacks_t cb; cb.onError = smError;
		Session srv(endpoint_t::SERVER, cb, &c); srv.submitPreface();
		srv.setControlRateLimit(2, 0); // burst 2
		std::string in = PREFACE;
		frame::serializeSettings(in, nullptr, 0, false);
		frame::serializeSettings(in, nullptr, 0, false);
		frame::serializeSettings(in, nullptr, 0, false); // 3-й => превышение
		const auto st = feed(srv, in);
		check((st == status_t::ERROR) && (c.err == error_t::ENHANCE_YOUR_CALM), "SETTINGS flood => ENHANCE_YOUR_CALM");
	}

	// Пополнение токенов со временем (updateTime): через паузу снова можно.
	{
		sm_ctx_t c; callbacks_t cb; cb.onError = smError; cb.onStreamClose = smClose;
		Session srv(endpoint_t::SERVER, cb, &c); srv.submitPreface();
		srv.setResetRateLimit(1, 1); // burst 1, пополнение 1/с
		srv.updateTime(0);
		std::string a = PREFACE; appendRequest(a, 1, false); frame::serializeRstStream(a, 1, error_t::CANCEL);
		check(feed(srv, a) == status_t::OK, "первый RST в пределах burst");
		srv.updateTime(10); // спустя 10 секунд токены восстановились
		std::string b; appendRequest(b, 3, false); frame::serializeRstStream(b, 3, error_t::CANCEL);
		check(feed(srv, b) == status_t::OK, "после паузы (updateTime) RST снова разрешён");
	}
}

// ───────────────────── HTTP-семантика (этап 6) ─────────────────────

static void testHttpSemantics() {
	std::printf("[HTTP-семантика — RFC 9113 §8]\n");
	const std::vector <hpack::field_t> valid = {
		{ ":method", "GET" }, { ":scheme", "https" }, { ":path", "/" }, { ":authority", "x" }
	};

	// Корректный запрос — доставляется приложению, поток не сброшен.
	{
		sm_ctx_t c; const auto st = runRequest(valid, c);
		check((st == status_t::OK) && (c.headers == 4) && (c.headersComplete == 1) && (c.closes == 0),
			"валидный запрос доставлен (4 заголовка)");
	}
	// Отсутствует обязательный :path => malformed => RST_STREAM (соединение живёт).
	{
		sm_ctx_t c; const auto st = runRequest({ { ":method", "GET" }, { ":scheme", "https" }, { ":authority", "x" } }, c);
		check((st == status_t::OK) && (c.headers == 0) && (c.closes == 1) && (c.lastClose == error_t::PROTOCOL_ERROR) && !c.errored,
			"нет :path => RST_STREAM (PROTOCOL_ERROR), соединение живёт");
	}
	// Псевдо-заголовок после обычного => malformed.
	{
		sm_ctx_t c; const auto st = runRequest({ { ":method", "GET" }, { ":scheme", "https" }, { ":path", "/" }, { "x", "y" }, { ":authority", "z" } }, c);
		check((st == status_t::OK) && (c.headers == 0) && (c.lastClose == error_t::PROTOCOL_ERROR), "псевдо-заголовок после обычного => malformed");
	}
	// Имя заголовка в верхнем регистре => malformed.
	{
		auto f = valid; f.push_back({ "Foo", "bar" });
		sm_ctx_t c; const auto st = runRequest(f, c);
		check((st == status_t::OK) && (c.lastClose == error_t::PROTOCOL_ERROR), "имя в верхнем регистре => malformed");
	}
	// Connection-specific заголовок => malformed.
	{
		auto f = valid; f.push_back({ "connection", "keep-alive" });
		sm_ctx_t c; const auto st = runRequest(f, c);
		check((st == status_t::OK) && (c.lastClose == error_t::PROTOCOL_ERROR), "connection-specific заголовок => malformed");
	}
	// TE: gzip запрещён, TE: trailers разрешён.
	{
		auto bad = valid; bad.push_back({ "te", "gzip" });
		sm_ctx_t c1; runRequest(bad, c1);
		check(c1.lastClose == error_t::PROTOCOL_ERROR, "TE: gzip => malformed");
		auto good = valid; good.push_back({ "te", "trailers" });
		sm_ctx_t c2; runRequest(good, c2);
		check((c2.headers == 5) && (c2.closes == 0), "TE: trailers => допустим");
	}
	// Дубликат :method => malformed.
	{
		sm_ctx_t c; const auto st = runRequest({ { ":method", "GET" }, { ":method", "POST" }, { ":scheme", "https" }, { ":path", "/" } }, c);
		check((st == status_t::OK) && (c.lastClose == error_t::PROTOCOL_ERROR), "дубликат :method => malformed");
	}
	// CONNECT: только :authority, без :scheme/:path.
	{
		sm_ctx_t c; const auto st = runRequest({ { ":method", "CONNECT" }, { ":authority", "example.com:443" } }, c);
		check((st == status_t::OK) && (c.headers == 2) && (c.closes == 0), "CONNECT с :authority без :scheme/:path => допустим");
	}
}

// ───────────────────── Server push (PUSH_PROMISE) ─────────────────────

namespace {
	struct push_ctx_t {
		int      pushPromises = 0;
		uint32_t lastAssoc = 0, lastPromised = 0;
		std::string promisedPath;  // :path обещанного запроса (приходит на push-потоке)
		std::string status;        // :status ответа сервера (тоже на push-потоке)
		std::string body;          // тело ответа push-потока
		int      closes = 0;
		uint32_t closedStream = 0;
		error_t  lastClose = error_t::NO_ERROR;
		bool     errored = false;
	};
	void pcPush(void * u, uint32_t assoc, uint32_t promised) {
		auto * c = static_cast <push_ctx_t *> (u);
		c->pushPromises++; c->lastAssoc = assoc; c->lastPromised = promised;
	}
	void pcHeader(void * u, uint32_t, std::string_view n, std::string_view v) {
		auto * c = static_cast <push_ctx_t *> (u);
		if(n == ":path") c->promisedPath.assign(v);
		else if(n == ":status") c->status.assign(v);
	}
	void pcData(void * u, uint32_t, const uint8_t * d, size_t len, bool) {
		static_cast <push_ctx_t *> (u)->body.append(reinterpret_cast <const char *> (d), len);
	}
	void pcClose(void * u, uint32_t id, error_t code) {
		auto * c = static_cast <push_ctx_t *> (u);
		c->closes++; c->closedStream = id; c->lastClose = code;
	}
	void pcError(void * u, error_t, const char *) { static_cast <push_ctx_t *> (u)->errored = true; }

	/// Прокачать накопленные исходящие байты между двумя сессиями до сходимости.
	void exchange(Session & a, Session & b) {
		for(int i = 0; i < 10; ++i){
			bool moved = false;
			if(!a.pending().empty()){
				const std::string s{a.pending()};
				b.feed(reinterpret_cast <const uint8_t *> (s.data()), s.size());
				a.consumePending(s.size());
				moved = true;
			}
			if(!b.pending().empty()){
				const std::string s{b.pending()};
				a.feed(reinterpret_cast <const uint8_t *> (s.data()), s.size());
				b.consumePending(s.size());
				moved = true;
			}
			if(!moved) break;
		}
	}
}

static void testOutgoingHeaderBlock() {
	std::printf("[Исходящий HEADERS+CONTINUATION — submitHeaders]\n");
	struct big_ctx_t {
		int      headerCount = 0;
		std::string bigValue;
		bool     errored = false;
	};
	auto onHeader = [](void * u, uint32_t, std::string_view n, std::string_view v) {
		auto * c = static_cast <big_ctx_t *> (u);
		c->headerCount++;
		if(n == "x-big") c->bigValue.assign(v);
	};
	auto onError = [](void * u, error_t, const char *) { static_cast <big_ctx_t *> (u)->errored = true; };

	const std::string bigVal(50000, 'z');
	const std::vector <hpack::field_t> bigReq = {
		{ ":method", "GET" }, { ":scheme", "https" }, { ":path", "/big" }, { ":authority", "example.com" },
		{ "x-big", bigVal }
	};

	big_ctx_t srvCtx;
	callbacks_t scb;
	scb.onHeader = onHeader;
	scb.onError  = onError;
	Session client(endpoint_t::CLIENT, callbacks_t{}, nullptr);
	Session server(endpoint_t::SERVER, scb, &srvCtx);
	client.submitPreface();
	server.submitPreface();
	client.submitHeaders(1, bigReq, true);
	exchange(client, server);

	check((srvCtx.headerCount == 5) && (srvCtx.bigValue.size() == 50000) && (srvCtx.bigValue == bigVal) && !srvCtx.errored,
		"клиент отправил большой блок => сервер получил все заголовки через CONTINUATION");
}

static void testServerPush() {
	std::printf("[Server push — RFC 9113 §6.6/§8.4]\n");
	const std::vector <hpack::field_t> req = {
		{ ":method", "GET" }, { ":scheme", "https" }, { ":path", "/" }, { ":authority", "example.com" }
	};
	const std::vector <hpack::field_t> promisedReq = {
		{ ":method", "GET" }, { ":scheme", "https" }, { ":path", "/style.css" }, { ":authority", "example.com" }
	};
	const std::vector <hpack::field_t> resp = { { ":status", "200" } };

	// Happy path: сервер пушит ресурс, клиент получает PUSH_PROMISE + ответ + тело.
	{
		push_ctx_t cc;
		callbacks_t ccb;
		ccb.onPushPromise = pcPush; ccb.onHeader = pcHeader; ccb.onData = pcData;
		ccb.onStreamClose = pcClose; ccb.onError = pcError;
		callbacks_t scb; // серверу callbacks не важны в этом тесте
		Session client(endpoint_t::CLIENT, ccb, &cc);
		Session server(endpoint_t::SERVER, scb, nullptr);
		client.submitPreface(); server.submitPreface();
		client.submitHeaders(1, req, true);
		exchange(client, server);

		const uint32_t pid = server.submitPushPromise(1, promisedReq);
		check(pid == 2, "submitPushPromise => чётный push-поток (id=2)");
		server.submitHeaders(pid, resp, false);
		server.submitData(pid, "body!", 5, true);
		exchange(client, server);

		check((cc.pushPromises == 1) && (cc.lastAssoc == 1) && (cc.lastPromised == 2),
			"клиент получил onPushPromise(assoc=1, promised=2)");
		check(cc.promisedPath == "/style.css", "клиент видит :path обещанного запроса");
		check(cc.status == "200", "клиент получил :status ответа push-потока");
		check(cc.body == "body!", "клиент получил тело push-потока");
		check((cc.closes == 1) && (cc.closedStream == 2) && (cc.lastClose == error_t::NO_ERROR) && !cc.errored,
			"push-поток закрыт штатно после END_STREAM");
	}

	// Клиент запретил push (SETTINGS_ENABLE_PUSH=0) => сервер не может пушить.
	{
		push_ctx_t cc; callbacks_t ccb; ccb.onError = pcError;
		Session client(endpoint_t::CLIENT, ccb, &cc);
		Session server(endpoint_t::SERVER, callbacks_t{}, nullptr);
		client.submitPreface();
		settings_t cs; cs.enablePush = 0; client.submitSettings(cs); // объявляем «push не нужен»
		server.submitPreface();
		client.submitHeaders(1, req, true);
		exchange(client, server);
		const uint32_t pid = server.submitPushPromise(1, promisedReq);
		check(pid == 0, "ENABLE_PUSH=0 => submitPushPromise отказывает (0)");
	}

	// Малформированный обещанный запрос (без :path) => клиент сбрасывает push-поток.
	{
		push_ctx_t cc;
		callbacks_t ccb;
		ccb.onPushPromise = pcPush; ccb.onHeader = pcHeader; ccb.onStreamClose = pcClose; ccb.onError = pcError;
		Session client(endpoint_t::CLIENT, ccb, &cc);
		Session server(endpoint_t::SERVER, callbacks_t{}, nullptr);
		client.submitPreface(); server.submitPreface();
		client.submitHeaders(1, req, true);
		exchange(client, server);
		server.submitPushPromise(1, { { ":method", "GET" }, { ":scheme", "https" }, { ":authority", "example.com" } });
		exchange(client, server);
		check((cc.pushPromises == 0) && (cc.closes == 1) && (cc.closedStream == 2) && (cc.lastClose == error_t::PROTOCOL_ERROR) && !cc.errored,
			"обещанный запрос без :path => RST push-потока, соединение живёт");
	}
}

int main() {
	testInteger();
	testHuffman();
	testHpackBlock();
	testFraming();
	testStateMachine();
	testFlowControl();
	testSecurity();
	testHttpSemantics();
	testOutgoingHeaderBlock();
	testServerPush();
	std::printf("\n%d/%d проверок пройдено\n", g_total - g_failed, g_total);
	return (g_failed == 0) ? 0 : 1;
}
