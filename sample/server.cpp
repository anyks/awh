/**
 * @file: server.cpp
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
 * Подключаем заголовочный файл проекта
 */
#include <server/sample.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Server Класс объекта исполнителя
 */
class Server {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
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
		 * launched Метод получения события запуска сервера
		 * @param host хост запущенного сервера
		 * @param port порт запущенного сервера
		 */
		void launched(const string & host, const uint32_t port){
			// Выводим информацию в лог
			this->_log->print("Launched: HOST=%s, PORT=%d", log_t::flag_t::INFO, host.c_str(), port);
		}
		/**
		 * active Метод идентификации активности на сервере
		 * @param bid  идентификатор брокера
		 * @param mode режим события подключения
		 */
		void active([[maybe_unused]] const uint64_t bid, const server::sample_t::mode_t mode){
			// Выводим информацию в лог
			this->_log->print("%s client", log_t::flag_t::INFO, (mode == server::sample_t::mode_t::CONNECT ? "Connect" : "Disconnect"));
		}
		/**
		 * message Метод получения сообщений
		 * @param bid    идентификатор брокера
		 * @param buffer буфер входящих данных
		 * @param sample объект активного сервера
		 */
		void message(const uint64_t bid, const vector <char> & buffer, server::sample_t * sample){
			// Выводим информацию в лог
			this->_log->print("%s", log_t::flag_t::INFO, string(buffer.begin(), buffer.end()).c_str());
			// Отправляем сообщение обратно
			sample->send(bid, buffer.data(), buffer.size());
		}
	public:
		/**
		 * Server Конструктор
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		Server(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
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
	// Объект DNS-резолвера
	dns_t dns(&fmk, &log);
	// Создаём объект сетевого ядра
	server::core_t core(&dns, &fmk, &log);
	// Создаём объект сервера
	server::sample_t sample(&core, &fmk, &log);
	// Создаём объект исполнителя
	Server executor(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("SAMPLE Server");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	/**
	 * Запрет вывода информационных сообщений
	 */
	// sample.mode({server::sample_t::flag_t::NOT_INFO});
	// Устанавливаем простое чтение базы событий
	// core.easily(true);
	// Активируем максимальное количество рабочих процессов
	// core.cluster(awh::scheme_t::mode_t::ENABLED);
	// Отключаем валидацию сертификата
	ssl.verify = false;
	// Устанавливаем SSL сертификаты сервера
	ssl.key  = "./certs/certificates/server-key.pem";
	ssl.cert = "./certs/certificates/server-cert.pem";
	// Выполняем установку параметров SSL-шифрования
	core.ssl(ssl);
	// Устанавливаем тип сокета unix-сокет
	// core.family(awh::scheme_t::family_t::IPC);
	// Устанавливаем тип сокета
	core.sonet(awh::scheme_t::sonet_t::DTLS);
	// core.sonet(awh::scheme_t::sonet_t::TLS);
	// core.sonet(awh::scheme_t::sonet_t::UDP);
	// core.sonet(awh::scheme_t::sonet_t::TCP);
	// core.sonet(awh::scheme_t::sonet_t::SCTP);
	// Выполняем инициализацию Sample сервера
	// sample.init(2222);
	// sample.init("anyks");
	sample.init(2222, "127.0.0.1");
	// Устанавливаем длительное подключение
	// sample.keepAlive(100, 30, 10);
	// Устанавливаем таймеры ожидания по одной секунде на чтение и запись
	sample.waitTimeDetect(1, 1);
	// Запрещаем перехват сигналов
	core.signalInterception(scheme_t::mode_t::DISABLED);
	// Активируем правило асинхронной работы передачи данных
	core.transferRule(server::core_t::transfer_t::ASYNC);
	// Устанавливаем функцию обратного вызова для выполнения события запуска сервера
	sample.on <void (const string &, const uint32_t)> ("launched", &Server::launched, &executor, _1, _2);
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	sample.on <void (const uint64_t, const server::sample_t::mode_t)> ("active", &Server::active, &executor, _1, _2);
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	sample.on <bool (const string &, const string &, const uint32_t)> ("accept", &Server::accept, &executor, _1, _2, _3);
	// Установливаем функцию обратного вызова на событие получения сообщений
	sample.on <void (const uint64_t, const vector <char> &)> ("message", &Server::message, &executor, _1, _2, &sample);
	// Выполняем запуск SAMPLE сервер
	sample.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
