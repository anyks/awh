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
	client::core_t core(&fmk, &log);
	// Создаём объект REST запроса
	client::rest_t rest(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("REST Client");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем отложенные вызовы
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 */
	rest.setMode(
		(uint8_t) client::rest_t::flag_t::DEFER |
		// (uint8_t) client::rest_t::flag_t::NOINFO |
		(uint8_t) client::rest_t::flag_t::WAITMESS |
		(uint8_t) client::rest_t::flag_t::VERIFYSSL
	);
	// Устанавливаем режим мультипоточной обработки
	core.setMultiThreads(true);
	// Устанавливаем адрес сертификата
	core.setCA("./ca/cert.pem");
	// Устанавливаем логин и пароль пользователя
	rest.setUser("user", "password");
	// Устанавливаем данные прокси-сервера
	// rest.setProxy("http://B80TWR:uRMhnd@196.17.249.64:8000");
	// rest.setProxy("socks5://rfbPbd:XcCuZH@45.144.169.109:8000");
	// rest.setProxy("socks5://6S7rAk:g6K8XD@217.29.62.231:30810");
	// Устанавливаем тип компрессии
	rest.setCompress(http_t::compress_t::ALL_COMPRESS);
	// Устанавливаем тип авторизации прокси-сервера
	// rest.setAuthTypeProxy();
	// Выполняем инициализацию типа авторизации
	// rest.setAuthType();
	// rest.setAuthType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем получение URL адреса сервера
	// uri_t::url_t url = uri.parseUrl("https://2ip.ru");
	// uri_t::url_t url = uri.parseUrl("https://www.anyks.com");
	// uri_t::url_t url = uri.parseUrl("https://apple.com");
	// uri_t::url_t url = uri.parseUrl("https://ru.wikipedia.org/wiki/HTTP");
	// uri_t::url_t url = uri.parseUrl("https://api.binance.com/api/v3/exchangeInfo?symbol=BTCUSDT");
	// uri_t::url_t url = uri.parseUrl("https://testnet.binance.vision/api/v3/exchangeInfo");
	uri_t::url_t url = uri.parseUrl("https://api.coingecko.com/api/v3/coins/list?include_platform=true");
	// Подписываемся на событие коннекта и дисконнекта клиента
	rest.on(&log, [](const client::rest_t::mode_t mode, client::rest_t * web, void * ctx){
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (mode == client::rest_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
	});
	/*
	// Подписываемся на событие получения сообщения
	rest.on(&log, [](const client::rest_t::res_t & res, client::rest_t * web, void * ctx){
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Переходим по всем заголовкам
		for(auto & header : res.headers){
			// Выводим информацию в лог
			log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
		}
		// Получаем результат
		const string result(res.entity.begin(), res.entity.end());
		// Создаём объект JSON
		json data = json::parse(result);
		// Выводим полученный результат
		cout << " =========== " << data.dump(4) << endl;
		// cout << " =========== " << result << " == " << res.code << " == " << res.ok << endl;
		// Выполняем остановку
		web->stop();
	});
	*/
	/*
	// Список запросов
	vector <client::rest_t::req_t> request;
	// Создаём объект запроса
	client::rest_t::req_t req1, req2, req3;
	// Устанавливаем URL адрес запроса
	req1.url = uri.parseUrl("http://127.0.0.1:2222/action/1/?test=12&goga=124");
	req2.url = uri.parseUrl("http://127.0.0.1:2222/action/2/?test=13&goga=125");
	req3.url = uri.parseUrl("http://127.0.0.1:2222/action/3/?test=14&goga=126");
	// Устанавливаем метод запроса
	req1.method = web_t::method_t::GET;
	req2.method = web_t::method_t::GET;
	req3.method = web_t::method_t::GET;
	// Добавляем объект запроса в список
	request.push_back(req1);
	request.push_back(req2);
	request.push_back(req3);
	// Формируем запрос
	rest.REST(request);
	// Выполняем REST запрос
	rest.start();
	*/
	// Замеряем время начала работы
	auto timeShifting = chrono::system_clock::now();
	// Формируем GET запрос
	const auto & body = rest.GET(url);
	// Выводим время запроса // 3862 || 3869 == 3893
	cout << " ++++++++++ Time Shifting " << chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now() - timeShifting).count() << endl;
	// const auto & body = rest.GET(url, {{"Connection", "close"}});
	// const auto & body = rest.GET(url, {{"User-Agent", "curl/7.64.1"}});
	// Если данные получены
	if(!body.empty()){
		// Создаём объект JSON
		json data = json::parse(body);
		// Выводим полученный результат
		cout << " =========== " << data.dump(4) << endl;
		// cout << " =========== " << string(body.begin(), body.end()) << endl;
	}
	// Выводим результат
	return 0;
}
