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
 * Proxy Класс объекта исполнителя
 */
class Proxy {
	private:
		// Объект логирования
		log_t * _log;
	public:
		/**
		 * password Метод извлечения пароля (для авторизации методом Digest)
		 * @param login логин пользователя
		 * @return      пароль пользователя хранящийся в базе данных
		 */
		string password(const string & login){
			// Выводим информацию в лог
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");
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
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
			// Разрешаем авторизацию
			return true;
		}
	public:
		/**
		 * accept Метод активации клиента на сервере
		 * @param ip    адрес интернет подключения
		 * @param mac   аппаратный адрес подключения
		 * @param port  порт подключения
		 * @param proxy объект PROXY сервера
		 * @return      результат проверки
		 */
		bool accept(const string & ip, const string & mac, const u_int port, server::proxy_t * proxy){
			// Выводим информацию в лог
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод идентификации активности на WebSocket сервере
		 * @param aid   идентификатор адъютанта (клиента)
		 * @param mode  режим события подключения
		 * @param proxy объект PROXY сервера
		 */
		void active(const size_t aid, const server::proxy_t::mode_t mode, server::proxy_t * proxy){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::proxy_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * message Метод получения сообщений
		 * @param aid   идентификатор адъютанта (клиента)
		 * @param event событие прокси-сервера (запрос / ответ)
		 * @param http  объект http запроса
		 * @param proxy объект прокси-сервера
		 * @return      результат обработки запроса
		 */
		bool message(const size_t aid, const server::proxy_t::event_t event, awh::http_t * http, server::proxy_t * proxy){
			// Определяем тип запроса
			cout << (event == server::proxy_t::event_t::REQUEST ? "REQUEST" : "RESPONSE") << endl;
			// Выводим список полученных заголовков
			for(auto & header : http->headers())
				// Выводим все полученные заголовки
				cout << "Header: " << header.first << " = " << header.second << endl << endl;
			// Разрешаем дальнейшую работу
			return true;
		}
	public:
		/**
		 * Proxy Конструктор
		 * @param log объект логирования
		 */
		Proxy(log_t * log) : _log(log) {}
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
	Proxy executor(&log);
	// Создаём объект PROXY сервера
	server::proxy_t proxy(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Proxy Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * 1. Устанавливаем ожидание входящих сообщений
	 */
	proxy.mode(
		// (uint8_t) server::proxy_t::flag_t::NOIFNO |
		(uint8_t) server::proxy_t::flag_t::WAITMESS
	);
	// Устанавливаем адрес сертификата
	// proxy.ca("./ca/cert.pem");
	// Устанавливаем название сервера
	// proxy.nameServer("anyks");
	// Устанавливаем тип сокета unix-сокет
	// proxy.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	// proxy.sonet(awh::scheme_t::sonet_t::DTLS);
	// proxy.sonet(awh::scheme_t::sonet_t::TLS);
	// proxy.sonet(awh::scheme_t::sonet_t::UDP);
	proxy.sonet(awh::scheme_t::sonet_t::TCP);
	// proxy.sonet(awh::scheme_t::sonet_t::SCTP);
	// Отключаем валидацию сертификата
	proxy.verifySSL(true);
	// Активируем максимальное количество рабочих процессов
	// proxy.clusterSize();
	// Устанавливаем таймаут ожидания получения сообщений
	// proxy.waitTimeDetect(60, 60);
	// Устанавливаем название сервера
	// proxy.realm("ANYKS");
	// Устанавливаем временный ключ сессии
	// proxy.opaque("keySession");
	// Устанавливаем тип авторизации
	// proxy.authType(auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// proxy.authType(auth_t::type_t::DIGEST, auth_t::hash_t::SHA512);
	// proxy.authType(auth_t::type_t::BASIC);
	// Выполняем инициализацию прокси-сервера
	// proxy.init(2222, "", http_t::compress_t::GZIP);
	proxy.init(2222, "127.0.0.1", http_t::compress_t::GZIP);
	// proxy.init("anyks", http_t::compress_t::GZIP);
	// Устанавливаем длительное подключение
	// proxy.keepAlive(100, 30, 10);
	/*
	// Устанавливаем SSL сертификаты сервера
	proxy.certificate(
		"/usr/local/etc/letsencrypt/live/anyks.net/fullchain.pem",
		"/usr/local/etc/letsencrypt/live/anyks.net/privkey.pem"
	);
	*/
	// proxy.certificate("./ca/certs/server-cert.pem", "./ca/certs/server-key.pem");
	// Устанавливаем шифрование
	// proxy.crypto("PASS");
	// Устанавливаем функцию извлечения пароля
	// proxy.on((function <string (const string &)>) bind(&Proxy::password, &executor, _1));
	// Устанавливаем функцию проверки авторизации
	// proxy.on((function <bool (const string &, const string &)>) bind(&Proxy::auth, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	proxy.on((function <void (const size_t, const server::proxy_t::mode_t, server::proxy_t *)>) bind(&Proxy::active, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	proxy.on((function <bool (const string &, const string &, const u_int, server::proxy_t *)>) bind(&Proxy::accept, &executor, _1, _2, _3, _4));
	// Установливаем функцию обратного вызова на событие получения сообщений
	proxy.on((function <bool (const size_t, const server::proxy_t::event_t, awh::http_t *, server::proxy_t *)>) bind(&Proxy::message, &executor, _1, _2, _3, _4));
	// Выполняем запуск Socks5 сервер
	proxy.start();
	// Выводим результат
	return 0;
}
