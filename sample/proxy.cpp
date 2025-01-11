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
		 * @param bid   идентификатор брокера
		 * @param login логин пользователя
		 * @return      пароль пользователя хранящийся в базе данных
		 */
		string password(const uint64_t bid, const string & login){
			// Выводим информацию в лог
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), "password");
			// Выводим пароль
			return "password";
		}
		/**
		 * auth Метод проверки авторизации пользователя (для авторизации методом Basic)
		 * @param bid      идентификатор брокера
		 * @param login    логин пользователя (от клиента)
		 * @param password пароль пользователя (от клиента)
		 * @return         результат авторизации
		 */
		bool auth(const uint64_t bid, const string & login, const string & password){
			// Выводим информацию в лог
			this->_log->print("USER: %s, PASS: %s", log_t::flag_t::INFO, login.c_str(), password.c_str());
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
			this->_log->print("ACCEPT: ip = %s, mac = %s, port = %d", log_t::flag_t::INFO, ip.c_str(), mac.c_str(), port);
			// Разрешаем подключение клиенту
			return true;
		}
		/**
		 * active Метод идентификации активности на Websocket сервере
		 * @param bid    идентификатор брокера
		 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
		 * @param mode   режим события подключения
		 */
		void active(const uint64_t bid, const server::proxy_t::broker_t broker, const server::web_t::mode_t mode){
			// Определяем тип подключения
			switch(static_cast <uint8_t> (broker)){
				// Если событие принадлежит клиенту
				case static_cast <uint8_t> (server::proxy_t::broker_t::CLIENT):
					// Выводим информацию в лог
					this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
				break;
				// Если событие принадлежит серверу
				case static_cast <uint8_t> (server::proxy_t::broker_t::SERVER):
					// Выводим информацию в лог
					this->_log->print("%s server", log_t::flag_t::INFO, (mode == server::web_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
				break;
			}
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
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект параметров SSL-шифрования
	node_t::ssl_t ssl;
	// Создаём объект исполнителя
	Proxy executor(&log);
	// Создаём объект прокси-сервера
	server::proxy_t proxy(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Proxy Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Активируем максимальное количество рабочих процессов
	proxy.cluster(awh::scheme_t::mode_t::ENABLED);
	// Устанавливаем название сервера
	// proxy.realm("ANYKS");
	// Устанавливаем временный ключ сессии
	// proxy.opaque("keySession");
	// Отключаем валидацию сертификата
	ssl.verify = false;
	// Устанавливаем адрес сертификата
	ssl.ca = "./certs/ca.pem";
	/*
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "./certs/certificates/server-key.pem";
	ssl.cert = "./certs/certificates/server-cert.pem";
	*/
	/*
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "/usr/local/etc/letsencrypt/live/anyks.net/privkey.pem";
	ssl.cert = "/usr/local/etc/letsencrypt/live/anyks.net/fullchain.pem";
	*/
	// Выполняем установку параметров SSL-шифрования
	proxy.ssl(ssl);
	// Подключаем локальные хосты
	proxy.hosts(server::proxy_t::broker_t::CLIENT, "/etc/hosts");
	/**
	 * 1. Устанавливаем синхронизацию протоколов клиента и сервера
	 * 2. Устанавливаем разрешение на выполнение автоматических редиректов
	 * 3. Устанавливаем разрешение на использоваения метода CONNECT
	 */
	proxy.mode({
		server::proxy_t::flag_t::SYNCPROTO,
		server::proxy_t::flag_t::REDIRECTS,
		server::proxy_t::flag_t::CONNECT_METHOD_SERVER_ENABLE
	});
	// Устанавливаем таймаут ожидания получения сообщений
	proxy.waitTimeDetect(server::proxy_t::broker_t::CLIENT, 10, 10, 5);
	// proxy.waitTimeDetect(server::proxy_t::broker_t::SERVER, 60, 60);
	// Устанавливаем тип сокета unix-сокет
	// proxy.family(server::proxy_t::broker_t::SERVER, awh::scheme_t::family_t::NIX);
	// Устанавливаем тип авторизации
	// proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::BASIC);
	// proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::DIGEST, auth_t::hash_t::SHA512);
	proxy.authType(server::proxy_t::broker_t::SERVER, auth_t::type_t::DIGEST, auth_t::hash_t::MD5);
	// Выполняем инициализацию прокси-сервера
	// proxy.init(2222, "", http_t::compressor_t::GZIP);
	proxy.init(2222, "127.0.0.1", http_t::compressor_t::GZIP);
	// proxy.init("anyks", http_t::compressor_t::GZIP);
	// Устанавливаем длительное подключение
	// proxy.keepAlive(100, 30, 10);
	// Устанавливаем шифрование
	// proxy.encryption(server::proxy_t::broker_t::SERVER, "PASS");
	// Устанавливаем функцию извлечения пароля
	proxy.callback <string (const uint64_t, const string &)> ("extractPassword", std::bind(&Proxy::password, &executor, _1, _2));
	// Устанавливаем функцию проверки авторизации
	proxy.callback <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&Proxy::auth, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	proxy.callback <bool (const string &, const string &, const uint32_t)> ("accept", std::bind(&Proxy::accept, &executor, _1, _2, _3));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	proxy.callback <void (const uint64_t, const server::proxy_t::broker_t, const server::web_t::mode_t)> ("active", std::bind(&Proxy::active, &executor, _1, _2, _3));
	// Выполняем запуск Proxy-сервер
	proxy.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
