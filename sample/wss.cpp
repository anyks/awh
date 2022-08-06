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
 * WebSocket Класс объекта исполнителя
 */
class WebSocket {
	private:
		// Объект логирования
		log_t * log;
	public:
		/**
		 * password Метод извлечения пароля (для авторизации методом Digest)
		 * @param login логин пользователя
		 * @return      пароль пользователя хранящийся в базе данных
		 */
		string password(const string & login){
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
		bool auth(const string & login, const string & password){
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
		 * @param ws   объект WebSocket сервера
		 * @return     результат проверки
		 */
		bool accept(const string & ip, const string & mac, const u_int port, server::ws_t * ws){
			// Выводим информацию в лог
			this->log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод идентификации активности на WebSocket сервере
		 * @param aid  идентификатор адъютанта (клиента)
		 * @param mode режим события подключения
		 * @param ws   объект WebSocket сервера
		 */
		void active(const size_t aid, const server::ws_t::mode_t mode, server::ws_t * ws){
			// Выводим информацию в лог
			this->log->print("%s client", log_t::flag_t::INFO, (mode == server::ws_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * error Метод вывода ошибок WebSocket сервера
		 * @param aid  идентификатор адъютанта (клиента)
		 * @param code код ошибки
		 * @param mess сообщение ошибки
		 * @param ws   объект WebSocket сервера
		 */
		void error(const size_t aid, const u_int code, const string & mess, server::ws_t * ws){
			// Выводим информацию в лог
			this->log->print("%s [%u]", log_t::flag_t::CRITICAL, mess.c_str(), code);
		}
		/**
		 * message Метод получения сообщений
		 * @param aid    идентификатор адъютанта (клиента)
		 * @param buffer бинарный буфер сообщения
		 * @param utf8   тип буфера сообщения
		 * @param ws     объект WebSocket сервера
		 */
		void message(const size_t aid, const vector <char> & buffer, const bool utf8, server::ws_t * ws){
			// Если даныне получены
			if(!buffer.empty()){
				// Выводим информацию в лог
				this->log->print("message: %s [%s]", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str(), ws->sub(aid).c_str());
				// Отправляем сообщение обратно
				ws->send(aid, buffer.data(), buffer.size(), utf8);
			}
		}
	public:
		/**
		 * WebSocket Конструктор
		 * @param log объект логирования
		 */
		WebSocket(log_t * log) : log(log) {}
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
	// Создаём объект исполнителя
	WebSocket executor(&log);
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
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Активируем максимальное количество рабочих процессов
	// core.setForks();
	// Устанавливаем адрес сертификата
	// core.ca("./ca/cert.pem");
	// Устанавливаем название сервера
	// core.nameServer("anyks");
	// Устанавливаем тип сокета unix-сокет
	// core.family(core_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	core.sonet(core_t::sonet_t::DTLS);
	// core.sonet(core_t::sonet_t::TLS);
	// core.sonet(core_t::sonet_t::UDP);
	// core.sonet(core_t::sonet_t::TCP);

	// Выполняем активацию многопоточности
	// ws.multiThreads(22);
	// Устанавливаем название сервера
	// ws.setRealm("ANYKS");
	// Устанавливаем временный ключ сессии
	// ws.setOpaque("keySession");
	// Устанавливаем тип авторизации
	// ws.setAuthType(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
	// Выполняем инициализацию WebSocket сервера
	ws.init(2222, "127.0.0.1", http_t::compress_t::DEFLATE);
	// ws.init(2222, "", http_t::compress_t::DEFLATE);
	// ws.init(2222, "127.0.0.1", http_t::compress_t::NONE);
	// ws.init("anyks", http_t::compress_t::DEFLATE);

	
	// Устанавливаем SSL сертификаты сервера
	core.certificate(
		"/usr/local/etc/letsencrypt/live/anyks.net/fullchain.pem",
		"/usr/local/etc/letsencrypt/live/anyks.net/privkey.pem"
	);
	
	

	// core.certificate("./certs/server-cert.pem", "./certs/server-key.pem");

	// Устанавливаем шифрование
	// ws.setCrypt("PASS");
	// Устанавливаем сабпротоколы
	// ws.setSubs({"test1", "test2", "test3"});
	// Устанавливаем функцию извлечения пароля
	// ws.on((function <string (const string &)>) bind(&WebSocket::password, &executor, _1));
	// Устанавливаем функцию проверки авторизации
	// ws.on((function <bool (const string &, const string &)>) bind(&WebSocket::auth, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	ws.on((function <void (const size_t, const server::ws_t::mode_t, server::ws_t *)>) bind(&WebSocket::active, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие получения ошибок
	ws.on((function <void (const size_t, const u_int, const string &, server::ws_t *)>) bind(&WebSocket::error, &executor, _1, _2, _3, _4));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	ws.on((function <bool (const string &, const string &, const u_int, server::ws_t *)>) bind(&WebSocket::accept, &executor, _1, _2, _3, _4));
	// Установливаем функцию обратного вызова на событие получения сообщений
	ws.on((function <void (const size_t, const vector <char> &, const bool, server::ws_t *)>) bind(&WebSocket::message, &executor, _1, _2, _3, _4));
	// Выполняем запуск WebSocket сервер
	ws.start();
	// Выводим результат
	return 0;
}
