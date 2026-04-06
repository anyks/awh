/**
 * @file: tls-web.cpp
 * @date: 2026-04-06
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <client/client.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Класс объекта исполнителя
 *
 */
class Executor {
	private:
		// Создаём объект фреймворка
		const fmk_t * _fmk;
		// Создаём объект работы с логами
		const log_t * _log;
	public:
		/**
		 * @brief Метод обработки событий изменения состояния клиента
		 *
		 * @param eid    идентификатор клиента
		 * @param status новый статус клиента
		 */
		void state([[maybe_unused]] const event::id_t eid, const event::status_t status) noexcept {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Выводим сообщение о запуске события клиента
					this->_log->print("Client event is launched", log_t::flag_t::INFO);
				break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Выводим сообщение об остановке события клиента
					this->_log->print("Событие клинта было остановлено", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий записи данных клиентом
		 *
		 * @param eid  идентификатор клиента
		 * @param size размер данных для записи
		 */
		void write(const event::id_t eid, const size_t size) noexcept {
			// Выводим информацию о событии записи данных клиентом
			this->_log->print("Client write event: %zu bytes", log_t::flag_t::INFO, size);
		}
		/**
		 * @brief Метод обработки событий чтения данных клиентом
		 *
		 * @param eid    идентификатор клиента
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param client объект клиента
		 */
		void read([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size, client_t * client) noexcept {
			// Если данные получены
			if(size > 0)
				// Выводим данные в лог
				this->_log->print("%s", log_t::flag_t::INFO, string(reinterpret_cast <const char *> (data), size).c_str());
			// Если данные не получены, то выводим сообщение об отсутствии данных
			else this->_log->print("No data received", log_t::flag_t::WARNING);
			// Останавливаем событие клиента
			client->stop();
		}
		/**
		 * @brief Метод обработки событий подключения клиента к удалённому серверу
		 *
		 * @param eid    идентификатор клиента
		 * @param ok     результат подключения
		 * @param client объект клиента
		 */
		void connect([[maybe_unused]] const event::id_t eid, const bool ok, client_t * client) noexcept {
			// Если подключение выполнено
			if(ok){
				// Текст запроса к серверу
				const string request =
					"GET / HTTP/1.1\r\n"
					"Host: anyks.com\r\n"
					"Connection: close\r\n"
					"User-Agent: iouring-openssl-sample/1.0\r\n"
					"\r\n";
				// Если отправка данных данных клиентом на сервер не выполнена
				if(client->send(request.c_str(), request.size()) == 0)
					// Выводим сообщение об ошибке отправки данных клиентом на сервер
					this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
			// Если подключение не выполнено, то выводим сообщение об ошибке подключения клиента к удалённому серверу
			} else this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
		}
		/**
		 * @brief Метод обработки событий готовности клиента к работе
		 *
		 * @param eid    идентификатор клиента
		 * @param family семейство адресов клиента
		 * @param domain доменное имя клиента
		 * @param ip     IP-адрес клиента
		 * @param client объект клиента
		 */
		void ready([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::family_t family, const string & domain, const string & ip, client_t * client) noexcept {
			// Выполняем подключение клиента к удалённому серверу
			if(!client->connect())
				// Выводим сообщение об ошибке
				this->_log->print("Failed to connect to remote server: %s (%s)", log_t::flag_t::WARNING, domain.c_str(), ip.c_str());
			// Если подключение выполнено, то выводим сообщение об успешном подключении клиента к удалённому серверу
			else this->_log->print("Successfully connected to remote server: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки ошибок клиента
		 *
		 * @param eid    идентификатор клиента
		 * @param error  код ошибки
		 * @param message сообщение об ошибке
		 */
		void error([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & message) noexcept {
			// Выводим сообщение об ошибке
			this->_log->print("Client error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
		/**
		 * @brief Метод обработки ошибок транспортного уровня безопасности TLS
		 *
		 * @param id      идентификатор TLS
		 * @param error   код ошибки TLS
		 * @param message сообщение об ошибке TLS
		 */
		void errorTLS([[maybe_unused]] const tls_t::id_t id, [[maybe_unused]] const tls_t::error_t error, const string & message) noexcept {
			// Выводим сообщение об ошибке TLS
			this->_log->print("TLS error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 */
		Executor(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект транспортного уровня безопасности
	tls_t tls(&fmk, &log);
	// Создаём объект исполнителя для обработки событий клиента
	Executor executor(&fmk, &log);
	// Создаём объект юнита клиента
	unit::client_t unit(&fmk, &log);
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Создаём объект клиента
	client_t client(&unit, &dns, &tls, &fmk, &log);
	// Создаём событие клиента и сохраняем его идентификатор
	const event::id_t eid = unit.issue(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Устананавливаем опции события
	if(unit.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
		// Выводим сообщение об успешной установке опций события
		cout << " Успешно установлены опции события!" << endl;
	// Выводим сообщение об ошибке установки опций события
	else cout << " Ошибка установки опций события!" << endl;
	// Регистрируем объект транспортного уровня безопасности
	const tls_t::id_t cts = tls.context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Устанавливаем хост сервера для подключения клиента
	const string host = "anyks.com";
	// Устанавливаем ALPN протоколы TLS
	tls.alpn(cts, {{0,"http/1.1"}});
	// Устанавливаем файл центра сертификации TLS
	tls.ca(cts, "../sh/certificates", "ca.pem");
	// Включаем проверку имени хоста TLS
	tls.validateServerNameIndication(cts, false);
	// Устанавливаем имя хоста TLS
	tls.serverNameIndication(cts, host);
	// Создаём идентификатор транспортного уровня TLS
	const tls_t::id_t ctl = tls.transport(cts);
	// Устанавливаем идентификатор события клиента
	client.setEventId(eid);
	// Устанавливаем идентификатор TLS для клиента
	client.setSecurityId(ctl);
	// Устанавливаем порт клиента
	client.setPort(443);
	// Устанавливаем целевой хост для клиента
	client.setTarget(host);
	// Регистрируем функцию обратного вызова на событие записи данных клиентом
	client.on <void (const event::id_t, const size_t)> ("write", &Executor::write, &executor, _1, _2);
	// Регистрируем функцию обратного вызова на событие изменения состояния клиента
	client.on <void (const event::id_t, const event::status_t)> ("state", &Executor::state, &executor, _1, _2);
	// Регистрируем функцию обратного вызова на событие подключения клиента к удалённому серверу
	client.on <void (const event::id_t, const bool)> ("connect", &Executor::connect, &executor, _1, _2, &client);
	// Регистрируем функцию обратного вызова на событие чтения данных клиентом
	client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &Executor::read, &executor, _1, _2, _3, &client);
	// Регистрируем функцию обратного вызова на событие ошибок клиента
	client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2, _3);
	// Регистрируем функцию обратного вызова на событие ошибок транспортного уровня безопасности TLS
	client.on <void (const tls_t::id_t, const tls_t::error_t, const string &)> ("errorTLS", &Executor::errorTLS, &executor, _1, _2, _3);
	// Регистрируем функцию обратного вызова на событие готовности клиента к работе
	client.on <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3, _4, &client);
	// Запускаем событие клиента
	client.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
