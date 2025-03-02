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
#include <server/awh.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * WebServer Класс объекта исполнителя
 */
class WebServer {
	private:
		// Объект хэширования
		hash_t _hash;
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	private:
		// Метод запроса клиента
		awh::web_t::method_t _method;
	private:
		// Буферы отправляемой полезной нагрузки
		map <uint64_t, queue <vector <char>>> _payloads;
	public:
		/**
		 * crash Метод обработки вызова крашей в приложении
		 * @param sig номер сигнала операционной системы
		 */
		void crash(const int sig) noexcept {
			// Если мы получили сигнал завершения работы
			if(sig == 2)
				// Выводим сообщение о заверщении работы
				this->_log->print("%s finishing work, goodbye!", log_t::flag_t::INFO, AWH_NAME);
			// Выводим сообщение об ошибке
			else this->_log->print("%s cannot be continued, signal: [%u]. Finishing work, goodbye!", log_t::flag_t::CRITICAL, AWH_NAME, sig);
			// Выполняем отключение вывода лога
			const_cast <awh::log_t *> (this->_log)->level(awh::log_t::level_t::NONE);
			// Завершаем работу приложения
			::exit(sig);
		}
	public:
		/**
		 * password Метод извлечения пароля (для авторизации методом Digest)
		 * @param bid   идентификатор брокера (клиента)
		 * @param login логин пользователя
		 * @return      пароль пользователя хранящийся в базе данных
		 */
		string password(const uint64_t bid, const string & login){
			// Выводим информацию в лог
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), "password", bid);
			// Выводим пароль
			return "password";
		}
		/**
		 * auth Метод проверки авторизации пользователя (для авторизации методом Basic)
		 * @param bid      идентификатор брокера (клиента)
		 * @param login    логин пользователя (от клиента)
		 * @param password пароль пользователя (от клиента)
		 * @return         результат авторизации
		 */
		bool auth(const uint64_t bid, const string & login, const string & password){
			// Выводим информацию в лог
			this->_log->print("USER: %s, PASS: %s, ID: %zu", log_t::flag_t::INFO, login.c_str(), password.c_str(), bid);
			// Разрешаем авторизацию
			return true;
		}
	public:
		/**
		 * accept Метод активации клиента на сервере
		 * @param ip   адрес интернет подключения
		 * @param mac  аппаратный адрес подключения
		 * @param port порт подключения
		 * @return     результат проверки
		 */
		bool accept(const string & ip, const string & mac, const uint32_t port){
			// Выводим информацию в лог
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * available Метод получения событий освобождения памяти буфера полезной нагрузки
		 * @param bid  идентификатор брокера
		 * @param size размер буфера полезной нагрузки
		 * @param core объект сетевого ядра
		 */
		void available(const uint64_t bid, const size_t size, server::core_t * core){
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
		 * active Метод идентификации активности на Web сервере
		 * @param bid  идентификатор брокера (клиента)
		 * @param mode режим события подключения
		 * @param core объект сетевого ядра
		 */
		void active(const uint64_t bid, const server::web_t::mode_t mode, server::core_t * core){
			// Определяем тип события
			switch(static_cast <uint8_t> (mode)){
				// Если произведено подключение клиента к серверу
				case static_cast <uint8_t> (server::web_t::mode_t::CONNECT): {
					// Выполняем установку ограничения пропускной способности сети
					core->bandwidth(bid, "1Mbps", "1Mbps");
					// Выводим информацию в лог
					this->_log->print("%s client", log_t::flag_t::INFO, "Connect");
				} break;
				// Если произведено отключение клиента от сервера
				case static_cast <uint8_t> (server::web_t::mode_t::DISCONNECT):
					// Выводим информацию в лог
					this->_log->print("%s client", log_t::flag_t::INFO, "Disconnect");
				break;
			}
		}
		/**
		 * error Метод вывода ошибок Websocket сервера
		 * @param bid  идентификатор брокера (клиента)
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 */
		void error(const uint64_t bid, const uint32_t code, const string & mess){
			// Выводим информацию в лог
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		/**
		 * message Метод получения сообщений
		 * @param bid    идентификатор брокера (клиента)
		 * @param buffer бинарный буфер сообщения
		 * @param text   тип буфера сообщения
		 * @param awh    объект веб-сервера
		 */
		void message(const uint64_t bid, const vector <char> & buffer, const bool text, server::awh_t * awh){
			// Если даныне получены
			if(!buffer.empty()){
				// Выбранный сабпротокол
				string subprotocol = "";
				// Получаем список выбранных сабпротоколов
				const auto subprotocols = awh->subprotocols(bid);
				// Если список выбранных сабпротоколов получен
				if(!subprotocols.empty())
					// Выполняем получение выбранного сабпротокола
					subprotocol = (* subprotocols.begin());
				// Выводим информацию в лог
				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), subprotocol.c_str());
				// Отправляем сообщение обратно
				awh->sendMessage(bid, buffer, text);
			}
		}
		/**
		 * handshake Метод получения удачного запроса
		 * @param sid   идентификатор потока
		 * @param bid   идентификатор брокера
		 * @param agent идентификатор агента клиента
		 * @param awh   объект веб-сервера
		 */
		void handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent, server::awh_t * awh){
			// Если метод запроса соответствует GET-запросу и агент является HTTP-клиентом
			if((this->_method == awh::web_t::method_t::GET) && (agent == server::web_t::agent_t::HTTP)){
				// Деактивируем шифрование
				awh->encrypt(sid, bid, false);
				// Извлекаем адрес URL-запроса
				cout << " URL: " << awh->parser(sid, bid)->request().url << endl;
				// Формируем тело ответа
				const string body = "<html>\n<head>\n<title>Hello World!</title>\n</head>\n<body>\n"
				"<h1>\"Hello, World!\" program</h1>\n"
				"<div>\nFrom Wikipedia, the free encyclopedia<br>\n"
				"(Redirected from Hello, world!)<br>\n"
				"Jump to navigationJump to search<br>\n"
				"<strong>\"Hello World\"</strong> redirects here. For other uses, see Hello World (disambiguation).<br>\n"
				"A <strong>\"Hello, World!\"</strong> program generally is a computer program that outputs or displays the message \"Hello, World!\".<br>\n"
				"Such a program is very simple in most programming languages, and is often used to illustrate the basic syntax of a programming language. It is often the first program written by people learning to code. It can also be used as a sanity test to make sure that computer software intended to compile or run source code is correctly installed, and that the operator understands how to use it.\n"
				"</div>\n</body>\n</html>\n";
				/*
				// Если протокол подключения принадлежит к HTTP/2
				if(awh->proto(bid) == engine_t::proto_t::HTTP2){
					// Выполняем отправку заголовковй временного овтета
					vector <pair <string, string>> headers = {
						{":method", "GET"},
						{":scheme", "https"},
						{":path", "/stylesheets/screen.css"},
						{":authority", "nghttp2.org"},
						{"accept-encoding", "gzip, deflate"},
						{"user-agent", "nghttp2/1.0.1-DEV"}
					};
					// Выполняем отправку push-уведомлений
					if(awh->push2(sid, bid, headers, awh::http2_t::flag_t::NONE) < 0)
						// Если запрос не был отправлен выводим сообщение об ошибке
						this->_log->print("Push message is not send", log_t::flag_t::WARNING);
				}
				*/
				// Если клиент запросил передачу трейлеров
				if(awh->trailers(sid, bid)){
					// Устанавливаем тестовые трейлеры
					awh->trailer(sid, bid, "Goga", "Hello");
					awh->trailer(sid, bid, "Hello", "World");
					awh->trailer(sid, bid, "Anyks", "Best of the best");
					awh->trailer(sid, bid, "Checksum", this->_hash.hashing <string> (body, hash_t::type_t::MD5));
				}
				// Отправляем сообщение клиенту
				awh->send(sid, bid, 200, "OK", vector <char> (body.begin(), body.end()));
			}
		}
		/**
		 * request Метод запроса клиента
		 * @param sid    идентификатор потока
		 * @param bid    идентификатор брокера
		 * @param method метод запроса
		 * @param url    url-адрес запроса
		 * @param awh    объект веб-сервера
		 */
		void request(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, server::awh_t * awh){
			// Запоминаем метод запроса
			this->_method = method;
			// Если пришёл запрос на фавиконку
			if(!url.empty() && (!url.path.empty() && url.path.back().compare("favicon.ico") == 0))
				// Выполняем реджект
				awh->send(sid, bid, 404);
		}
		/**
		 * complete Метод завершения получения запроса клиента
		 * @param sid     идентификатор потока
		 * @param bid     идентификатор брокера
		 * @param method  метод запроса
		 * @param url     url-адрес запроса
		 * @param entity  тело запроса
		 * @param headers заголовки запроса
		 * @param awh     объект веб-сервера
		 */
		void complete(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers, server::awh_t * awh){
			// Переходим по всем заголовкам
			for(auto & header : headers)
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());

			// Если данные запроса получены
			if(!entity.empty()){
				// Выводим информацию о входящих данных
				cout << " ================ " << url << " == " << string(entity.begin(), entity.end()) << endl;
				// Отправляем сообщение клиенту
				awh->send(sid, bid, 200, "OK", entity, {{"Connection", "close"}});
			}
		}
	public:
		/**
		 * WebServer Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		WebServer(const fmk_t * fmk, const log_t * log) : _hash(log), _fmk(fmk), _log(log), _method(awh::web_t::method_t::NONE) {}
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
	// Создаём объект сетевого ядра
	server::core_t core(&fmk, &log);
	// Создаём объект AWH-сервера
	server::awh_t awh(&core, &fmk, &log);
	// Создаём объект исполнителя
	WebServer executor(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("WEB Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем флаг перехвата контекста декомпрессии
	 * 2. Устанавливаем флаг перехвата контекста компрессии
	 * 3. Устанавливаем разрешение использовать протокол Websocket
	 * 4. Устанавливаем флаг разрешения метода CONNECT
	 */
	awh.mode({
		// server::web_t::flag_t::NOT_STOP,
		// server::web_t::flag_t::NOT_INFO,
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER,
		server::web_t::flag_t::WEBSOCKET_ENABLE,
		server::web_t::flag_t::CONNECT_METHOD_ENABLE
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Отключаем валидацию сертификата
	ssl.verify = false;
	// Устанавливаем адрес сертификата
	ssl.ca = "./certs/ca.pem";
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "./certs/certificates/server-key.pem";
	ssl.cert = "./certs/certificates/server-cert.pem";
	/*
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "/usr/local/etc/letsencrypt/live/anyks.net/privkey.pem";
	ssl.cert = "/usr/local/etc/letsencrypt/live/anyks.net/fullchain.pem";
	*/
	// Выполняем установку параметров SSL-шифрования
	core.ssl(ssl);
	// Устанавливаем активный протокол подключения
	core.proto(awh::engine_t::proto_t::HTTP2);
	// core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	// core.sonet(awh::scheme_t::sonet_t::DTLS);
	core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	// core.sonet(awh::scheme_t::sonet_t::TCP);
	// Разрешаем выполняем автоматический перезапуск упавшего процесса
	// core.clusterAutoRestart(true);
	// Активируем максимальное количество рабочих процессов
	core.cluster(awh::scheme_t::mode_t::ENABLED);
	// Устанавливаем режим мультипоточной обработки
	// core.multiThreads(22);
	// Устанавливаем название сервера
	// awh.realm("ANYKS");
	// Устанавливаем временный ключ сессии
	// awh.opaque("keySession");
	// Устанавливаем тип авторизации
	// awh.authType(auth_t::type_t::BASIC);
	awh.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию Web-сервера
	awh.init(2222, "127.0.0.1", {
		awh::http_t::compressor_t::ZSTD,
		awh::http_t::compressor_t::BROTLI,
		awh::http_t::compressor_t::GZIP,
		awh::http_t::compressor_t::DEFLATE
	});
	/*
	awh.init(2222, "anyks.net", {
		awh::http_t::compressor_t::ZSTD,
		awh::http_t::compressor_t::BROTLI,
		awh::http_t::compressor_t::GZIP,
		awh::http_t::compressor_t::DEFLATE
	});
	*/
	/*
	awh.init("anyks", {
		awh::http_t::compressor_t::ZSTD,
		awh::http_t::compressor_t::BROTLI,
		awh::http_t::compressor_t::GZIP,
		awh::http_t::compressor_t::DEFLATE
	});
	*/
	// Устанавливаем длительное подключение
	// awh.keepAlive(100, 30, 10);
	// Активируем шифрование
	// awh.encryption(true);
	// Устанавливаем пароль шифрования
	// awh.encryption(string{"PASS"});
	// Устанавливаем разрешённый источник
	awh.addOrigin("anyks.net");
	// Устанавливаем альтернативный сервис
	awh.addAltSvc("anyks.net", "h2=\":2222\"");
	awh.addAltSvc("example.com", "h2=\":8000\"");
	// Устанавливаем сабпротоколы
	awh.subprotocols({"test1", "test2", "test3"});
	// Разрешаем перехват сигналов
	core.signalInterception(awh::scheme_t::mode_t::ENABLED);
	// Устанавливаем функцию обработки сигналов завершения работы приложения
	core.callback <void (const int)> ("crash", std::bind(&WebServer::crash, &executor, _1));
	// Подписываемся на получении события освобождения памяти протокола сетевого ядра
	core.callback <void (const uint64_t, const size_t)> ("available", std::bind(&WebServer::available, &executor, _1, _2, &core));
	// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
	core.callback <void (const uint64_t, const char *, const size_t)> ("unavailable", std::bind(&WebServer::unavailable, &executor, _1, _2, _3));
	// Устанавливаем функцию извлечения пароля пользователя для авторизации
	awh.callback <string (const uint64_t, const string &)> ("extractPassword", std::bind(&WebServer::password, &executor, _1, _2));
	// Устанавливаем функцию проверки авторизации прользователя
	awh.callback <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&WebServer::auth, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	awh.callback <bool (const string &, const string &, const uint32_t)> ("accept", std::bind(&WebServer::accept, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	awh.callback <void (const uint64_t, const server::web_t::mode_t)> ("active", std::bind(&WebServer::active, &executor, _1, _2, &core));
	// Установливаем функцию обратного вызова на событие получения ошибок
	awh.callback <void (const uint64_t, const uint32_t, const string &)> ("errorWebsocket", std::bind(&WebServer::error, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие получения сообщений
	awh.callback <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", std::bind(&WebServer::message, &executor, _1, _2, _3, &awh));
	// Устанавливаем функцию обратного вызова при выполнении удачного рукопожатия
	awh.callback <void (const int32_t, const uint64_t, const server::web_t::agent_t)> ("handshake", std::bind(&WebServer::handshake, &executor, _1, _2, _3, &awh));
	// Установливаем функцию обратного вызова на событие получения запроса
	awh.callback <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", std::bind(&WebServer::request, &executor, _1, _2, _3, _4, &awh));
	// Установливаем функцию обратного вызова на событие получения полного запроса клиента
	awh.callback <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", std::bind(&WebServer::complete, &executor, _1, _2, _3, _4, _5, _6, &awh));
	// Выполняем запуск WEB-сервер
	awh.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
