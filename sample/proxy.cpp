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

// Подключаем пространство имён
using namespace std;
using namespace awh;

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
	server::proxy_t proxy(&fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("Proxy Server");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	proxy.setMode((uint8_t) server::proxy_t::flag_t::WAITMESS);
	// Устанавливаем название сервера
	proxy.setRealm("ANYKS");
	// Устанавливаем временный ключ сессии
	proxy.setOpaque("keySession");
	// Устанавливаем таймаут ожидания получения сообщений
	proxy.setWaitTimeDetect(30, 15);
	// Устанавливаем тип авторизации
	// proxy.setAuthType();
	proxy.setAuthType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию WebSocket сервера
	proxy.init(2222, "127.0.0.1", http_t::compress_t::GZIP);
	// Устанавливаем шифрование
	// proxy.setCrypt("PASS");
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
	proxy.on(&log, [](const string & ip, const string & mac, server::proxy_t * proxy, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("ACCEPT: ip = %s, mac = %s", log_t::flag_t::INFO, ip.c_str(), mac.c_str());
		// Разрешаем подключение клиенту
		return true;
	});
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	proxy.on(&log, [](const size_t aid, const server::proxy_t::mode_t mode, server::proxy_t * proxy, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (mode == server::proxy_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
	});
	// Установливаем функцию обратного вызова на событие получения сообщений
	proxy.on(&log, [](const size_t aid, const server::proxy_t::event_t event, http_t * http, server::proxy_t * proxy, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (event == server::proxy_t::event_t::RESPONSE ? "Response" : "Request"));
		// Выводим результат
		return true;
	});
	// Выполняем запуск Proxy сервер
	proxy.start();
	// Выводим результат
	return 0;
}
