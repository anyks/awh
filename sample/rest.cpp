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
 * WebClient Класс объекта исполнителя
 */
class WebClient {
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * active Метод идентификации активности на WebClient клиенте
		 * @param mode режим события подключения
		 * @param web  объект WebClient-а
		 */
		void active(const client::rest_t::mode_t mode, client::rest_t * web){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::rest_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * message Метод получения сообщений
		 * @param res объект ответа с Web-сервера
		 * @param web объект WebClient-а
		 */
		void message(const client::rest_t::res_t & res, client::rest_t * web){
			// Переходим по всем заголовкам
			for(auto & header : res.headers){
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
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
		}
	public:
		/**
		 * WebClient Конструктор
		 * @param log объект логирования
		 */
		WebClient(log_t * log) : _log(log) {}
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
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
	// Создаём объект исполнителя
	WebClient executor(&log);
	// Создаём биндинг
	client::core_t core(true, &fmk, &log);
	// Создаём объект REST запроса
	client::rest_t rest(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.name("REST Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем отложенные вызовы
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 */
	rest.mode(
		// (uint8_t) client::rest_t::flag_t::NOINFO |
		(uint8_t) client::rest_t::flag_t::WAITMESS |
		(uint8_t) client::rest_t::flag_t::REDIRECTS |
		(uint8_t) client::rest_t::flag_t::VERIFYSSL
	);
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	core.ca("./ca/cert.pem");
	// Устанавливаем логин и пароль пользователя
	rest.user("user", "password");
	// Устанавливаем данные прокси-сервера
	// rest.proxy("http://qKseEr:t5QrcW@212.102.146.33:8000");
	// rest.proxy("socks5://3JMFxD:CWv6MP@45.130.126.236:8000");
	// rest.proxy("socks5://127.0.0.1:2222");
	// rest.proxy("socks5://test1:test@127.0.0.1:2222");
	// rest.proxy("http://test1:password@127.0.0.1:2222");
	// rest.proxy("http://127.0.0.1:2222");
	// rest.proxy("socks5://unix:anyks", awh::scheme_t::family_t::NIX);
	// rest.proxy("http://unix:anyks", awh::scheme_t::family_t::NIX);
	// rest.proxy("http://fn3nzc:GZJAeP@217.29.62.232:11283");
	// rest.proxy("socks5://xYkj89:eqCQJA@85.195.81.167:12387");
	// Устанавливаем тип компрессии
	rest.compress(http_t::compress_t::ALL_COMPRESS);
	// Устанавливаем тип авторизации прокси-сервера
	// rest.authTypeProxy();
	// rest.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию типа авторизации
	// rest.authType();
	// rest.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем получение URL адреса сервера
	// uri_t::url_t url = uri.parse("https://2ip.ru");
	// uri_t::url_t url = uri.parse("https://ipv6.google.com");
	// uri_t::url_t url = uri.parse("http://localhost/test");
	// uri_t::url_t url = uri.parse("https://www.anyks.com");
	uri_t::url_t url = uri.parse("https://apple.com/ru/mac");
	// uri_t::url_t url = uri.parse("https://ru.wikipedia.org/wiki/HTTP");
	// uri_t::url_t url = uri.parse("https://api.binance.com/api/v3/exchangeInfo?symbol=BTCUSDT");
	// uri_t::url_t url = uri.parse("https://testnet.binance.vision/api/v3/exchangeInfo");
	// uri_t::url_t url = uri.parse("https://api.coingecko.com/api/v3/coins/list?include_platform=true");
	// Подписываемся на событие коннекта и дисконнекта клиента
	rest.on(bind(&WebClient::active, &executor, _1, _2));
	// Подписываемся на событие получения сообщения
	// rest.on(bind(&WebClient::message, &executor, _1, _2));
	/*
	// Список запросов
	vector <client::rest_t::req_t> request;
	// Создаём объект запроса
	client::rest_t::req_t req1, req2, req3;
	// Устанавливаем URL адрес запроса
	req1.url = uri.parse("http://127.0.0.1:2222/action/1/?test=12&goga=124");
	req2.url = uri.parse("http://127.0.0.1:2222/action/2/?test=13&goga=125");
	req3.url = uri.parse("http://127.0.0.1:2222/action/3/?test=14&goga=126");
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
	// const auto & body = rest.GET(url);
	// const auto & body = rest.GET(url, {{"Connection", "close"}});
	const auto & body = rest.GET(url, {{"User-Agent", "curl/7.64.1"}});
	// Выводим время запроса // 3862 || 3869 == 3893
	cout << " ++++++++++ Time Shifting " << chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now() - timeShifting).count() << endl;
	// Если данные получены
	if(!body.empty()){
		// Создаём объект JSON
		// json data = json::parse(body);
		// Выводим полученный результат
		// cout << " =========== " << data.dump(4) << endl;
		cout << " =========== " << string(body.begin(), body.end()) << endl;
	}
	// Выводим результат
	return 0;
}
