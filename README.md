[![ANYKS - WEB](https://raw.githubusercontent.com/anyks/awh/main/img/banner.jpg)](https://anyks.com)

# ANYKS - WEB (AWH) C++

## Project goals and features

- **HTTP/HTTPS**: REST - CLIENT.
- **WS/WSS**: WebSocket - CLIENT.
- **Proxy**: HTTP PROXY server support.
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
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);
	uri_t uri(&fmk, &nwk);
	ccli_t core(&fmk, &log);
	rest_t rest(&core, &fmk, &log);

	log.setLogName("REST Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	rest.setMode((u_short) core_t::flag_t::WAITMESS | (u_short) core_t::flag_t::VERIFYSSL);

	core.setCA("./ca/cert.pem");

	rest.setProxy("http://user:password@example.com:port");
	rest.setAuthTypeProxy();

	uri_t::url_t url = uri.parseUrl("https://2ip.ru");

	const auto & body = rest.GET(url, {{"User-Agent", "curl/7.64.1"}});

	cout << " RESULT: " << string(body.begin(), body.end()) << endl;

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
	fmk_t fmk;
	log_t log(&fmk);
	network_t nwk(&fmk);
	uri_t uri(&fmk, &nwk);
	ccli_t core(&fmk, &log);
	wcli_t ws(&core, &fmk, &log);

	log.setLogName("WebSocket Client");
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	ws.setMode(
		(u_short) core_t::flag_t::NOTSTOP |
		(u_short) core_t::flag_t::WAITMESS |
		(u_short) core_t::flag_t::VERIFYSSL |
		(u_short) core_t::flag_t::KEEPALIVE
	);

	core.setCA("./ca/cert.pem");

	ws.setProxy("http://user:password@example.com:port");
	ws.setAuthTypeProxy();

	ws.init("wss://stream.binance.com:9443/stream", http_t::compress_t::DEFLATE);

	ws.on([](const bool mode, wcli_t * ws){
		cout << (mode ? "Connect" : "Disconnect") << " on server" << endl;

		if(mode){
			json data = json::object();
			data["id"] = 1;
			data["method"] = "SUBSCRIBE";
			data["params"] = json::array();
			data["params"][0] = "btcusdt@aggTrade";

			const string query = data.dump();

			ws->send(query.data(), query.size());
		}
	});

	ws.on([](const u_short code, const string & mess, wcli_t * ws){
		cout << " Error: [" << code << "] " << mess << endl;
	});

	ws.on([](const string & mess, wcli_t * ws){
		cout << " get: [" << "PONG" << "] " << mess << endl;
	});

	ws.on([](const vector <char> & buffer, const bool utf8, wcli_t * ws){
		if(utf8){
			json data = json::parse(string(buffer.begin(), buffer.end()));
			cout << " Message: " << data.dump(4) << endl;
		} else cout << " Binary size message: " << buffer.size() << " bytes" << endl;
	});

	ws.start();

	return 0;
}
```
