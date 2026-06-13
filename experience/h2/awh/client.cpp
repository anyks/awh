/**
 * @file client.cpp
 * @brief HTTP/2-клиент поверх TCP-транспорта AWH (прямое знание, без TLS/ALPN).
 *
 * Парный к server.cpp пример: подключается к 127.0.0.1:2222, отправляет один
 * GET-запрос по HTTP/2 (preface + SETTINGS + HEADERS) и печатает ответ.
 *   - на событие "connect" формируется и отправляется запрос;
 *   - входящие байты из "read" скармливаются в Session::feed();
 *   - ответные заголовки и тело выводятся через callbacks;
 *   - по закрытию потока соединение останавливается.
 *
 * Запуск: ./h2-awh-client  (после старта ./h2-awh-server)
 */

#include <client/client.hpp>

#include "../session.hpp"

#include <memory>
#include <string>

using namespace awh;
using namespace awh::http2;
using namespace placeholders;

namespace {
	/**
	 * @brief Состояние клиентского соединения.
	 */
	struct state_t {
		Session       session;
		client_t *    client;
		const log_t * log;
		bool          finished = false;
		state_t(client_t * c, const log_t * l, const callbacks_t & cb) noexcept
		 : session(endpoint_t::CLIENT, cb, this), client(c), log(l) {}
	};

	/// Отправить накопленные сессией байты в сокет.
	void flush(state_t & s) noexcept {
		const std::string_view out = s.session.pending();
		if(!out.empty()){
			s.client->send(out.data(), out.size());
			s.session.consumePending(out.size());
		}
	}

	// ───────── callbacks http2::Session (user == state_t*) ─────────

	void onHeader(void * user, uint32_t streamId, std::string_view name, std::string_view value) noexcept {
		state_t * s = static_cast <state_t *> (user);
		s->log->print("[h2] <- stream %u  %.*s: %.*s", log_t::flag_t::INFO,
			streamId, static_cast <int> (name.size()), name.data(), static_cast <int> (value.size()), value.data());
	}
	void onData(void * user, uint32_t streamId, const uint8_t * data, size_t len, bool endStream) noexcept {
		state_t * s = static_cast <state_t *> (user);
		s->log->print("[h2] <- stream %u  body[%zu]: %.*s", log_t::flag_t::INFO,
			streamId, len, static_cast <int> (len), reinterpret_cast <const char *> (data));
		(void) endStream;
	}
	void onStreamClose(void * user, uint32_t streamId, error_t code) noexcept {
		state_t * s = static_cast <state_t *> (user);
		s->log->print("[h2] stream %u closed (code=%u)", log_t::flag_t::INFO, streamId, static_cast <uint32_t> (code));
		s->finished = true;
	}
	void onError(void * user, error_t code, const char * message) noexcept {
		state_t * s = static_cast <state_t *> (user);
		s->log->print("[h2] connection error %u: %s", log_t::flag_t::WARNING, static_cast <uint32_t> (code), message);
		s->finished = true;
	}

	callbacks_t makeCallbacks() noexcept {
		callbacks_t cb;
		cb.onHeader      = onHeader;
		cb.onData        = onData;
		cb.onStreamClose = onStreamClose;
		cb.onError       = onError;
		return cb;
	}
}

/**
 * @brief Исполнитель событий транспорта AWH.
 */
class Executor {
	private:
		const fmk_t *            _fmk;
		const log_t *            _log;
		callbacks_t              _cb;
		std::unique_ptr <state_t> _state;
	public:
		void status(const event::status_t status, client_t * client) noexcept {
			switch(static_cast <uint8_t> (status)){
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					if(!client->connect())
						this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
				break;
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					this->_log->print("Client destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		void connect(const event::id_t, const bool ok, client_t * client) noexcept {
			if(!ok){ this->_log->print("Failed to connect", log_t::flag_t::WARNING); return; }
			this->_state = std::make_unique <state_t> (client, this->_log, this->_cb);
			state_t & s = *this->_state;
			// Клиентский preface (magic + SETTINGS) и GET-запрос на поток 1.
			s.session.submitPreface();
			const std::vector <hpack::field_t> req = {
				{ ":method",    "GET" },
				{ ":scheme",    "http" },
				{ ":path",      "/" },
				{ ":authority", "127.0.0.1:2222" },
				{ "user-agent", "awh-h2-client/0.1" }
			};
			s.session.submitHeaders(1, req, true);
			flush(s);
		}
		void read(const event::id_t, const uint8_t * data, const size_t size, client_t * client) noexcept {
			if(!this->_state) return;
			state_t & s = *this->_state;
			s.session.feed(data, size);
			flush(s);
			if(s.finished) client->stop(); // ответ получен — завершаем
		}
		void error(const event::id_t, const event::error_t, const string & message) noexcept {
			this->_log->print("Client error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
	public:
		Executor(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log), _cb(makeCallbacks()) {}
};

/**
 * @brief Точка входа.
 */
int32_t main(int32_t argc, char * argv[]){
	(void) argc; (void) argv;
	fmk_t fmk;
	log_t log(&fmk);
	Executor executor(&fmk, &log);
	client_t client(&fmk, &log);
	client.init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	client.setOptions(event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY);
	if(client.setTarget("127.0.0.1") && client.setTargetPort(2222)){
		client.setTimeout(event::action_t::READ, 6000);
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		client.on <void (const event::id_t, const bool)> ("connect", &Executor::connect, &executor, _1, _2, &client);
		client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &Executor::read, &executor, _1, _2, _3, &client);
		client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2, _3);
		client.start();
	}
	return EXIT_SUCCESS;
}
