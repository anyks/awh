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
#include <server/proxy.hpp>
#include <nlohmann/json.hpp>

// Подключаем пространство имён
using namespace std;
using namespace awh;

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int main(int argc, char * argv[]) noexcept {
	// Создаём объект фреймворка
	fmk_t fmk(true);
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект PROXY сервера
	proxySrv_t proxy(&fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("Proxy Server");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	// proxy.setMode((uint8_t) wsSrv_t::flag_t::WAITMESS);
	// Устанавливаем название сервера
	proxy.setRealm("ANYKS");
	// Устанавливаем временный ключ сессии
	proxy.setOpaque("keySession");
	// Устанавливаем тип авторизации
	// proxy.setAuthType();
	// proxy.setAuthType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию WebSocket сервера
	proxy.init(2222, "127.0.0.1", http_t::compress_t::GZIP);
	// Устанавливаем шифрование
	// proxy.setCrypt("PASS");

	/*
	// Устанавливаем функцию извлечения пароля
	proxy.on(&log, [](const string & user, void * ctx) -> string {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), "password");
		// Выводим пароль
		return "password";
	});
	// Устанавливаем функцию проверки авторизации
	proxy.on(&log, [](const string & user, const string & password, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), password.c_str());
		// Разрешаем авторизацию
		return true;
	});
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	proxy.on(&log, [](const string & ip, const string & mac, restSrv_t * rest, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("ACCEPT: ip = %s, mac = %s", log_t::flag_t::INFO, ip.c_str(), mac.c_str());
		// Разрешаем подключение клиенту
		return true;
	});
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	proxy.on(&log, [](const size_t aid, const bool mode,  restSrv_t * rest, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (mode ? "Connect" : "Disconnect"));
	});
	// Установливаем функцию обратного вызова на событие получения сообщений
	proxy.on(&log, [](const size_t aid, const restSrv_t::req_t & req, restSrv_t * rest, void * ctx) noexcept {
		// Если пришёл запрос на фавиконку
		if(!req.path.empty() && (req.path.front().compare("favicon.ico") == 0))
			// Выполняем реджект
			rest->reject(aid, 404);
		// Если метод GET
		else if(req.method == web_t::method_t::GET){
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
			rest->response(aid, 200, "OK", vector <char> (body.begin(), body.end()));//, {{"Connection", "close"}});
		}
	});
	*/
	// Выполняем запуск REST сервер
	proxy.start();
	// Выводим результат
	return 0;
}
