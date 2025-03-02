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
#include <client/awh.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * WebClient Класс объекта исполнителя
 */
class WebClient {
	private:
		// Количество выполненных запросов
		uint8_t _count;
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	public:
		/**
		 * response Метод получения статуса результата запроса
		 * @param sid     идентификатор потока
		 * @param rid     идентификатор запроса
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 */
		void response(const int32_t sid, [[maybe_unused]] const uint64_t rid, const uint32_t code, const string & message){
			// Проверяем на наличие ошибок
			if(code >= 300)
				// Выводим сообщение о неудачном запросе
				this->_log->print("Request failed: %u %s stream=%i", log_t::flag_t::WARNING, code, message.c_str(), sid);
		}
		/**
		 * active Метод идентификации активности на Web-клиенте
		 * @param mode режим события подключения
		 * @param awh  объект web-клиента
		 */
		void active(const client::web_t::mode_t mode, client::awh_t * awh){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == client::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
			// Если подключение выполнено
			if(mode == client::web_t::mode_t::CONNECT){
				// Создаём объект URI
				uri_t uri(this->_fmk, this->_log);
				// Создаём объект запроса
				client::web_t::request_t req1, req2;
				// Устанавливаем метод запроса
				req1.method = web_t::method_t::GET;
				// Устанавливаем метод запроса
				req2.method = web_t::method_t::GET;
				// Устанавливаем параметры запроса
				req1.url = uri.parse("/api/v3/exchangeInfo");
				// Устанавливаем параметры запроса
				req2.url = uri.parse("/api/v3/exchangeInfo?symbol=BTCUSDT");
				// Выполняем первый запрос на сервер
				awh->send(std::move(req1));
				// Выполняем второй запрос на сервер
				awh->send(std::move(req2));
			}
		}
		/**
		 * entity Метод получения тела ответа сервера
		 * @param sid     идентификатор потока
		 * @param rid     идентификатор запроса
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param entity  тело ответа сервера
		 * @param awh     объект web-клиента
		 */
		void entity([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, [[maybe_unused]] const uint32_t code, [[maybe_unused]] const string & message, const vector <char> & entity, client::awh_t * awh){
			// Увеличиваем количество выполненных запросов
			this->_count++;
			// Выводим полученный результат
			cout << " =========== " << string(entity.begin(), entity.end()) << endl;
			// Если оба запроса выполнены
			if(this->_count == 2)
				// Выполняем остановку
				awh->stop();
		}
		/**
		 * headers Метод получения заголовков ответа сервера
		 * @param sid     идентификатор потока
		 * @param rid     идентификатор запроса
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param headers заголовки ответа сервера
		 */
		void headers([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, [[maybe_unused]] const uint32_t code, [[maybe_unused]] const string & message, const unordered_multimap <string, string> & headers){
			// Переходим по всем заголовкам
			for(auto & header : headers)
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
		}
		/**
		 * complete Метод получения ответа с сервера
		 * @param sid     идентификатор потока
		 * @param rid     идентификатор запроса
		 * @param code    код ответа сервера
		 * @param message сообщение ответа сервера
		 * @param body    данные полученного тела сообщения
		 * @param data    данные полученных заголовков сообщения
		 * @param awh     объект web-клиента
		 */
		void complete([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, [[maybe_unused]] const uint32_t code, [[maybe_unused]] const string & message, const vector <char> & entity, const unordered_multimap <string, string> & headers, client::awh_t * awh){
			// Увеличиваем количество выполненных запросов
			this->_count++;
			// Переходим по всем заголовкам
			for(auto & header : headers)
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
			// Выводим полученный результат
			cout << " =========== " << string(entity.begin(), entity.end()) << endl;
			// Если оба запроса выполнены
			if(this->_count == 2)
				// Выполняем остановку
				awh->stop();
		}
	public:
		/**
		 * WebClient Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		WebClient(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk{};
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект работы с адресами-URI
	uri_t uri(&fmk, &log);
	// Создаём объект сетевого ядра
	client::core_t core(&fmk, &log);
	// Создаём объект AWH-клиента
	client::awh_t awh(&core, &fmk, &log);
	// Создаём объект исполнителя
	WebClient executor(&fmk, &log);
	// Активируем протокол интернета IPv6
	// core.family(scheme_t::family_t::IPV6);
	// Устанавливаем активный протокол подключения
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Устанавливаем название сервиса
	log.name("WEB Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем поддержку редиректов
	 * 2. Устанавливаем флаг разрешения метода CONNECT
	 */
	awh.mode({
		// client::web_t::flag_t::ALIVE,
		// client::web_t::flag_t::NOT_INFO,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Создаём объект параметров SSL-шифрования
	node_t::ssl_t ssl;
	// Отключаем валидацию сертификата
	ssl.verify = true;
	// Устанавливаем адрес сертификата
	ssl.ca = "./certs/ca.pem";
	/*
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "./certs/certificates/client-key.pem";
	ssl.cert = "./certs/certificates/client-cert.pem";
	*/
	// Выполняем установку параметров SSL-шифрования
	core.ssl(ssl);
	// Активируем шифрование
	// awh.encryption(true);
	// Устанавливаем пароль шифрования
	// awh.encryption(string{"PASS"});
	// Устанавливаем логин и пароль пользователя
	// awh.user("user", "password");
	// Активируем получение PUSH уведомлений
	awh.settings({
		{awh::http2_t::settings_t::ENABLE_PUSH, 1},
		{awh::http2_t::settings_t::ENABLE_ALTSVC, 1},
		{awh::http2_t::settings_t::ENABLE_ORIGIN, 1}
	});
	// Устанавливаем длительное подключение
	// awh.keepAlive(2, 3, 1);
	// Устанавливаем длительное подключение
	// awh.keepAlive(100, 30, 10);
	// Отключаем таймер ожидания входящих данных
	// awh.waitTimeDetect(0, 0, CONNECT_TIMEOUT);
	// Устанавливаем данные прокси-сервера
	// awh.proxy("http://qKseEr:t5QrcW@212.102.146.33:8000");
	// awh.proxy("socks5://3JMFxD:CWv6MP@45.130.126.236:8000");
	// awh.proxy("socks5://127.0.0.1:2222");
	// awh.proxy("socks5://user:password@127.0.0.1:2222");
	// awh.proxy("http://user:password@127.0.0.1:2222");
	// awh.proxy("http://127.0.0.1:2222");
	// awh.proxy("socks5://user:password@anyks", awh::scheme_t::family_t::NIX);
	// awh.proxy("http://anyks", awh::scheme_t::family_t::NIX);
	// awh.proxy("http://user:password@anyks", awh::scheme_t::family_t::NIX);
	// awh.proxy("http://3pvhoe:U8QFWd@193.56.188.250:8000");
	// awh.proxy("http://tARdXT:uWoRp1@217.29.62.214:13699");
	// awh.proxy("socks5://2faD0Q:mm9mw4@193.56.188.192:8000");
	// awh.proxy("socks5://kLV5jZ:ypKUKp@217.29.62.214:13700");

	// awh.proxy("http://user:password@anyks.net:2222");
	// awh.proxy("https://user:password@anyks.net:2222");
	// awh.proxy("socks5://user:password@anyks.net:2222");
	// Активируем работу прокси-сервера
	// awh.proxy(client::scheme_t::work_t::ALLOW);
	/*
	// Устанавливаем тип компрессии
	awh.compressors({
		http_t::compressor_t::ZSTD,
		http_t::compressor_t::BROTLI,
		http_t::compressor_t::GZIP,
		http_t::compressor_t::DEFLATE
	});
	*/
	// Устанавливаем тип авторизации прокси-сервера
	// awh.authTypeProxy(auth_t::type_t::BASIC);
	// awh.authTypeProxy(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию типа авторизации
	// awh.authType(auth_t::type_t::BASIC);
	// awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	/**
	 * Вариант с мультизапросами
	 */
	/*
	// Устанавливаем метод активации подключения
	awh.callback <void (const client::web_t::mode_t)> ("active", std::bind(&WebClient::active, &executor, _1, &awh));
	// Устанавливаем метод получения сообщения сервера
	awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", std::bind(&WebClient::response, &executor, _1, _2, _3, _4));
	// Устанавливаем метод получения тела ответа
	// awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entity", std::bind(&WebClient::entity, &executor, _1, _2, _3, _4, _5, &awh));
	// Устанавливаем метод получения заголовков
	// awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", std::bind(&WebClient::headers, &executor, _1, _2, _3, _4, _5));
	// Устанавливаем метод получения ответа с сервера
	awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", std::bind(&WebClient::complete, &executor, _1, _2, _3, _4, _5, _6, &awh));
	// Выполняем инициализацию подключения
	awh.init("https://api.binance.com");
	// Выполняем запуск работы
	awh.start();
	*/
	/**
	 * Вариант с монозапросом
	 */
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
	// uri_t::url_t url = uri.parse("https://anyks.net:2222");
	// uri_t::url_t url = uri.parse("https://127.0.0.1:2222");
	// uri_t::url_t url = uri.parse("https://anyks.com/test.php");
	// uri_t::url_t url = uri.parse("https://www.anyks.com/test.php");
	uri_t::url_t url = uri.parse("https://apple.com/ru/mac");
	// uri_t::url_t url = uri.parse("https://support.apple.com/ru-ru/mac");
	// uri_t::url_t url = uri.parse("https://ru.wikipedia.org/wiki/HTTP");
	// uri_t::url_t url = uri.parse("https://api.binance.com/api/v3/exchangeInfo?symbol=BTCUSDT");
	// uri_t::url_t url = uri.parse("https://testnet.binance.vision/api/v3/exchangeInfo");
	// uri_t::url_t url = uri.parse("https://api.coingecko.com/api/v3/coins/list?include_platform=true");
	// uri_t::url_t url = uri.parse("https://api.coingecko.com/api/v3/simple/price?ids=tron&vs_currencies=usd");
	// Замеряем время начала работы
	auto timeShifting = chrono::system_clock::now();
	// Формируем GET запрос
	// const auto & body = awh.GET(url);
	// const auto & body = awh.GET(url, {{"Connection", "close"}});
	const auto & body = awh.GET(url, {{"User-Agent", "curl/7.64.1"},{"te", "trailers, gzip;q=0.5"}});
	// Подготавливаем тело запроса
	// const string entity = "<html><head><title>404</title></head><body><h1>Hello World!!!</h1></body></html>";
	// Выполняем тело запроса на сервер
	// const auto & body = awh.POST(url, vector <char> (entity.begin(), entity.end()), {{"User-Agent", "curl/7.64.1"}});
	// Выводим время запроса
	cout << " ++++++++++ Time Shifting " << chrono::duration_cast <chrono::milliseconds> (chrono::system_clock::now() - timeShifting).count() << endl;
	// Если данные получены
	if(!body.empty())
		// Выводим полученный результат
		cout << " =========== " << string(body.begin(), body.end()) << endl;
	// Выводим результат
	return EXIT_SUCCESS;
}
