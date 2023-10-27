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

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * WebServer Класс объекта исполнителя
 */
class WebServer {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	private:
		// Объект веб-сервера
		server::awh_t * _awh;
	private:
		// Метод запроса клиента
		awh::web_t::method_t _method;
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
		bool accept(const string & ip, const string & mac, const u_int port){
			// Выводим информацию в лог
			this->_log->print("ACCEPT: IP=%s, MAC=%s, PORT=%d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод идентификации активности на Web сервере
		 * @param bid  идентификатор брокера (клиента)
		 * @param mode режим события подключения
		 */
		void active(const uint64_t bid, const server::web_t::mode_t mode){
			// Если подключение клиента установлено
			if(mode == server::web_t::mode_t::CONNECT)
				// Аактивируем шифрование
				this->_awh->encrypt(bid, true);
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * error Метод вывода ошибок WebSocket сервера
		 * @param bid  идентификатор брокера (клиента)
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 */
		void error(const uint64_t bid, const u_int code, const string & mess){
			// Выводим информацию в лог
			this->_log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		/**
		 * message Метод получения сообщений
		 * @param bid    идентификатор брокера (клиента)
		 * @param buffer бинарный буфер сообщения
		 * @param text   тип буфера сообщения
		 */
		void message(const uint64_t bid, const vector <char> & buffer, const bool text){
			// Если даныне получены
			if(!buffer.empty()){
				// Выбранный сабпротокол
				string subprotocol = "";
				// Получаем список выбранных сабпротоколов
				const auto subprotocols = this->_awh->subprotocols(bid);
				// Если список выбранных сабпротоколов получен
				if(!subprotocols.empty())
					// Выполняем получение выбранного сабпротокола
					subprotocol = (* subprotocols.begin());
				// Выводим информацию в лог
				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), subprotocol.c_str());
				// Отправляем сообщение обратно
				this->_awh->sendMessage(bid, buffer, text);
			}
		}
		/**
		 * handshake Метод получения удачного запроса
		 * @param sid   идентификатор потока
		 * @param bid   идентификатор брокера
		 * @param agent идентификатор агента клиента
		 */
		void handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent){
			// Если метод запроса соответствует GET-запросу и агент является HTTP-клиентом
			if((this->_method == awh::web_t::method_t::GET) && (agent == server::web_t::agent_t::HTTP)){
				// Деактивируем шифрование
				this->_awh->encrypt(bid, false);
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
				// Если протокол подключения принадлежит к HTTP/2
				if(this->_awh->proto(bid) == engine_t::proto_t::HTTP2){
					/*
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
					if(this->_awh->push2(sid, bid, headers, awh::http2_t::flag_t::NONE) < 0)
						// Если запрос не был отправлен выводим сообщение об ошибке
						this->_log->print("Push message is not send", log_t::flag_t::WARNING);
					*/
					
					// Выполняем установку нового размера окна
					// this->_awh->windowUpdate2(sid, bid, 4964); // ++++++++++ НЕ РАБОТАЕТ
					// Выполняем отправку алтернативных протоколов
					// this->_awh->altsvc2(sid, bid, "example.com", "h2=\":8000\""); // ++++++++++ НЕ РАБОТАЕТ
					// Выполняем отправку списка доступных ресурсов которым разрешено подключаться к серверу
					// this->_awh->sendOrigin(bid, {"https://anyks.net", "https://nghttp2.org"}); // +++++++++++++ РАБОТАЕТ (научиться извлекать данные)

					// Устанавливаем список доступных ресурсов
					this->_awh->sendOrigin({"https://anyks.net:2222"});
				}
				// Если клиент запросил передачу трейлеров
				if(this->_awh->trailers(bid)){
					// Устанавливаем тестовые трейлеры
					this->_awh->trailer(bid, "Goga", "Hello");
					this->_awh->trailer(bid, "Hello", "World");
					this->_awh->trailer(bid, "Anyks", "Best of the best");
					this->_awh->trailer(bid, "Checksum", this->_fmk->hash(body, fmk_t::hash_t::MD5));
				}
				// Отправляем сообщение клиенту
				this->_awh->send(bid, 200, "OK", vector <char> (body.begin(), body.end()));
			}
		}
		/**
		 * request Метод запроса клиента
		 * @param sid    идентификатор потока
		 * @param bid    идентификатор брокера
		 * @param method метод запроса
		 * @param url    url-адрес запроса
		 */
		void request(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url){
			// Запоминаем метод запроса
			this->_method = method;
			// Если пришёл запрос на фавиконку
			if(!url.empty() && (!url.path.empty() && url.path.back().compare("favicon.ico") == 0))
				// Выполняем реджект
				this->_awh->send(bid, 404);
		}
		/**
		 * headers Метод получения заголовков запроса
		 * @param sid     идентификатор потока
		 * @param bid     идентификатор брокера
		 * @param method  метод запроса
		 * @param url     url-адрес запроса
		 * @param headers заголовки запроса
		 */
		void headers(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			// Переходим по всем заголовкам
			for(auto & header : headers)
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
		}
		/**
		 * entity Метод получения тела запроса
		 * @param sid    идентификатор потока
		 * @param bid    идентификатор брокера
		 * @param method метод запроса
		 * @param url    url-адрес запроса
		 * @param entity тело запроса
		 */
		void entity(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity){
			// Выводим информацию о входящих данных
			cout << " ================ " << url << " == " << string(entity.begin(), entity.end()) << endl;
			// Отправляем сообщение клиенту
			this->_awh->send(bid, 200, "OK", entity, {{"Connection", "close"}});
		}
	public:
		/**
		 * WebServer Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 * @param awh объект WEB-сервера
		 */
		WebServer(const fmk_t * fmk, const log_t * log, server::awh_t * awh) : _fmk(fmk), _log(log), _awh(awh), _method(awh::web_t::method_t::NONE) {}
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
	// Создаём биндинг
	server::core_t core(&fmk, &log);
	// Создаём объект WEB-клиента
	server::awh_t awh(&core, &fmk, &log);
	// Создаём объект исполнителя
	WebServer executor(&fmk, &log, &awh);
	// Устанавливаем название сервиса
	log.name("WEB Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем валидацию SSL сертификата
	 * 2. Устанавливаем флаг перехвата контекста декомпрессии
	 * 3. Устанавливаем флаг перехвата контекста компрессии
	 * 4. Устанавливаем разрешение использовать протокол WebSocket
	 */
	awh.mode({
		// server::web_t::flag_t::NOT_STOP,
		// server::web_t::flag_t::NOT_INFO,
		// server::web_t::flag_t::WAIT_MESS,
		server::web_t::flag_t::VERIFY_SSL,
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER,
		server::web_t::flag_t::WEBSOCKET_ENABLE
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	// core.ca("./ca/cert.pem");
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
	// Отключаем валидацию сертификата
	// core.verifySSL(false);
	// Активируем максимальное количество рабочих процессов
	core.clusterSize();
	// Разрешаем выполняем автоматический перезапуск упавшего процесса
	awh.clusterAutoRestart(true);
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
	/*
	awh.init(2222, "127.0.0.1", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});
	*/
	awh.init(2222, "", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});
	/*
	awh.init(2222, "127.0.0.1", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});
	awh.init("anyks", {
		awh::http_t::compress_t::BROTLI,
		awh::http_t::compress_t::GZIP,
		awh::http_t::compress_t::DEFLATE,
	});
	*/
	// Устанавливаем длительное подключение
	// awh.keepAlive(100, 30, 10);
	// Устанавливаем SSL сертификаты сервера
	core.certificate(
		"/usr/local/etc/letsencrypt/live/anyks.net/fullchain.pem",
		"/usr/local/etc/letsencrypt/live/anyks.net/privkey.pem"
	);
	// core.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");
	// Активируем шифрование
	awh.encryption(true);
	// Устанавливаем пароль шифрования
	awh.encryption(string{"PASS"});
	// Устанавливаем сабпротоколы
	awh.subprotocols({"test1", "test2", "test3"});
	// Разрешаем метод CONNECT для сервера
	awh.settings({{server::web2_t::settings_t::CONNECT, 1}});
	// Устанавливаем функцию извлечения пароля
	awh.on((function <string (const uint64_t, const string &)>) std::bind(&WebServer::password, &executor, _1, _2));
	// Устанавливаем функцию проверки авторизации
	// awh.on((function <bool (const uint64_t, const string &, const string &)>) std::bind(&WebServer::auth, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	awh.on((function <void (const uint64_t, const server::web_t::mode_t)>) std::bind(&WebServer::active, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие получения ошибок
	awh.on((function <void (const uint64_t, const u_int, const string &)>) std::bind(&WebServer::error, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	awh.on((function <bool (const string &, const string &, const u_int)>) std::bind(&WebServer::accept, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие получения сообщений
	awh.on((function <void (const uint64_t, const vector <char> &, const bool)>) bind(&WebServer::message, &executor, _1, _2, _3));
	// Устанавливаем функцию обратного вызова при выполнении удачного рукопожатия
	awh.on((function <void (const int32_t, const uint64_t, const server::web_t::agent_t)>) std::bind(&WebServer::handshake, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие получения запроса
	awh.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)>) std::bind(&WebServer::request, &executor, _1, _2, _3, _4));
	// Установливаем функцию обратного вызова на событие получения тела сообщения
	awh.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)>) std::bind(&WebServer::entity, &executor, _1, _2, _3, _4, _5));
	// Установливаем функцию обратного вызова на событие получения заголовки сообщения
	awh.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)>) std::bind(&WebServer::headers, &executor, _1, _2, _3, _4, _5));
	// Выполняем запуск WEB-сервер
	awh.start();
	// Выводим результат
	return 0;
}
