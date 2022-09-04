[![ANYKS - WEB](https://raw.githubusercontent.com/anyks/awh/main/img/banner.jpg)](https://anyks.com)

# ANYKS - WEB (AWH) C++

## Project goals and features

- **HTTP/HTTPS**: REST - CLIENT/SERVER.
- **WS/WSS**: WebSocket - CLIENT/SERVER.
- **Proxy**: HTTP/SOCKS5 PROXY - CLIENT/SERVER.
- **Compress**: GZIP/DEFLATE/BROTLI - compression support.
- **Authentication**: BASIC/DIGEST - authentication support.

## Requirements

- [Zlib](http://www.zlib.net)
- [Brotli](https://github.com/google/brotli)
- [OpenSSL](https://www.openssl.org)
- [LibEv](http://software.schmorp.de/pkg/libev.html)
- [LibIconv](https://www.gnu.org/software/libiconv)
- [LibIdn2](https://www.gnu.org/software/libidn)
- [Libev-Win](https://github.com/disenone/libev-win)
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

### Build third party

```bash
$ ./build_third_party.sh
```

### Build on MacOS X, Linux and FreeBSD

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -DCMAKE_BUILD_TYPE=Release \
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
$ pacman -S --needed base-devel mingw-w64-x86_64-toolchain
$ pacman -S mingw-w64-x86_64-dlfcn
```

#### Project build

```bash
$ mkdir ./build
$ cd ./build

$ cmake \
 -G "MinGW Makefiles" \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 ..

$ cmake --build .
```

### Example REST Client

```c++
#include <client/rest.hpp>

using namespace std;
using namespace awh;

class WebClient {
	private:
		log_t * _log;
	public:
		void active(const client::rest_t::mode_t mode, client::rest_t * web){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::rest_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
	public:
		WebClient(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);
	uri_t uri(&fmk, &nwk);

	WebClient executor(&log);

	client::core_t core(&fmk, &log);
	client::rest_t rest(&core, &fmk, &log);

	log.setLogName("REST Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.ca("./ca/cert.pem");
	// core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TCP);

	rest.mode(
		(uint8_t) client::rest_t::flag_t::NOINFO |
		(uint8_t) client::rest_t::flag_t::WAITMESS |
		(uint8_t) client::rest_t::flag_t::REDIRECTS |
		(uint8_t) client::rest_t::flag_t::VERIFYSSL
	);
	// rest.proxy("http://user:password@host.com:port");
	rest.proxy("socks5://user:password@host.com:port");
	rest.compress(http_t::compress_t::ALL_COMPRESS);
	rest.on(bind(&WebClient::active, &executor, _1, _2));

	const auto & body = rest.GET(uri.parse("https://2ip.ru"), {{"User-Agent", "curl/7.64.1"}});

	log.print("ip: %s", log_t::flag_t::INFO, body.data());

	return 0;
}
```

### Example REST Server

```c++
#include <server/rest.hpp>

using namespace std;
using namespace awh;

class WebServer {
	private:
		log_t * _log;
	public:
		string password(const string & login){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");
			return "password";
		}
		bool auth(const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
			return true;
		}
	public:
		bool accept(const string & ip, const string & mac, const u_int port, server::rest_t * web){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const server::rest_t::mode_t mode, server::rest_t * web){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::rest_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		void message(const size_t aid, const awh::http_t * http, server::rest_t * web){
			const auto & query = http->query();

			if(!query.uri.empty() && (query.uri.find("favicon.ico") != string::npos))
				web->reject(aid, 404);
			else if(query.method == web_t::method_t::GET){
				const string body = "<html>\n<head>\n<title>Hello World!</title>\n</head>\n<body>\n"
				"<h1>\"Hello, World!\" program</h1>\n"
				"<div>\nFrom Wikipedia, the free encyclopedia<br>\n"
				"(Redirected from Hello, world!)<br>\n"
				"Jump to navigationJump to search<br>\n"
				"<strong>\"Hello World\"</strong> redirects here. For other uses, see Hello World (disambiguation).<br>\n"
				"A <strong>\"Hello, World!\"</strong> program generally is a computer program that outputs or displays the message \"Hello, World!\".<br>\n"
				"Such a program is very simple in most programming languages, and is often used to illustrate the basic syntax of a programming language. It is often the first program written by people learning to code. It can also be used as a sanity test to make sure that computer software intended to compile or run source code is correctly installed, and that the operator understands how to use it.\n"
				"</div>\n</body>\n</html>\n";
				web->response(aid, 200, "OK", vector <char> (body.begin(), body.end()));
			}
		}
	public:
		WebServer(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	WebServer executor(&log);

	server::core_t core(&fmk, &log);
	server::rest_t rest(&core, &fmk, &log);

	log.setLogName("Web Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.clusterSize();
	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	rest.realm("ANYKS");
	rest.opaque("keySession");
	rest.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	rest.init(2222, "127.0.0.1", http_t::compress_t::ALL_COMPRESS);

	rest.on((function <string (const string &)>) bind(&WebServer::password, &executor, _1));
	// rest.on((function <bool (const string &, const string &)>) bind(&WebServer::auth, &executor, _1, _2));
	rest.on((function <void (const size_t, const awh::http_t *, server::rest_t *)>) bind(&WebServer::message, &executor, _1, _2, _3));
	rest.on((function <void (const size_t, const server::rest_t::mode_t, server::rest_t *)>) bind(&WebServer::active, &executor, _1, _2, _3));
	rest.on((function <bool (const string &, const string &, const u_int, server::rest_t *)>) bind(&WebServer::accept, &executor, _1, _2, _3, _4));
	
	rest.start();

	return 0;
}
```

### Example WebSocket Client

```c++
#include <client/ws.hpp>

using namespace std;
using namespace awh;

class WebSocket {
	private:
		log_t * _log;
	public:
		void active(const client::ws_t::mode_t mode, client::ws_t * ws){
			this->_log->print("%s server", log_t::flag_t::INFO, (mode == client::ws_t::mode_t::CONNECT ? "Start" : "Stop"));

			if(mode == client::ws_t::mode_t::CONNECT){
				const string query = "{\"text\":\"Hello World!\"}";
				ws->send(query.data(), query.size());
			}
		}
		void error(const u_int code, const string & mess, client::ws_t * ws){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		void message(const vector <char> & buffer, const bool utf8, client::ws_t * ws){
			if(utf8 && !buffer.empty())
				this->_log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->sub().c_str());
		}
	public:
		WebSocket(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	WebSocket executor(&log);

	client::core_t core(&fmk, &log);
	client::ws_t ws(&core, &fmk, &log);

	log.setLogName("WebSocket Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	ws.mode(
		(uint8_t) client::ws_t::flag_t::TAKEOVERCLI |
		(uint8_t) client::ws_t::flag_t::TAKEOVERSRV |
		(uint8_t) client::ws_t::flag_t::VERIFYSSL |
		(uint8_t) client::ws_t::flag_t::KEEPALIVE
	);

	core.verifySSL(false);
	core.ca("./ca/cert.pem");
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	// ws.proxy("http://user:password@host.com:port");
	// ws.proxy("socks5://user:password@host.com:port");
	// ws.authTypeProxy(auth_t::type_t::BASIC);
	// ws.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	ws.user("user", "password");
	// ws.authType(auth_t::type_t::BASIC);
	ws.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);

	ws.subs({"test2", "test8", "test9"});
	ws.init("wss://127.0.0.1:2222", http_t::compress_t::DEFLATE);

	ws.on(bind(&WebSocket::active, &executor, _1, _2));
	ws.on(bind(&WebSocket::error, &executor, _1, _2, _3));
	ws.on(bind(&WebSocket::message, &executor, _1, _2, _3));

	ws.start();	

	return 0;
}
```

### Example WebSocket Server

```c++
#include <server/ws.hpp>

using namespace std;
using namespace awh;

class WebSocket {
	private:
		log_t * _log;
	public:
		string password(const string & login){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");
			return "password";
		}
		bool auth(const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
			return true;
		}
	public:
		bool accept(const string & ip, const string & mac, const u_int port, server::ws_t * ws){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const server::ws_t::mode_t mode, server::ws_t * ws){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::ws_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		void error(const size_t aid, const u_int code, const string & mess, server::ws_t * ws){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		void message(const size_t aid, const vector <char> & buffer, const bool utf8, server::ws_t * ws){
			if(!buffer.empty()){
				this->_log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->sub(aid).c_str());
				ws->send(aid, buffer.data(), buffer.size(), utf8);
			}
		}
	public:
		WebSocket(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	WebSocket executor(&log);

	server::core_t core(&fmk, &log);
	server::ws_t ws(&core, &fmk, &log);

	log.setLogName("WebSocket Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.clusterSize();
	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	ws.realm("ANYKS");
	ws.opaque("keySession");
	ws.subs({"test1", "test2", "test3"});

	// ws.authType(auth_t::type_t::BASIC);
	ws.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	ws.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);

	ws.on((function <string (const string &)>) bind(&WebSocket::password, &executor, _1));
	// ws.on((function <bool (const string &, const string &)>) bind(&WebSocket::auth, &executor, _1, _2));
	ws.on((function <void (const size_t, const server::ws_t::mode_t, server::ws_t *)>) bind(&WebSocket::active, &executor, _1, _2, _3));
	ws.on((function <void (const size_t, const u_int, const string &, server::ws_t *)>) bind(&WebSocket::error, &executor, _1, _2, _3, _4));
	ws.on((function <bool (const string &, const string &, const u_int, server::ws_t *)>) bind(&WebSocket::accept, &executor, _1, _2, _3, _4));
	ws.on((function <void (const size_t, const vector <char> &, const bool, server::ws_t *)>) bind(&WebSocket::message, &executor, _1, _2, _3, _4));

	ws.start();

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
		string password(const string & login){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");
			return "password";
		}
		bool auth(const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
			return true;
		}
	public:
		bool accept(const string & ip, const string & mac, const u_int port, server::proxy_t * proxy){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const server::proxy_t::mode_t mode, server::proxy_t * proxy){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::proxy_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		bool message(const size_t aid, const server::proxy_t::event_t event, awh::http_t * http, server::proxy_t * proxy){
			cout << (event == server::proxy_t::event_t::REQUEST ? "REQUEST" : "RESPONSE") << endl;
			for(auto & header : http->headers())
				cout << "Header: " << header.first << " = " << header.second << endl << endl;
			return true;
		}
	public:
		Proxy(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Proxy executor(&log);

	server::proxy_t proxy(&fmk, &log);

	log.setLogName("Proxy Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	proxy.mode(
		(uint8_t) server::proxy_t::flag_t::NOIFNO |
		(uint8_t) server::proxy_t::flag_t::WAITMESS
	);
	proxy.clusterSize();

	proxy.realm("ANYKS");
	proxy.opaque("keySession");

	proxy.authType(auth_t::type_t::BASIC);
	// proxy.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	proxy.sonet(awh::scheme_t::sonet_t::TCP);
	proxy.init(2222, "127.0.0.1", http_t::compress_t::GZIP);

	// proxy.on((function <string (const string &)>) bind(&Proxy::password, &executor, _1));
	proxy.on((function <bool (const string &, const string &)>) bind(&Proxy::auth, &executor, _1, _2));
	proxy.on((function <void (const size_t, const server::proxy_t::mode_t, server::proxy_t *)>) bind(&Proxy::active, &executor, _1, _2, _3));
	proxy.on((function <bool (const string &, const string &, const u_int, server::proxy_t *)>) bind(&Proxy::accept, &executor, _1, _2, _3, _4));
	proxy.on((function <bool (const size_t, const server::proxy_t::event_t, awh::http_t *, server::proxy_t *)>) bind(&Proxy::message, &executor, _1, _2, _3, _4));

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
		log_t * _log;
	public:
		bool auth(const string & login, const string & password){
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
			return true;
		}
	public:
		bool accept(const string & ip, const string & mac, const u_int port, proxy_socks5_t * proxy){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const proxy_socks5_t::mode_t mode, proxy_socks5_t * proxy){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == proxy_socks5_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
	public:
		Proxy(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Proxy executor(&log);

	proxy_socks5_t proxy(&fmk, &log);

	log.setLogName("Proxy Socks5 Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	proxy.mode(
		(uint8_t) proxy_socks5_t::flag_t::NOIFNO |
		(uint8_t) proxy_socks5_t::flag_t::WAITMESS
	);
	proxy.clusterSize();

	proxy.init(2222, "127.0.0.1");

	proxy.on((function <bool (const string &, const string &)>) bind(&Proxy::auth, &executor, _1, _2));
	proxy.on((function <void (const size_t, const proxy_socks5_t::mode_t, proxy_socks5_t *)>) bind(&Proxy::active, &executor, _1, _2, _3));
	proxy.on((function <bool (const string &, const string &, const u_int, proxy_socks5_t *)>) bind(&Proxy::accept, &executor, _1, _2, _3, _4));

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

class Timer {
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
			this->_log->print("Timeout: %u seconds", log_t::flag_t::INFO, chrono::duration_cast <chrono::seconds> (chrono::system_clock::now() - this->ts).count());
		}
		void run(const bool mode, Core * core){
			if(mode){
				this->ts = chrono::system_clock::now();
				this->is = chrono::system_clock::now();

				this->_log->print("%s", log_t::flag_t::INFO, "Start timer");

				core->setTimeout(10000, (function <void (const u_short, core_t *)>) bind(&Timer::timeout, this, _1, _2));
				core->setInterval(5000, (function <void (const u_short, core_t *)>) bind(&Timer::interval, this, _1, _2));
			} else this->log->print("%s", log_t::flag_t::INFO, "Stop timer");
		}
	public:
		Timer(log_t * log) : ts(chrono::system_clock::now()), is(chrono::system_clock::now()), count(0), _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Timer executor(&log);

	core_t core(&fmk, &log);

	log.setLogName("Timer");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.callback((function <void (const bool, core_t *)>) bind(&Timer::run, &executor, _1, _2));

	core.start();

	return 0;
}
```

### Example DNS Resolver

```c++
#include <core/core.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	core_t core(&fmk, &log);

	log.setLogName("DNS");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.resolve("google.com", scheme_t::family_t::IPV4, [&log](const string & ip, const scheme_t::family_t family, core_t * core){
		log.print("IP: %s", log_t::flag_t::INFO, ip.c_str());
		core->stop();
	});

	core.start();

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
		log_t * _log;
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
		Client(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.setLogName("SAMPLE Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOINFO |
		(uint8_t) client::sample_t::flag_t::WAITMESS |
		(uint8_t) client::sample_t::flag_t::VERIFYSSL
	);
	core.verifySSL(false);
	core.ca("./ca/cert.pem");
	core.sonet(awh::scheme_t::sonet_t::SCTP);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	sample.init(2222, "127.0.0.1");
	sample.on(bind(&Client::active, &executor, _1, _2));
	sample.on(bind(&Client::message, &executor, _1, _2));

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
		log_t * _log;
	public:
		bool accept(const string & ip, const string & mac, const u_int port, server::sample_t * sample){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const server::sample_t::mode_t mode, server::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		void message(const size_t aid, const vector <char> & buffer, server::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			sample->send(aid, buffer.data(), buffer.size());
		}
	public:
		Server(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.setLogName("SAMPLE Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::SCTP);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	sample.init(2222, "127.0.0.1");
	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));

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
		log_t * _log;
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
		Client(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.setLogName("SAMPLE Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOINFO |
		(uint8_t) client::sample_t::flag_t::WAITMESS |
		(uint8_t) client::sample_t::flag_t::VERIFYSSL
	);
	core.verifySSL(false);
	core.ca("./ca/cert.pem");
	core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	sample.init(2222, "127.0.0.1");
	sample.on(bind(&Client::active, &executor, _1, _2));
	sample.on(bind(&Client::message, &executor, _1, _2));

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
		log_t * _log;
	public:
		bool accept(const string & ip, const string & mac, const u_int port, server::sample_t * sample){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const server::sample_t::mode_t mode, server::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		void message(const size_t aid, const vector <char> & buffer, server::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			sample->send(aid, buffer.data(), buffer.size());
		}
	public:
		Server(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.setLogName("SAMPLE Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	sample.init(2222, "127.0.0.1");
	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));

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
		log_t * _log;
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
		Client(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.setLogName("SAMPLE Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOINFO |
		(uint8_t) client::sample_t::flag_t::WAITMESS |
		(uint8_t) client::sample_t::flag_t::VERIFYSSL
	);
	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TCP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");
	sample.on(bind(&Client::active, &executor, _1, _2));
	sample.on(bind(&Client::message, &executor, _1, _2));

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
		log_t * _log;
	public:
		bool accept(const string & ip, const string & mac, const u_int port, server::sample_t * sample){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const server::sample_t::mode_t mode, server::sample_t * sample){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		void message(const size_t aid, const vector <char> & buffer, server::sample_t * sample){
			const string message(buffer.begin(), buffer.end());
			this->_log->print("%s", log_t::flag_t::INFO, message.c_str());
			sample->send(aid, buffer.data(), buffer.size());
		}
	public:
		Server(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.setLogName("SAMPLE Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TCP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");
	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));

	sample.start();

	return 0;
}
```
