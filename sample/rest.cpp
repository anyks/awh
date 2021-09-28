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
#include <client/rest.hpp>

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
	ccl_t core(&fmk, &log);
	// Создаём объект REST запроса
	rest_t rest(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("REST Client");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 * 2. Устанавливаем валидацию SSL сертификата
	 */
	rest.setMode((u_short) rest_t::flag_t::WAITMESS | (u_short) rest_t::flag_t::VERIFYSSL);
	// Устанавливаем адрес сертификата
	core.setCA("./ca/cert.pem");
	// Устанавливаем логин и пароль пользователя
	rest.setUser("user", "password");
	// Устанавливаем данные прокси-сервера
	rest.setProxy("http://B80TWR:uRMhnd@196.17.249.64:8000");
	// Выполняем инициализацию типа авторизации
	rest.setAuthType();
	// Устанавливаем тип авторизации прокси-сервера
	rest.setAuthTypeProxy();
	// Выполняем получение URL адреса сервера
	// uri_t::url_t url = uri.parseUrl("https://2ip.ru");
	// uri_t::url_t url = uri.parseUrl("https://www.anyks.com");
	// uri_t::url_t url = uri.parseUrl("https://www.apple.com");
	// uri_t::url_t url = uri.parseUrl("https://ru.wikipedia.org/wiki/HTTP");
	uri_t::url_t url = uri.parseUrl("https://api.binance.com/api/v3/exchangeInfo?symbol=BTCUSDT");
	// Выполняем запрос на получение IP адреса
	const auto & body = rest.GET(url);
	// const auto & body = rest.GET(url, {{"Connection", "close"}});
	// const auto & body = rest.GET(url, {{"User-Agent", "curl/7.64.1"}});
	// Получаем результат
	const string result(body.begin(), body.end());
	// Создаём объект JSON
	json data = json::parse(result);
	// Выводим полученный результат
	// cout << " +++++++++++++ " << result << endl;
	cout << " +++++++++++++ " << data.dump(4) << endl;
	// Выводим результат
	return 0;
}
