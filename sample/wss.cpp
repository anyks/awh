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
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект сети
	network_t nwk(&fmk);
	// Создаём объект URI
	uri_t uri(&fmk, &nwk);
	// Создаём биндинг
	coreSrv_t core(&fmk, &log);
	// Создаём объект REST запроса
	wsSrv_t ws(&core, &fmk, &log);
	// Устанавливаем название сервиса
	log.setLogName("WebSocket Server");
	// Устанавливаем формат времени
	log.setLogFormat("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	// ws.setMode((uint8_t) wsSrv_t::flag_t::WAITMESS);
	// Устанавливаем название сервера
	ws.setRealm("ANYKS");
	// Устанавливаем временный ключ сессии
	ws.setOpaque("keySession");
	// Устанавливаем тип авторизации
	ws.setAuthType(auth_t::type_t::DIGEST, auth_t::alg_t::SHA256);
	// Выполняем инициализацию WebSocket сервера
	ws.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);
	// Устанавливаем функцию извлечения пароля
	ws.setExtractPassCallback([](const string & user) -> string {

		cout << " @@@@@@@@@@@@@ " << user << endl;

		// Выводим пароль
		return "password";
	});
	// Устанавливаем функцию проверки авторизации
	ws.setAuthCallback([](const string & user, const string & password) -> bool {

		cout << " @@@@@@@@@@@@@ " << user << " == " << password << endl;

		// Разрешаем авторизацию
		return true;
	});
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	ws.on(nullptr, [](const string & ip, const string & mac, wsSrv_t * ws, void * ctx) -> bool {

		cout << " +++++++++++++++ ACCEPT " << ip << " == " << mac << endl;

		// Разрешаем подключение клиенту
		return true;
	});
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	ws.on(&log, [](const size_t aid, const bool mode,  wsSrv_t * ws, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s client", log_t::flag_t::INFO, (mode ? "Connect" : "Disconnect"));
	});
	// Установливаем функцию обратного вызова на событие получения ошибок
	ws.on(&log, [](const size_t aid, const u_short code, const string & mess,  wsSrv_t * ws, void * ctx) noexcept {
		// Получаем объект логирования
		log_t * log = reinterpret_cast <log_t *> (ctx);
		// Выводим информацию в лог
		log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
	});
	/// Установливаем функцию обратного вызова на событие получения сообщений
	ws.on(nullptr, [](const size_t aid, const vector <char> & buffer, const bool utf8,  wsSrv_t * ws, void * ctx) noexcept {
		// Если даныне получены
		if(!buffer.empty()){

			cout << " !!!!!!!!!! " << string(buffer.begin(), buffer.end()) << endl;

			// Отправляем сообщение обратно
			ws->send(aid, buffer.data(), buffer.size(), utf8);
		}
	});
	// Выполняем запуск WebSocket клиента
	ws.start();
	// Выводим результат
	return 0;
}
