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
#include <client/web.hpp>

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
		 * active Метод идентификации активности на Web-клиенте
		 * @param mode режим события подключения
		 * @param web  объект WebClient-а
		 */
		void active(const client::web_t::mode_t mode, client::web_t * web){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			// Если подключение выполнено
			if(mode == client::web_t::mode_t::CONNECT){
				// Создаём объект запроса
				client::web_t::req_t req1, req2;
				// Устанавливаем метод запроса
				req1.method = web_t::method_t::GET;
				// Устанавливаем метод запроса
				req2.method = web_t::method_t::GET;
				// Устанавливаем параметры запроса
				req1.query = "/api/v3/exchangeInfo?symbol=BTCUSDT";
				// Устанавливаем параметры запроса
				req2.query = "/api/v3/exchangeInfo";
				// Выполняем запрос на сервер
				web->send({req1, req2});
			}
		}
		/**
		 * message Метод получения сообщений
		 * @param res объект ответа с сервера
		 * @param web объект WebClient-а
		 */
		void message(const client::web_t::res_t & res, client::web_t * web){
			/**
			 * Выполняем обработку ошибки
			 */
			try {
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
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выводим полученный результат
				cout << " =========== " << string(res.entity.begin(), res.entity.end()) << endl;
			}
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
	fmk_t fmk{};
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk);
	// Создаём объект исполнителя
	WebClient executor(&log);
	// Создаём биндинг
	client::core_t core(&fmk, &log);
	// Создаём объект WEB запроса
	client::web_t web(&core, &fmk, &log);
	// Устанавливаем активный протокол подключения
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Устанавливаем название сервиса
	log.name("WEB Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем отложенные вызовы
	 * 2. Устанавливаем ожидание входящих сообщений
	 * 3. Устанавливаем валидацию SSL сертификата
	 */
	web.mode(
		// (uint8_t) client::web_t::flag_t::ALIVE |
		// (uint8_t) client::web_t::flag_t::NOT_INFO |
		// (uint8_t) client::web_t::flag_t::WAIT_MESS |
		(uint8_t) client::web_t::flag_t::REDIRECTS |
		(uint8_t) client::web_t::flag_t::VERIFY_SSL
	);
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	core.ca("./ca/cert.pem");
	// Устанавливаем логин и пароль пользователя
	web.user("user", "password");
	// Устанавливаем длительное подключение
	// web.keepAlive(2, 3, 1);
	// Устанавливаем длительное подключение
	// web.keepAlive(100, 30, 10);
	// Отключаем таймер ожидания входящих данных
	// web.waitTimeDetect(0, 0, CONNECT_TIMEOUT);
	// Устанавливаем данные прокси-сервера
	// web.proxy("http://qKseEr:t5QrcW@212.102.146.33:8000");
	// web.proxy("socks5://3JMFxD:CWv6MP@45.130.126.236:8000");
	// web.proxy("socks5://127.0.0.1:2222");
	// web.proxy("socks5://test1:test@127.0.0.1:2222");
	// web.proxy("http://test1:password@127.0.0.1:2222");
	// web.proxy("http://127.0.0.1:2222");
	// web.proxy("socks5://unix:anyks", awh::scheme_t::family_t::NIX);
	// web.proxy("http://unix:anyks", awh::scheme_t::family_t::NIX);
	// web.proxy("http://fn3nzc:GZJAeP@217.29.62.232:11283");
	// web.proxy("socks5://xYkj89:eqCQJA@85.195.81.167:12387");
	// Устанавливаем тип компрессии
	// web.compress(http_t::compress_t::ALL_COMPRESS);
	// Устанавливаем тип авторизации прокси-сервера
	// web.authTypeProxy();
	// web.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию типа авторизации
	// web.authType();
	// web.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем получение URL адреса сервера
	// uri_t::url_t url = uri.parse("https://2ip.ru");
	// uri_t::url_t url = uri.parse("https://ipv6.google.com");
	// uri_t::url_t url = uri.parse("http://localhost/test");
	// uri_t::url_t url = uri.parse("https://anyks.com");
	// uri_t::url_t url = uri.parse("https://www.anyks.com");
	// uri_t::url_t url = uri.parse("https://anyks.com/test.php");
	// uri_t::url_t url = uri.parse("https://www.anyks.com/test.php");
	uri_t::url_t url = uri.parse("https://apple.com/ru/mac");
	// uri_t::url_t url = uri.parse("https://ru.wikipedia.org/wiki/HTTP");
	// uri_t::url_t url = uri.parse("https://api.binance.com/api/v3/exchangeInfo?symbol=BTCUSDT");
	// uri_t::url_t url = uri.parse("https://testnet.binance.vision/api/v3/exchangeInfo");
	// uri_t::url_t url = uri.parse("https://api.coingecko.com/api/v3/coins/list?include_platform=true");
	// uri_t::url_t url = uri.parse("https://api.coingecko.com/api/v3/simple/price?ids=tron&vs_currencies=usd");
	/*
	// Подписываемся на событие коннекта и дисконнекта клиента
	web.on(bind(&WebClient::active, &executor, _1, _2));
	// Подписываемся на событие получения сообщения
	web.on(bind(&WebClient::message, &executor, _1, _2));
	// Выполняем инициализацию подключения
	web.init("https://api.binance.com");
	// Выполняем запуск работы
	web.start();
	*/

	// 1. Реализация Web-сервера
	// 2. Протестировать работу своего прокси-сервера и прокси-клиента
	// 3. Протестировать авторизацию
	// 4. С LibEvent есть затыки в получении данных, база событий отказывается работать

	// 5. Сделать методы остановки для Danube и протестить с названиями архивов с точками

	// Замеряем время начала работы
	auto timeShifting = chrono::system_clock::now();
	// Формируем GET запрос
	// const auto & body = web.GET(url);
	// const auto & body = web.GET(url, {{"Connection", "close"}});
	const auto & body = web.GET(url, {{"User-Agent", "curl/7.64.1"}});
	// Подготавливаем тело запроса
	// const string entity = "<html><head><title>404</title></head><body><h1>Hello World!!!</h1></body></html>";
	// Выполняем тело запроса на сервер
	// const auto & body = web.POST(url, vector <char> (entity.begin(), entity.end()), {{"User-Agent", "curl/7.64.1"}});
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
