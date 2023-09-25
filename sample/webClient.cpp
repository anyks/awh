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
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	private:
		// Объект веб-клиента
		client::rest_t * _rest;
	public:
		/**
		 * message Метод получения статуса результата запроса
		 * @param id      идентификатор потока
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 */
		void message(const int32_t id, const u_int code, const string & message){
			// Проверяем на наличие ошибок
			if(code >= 300)
				// Выводим сообщение о неудачном запросе
				this->_log->print("request failed: %u %s stream=%i", log_t::flag_t::WARNING, code, message.c_str(), id);
		}
		/**
		 * active Метод идентификации активности на Web-клиенте
		 * @param mode режим события подключения
		 */
		void active(const client::web_t::mode_t mode){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			// Если подключение выполнено
			if(mode == client::web_t::mode_t::CONNECT){
				// Создаём объект URI
				uri_t uri(this->_fmk);
				// Создаём объект запроса
				client::web_t::request_t req1, req2;
				// Устанавливаем метод запроса
				req1.method = web_t::method_t::GET;
				// Устанавливаем метод запроса
				req2.method = web_t::method_t::GET;
				// Устанавливаем параметры запроса
				req1.url = uri.parse("/api/v3/exchangeInfo?symbol=BTCUSDT");
				// Устанавливаем параметры запроса
				req2.url = uri.parse("/api/v3/exchangeInfo");
				// Выполняем первый запрос на сервер
				this->_rest->send(client::web_t::agent_t::HTTP, req1);
				// Выполняем второй запрос на сервер
				this->_rest->send(client::web_t::agent_t::HTTP, req2);
			}
		}
		/**
		 * entity Метод получения тела ответа сервера
		 * @param id      идентификатор потока
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param entity  тело ответа сервера
		 */
		void entity(const int32_t id, const u_int code, const string & message, const vector <char> & entity){
			/**
			 * Выполняем обработку ошибки
			 */
			try {
				// Получаем результат
				const string result(entity.begin(), entity.end());
				// Создаём объект JSON
				json data = json::parse(result);
				// Выводим полученный результат
				cout << " =========== " << data.dump(4) << endl;
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				// Выводим полученный результат
				cout << " =========== " << string(entity.begin(), entity.end()) << endl;
			}
			// cout << " =========== " << result << " == " << res.code << " == " << res.ok << endl;
			// Выполняем остановку
			this->_rest->stop();
		}
		/**
		 * headers Метод получения заголовков ответа сервера
		 * @param id      идентификатор потока
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param headers заголовки ответа сервера
		 */
		void headers(const int32_t id, const u_int code, const string & message, const unordered_multimap <string, string> & headers){
			// Переходим по всем заголовкам
			for(auto & header : headers)
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
		}
	public:
		/**
		 * WebClient Конструктор
		 * @param fmk  объект фреймворка
		 * @param log  объект логирования
		 * @param rest объект REST-клиента
		 */
		WebClient(const fmk_t * fmk, const log_t * log, client::rest_t * rest) : _fmk(fmk), _log(log), _rest(rest) {}
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
	// Создаём биндинг
	client::core_t core(&fmk, &log);
	// Создаём объект WEB запроса
	client::rest_t rest(&core, &fmk, &log);
	// Создаём объект исполнителя
	WebClient executor(&fmk, &log, &rest);
	// Устанавливаем активный протокол подключения
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Устанавливаем название сервиса
	log.name("WEB Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем поддержку редиректов
	 * 2. Устанавливаем валидацию SSL сертификата
	 */
	rest.mode({
		// client::web_t::flag_t::ALIVE,
		// client::web_t::flag_t::NOT_INFO,
		// client::web_t::flag_t::WAIT_MESS,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::VERIFY_SSL,
		// client::web_t::flag_t::PROXY_NOCONNECT
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	core.ca("./ca/cert.pem");
	// Устанавливаем логин и пароль пользователя
	// rest.user("user", "password");
	// Устанавливаем длительное подключение
	// rest.keepAlive(2, 3, 1);
	// Устанавливаем длительное подключение
	// rest.keepAlive(100, 30, 10);
	// Отключаем таймер ожидания входящих данных
	// rest.waitTimeDetect(0, 0, CONNECT_TIMEOUT);
	// Устанавливаем данные прокси-сервера
	// rest.proxy("http://qKseEr:t5QrcW@212.102.146.33:8000");
	// rest.proxy("socks5://3JMFxD:CWv6MP@45.130.126.236:8000");
	// rest.proxy("socks5://127.0.0.1:2222");
	// rest.proxy("socks5://test1:test@127.0.0.1:2222");
	// rest.proxy("http://test1:password@127.0.0.1:2222");
	// rest.proxy("http://127.0.0.1:2222");
	// rest.proxy("socks5://unix:anyks", awh::scheme_t::family_t::NIX);
	// rest.proxy("http://unix:anyks", awh::scheme_t::family_t::NIX);
	
	
	rest.proxy("http://3pvhoe:U8QFWd@193.56.188.250:8000");
	// rest.proxy("http://tARdXT:uWoRp1@217.29.62.214:13699");
	
	// rest.proxy("socks5://2faD0Q:mm9mw4@193.56.188.192:8000");
	// rest.proxy("socks5://kLV5jZ:ypKUKp@217.29.62.214:13700");
	
	// Устанавливаем тип компрессии
	// rest.compress(http_t::compress_t::ALL_COMPRESS);
	// Устанавливаем тип авторизации прокси-сервера
	rest.authTypeProxy();
	// rest.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию типа авторизации
	// rest.authType();
	// rest.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем получение URL адреса сервера
	// uri_t::url_t url = uri.parse("https://2ip.ru");
	// uri_t::url_t url = uri.parse("https://ipv6.google.com");
	// uri_t::url_t url = uri.parse("http://localhost/test");
	// uri_t::url_t url = uri.parse("http://stalin.info");
	// uri_t::url_t url = uri.parse("http://ww38.stalin.info/");
	// uri_t::url_t url = uri.parse("http://anyks.com");
	// uri_t::url_t url = uri.parse("http://www.anyks.com");
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
	// Устанавливаем метод активации подключения
	rest.on((function <void (const client::web_t::mode_t)>) std::bind(&WebClient::active, &executor, _1));
	// Устанавливаем метод получения сообщения сервера
	rest.on((function <void (const int32_t, const u_int, const string &)>) std::bind(&WebClient::message, &executor, _1, _2, _3));
	// Устанавливаем метод получения тела ответа
	rest.on((function <void (const int32_t, const u_int, const string &, const vector <char> &)>) std::bind(&WebClient::entity, &executor, _1, _2, _3, _4));
	// Устанавливаем метод получения заголовков
	rest.on((function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&WebClient::headers, &executor, _1, _2, _3, _4));
	// Выполняем инициализацию подключения
	rest.init("https://api.binance.com");
	// Выполняем запуск работы
	rest.start();
	*/

	// 1. Реализация Web-сервера
	// 2. Протестировать работу своего прокси-сервера и прокси-клиента
	// 3. Протестировать авторизацию
	// 4. С LibEvent есть затыки в получении данных, база событий отказывается работать

	// 1. Отладить формирование запроса на WebSocket/http2
	// 2. Разобраться в принципе передачи данных по WebSocket/http2
	// 3. Сделать поддержку расширений в WebSocket

	// 1. Сделать методы остановки для Danube и протестить с названиями архивов с точками

	// Замеряем время начала работы
	auto timeShifting = chrono::system_clock::now();
	// Формируем GET запрос
	const auto & body = rest.GET(url);
	// const auto & body = rest.GET(url, {{"Connection", "close"}});
	// const auto & body = rest.GET(url, {{"User-Agent", "curl/7.64.1"},{"Proxy-Authorization", "Basic M3B2aG9lOlU4UUZXZA=="}});
	// Подготавливаем тело запроса
	// const string entity = "<html><head><title>404</title></head><body><h1>Hello World!!!</h1></body></html>";
	// Выполняем тело запроса на сервер
	// const auto & body = rest.POST(url, vector <char> (entity.begin(), entity.end()), {{"User-Agent", "curl/7.64.1"}});
	// Выводим время запроса
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
