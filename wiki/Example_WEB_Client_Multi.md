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

	awh.callback <void (const client::web_t::mode_t)> ("active", std::bind(&WebClient::active, &executor, _1));
	awh.callback <void (const int32_t, const uint64_t, const u_int, const string &)> ("response", std::bind(&WebClient::message, &executor, _1, _2, _3, _4));
	awh.callback <void (const int32_t, const uint64_t, const u_int, const string &, const vector <char> &)> ("entity", std::bind(&WebClient::entity, &executor, _1, _2, _3, _4, _5));
	awh.callback <void (const int32_t, const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", std::bind(&WebClient::headers, &executor, _1, _2, _3, _4, _5));
	
	awh.init("https://apple.com");
	awh.start();

	return 0;
}
```
