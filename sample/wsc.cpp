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
#include <nlohmann/json.hpp>
#include <client/websocket.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;
using namespace awh::client;

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/**
 * Executor Класс объекта исполнителя
 */
class Executor {
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * subscribe Метод подписки на сообщения логов
		 * @param flag    флаг сообщения лога
		 * @param message текст сообщения лога
		 */
		void subscribe(const log_t::flag_t flag, const string & message){
			// Выводим сообщение
			// cout << " ============= " << message << endl;
		}
	public:
		/**
		 * active Метод идентификации активности на WebSocket клиенте
		 * @param mode режим события подключения
		 * @param ws   объект WebSocket клиента
		 */
		void active(const websocket_t::mode_t mode, websocket_t * ws){
			// Выводим информацию в лог
			this->_log->print("%s server", log_t::flag_t::INFO, (mode == websocket_t::mode_t::CONNECT ? "Start" : "Stop"));
			// Если подключение произошло удачно
			if(mode == websocket_t::mode_t::CONNECT){
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
				ws->send(query.data(), query.size());
			}
		}
		/**
		 * error Метод вывода ошибок WebSocket клиента
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 * @param ws   объект WebSocket клиента
		 */
		void error(const u_int code, const string & mess, websocket_t * ws){
			// Выводим информацию в лог
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		/**
		 * message Метод получения сообщений
		 * @param buffer бинарный буфер сообщения
		 * @param utf8   тип буфера сообщения
		 * @param ws     объект WebSocket клиента
		 */
		void message(const vector <char> & buffer, const bool utf8, websocket_t * ws){
			// Если данные пришли в виде текста, выводим
			if(utf8){
				try {
					// Создаём объект JSON
					json data = json::parse(buffer.begin(), buffer.end());
					// Выводим полученный результат
					cout << " +++++++++++++ " << data.dump(4) << " == " << ws->sub() << endl;
				// Обрабатываем ошибку
				} catch(const exception & e) {}
			// Сообщаем количество полученных байт
			} else cout << " +++++++++++++ " << buffer.size() << " bytes" << " == " << ws->sub() << endl;
		}
	public:
		/**
		 * Executor Конструктор
		 * @param log объект логирования
		 */
		Executor(log_t * log) : _log(log) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект исполнителя
	Executor executor(&log);
	// Создаём биндинг сетевого ядра
	client::core_t core(&fmk, &log);
	// Создаём объект WebSocket клиента
	websocket_t ws(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.name("WebSocket Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем запрет остановки сервиса
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 * 4. Устанавливаем флаг поддержания активным подключение
	 */
	ws.mode({
		// websocket_t::flag_t::NOT_STOP,
		websocket_t::flag_t::ALIVE,
		// websocket_t::flag_t::WAIT_MESS,
		websocket_t::flag_t::VERIFY_SSL,
		websocket_t::flag_t::TAKEOVER_CLIENT,
		websocket_t::flag_t::TAKEOVER_SERVER
	});
	// Разрешаем простое чтение базы событий
	// core.frequency(0);
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	core.ca("./ca/cert.pem");
	// Устанавливаем активный протокол подключения
	core.proto(awh::engine_t::proto_t::HTTP2);
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	// core.sonet(awh::scheme_t::sonet_t::DTLS);
	// core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	core.sonet(awh::scheme_t::sonet_t::TCP);
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
	// Отключаем валидацию сертификата
	// core.verifySSL(false);
	// core.certificate("./ca/certs/client-cert.pem", "./ca/certs/client-key.pem");
	// Устанавливаем логин и пароль пользователя
	// ws.user("user", "password");
	// Выполняем активацию многопоточности
	// ws.multiThreads(22);
	// Устанавливаем данные прокси-сервера
	// ws.proxy("http://qKseEr:t5QrcW@212.102.146.33:8000");
	// ws.proxy("socks5://3JMFxD:CWv6MP@45.130.126.236:8000");
	// ws.proxy("http://fn3nzc:GZJAeP@217.29.62.232:11283");
	// ws.proxy("socks5://xYkj89:eqCQJA@85.195.81.167:12387");
	// ws.proxy("socks5://test1:password@127.0.0.1:2222");
	// ws.proxy("http://127.0.0.1:2222");
	// ws.proxy("http://test1:password@127.0.0.1:2222");
	// ws.proxy("socks5://unix:anyks", awh::scheme_t::family_t::NIX);
	// ws.proxy("http://unix:anyks", awh::scheme_t::family_t::NIX);
	// Выполняем инициализацию типа авторизации
	// ws.authType(awh::auth_t::type_t::BASIC);
	// ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::SHA256);
	// Устанавливаем тип авторизации прокси-сервера
	// ws.authTypeProxy();
	// Устанавливаем тип авторизации прокси-сервера
	// ws.authTypeProxy(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);
	// Выполняем инициализацию WebSocket клиента
	ws.init("wss://stream.binance.com:9443/stream", awh::http_t::compress_t::DEFLATE);
	// ws.init("ws://127.0.0.1:2222", http_t::compress_t::DEFLATE);
	// ws.init("wss://mimi.anyks.net:2222", http_t::compress_t::DEFLATE);
	// ws.init("wss://92.63.110.56:2222", http_t::compress_t::DEFLATE);
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	// ws.init("anyks", http_t::compress_t::DEFLATE);
	// Устанавливаем шифрование
	// ws.crypto("PASS");
	// Устанавливаем сабпротоколы
	ws.subs({"test2", "test8", "test9"});
	// Выполняем подписку на получение логов
	log.subscribe(bind(&Executor::subscribe, &executor, _1, _2));
	// Подписываемся на событие запуска и остановки сервера
	ws.on(bind(&Executor::active, &executor, _1, _2));
	// Подписываемся на событие получения ошибки работы клиента
	ws.on(bind(&Executor::error, &executor, _1, _2, _3));
	// Подписываемся на событие получения сообщения с сервера
	ws.on(bind(&Executor::message, &executor, _1, _2, _3));
	// Выполняем запуск WebSocket клиента
	ws.start();	
	// Выводим результат
	return 0;
}
