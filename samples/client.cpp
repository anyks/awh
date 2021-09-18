/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

/**
 * Подключаем заголовочный файл торгового бота
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
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
	// Создаём объект клиента WebSocket
	client_t ws(&fmk, &uri, &nwk);
	// Разрешаем верифицировать доменное имя на которое выдан сертификат
	ws.setVerifySSL(true);
	// Разрешаем автоматическое восстановление подключения
	ws.setAutoReconnect(true);
	// Выполняем инициализацию типа авторизации
	ws.setAuthType();
	// Устанавливаем логин и пароль пользователя
	ws.setUser("user", "password");
	// Устанавливаем адрес сертификата
	ws.setCA("./ca/cert.pem");
	// Выполняем инициализацию WebSocket клиента
	ws.init("wss://stream.binance.com:9443/stream", true);
	// Подписываемся на событие запуска и остановки сервера
	ws.on([](bool mode, client_t * ws){
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
			ws->send(query.data(), query.size(), true);
		}
	});
	// Подписываемся на событие получения ошибки работы клиента
	ws.on([](u_short code, const string & mess, client_t * ws){
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
		if(utf8) cout << " +++++++++++++ " << string(buffer.begin(), buffer.end()) << endl;
		// Сообщаем количество полученных байт
		else cout << " +++++++++++++ " << buffer.size() << " bytes" << endl;
	});
	// Выполняем запуск WebSocket клиента
	ws.start();
	// Выводим результат
	return 0;
}
