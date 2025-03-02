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
#include <server/ws.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;
using namespace awh::server;

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
		 * active Метод событий сервера
		 * @param bid  идентификатор брокера (клиента)
		 * @param mode флаг события
		 * @param core объект сетевого ядра
		 */
		void active(const uint64_t bid, const server::web_t::mode_t mode, server::core_t * core){
			// Определяем флаг события сервера
			switch(static_cast <uint8_t> (mode)){
				// Если клиент подключился к серверу
				case static_cast <uint8_t> (server::web_t::mode_t::CONNECT): {
					// Выполняем установку ограничения пропускной способности сети
					core->bandwidth(bid, "1Mbps", "1Mbps");
					// Выводим информацию в лог
					this->_log->print("CONNECT", log_t::flag_t::INFO);
				} break;
				// Если клиент отключился от сервера
				case static_cast <uint8_t> (server::web_t::mode_t::DISCONNECT):
					// Выводим информацию в лог
					this->_log->print("DISCONNECT", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * error Метод вывода ошибок Websocket сервера
		 * @param bid  идентификатор брокера (клиента)
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 * @param ws   объект Websocket-сервера
		 */
		void error(const uint64_t bid, const uint32_t code, const string & mess, server::websocket_t * ws){
			// Выводим информацию в лог
			this->_log->print("%s [%u] IP=%s", log_t::flag_t::CRITICAL, mess.c_str(), code, ws->ip(bid).c_str());
		}
		/**
		 * message Метод получения сообщений
		 * @param bid    идентификатор брокера (клиента)
		 * @param buffer бинарный буфер сообщения
		 * @param text   тип буфера сообщения
		 * @param ws     объект Websocket-сервера
		 */
		void message(const uint64_t bid, const vector <char> & buffer, const bool text, server::websocket_t * ws){
			// Если даныне получены
			if(!buffer.empty()){
				// Выбранный сабпротокол
				string subprotocol = "";
				// Получаем список выбранных сабпротоколов
				const auto subprotocols = ws->subprotocols(bid);
				// Если список выбранных сабпротоколов получен
				if(!subprotocols.empty())
					// Выполняем получение выбранного сабпротокола
					subprotocol = (* subprotocols.begin());
				// Выводим информацию в лог
				this->_log->print("Message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), subprotocol.c_str());
				// Отправляем сообщение обратно
				ws->sendMessage(bid, buffer, text);
			}
		}
		/**
		 * request Метод вывода входящего запроса
		 * @param sid     идентификатор входящего потока
		 * @param bid     идентификатор брокера (клиента)
		 * @param method  метод входящего запроса
		 * @param url     адрес входящего запроса
		 * @param headers заголовки запроса
		 */
		void headers(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers){
			// Создаём объект URI
			uri_t uri(this->_fmk, this->_log);
			// Выводим информацию в лог
			this->_log->print("REQUEST ID=%zu URL=%s", log_t::flag_t::INFO, bid, uri.url(url).c_str());
			// Переходим по всем заголовкам
			for(auto & header : headers)
				// Выводим информацию в лог
				this->_log->print("%s : %s", log_t::flag_t::INFO, header.first.c_str(), header.second.c_str());
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
	// Создаём объект сетевого ядра
	server::core_t core(&fmk, &log);
	// Создаём объект WebSocket-сервера
	websocket_t ws(&core, &fmk, &log);
	// Создаём объект исполнителя
	Executor executor(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Websocket Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем флаг перехвата контекста декомпрессии
	 * 2. Устанавливаем флаг перехвата контекста компрессии
	 */
	ws.mode({
		// server::web_t::flag_t::NOT_STOP,
		// server::web_t::flag_t::NOT_INFO,
		// server::web_t::flag_t::NOT_PING,
		server::web_t::flag_t::TAKEOVER_CLIENT,
		server::web_t::flag_t::TAKEOVER_SERVER
	});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
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
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
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
	// Разрешаем выполняем автоматический перезапуск упавшего процесса
	// core.clusterAutoRestart(true);
	// Активируем максимальное количество рабочих процессов
	core.cluster(awh::scheme_t::mode_t::ENABLED);
	// Выполняем активацию многопоточности
	// ws.multiThreads(22);
	// Устанавливаем название сервера
	// ws.realm("ANYKS");
	// Устанавливаем временный ключ сессии
	// ws.opaque("keySession");
	// Устанавливаем тип авторизации
	// ws.authType(awh::auth_t::type_t::BASIC);
	ws.authType(awh::auth_t::type_t::DIGEST, awh::auth_t::hash_t::MD5);
	// Выполняем инициализацию Websocket сервера
	ws.init(2222, "127.0.0.1", {awh::http_t::compressor_t::DEFLATE});
	// ws.init(2222, "", {awh::http_t::compressor_t::DEFLATE});
	// ws.init("anyks", {awh::http_t::compressor_t::DEFLATE});
	// Устанавливаем длительное подключение
	// ws.keepAlive(100, 30, 10);
	// Активируем шифрование
	// ws.encryption(true);
	// Устанавливаем пароль шифрования
	// ws.encryption(string{"PASS"});
	// Устанавливаем сабпротоколы
	ws.subprotocols({"test1", "test2", "test3"});
	// Подписываемся на получении события освобождения памяти протокола сетевого ядра
	core.callback <void (const uint64_t, const size_t)> ("available", std::bind(&Executor::available, &executor, _1, _2, &core));
	// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
	core.callback <void (const uint64_t, const char *, const size_t)> ("unavailable", std::bind(&Executor::unavailable, &executor, _1, _2, _3));
	// Устанавливаем функцию извлечения пароля пользователя для авторизации
	ws.callback <string (const uint64_t, const string &)> ("extractPassword", std::bind(&Executor::password, &executor, _1, _2));
	// Устанавливаем функцию проверки авторизации прользователя
	ws.callback <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&Executor::auth, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	ws.callback <bool (const string &, const string &, const uint32_t)> ("accept", std::bind(&Executor::accept, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	ws.callback <void (const uint64_t, const server::web_t::mode_t)> ("active", std::bind(&Executor::active, &executor, _1, _2, &core));
	// Установливаем функцию обратного вызова на событие получения ошибок
	ws.callback <void (const uint64_t, const uint32_t, const string &)> ("errorWebsocket", std::bind(&Executor::error, &executor, _1, _2, _3, &ws));
	// Установливаем функцию обратного вызова на событие получения сообщений
	ws.callback <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", std::bind(&Executor::message, &executor, _1, _2, _3, &ws));
	// Устанавливаем функцию обратного вызова на получение входящих сообщений запросов
	ws.callback <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", std::bind(&Executor::headers, &executor, _1, _2, _3, _4, _5));
	// Выполняем запуск Websocket сервер
	ws.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
