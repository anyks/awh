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
#include <server/rest.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

/**
 * Rest Класс объекта исполнителя
 */
class Rest {
	private:
		// Объект логирования
		log_t * log;
	public:
		/**
		 * password Метод извлечения пароля (для авторизации методом Digest)
		 * @param login логин пользователя
		 * @return      пароль пользователя хранящийся в базе данных
		 */
		string password(const string & login) noexcept {
			// Выводим информацию в лог
			this->log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");
			// Выводим пароль
			return "password";
		}
		/**
		 * auth Метод проверки авторизации пользователя (для авторизации методом Basic)
		 * @param login    логин пользователя (от клиента)
		 * @param password пароль пользователя (от клиента)
		 * @return         результат авторизации
		 */
		bool auth(const string & login, const string & password) noexcept {
			// Выводим информацию в лог
			this->log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
			// Разрешаем авторизацию
			return true;
		}
	public:
		/**
		 * accept Метод активации клиента на сервере
		 * @param ip   адрес интернет подключения
		 * @param mac  аппаратный адрес подключения
		 * @param port порт подключения
		 * @param web  объект WEB сервера
		 * @return     результат проверки
		 */
		bool accept(const string & ip, const string & mac, const u_int port, server::rest_t * web) noexcept {
			// Выводим информацию в лог
			this->log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод идентификации активности на Web сервере
		 * @param aid  идентификатор адъютанта (клиента)
		 * @param mode режим события подключения
		 * @param web  объект WEB сервера
		 */
		void active(const size_t aid, const server::rest_t::mode_t mode, server::rest_t * web) noexcept {
			// Выводим информацию в лог
			this->log->print("%s client", log_t::flag_t::INFO, (mode == server::rest_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * message Метод получения сообщений
		 * @param aid  идентификатор адъютанта (клиента)
		 * @param http объект http запроса
		 * @param web  объект WEB сервера
		 */
		void message(const size_t aid, const awh::http_t * http, server::rest_t * web) noexcept {
			// Получаем данные запроса
			const auto & query = http->query();
			// Если пришёл запрос на фавиконку
			if(!query.uri.empty() && (query.uri.find("favicon.ico") != string::npos))
				// Выполняем реджект
				web->reject(aid, 404);
			// Если метод GET
			else if(query.method == web_t::method_t::GET){
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
				// Отправляем сообщение клиенту
				web->response(aid, 200, "OK", vector <char> (body.begin(), body.end()));//, {{"Connection", "close"}});
			}
		}
	public:
		/**
		 * Rest Конструктор
		 * @param log объект логирования
		 */
		Rest(log_t * log) : log(log) {}
};

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
	// Создаём объект исполнителя
	Rest executor(&log);
	// Создаём биндинг
	server::core_t core(&fmk, &log);
	// Создаём объект REST запроса
	server::rest_t rest(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("Rest Server");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");

	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	/*
	rest.mode((uint8_t) server::rest_t::flag_t::WAITMESS);
	*/

	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Устанавливаем адрес сертификата
	// core.ca("./ca/cert.pem");
	// Устанавливаем название сервера
	// core.nameServer("anyks");
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	// core.sonet(awh::scheme_t::sonet_t::DTLS);
	// core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	core.sonet(awh::scheme_t::sonet_t::TCP);

	// Отключаем валидацию сертификата
	core.verifySSL(false);

	// Активируем максимальное количество рабочих процессов
	core.clusterSize();

	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	// Устанавливаем режим мультипоточной обработки
	// core.multiThreads(22);

	// Устанавливаем название сервера
	// rest.realm("ANYKS");
	// Устанавливаем временный ключ сессии
	// rest.opaque("keySession");
	// Устанавливаем тип авторизации
	// rest.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// rest.authType(auth_t::type_t::BASIC);
	// Выполняем инициализацию WebSocket сервера
	// rest.init(2222, "127.0.0.1", http_t::compress_t::ALL_COMPRESS);
	// rest.init(2222, "", http_t::compress_t::ALL_COMPRESS);
	rest.init(2222, "127.0.0.1", http_t::compress_t::ALL_COMPRESS);
	// rest.init("anyks", http_t::compress_t::ALL_COMPRESS);
	// Устанавливаем длительное подключение
	// rest.keepAlive(100, 30, 10);


	/*
	// Устанавливаем SSL сертификаты сервера
	core.certificate(
		"/usr/local/etc/letsencrypt/live/anyks.net/fullchain.pem",
		"/usr/local/etc/letsencrypt/live/anyks.net/privkey.pem"
	);
	*/
	

	// core.certificate("./certs/server-cert.pem", "./certs/server-key.pem");

	// Устанавливаем шифрование
	// rest.crypto("PASS");


	// Устанавливаем функцию извлечения пароля
	// rest.on((function <string (const string &)>) bind(&Rest::password, &executor, _1));
	// Устанавливаем функцию проверки авторизации
	// rest.on((function <bool (const string &, const string &)>) bind(&Rest::auth, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие получения сообщений
	rest.on((function <void (const size_t, const awh::http_t *, server::rest_t *)>) bind(&Rest::message, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	rest.on((function <void (const size_t, const server::rest_t::mode_t, server::rest_t *)>) bind(&Rest::active, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	rest.on((function <bool (const string &, const string &, const u_int, server::rest_t *)>) bind(&Rest::accept, &executor, _1, _2, _3, _4));
	// Выполняем запуск REST сервер
	rest.start();
	// Выводим результат
	return 0;
}
