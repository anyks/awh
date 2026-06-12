/**
 * @file server.cpp
 * @brief HTTP/2-сервер поверх TCP-транспорта AWH (прямое знание, без TLS/ALPN).
 *
 * Демонстрирует подключение нашего модуля HTTP/2 (experience/h2) к реальному
 * сокету через низкоуровневый транспорт AWH (`server_t`):
 *   - на каждое соединение заводится своя http2::Session (роль SERVER);
 *   - входящие байты из события "read" скармливаются в Session::feed();
 *   - всё, что Session накопила в pending(), отправляется обратно server->send();
 *   - по завершению блока заголовков запроса формируется ответ 200 + тело.
 *
 * Это «h2c с прямым знанием»: клиент сразу шлёт connection preface, без HTTP/1.1
 * Upgrade и без TLS. Для боевого h2 поверх TLS нужен ALPN "h2" на уровне AWH-TLS.
 *
 * Запуск: ./h2-awh-server  (слушает 127.0.0.1:2222)
 * Проверка: ./h2-awh-client  ИЛИ  nghttp -v http://127.0.0.1:2222/
 */

#include <server/server.hpp>

#include "../session.hpp"

#include <map>
#include <memory>
#include <string>

using namespace awh;
using namespace awh::http2;
using namespace placeholders;

namespace {
	/**
	 * @brief Состояние одного соединения: сессия HTTP/2 + указатели на транспорт.
	 */
	struct conn_t {
		Session       session;   // конечный автомат HTTP/2 этого соединения
		server_t *    server;    // транспорт AWH
		event::id_t   eid;       // идентификатор соединения
		const log_t * log;
		conn_t(server_t * s, event::id_t e, const log_t * l, const callbacks_t & cb) noexcept
		 : session(endpoint_t::SERVER, cb, this), server(s), eid(e), log(l) {}
	};

	/// Отправить накопленные сессией исходящие байты в сокет и очистить буфер.
	void flush(conn_t & c) noexcept {
		const std::string & out = c.session.pending();
		if(!out.empty()){
			c.server->send(c.eid, out.data(), out.size());
			c.session.consumePending(out.size());
		}
	}

	// ───────── callbacks нашей http2::Session (user == conn_t*) ─────────

	void onHeader(void * user, uint32_t streamId, std::string_view name, std::string_view value) noexcept {
		conn_t * c = static_cast <conn_t *> (user);
		c->log->print("[h2] stream %u  %.*s: %.*s", log_t::flag_t::INFO,
			streamId, static_cast <int> (name.size()), name.data(), static_cast <int> (value.size()), value.data());
	}

	void onHeadersComplete(void * user, uint32_t streamId, bool endStream) noexcept {
		conn_t * c = static_cast <conn_t *> (user);
		// Формируем простой ответ 200 с текстовым телом.
		const std::vector <hpack::field_t> resp = {
			{ ":status",      "200" },
			{ "content-type", "text/plain; charset=utf-8" },
			{ "server",       "awh-h2/0.1" }
		};
		c->session.submitHeaders(streamId, resp, false);
		static const std::string body = "Hello from AWH transport + own HTTP/2!\n";
		c->session.submitData(streamId, body.data(), body.size(), true);
		flush(*c);
	}

	void onStreamClose(void * user, uint32_t streamId, error_t code) noexcept {
		conn_t * c = static_cast <conn_t *> (user);
		c->log->print("[h2] stream %u closed (code=%u)", log_t::flag_t::INFO, streamId, static_cast <uint32_t> (code));
	}

	void onError(void * user, error_t code, const char * message) noexcept {
		conn_t * c = static_cast <conn_t *> (user);
		c->log->print("[h2] connection error %u: %s", log_t::flag_t::WARNING, static_cast <uint32_t> (code), message);
	}

	/// Общий набор callbacks для всех соединений (user задаётся индивидуально).
	callbacks_t makeCallbacks() noexcept {
		callbacks_t cb;
		cb.onHeader          = onHeader;
		cb.onHeadersComplete = onHeadersComplete;
		cb.onStreamClose     = onStreamClose;
		cb.onError           = onError;
		return cb;
	}
}

/**
 * @brief Исполнитель событий транспорта AWH.
 */
class Executor {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
		callbacks_t   _cb;
		// Карта соединений: eid -> состояние HTTP/2.
		std::map <event::id_t, std::unique_ptr <conn_t>> _conns;
	public:
		void status(const event::status_t status, server_t * server) noexcept {
			switch(static_cast <uint8_t> (status)){
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					if(!server->listen(100))
						this->_log->print("Failed to listen on port %d", log_t::flag_t::WARNING, server->getPort());
					else this->_log->print("HTTP/2 server listening on port %d", log_t::flag_t::INFO, server->getPort());
				break;
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					this->_log->print("Server destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		void accept(const event::id_t, const event::id_t cid, const tls::coder_t::id_t, server_t * server) noexcept {
			server->setOptions(cid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY | event::options::KEEPALIVE);
		}
		void read(const event::id_t eid, const uint8_t * data, const size_t size, server_t * server) noexcept {
			// Лениво создаём сессию при первом чтении на соединении.
			auto it = this->_conns.find(eid);
			if(it == this->_conns.end()){
				auto c = std::make_unique <conn_t> (server, eid, this->_log, this->_cb);
				c->session.submitPreface(); // серверный preface (SETTINGS)
				flush(*c);
				it = this->_conns.emplace(eid, std::move(c)).first;
			}
			conn_t & c = *it->second;
			if(c.session.feed(data, size) == status_t::ERROR)
				flush(c); // отправить GOAWAY перед закрытием
			else flush(c);
		}
		void error(const event::id_t, const event::error_t, const string & message) noexcept {
			this->_log->print("Server error: %s", log_t::flag_t::CRITICAL, message.c_str());
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
	server_t server(&fmk, &log);
	const event::id_t eid = server.init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	server.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY);
	if(server.setPort(2222) && server.setHost("127.0.0.1")){
		server.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &server);
		server.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &Executor::read, &executor, _1, _2, _3, &server);
		server.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2, _3);
		server.on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Executor::accept, &executor, _1, _2, _3, &server);
		server.start();
	}
	return EXIT_SUCCESS;
}
