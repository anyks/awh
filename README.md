[![ANYKS - WEB](https://raw.githubusercontent.com/anyks/awh/main/img/banner.jpg)](https://anyks.com)

# ANYKS - WEB (AWH) C++

## Project goals and features

- **HTTP / HTTPS**: WEB - CLIENT / SERVER.
- **WS / WSS**: WebSocket - CLIENT / SERVER.
- **Proxy**: HTTP(S) / SOCKS5 PROXY - CLIENT / SERVER.
- **Compress**: GZIP / DEFLATE / BROTLI - compression support.
- **Authentication**: BASIC / DIGEST - authentication support.

## Supported protocols HTTP/1.1 and HTTP/2 (RFC9113)

## Requirements

- [Zlib](http://www.zlib.net)
- [PCRE](https://github.com/luvit/pcre)
- [Brotli](https://github.com/google/brotli)
- [OpenSSL](https://www.openssl.org)
- [LibEv](http://software.schmorp.de/pkg/libev.html)
- [LibIconv](https://www.gnu.org/software/libiconv)
- [LibIdn2](https://www.gnu.org/software/libidn)
- [LibXML2](https://github.com/GNOME/libxml2)
- [JeMalloc](https://jemalloc.net)
- [NgHttp2](https://nghttp2.org/documentation/)
- [NgHttp3](https://nghttp2.org/nghttp3/programmers-guide.html)
- [NgTCP2](https://nghttp2.org/ngtcp2/programmers-guide.html)
- [Libev-Win](https://github.com/disenone/libev-win)
- [LibEvent2](https://github.com/libevent/libevent)
- [NLohmann::json](https://github.com/nlohmann/json)

## To build and launch the project

### To clone the project

```bash
$ git clone --recursive https://github.com/anyks/awh.git
```

### Activate SCTP only (FreeBSD / Linux)

#### FreeBSD

```bash
$ sudo kldload sctp
```

#### Linux (Ubuntu)

```bash
$ sudo apt install libsctp-dev
$ modprobe sctp
$ sysctl -w net.sctp.auth_enable=1
```

```bash
$ cd ./ca
$ ./cert.sh
```

### Build third party for MacOS X, Linux and FreeBSD

```bash
$ ./build_third_party.sh --idn
```

### Build on MacOS X, Linux and FreeBSD

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 ..

$ make
```

### Build on Windows [MSYS2 MinGW]

#### Development environment configuration
- [GIT](https://git-scm.com)
- [Perl](https://strawberryperl.com)
- [Python](https://www.python.org/downloads/windows)
- [MSYS2](https://www.msys2.org)
- [CMAKE](https://cmake.org/download)

#### Assembly is done in MSYS264 terminal

```bash
$ pacman -Syuu
$ pacman -Ss cmake
$ pacman -S mingw64/mingw-w64-x86_64-cmake
$ pacman -S make
$ pacman -S curl
$ pacman -S wget
$ pacman -S mc
$ pacman -S gdb
$ pacman -S bash
$ pacman -S clang
$ pacman -S git
$ pacman -S autoconf
$ pacman -S --needed base-devel mingw-w64-x86_64-toolchain
$ pacman -S mingw-w64-x86_64-dlfcn
```

### Build third party for MS Windows
```bash
$ ./build_third_party.sh --event2
```

#### Project build

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -G "MSYS Makefiles" \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_EVENT2=YES \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 ..

$ cmake --build .
```

### Example WEB Client Multi requests

```c++
#include <client/awh.hpp>

using namespace std;
using namespace awh;

class WebClient {
	private:
		uint8_t _count;
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::awh_t * _awh;
	public:
		
		void message(const int32_t sid, const uint64_t rid, const u_int code, const string & message){
			(void) rid;
			
			if(code >= 300)
				this->_log->print("Request failed: %u %s stream=%i", log_t::flag_t::WARNING, code, message.c_str(), sid);
		}
		
		void active(const client::web_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			
			if(mode == client::web_t::mode_t::CONNECT){
				uri_t uri(this->_fmk);

				client::web_t::request_t req1, req2;

				req1.method = web_t::method_t::GET;
				req2.method = web_t::method_t::GET;

				req1.url = uri.parse("/mac/");
				req2.url = uri.parse("/iphone/");

				this->_awh->send(std::move(req1));
				this->_awh->send(std::move(req2));
			}
		}

		void entity(const int32_t sid, const uint64_t rid, const u_int code, const string & message, const vector <char> & entity){

			(void) sid;
			(void) rid;

			this->_count++;

			cout << "RESPONSE: " << string(entity.begin(), entity.end()) << endl;

			if(this->_count == 2)
				this->_awh->stop();
		}

		void headers(const int32_t sid, const uint64_t rid, const u_int code, const string & message, const unordered_multimap <string, string> & headers){
			(void) sid;
			(void) rid;

			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;

			cout << endl;
		}
	public:
		WebClient(const fmk_t * fmk, const log_t * log, client::awh_t * awh) : _fmk(fmk), _log(log), _awh(awh) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk{};
	log_t log(&fmk);
	uri_t uri(&fmk);

	client::core_t core(&fmk, &log);
	client::awh_t awh(&core, &fmk, &log);

	WebClient executor(&fmk, &log, &awh);

	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	log.name("WEB Client");
	log.format("%H:%M:%S %d.%m.%Y");

	awh.mode({
		client::web_t::flag_t::NOT_INFO,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::VERIFY_SSL,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	core.ca("./ca/cert.pem");

	// awh.proxy("http://user:password@host.com:port");
	// awh.proxy("https://user:password@host.com:port");
	// awh.proxy("socks5://user:password@host.com:port");

	// awh.authTypeProxy(auth_t::type_t::BASIC);
	// awh.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	fn_t callback(&log);

	callback.set <void (const client::web_t::mode_t)> ("active", std::bind(&WebClient::active, &executor, _1));
	callback.set <void (const int32_t, const uint64_t, const u_int, const string &)> ("response", std::bind(&WebClient::message, &executor, _1, _2, _3, _4));
	callback.set <void (const int32_t, const uint64_t, const u_int, const string &, const vector <char> &)> ("entity", std::bind(&WebClient::entity, &executor, _1, _2, _3, _4, _5));
	callback.set <void (const int32_t, const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", std::bind(&WebClient::headers, &executor, _1, _2, _3, _4, _5));

	awh.callback(std::move(callback));
	
	awh.init("https://apple.com");
	awh.start();

	return 0;
}
```

### Example WEB Client Simple

```c++
#include <client/awh.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]){
	fmk_t fmk{};
	log_t log(&fmk);
	uri_t uri(&fmk);

	client::core_t core(&fmk, &log);
	client::awh_t awh(&core, &fmk, &log);

	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	log.name("WEB Client");
	log.format("%H:%M:%S %d.%m.%Y");

	awh.mode({
		client::web_t::flag_t::NOT_INFO,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::VERIFY_SSL,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	core.ca("./ca/cert.pem");

	// awh.proxy("http://user:password@host.com:port");
	// awh.proxy("https://user:password@host.com:port");
	// awh.proxy("socks5://user:password@host.com:port");

	// awh.authTypeProxy(auth_t::type_t::BASIC);
	// awh.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	uri_t::url_t url = uri.parse("https://apple.com/ru/mac");

	const auto & body = awh.GET(url);
	
	if(!body.empty())
		cout << "RESPONSE: " << string(body.begin(), body.end()) << endl;

	return 0;
}
```

### Example WEB Server

```c++
#include <server/awh.hpp>

using namespace std;
using namespace awh;

class WebServer {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::awh_t * _awh;
	private:
		awh::web_t::method_t _method;
	public:

		string password(const uint64_t bid, const string & login){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), "password", bid);

			return "password";
		}

		bool auth(const uint64_t bid, const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), password.c_str(), bid);

			return true;
		}
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::web_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent){

			if((this->_method == awh::web_t::method_t::GET) && (agent == server::web_t::agent_t::HTTP)){

				cout << "URL: " << this->_awh->parser(sid, bid)->request().url << endl << endl;
			
				const string body = "<html>\n<head>\n<title>Hello World!</title>\n</head>\n<body>\n"
				"<h1>\"Hello, World!\" program</h1>\n"
				"<div>\nFrom Wikipedia, the free encyclopedia<br>\n"
				"(Redirected from Hello, world!)<br>\n"
				"Jump to navigationJump to search<br>\n"
				"<strong>\"Hello World\"</strong> redirects here. For other uses, see Hello World (disambiguation).<br>\n"
				"A <strong>\"Hello, World!\"</strong> program generally is a computer program that outputs or displays the message \"Hello, World!\".<br>\n"
				"Such a program is very simple in most programming languages, and is often used to illustrate the basic syntax of a programming language. It is often the first program written by people learning to code. It can also be used as a sanity test to make sure that computer software intended to compile or run source code is correctly installed, and that the operator understands how to use it.\n"
				"</div>\n</body>\n</html>\n";

				if(this->_awh->proto(bid) == engine_t::proto_t::HTTP2){

					vector <pair <string, string>> headers = {
						{":method", "GET"},
						{":scheme", "https"},
						{":path", "/stylesheets/screen.css"},
						{":authority", "example.com"},
						{"accept-encoding", "gzip, deflate"}
					};

					if(this->_awh->push2(sid, bid, headers, awh::http2_t::flag_t::NONE) < 0)
						this->_log->print("Push message is not send", log_t::flag_t::WARNING);
				}

				if(this->_awh->trailers(sid, bid)){

					this->_awh->trailer(sid, bid, "Goga", "Hello");
					this->_awh->trailer(sid, bid, "Hello", "World");
					this->_awh->trailer(sid, bid, "Anyks", "Best of the best");
					this->_awh->trailer(sid, bid, "Checksum", this->_fmk->hash(body, fmk_t::hash_t::MD5));

				}

				this->_awh->send(sid, bid, 200, "OK", vector <char> (body.begin(), body.end()));
			}
		}

		void request(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url){
			this->_method = method;

			if(!url.empty() && (!url.path.empty() && url.path.back().compare("favicon.ico") == 0))
				this->_awh->send(sid, bid, 404);
		}

		void headers(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			(void) sid;
			(void) bid;
			(void) url;
			(void) method;

			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;
		}

		void entity(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity){
			(void) method;

			cout << "URL: " << url << endl << endl;

			cout << "BODY: " << string(entity.begin(), entity.end()) << endl;

			this->_awh->send(sid, bid, 200, "OK", entity, {{"Connection", "close"}});
		}
	public:
		WebServer(const fmk_t * fmk, const log_t * log, server::awh_t * awh) : _fmk(fmk), _log(log), _awh(awh), _method(awh::web_t::method_t::NONE) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	server::core_t core(&fmk, &log);
	server::awh_t awh(&core, &fmk, &log);

	WebServer executor(&fmk, &log, &awh);

	log.name("WEB Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	
	core.clusterSize();

	awh.clusterAutoRestart(true);

	// awh.authType(auth_t::type_t::BASIC);
	awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	awh.init(2222, "127.0.0.1", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});

	/*
	awh.init("anyks", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});
	*/

	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	// awh.addOrigin("example.net");

	// awh.addAltSvc("example.net", "h2=\":2222\"");
	// awh.addAltSvc("example.com", "h2=\":8000\"");

	fn_t callback(&log);

	callback.set <string (const uint64_t, const string &)> ("extractPassword", std::bind(&WebServer::password, &executor, _1, _2));
	callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&WebServer::auth, &executor, _1, _2, _3));
	callback.set <void (const uint64_t, const server::web_t::mode_t)> ("active", std::bind(&WebServer::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&WebServer::accept, &executor, _1, _2, _3));
	callback.set <void (const int32_t, const uint64_t, const server::web_t::agent_t)> ("handshake", std::bind(&WebServer::handshake, &executor, _1, _2, _3));
	callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", std::bind(&WebServer::request, &executor, _1, _2, _3, _4));
	callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", std::bind(&WebServer::entity, &executor, _1, _2, _3, _4, _5));
	callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", std::bind(&WebServer::headers, &executor, _1, _2, _3, _4, _5));

	awh.callback(std::move(callback));

	awh.start();

	return 0;
}
```

### Example WebSocket Client

```c++
#include <client/ws.hpp>

using namespace std;
using namespace awh;
using namespace awh::client;

class Executor {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::websocket_t * _ws;
	public:

		void status(const awh::core_t::status_t status, awh::core_t * core){
			(void) core;

			switch(static_cast <uint8_t> (status)){
				case static_cast <uint8_t> (awh::core_t::status_t::START):
					this->_log->print("START", log_t::flag_t::INFO);
				break;
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					this->_log->print("STOP", log_t::flag_t::INFO);
				break;
			}
		}

		void handshake(const int32_t sid, const uint64_t rid, const client::web_t::agent_t agent){
			(void) sid;
			(void) rid;

			if(agent == client::web_t::agent_t::WEBSOCKET){
				this->_log->print("Handshake", log_t::flag_t::INFO);
				
				const string query = "Hello World!!!";

				this->_ws->sendMessage(vector <char> (query.begin(), query.end()));
			}
		}
	public:

		void error(const u_int code, const string & mess){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}

		void message(const vector <char> & buffer, const bool utf8){
			string subprotocol = "";

			const auto subprotocols = this->_ws->subprotocols();

			if(!subprotocols.empty())
				subprotocol = (* subprotocols.begin());

			if(utf8){
				
				cout << "MESSAGE: " << string(buffer.begin(), buffer.end()) << endl;

				cout << "SUB PROTOCOL: " << subprotocol << endl;
			}
		}
	public:
		Executor(const fmk_t * fmk, const log_t * log, client::websocket_t * ws) : _fmk(fmk), _log(log), _ws(ws) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	client::core_t core(&fmk, &log);
	websocket_t ws(&core, &fmk, &log);

	Executor executor(&fmk, &log, &ws);

	log.name("WebSocket Client");
	log.format("%H:%M:%S %d.%m.%Y");

	ws.mode({
		client::web_t::flag_t::ALIVE,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::TAKEOVER_CLIENT,
		client::web_t::flag_t::TAKEOVER_SERVER,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	core.ca("./ca/cert.pem");

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	// ws.proxy("http://user:password@host.com:port");
	// ws.proxy("https://user:password@host.com:port");
	// ws.proxy("socks5://user:password@host.com:port");

	// ws.authTypeProxy(auth_t::type_t::BASIC);
	// ws.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	
	ws.user("user", "password");

	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);

	ws.init("wss://127.0.0.1:2222", {awh::http_t::compress_t::DEFLATE});

	ws.subprotocols({"test2", "test8", "test9"});
	// ws.extensions({{"test1", "test2", "test3"},{"good1", "good2", "good3"}});

	fn_t callback(&log);

	callback.set <void (const u_int, const string &)> ("errorWebsocket", std::bind(&Executor::error, &executor, _1, _2));
	callback.set <void (const vector <char> &, const bool)> ("messageWebsocket", std::bind(&Executor::message, &executor, _1, _2));
	callback.set <void (const awh::core_t::status_t, awh::core_t *)> ("status", std::bind(&Executor::status, &executor, _1, _2));
	callback.set <void (const int32_t, const uint64_t, const client::web_t::agent_t)> ("handshake", std::bind(&Executor::handshake, &executor, _1, _2, _3));

	ws.callback(std::move(callback));

	ws.start();

	return 0;
}
```

### Example WebSocket Server

```c++
#include <server/ws.hpp>

using namespace std;
using namespace awh;
using namespace awh::server;

class Executor {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::websocket_t * _ws;
	public:

		string password(const uint64_t bid, const string & login){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), "password", bid);

			return "password";
		}

		bool auth(const uint64_t bid, const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), password.c_str(), bid);

			return true;
		}
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::web_t::mode_t mode){
			(void) bid;

			switch(static_cast <uint8_t> (mode)){
				case static_cast <uint8_t> (server::web_t::mode_t::OPEN):
					this->_log->print("CONNECT", log_t::flag_t::INFO);
				break;
				case static_cast <uint8_t> (server::web_t::mode_t::CLOSE):
					this->_log->print("DISCONNECT", log_t::flag_t::INFO);
				break;
			}
		}

		void error(const uint64_t bid, const u_int code, const string & mess){
			(void) bid;

			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}

		void message(const uint64_t bid, const vector <char> & buffer, const bool text){
			if(!buffer.empty()){
				string subprotocol = "";

				const auto subprotocols = this->_ws->subprotocols(bid);

				if(!subprotocols.empty())
					subprotocol = (* subprotocols.begin());

				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), subprotocol.c_str());

				this->_ws->sendMessage(bid, buffer, text);
			}
		}

		void headers(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			(void) sid;
			(void) bid;
			(void) method;

			uri_t uri(this->_fmk);

			this->_log->print("REQUEST ID=%zu URL=%s", log_t::flag_t::INFO, bid, uri.url(url).c_str());

			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;
		}
	public:
		Executor(const fmk_t * fmk, const log_t * log, server::websocket_t * ws) : _fmk(fmk), _log(log), _ws(ws) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	server::core_t core(&fmk, &log);
	websocket_t ws(&core, &fmk, &log);

	Executor executor(&fmk, &log, &ws);

	log.name("WebSocket Server");
	log.format("%H:%M:%S %d.%m.%Y");

	ws.mode({
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER
	});

	core.clusterSize();

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	ws.subprotocols({"test1", "test2", "test3"});

	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);

	// ws.init("anyks", {awh::http_t::compress_t::DEFLATE});
	// ws.init(2222, "", {awh::http_t::compress_t::DEFLATE});
	ws.init(2222, "127.0.0.1", {awh::http_t::compress_t::DEFLATE});

	fn_t callback(&log);

	callback.set <string (const uint64_t, const string &)> ("extractPassword", std::bind(&Executor::password, &executor, _1, _2));
	callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&Executor::auth, &executor, _1, _2, _3));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Executor::accept, &executor, _1, _2, _3));
	callback.set <void (const uint64_t, const server::web_t::mode_t)> ("active", std::bind(&Executor::active, &executor, _1, _2));
	callback.set <void (const uint64_t, const u_int, const string &)> ("errorWebsocket", std::bind(&Executor::error, &executor, _1, _2, _3));
	callback.set <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", std::bind(&Executor::message, &executor, _1, _2, _3));
	callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", std::bind(&Executor::headers, &executor, _1, _2, _3, _4, _5));

	ws.callback(std::move(callback));

	ws.start();

	return 0;
}
```

### Example HTTPS Multi Server
```c++
#include <server/awh.hpp>

using namespace std;
using namespace awh;

class WebServer {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::awh_t * _awh;
	private:
		awh::web_t::method_t _method;
	public:

		string password(const uint64_t bid, const string & login){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), "password", bid);

			return "password";
		}

		bool auth(const uint64_t bid, const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), password.c_str(), bid);

			return true;
		}
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::web_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void error(const uint64_t bid, const u_int code, const string & mess){
			(void) bid;

			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}

		void message(const uint64_t bid, const vector <char> & buffer, const bool text){
			if(!buffer.empty()){
				string subprotocol = "";

				const auto subprotocols = this->_awh->subprotocols(bid);

				if(!subprotocols.empty())
					subprotocol = (* subprotocols.begin());

				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), subprotocol.c_str());

				this->_awh->sendMessage(bid, buffer, text);
			}
		}

		void handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent){
			if((this->_method == awh::web_t::method_t::GET) && (agent == server::web_t::agent_t::HTTP)){
				cout << " URL: " << this->_awh->parser(sid, bid)->request().url << endl;

				const string body = "<html>\n<head>\n<title>Hello World!</title>\n</head>\n<body>\n"
				"<h1>\"Hello, World!\" program</h1>\n"
				"<div>\nFrom Wikipedia, the free encyclopedia<br>\n"
				"(Redirected from Hello, world!)<br>\n"
				"Jump to navigationJump to search<br>\n"
				"<strong>\"Hello World\"</strong> redirects here. For other uses, see Hello World (disambiguation).<br>\n"
				"A <strong>\"Hello, World!\"</strong> program generally is a computer program that outputs or displays the message \"Hello, World!\".<br>\n"
				"Such a program is very simple in most programming languages, and is often used to illustrate the basic syntax of a programming language. It is often the first program written by people learning to code. It can also be used as a sanity test to make sure that computer software intended to compile or run source code is correctly installed, and that the operator understands how to use it.\n"
				"</div>\n</body>\n</html>\n";

				if(this->_awh->trailers(sid, bid)){
					this->_awh->trailer(sid, bid, "Goga", "Hello");
					this->_awh->trailer(sid, bid, "Hello", "World");
					this->_awh->trailer(sid, bid, "Anyks", "Best of the best");
					this->_awh->trailer(sid, bid, "Checksum", this->_fmk->hash(body, fmk_t::hash_t::MD5));
				}

				this->_awh->send(sid, bid, 200, "OK", vector <char> (body.begin(), body.end()));
			}
		}

		void request(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url){
			this->_method = method;

			if(!url.empty() && (!url.path.empty() && url.path.back().compare("favicon.ico") == 0))
				this->_awh->send(sid, bid, 404);
		}

		void headers(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			(void) sid;
			(void) bid;
			(void) method;
			(void) url;

			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;
		}

		void entity(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity){
			(void) method;

			cout << "URL: " << url << endl << endl;

			cout << "BODY: " << string(entity.begin(), entity.end()) << endl;

			this->_awh->send(sid, bid, 200, "OK", entity, {{"Connection", "close"}});
		}
	public:
		WebServer(const fmk_t * fmk, const log_t * log, server::awh_t * awh) : _fmk(fmk), _log(log), _awh(awh), _method(awh::web_t::method_t::NONE) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	server::core_t core(&fmk, &log);
	server::awh_t awh(&core, &fmk, &log);

	WebServer executor(&fmk, &log, &awh);

	log.name("WEB Server");
	log.format("%H:%M:%S %d.%m.%Y");

	awh.mode({
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER,
		server::web_t::flag_t::WEBSOCKET_ENABLE,
		server::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	core.clusterSize();

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	awh.clusterAutoRestart(true);

	// awh.authType(auth_t::type_t::BASIC);
	awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	awh.init(2222, "127.0.0.1", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});

	/*
	awh.init("anyks", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});
	*/

	awh.addOrigin("example.net");

	awh.addAltSvc("example.net", "h2=\":2222\"");
	awh.addAltSvc("example.com", "h2=\":8000\"");

	awh.subprotocols({"test1", "test2", "test3"});

	callback.set <string (const uint64_t, const string &)> ("extractPassword", std::bind(&WebServer::password, &executor, _1, _2));
	callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&WebServer::auth, &executor, _1, _2, _3));
	callback.set <void (const uint64_t, const server::web_t::mode_t)> ("active", std::bind(&WebServer::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&WebServer::accept, &executor, _1, _2, _3));
	callback.set <void (const uint64_t, const u_int, const string &)> ("errorWebsocket", std::bind(&WebServer::error, &executor, _1, _2, _3));
	callback.set <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", std::bind(&WebServer::message, &executor, _1, _2, _3));
	callback.set <void (const int32_t, const uint64_t, const server::web_t::agent_t)> ("handshake", std::bind(&WebServer::handshake, &executor, _1, _2, _3));
	callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", std::bind(&WebServer::request, &executor, _1, _2, _3, _4));
	callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", std::bind(&WebServer::entity, &executor, _1, _2, _3, _4, _5));
	callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", std::bind(&WebServer::headers, &executor, _1, _2, _3, _4, _5));

	awh.start();

	return 0;
}
```

### Example HTTPS PROXY Server

```c++
#include <server/proxy.hpp>

using namespace std;
using namespace awh;

class Proxy {
	private:
		log_t * _log;
	public:

		string password(const uint64_t bid, const string & login){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");

			return "password";
		}

		bool auth(const uint64_t bid, const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());

			return true;
		}
	public:
		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::proxy_t::broker_t broker, const server::web_t::mode_t mode){
			(void) bid;

			switch(static_cast <uint8_t> (broker)){
				case static_cast <uint8_t> (server::proxy_t::broker_t::CLIENT):
					this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
				break;
				case static_cast <uint8_t> (server::proxy_t::broker_t::SERVER):
					this->_log->print("%s server", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
				break;
			}
		}
	public:
		Proxy(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Proxy executor(&log);
	server::proxy_t proxy(&fmk, &log);

	log.name("Proxy Server");
	log.format("%H:%M:%S %d.%m.%Y");

	proxy.clusterSize();

	proxy.ca(server::proxy_t::broker_t::CLIENT, "./ca/cert.pem");

	proxy.mode({
		server::proxy_t::flag_t::SYNCPROTO,
		server::proxy_t::flag_t::REDIRECTS,
		server::proxy_t::flag_t::CONNECT_METHOD_SERVER_ENABLE
	});

	// proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::BASIC);
	// proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::DIGEST, auth_t::hash_t::SHA512);
	proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	// proxy.init("anyks", http_t::compress_t::GZIP);
	// proxy.init(2222, "", http_t::compress_t::GZIP);
	proxy.init(2222, "127.0.0.1", http_t::compress_t::GZIP);
	
	proxy.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	fn_t callback(&log);

	callback.set <string (const uint64_t, const string &)> ("extractPassword", std::bind(&Proxy::password, &executor, _1, _2));
	callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&Proxy::auth, &executor, _1, _2, _3));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Proxy::accept, &executor, _1, _2, _3));
	callback.set <void (const uint64_t, const server::proxy_t::broker_t, const server::web_t::mode_t)> ("active", std::bind(&Proxy::active, &executor, _1, _2, _3));

	proxy.callback(std::move(callback));

	proxy.start();

	return 0;
}
```

### Example Socks5 PROXY Server

```c++
#include <server/socks5.hpp>

using namespace std;
using namespace awh;
using namespace server;

class Proxy {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		proxy_socks5_t * _socks5;
	public:

		bool auth(const uint64_t bid, const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), password.c_str(), bid);

			return true;
		}
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const proxy_socks5_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == proxy_socks5_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
	public:
		Proxy(const fmk_t * fmk, const log_t * log, proxy_socks5_t * socks5) : _fmk(fmk), _log(log), _socks5(socks5) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	proxy_socks5_t proxy(&fmk, &log);
	Proxy executor(&fmk, &log, &proxy);

	log.name("Proxy Socks5 Server");
	log.format("%H:%M:%S %d.%m.%Y");

	proxy.clusterSize();

	proxy.ca("./ca/cert.pem");

	proxy.sonet(awh::scheme_t::sonet_t::TCP);

	proxy.mode({proxy_socks5_t::flag_t::WAIT_MESS});

	// proxy.init("anyks");
	// proxy.init(2222);
	proxy.init(2222, "127.0.0.1");
	
	fn_t callback(&log);

	callback.set <void (const size_t, const proxy_socks5_t::mode_t)> ("active", std::bind(&Proxy::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Proxy::accept, &executor, _1, _2, _3));
	// callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&Proxy::auth, &executor, _1, _2, _3));

	proxy.callback(std::move(callback));

	proxy.start();

	return 0;
}
```

### Example Timer

```c++
#include <chrono>
#include <core/core.hpp>

using namespace std;
using namespace awh;

class Executor {
	private:
		chrono::time_point <chrono::system_clock> ts;
		chrono::time_point <chrono::system_clock> is;
	private:
		u_short count;
	private:
		log_t * _log;
	public:

		void interval(const u_short id, core_t * core){
			auto shift = chrono::system_clock::now();

			this->_log->print("Interval: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (shift - this->is).count());

			this->is = shift;

			if((this->count++) >= 10){
				core->clearTimer(id);
				core->stop();
			}
		}

		void timeout(const u_short id, core_t * core){
			(void) id;
			(void) core;

			this->_log->print("Timeout: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - this->ts).count());
		}

		void run(const awh::core_t::status_t status, core_t * core){
			switch(static_cast <uint8_t> (status)){
				case static_cast <uint8_t> (awh::core_t::status_t::START): {
					this->ts = chrono::system_clock::now();
					this->is = this->ts;

					this->_log->print("%s", log_t::flag_t::INFO, "Start timer");

					core->setTimeout(10000, std::bind(&Executor::timeout, this, _1, _2));
					core->setInterval(5000, std::bind(&Executor::interval, this, _1, _2));
				} break;
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					this->_log->print("%s", log_t::flag_t::INFO, "Stop timer");
				break;
			}
		}
	public:
		Executor(log_t * log) : ts(chrono::system_clock::now()), is(chrono::system_clock::now()), count(0), _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Executor executor(&log);
	core_t core(&fmk, &log);

	log.name("Timer");
	log.format("%H:%M:%S %d.%m.%Y");

	fn_t callback(&log);

	callback.set <void (const awh::core_t::status_t, core_t *)> ("status", std::bind(&Executor::run, &executor, _1, _2));

	core.callback(std::move(callback));

	core.start();

	return 0;
}
```

### Example DNS Resolver

```c++
#include <net/dns.hpp>
#include <core/core.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);
	core_t core(&fmk, &log);

	log.name("DNS");
	log.format("%H:%M:%S %d.%m.%Y");

	dns.prefix("AWH");

	dns.servers({"77.88.8.88", "77.88.8.2"});

	log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("localhost").c_str());
	log.print("IP2: %s", log_t::flag_t::INFO, dns.resolve("yandex.ru").c_str());
	log.print("IP3: %s", log_t::flag_t::INFO, dns.resolve("google.com").c_str());

	log.print("IP4: %s", log_t::flag_t::INFO, dns.resolve("stalin.info").c_str());
	log.print("Encode domain \"ремпрофи.рф\" == \"%s\"", log_t::flag_t::INFO, dns.encode("ремпрофи.рф").c_str());
	log.print("Decode domain \"xn--e1agliedd7a.xn--p1ai\" == \"%s\"", log_t::flag_t::INFO, dns.decode("xn--e1agliedd7a.xn--p1ai").c_str());

	return 0;
}
```

### Example NTP Client

```c++
#include <net/ntp.hpp>
#include <core/core.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	ntp_t ntp(&fmk, &log);
	core_t core(&fmk, &log);

	log.name("NTP");
	log.format("%H:%M:%S %d.%m.%Y");

	ntp.ns({"77.88.8.88", "77.88.8.2"});

	ntp.servers({"0.ru.pool.ntp.org", "1.ru.pool.ntp.org", "2.ru.pool.ntp.org", "3.ru.pool.ntp.org"});

	log.print("Time: %s", log_t::flag_t::INFO, fmk.time2str((ntp.request() / 1000), "%H:%M:%S %d.%m.%Y").c_str());

	return 0;
}
```

### Example PING

```c++
#include <net/ping.hpp>
#include <core/core.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	core_t core(&fmk, &log);
	ping_t ping(&fmk, &log);

	log.name("PING");
	log.format("%H:%M:%S %d.%m.%Y");

	const double result = ping.ping("api.telegram.org", 10);

	log.print("PING result=%f", log_t::flag_t::INFO, result);

	return 0;
}
```

### Example TCP Client

```c++
#include <client/sample.hpp>

using namespace std;
using namespace awh;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::sample_t * _sample;
	public:

		void active(const client::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				this->_sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			this->_sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log, &sample);

	log.name("TCP Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
	});

	core.sonet(awh::scheme_t::sonet_t::TCP);

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example TCP Server

```c++
#include <server/sample.hpp>

using namespace std;
using namespace awh;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::sample_t * _sample;
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer){
			(void) bid;

			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log, &sample);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TCP);

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	callback.set <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example TLS Client

```c++
#include <client/sample.hpp>

using namespace std;
using namespace awh;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::sample_t * _sample;
	public:

		void active(const client::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				this->_sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			this->_sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log, &sample);

	log.name("TLS Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
	});
	core.verifySSL(false);

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example TLS Server

```c++
#include <server/sample.hpp>

using namespace std;
using namespace awh;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::sample_t * _sample;
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer){
			(void) bid;

			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log, &sample);

	log.name("TLS Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	callback.set <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example UDP Client

```c++
#include <client/sample.hpp>

using namespace std;
using namespace awh;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::sample_t * _sample;
	public:

		void active(const client::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				this->_sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			this->_sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log, &sample);

	log.name("UDP Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
	});

	core.sonet(awh::scheme_t::sonet_t::UDP);

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example UDP Server

```c++
#include <server/sample.hpp>

using namespace std;
using namespace awh;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::sample_t * _sample;
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer){
			(void) bid;

			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log, &sample);

	log.name("UDP Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::UDP);

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	callback.set <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example SCTP Client

```c++
#include <client/sample.hpp>

using namespace std;
using namespace awh;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::sample_t * _sample;
	public:

		void active(const client::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				this->_sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			this->_sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log, &sample);

	log.name("SCTP Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
	});
	core.verifySSL(false);

	core.sonet(awh::scheme_t::sonet_t::SCTP);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example SCTP Server

```c++
#include <server/sample.hpp>

using namespace std;
using namespace awh;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::sample_t * _sample;
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer){
			(void) bid;

			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log, &sample);

	log.name("SCTP Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::SCTP);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	callback.set <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example DTLS Client

```c++
#include <client/sample.hpp>

using namespace std;
using namespace awh;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::sample_t * _sample;
	public:

		void active(const client::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				this->_sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			this->_sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log, &sample);

	log.name("DTLS Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
	});
	core.verifySSL(false);

	core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example DTLS Server

```c++
#include <server/sample.hpp>

using namespace std;
using namespace awh;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::sample_t * _sample;
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer){
			(void) bid;

			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log, &sample);

	log.name("DTLS Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	sample.init(2222, "127.0.0.1");

	fn_t callback(&log);

	callback.set <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	callback.set <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example TCP UnixSocket Client

```c++
#include <client/sample.hpp>

using namespace std;
using namespace awh;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::sample_t * _sample;
	public:

		void active(const client::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				this->_sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			this->_sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log, &sample);

	log.name("UnixSocket Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
	});
	
	core.sonet(awh::scheme_t::sonet_t::TCP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");

	fn_t callback(&log);

	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example TCP UnixSocket Server

```c++
#include <server/sample.hpp>

using namespace std;
using namespace awh;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::sample_t * _sample;
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer){
			(void) bid;

			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log, &sample);

	log.name("UnixSocket Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TCP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");

	fn_t callback(&log);

	callback.set <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	callback.set <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example UDP UnixSocket Client

```c++
#include <client/sample.hpp>

using namespace std;
using namespace awh;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		client::sample_t * _sample;
	public:

		void active(const client::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				this->_sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			this->_sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log, client::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log, &sample);

	log.name("UDP UnixSocket Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode({
		// client::sample_t::flag_t::NOT_INFO,
		client::sample_t::flag_t::WAIT_MESS,
	});
	
	core.sonet(awh::scheme_t::sonet_t::UDP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");

	fn_t callback(&log);

	callback.set <void (const vector <char> &)> ("message", std::bind(&Client::message, &executor, _1));
	callback.set <void (const client::sample_t::mode_t)> ("active", std::bind(&Client::active, &executor, _1));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example UDP UnixSocket Server

```c++
#include <server/sample.hpp>

using namespace std;
using namespace awh;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	private:
		server::sample_t * _sample;
	public:

		bool accept(const string & ip, const string & mac, const u_int port){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active(const uint64_t bid, const server::sample_t::mode_t mode){
			(void) bid;

			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer){
			(void) bid;

			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			this->_sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log, server::sample_t * sample) : _fmk(fmk), _log(log), _sample(sample) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log, &sample);

	log.name("UDP UnixSocket Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::UDP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");

	fn_t callback(&log);

	callback.set <void (const uint64_t, const vector <char> &)> ("message", std::bind(&Server::message, &executor, _1, _2));
	callback.set <void (const uint64_t, const server::sample_t::mode_t)> ("active", std::bind(&Server::active, &executor, _1, _2));
	callback.set <bool (const string &, const string &, const u_int)> ("accept", std::bind(&Server::accept, &executor, _1, _2, _3));

	sample.callback(std::move(callback));

	sample.start();

	return 0;
}
```

### Example Cluster

```c++
#include <cluster/core.hpp>

using namespace std;
using namespace awh;

class Executor {
	private:
		log_t * _log;
	public:

		void events(const cluster_t::family_t worker, const pid_t pid, const cluster_t::event_t event, cluster::core_t * core){
			(void) pid;

			if(event == cluster_t::event_t::START){
				switch(static_cast <uint8_t> (worker)){
					case static_cast <uint8_t> (cluster_t::family_t::MASTER): {
						const char * message = "Hi!";

						core->broadcast(message, strlen(message));
					} break;
					case static_cast <uint8_t> (cluster_t::family_t::CHILDREN): {
						const char * message = "Hello";

						core->send(message, strlen(message));
					} break;
				}
			}
		}

		void message(const cluster_t::family_t worker, const pid_t pid, const char * buffer, const size_t size, cluster::core_t * core){
			(void) core;

			switch(static_cast <uint8_t> (worker)){
				case static_cast <uint8_t> (cluster_t::family_t::MASTER):
					this->_log->print("Message from children [%u]: %s", log_t::flag_t::INFO, pid, string(buffer, size).c_str());
				break;
				case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
					this->_log->print("Message from master: %s [%u]", log_t::flag_t::INFO, string(buffer, size).c_str(), getpid());
				break;
			}
		}

		void run(const awh::core_t::status_t status, core_t * core){
			(void) core;

			switch(static_cast <uint8_t> (status)){
				case static_cast <uint8_t> (awh::core_t::status_t::START):
					this->_log->print("%s", log_t::flag_t::INFO, "Start cluster");
				break;
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					this->_log->print("%s", log_t::flag_t::INFO, "Stop cluster");
				break;
			}
		}
	public:
		Executor(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Executor executor(&log);
	cluster::core_t core(&fmk, &log);

	log.name("Cluster");
	log.format("%H:%M:%S %d.%m.%Y");

	core.size();
	core.autoRestart(true);

	fn_t callback(&log);

	callback.set <void (const awh::core_t::status_t, core_t *)> ("status", std::bind(&Executor::run, &executor, _1, _2));
	callback.set <void (const cluster_t::family_t, const pid_t, const cluster_t::event_t, cluster::core_t *)> ("events", std::bind(&Executor::events, &executor, _1, _2, _3, _4));
	callback.set <void (const cluster_t::family_t, const pid_t, const char *, const size_t, cluster::core_t *)> ("message", std::bind(&Executor::message, &executor, _1, _2, _3, _4, _5));

	core.callback(std::move(callback));

	core.start();

	return 0;
}
```

### Example IP-Address

```c++
#include <net/net.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]){
	net_t net{};

	net = "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d]";
	cout << " [2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] || " << net << " === " << net.get(net_t::format_t::LONG_IPV4) << " === " << net.get(net_t::format_t::MIDDLE_IPV4) << " === " << net.get(net_t::format_t::SHORT_IPV4) << endl;

	net = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	cout << " 2001:0db8:0000:0000:0000:0000:ae21:ad12 || " << net << endl;

	net = "2001:db8::ae21:ad12";
	cout << " 2001:db8::ae21:ad12 || " << net.get(net_t::format_t::LONG) << " and " << net.get(net_t::format_t::MIDDLE) << endl;

	net = "0000:0000:0000:0000:0000:0000:ae21:ad12";
	cout << " 0000:0000:0000:0000:0000:0000:ae21:ad12 || " << net.get(net_t::format_t::SHORT) << endl;

	net = "::ae21:ad12";
	cout << " ::ae21:ad12 || " << net.get(net_t::format_t::MIDDLE) << endl;

	net = "2001:0db8:11a3:09d7:1f34::";
	cout << " 2001:0db8:11a3:09d7:1f34:: || " << net.get(net_t::format_t::LONG) << endl;

	net = "::ffff:192.0.2.1";
	cout << boolalpha;
	cout << " ::ffff:192.0.2.1 || " << net << " ==== " << net.broadcastIPv6ToIPv4() << endl;

	net = "::1";
	cout << " ::1 || " << net.get(net_t::format_t::LONG) << endl;

	net = "[::]";
	cout << " [::] || " << net.get(net_t::format_t::LONG) << endl;

	net = "46.39.230.51";
	cout << " 46.39.230.51 || " << net.get(net_t::format_t::LONG) << " ==== " << net.broadcastIPv6ToIPv4() << endl;

	net = "192.16.0.1";
	cout << " 192.16.0.1 || " << net.get(net_t::format_t::LONG) << " === " << net.get(net_t::format_t::LONG_IPV6) << " === " << net.get(net_t::format_t::SHORT_IPV6) << " === " << net.get(net_t::format_t::MIDDLE_IPV6) << endl;

	net = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	cout << " Составная часть адреса: 2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d || " << net.v6()[0] << " and " << net.v6()[1] << endl;

	net = "46.39.230.51";
	cout << " Составная часть адреса: 46.39.230.51 || " << net.v4() << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose(53, net_t::addr_t::NETW);
	cout << " Наложение префикса: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 || " << net << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose("FFFF:FFFF:FFFF:F800::", net_t::addr_t::NETW);
	cout << " Наложение префикса: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: || " << net << endl;

	net = "192.168.3.192";
	net.impose(9, net_t::addr_t::NETW);
	cout << " Наложение префикса: 192.168.3.192/9 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.128.0.0", net_t::addr_t::NETW);
	cout << " Наложение префикса: 192.168.3.192/255.128.0.0 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.255.255.0", net_t::addr_t::NETW);
	cout << " Наложение префикса: 192.168.3.192/255.255.255.0 || " << net << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose(53, net_t::addr_t::HOST);
	cout << " Получаем хост адреса: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 || " << net << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose("FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST);
	cout << " Получаем хост адреса: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: || " << net << endl;

	net = "192.168.3.192";
	net.impose(9, net_t::addr_t::HOST);
	cout << " Получаем хост адреса: 192.168.3.192/9 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.128.0.0", net_t::addr_t::HOST);
	cout << " Получаем хост адреса: 192.168.3.192/255.128.0.0 || " << net << endl;

	net = "192.168.3.192";
	net.impose(24, net_t::addr_t::HOST);
	cout << " Получаем хост адреса: 192.168.3.192/24 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.255.255.0", net_t::addr_t::HOST);
	cout << " Получаем хост адреса: 192.168.3.192/255.255.255.0 || " << net << endl;

	net = "192.168.3.192";
	cout << " Получаем маску адреса из префикса сети 9 || " << net.prefix2Mask(9) << endl;
	cout << " Получаем префикс сети из маски адреса 255.128.0.0 || " << (u_short) net.mask2Prefix("255.128.0.0") << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << " Получаем маску адреса из префикса сети 53 || " << net.prefix2Mask(53) << endl;
	cout << " Получаем префикс сети из маски адреса FFFF:FFFF:FFFF:F800:: || " << (u_short) net.mask2Prefix("FFFF:FFFF:FFFF:F800::") << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 сети 192.168.0.0 || " << net.mapping("192.168.0.0") << endl;
	
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5678:: || " << net.mapping("2001:1234:abcd:5678::") << endl;
	
	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 сети 192.128.0.0/9 || " << net.mapping("192.128.0.0", 9, net_t::addr_t::NETW) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5000::/53 || " << net.mapping("2001:1234:abcd:5000::", 53, net_t::addr_t::NETW) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 сети 192.128.0.0/255.128.0.0 || " << net.mapping("192.128.0.0", "255.128.0.0", net_t::addr_t::NETW) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5000::/FFFF:FFFF:FFFF:F800:: || " << net.mapping("2001:1234:abcd:5000::", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::NETW) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 хосту 9/0.40.3.192 || " << net.mapping("0.40.3.192", 9, net_t::addr_t::HOST) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb хосту 53/::678:9877:3322:5541:AABB || " << net.mapping("::678:9877:3322:5541:AABB", 53, net_t::addr_t::HOST) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 хосту 255.128.0.0/0.40.3.192 || " << net.mapping("0.40.3.192", "255.128.0.0", net_t::addr_t::HOST) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb хосту FFFF:FFFF:FFFF:F800::/::678:9877:3322:5541:AABB || " << net.mapping("::678:9877:3322:5541:AABB", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Выполняем проверку на вхождение адреса в диапазон 192.168.3.192 в диапазон [192.168.3.100 - 192.168.3.200] || " << net.range("192.168.3.100", "192.168.3.200", 24) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	const auto & dump = net.data <vector <uint16_t>> ();
	for(auto & item : dump)
		cout << " Данные чанков адреса IPv6 || " << item << endl;

	net = "46.39.230.51";
	cout << boolalpha;
	cout << " Выполняем проверку является ли IP адрес глобальным 46.39.230.51 || " << (net.mode() == net_t::mode_t::GLOBAL) << endl;

	net = "192.168.31.12";
	cout << boolalpha;
	cout << " Выполняем проверку является ли IP адрес локальным 192.168.31.12 || " << (net.mode() == net_t::mode_t::LOCAL) << endl;

	net = "0.0.0.0";
	cout << boolalpha;
	cout << " Выполняем проверку является ли IP адрес зарезервированным 0.0.0.0 || " << (net.mode() == net_t::mode_t::RESERV) << endl;

	net = "[2a00:1450:4010:c0a::8b]";
	cout << boolalpha;
	cout << " Выполняем проверку является ли IP адрес глобальным [2a00:1450:4010:c0a::8b] || " << (net.mode() == net_t::mode_t::GLOBAL) << endl;

	net = "::1";
	cout << boolalpha;
	cout << " Выполняем проверку является ли IP адрес локальным [::1] || " << (net.mode() == net_t::mode_t::LOCAL) << endl;

	net = "::";
	cout << boolalpha;
	cout << " Выполняем проверку является ли IP адрес зарезервированным [::] || " << (net.mode() == net_t::mode_t::RESERV) << endl;

	string ip = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	cout << " Длинная запись адреса || " << ip << endl;
	ip = net = ip;
	cout << " Короткая запись адреса || " << ip << endl;

	net = "73:0b:04:0d:db:79";
	cout << " 73:0b:04:0d:db:79 || " << net << endl;

	return 0;
}
```
