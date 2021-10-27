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
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
	// Создаём биндинг
	coreSrv_t core(&fmk, &log);
	// Создаём объект REST запроса
	restSrv_t rest(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("Rest Server");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	// rest.setMode((uint8_t) wsSrv_t::flag_t::WAITMESS);
	// Устанавливаем название сервера
	// rest.setRealm("ANYKS");
	// Устанавливаем временный ключ сессии
	// rest.setOpaque("keySession");
	// Устанавливаем тип авторизации
	// rest.setAuthType(auth_t::type_t::DIGEST, auth_t::alg_t::SHA256);
	// Выполняем инициализацию WebSocket сервера
	rest.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);
	// Устанавливаем шифрование
	// rest.setCrypt("PASS");
	// Устанавливаем сабпротоколы
	// rest.setSubs({"test1", "test2", "test3"});
	/*
	// Устанавливаем функцию извлечения пароля
	rest.setExtractPassCallback(&log, [](const string & user, void * ctx) -> string {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), "password");
		// Выводим пароль
		return "password";
	});
	// Устанавливаем функцию проверки авторизации
	rest.setAuthCallback(&log, [](const string & user, const string & password, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, user.c_str(), password.c_str());
		// Разрешаем авторизацию
		return true;
	});
	*/
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	rest.on(&log, [](const string & ip, const string & mac, restSrv_t * rest, void * ctx) -> bool {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("ACCEPT: ip = %s, mac = %s", log_t::flag_t::INFO, ip.c_str(), mac.c_str());
		// Разрешаем подключение клиенту
		return true;
	});
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	rest.on(&log, [](const size_t aid, const bool mode,  restSrv_t * rest, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (mode ? "Connect" : "Disconnect"));
	});
	// Установливаем функцию обратного вызова на событие получения ошибок
	rest.on(&log, [](const size_t aid, const u_short code, const string & mess,  restSrv_t * rest, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});
	// Установливаем функцию обратного вызова на событие получения сообщений
	rest.on(&log, [](const size_t aid, const vector <char> & buffer, const bool utf8,  restSrv_t * rest, void * ctx) noexcept {
		// Если даныне получены
		if(!buffer.empty()){
			// Получаем объект логирования
			log_t * log = reinterpret_cast <log_t *> (ctx);
			// Выводим информацию в лог
			log->print("message: %s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());
			// Отправляем сообщение обратно
			// rest->send(aid, buffer.data(), buffer.size(), utf8);
		}
	});
	// Выполняем запуск REST сервер
	rest.start();
	// Выводим результат
	return 0;
}
