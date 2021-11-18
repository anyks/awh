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
#include <client/ws.hpp>
#include <nlohmann/json.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]) noexcept {
	// Создаём объект фреймворка
	fmk_t fmk(true);
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём биндинг
	coreCli_t core(&fmk, &log);
	// Создаём объект REST запроса
	wsCli_t ws(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("WebSocket Client");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем запрет остановки сервиса
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 * 4. Устанавливаем флаг поддержания активным подключение
	 */
	ws.setMode(
		(uint8_t) wsCli_t::flag_t::NOTSTOP |
		(uint8_t) wsCli_t::flag_t::WAITMESS |
		(uint8_t) wsCli_t::flag_t::VERIFYSSL |
		(uint8_t) wsCli_t::flag_t::KEEPALIVE
	);
	// Устанавливаем адрес сертификата
	core.setCA("./ca/cert.pem");
	// Устанавливаем логин и пароль пользователя
	// ws.setUser("user", "password");
	// Устанавливаем данные прокси-сервера
	// ws.setProxy("http://B80TWR:uRMhnd@196.17.249.64:8000");
	// ws.setProxy("socks5://rfbPbd:XcCuZH@45.144.169.109:8000");
	// ws.setProxy("socks5://6S7rAk:g6K8XD@217.29.62.231:30810");
	// Выполняем инициализацию типа авторизации
	// ws.setAuthType();
	// ws.setAuthType(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
	// Устанавливаем тип авторизации прокси-сервера
	// ws.setAuthTypeProxy();
	// Выполняем инициализацию WebSocket клиента
	ws.init("wss://stream.binance.com:9443/stream", http_t::compress_t::DEFLATE);
	// ws.init("ws://127.0.0.1:2222", http_t::compress_t::DEFLATE);
	// Устанавливаем шифрование
	// ws.setCrypt("PASS");
	// Устанавливаем сабпротоколы
	// ws.setSubs({"test2", "test8", "test9"});
	// Выполняем подписку на получение логов
	log.subscribe([](const log_t::flag_t flag, const string & message){
		// Выводим сообщение
		// cout << " ============= " << message << endl;
	});
	// Подписываемся на событие запуска и остановки сервера
	ws.on(&log, [](const wsCli_t::mode_t mode, wsCli_t * ws, void * ctx){
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s server", log_t::flag_t::INFO, (mode == wsCli_t::mode_t::CONNECT ? "Start" : "Stop"));
		// Если подключение произошло удачно
		if(mode == wsCli_t::mode_t::CONNECT){
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
	});
	// Подписываемся на событие получения ошибки работы клиента
	ws.on(&log, [](const u_int code, const string & mess, wsCli_t * ws, void * ctx){
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});
	// Подписываемся на событие получения сообщения с сервера
	ws.on(nullptr, [](const vector <char> & buffer, const bool utf8, wsCli_t * ws, void * ctx){
		// Если данные пришли в виде текста, выводим
		if(utf8){
			try {
				// Создаём объект JSON
				json data = json::parse(buffer.begin(), buffer.end());
				// Выводим полученный результат
				cout << " +++++++++++++ " << data.dump(4) << " == " << ws->getSub() << endl;
			// Обрабатываем ошибку
			} catch(const exception & e) {}
		// Сообщаем количество полученных байт
		} else cout << " +++++++++++++ " << buffer.size() << " bytes" << endl;
	});
	// Выполняем запуск WebSocket клиента
	ws.start();
	// Выводим результат
	return 0;
}
