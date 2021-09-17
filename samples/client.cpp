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

// Подключаем пространство имён
using namespace std;
using namespace awh;

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
	ws.init("wss://stream.binance.com:9443/ws/btcusdt@aggTrade", true);
	// Выполняем запуск WebSocket клиента
	ws.start();
	// Выводим результат
	return 0;
}
