/**
 * @file: wsClient.cpp
 * @date: 2025-03-02
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <client/ws.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Executor Класс объекта исполнителя
 */
class Executor {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	private:
		// Буферы отправляемой полезной нагрузки
		map <uint64_t, queue <vector <char>>> _payloads;
	public:
		/**
		 * status Метод статуса запуска/остановки сервера
		 * @param status статус события сервера
		 */
		void status(const awh::core_t::status_t status){
			// Определяем флаг статуса сервера
			switch(static_cast <uint8_t> (status)){
				// Если сервер запущен
				case static_cast <uint8_t> (awh::core_t::status_t::START):
					// Выводим информацию в лог
					this->_log->print("START", log_t::flag_t::INFO);
				break;
				// Если сервер остановлен
				case static_cast <uint8_t> (awh::core_t::status_t::STOP):
					// Выводим информацию в лог
					this->_log->print("STOP", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * available Метод получения событий освобождения памяти буфера полезной нагрузки
		 * @param bid  идентификатор брокера
		 * @param size размер буфера полезной нагрузки
		 * @param core объект сетевого ядра
		 */
		void available(const uint64_t bid, const size_t size, client::core_t * core){
			// Ещем для указанного потока очередь полезной нагрузки
			auto i = this->_payloads.find(bid);
			// Если для потока очередь полезной нагрузки получена
			if((i != this->_payloads.end()) && !i->second.empty()){
				// Если места достаточно в буфере данных для отправки
				if(i->second.front().size() <= size){
					// Выполняем отправку заголовков запроса на сервер
					if(core->send(i->second.front().data(), i->second.front().size(), bid))
						// Выполняем удаление буфера полезной нагрузки
						i->second.pop();
				}
			}
		}
		/**
		 * unavailable Метод получения событий недоступности памяти буфера полезной нагрузки
		 * @param bid    идентификатор брокера
		 * @param buffer буфер полезной нагрузки которую не получилось отправить
		 * @param size   размер буфера полезной нагрузки
		 */
		void unavailable(const uint64_t bid, const char * buffer, const size_t size){
			// Ещем для указанного потока очередь полезной нагрузки
			auto i = this->_payloads.find(bid);
			// Если для потока очередь полезной нагрузки получена
			if(i != this->_payloads.end())
				// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
				i->second.push(vector <char> (buffer, buffer + size));
			// Если для потока почередь полезной нагрузки ещё не сформированна
			else {
				// Создаём новую очередь полезной нагрузки
				auto ret = this->_payloads.emplace(bid, queue <vector <char>> ());
				// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
				ret.first->second.push(vector <char> (buffer, buffer + size));
			}
		}
		/**
		 * handshake Метод рукопожатия
		 * @param sid   идентификатор потока
		 * @param rid   идентификатор запроса
		 * @param agent идентификатор агента клиента
		 * @param ws    бъект websocket-клиента
		 */
		void handshake([[maybe_unused]] const int32_t sid, [[maybe_unused]] const uint64_t rid, const client::web_t::agent_t agent, client::websocket_t * ws){
			// Если агент соответствует Websocket
			if(agent == client::web_t::agent_t::WEBSOCKET){
				// Выводим информацию в лог
				this->_log->print("Handshake", log_t::flag_t::INFO);
				// Выполняем установку ограничения пропускной способности сети
				ws->bandwidth("1Mbps", "1Mbps");
				// Получаем параметры запроса в виде строки
				const string query = R"({"id":1,"method":"SUBSCRIBE","params":["omgusdt@aggTrade","umausdt@aggTrade","radusdt@aggTrade","fttusdt@aggTrade","xrpusdt@aggTrade","adausdt@aggTrade","solusdt@aggTrade","dotusdt@aggTrade","ethusdt@aggTrade","bnbusdt@aggTrade","btcusdt@aggTrade","ltcusdt@aggTrade","trxusdt@aggTrade","xlmusdt@aggTrade","zecusdt@aggTrade","zilusdt@aggTrade","vetusdt@aggTrade","wrxusdt@aggTrade","oneusdt@aggTrade","xtzusdt@aggTrade","filusdt@aggTrade","eosusdt@aggTrade","xmrusdt@aggTrade","neousdt@aggTrade","kdausdt@aggTrade","ksmusdt@aggTrade","icxusdt@aggTrade","dgbusdt@aggTrade","bchusdt@aggTrade","ontusdt@aggTrade","etcusdt@aggTrade","chzusdt@aggTrade","kncusdt@aggTrade","astusdt@aggTrade","atausdt@aggTrade","snxusdt@aggTrade","grtusdt@aggTrade","mkrusdt@aggTrade","dcrusdt@aggTrade","c98usdt@aggTrade","uniusdt@aggTrade","rvnusdt@aggTrade","zenusdt@aggTrade","lrcusdt@aggTrade","ftmusdt@aggTrade","xecusdt@aggTrade","xemusdt@aggTrade","btgusdt@aggTrade","srmusdt@aggTrade","ckbusdt@aggTrade","xvgusdt@aggTrade","lskusdt@aggTrade","ernusdt@aggTrade","stxusdt@aggTrade","bcdusdt@aggTrade","sysusdt@aggTrade","axsusdt@aggTrade","qntusdt@aggTrade","enjusdt@aggTrade","hotusdt@aggTrade","algousdt@aggTrade","arpausdt@aggTrade","compusdt@aggTrade","iostusdt@aggTrade","flowusdt@aggTrade","aaveusdt@aggTrade","runeusdt@aggTrade","celrusdt@aggTrade","linkusdt@aggTrade","qtumusdt@aggTrade","egldusdt@aggTrade","dashusdt@aggTrade","lunausdt@aggTrade","cakeusdt@aggTrade","atomusdt@aggTrade","minausdt@aggTrade","idexusdt@aggTrade","dogeusdt@aggTrade","avaxusdt@aggTrade","iotausdt@aggTrade","hbarusdt@aggTrade","nearusdt@aggTrade","klayusdt@aggTrade","maskusdt@aggTrade","iotxusdt@aggTrade","celousdt@aggTrade","waxpusdt@aggTrade","scrtusdt@aggTrade","manausdt@aggTrade","reefusdt@aggTrade","nanousdt@aggTrade","shibusdt@aggTrade","sandusdt@aggTrade","thetausdt@aggTrade","wavesusdt@aggTrade","audiousdt@aggTrade","maticusdt@aggTrade","sushiusdt@aggTrade","1inchusdt@aggTrade","oceanusdt@aggTrade"]})";
				// Отправляем сообщение на сервер
				ws->sendMessage(query.data(), query.size());
			}
		}
	public:
		/**
		 * subscribe Метод подписки на сообщения логов
		 * @param flag    флаг сообщения лога
		 * @param message текст сообщения лога
		 */
		void subscribe([[maybe_unused]] const log_t::flag_t flag, [[maybe_unused]] const string & message){
			// Выводим сообщение
			// cout << " ============= " << message << endl;
		}
	public:
		/**
		 * error Метод вывода ошибок Websocket клиента
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 */
		void error(const uint32_t code, const string & mess){
			// Выводим информацию в лог
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		/**
		 * message Метод получения сообщений
		 * @param buffer бинарный буфер сообщения
		 * @param utf8   тип буфера сообщения
		 * @param ws     объект websocket-клиента
		 */
		void message(const vector <char> & buffer, const bool utf8, client::websocket_t * ws){
			// Выбранный сабпротокол
			string subprotocol = "";
			// Получаем список выбранных сабпротоколов
			const auto subprotocols = ws->subprotocols();
			// Если список выбранных сабпротоколов получен
			if(!subprotocols.empty())
				// Выполняем получение выбранного сабпротокола
				subprotocol = (* subprotocols.begin());
			// Если данные пришли в виде текста, выводим
			if(utf8)
				// Выводим полученный результат
				cout << " +++++++++++++ " << string(buffer.begin(), buffer.end()) << " == " << subprotocol << endl;
			// Сообщаем количество полученных байт
			else cout << " +++++++++++++ " << buffer.size() << " bytes" << " == " << subprotocol << endl;
		}
	public:
		/**
		 * Executor Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		Executor(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект параметров SSL-шифрования
	node_t::ssl_t ssl;
	// Создаём биндинг сетевого ядра
	client::core_t core(&fmk, &log);
	// Создаём объект Websocket-клиента
	client::websocket_t ws(&core, &fmk, &log);
	// Создаём объект исполнителя
	Executor executor(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Websocket Client");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем постоянное подключение
	 * 2. Устанавливаем поддержку редиректов
	 * 3. Устанавливаем флаг перехвата контекста компрессии
	 * 4. Устанавливаем флаг перехвата контекста декомпрессии
	 * 5. Устанавливаем флаг разрешения метода CONNECT
	 */
	ws.mode({
		client::web_t::flag_t::ALIVE,
		// client::web_t::flag_t::NOT_STOP,
		// client::web_t::flag_t::NOT_INFO,
		// client::web_t::flag_t::NOT_PING,
		client::web_t::flag_t::REDIRECTS,
		client::web_t::flag_t::TAKEOVER_CLIENT,
		client::web_t::flag_t::TAKEOVER_SERVER,
		client::web_t::flag_t::CONNECT_METHOD_ENABLE
	});
	// Устанавливаем время ожидания ответа WebSocket-сервера
	// ws.waitPong(300);
	// Разрешаем простое чтение базы событий
	// core.frequency(0);
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Отключаем валидацию сертификата
	ssl.verify = false;
	// Устанавливаем адрес сертификата
	ssl.ca = "./certs/ca.pem";
	/*
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "./certs/certificates/client-key.pem";
	ssl.cert = "./certs/certificates/client-cert.pem";
	*/
	// Выполняем установку параметров SSL-шифрования
	core.ssl(ssl);
	// Устанавливаем активный протокол подключения
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета DTLS
	// core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	// core.sonet(awh::scheme_t::sonet_t::TCP);
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
	// Устанавливаем размер доступной кэш-памяти для одного подключения
	// core.brokerAvailableSize(0x1FFFFFFF);
	// Устанавливаем размер доступной кэш-памяти для всех подключений
	// core.memoryAvailableSize(0x7FFFFFFF);
	// Устанавливаем режим отправки сообщений
	// core.sending(node_t::sending_t::DEFFER);
	// Устанавливаем логин и пароль пользователя
	ws.user("user", "password");
	// Выполняем активацию многопоточности
	// ws.multiThreads(22);
	// Устанавливаем данные прокси-сервера
	// ws.proxy("http://qKseEr:t5QrcW@212.102.146.33:8000");
	// ws.proxy("socks5://3JMFxD:CWv6MP@45.130.126.236:8000");
	// ws.proxy("http://fn3nzc:GZJAeP@217.29.62.232:11283");
	// ws.proxy("socks5://xYkj89:eqCQJA@85.195.81.167:12387");
	// ws.proxy("socks5://test1:password@127.0.0.1:2222");
	// ws.proxy("http://127.0.0.1:2222");
	// ws.proxy("http://test1:password@127.0.0.1:2222");
	// ws.proxy("socks5://unix:anyks", awh::scheme_t::family_t::NIX);
	// ws.proxy("http://unix:anyks", awh::scheme_t::family_t::NIX);

	// ws.proxy("http://3pvhoe:U8QFWd@193.56.188.250:8000");
	// ws.proxy("http://tARdXT:uWoRp1@217.29.62.214:13699");

	// ws.proxy("socks5://2faD0Q:mm9mw4@193.56.188.192:8000");
	// ws.proxy("socks5://kLV5jZ:ypKUKp@217.29.62.214:13700");

	// ws.proxy("http://user:password@anyks.net:2222");
	// ws.proxy("https://user:password@anyks.net:2222");
	// ws.proxy("socks5://user:password@anyks.net:2222");

	// Активируем работу прокси-сервера
	// ws.proxy(client::scheme_t::work_t::ALLOW);

	// Устанавливаем тип авторизации прокси-сервера
	// ws.authTypeProxy(awh::auth_t::type_t::BASIC);
	// Устанавливаем тип авторизации прокси-сервера
	// ws.authTypeProxy(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);
	// Выполняем инициализацию типа авторизации
	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);
	// Выполняем инициализацию Websocket клиента
	// ws.init("wss://stream.binance.com:9443/stream");
	// ws.init("ws://127.0.0.1:2222", {awh::http_t::compressor_t::DEFLATE});
	ws.init("wss://127.0.0.1:2222", {awh::http_t::compressor_t::DEFLATE});
	// ws.init("wss://anyks.net:2222", {awh::http_t::compressor_t::DEFLATE});
	// ws.init("wss://92.63.110.56:2222", {awh::http_t::compressor_t::DEFLATE});
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	// ws.init("anyks", {http_t::compressor_t::DEFLATE});
	// Активируем шифрование
	// ws.encryption(true);
	// Устанавливаем пароль шифрования
	// ws.encryption(string{"PASS"});
	// Устанавливаем дополнительные заголовки
	// ws.setHeaders({{"hello", "world!!"}});
	// Устанавливаем таймер ожидания входящих сообщений
	// ws.waitMessage(5);
	// Устанавливаем сабпротоколы
	ws.subprotocols({"test2", "test8", "test9"});
	// Устанавливаем поддерживаемые расширения
	// ws.extensions({{"test1", "test2", "test3"},{"good1", "good2", "good3"}});
	// Выполняем подписку на получение логов
	// log.subscribe(std::bind(&Executor::subscribe, &executor, _1, _2));
	// Подписываемся на получении события освобождения памяти протокола сетевого ядра
	core.callback <void (const uint64_t, const size_t)> ("available", std::bind(&Executor::available, &executor, _1, _2, &core));
	// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
	core.callback <void (const uint64_t, const char *, const size_t)> ("unavailable", std::bind(&Executor::unavailable, &executor, _1, _2, _3));
	// Подписываемся на событие запуска/остановки сервера
	ws.callback <void (const awh::core_t::status_t)> ("status", std::bind(&Executor::status, &executor, _1));
	// Подписываемся на событие получения ошибки работы клиента
	ws.callback <void (const uint32_t, const string &)> ("errorWebsocket", std::bind(&Executor::error, &executor, _1, _2));
	// Подписываемся на событие получения сообщения с сервера
	ws.callback <void (const vector <char> &, const bool)> ("messageWebsocket", std::bind(&Executor::message, &executor, _1, _2, &ws));
	// Подписываемся на событие рукопожатия
	ws.callback <void (const int32_t, const uint64_t, const client::web_t::agent_t)> ("handshake", std::bind(&Executor::handshake, &executor, _1, _2, _3, &ws));
	// Выполняем запуск Websocket клиента
	ws.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
