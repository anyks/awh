[![ANYKS - WEB](https://raw.githubusercontent.com/anyks/awh/main/img/banner.jpg)](https://anyks.com)

# ANYKS - WEB (AWH) C++

## Project goals and features

- **HTTP / HTTPS**: WEB - CLIENT / SERVER.
- **WS / WSS**: WebSocket - CLIENT / SERVER.
- **Proxy**: HTTP(S) / SOCKS5 PROXY - CLIENT / SERVER.
- **Compress**: GZIP / BZIP2 / ZSTD / LZ4 / LZMA / DEFLATE / BROTLI - compression support.
- **Authentication**: BASIC / DIGEST - authentication support.

## Supported protocols HTTP/1.1 and HTTP/2 (RFC9113)

## Requirements

- [LZ4](https://lz4.org)
- [Zlib](http://www.zlib.net)
- [BZip2](http://www.bzip.org)
- [Brotli](https://brotli.org)
- [ZStandart](https://github.com/facebook/zstd)
- [Lempel–Ziv–Markov](https://github.com/hunter-packages/lzma)
- [PCRE2](https://www.pcre.org)
- [OpenSSL](https://www.openssl.org)
- [CityHash](https://github.com/google/cityhash)
- [LibIconv](https://www.gnu.org/software/libiconv)
- [LibIdn2](https://www.gnu.org/software/libidn)
- [NgHttp2](https://nghttp2.org/documentation)
- [GPerfTools](https://github.com/gperftools/gperftools)

## To build and launch the project

### To clone the project

```bash
$ git clone --recursive https://gitflic.ru/project/anyks/awh.git
```

### Activate SCTP only (FreeBSD / Linux)

#### FreeBSD

```bash
$ sudo kldload sctp
```

#### Linux (Ubuntu)

```bash
$ sudo apt install libsctp-dev
$ sudo modprobe sctp
$ sudo sysctl -w net.sctp.auth_enable=1
```

#### Linux (Fedora)

```bash
$ sudo yum install lksctp-tools-devel
$ sudo modprobe sctp
$ sudo sysctl -w net.sctp.auth_enable=1
```

#### Linux (openSUSE)

```bash
$ sudo zypper install lksctp-tools-devel
$ sudo modprobe sctp
$ sudo sysctl -w net.sctp.auth_enable=1
```

```bash
$ cd ./certs
$ ./certs.sh example.com
```

### Build third party for MacOS X, Linux, FreeBSD

```bash
$ ./build_third_party.sh --idn
```

### Build on MacOS X, Linux, FreeBSD

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

#### Assembly is done in MSYS2 - MINGW64 terminal

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
$ ./build_third_party.sh
```

#### Project build

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -G "MSYS Makefiles" \
 -DCMAKE_BUILD_IDN=YES \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_SHARED_BUILD_LIB=YES \
 ..

$ cmake --build .
```

---

### Example WEB-client multirequests
```c++
#include <client/awh.hpp>

using namespace awh;
using namespace placeholders;

class WebClient {
	private:
		uint8_t _count;
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:
		
		void message(const int32_t sid, [[maybe_unused]] const uint64_t rid, const uint32_t code, const string & message){
			if(code >= 300)
				this->_log->print("Request failed: %u %s stream=%i", log_t::flag_t::WARNING, code, message.c_str(), sid);
		}
		
		void active(const client::web_t::mode_t mode, client::awh_t * awh){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			
			if(mode == client::web_t::mode_t::CONNECT){
				uri_t uri(this->_fmk, this->_log);

				client::web_t::request_t req1, req2;

				req1.method = web_t::method_t::GET;
				req2.method = web_t::method_t::GET;

				req1.url = uri.parse("/mac/");
				req2.url = uri.parse("/iphone/");

				awh->send(std::move(req1));
				awh->send(std::move(req2));
			}
		}

		void entity([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, [[maybe_unused]] const uint32_t code, [[maybe_unused]] const string & message, const vector <char> & entity, client::awh_t * awh){
			this->_count++;

			cout << "RESPONSE: " << string(entity.begin(), entity.end()) << endl;

			if(this->_count == 2)
				awh->stop();
		}

		void headers([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, [[maybe_unused]] const uint32_t code, [[maybe_unused]] const string & message, const unordered_multimap <string, string> & headers){
			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;

			cout << endl;
		}

		void complete([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, [[maybe_unused]] const uint32_t code, [[maybe_unused]] const string & message, const vector <char> & entity, const unordered_multimap <string, string> & headers, client::awh_t * awh){
			this->_count++;

			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;
			
			cout << endl;

			cout << "RESPONSE: " << string(entity.begin(), entity.end()) << endl;

			if(this->_count == 2)
				awh->stop();
		}
	public:
		WebClient(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk{};
	log_t log(&fmk);

	client::core_t core(&fmk, &log);
	client::awh_t awh(&core, &fmk, &log);

	WebClient executor(&fmk, &log);

	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	log.name("WEB Client");
	log.format("%H:%M:%S %d.%m.%Y");

	awh.mode({
		client::web_t::flag_t::NOT_INFO,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	node_t::ssl_t ssl;
	ssl.verify = true;
	ssl.ca     = "./certs/ca.pem";
	// ssl.key  = "./certs/certificates/client-key.pem";
	// ssl.cert = "./certs/certificates/client-cert.pem";
	core.ssl(ssl);

	// awh.user("user", "password");
	// awh.authType(auth_t::type_t::BASIC);
	// awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	// awh.proxy("http://user:password@host.com:port");
	// awh.proxy("https://user:password@host.com:port");
	// awh.proxy("socks5://user:password@host.com:port");

	// awh.proxy(client::scheme_t::work_t::ALLOW);
	// awh.proxy(client::scheme_t::work_t::DISALLOW);

	// awh.authTypeProxy(auth_t::type_t::BASIC);
	// awh.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	/*
	awh.compressors({
		http_t::compressor_t::ZSTD,
		http_t::compressor_t::BROTLI,
		http_t::compressor_t::GZIP,
		http_t::compressor_t::DEFLATE
	});
	*/

	awh.on <void (const client::web_t::mode_t)> ("active", &WebClient::active, &executor, _1, &awh);
	awh.on <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", &WebClient::message, &executor, _1, _2, _3, _4);
	// awh.on <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", &WebClient::headers, &executor, _1, _2, _3, _4, _5);
	// awh.on <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entity", &WebClient::entity, &executor, _1, _2, _3, _4, _5, &awh);
	awh.on <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", &WebClient::complete, &executor, _1, _2, _3, _4, _5, _6, &awh);
	
	awh.init("https://apple.com");
	awh.start();

	return EXIT_SUCCESS;
}
```

---

### Example WEB-client
```c++
#include <client/awh.hpp>

using namespace awh;

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk{};
	log_t log(&fmk);
	uri_t uri(&fmk, &log);

	client::core_t core(&fmk, &log);
	client::awh_t awh(&core, &fmk, &log);

	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	log.name("WEB Client");
	log.format("%H:%M:%S %d.%m.%Y");

	awh.mode({
		client::web_t::flag_t::NOT_INFO,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	node_t::ssl_t ssl;
	ssl.verify = true;
	ssl.ca     = "./certs/ca.pem";
	// ssl.key  = "./certs/certificates/client-key.pem";
	// ssl.cert = "./certs/certificates/client-cert.pem";
	core.ssl(ssl);

	// awh.user("user", "password");
	// awh.authType(auth_t::type_t::BASIC);
	// awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	// awh.proxy("http://user:password@host.com:port");
	// awh.proxy("https://user:password@host.com:port");
	// awh.proxy("socks5://user:password@host.com:port");

	// awh.proxy(client::scheme_t::work_t::ALLOW);
	// awh.proxy(client::scheme_t::work_t::DISALLOW);

	// awh.authTypeProxy(auth_t::type_t::BASIC);
	// awh.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	/*
	awh.compressors({
		http_t::compressor_t::ZSTD,
		http_t::compressor_t::BROTLI,
		http_t::compressor_t::GZIP,
		http_t::compressor_t::DEFLATE
	});
	*/

	uri_t::url_t url = uri.parse("https://apple.com/ru/mac");

	const auto & body = awh.GET(url);
	
	if(!body.empty())
		cout << "RESPONSE: " << string(body.begin(), body.end()) << endl;

	return EXIT_SUCCESS;
}
```

---

### Example WEB-server
```c++
#include <server/awh.hpp>

using namespace awh;
using namespace placeholders;

class WebServer {
	private:
		hash_t _hash;
	private:
		const fmk_t * _fmk;
		const log_t * _log;
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

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::web_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent, server::awh_t * awh){

			if((this->_method == awh::web_t::method_t::GET) && (agent == server::web_t::agent_t::HTTP)){

				cout << "URL: " << awh->parser(sid, bid)->request().url << endl << endl;
			
				const string body = "<html>\n<head>\n<title>Hello World!</title>\n</head>\n<body>\n"
				"<h1>\"Hello, World!\" program</h1>\n"
				"<div>\nFrom Wikipedia, the free encyclopedia<br>\n"
				"(Redirected from Hello, world!)<br>\n"
				"Jump to navigationJump to search<br>\n"
				"<strong>\"Hello World\"</strong> redirects here. For other uses, see Hello World (disambiguation).<br>\n"
				"A <strong>\"Hello, World!\"</strong> program generally is a computer program that outputs or displays the message \"Hello, World!\".<br>\n"
				"Such a program is very simple in most programming languages, and is often used to illustrate the basic syntax of a programming language. It is often the first program written by people learning to code. It can also be used as a sanity test to make sure that computer software intended to compile or run source code is correctly installed, and that the operator understands how to use it.\n"
				"</div>\n</body>\n</html>\n";

				if(awh->proto(bid) == engine_t::proto_t::HTTP2){

					vector <pair <string, string>> headers = {
						{":method", "GET"},
						{":scheme", "https"},
						{":path", "/stylesheets/screen.css"},
						{":authority", "example.com"},
						{"accept-encoding", "gzip, deflate"}
					};

					if(awh->push2(sid, bid, headers, awh::http2_t::flag_t::NONE) < 0)
						this->_log->print("Push message is not send", log_t::flag_t::WARNING);
				}

				if(awh->trailers(sid, bid)){

					awh->trailer(sid, bid, "Goga", "Hello");
					awh->trailer(sid, bid, "Hello", "World");
					awh->trailer(sid, bid, "Anyks", "Best of the best");
					awh->trailer(sid, bid, "Checksum", this->_hash.hashing <string> (body, hash_t::type_t::MD5));

				}

				awh->send(sid, bid, 200, "OK", vector <char> (body.begin(), body.end()));
			}
		}

		void request(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, server::awh_t * awh){
			this->_method = method;

			if(!url.empty() && (!url.path.empty() && url.path.back().compare("favicon.ico") == 0))
				awh->send(sid, bid, 404);
		}

		void headers([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t bid, [[maybe_unused]] const awh::web_t::method_t method, [[maybe_unused]] const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;
		}

		void entity(const int32_t sid, const uint64_t bid, [[maybe_unused]] const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity, server::awh_t * awh){
			cout << "URL: " << url << endl << endl;

			cout << "BODY: " << string(entity.begin(), entity.end()) << endl;

			awh->send(sid, bid, 200, "OK", entity, {{"Connection", "close"}});
		}

		void complete(const int32_t sid, const uint64_t bid, [[maybe_unused]] const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers, server::awh_t * awh){
			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;

			cout << "URL: " << url << endl << endl;

			if(!entity.empty()){
				cout << "BODY: " << string(entity.begin(), entity.end()) << endl;

				awh->send(sid, bid, 200, "OK", entity, {{"Connection", "close"}});
			}
		}
	public:
		WebServer(const fmk_t * fmk, const log_t * log) : _hash(log), _fmk(fmk), _log(log), _method(awh::web_t::method_t::NONE) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	server::core_t core(&fmk, &log);
	server::awh_t awh(&core, &fmk, &log);

	WebServer executor(&fmk, &log);

	log.name("WEB Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	
	core.cluster(awh::scheme_t::mode_t::ENABLED);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/server-key.pem";
	ssl.cert   = "./certs/certificates/server-cert.pem";
	core.ssl(ssl);

	core.transferRule(server::core_t::transfer_t::ASYNC);

	// awh.authType(auth_t::type_t::BASIC);
	awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	awh.init(2222, "127.0.0.1", {
		awh::http_t::compressor_t::ZSTD,
		awh::http_t::compressor_t::BROTLI,
		awh::http_t::compressor_t::GZIP,
		awh::http_t::compressor_t::DEFLATE,
	});

	/*
	awh.init("anyks", {
		awh::http_t::compressor_t::ZSTD,
		awh::http_t::compressor_t::BROTLI,
		awh::http_t::compressor_t::GZIP,
		awh::http_t::compressor_t::DEFLATE,
	});
	*/

	// awh.addOrigin("example.net");

	// awh.addAltSvc("example.net", "h2=\":2222\"");
	// awh.addAltSvc("example.com", "h2=\":8000\"");

	awh.on <void (const string &, const uint32_t)> ("launched", &WebServer::launched, &executor, _1, _2);
	awh.on <string (const uint64_t, const string &)> ("extractPassword", &WebServer::password, &executor, _1, _2);
	awh.on <bool (const uint64_t, const string &, const string &)> ("checkPassword", &WebServer::auth, &executor, _1, _2, _3);
	awh.on <void (const uint64_t, const server::web_t::mode_t)> ("active", &WebServer::active, &executor, _1, _2);
	awh.on <bool (const string &, const string &, const uint32_t)> ("accept", &WebServer::accept, &executor, _1, _2, _3);
	awh.on <void (const int32_t, const uint64_t, const server::web_t::agent_t)> ("handshake", &WebServer::handshake, &executor, _1, _2, _3, &awh);
	awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", &WebServer::request, &executor, _1, _2, _3, _4, &awh);
	// awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", &WebServer::headers, &executor, _1, _2, _3, _4, _5);
	// awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", &WebServer::entity, &executor, _1, _2, _3, _4, _5, &awh);
	awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", &WebServer::complete, &executor, _1, _2, _3, _4, _5, _6, &awh);

	awh.start();

	return EXIT_SUCCESS;
}
```

---

### Example WebSocket-client
```c++
#include <client/ws.hpp>

using namespace awh;
using namespace placeholders;

class Executor {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void status(const awh::core_t::status_t status){
			switch(static_cast <uint8_t> (status)){
				case static_cast <uint8_t> (awh::core_t::status_t::START):
					this->_log->print("START", log_t::flag_t::INFO);
				break;
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					this->_log->print("STOP", log_t::flag_t::INFO);
				break;
			}
		}

		void handshake([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, const client::web_t::agent_t agent, client::websocket_t * ws){
			if(agent == client::web_t::agent_t::WEBSOCKET){
				this->_log->print("Handshake", log_t::flag_t::INFO);
				
				const string query = "Hello World!!!";

				ws->sendMessage(vector <char> (query.begin(), query.end()));
			}
		}
	public:

		void error(const uint32_t code, const string & mess){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}

		void message(const vector <char> & buffer, const bool utf8, client::websocket_t * ws){
			string subprotocol = "";

			const auto subprotocols = ws->subprotocols();

			if(!subprotocols.empty())
				subprotocol = (* subprotocols.begin());

			if(utf8){
				
				cout << "MESSAGE: " << string(buffer.begin(), buffer.end()) << endl;

				cout << "SUB PROTOCOL: " << subprotocol << endl;
			}
		}
	public:
		Executor(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	client::core_t core(&fmk, &log);
	client::websocket_t ws(&core, &fmk, &log);

	Executor executor(&fmk, &log);

	log.name("WebSocket Client");
	log.format("%H:%M:%S %d.%m.%Y");

	ws.mode({
		client::web_t::flag_t::ALIVE,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::TAKEOVER_CLIENT,
		client::web_t::flag_t::TAKEOVER_SERVER,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/client-key.pem";
	ssl.cert   = "./certs/certificates/client-cert.pem";
	core.ssl(ssl);

	// ws.proxy("http://user:password@host.com:port");
	// ws.proxy("https://user:password@host.com:port");
	// ws.proxy("socks5://user:password@host.com:port");

	// ws.proxy(client::scheme_t::work_t::ALLOW);
	// ws.proxy(client::scheme_t::work_t::DISALLOW);

	// ws.authTypeProxy(auth_t::type_t::BASIC);
	// ws.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	
	ws.user("user", "password");

	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);

	ws.init("wss://127.0.0.1:2222", {awh::http_t::compressor_t::DEFLATE});

	ws.subprotocols({"test2", "test8", "test9"});
	// ws.extensions({{"test1", "test2", "test3"},{"good1", "good2", "good3"}});

	ws.on <void (const awh::core_t::status_t)> ("status", &Executor::status, &executor, _1);
	ws.on <void (const uint32_t, const string &)> ("errorWebsocket", &Executor::error, &executor, _1, _2);
	ws.on <void (const vector <char> &, const bool)> ("messageWebsocket", &Executor::message, &executor, _1, _2, &ws);
	ws.on <void (const int32_t, const uint64_t, const client::web_t::agent_t)> ("handshake", &Executor::handshake, &executor, _1, _2, _3, &ws);

	ws.start();

	return EXIT_SUCCESS;
}
```

---

### Example WebSocket-server
```c++
#include <server/ws.hpp>

using namespace awh;
using namespace placeholders;

class Executor {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
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

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::web_t::mode_t mode){
			switch(static_cast <uint8_t> (mode)){
				case static_cast <uint8_t> (server::web_t::mode_t::CONNECT):
					this->_log->print("CONNECT", log_t::flag_t::INFO);
				break;
				case static_cast <uint8_t> (server::web_t::mode_t::DISCONNECT):
					this->_log->print("DISCONNECT", log_t::flag_t::INFO);
				break;
			}
		}

		void error([[maybe_unused]] const uint64_t bid, const uint32_t code, const string & mess){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}

		void message(const uint64_t bid, const vector <char> & buffer, const bool text, server::websocket_t * ws){
			if(!buffer.empty()){
				string subprotocol = "";

				const auto subprotocols = ws->subprotocols(bid);

				if(!subprotocols.empty())
					subprotocol = (* subprotocols.begin());

				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), subprotocol.c_str());

				ws->sendMessage(bid, buffer, text);
			}
		}

		void headers([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t bid, [[maybe_unused]] const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			uri_t uri(this->_fmk, this->_log);

			this->_log->print("REQUEST ID=%zu URL=%s", log_t::flag_t::INFO, bid, uri.url(url).c_str());

			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;
		}
	public:
		Executor(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	server::core_t core(&fmk, &log);
	server::websocket_t ws(&core, &fmk, &log);

	Executor executor(&fmk, &log);

	log.name("WebSocket Server");
	log.format("%H:%M:%S %d.%m.%Y");

	ws.mode({
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER
	});

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	core.cluster(awh::scheme_t::mode_t::ENABLED);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/server-key.pem";
	ssl.cert   = "./certs/certificates/server-cert.pem";
	core.ssl(ssl);

	core.transferRule(server::core_t::transfer_t::ASYNC);

	ws.subprotocols({"test1", "test2", "test3"});

	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);

	// ws.init("anyks", {awh::http_t::compressor_t::DEFLATE});
	// ws.init(2222, "", {awh::http_t::compressor_t::DEFLATE});
	ws.init(2222, "127.0.0.1", {awh::http_t::compressor_t::DEFLATE});

	ws.on <void (const string &, const uint32_t)> ("launched", &Executor::launched, &executor, _1, _2);
	ws.on <string (const uint64_t, const string &)> ("extractPassword", &Executor::password, &executor, _1, _2);
	ws.on <bool (const uint64_t, const string &, const string &)> ("checkPassword", &Executor::auth, &executor, _1, _2, _3);
	ws.on <bool (const string &, const string &, const uint32_t)> ("accept", &Executor::accept, &executor, _1, _2, _3);
	ws.on <void (const uint64_t, const server::web_t::mode_t)> ("active", &Executor::active, &executor, _1, _2);
	ws.on <void (const uint64_t, const uint32_t, const string &)> ("errorWebsocket", &Executor::error, &executor, _1, _2, _3);
	ws.on <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", &Executor::message, &executor, _1, _2, _3, &ws);
	ws.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", &Executor::headers, &executor, _1, _2, _3, _4, _5);

	ws.start();

	return EXIT_SUCCESS;
}
```

---

### Example multiprotocol HTTPS-server
```c++
#include <server/awh.hpp>

using namespace awh;
using namespace placeholders;

class WebServer {
	private:
		hash_t _hash;
	private:
		const fmk_t * _fmk;
		const log_t * _log;
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

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::web_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void error([[maybe_unused]] const uint64_t bid, const uint32_t code, const string & mess){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}

		void message(const uint64_t bid, const vector <char> & buffer, const bool text, server::awh_t * awh){
			if(!buffer.empty()){
				string subprotocol = "";

				const auto subprotocols = awh->subprotocols(bid);

				if(!subprotocols.empty())
					subprotocol = (* subprotocols.begin());

				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), subprotocol.c_str());

				awh->sendMessage(bid, buffer, text);
			}
		}

		void handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent, server::awh_t * awh){
			if((this->_method == awh::web_t::method_t::GET) && (agent == server::web_t::agent_t::HTTP)){
				cout << " URL: " << awh->parser(sid, bid)->request().url << endl;

				const string body = "<html>\n<head>\n<title>Hello World!</title>\n</head>\n<body>\n"
				"<h1>\"Hello, World!\" program</h1>\n"
				"<div>\nFrom Wikipedia, the free encyclopedia<br>\n"
				"(Redirected from Hello, world!)<br>\n"
				"Jump to navigationJump to search<br>\n"
				"<strong>\"Hello World\"</strong> redirects here. For other uses, see Hello World (disambiguation).<br>\n"
				"A <strong>\"Hello, World!\"</strong> program generally is a computer program that outputs or displays the message \"Hello, World!\".<br>\n"
				"Such a program is very simple in most programming languages, and is often used to illustrate the basic syntax of a programming language. It is often the first program written by people learning to code. It can also be used as a sanity test to make sure that computer software intended to compile or run source code is correctly installed, and that the operator understands how to use it.\n"
				"</div>\n</body>\n</html>\n";

				if(awh->trailers(sid, bid)){
					awh->trailer(sid, bid, "Goga", "Hello");
					awh->trailer(sid, bid, "Hello", "World");
					awh->trailer(sid, bid, "Anyks", "Best of the best");
					awh->trailer(sid, bid, "Checksum", this->_hash.hashing <string> (body, hash_t::type_t::MD5));
				}

				awh->send(sid, bid, 200, "OK", vector <char> (body.begin(), body.end()));
			}
		}

		void request(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, server::awh_t * awh){
			this->_method = method;

			if(!url.empty() && (!url.path.empty() && url.path.back().compare("favicon.ico") == 0))
				awh->send(sid, bid, 404);
		}

		void headers([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t bid, [[maybe_unused]] const awh::web_t::method_t method, [[maybe_unused]] const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;
		}

		void entity(const int32_t sid, const uint64_t bid, [[maybe_unused]] const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity, server::awh_t * awh){
			cout << "URL: " << url << endl << endl;

			cout << "BODY: " << string(entity.begin(), entity.end()) << endl;

			awh->send(sid, bid, 200, "OK", entity, {{"Connection", "close"}});
		}

		void complete(const int32_t sid, const uint64_t bid, [[maybe_unused]] const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers, server::awh_t * awh){
			for(auto & header : headers)
				cout << "HEADER: " << header.first << ": " << header.second << endl;

			cout << "URL: " << url << endl << endl;

			if(!entity.empty()){
				cout << "BODY: " << string(entity.begin(), entity.end()) << endl;

				awh->send(sid, bid, 200, "OK", entity, {{"Connection", "close"}});
			}
		}
	public:
		WebServer(const fmk_t * fmk, const log_t * log) : _hash(log), _fmk(fmk), _log(log), _method(awh::web_t::method_t::NONE) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	server::core_t core(&fmk, &log);
	server::awh_t awh(&core, &fmk, &log);

	WebServer executor(&fmk, &log);

	log.name("WEB Server");
	log.format("%H:%M:%S %d.%m.%Y");

	awh.mode({
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER,
		server::web_t::flag_t::WEBSOCKET_ENABLE,
		server::web_t::flag_t::CONNECT_METHOD_ENABLE
	});

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);

	core.cluster(awh::scheme_t::mode_t::ENABLED);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/server-key.pem";
	ssl.cert   = "./certs/certificates/server-cert.pem";
	core.ssl(ssl);

	core.transferRule(server::core_t::transfer_t::ASYNC);

	// awh.authType(auth_t::type_t::BASIC);
	awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	awh.init(2222, "127.0.0.1", {
		awh::http_t::compressor_t::ZSTD,
		awh::http_t::compressor_t::BROTLI,
		awh::http_t::compressor_t::GZIP,
		awh::http_t::compressor_t::DEFLATE,
	});

	/*
	awh.init("anyks", {
		awh::http_t::compressor_t::ZSTD,
		awh::http_t::compressor_t::BROTLI,
		awh::http_t::compressor_t::GZIP,
		awh::http_t::compressor_t::DEFLATE,
	});
	*/

	awh.addOrigin("example.net");

	awh.addAltSvc("example.net", "h2=\":2222\"");
	awh.addAltSvc("example.com", "h2=\":8000\"");

	awh.subprotocols({"test1", "test2", "test3"});

	awh.on <void (const string &, const uint32_t)> ("launched", &WebServer::launched, &executor, _1, _2);
	awh.on <string (const uint64_t, const string &)> ("extractPassword", &WebServer::password, &executor, _1, _2);
	awh.on <bool (const uint64_t, const string &, const string &)> ("checkPassword", &WebServer::auth, &executor, _1, _2, _3);
	awh.on <void (const uint64_t, const server::web_t::mode_t)> ("active", &WebServer::active, &executor, _1, _2);
	awh.on <bool (const string &, const string &, const uint32_t)> ("accept", &WebServer::accept, &executor, _1, _2, _3);
	awh.on <void (const uint64_t, const uint32_t, const string &)> ("errorWebsocket", &WebServer::error, &executor, _1, _2, _3);
	awh.on <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", &WebServer::message, &executor, _1, _2, _3, &awh);
	awh.on <void (const int32_t, const uint64_t, const server::web_t::agent_t)> ("handshake", &WebServer::handshake, &executor, _1, _2, _3, &awh);
	awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", &WebServer::request, &executor, _1, _2, _3, _4, &awh);
	// awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", &WebServer::entity, &executor, _1, _2, _3, _4, _5, &awh);
	// awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", &WebServer::headers, &executor, _1, _2, _3, _4, _5);
	awh.on <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", &WebServer::complete, &executor, _1, _2, _3, _4, _5, _6, &awh);

	awh.start();

	return EXIT_SUCCESS;
}
```

---

### Example HTTPS PROXY-server
```c++
#include <server/proxy.hpp>

using namespace awh;
using namespace placeholders;

class Proxy {
	private:
		log_t * _log;
	public:

		string password([[maybe_unused]] const uint64_t bid, const string & login){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");

			return "password";
		}

		bool auth([[maybe_unused]] const uint64_t bid, const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());

			return true;
		}
	public:
		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active([[maybe_unused]] const uint64_t bid, const server::proxy_t::broker_t broker, const server::web_t::mode_t mode){
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

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Proxy executor(&log);
	server::proxy_t proxy(&fmk, &log);

	log.name("Proxy Server");
	log.format("%H:%M:%S %d.%m.%Y");

	proxy.cluster(awh::scheme_t::mode_t::ENABLED);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.ca     = "./certs/ca.pem";
	ssl.key    = "./certs/certificates/server-key.pem";
	ssl.cert   = "./certs/certificates/server-cert.pem";
	proxy.ssl(ssl);

	proxy.mode({
		server::proxy_t::flag_t::SYNCPROTO,
		server::proxy_t::flag_t::REDIRECTS,
		server::proxy_t::flag_t::CONNECT_METHOD_SERVER_ENABLE
	});

	proxy.hosts(server::proxy_t::broker_t::CLIENT, "/etc/hosts");

	// proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::BASIC);
	// proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::DIGEST, auth_t::hash_t::SHA512);
	proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	// proxy.init("anyks", http_t::compressor_t::GZIP);
	// proxy.init(2222, "", http_t::compressor_t::GZIP);
	proxy.init(2222, "127.0.0.1", http_t::compressor_t::GZIP);

	proxy.on <string (const uint64_t, const string &)> ("extractPassword", &Proxy::password, &executor, _1, _2);
	proxy.on <bool (const uint64_t, const string &, const string &)> ("checkPassword", &Proxy::auth, &executor, _1, _2, _3);
	proxy.on <bool (const string &, const string &, const uint32_t)> ("accept", &Proxy::accept, &executor, _1, _2, _3);
	proxy.on <void (const uint64_t, const server::proxy_t::broker_t, const server::web_t::mode_t)> ("active", &Proxy::active, &executor, _1, _2, _3);

	proxy.start();

	return EXIT_SUCCESS;
}
```

---

### Example Socks5 PROXY-server
```c++
#include <server/socks5.hpp>

using namespace awh;
using namespace server;
using namespace placeholders;

class Proxy {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool auth(const uint64_t bid, const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), password.c_str(), bid);

			return true;
		}
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void active([[maybe_unused]] const uint64_t bid, const proxy_socks5_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == proxy_socks5_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
	public:
		Proxy(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	proxy_socks5_t proxy(&fmk, &log);
	Proxy executor(&fmk, &log);

	log.name("Proxy Socks5 Server");
	log.format("%H:%M:%S %d.%m.%Y");

	node_t::ssl_t ssl;
	ssl.verify = true;
	ssl.ca     = "./certs/ca.pem";
	proxy.ssl(ssl);

	proxy.sonet(awh::scheme_t::sonet_t::TCP);

	proxy.cluster(awh::scheme_t::mode_t::ENABLED);

	// proxy.init("anyks");
	// proxy.init(2222);
	proxy.init(2222, "127.0.0.1");

	proxy.on <void (const size_t, const proxy_socks5_t::mode_t)> ("active", &Proxy::active, &executor, _1, _2);
	proxy.on <bool (const string &, const string &, const uint32_t)> ("accept", &Proxy::accept, &executor, _1, _2, _3);
	// proxy.on <bool (const uint64_t, const string &, const string &)> ("checkPassword", &Proxy::auth, &executor, _1, _2, _3);

	proxy.start();

	return EXIT_SUCCESS;
}
```

---

### Example Timer
```c++
#include <chrono>
#include <core/timer.hpp>

using namespace awh;
using namespace placeholders;

class Executor {
	private:
		chrono::time_point <chrono::system_clock> _ts;
		chrono::time_point <chrono::system_clock> _is;
	private:
		uint16_t _count;
	private:
		log_t * _log;
	public:

		void interval(const uint16_t tid, awh::timer_t * timer){
			auto shift = chrono::system_clock::now();

			this->_log->print("Interval: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (shift - this->_is).count());

			this->_is = shift;

			if((this->_count++) >= 10){
				timer->clear(tid);
				timer->stop();
			}
		}

		void timeout([[maybe_unused]] const uint16_t id){
			this->_log->print("Timeout: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - this->_ts).count());
		}

		void launched(const awh::core_t::status_t status, awh::timer_t * timer){
			switch(static_cast <uint8_t> (status)){
				case static_cast <uint8_t> (awh::core_t::status_t::START): {
					this->_ts = chrono::system_clock::now();
					this->_is = this->_ts;

					this->_log->print("%s", log_t::flag_t::INFO, "Start timer");

					uint16_t tid = timer->timeout(10000);

					timer->on(tid, &Executor::timeout, this, tid);

					tid = timer->interval(5000);

					timer->on(tid, &Executor::interval, this, tid, timer);
				} break;
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					this->_log->print("%s", log_t::flag_t::INFO, "Stop timer");
				break;
			}
		}
	public:
		Executor(log_t * log) : _ts(chrono::system_clock::now()), _is(chrono::system_clock::now()), _count(0), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Executor executor(&log);

	awh::timer_t timer(&fmk, &log);

	log.name("Timer");
	log.format("%H:%M:%S %d.%m.%Y");

	dynamic_cast <awh::core_t &> (timer).on <void (const awh::core_t::status_t)> ("status", &Executor::launched, &executor, _1, &timer);

	timer.start();

	return EXIT_SUCCESS;
}
```

---

### Example DNS-resolver
```c++
#include <net/dns.hpp>
#include <core/core.hpp>

using namespace awh;

int32_t main(int32_t argc, char * argv[]){
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

	const auto & yandex = dns.search("77.88.55.60");
	if(!yandex.empty()){
		for(auto & domain : yandex)
			log.print("Domain: %s => %s", log_t::flag_t::INFO, domain.c_str(), "77.88.55.60");
	}
	
	log.print("Encode domain \"ремпрофи.рф\" == \"%s\"", log_t::flag_t::INFO, dns.encode("ремпрофи.рф").c_str());
	log.print("Decode domain \"xn--e1agliedd7a.xn--p1ai\" == \"%s\"", log_t::flag_t::INFO, dns.decode("xn--e1agliedd7a.xn--p1ai").c_str());

	return EXIT_SUCCESS;
}
```

---

### Example NTP-client
```c++
#include <net/ntp.hpp>
#include <core/core.hpp>

using namespace awh;

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	chrono_t chrono(&fmk);
	ntp_t ntp(&fmk, &log);
	core_t core(&fmk, &log);

	log.name("NTP");
	log.format("%H:%M:%S %d.%m.%Y");

	ntp.ns({"77.88.8.88", "77.88.8.2"});

	ntp.servers({"0.ru.pool.ntp.org", "1.ru.pool.ntp.org", "2.ru.pool.ntp.org", "3.ru.pool.ntp.org"});

	log.print("Time: %s", log_t::flag_t::INFO, chrono.format(ntp.request(), "%H:%M:%S %d.%m.%Y").c_str());

	return EXIT_SUCCESS;
}
```

---

### Example ICMP-client PINGER
```c++
#include <net/ping.hpp>
#include <core/core.hpp>

using namespace awh;

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	core_t core(&fmk, &log);
	ping_t ping(&fmk, &log);

	log.name("PING");
	log.format("%H:%M:%S %d.%m.%Y");

	const double result = ping.ping("api.telegram.org", 10);

	log.print("PING result=%.1f", log_t::flag_t::INFO, result);

	return EXIT_SUCCESS;
}
```

---

### Example TCP-client
```c++
#include <client/sample.hpp>

using namespace awh;
using namespace placeholders;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer, client::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log);

	log.name("TCP Client");
	log.format("%H:%M:%S %d.%m.%Y");

	// sample.mode({client::sample_t::flag_t::NOT_INFO});

	core.sonet(awh::scheme_t::sonet_t::TCP);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const vector <char> &)> ("message", &Client::message, &executor, _1, &sample);
	sample.on <void (const client::sample_t::mode_t)> ("active", &Client::active, &executor, _1, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example TCP-server
```c++
#include <server/sample.hpp>

using namespace awh;
using namespace placeholders;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TCP);
	core.cluster(awh::scheme_t::mode_t::ENABLED);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example TLS-client
```c++
#include <client/sample.hpp>

using namespace awh;
using namespace placeholders;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer, client::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log);

	log.name("TLS Client");
	log.format("%H:%M:%S %d.%m.%Y");

	// sample.mode({client::sample_t::flag_t::NOT_INFO});

	core.sonet(awh::scheme_t::sonet_t::TLS);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/client-key.pem";
	ssl.cert   = "./certs/certificates/client-cert.pem";
	core.ssl(ssl);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const vector <char> &)> ("message", &Client::message, &executor, _1, &sample);
	sample.on <void (const client::sample_t::mode_t)> ("active", &Client::active, &executor, _1, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example TLS-server
```c++
#include <server/sample.hpp>

using namespace awh;
using namespace placeholders;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log);

	log.name("TLS Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.cluster(awh::scheme_t::mode_t::ENABLED);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/server-key.pem";
	ssl.cert   = "./certs/certificates/server-cert.pem";
	core.ssl(ssl);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example UDP-client
```c++
#include <client/sample.hpp>

using namespace awh;
using namespace placeholders;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer, client::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log);

	log.name("UDP Client");
	log.format("%H:%M:%S %d.%m.%Y");

	// sample.mode({client::sample_t::flag_t::NOT_INFO});

	core.sonet(awh::scheme_t::sonet_t::UDP);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const vector <char> &)> ("message", &Client::message, &executor, _1, &sample);
	sample.on <void (const client::sample_t::mode_t)> ("active", &Client::active, &executor, _1, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example UDP-server
```c++
#include <server/sample.hpp>

using namespace awh;
using namespace placeholders;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log);

	log.name("UDP Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::UDP);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example SCTP-client
```c++
#include <client/sample.hpp>

using namespace awh;
using namespace placeholders;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer, client::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log);

	log.name("SCTP Client");
	log.format("%H:%M:%S %d.%m.%Y");

	// sample.mode({client::sample_t::flag_t::NOT_INFO});

	core.sonet(awh::scheme_t::sonet_t::SCTP);
	
	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/client-key.pem";
	ssl.cert   = "./certs/certificates/client-cert.pem";
	core.ssl(ssl);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const vector <char> &)> ("message", &Client::message, &executor, _1, &sample);
	sample.on <void (const client::sample_t::mode_t)> ("active", &Client::active, &executor, _1, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example SCTP-server
```c++
#include <server/sample.hpp>

using namespace awh;
using namespace placeholders;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log);

	log.name("SCTP Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::SCTP);
	core.cluster(awh::scheme_t::mode_t::ENABLED);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/server-key.pem";
	ssl.cert   = "./certs/certificates/server-cert.pem";
	core.ssl(ssl);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example DTLS-client
```c++
#include <client/sample.hpp>

using namespace awh;
using namespace placeholders;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer, client::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log);

	log.name("DTLS Client");
	log.format("%H:%M:%S %d.%m.%Y");

	// sample.mode({client::sample_t::flag_t::NOT_INFO});

	core.sonet(awh::scheme_t::sonet_t::DTLS);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/client-key.pem";
	ssl.cert   = "./certs/certificates/client-cert.pem";
	core.ssl(ssl);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const vector <char> &)> ("message", &Client::message, &executor, _1, &sample);
	sample.on <void (const client::sample_t::mode_t)> ("active", &Client::active, &executor, _1, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example DTLS-server
```c++
#include <server/sample.hpp>

using namespace awh;
using namespace placeholders;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log);

	log.name("DTLS Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::DTLS);

	node_t::ssl_t ssl;
	ssl.verify = false;
	ssl.key    = "./certs/certificates/server-key.pem";
	ssl.cert   = "./certs/certificates/server-cert.pem";
	core.ssl(ssl);

	sample.init(2222, "127.0.0.1");

	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example UnixSocket TCP-client
```c++
#include <client/sample.hpp>

using namespace awh;
using namespace placeholders;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer, client::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log);

	log.name("UnixSocket Client");
	log.format("%H:%M:%S %d.%m.%Y");

	// sample.mode({client::sample_t::flag_t::NOT_INFO});
	
	core.sonet(awh::scheme_t::sonet_t::TCP);
	core.family(awh::scheme_t::family_t::IPC);

	sample.init("anyks");

	sample.on <void (const vector <char> &)> ("message", &Client::message, &executor, _1, &sample);
	sample.on <void (const client::sample_t::mode_t)> ("active", &Client::active, &executor, _1, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example UnixSocket TCP-server
```c++
#include <server/sample.hpp>

using namespace awh;
using namespace placeholders;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log);

	log.name("UnixSocket Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TCP);
	core.family(awh::scheme_t::family_t::IPC);
	core.cluster(awh::scheme_t::mode_t::ENABLED);

	sample.init("anyks");

	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example UnixSocket UDP-client
```c++
#include <client/sample.hpp>

using namespace awh;
using namespace placeholders;

class Client {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		void active(const client::sample_t::mode_t mode, client::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));

			if(mode == client::sample_t::mode_t::CONNECT){
				const string message = "Hello World!!!";

				sample->send(message.data(), message.size());
			}
		}

		void message(const vector <char> & buffer, client::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			
			sample->stop();
		}
	public:
		Client(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	client::core_t core(&dns, &fmk, &log);
	client::sample_t sample(&core, &fmk, &log);
	Client executor(&fmk, &log);

	log.name("UDP UnixSocket Client");
	log.format("%H:%M:%S %d.%m.%Y");

	// sample.mode({client::sample_t::flag_t::NOT_INFO});
	
	core.sonet(awh::scheme_t::sonet_t::UDP);
	core.family(awh::scheme_t::family_t::IPC);

	sample.init("anyks");

	sample.on <void (const vector <char> &)> ("message", &Client::message, &executor, _1, &sample);
	sample.on <void (const client::sample_t::mode_t)> ("active", &Client::active, &executor, _1, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example UnixSocket UDP-server
```c++
#include <server/sample.hpp>

using namespace awh;
using namespace placeholders;

class Server {
	private:
		const fmk_t * _fmk;
		const log_t * _log;
	public:

		bool accept(const string & ip, const string & mac, const uint32_t port){
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);

			return true;
		}

		void launched(const string & host, const uint32_t port){
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}

		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}

		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());

			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	dns_t dns(&fmk, &log);

	server::core_t core(&dns, &fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	Server executor(&fmk, &log);

	log.name("UDP UnixSocket Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::UDP);
	core.family(awh::scheme_t::family_t::IPC);

	sample.init("anyks");

	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);

	sample.start();

	return EXIT_SUCCESS;
}
```

---

### Example Cluster
```c++
#include <core/cluster.hpp>

using namespace awh;
using namespace placeholders;

class Executor {
	private:
		log_t * _log;
	public:

		void events(const cluster_t::family_t worker, [[maybe_unused]] const pid_t pid, const cluster_t::event_t event, cluster::core_t * core){
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

		void message(const cluster_t::family_t worker, const pid_t pid, const char * buffer, const size_t size){
			switch(static_cast <uint8_t> (worker)){
				case static_cast <uint8_t> (cluster_t::family_t::MASTER):
					this->_log->print("Message from children [%u]: %s", log_t::flag_t::INFO, pid, string(buffer, size).c_str());
				break;
				case static_cast <uint8_t> (cluster_t::family_t::CHILDREN):
					this->_log->print("Message from master: %s [%u]", log_t::flag_t::INFO, string(buffer, size).c_str(), ::getpid());
				break;
			}
		}

		void launched(const awh::core_t::status_t status){
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

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Executor executor(&log);
	cluster::core_t core(&fmk, &log);

	log.name("Cluster");
	log.format("%H:%M:%S %d.%m.%Y");

	core.size();
	core.autoRestart(true);

	// Setting the cluster name
	core.name("ANYKS");

	// Activating the mode of exchanging messages between processes via a unix socket
	// core.transfer(cluster_t::transfer_t::IPC);

	core.on <void (const awh::core_t::status_t)> ("status", &Executor::launched, &executor, _1);
	core.on <void (const cluster_t::family_t, const pid_t, const cluster_t::event_t)> ("events", &Executor::events, &executor, _1, _2, _3, &core);
	core.on <void (const cluster_t::family_t, const pid_t, const char *, const size_t)> ("message", &Executor::message, &executor, _1, _2, _3, _4);

	core.start();

	return EXIT_SUCCESS;
}
```

---

### Example IP-address
```c++
#include <net/net.hpp>

using namespace awh;

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	net_t net(&log);

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
	cout << " Part of the address: 2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d || " << net.v6()[0] << " and " << net.v6()[1] << endl;

	net = "46.39.230.51";
	cout << " Part of the address: 46.39.230.51 || " << net.v4() << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose(53, net_t::addr_t::NETWORK);
	cout << " Prefix set: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 || " << net << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose("FFFF:FFFF:FFFF:F800::", net_t::addr_t::NETWORK);
	cout << " Mask set: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: || " << net << endl;

	net = "192.168.3.192";
	net.impose(9, net_t::addr_t::NETWORK);
	cout << " Prefix set: 192.168.3.192/9 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.128.0.0", net_t::addr_t::NETWORK);
	cout << " Mask set: 192.168.3.192/255.128.0.0 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.255.255.0", net_t::addr_t::NETWORK);
	cout << " Mask set: 192.168.3.192/255.255.255.0 || " << net << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose(53, net_t::addr_t::HOST);
	cout << " Get host from IP-address: 53/2001:1234:abcd:5678:9877:3322:5541:aabb || " << net << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose("FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST);
	cout << " Get host from IP-address: FFFF:FFFF:FFFF:F800::/2001:1234:abcd:5678:9877:3322:5541:aabb || " << net << endl;

	net = "192.168.3.192";
	net.impose(9, net_t::addr_t::HOST);
	cout << " Get host from IP-address: 9/192.168.3.192 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.128.0.0", net_t::addr_t::HOST);
	cout << " Get host from IP-address: 255.128.0.0/192.168.3.192 || " << net << endl;

	net = "192.168.3.192";
	net.impose(24, net_t::addr_t::HOST);
	cout << " Get host from IP-address: 24/192.168.3.192 || " << net << endl;

	net = "192.168.3.192";
	net.impose("255.255.255.0", net_t::addr_t::HOST);
	cout << " Get host from IP-address: 255.255.255.0/192.168.3.192 || " << net << endl;

	net = "192.168.3.192";
	cout << " Get address mask from network prefix = 9 || " << net.prefix2Mask(9) << endl;
	cout << " Get address prefix from network mask = 255.128.0.0 || " << static_cast <uint16_t> (net.mask2Prefix("255.128.0.0")) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << " Get address mask from network prefix = 53 || " << net.prefix2Mask(53) << endl;
	cout << " Get address prefix from network mask = FFFF:FFFF:FFFF:F800:: || " << static_cast <uint16_t> (net.mask2Prefix("FFFF:FFFF:FFFF:F800::")) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Check the address compliance 192.168.3.192 by network 192.168.0.0 || " << net.mapping("192.168.0.0") << endl;
	
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Check the address compliance 2001:1234:abcd:5678:9877:3322:5541:aabb by network 2001:1234:abcd:5678:: || " << net.mapping("2001:1234:abcd:5678::") << endl;
	
	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Check the address compliance 192.168.3.192 by network 192.128.0.0/9 || " << net.mapping("192.128.0.0", 9, net_t::addr_t::NETWORK) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Check the address compliance 2001:1234:abcd:5678:9877:3322:5541:aabb by network 2001:1234:abcd:5000::/53 || " << net.mapping("2001:1234:abcd:5000::", 53, net_t::addr_t::NETWORK) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Check the address compliance 192.168.3.192 by network 192.128.0.0/255.128.0.0 || " << net.mapping("192.128.0.0", "255.128.0.0", net_t::addr_t::NETWORK) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Check the address compliance 2001:1234:abcd:5678:9877:3322:5541:aabb by network 2001:1234:abcd:5000::/FFFF:FFFF:FFFF:F800:: || " << net.mapping("2001:1234:abcd:5000::", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::NETWORK) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Check the address compliance 192.168.3.192 by host 9/0.40.3.192 || " << net.mapping("0.40.3.192", 9, net_t::addr_t::HOST) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Check the address compliance 2001:1234:abcd:5678:9877:3322:5541:aabb by host 53/::678:9877:3322:5541:AABB || " << net.mapping("::678:9877:3322:5541:AABB", 53, net_t::addr_t::HOST) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Check the address compliance 192.168.3.192 by host 255.128.0.0/0.40.3.192 || " << net.mapping("0.40.3.192", "255.128.0.0", net_t::addr_t::HOST) << endl;

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Check the address compliance 2001:1234:abcd:5678:9877:3322:5541:aabb by host FFFF:FFFF:FFFF:F800::/::678:9877:3322:5541:AABB || " << net.mapping("::678:9877:3322:5541:AABB", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST) << endl;

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Check whether the address 192.168.3.192 is in range [192.168.3.100 - 192.168.3.200] || " << net.range("192.168.3.100", "192.168.3.200", 24) << endl;

	net = "46.39.230.51";
	cout << boolalpha;
	cout << " Checking whether IP-address is global 46.39.230.51 || " << (net.mode() == net_t::mode_t::WAN) << endl;

	net = "192.168.31.12";
	cout << boolalpha;
	cout << " Checking whether IP-address is local 192.168.31.12 || " << (net.mode() == net_t::mode_t::LAN) << endl;

	net = "0.0.0.0";
	cout << boolalpha;
	cout << " Checking whether IP-address is system 0.0.0.0 || " << (net.mode() == net_t::mode_t::SYS) << endl;

	net = "[2a00:1450:4010:c0a::8b]";
	cout << boolalpha;
	cout << " Checking whether IP-address is global [2a00:1450:4010:c0a::8b] || " << (net.mode() == net_t::mode_t::WAN) << endl;

	net = "::1";
	cout << boolalpha;
	cout << " Checking whether IP-address is local [::1] || " << (net.mode() == net_t::mode_t::LAN) << endl;

	net = "::";
	cout << boolalpha;
	cout << " Checking whether IP-address is system [::] || " << (net.mode() == net_t::mode_t::SYS) << endl;

	string ip = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	cout << " Long record address || " << ip << endl;
	ip = net = ip;
	cout << " Short record address || " << ip << endl;

	net = "73:0b:04:0d:db:79";
	cout << " MAC: 73:0b:04:0d:db:79 || " << net << endl;

	net.arpa("70.255.255.5.in-addr.arpa");
	cout << " ARPA: 70.255.255.5.in-addr.arpa || " << net << endl;
	cout << " ARPA IPv4: " << net.arpa() << endl;

	net.arpa("b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa");
	cout << " ARPA: b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa || " << net << endl;
	cout << " ARPA IPv6: " << net.arpa() << endl;

	return EXIT_SUCCESS;
}
```

---

### Example Investigator
```c++
#include <sys/investigator.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

using namespace awh;

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	igtr_t igtr;
	log_t log(&fmk);

	log.name("Investigator");
	log.format("%H:%M:%S %d.%m.%Y");

	if(argc > 1){
		const pid_t pid = static_cast <pid_t> (::stoi(argv[1]));
		log.print("Investigator: NAME=%s", log_t::flag_t::INFO, igtr.inquiry(pid).c_str());
	} else log.print("Investigator: NAME=%s", log_t::flag_t::INFO, igtr.inquiry().c_str());

	return EXIT_SUCCESS;
}
```

---

### Date and Time Formatting Rules
| **Format Date and Time** | **Description**                                                                                                                           |
|--------------------------|-------------------------------------------------------------------------------------------------------------------------------------------|
| **%y**                   | Abbreviated year entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **25**.                                     |
| **%Y**                   | Full year entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **2025**.                                          |
| **%b**                   | Abbreviated month name entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **Mar**.                              |
| **%B**                   | Full month name entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **Mar**.                                     |
| **%m**                   | Full month entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **03**.                                           |
| **%d**                   | Full date entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **02**.                                            |
| **%e**                   | Abbreviated date entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **2**.                                      |
| **%a**                   | Abbreviated day name entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **Sun**.                                |
| **%A**                   | Full day name entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **Sunday**.                                    |
| **%u**                   | Count of days from the beginning of the week, starting from Monday. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **7**. |
| **%w**                   | Count of days from the beginning of the week, starting from Sunday. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **0**. |
| **%W**                   | Week count since the beginning of the year. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **09**.                 |
| **%j**                   | Count of days since the beginning of the year. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **061**.             |
| **%D**                   | Equivalent to **%m/%d/%y**. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **03/02/25**.                           |
| **%F**                   | Equivalent to **%Y-%m-%d**. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **2025-03-02**.                         |
| **%H**                   | Full hour entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **15**.                                            |
| **%I**                   | Parses the hour (12-hour clock) as a decimal number. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **03**.        |
| **%M**                   | Full minutes entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **44**.                                         |
| **%S**                   | Full seconds entry. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **32**.                                         |
| **%s**                   | Full milliseconds entry. For example date: **2025-03-02 15:44:32.032 GMT+0300** will correspond to **032**.                                   |
| **%p**                   | Parses the locale's equivalent of the **AM/PM** designations associated with a 12-hour clock. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **PM**. |
| **%R**                   | Equivalent to **%H:%M**. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **15:44**.                                 |
| **%T**                   | Equivalent to **%H:%M:%S**. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **15:44:32**.                           |
| **%r**                   | Parses the locale's 12-hour clock time. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **03:44:32 PM**.            |
| **%c**                   | Standard date and time representation. For example date: **2025-03-02 15:44:32 GMT+0300** will correspond to **Sun Mar 2 15:44:32 2025**. |
| **%o**                   | Format \[+\|-\]h\[h\]\[:mm\] (i.e., requiring a (**:**) between the hours and minutes and making the leading zero for hour optional). For example date: **2025-03-02 15:44:32 GMT+03:00** will correspond to **+03:00**. |
| **%z**                   | For example **-0430** refers to 4 hours 30 minutes behind UTC and 04 refers to 4 hours ahead of UTC.                                      |
| **%Z**                   | Parses the time zone abbreviation or name, taken as the longest sequence of characters that only contains the characters **A** through **Z**, **a** through **z**. For example date: **2025-03-02 15:44:32 MSK** will correspond to **MSK**. |

### Example Date and Time
```c++
#include <sys/log.hpp>
#include <sys/chrono.hpp>

using namespace awh;

int32_t main(int32_t argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	chrono_t chrono(&fmk);

	uint64_t date = chrono.parse("2023-03-05T12:55:58.0490925Z", "%Y-%m-%dT%H:%M:%S.%s%Z");
	string result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("2024-08-06T11:08:55.101Z", "%Y-%m-%dT%H:%M:%S.%s%Z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("2024-08-06T14:47:34.741876+03:00", "%Y-%m-%dT%H:%M:%S.%s%o");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("2024-08-06T14:47:34.728093306+03:0", "%Y-%m-%dT%H:%M:%S.%s%o");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("7/26/2023 2:39:42 PM", "%m/%d/%Y %I:%M:%S %p");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("2023-07-26T14:39:4", "%Y-%m-%dT%H:%M:%S");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("7/26/2023 2:39:42 PM (2934007)", "%m/%d/%Y %I:%M:%S %p (%s)");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("2024-11-15 17:14:03,331", "%Y-%m-%d %H:%M:%S,%s");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Jun 11 09:56:56", "%h %d %H:%M:%S");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Tue Jul 16 10:45:40.020399 2024", "%a %h %d %H:%M:%S.%s %Y");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("05/Apr/2023:12:45:12.345678901 +0300", "%d/%h/%Y:%H:%M:%S.%s %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Aug 13 17:43:12", "%h %d %H:%M:%S");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("2024-10-16 10:30:45.789", "%Y-%m-%d %H:%M:%S.%s");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("[18/Jul/2024:13:34:00 +0300]", "%d/%h/%Y:%H:%M:%S %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("2024/07/18 13:33:17", "%Y/%m/%d %H:%M:%S");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("17.07.2023 13:25:53", "%d.%m.%Y %H:%M:%S");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("[04-22 13:54:55.343240]", "%m-%d %H:%M:%S.%s");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("17:54:49", "%H:%M:%S");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 19 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 19 2025 15:51:10", "%a %h %e %Y %H:%M:%S %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 19 15:51:10 GMT+0300", "%a %h %e %H:%M:%S %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 20 19:56:10 GMT+0300", "%a %h %e %H:%M:%S %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 20 19:56:10", "%a %h %e %H:%M:%S %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 20 19:56:10 GMT+0430", "%a %h %e %H:%M:%S %z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 30 2025 15:51:10 GMT+0300", "%a %h %e %Y %H:%M:%S %Z%z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Dec 03 12:00 MSK+0332", "%h %d %H:%M %Z%z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("Wed Mar 31 00:51:10 11 GMT+0300 080", "%a %h %e %H:%M:%S %W %z %j");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	date = chrono.parse("20050809T183142+0330", "%Y%m%dT%H%M%S%z");
	result = chrono.format(date, "%A %e %B %Y %H:%M:%S.%s %Z AND %a %e %h %y %I:%M:%S.%s %p %o");
	log.print("Date: %s (%llu)", log_t::flag_t::INFO, result.c_str(), date);

	return EXIT_SUCCESS;
}
```
