[![ANYKS - WEB](https://raw.githubusercontent.com/anyks/awh/main/img/banner.jpg)](https://anyks.com)

# ANYKS - WEB (AWH) C++

## Project goals and features

- **HTTP/HTTPS**: REST - CLIENT/SERVER.
- **WS/WSS**: WebSocket - CLIENT/SERVER.
- **Proxy**: HTTP/SOCKS5 PROXY server support.
- **Compress**: GZIP/DEFLATE/BROTLI compression support.
- **Authentication**: BASIC/DIGEST authentication support.

## Requirements

- [Zlib](http://www.zlib.net)
- [Brotli](https://github.com/google/brotli)
- [OpenSSL](https://www.openssl.org)
- [LibEvent2](https://github.com/libevent/libevent)
- [NLohmann::json](https://github.com/nlohmann/json)

## To build and launch the project

### To clone the project

```bash
$ git clone --recursive https://github.com/anyks/awh.git
```

### Build third party

```bash
$ ./build_third_party.sh
```

### Build on Linux and FreeBSD

```bash
$ mkdir ./build
$ cd ./build

$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

### Build on Windows [MSYS2 MinGW]

```bash
$ mkdir ./build
$ cd ./build

$ cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Windows ..
$ mingw32-make
```

### Example REST Client

```c++
#include <client/rest.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]) noexcept {
	fmk_t fmk(true);
	log_t log(&fmk);
	network_t nwk(&fmk);
	uri_t uri(&fmk, &nwk);
	coreCli_t core(&fmk, &log);
	restCli_t rest(&core, &fmk, &log);

	log.setLogName("REST Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	rest.setMode(
		(uint8_t) restCli_t::flag_t::DEFER |
		(uint8_t) restCli_t::flag_t::WAITMESS |
		(uint8_t) restCli_t::flag_t::VERIFYSSL
	);

	core.setCA("./ca/cert.pem");

	rest.setCompress(http_t::compress_t::GZIP);

	// rest.setProxy("http://user:password@host.com:port");
	rest.setProxy("socks5://user:password@host.com:port");
	rest.setAuthTypeProxy(auth_t::type_t::BASIC);

	rest.setUser("user", "password");
	rest.setAuthType(auth_t::type_t::DIGEST, auth_t::alg_t::MD5);

	uri_t::url_t url = uri.parseUrl("https://2ip.ru");

	rest.on(&log, [](const bool mode, restCli_t * web, void * ctx){
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("%s client", log_t::flag_t::INFO, (mode ? "Connect" : "Disconnect"));
	});

	rest.on(&log, [](const restCli_t::res_t & res, restCli_t * web, void * ctx){
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("ip: %s", log_t::flag_t::INFO, res.entity.data());

		web->stop();
	});

	rest.GET(url, {{"User-Agent", "curl/7.64.1"}});
	rest.start();

	return 0;
}
```

### Example REST Server

```c++
#include <server/rest.hpp>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]) noexcept {
	fmk_t fmk(true);
	log_t log(&fmk);
	network_t nwk(&fmk);
	uri_t uri(&fmk, &nwk);
	coreSrv_t core(&fmk, &log);
	restSrv_t rest(&core, &fmk, &log);

	log.setLogName("Rest Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	rest.init(2222, "127.0.0.1");
	rest.setAuthType(auth_t::type_t::DIGEST, auth_t::alg_t::MD5);

	rest.setExtractPassCallback(&log, [](const string & user, void * ctx) -> string {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), "password");

		return "password";
	});

	/* For Basic Auth type
	rest.setAuthCallback(&log, [](const string & user, const string & password, void * ctx) -> bool {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), password.c_str());

		return true;
	});
	*/

	rest.on(&log, [](const string & ip, const string & mac, restSrv_t * rest, void * ctx) -> bool {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("ACCEPT: ip = %s, mac = %s", log_t::flag_t::INFO, ip.c_str(), mac.c_str());

		return true;
	});

	rest.on(&log, [](const size_t aid, const bool mode,  restSrv_t * rest, void * ctx) noexcept {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("%s client", log_t::flag_t::INFO, (mode ? "Connect" : "Disconnect"));
	});

	rest.on(&log, [](const size_t aid, const restSrv_t::req_t & req, restSrv_t * rest, void * ctx) noexcept {
		if(req.method == web_t::method_t::GET){

			const string body = "<html>\n<head>\n<title>Hello World!</title>\n</head>\n<body>\n"
			"<h1>\"Hello, World!\" program</h1>\n"
			"<div>\nFrom Wikipedia, the free encyclopedia<br>\n"
			"(Redirected from Hello, world!)<br>\n"
			"Jump to navigationJump to search<br>\n"
			"<strong>\"Hello World\"</strong> redirects here. For other uses, see Hello World (disambiguation).<br>\n"
			"A <strong>\"Hello, World!\"</strong> program generally is a computer program that outputs or displays the message \"Hello, World!\".<br>\n"
			"Such a program is very simple in most programming languages, and is often used to illustrate the basic syntax of a programming language. It is often the first program written by people learning to code. It can also be used as a sanity test to make sure that computer software intended to compile or run source code is correctly installed, and that the operator understands how to use it.\n"
			"</div>\n</body>\n</html>\n";

			rest->response(aid, 200, "OK", vector <char> (body.begin(), body.end()), {{"Connection", "close"}});
		}
	});

	rest.start();

	return 0;
}
```

### Example WebSocket Client

```c++
#include <client/ws.hpp>
#include <nlohmann/json.hpp>

using namespace std;
using namespace awh;

using json = nlohmann::json;

int main(int argc, char * argv[]) noexcept {
	fmk_t fmk(true);
	log_t log(&fmk);
	network_t nwk(&fmk);
	uri_t uri(&fmk, &nwk);
	coreCli_t core(&fmk, &log);
	wsCli_t ws(&core, &fmk, &log);

	log.setLogName("WebSocket Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	ws.setMode(
		(uint8_t) wsCli_t::flag_t::NOTSTOP |
		(uint8_t) wsCli_t::flag_t::WAITMESS |
		(uint8_t) wsCli_t::flag_t::VERIFYSSL |
		(uint8_t) wsCli_t::flag_t::KEEPALIVE
	);

	ws.setCrypt("PASS");
	ws.setUser("user", "password");
	ws.setSubs({"test2", "test8", "test9"});
	ws.setAuthType(auth_t::type_t::DIGEST, auth_t::alg_t::SHA256);

	ws.init("ws://127.0.0.1:2222", http_t::compress_t::DEFLATE);

	ws.on(&log, [](const bool mode, wsCli_t * ws, void * ctx){
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("%s server", log_t::flag_t::INFO, (mode ? "Start" : "Stop"));

		if(mode){
			json data = json::parse("{\"text\":\"Hello World!\"}");

			const string query = data.dump();

			ws->send(query.data(), query.size());
		}
	});

	ws.on(&log, [](const u_short code, const string & mess, wsCli_t * ws, void * ctx){
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});

	ws.on(&log, [](const vector <char> & buffer, const bool utf8, wsCli_t * ws, void * ctx){
		if(utf8 && !buffer.empty()){
			log_t * log = reinterpret_cast <log_t *> (ctx);
			log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->getSub().c_str());
		}
	});

	ws.start();

	return 0;
}
```

### Example WebSocket Server

```c++
#include <server/ws.hpp>
#include <nlohmann/json.hpp>

using namespace std;
using namespace awh;

using json = nlohmann::json;

int main(int argc, char * argv[]) noexcept {
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);
	uri_t uri(&fmk, &nwk);
	coreSrv_t core(&fmk, &log);
	wsSrv_t ws(&core, &fmk, &log);

	log.setLogName("WebSocket Server");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	ws.setCrypt("PASS");
	ws.setRealm("ANYKS");
	ws.setOpaque("keySession");
	ws.setSubs({"test1", "test2", "test3"});
	ws.setAuthType(auth_t::type_t::DIGEST, auth_t::alg_t::SHA256);

	ws.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);

	ws.setExtractPassCallback(&log, [](const string & user, void * ctx) -> string {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), "password");

		return "password";
	});

	/* For Basic Auth type
	ws.setAuthCallback(&log, [](const string & user, const string & password, void * ctx) -> bool {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), password.c_str());

		return true;
	});
	*/

	ws.on(&log, [](const string & ip, const string & mac, wsSrv_t * ws, void * ctx) -> bool {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("ACCEPT: ip = %s, mac = %s", log_t::flag_t::INFO, ip.c_str(), mac.c_str());

		return true;
	});

	ws.on(&log, [](const size_t aid, const bool mode,  wsSrv_t * ws, void * ctx) noexcept {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("%s client", log_t::flag_t::INFO, (mode ? "Connect" : "Disconnect"));
	});

	ws.on(&log, [](const size_t aid, const u_short code, const string & mess,  wsSrv_t * ws, void * ctx) noexcept {
		log_t * log = reinterpret_cast <log_t *> (ctx);
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});

	ws.on(&log, [](const size_t aid, const vector <char> & buffer, const bool utf8,  wsSrv_t * ws, void * ctx) noexcept {
		if(utf8 && !buffer.empty()){
			log_t * log = reinterpret_cast <log_t *> (ctx);
			log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->getSub(aid).c_str());

			ws->send(aid, buffer.data(), buffer.size(), utf8);
		}
	});

	ws.start();

	return 0;
}
```
