[![ANYKS - WEB](https://raw.githubusercontent.com/anyks/awh/main/img/banner.jpg)](https://anyks.com)

# ANYKS - WEB (AWH) C++

## Project goals and features

- **HTTP/HTTPS**: WEB - CLIENT/SERVER.
- **WS/WSS**: WebSocket - CLIENT/SERVER.
- **Proxy**: HTTP/SOCKS5 PROXY - CLIENT/SERVER.
- **Compress**: GZIP/DEFLATE/BROTLI - compression support.
- **Authentication**: BASIC/DIGEST - authentication support.

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
 -DCMAKE_BUILD_IDN=yes \
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
 -G "MinGW Makefiles" \
 -DCMAKE_BUILD_IDN=yes \
 -DCMAKE_BUILD_EVENT2=yes \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_SYSTEM_NAME=Windows \
 ..

$ cmake --build .
```

### Example WEB Client

```c++
#include <client/web.hpp>

using namespace std;
using namespace awh;

class WebClient {
	private:
		log_t * _log;
	public:
		void active(const client::web_t::mode_t mode, client::web_t * web){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
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
	client::web_t web(&core, &fmk, &log);

	log.name("WEB Client");
	log.format("%H:%M:%S %d.%m.%Y");

	core.ca("./ca/cert.pem");
	// core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TCP);

	web.mode(
		(uint8_t) client::web_t::flag_t::NOT_INFO |
		(uint8_t) client::web_t::flag_t::WAIT_MESS |
		(uint8_t) client::web_t::flag_t::REDIRECTS |
		(uint8_t) client::web_t::flag_t::VERIFY_SSL
	);
	// web.proxy("http://user:password@host.com:port");
	web.proxy("socks5://user:password@host.com:port");
	web.compress(http_t::compress_t::ALL_COMPRESS);
	web.on(bind(&WebClient::active, &executor, _1, _2));

	const auto & body = web.GET(uri.parse("https://2ip.ru"), {{"User-Agent", "curl/7.64.1"}});

	log.print("ip: %s", log_t::flag_t::INFO, body.data());

	return 0;
}
```

### Example WEB Server

```c++
#include <server/web.hpp>

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
		bool accept(const string & ip, const string & mac, const u_int port, server::web_t * web){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const server::web_t::mode_t mode, server::web_t * web){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		void message(const size_t aid, const awh::http_t * http, server::web_t * web){
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
	server::web_t web(&core, &fmk, &log);

	log.name("Web Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.clusterSize();
	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	web.realm("ANYKS");
	web.opaque("keySession");
	web.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	web.init(2222, "127.0.0.1", http_t::compress_t::ALL_COMPRESS);

	web.on((function <string (const string &)>) bind(&WebServer::password, &executor, _1));
	// web.on((function <bool (const string &, const string &)>) bind(&WebServer::auth, &executor, _1, _2));
	web.on((function <void (const size_t, const awh::http_t *, server::web_t *)>) bind(&WebServer::message, &executor, _1, _2, _3));
	web.on((function <void (const size_t, const server::web_t::mode_t, server::web_t *)>) bind(&WebServer::active, &executor, _1, _2, _3));
	web.on((function <bool (const string &, const string &, const u_int, server::web_t *)>) bind(&WebServer::accept, &executor, _1, _2, _3, _4));
	
	web.start();

	return 0;
}
```

### Example WebSocket Client

```c++
#include <client/websocket.hpp>

using namespace std;
using namespace awh;
using namespace awh::client;

class Executor {
	private:
		log_t * _log;
	public:
		void active(const websocket_t::mode_t mode, websocket_t * ws){
			this->_log->print("%s server", log_t::flag_t::INFO, (mode == websocket_t::mode_t::CONNECT ? "Start" : "Stop"));

			if(mode == websocket_t::mode_t::CONNECT){
				const string query = "{\"text\":\"Hello World!\"}";
				ws->send(query.data(), query.size());
			}
		}
		void error(const u_int code, const string & mess, websocket_t * ws){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		void message(const vector <char> & buffer, const bool utf8, websocket_t * ws){
			if(utf8 && !buffer.empty())
				this->_log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->sub().c_str());
		}
	public:
		Executor(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Executor executor(&log);

	client::core_t core(&fmk, &log);
	websocket_t ws(&core, &fmk, &log);

	log.name("WebSocket Client");
	log.format("%H:%M:%S %d.%m.%Y");

	ws.mode(
		(uint8_t) websocket_t::flag_t::ALIVE |
		(uint8_t) websocket_t::flag_t::VERIFY_SSL |
		(uint8_t) websocket_t::flag_t::TAKEOVER_CLIENT |
		(uint8_t) websocket_t::flag_t::TAKEOVER_SERVER
	);

	core.verifySSL(false);
	core.ca("./ca/cert.pem");
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	// ws.proxy("http://user:password@host.com:port");
	// ws.proxy("socks5://user:password@host.com:port");
	// ws.authTypeProxy(awh::auth_t::type_t::BASIC);
	// ws.authTypeProxy(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);

	ws.user("user", "password");
	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);

	ws.subs({"test2", "test8", "test9"});
	ws.init("wss://127.0.0.1:2222", awh::http_t::compress_t::DEFLATE);

	ws.on(bind(&Executor::active, &executor, _1, _2));
	ws.on(bind(&Executor::error, &executor, _1, _2, _3));
	ws.on(bind(&Executor::message, &executor, _1, _2, _3));

	ws.start();	

	return 0;
}
```

### Example WebSocket Server

```c++
#include <server/websocket.hpp>

using namespace std;
using namespace awh;
using namespace awh::server;

class Executor {
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
		bool accept(const string & ip, const string & mac, const u_int port, websocket_t * ws){
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			return true;
		}
		void active(const size_t aid, const websocket_t::mode_t mode, websocket_t * ws){
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == websocket_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		void error(const size_t aid, const u_int code, const string & mess, websocket_t * ws){
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		void message(const size_t aid, const vector <char> & buffer, const bool utf8, websocket_t * ws){
			if(!buffer.empty()){
				this->_log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->sub(aid).c_str());
				ws->send(aid, buffer.data(), buffer.size(), utf8);
			}
		}
	public:
		Executor(log_t * log) : _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Executor executor(&log);

	server::core_t core(&fmk, &log);
	websocket_t ws(&core, &fmk, &log);

	log.name("WebSocket Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.clusterSize();
	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	ws.realm("ANYKS");
	ws.opaque("keySession");
	ws.subs({"test1", "test2", "test3"});

	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);
	ws.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);

	ws.on((function <string (const string &)>) bind(&Executor::password, &executor, _1));
	// ws.on((function <bool (const string &, const string &)>) bind(&Executor::auth, &executor, _1, _2));
	ws.on((function <void (const size_t, const websocket_t::mode_t, websocket_t *)>) bind(&Executor::active, &executor, _1, _2, _3));
	ws.on((function <void (const size_t, const u_int, const string &, websocket_t *)>) bind(&Executor::error, &executor, _1, _2, _3, _4));
	ws.on((function <bool (const string &, const string &, const u_int, websocket_t *)>) bind(&Executor::accept, &executor, _1, _2, _3, _4));
	ws.on((function <void (const size_t, const vector <char> &, const bool, websocket_t *)>) bind(&Executor::message, &executor, _1, _2, _3, _4));

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

	log.name("Proxy Server");
	log.format("%H:%M:%S %d.%m.%Y");

	proxy.mode(
		(uint8_t) server::proxy_t::flag_t::NOT_INFO |
		(uint8_t) server::proxy_t::flag_t::WAIT_MESS
	);
	proxy.clusterSize();

	// proxy.realm("ANYKS");
	// proxy.opaque("keySession");

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

	log.name("Proxy Socks5 Server");
	log.format("%H:%M:%S %d.%m.%Y");

	proxy.mode(
		(uint8_t) proxy_socks5_t::flag_t::NOT_INFO |
		(uint8_t) proxy_socks5_t::flag_t::WAIT_MESS
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
			} else this->_log->print("%s", log_t::flag_t::INFO, "Stop timer");
		}
	public:
		Timer(log_t * log) : ts(chrono::system_clock::now()), is(chrono::system_clock::now()), count(0), _log(log) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Timer executor(&log);

	log.name("Timer");
	log.format("%H:%M:%S %d.%m.%Y");

	core.callback((function <void (const bool, core_t *)>) bind(&Timer::run, &executor, _1, _2));

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

	dns.servers({"77.88.8.88", "77.88.8.2"});

	log.print("IP1: %s", log_t::flag_t::INFO, dns.resolve("localhost").c_str());
	log.print("IP2: %s", log_t::flag_t::INFO, dns.resolve("yandex.ru").c_str());
	log.print("IP3: %s", log_t::flag_t::INFO, dns.resolve("google.com").c_str());

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

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOT_INFO |
		(uint8_t) client::sample_t::flag_t::WAIT_MESS |
		(uint8_t) client::sample_t::flag_t::VERIFY_SSL
	);
	core.sonet(awh::scheme_t::sonet_t::TCP);

	sample.init(2222, "127.0.0.1");
	sample.on(bind(&Client::active, &executor, _1, _2));
	sample.on(bind(&Client::message, &executor, _1, _2));

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

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::TCP);

	sample.init(2222, "127.0.0.1");
	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));

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

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOT_INFO |
		(uint8_t) client::sample_t::flag_t::WAIT_MESS |
		(uint8_t) client::sample_t::flag_t::VERIFY_SSL
	);
	core.verifySSL(false);
	core.ca("./ca/cert.pem");
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");

	sample.init(2222, "127.0.0.1");
	sample.on(bind(&Client::active, &executor, _1, _2));
	sample.on(bind(&Client::message, &executor, _1, _2));

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

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.verifySSL(false);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");

	sample.init(2222, "127.0.0.1");
	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));

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

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOT_INFO |
		(uint8_t) client::sample_t::flag_t::WAIT_MESS |
		(uint8_t) client::sample_t::flag_t::VERIFY_SSL
	);
	core.sonet(awh::scheme_t::sonet_t::UDP);

	sample.init(2222, "127.0.0.1");
	sample.on(bind(&Client::active, &executor, _1, _2));
	sample.on(bind(&Client::message, &executor, _1, _2));

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

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::UDP);

	sample.init(2222, "127.0.0.1");
	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));

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

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOT_INFO |
		(uint8_t) client::sample_t::flag_t::WAIT_MESS |
		(uint8_t) client::sample_t::flag_t::VERIFY_SSL
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

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

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

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOT_INFO |
		(uint8_t) client::sample_t::flag_t::WAIT_MESS |
		(uint8_t) client::sample_t::flag_t::VERIFY_SSL
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

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

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

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOT_INFO |
		(uint8_t) client::sample_t::flag_t::WAIT_MESS |
		(uint8_t) client::sample_t::flag_t::VERIFY_SSL
	);
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

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

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

