/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <client/awh.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * WebClient Класс объекта исполнителя
 */
class WebClient {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	public:
		/**
		 * status Метод статуса запуска/остановки сервера
		 * @param status статус события сервера
		 */
		void status(const awh::core_t::status_t status){
			// Определяем флаг статуса сервера
			switch(static_cast <uint8_t> (status)){
				// Если сервер запущен
				case static_cast <uint8_t> (awh::core_t::status_t::START): {
					// Выводим информацию в лог
					this->_log->print("START", log_t::flag_t::INFO);
				} break;
				// Если сервер остановлен
				case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
					// Выводим информацию в лог
					this->_log->print("STOP", log_t::flag_t::INFO);
				} break;
			}
		}
		/**
		 * handshake Метод события рукопожатия
		 * @param sid   идентификатор потока
		 * @param rid   идентификатор запроса
		 * @param agent идентификатор агента клиента
		 * @param awh   объект web-клиента
		 */
		void handshake(const int32_t sid, const uint64_t rid, const client::web_t::agent_t agent, client::awh_t * awh){
			// Блокируем неиспользуемые переменные
			(void) sid;
			(void) rid;
			// Если агент соответствует Websocket
			if(agent == client::web_t::agent_t::WEBSOCKET){
				// Выводим информацию в лог
				this->_log->print("WS Handshake", log_t::flag_t::INFO);
				// Создаём объект JSON
				json data = json::object();
				// Формируем идентификатор объекта
				data["id"] = 1;
				// Формируем метод подписки
				data["method"] = "SUBSCRIBE";
				// Формируем параметры запрашиваемых криптовалютных пар
				data["params"] = json::array();
				// Формируем параметры криптовалютных пар
				data["params"][0] = "omgusdt@aggTrade";
				data["params"][1] = "umausdt@aggTrade";
				data["params"][2] = "radusdt@aggTrade";
				data["params"][3] = "fttusdt@aggTrade";
				data["params"][4] = "xrpusdt@aggTrade";
				data["params"][5] = "adausdt@aggTrade";
				data["params"][6] = "solusdt@aggTrade";
				data["params"][7] = "dotusdt@aggTrade";
				data["params"][8] = "ethusdt@aggTrade";
				data["params"][9] = "bnbusdt@aggTrade";
				data["params"][10] = "btcusdt@aggTrade";
				data["params"][11] = "ltcusdt@aggTrade";
				data["params"][12] = "trxusdt@aggTrade";
				data["params"][13] = "xlmusdt@aggTrade";
				data["params"][14] = "zecusdt@aggTrade";
				data["params"][15] = "zilusdt@aggTrade";
				data["params"][16] = "vetusdt@aggTrade";
				data["params"][17] = "wrxusdt@aggTrade";
				data["params"][18] = "oneusdt@aggTrade";
				data["params"][19] = "xtzusdt@aggTrade";
				data["params"][20] = "filusdt@aggTrade";
				data["params"][21] = "eosusdt@aggTrade";
				data["params"][22] = "xmrusdt@aggTrade";
				data["params"][23] = "neousdt@aggTrade";
				data["params"][24] = "kdausdt@aggTrade";
				data["params"][25] = "ksmusdt@aggTrade";
				data["params"][26] = "icxusdt@aggTrade";
				data["params"][27] = "dgbusdt@aggTrade";
				data["params"][28] = "bchusdt@aggTrade";
				data["params"][29] = "ontusdt@aggTrade";
				data["params"][30] = "etcusdt@aggTrade";
				data["params"][31] = "chzusdt@aggTrade";
				data["params"][32] = "kncusdt@aggTrade";
				data["params"][33] = "astusdt@aggTrade";
				data["params"][34] = "atausdt@aggTrade";
				data["params"][35] = "snxusdt@aggTrade";
				data["params"][36] = "grtusdt@aggTrade";
				data["params"][37] = "mkrusdt@aggTrade";
				data["params"][38] = "dcrusdt@aggTrade";
				data["params"][39] = "c98usdt@aggTrade";
				data["params"][40] = "uniusdt@aggTrade";
				data["params"][41] = "rvnusdt@aggTrade";
				data["params"][42] = "zenusdt@aggTrade";
				data["params"][43] = "lrcusdt@aggTrade";
				data["params"][44] = "ftmusdt@aggTrade";
				data["params"][45] = "xecusdt@aggTrade";
				data["params"][46] = "xemusdt@aggTrade";
				data["params"][47] = "btgusdt@aggTrade";
				data["params"][48] = "srmusdt@aggTrade";
				data["params"][49] = "ckbusdt@aggTrade";
				data["params"][50] = "xvgusdt@aggTrade";
				data["params"][51] = "lskusdt@aggTrade";
				data["params"][52] = "ernusdt@aggTrade";
				data["params"][53] = "stxusdt@aggTrade";
				data["params"][54] = "bcdusdt@aggTrade";
				data["params"][55] = "sysusdt@aggTrade";
				data["params"][56] = "axsusdt@aggTrade";
				data["params"][57] = "qntusdt@aggTrade";
				data["params"][58] = "enjusdt@aggTrade";
				data["params"][59] = "hotusdt@aggTrade";
				data["params"][60] = "algousdt@aggTrade";
				data["params"][61] = "arpausdt@aggTrade";
				data["params"][62] = "compusdt@aggTrade";
				data["params"][63] = "iostusdt@aggTrade";
				data["params"][64] = "flowusdt@aggTrade";
				data["params"][65] = "aaveusdt@aggTrade";
				data["params"][66] = "runeusdt@aggTrade";
				data["params"][67] = "celrusdt@aggTrade";
				data["params"][68] = "linkusdt@aggTrade";
				data["params"][69] = "qtumusdt@aggTrade";
				data["params"][70] = "egldusdt@aggTrade";
				data["params"][71] = "dashusdt@aggTrade";
				data["params"][72] = "lunausdt@aggTrade";
				data["params"][73] = "cakeusdt@aggTrade";
				data["params"][74] = "atomusdt@aggTrade";
				data["params"][75] = "minausdt@aggTrade";
				data["params"][76] = "idexusdt@aggTrade";
				data["params"][77] = "dogeusdt@aggTrade";
				data["params"][78] = "avaxusdt@aggTrade";
				data["params"][79] = "iotausdt@aggTrade";
				data["params"][80] = "hbarusdt@aggTrade";
				data["params"][81] = "nearusdt@aggTrade";
				data["params"][82] = "klayusdt@aggTrade";
				data["params"][83] = "maskusdt@aggTrade";
				data["params"][84] = "iotxusdt@aggTrade";
				data["params"][85] = "celousdt@aggTrade";
				data["params"][86] = "waxpusdt@aggTrade";
				data["params"][87] = "scrtusdt@aggTrade";
				data["params"][88] = "manausdt@aggTrade";
				data["params"][89] = "reefusdt@aggTrade";
				data["params"][90] = "nanousdt@aggTrade";
				data["params"][91] = "shibusdt@aggTrade";
				data["params"][92] = "sandusdt@aggTrade";
				data["params"][93] = "thetausdt@aggTrade";
				data["params"][94] = "wavesusdt@aggTrade";
				data["params"][95] = "audiousdt@aggTrade";
				data["params"][96] = "maticusdt@aggTrade";
				data["params"][97] = "sushiusdt@aggTrade";
				data["params"][98] = "1inchusdt@aggTrade";
				data["params"][99] = "oceanusdt@aggTrade";
				// Получаем параметры запроса в виде строки
				const string query = data.dump();
				// Отправляем сообщение на сервер
				awh->sendMessage(vector <char> (query.begin(), query.end()));
			}
		}
		/**
		 * error Метод вывода ошибок Websocket клиента
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 */
		void error(const u_int code, const string & mess){
			// Выводим информацию в лог
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		/**
		 * message Метод получения сообщений
		 * @param buffer бинарный буфер сообщения
		 * @param utf8   тип буфера сообщения
		 * @param awh    объект web-клиента
		 */
		void message(const vector <char> & buffer, const bool utf8, client::awh_t * awh){
			// Выбранный сабпротокол
			string subprotocol = "";
			// Получаем список выбранных сабпротоколов
			const auto subprotocols = awh->subprotocols();
			// Если список выбранных сабпротоколов получен
			if(!subprotocols.empty())
				// Выполняем получение выбранного сабпротокола
				subprotocol = (* subprotocols.begin());
			// Если данные пришли в виде текста, выводим
			if(utf8){
				try {
					// Создаём объект JSON
					json data = json::parse(buffer.begin(), buffer.end());
					// Выводим полученный результат
					cout << " +++++++++++++ " << data.dump(4) << " == " << subprotocol << endl;
				// Обрабатываем ошибку
				} catch(const exception & e) {}
			// Сообщаем количество полученных байт
			} else cout << " +++++++++++++ " << buffer.size() << " bytes" << " == " << subprotocol << endl;
		}
		/**
		 * response Метод получения статуса результата запроса
		 * @param sid     идентификатор потока
		 * @param rid     идентификатор запроса
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 */
		void response(const int32_t sid, const uint64_t rid, const u_int code, const string & message){
			// Блокируем неиспользуемые переменные
			(void) rid;
			// Проверяем на наличие ошибок
			if(code >= 300)
				// Выводим сообщение о неудачном запросе
				this->_log->print("request failed: %u %s stream=%i", log_t::flag_t::WARNING, code, message.c_str(), sid);
		}
		/**
		 * active Метод идентификации активности на Web-клиенте
		 * @param mode режим события подключения
		 * @param awh  объект web-клиента
		 */
		void active(const client::web_t::mode_t mode, client::awh_t * awh){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			// Если подключение выполнено
			if(mode == client::web_t::mode_t::CONNECT){
				// Создаём объект URI
				uri_t uri(this->_fmk);
				// Создаём объект запроса
				client::web_t::request_t req;
				// Устанавливаем параметры запроса
				req.url = uri.parse("/stream");
				// Устанавливаем метод запроса
				req.method = web_t::method_t::GET;
				// Устанавливаем агента Websocket
				req.agent = client::web_t::agent_t::WEBSOCKET;
				// Устанавливаем тип компрессии данных
				req.compressors = {http_t::compressor_t::DEFLATE};
				// Выполняем первый запрос на сервер
				awh->send(std::move(req));
			}
		}
		/**
		 * entity Метод получения тела ответа сервера
		 * @param sid     идентификатор потока
		 * @param rid     идентификатор запроса
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param entity  тело ответа сервера
		 * @param awh     объект web-клиента
		 */
		void entity(const int32_t sid, const uint64_t rid, const u_int code, const string & message, const vector <char> & entity, client::awh_t * awh){
			// Блокируем неиспользуемые переменные
			(void) sid;
			(void) rid;
			/**
			 * Выполняем обработку ошибки
			 */
			try {
				// Получаем результат
				const string result(entity.begin(), entity.end());
				// Создаём объект JSON
				json data = json::parse(result);
				// Выводим полученный результат
				cout << " =========== " << data.dump(4) << endl;
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выводим полученный результат
				cout << " =========== " << string(entity.begin(), entity.end()) << endl;
			}
			// cout << " =========== " << result << " == " << res.code << " == " << res.ok << endl;
			// Выполняем остановку
			awh->stop();
		}
		/**
		 * headers Метод получения заголовков ответа сервера
		 * @param sid     идентификатор потока
		 * @param rid     идентификатор запроса
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param headers заголовки ответа сервера
		 */
		void headers(const int32_t sid, const uint64_t rid, const u_int code, const string & message, const unordered_multimap <string, string> & headers){
			// Блокируем неиспользуемые переменные
			(void) sid;
			(void) rid;
			// Переходим по всем заголовкам
			for(auto & header : headers)
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
		}
	public:
		/**
		 * WebClient Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		WebClient(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk{};
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk);
	// Создаём объект параметров SSL-шифрования
	node_t::ssl_t ssl;
	// Создаём биндинг
	client::core_t core(&fmk, &log);
	// Создаём объект WEB запроса
	client::awh_t awh(&core, &fmk, &log);
	// Создаём объект исполнителя
	WebClient executor(&fmk, &log);
	// Устанавливаем активный протокол подключения
	// core.proto(awh::engine_t::proto_t::HTTP2);
	core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Устанавливаем название сервиса
	log.name("WEB Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем постоянное подключение
	 * 2. Устанавливаем поддержку редиректов
	 * 3. Устанавливаем валидацию SSL сертификата
	 * 4. Устанавливаем разрешение использовать протокол Websocket
	 * 5. Устанавливаем флаг поддержания активным подключение
	 */
	awh.mode({
		// client::web_t::flag_t::NOT_STOP,
		client::web_t::flag_t::ALIVE,
		// client::web_t::flag_t::NOT_INFO,
		// client::web_t::flag_t::WAIT_MESS,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::WEBSOCKET_ENABLE,
		client::web_t::flag_t::TAKEOVER_CLIENT,
		client::web_t::flag_t::TAKEOVER_SERVER,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Отключаем валидацию сертификата
	ssl.verify = true;
	// Устанавливаем адрес сертификата
	ssl.ca = "./certs/ca.pem";
	/*
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "./certs/certificates/client-key.pem";
	ssl.cert = "./certs/certificates/client-cert.pem";
	*/
	// Выполняем установку параметров SSL-шифрования
	core.ssl(ssl);
	// Устанавливаем логин и пароль пользователя
	// awh.user("user", "password");
	// Устанавливаем длительное подключение
	// awh.keepAlive(2, 3, 1);
	// Устанавливаем длительное подключение
	// awh.keepAlive(100, 30, 10);
	// Отключаем таймер ожидания входящих данных
	// awh.waitTimeDetect(0, 0, CONNECT_TIMEOUT);
	// Устанавливаем данные прокси-сервера
	// awh.proxy("http://qKseEr:t5QrcW@212.102.146.33:8000");
	// awh.proxy("socks5://3JMFxD:CWv6MP@45.130.126.236:8000");
	// awh.proxy("socks5://127.0.0.1:2222");
	// awh.proxy("socks5://test1:test@127.0.0.1:2222");
	// awh.proxy("http://test1:password@127.0.0.1:2222");
	// awh.proxy("http://127.0.0.1:2222");
	// awh.proxy("socks5://unix:anyks", awh::scheme_t::family_t::NIX);
	// awh.proxy("http://unix:anyks", awh::scheme_t::family_t::NIX);
	
	
	// awh.proxy("http://3pvhoe:U8QFWd@193.56.188.250:8000");
	// awh.proxy("http://tARdXT:uWoRp1@217.29.62.214:13699");
	
	// awh.proxy("socks5://2faD0Q:mm9mw4@193.56.188.192:8000");
	// awh.proxy("socks5://kLV5jZ:ypKUKp@217.29.62.214:13700");
	
	// Устанавливаем сабпротоколы
	// awh.subprotocols({"test2", "test8", "test9"});
	// Устанавливаем тип компрессии
	// awh.compress({http_t::compressor_t::DEFLATE});
	// Устанавливаем тип авторизации прокси-сервера
	// awh.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию типа авторизации
	// awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
	// Подписываемся на событие запуска/остановки сервера
	awh.callback <void (const awh::core_t::status_t)> ("status", std::bind(&WebClient::status, &executor, _1));
	// Подписываемся на событие получения ошибки работы клиента
	awh.callback <void (const u_int, const string &)> ("errorWebsocket", std::bind(&WebClient::error, &executor, _1, _2));
	// Устанавливаем метод активации подключения
	awh.callback <void (const client::web_t::mode_t, client::awh_t *)> ("active", std::bind(&WebClient::active, &executor, _1, &awh));
	// Подписываемся на событие получения сообщения с сервера
	awh.callback <void (const vector <char> &, const bool, client::awh_t *)> ("messageWebsocket", std::bind(&WebClient::message, &executor, _1, _2, &awh));
	// Устанавливаем метод получения сообщения сервера
	awh.callback <void (const int32_t, const uint64_t, const u_int, const string &)> ("response", std::bind(&WebClient::response, &executor, _1, _2, _3, _4));
	// Подписываемся на событие рукопожатия
	awh.callback <void (const int32_t, const uint64_t, const client::web_t::agent_t, client::awh_t *)> ("handshake", std::bind(&WebClient::handshake, &executor, _1, _2, _3, &awh));
	// Устанавливаем метод получения тела ответа
	awh.callback <void (const int32_t, const uint64_t, const u_int, const string &, const vector <char> &, client::awh_t *)> ("entity", std::bind(&WebClient::entity, &executor, _1, _2, _3, _4, _5, &awh));
	// Устанавливаем метод получения заголовков
	awh.callback <void (const int32_t, const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", std::bind(&WebClient::headers, &executor, _1, _2, _3, _4, _5));
	// Выполняем инициализацию подключения	
	awh.init("wss://stream.binance.com:9443");
	// awh.init("wss://anyks.net:2222");
	// Выполняем запуск работы
	awh.start();
	// Выводим результат
	return 0;
}
