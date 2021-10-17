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
#include <server/ws.hpp>
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
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
	// Создаём биндинг
	coreSrv_t core(&fmk, &log);
	// Создаём объект REST запроса
	wsSrv_t ws(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("WebSocket Server");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	// ws.setMode((uint8_t) wsSrv_t::flag_t::WAITMESS);
	// Выполняем инициализацию WebSocket сервера
	ws.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	ws.on(nullptr, [](const string & ip, const string & mac, wsSrv_t * ws, void * ctx) -> bool {

		cout << " +++++++++++++++ ACCEPT " << ip << " == " << mac << endl;

		// Разрешаем подключение клиенту
		return true;
	});
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	ws.on(&log, [](const size_t aid, const bool mode,  wsSrv_t * ws, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (mode ? "Connect" : "Disconnect"));
	});
	// Установливаем функцию обратного вызова на событие получения ошибок
	ws.on(&log, [](const size_t aid, const u_short code, const string & mess,  wsSrv_t * ws, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});
	/// Установливаем функцию обратного вызова на событие получения сообщений
	ws.on(nullptr, [](const size_t aid, const vector <char> & buffer, const bool utf8,  wsSrv_t * ws, void * ctx) noexcept {
		// Если даныне получены
		if(!buffer.empty()){

			cout << " !!!!!!!!!! " << string(buffer.begin(), buffer.end()) << endl;

			// Отправляем сообщение обратно
			ws->send(aid, buffer.data(), buffer.size(), utf8);
		}
	});
	// Выполняем запуск WebSocket клиента
	ws.start();
	// Устанавливаем адрес сертификата
	// core.setCA("./ca/cert.pem");
	// Устанавливаем логин и пароль пользователя
	// ws.setUser("user", "password");
	// Устанавливаем данные прокси-сервера
	// ws.setProxy("http://B80TWR:uRMhnd@196.17.249.64:8000");
	// ws.setProxy("socks5://rfbPbd:XcCuZH@45.144.169.109:8000");
	// ws.setProxy("socks5://6S7rAk:g6K8XD@217.29.62.231:30810");
	// Выполняем инициализацию типа авторизации
	// ws.setAuthType();
	// Устанавливаем тип авторизации прокси-сервера
	// ws.setAuthTypeProxy();
	// Устанавливаем время ожидания
	// ws.setWaitTimeDetect(10, 0);
	// Выполняем инициализацию WebSocket клиента
	// ws.init("wss://stream.binance.com:9443/stream", http_t::compress_t::DEFLATE);
	/*
	// Выполняем подписку на получение логов
	log.subscribe([](const log_t::flag_t flag, const string & message){
		// Выводим сообщение
		// cout << " ============= " << message << endl;
	});
	*/
	/*
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
			cout << " +++++++++++++ " << data.dump(4) << endl;
		// Сообщаем количество полученных байт
		} else cout << " +++++++++++++ " << buffer.size() << " bytes" << endl;
	});
	*/
	// Выполняем запуск WebSocket клиента
	// ws.start();
	// Выводим результат
	return 0;
}
