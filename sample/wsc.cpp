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
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
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
	ws.setProxy("socks5://rfbPbd:XcCuZH@45.144.169.109:8000");
	// ws.setProxy("socks5://6S7rAk:g6K8XD@217.29.62.231:30810");
	// Выполняем инициализацию типа авторизации
	// ws.setAuthType();
	// ws.setAuthType(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
	// Устанавливаем тип авторизации прокси-сервера
	ws.setAuthTypeProxy();
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
	ws.on(&log, [](const bool mode, wsCli_t * ws, void * ctx){
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s server", log_t::flag_t::INFO, (mode ? "Start" : "Stop"));
		// Если подключение произошло удачно
		if(mode){
			// Создаём объект JSON
			json data = json::object();
			// Формируем идентификатор объекта
			data["id"] = 1;
			// Формируем метод подписки
			data["method"] = "SUBSCRIBE";
			// Формируем параметры запрашиваемых криптовалютных пар
			data["params"] = json::array();
			// Формируем параметры криптовалютных пар
			data["params"][0] = "btcusdt@aggTrade";
			// Получаем параметры запроса в виде строки
			const string query = data.dump();
			// Отправляем сообщение на сервер
			ws->send(query.data(), query.size());
		}
	});
	// Подписываемся на событие получения ошибки работы клиента
	ws.on(&log, [](const u_short code, const string & mess, wsCli_t * ws, void * ctx){
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});
	// Подписываемся на событие получения сообщения с сервера
	ws.on(nullptr, [](const vector <char> & buffer, const bool utf8, wsCli_t * ws, void * ctx){
		// Если данные пришли в виде текста, выводим
		if(utf8){
			// Создаём объект JSON
			json data = json::parse(buffer.begin(), buffer.end());
			// Выводим полученный результат
			cout << " +++++++++++++ " << data.dump(4) << " == " << ws->getSub() << endl;
		// Сообщаем количество полученных байт
		} else cout << " +++++++++++++ " << buffer.size() << " bytes" << endl;
	});
	// Выполняем запуск WebSocket клиента
	ws.start();
	// Выводим результат
	return 0;
}
