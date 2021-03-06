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
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём биндинг
	server::core_t core(&fmk, &log);
	// Создаём объект REST запроса
	server::ws_t ws(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("WebSocket Server");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	/*
	ws.setMode(
		(uint8_t) server::ws_t::flag_t::WAITMESS |
		(uint8_t) server::ws_t::flag_t::TAKEOVERCLI |
		(uint8_t) server::ws_t::flag_t::TAKEOVERSRV
	);
	*/
	// Устанавливаем название сервера
	// ws.setRealm("ANYKS");
	// Устанавливаем временный ключ сессии
	// ws.setOpaque("keySession");
	// Устанавливаем тип авторизации
	// ws.setAuthType(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
	// Выполняем инициализацию WebSocket сервера
	ws.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);
	// ws.init(2222, "127.0.0.1", http_t::compress_t::NONE);
	// Устанавливаем шифрование
	// ws.setCrypt("PASS");
	// Устанавливаем сабпротоколы
	// ws.setSubs({"test1", "test2", "test3"});
	/*
	// Устанавливаем функцию извлечения пароля
	ws.on(&log, [](const string & user, void * ctx) -> string {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), "password");
		// Выводим пароль
		return "password";
	});
	// Устанавливаем функцию проверки авторизации
	ws.on(&log, [](const string & user, const string & password, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), password.c_str());
		// Разрешаем авторизацию
		return true;
	});
	*/
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	ws.on(&log, [](const string & ip, const string & mac, server::ws_t * ws, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("ACCEPT: ip = %s, mac = %s", log_t::flag_t::INFO, ip.c_str(), mac.c_str());
		// Разрешаем подключение клиенту
		return true;
	});
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	ws.on(&log, [](const size_t aid, const server::ws_t::mode_t mode, server::ws_t * ws, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (mode == server::ws_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
	});
	// Установливаем функцию обратного вызова на событие получения ошибок
	ws.on(&log, [](const size_t aid, const u_int code, const string & mess, server::ws_t * ws, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});
	// Установливаем функцию обратного вызова на событие получения сообщений
	ws.on(&log, [](const size_t aid, const vector <char> & buffer, const bool utf8, server::ws_t * ws, void * ctx) noexcept {
		// Если даныне получены
		if(!buffer.empty()){
			// Получаем объект логирования
			log_t * log = reinterpret_cast <log_t *> (ctx);
			// Выводим информацию в лог
			log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->getSub(aid).c_str());
			// Отправляем сообщение обратно
			ws->send(aid, buffer.data(), buffer.size(), utf8);
		}
	});
	// Выполняем запуск WebSocket сервер
	ws.start();
	// Выводим результат
	return 0;
}
