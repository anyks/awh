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
#include <core/rest.hpp>

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
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
	// Создаём биндинг
	core_t core(&fmk, &log);
	// Создаём объект REST запроса
	rest_t rest(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("REST Client");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	// Разрешаем верифицировать доменное имя на которое выдан сертификат
	core.setVerifySSL(true);
	// Выполняем инициализацию типа авторизации
	rest.setAuthType();
	// Выключаем анбиндинг после завершения запроса
	// rest.setUnbind(false);
	// Устанавливаем логин и пароль пользователя
	rest.setUser("user", "password");
	// Устанавливаем адрес сертификата
	core.setCA("./ca/cert.pem");
	// Устанавливаем данные прокси-сервера
	// rest.setProxy("http://B80TWR:uRMhnd@196.17.249.64:8000");
	// Устанавливаем тип авторизации прокси-сервера
	// rest.setAuthTypeProxy();
	// Выполняем получение URL адреса сервера
	uri_t::url_t url = uri.parseUrl("https://apple.com");// ("https://ru.wikipedia.org/wiki/HTTP");// ("https://www.anyks.com");
	// Выполняем запрос на получение данных
	const auto & result = rest.GET(url);// , {{"Connection", "close"}}); // {{"User-Agent", "curl/7.64.1"}});

	cout << " +++++++++++++ " << result << endl;

	// core.start();

	/*
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
	// Создаём объект клиента REST
	rest_t rest(&fmk, &log, &uri, &nwk);
	// Устанавливаем название сервиса
	log.setLogName("REST Client");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	// Разрешаем верифицировать доменное имя на которое выдан сертификат
	rest.setVerifySSL(true);
	// Выполняем инициализацию типа авторизации
	rest.setAuthType();
	// Устанавливаем логин и пароль пользователя
	rest.setUser("user", "password");
	// Устанавливаем адрес сертификата
	rest.setCA("./ca/cert.pem");

	rest.setProxy("http://B80TWR:uRMhnd@196.17.249.64:8000");

	// Устанавливаем параметры шифрования
	// rest.setCrypt("password");
	// Выполняем установку URL адреса сервера WebSocket
	uri_t::url_t url = uri.parseUrl("https://anyks.com");// ("https://api.binance.com/api/v3/exchangeInfo?symbol=BTCUSDT");
	// Выполняем запрос на получение данных
	const auto & result = rest.GET(url);

	cout << " +++++++++++++ " << result << endl;
	*/
	/*
	// Создаём объект JSON
	json data = json::parse(result);
	// Выводим полученный результат
	cout << " +++++++++++++ " << data.dump(4) << endl;
	*/
	// Выводим результат
	return 0;
}
