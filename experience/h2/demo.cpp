/**
 * @file demo.cpp
 * @brief Демонстрация: сервер принимает клиентский preface + SETTINGS + HEADERS.
 *
 * Показывает связку framing + HPACK + session на сквозном примере. Блок заголовков
 * кодируется нашим же HPACK-кодером (без Huffman), поэтому наш декодер его читает.
 *
 * Сборка:
 *   g++ -std=c++17 -O2 -Wall -Wextra h2.cpp frame.cpp hpack.cpp session.cpp demo.cpp -o h2_demo
 *   ./h2_demo
 */

#include "session.hpp"

#include <cstdio>
#include <string>

using namespace awh::http2;

static void onStreamBegin(void *, uint32_t id) {
	std::printf("[stream %u] открыт\n", id);
}
static void onHeader(void *, uint32_t id, std::string_view name, std::string_view value) {
	std::printf("[stream %u] %.*s: %.*s\n", id,
		static_cast <int> (name.size()), name.data(),
		static_cast <int> (value.size()), value.data());
}
static void onHeadersComplete(void *, uint32_t id, bool endStream) {
	std::printf("[stream %u] заголовки завершены (endStream=%s)\n", id, endStream ? "true" : "false");
}
static void onSettings(void *) {
	std::printf("[conn] SETTINGS пира применены, отправлен ACK\n");
}
static void onError(void *, error_t code, const char * msg) {
	std::printf("[conn] ОШИБКА %s: %s\n", errorName(code), msg);
}

int main() {
	// 1. Готовим входящий поток так, как его прислал бы клиент.
	std::string input;
	input.append(proto::PREFACE.data(), proto::PREFACE.size());

	// Клиентский SETTINGS.
	const frame::setting_entry_t clientSettings[] = {
		{ setting_t::MAX_CONCURRENT_STREAMS, 100 },
		{ setting_t::INITIAL_WINDOW_SIZE,    65535 }
	};
	frame::serializeSettings(input, clientSettings, 2, false);

	// HEADERS для потока 1 (GET https://example.com/).
	hpack::Encoder enc;
	std::vector <hpack::field_t> req = {
		{ ":method",    "GET" },
		{ ":path",      "/" },
		{ ":scheme",    "https" },
		{ ":authority", "example.com" },
		{ "user-agent", "awh-h2/0.1" }
	};
	std::string block;
	enc.encode(req, block, /* useHuffman */ false);
	frame::serializeHeaders(input, /* streamId */ 1, block, /* endStream */ true, /* endHeaders */ true);

	// 2. Поднимаем серверную сессию.
	callbacks_t cb;
	cb.onStreamBegin     = onStreamBegin;
	cb.onHeader          = onHeader;
	cb.onHeadersComplete = onHeadersComplete;
	cb.onSettings        = onSettings;
	cb.onError           = onError;

	Session server(endpoint_t::SERVER, cb, nullptr);
	server.submitPreface(); // сервер ставит в очередь свой SETTINGS

	// 3. Скармливаем входящие байты (можно по кускам — буферизация внутри).
	const status_t st = server.feed(reinterpret_cast <const uint8_t *> (input.data()), input.size());
	std::printf("\nfeed() => %s\n", (st == status_t::OK) ? "OK" : "ERROR");
	std::printf("Сервер сформировал %zu байт для отправки клиенту (SETTINGS + ACK).\n",
		server.pending().size());
	return 0;
}