### Example UDP UnixSocket Client

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

	Client executor(&log);

	client::core_t core(&fmk, &log);
	client::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Client");
	log.format("%H:%M:%S %d.%m.%Y");

	sample.mode(
		// (uint8_t) client::sample_t::flag_t::NOT_INFO |
		(uint8_t) client::sample_t::flag_t::WAIT_MESS |
		(uint8_t) client::sample_t::flag_t::VERIFY_SSL
	);
	core.sonet(awh::scheme_t::sonet_t::UDP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");
	sample.on(bind(&Client::active, &executor, _1, _2));
	sample.on(bind(&Client::message, &executor, _1, _2));

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

	Server executor(&log);

	server::core_t core(&fmk, &log);
	server::sample_t sample(&core, &fmk, &log);

	log.name("SAMPLE Server");
	log.format("%H:%M:%S %d.%m.%Y");

	core.sonet(awh::scheme_t::sonet_t::UDP);
	core.family(awh::scheme_t::family_t::NIX);

	sample.init("anyks");

	sample.on((function <void (const size_t, const vector <char> &, server::sample_t *)>) bind(&Server::message, &executor, _1, _2, _3));
	sample.on((function <void (const size_t, const server::sample_t::mode_t, server::sample_t *)>) bind(&Server::active, &executor, _1, _2, _3));
	sample.on((function <bool (const string &, const string &, const u_int, server::sample_t *)>) bind(&Server::accept, &executor, _1, _2, _3, _4));

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
	private:
		cluster::core_t * _core;
	public:
		void events(const cluster::core_t::worker_t worker, const pid_t pid, const cluster_t::event_t event){
			
			if(event == cluster_t::event_t::START){
				
				switch(static_cast <uint8_t> (worker)){
					case static_cast <uint8_t> (cluster::core_t::worker_t::MASTER): {
						const char * message = "Hi!";
						this->_core->broadcast(message, strlen(message));
					} break;
					case static_cast <uint8_t> (cluster::core_t::worker_t::CHILDREN): {
						const char * message = "Hello";
						this->_core->send(message, strlen(message));
					} break;
				}

			}

		}
		void message(const cluster::core_t::worker_t worker, const pid_t pid, const char * buffer, const size_t size){
			
			switch(static_cast <uint8_t> (worker)){
				case static_cast <uint8_t> (cluster::core_t::worker_t::MASTER):
					this->_log->print("Message from children [%u]: %s", log_t::flag_t::INFO, pid, string(buffer, size).c_str());
				break;
				case static_cast <uint8_t> (cluster::core_t::worker_t::CHILDREN):
					this->_log->print("Message from master: %s [%u]", log_t::flag_t::INFO, string(buffer, size).c_str(), getpid());
				break;
			}
			
		}
		void run(const bool mode, core_t * core){

			if(mode){
				this->_core = dynamic_cast <cluster::core_t *> (core);
				this->_core->run();
				this->_log->print("%s", log_t::flag_t::INFO, "Start cluster");
			} else {
				this->_core->end();
				this->_log->print("%s", log_t::flag_t::INFO, "Stop cluster");
			}

		}
	public:
		Executor(log_t * log) : _log(log), _core(nullptr) {}
};

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);

	Executor executor(&log);

	cluster::core_t core(&fmk, &log);

	log.name("Cluster");
	log.format("%H:%M:%S %d.%m.%Y");

	core.clusterSize();
	core.clusterAutoRestart(true);

	core.callback((function <void (const bool, core_t *)>) bind(&Executor::run, &executor, _1, _2));

	core.on((function <void (const cluster::core_t::worker_t, const pid_t, const cluster_t::event_t)>) bind(&Executor::events, &executor, _1, _2, _3));
	core.on((function <void (const cluster::core_t::worker_t, const pid_t, const char *, const size_t)>) bind(&Executor::message, &executor, _1, _2, _3, _4));

	core.start();

	return 0;
}
```

### Example IP Address

```c++
#include <net/net.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]){
	fmk_t fmk;
	log_t log(&fmk);
	net_t net(&fmk, &log);

	net = "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d]";
	cout << " [2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] == " << net << endl;
	// -> [2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] == 2001:DB8:11A3:9D7:1F34:8A2E:7A0:765D

	net = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	cout << " 2001:0db8:0000:0000:0000:0000:ae21:ad12 == " << net << endl;
	// -> 2001:0db8:0000:0000:0000:0000:ae21:ad12 == 2001:DB8::AE21:AD12

	net = "2001:db8::ae21:ad12";
	cout << " 2001:db8::ae21:ad12 == " << net.get(net_t::format_t::LONG) << " and " << net.get(net_t::format_t::MIDDLE) << endl;
	// -> 2001:db8::ae21:ad12 == 2001:0DB8:0000:0000:0000:0000:AE21:AD12 and 2001:DB8:0:0:0:0:AE21:AD12

	net = "0000:0000:0000:0000:0000:0000:ae21:ad12";
	cout << " 0000:0000:0000:0000:0000:0000:ae21:ad12 == " << net.get(net_t::format_t::SHORT) << endl;
	// -> 0000:0000:0000:0000:0000:0000:ae21:ad12 == ::AE21:AD12

	net = "::ae21:ad12";
	cout << " ::ae21:ad12 == " << net.get(net_t::format_t::MIDDLE) << endl;
	// -> ::ae21:ad12 == 0:0:0:0:0:0:AE21:AD12

	net = "2001:0db8:11a3:09d7:1f34::";
	cout << " 2001:0db8:11a3:09d7:1f34:: == " << net.get(net_t::format_t::LONG) << endl;
	// -> 2001:0db8:11a3:09d7:1f34:: == 2001:0DB8:11A3:09D7:1F34:0000:0000:0000

	net = "::ffff:192.0.2.1";
	cout << " ::ffff:192.0.2.1 == " << net << endl;
	// -> ::ffff:192.0.2.1 == ::FFFF:192.0.2.1

	net = "::1";
	cout << " ::1 == " << net.get(net_t::format_t::LONG) << endl;
	// -> ::1 == 0000:0000:0000:0000:0000:0000:0000:0001

	net = "[::]";
	cout << " [::] == " << net.get(net_t::format_t::LONG) << endl;
	// -> [::] == 0000:0000:0000:0000:0000:0000:0000:0000

	net = "46.39.230.51";
	cout << " 46.39.230.51 == " << net.get(net_t::format_t::LONG) << endl;
	// -> 46.39.230.51 == 046.039.230.051

	net = "192.16.0.1";
	cout << " 192.16.0.1 == " << net.get(net_t::format_t::LONG) << endl;
	// -> 192.16.0.1 == 192.016.000.001

	net = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	cout << " Part of the address: 2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d == " << net.v6()[0] << " and " << net.v6()[1] << endl;
	// -> Part of the address: 2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d == 709054858341654529 and 8528981655404027700

	net = "46.39.230.51";
	cout << " Part of the address: 46.39.230.51 == " << net.v4() << endl;
	// -> Part of the address: 46.39.230.51 == 870721326

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose(53, net_t::addr_t::NETW);
	cout << " Prefix Overlay: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 == " << net << endl;
	// -> Prefix Overlay: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 == 2001:1234:ABCD:5000::

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose("FFFF:FFFF:FFFF:F800::", net_t::addr_t::NETW);
	cout << " Mask Overlay: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: == " << net << endl;
	// -> Mask Overlay: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: == 2001:1234:ABCD:5000::

	net = "192.168.3.192";
	net.impose(9, net_t::addr_t::NETW);
	cout << " Prefix Overlay: 192.168.3.192/9 == " << net << endl;
	// -> Prefix Overlay: 192.168.3.192/9 == 192.128.0.0

	net = "192.168.3.192";
	net.impose("255.128.0.0", net_t::addr_t::NETW);
	cout << " Mask Overlay: 192.168.3.192/255.128.0.0 == " << net << endl;
	// -> Mask Overlay: 192.168.3.192/255.128.0.0 == 192.128.0.0

	net = "192.168.3.192";
	net.impose("255.255.255.0", net_t::addr_t::NETW);
	cout << " Mask Overlay: 192.168.3.192/255.255.255.0 == " << net << endl;
	// -> Mask Overlay: 192.168.3.192/255.255.255.0 == 192.168.3.0

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose(53, net_t::addr_t::HOST);
	cout << " Get the host address: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 == " << net << endl;
	// -> Get the host address: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 == ::678:9877:3322:5541:AABB

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	net.impose("FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST);
	cout << " Get the host address: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: == " << net << endl;
	// -> Get the host address: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: == ::678:9877:3322:5541:AABB

	net = "192.168.3.192";
	net.impose(9, net_t::addr_t::HOST);
	cout << " Get the host address: 192.168.3.192/9 == " << net << endl;
	// -> Get the host address: 192.168.3.192/9 == 0.40.3.192

	net = "192.168.3.192";
	net.impose("255.128.0.0", net_t::addr_t::HOST);
	cout << " Get the host address: 192.168.3.192/255.128.0.0 == " << net << endl;
	// -> Get the host address: 192.168.3.192/255.128.0.0 == 0.40.3.192

	net = "192.168.3.192";
	net.impose(24, net_t::addr_t::HOST);
	cout << " Get the host address: 192.168.3.192/24 == " << net << endl;
	// -> Get the host address: 192.168.3.192/24 == 0.0.0.192

	net = "192.168.3.192";
	net.impose("255.255.255.0", net_t::addr_t::HOST);
	cout << " Get the host address: 192.168.3.192/255.255.255.0 == " << net << endl;
	// -> Get the host address: 192.168.3.192/255.255.255.0 == 0.0.0.192

	net = "192.168.3.192";
	cout << " Getting the address mask from the network prefix 9 == " << net.prefix2Mask(9) << endl;
	cout << " Getting the network prefix from the address mask 255.128.0.0 == " << (u_short) net.mask2Prefix("255.128.0.0") << endl;
	// -> Getting the address mask from the network prefix 9 == 255.128.0.0
	// -> Getting the network prefix from the address mask 255.128.0.0 == 9

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << " Getting the address mask from the network prefix 53 == " << net.prefix2Mask(53) << endl;
	cout << " Getting the network prefix from the address mask FFFF:FFFF:FFFF:F800:: == " << (u_short) net.mask2Prefix("FFFF:FFFF:FFFF:F800::") << endl;
	// -> Getting the address mask from the network prefix 53 == FFFF:FFFF:FFFF:F800::
	// -> Getting the network prefix from the address mask FFFF:FFFF:FFFF:F800:: == 53

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Performing an address match check 192.168.3.192 network 192.168.0.0 == " << net.mapping("192.168.0.0") << endl;
	// -> Performing an address match check 192.168.3.192 network 192.168.0.0 == true
	
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb network 2001:1234:abcd:5678:: == " << net.mapping("2001:1234:abcd:5678::") << endl;
	// -> Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb network 2001:1234:abcd:5678:: == true
	
	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Performing an address match check 192.168.3.192 network 192.128.0.0/9 == " << net.mapping("192.128.0.0", 9, net_t::addr_t::NETW) << endl;
	// -> Performing an address match check 192.168.3.192 network 192.128.0.0/9 == true

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb network 2001:1234:abcd:5000::/53 == " << net.mapping("2001:1234:abcd:5000::", 53, net_t::addr_t::NETW) << endl;
	// -> Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb network 2001:1234:abcd:5000::/53 == true

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Performing an address match check 192.168.3.192 network 192.128.0.0/255.128.0.0 == " << net.mapping("192.128.0.0", "255.128.0.0", net_t::addr_t::NETW) << endl;
	// -> Performing an address match check 192.168.3.192 network сети 192.128.0.0/255.128.0.0 == true

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb network 2001:1234:abcd:5000::/FFFF:FFFF:FFFF:F800:: == " << net.mapping("2001:1234:abcd:5000::", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::NETW) << endl;
	// -> Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb network 2001:1234:abcd:5000::/FFFF:FFFF:FFFF:F800:: == true

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Performing an address match check 192.168.3.192 host 9/0.40.3.192 == " << net.mapping("0.40.3.192", 9, net_t::addr_t::HOST) << endl;
	// -> Performing an address match check 192.168.3.192 host 9/0.40.3.192 == true

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb host 53/::678:9877:3322:5541:AABB == " << net.mapping("::678:9877:3322:5541:AABB", 53, net_t::addr_t::HOST) << endl;
	// -> Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb host 53/::678:9877:3322:5541:AABB == true

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Performing an address match check 192.168.3.192 host 255.128.0.0/0.40.3.192 == " << net.mapping("0.40.3.192", "255.128.0.0", net_t::addr_t::HOST) << endl;
	// -> Performing an address match check 192.168.3.192 host 255.128.0.0/0.40.3.192 == true

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	cout << boolalpha;
	cout << " Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb host FFFF:FFFF:FFFF:F800::/::678:9877:3322:5541:AABB == " << net.mapping("::678:9877:3322:5541:AABB", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST) << endl;
	// -> Performing an address match check 2001:1234:abcd:5678:9877:3322:5541:aabb host FFFF:FFFF:FFFF:F800::/::678:9877:3322:5541:AABB == true

	net = "192.168.3.192";
	cout << boolalpha;
	cout << " Checking if the address is in the range 192.168.3.192 in the range [192.168.3.100 - 192.168.3.200] == " << net.range("192.168.3.100", "192.168.3.200", 24) << endl;
	// -> Checking if the address is in the range 192.168.3.192 in the range [192.168.3.100 - 192.168.3.200] == true

	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	const auto & dump = net.data <vector <uint16_t>> ();
	for(auto & item : dump)
		cout << " IPv6 address chunk data == " << item << endl;
	// -> IPv6 address chunk data == 8193
	// -> IPv6 address chunk data == 4660
	// -> IPv6 address chunk data == 43981
	// -> IPv6 address chunk data == 22136
	// -> IPv6 address chunk data == 39031
	// -> IPv6 address chunk data == 13090
	// -> IPv6 address chunk data == 21825
	// -> IPv6 address chunk data == 43707

	net = "46.39.230.51";
	cout << boolalpha;
	cout << " Checking if the IP address is global 46.39.230.51 == " << (net.mode() == net_t::mode_t::GLOBAL) << endl;
	// -> Checking if the IP address is global 46.39.230.51 == true

	net = "192.168.31.12";
	cout << boolalpha;
	cout << " Checking if the IP address is local 192.168.31.12 == " << (net.mode() == net_t::mode_t::LOCAL) << endl;
	// -> Checking if the IP address is local 192.168.31.12 == true

	net = "0.0.0.0";
	cout << boolalpha;
	cout << " Checking if the IP address is reserved 0.0.0.0 == " << (net.mode() == net_t::mode_t::RESERV) << endl;
	// -> Checking if the IP address is reserved 0.0.0.0 == true

	net = "[2a00:1450:4010:c0a::8b]";
	cout << boolalpha;
	cout << " Checking if the IP address is global [2a00:1450:4010:c0a::8b] == " << (net.mode() == net_t::mode_t::GLOBAL) << endl;
	// -> Checking if the IP address is global [2a00:1450:4010:c0a::8b] == true

	net = "::1";
	cout << boolalpha;
	cout << " Checking if the IP address is local [::1] == " << (net.mode() == net_t::mode_t::LOCAL) << endl;
	// -> Checking if the IP address is local [::1] == true

	net = "::";
	cout << boolalpha;
	cout << " Checking if the IP address is reserved [::] == " << (net.mode() == net_t::mode_t::RESERV) << endl;
	// -> Checking if the IP address is reserved [::] == true

	string ip = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	cout << " Long address entry == " << ip << endl;
	// -> Long address entry == 2001:0db8:0000:0000:0000:0000:ae21:ad12

	net = ip;
	ip = net;
	
	cout << " Short address entry == " << ip << endl;
	// -> Short address entry == 2001:DB8::AE21:AD12

	net = "73:0b:04:0d:db:79";
	cout << " MAC == " << net << endl;
	// -> MAC == 73:0B:04:0D:DB:79

	return 0;
}
```
