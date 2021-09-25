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
#include <ws/client.hpp>
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
	/*
	// Создаём объект клиента WebSocket
	client_t ws(&fmk, &log, &uri, &nwk);
	// Устанавливаем название сервиса
	log.setLogName("WebSocket Client");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	// Разрешаем верифицировать доменное имя на которое выдан сертификат
	ws.setVerifySSL(true);
	// Разрешаем ожидать входящие сообщения
	ws.setWaitMessage(true);
	// Разрешаем автоматическое восстановление подключения
	ws.setAutoReconnect(true);
	// Выполняем инициализацию типа авторизации
	ws.setAuthType();
	// Устанавливаем логин и пароль пользователя
	ws.setUser("user", "password");
	// Устанавливаем адрес сертификата
	ws.setCA("./ca/cert.pem");
	// Устанавливаем параметры шифрования
	// ws.setCrypt("password");
	// Выполняем инициализацию WebSocket клиента
	ws.init("wss://stream.binance.com:9443/stream", http_t::compress_t::DEFLATE);
	//
	// Выполняем подписку на получение логов
	log.subscribe([](const log_t::flag_t flag, const string & message){
		// Выводим сообщение
		cout << " ============= " << message << endl;
	});
	//
	// Устанавливаем адрес файла для сохранения логов
	// log.setLogFilename("./test.log");
	// Подписываемся на событие запуска и остановки сервера
	ws.on([](const bool mode, client_t * ws){
		// Выводим сообщение
		cout << " +++++++++++++ " << (mode ? "Start" : "Stop") << " server" << endl;
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
	ws.on([](const u_short code, const string & mess, client_t * ws){
		// Выводим сообщение об ошибке
		cout << " +++++++++++++ " << code << " === " << mess << endl;
	});
	// Подписываемся на событие ответа сервера PONG
	ws.on([](const string & mess, client_t * ws){
		cout << " +++++++++++++ " << "PONG" << " === " << mess << endl;
	});
	// Подписываемся на событие получения сообщения с сервера
	ws.on([](const vector <char> & buffer, const bool utf8, client_t * ws){
		// Если данные пришли в виде текста, выводим
		if(utf8){
			// Создаём объект JSON
			json data = json::parse(string(buffer.begin(), buffer.end()));
			// Выводим полученный результат
			cout << " +++++++++++++ " << data.dump(4) << endl;
		// Сообщаем количество полученных байт
		} else cout << " +++++++++++++ " << buffer.size() << " bytes" << endl;
	});
	// Выполняем запуск WebSocket клиента
	ws.start();
	*/
	// Выводим результат
	return 0;
}
