/**
 * @file: socks5.cpp
 * @date: 2025-03-02
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <server/socks5.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;
using namespace server;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Proxy Класс объекта исполнителя
 */
class Proxy {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	public:
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
		 * active Метод идентификации активности на Websocket сервере
		 * @param bid  идентификатор брокера (клиента)
		 * @param mode режим события подключения
		 */
		void active([[maybe_unused]] const uint64_t bid, const proxy_socks5_t::mode_t mode){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == proxy_socks5_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
	public:
		/**
		 * Proxy Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		Proxy(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
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
	// Создаём объект прокси-сервера
	proxy_socks5_t proxy(&fmk, &log);
	// Создаём объект исполнителя
	Proxy executor(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Proxy Socks5 Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * Запрет вывода информационных сообщений
	 */
	// proxy.mode({client::sample_t::flag_t::NOT_INFO});
	// Отключаем валидацию сертификата
	ssl.verify = true;
	// Устанавливаем адрес сертификата
	ssl.ca = "./certs/ca.pem";
	// Выполняем установку параметров SSL-шифрования
	proxy.ssl(ssl);
	// Устанавливаем тип сокета unix-сокет
	// proxy.family(awh::scheme_t::family_t::NIX);
	// Устанавливаем тип сокета UDP TLS
	// proxy.sonet(awh::scheme_t::sonet_t::DTLS);
	// proxy.sonet(awh::scheme_t::sonet_t::TLS);
	// proxy.sonet(awh::scheme_t::sonet_t::UDP);
	// proxy.sonet(awh::scheme_t::sonet_t::TCP);
	// proxy.sonet(awh::scheme_t::sonet_t::SCTP);
	// Активируем максимальное количество рабочих процессов
	proxy.cluster(awh::scheme_t::mode_t::ENABLED);
	// Устанавливаем таймаут ожидания получения сообщений
	// proxy.waitTimeDetect(10, 10);
	// Выполняем инициализацию Socks5-сервера
	// proxy.init(2222, "anyks.net");
	proxy.init(2222, "127.0.0.1");
	// proxy.init("anyks");
	// Устанавливаем длительное подключение
	// proxy.keepAlive(100, 30, 10);
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	proxy.callback <void (const size_t, const proxy_socks5_t::mode_t)> ("active", std::bind(&Proxy::active, &executor, _1, _2));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	proxy.callback <bool (const string &, const string &, const uint32_t)> ("accept", std::bind(&Proxy::accept, &executor, _1, _2, _3));
	// Устанавливаем функцию проверки авторизации
	// proxy.callback <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&Proxy::auth, &executor, _1, _2, _3));
	// Выполняем запуск Socks5 сервер
	proxy.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
