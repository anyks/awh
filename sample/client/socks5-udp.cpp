/**
 * @file: socks5-udp.cpp
 * @date: 2026-05-28
 * @license: LicenseRef-AWH-1.0
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
#include <client/socks5.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Класс объекта исполнителя
 *
 */
class Executor {
	private:
		// Объект фреймворка
		const fmk_t * _fmk;
		// Объект работы с логами
		const log_t * _log;
	public:
		/**
		 * @brief Метод обработки события запуска клиента
		 *
		 * @param host адрес хоста
		 * @param port порт хоста
		 */
		void launch(const string & host, const uint16_t port) noexcept {
			// Записываем в лог информацию о событии запуска клиента
			this->_log->print("Launched socks5 client (Host=%s, Port=%u)", log_t::flag_t::INFO, host.c_str(), port);
		}
		/**
		 * @brief Метод обработки событий записи данных клиентом
		 *
		 * @param size размер данных для записи
		 */
		void write(const size_t size) noexcept {
			// Записываем в лог информацию о событии записи данных клиентом
			this->_log->print("Client write event: %zu bytes", log_t::flag_t::INFO, size);
		}
		/**
		 * @brief Метод обработки событий чтения данных клиентом
		 *
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param client объект клиента
		 */
		void read(const uint8_t * data, const size_t size, client::socks5_t * client) noexcept {
			// Записываем в лог информацию о событии получения данных клиентом
			this->_log->print("DNS data response size=%zu", log_t::flag_t::INFO, size);
			// Останавливаем событие клиента
			client->stop();
		}
		/**
		 * @brief Метод обработки событий изменения статуса клиента
		 *
		 * @param status новый статус клиента
		 * @param client объект клиента
		 */
		void status(const event::status_t status, client::socks5_t * client) noexcept {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Если подключение выполнено, то выводим сообщение об успешном подключении клиента к удалённому серверу
					this->_log->print("Successfully launched socks5 client", log_t::flag_t::INFO);
				break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события клиента
					this->_log->print("Socks5 client destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий подключения клиента к удалённому серверу
		 *
		 * @param ok     результат подключения
		 * @param client объект клиента
		 */
		void connect(const bool ok, client::socks5_t * client) noexcept {
			// Если подключение выполнено
			if(ok){
				/**
				 * DNS Query: anyks.com, Type=A, Class=IN
				 * Размер: 31 байт
				 */
				constexpr uint8_t request[] = {
					// === DNS Header (12 bytes) ===
					0x12, 0x34,   // Transaction ID (любое, для сопоставления ответа)
					0x01, 0x00,   // Flags: 0x0100 = Standard query, Recursion Desired
					0x00, 0x01,   // QDCOUNT: 1 вопрос
					0x00, 0x00,   // ANCOUNT: 0 ответов
					0x00, 0x00,   // NSCOUNT: 0 записей
					0x00, 0x00, // ARCOUNT: 0 дополнительных

					// === Question Section (19 bytes) ===
					// QNAME: anyks.com в DNS label-формате
					0x05, 'a', 'n', 'y', 'k', 's', // длина 5 + "anyks"
					0x03, 'c', 'o', 'm',                     // длина 3 + "com"
					0x00,                                                   // корневая метка (конец домена)
					
					// QTYPE: A record = 1
					0x00, 0x01,
					// QCLASS: IN = 1
					0x00, 0x01
				};
				// Если отправка данных данных клиентом на сервер не выполнена
				if(client->send(request, sizeof(request)) == 0)
					// Записываем ошибку в лог отправки данных клиентом на сервер
					this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
				// Если отправка данных клиентом на сервер выполнена, то выводим сообщение об успешной отправке данных клиентом на сервер
				else this->_log->print("Sent DNS query to remote server", log_t::flag_t::INFO);
			// Если подключение не выполнено, то выводим сообщение об ошибке подключения клиента к удалённому серверу
			} else this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
		}
		/**
		 * @brief Метод обработки событий готовности клиента к работе
		 *
		 * @param family семейство адресов клиента
		 * @param domain доменное имя клиента
		 * @param ip     IP-адрес клиента
		 */
		void ready([[maybe_unused]] const event::family_t family, const string & domain, const string & ip) noexcept {
			// Записываем в лог сообщение о готовности клиента к работе
			this->_log->print("Client is ready to connect to remote server: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки ошибок клиента
		 *
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 */
		void error([[maybe_unused]] const event::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог
			this->_log->print("Client error: %s", log_t::flag_t::CRITICAL, message.c_str());
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
	// Создаём объект исполнителя для обработки событий клиента
	Executor executor(&fmk, &log);
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Создаём объект клиента
	client::socks5_t client(&dns, &fmk, &log);
	// Устанавливаем список поддерживаемых DNS-серверов
	dns.setServers({"77.88.8.8", "77.88.8.1"});
	// Создаём событие клиента и сохраняем его идентификатор
	const event::id_t eid = client.init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Устананавливаем опции события
	if(client.setOptions(event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set event options!" << endl;
	// Устанавливаем порт и целевой хост для клиента socks5 и добавляем идентификатор события клиента для конечной точки
	// if(client.setTargetPort(11613) && client.setTarget("217.29.53.105") && client.endpoint("77.88.8.8", 53) && client.udp("0.0.0.0")){
	// if(client.setTargetPort(2222) && client.setTarget("localhost") && client.endpoint("77.88.8.8", 53) && client.udp("0.0.0.0")){
	if(client.setTargetPort(2222) && client.setTarget("localhost") && client.endpoint("dns.yandex", 53) && client.udp("0.0.0.0")){
		// Устанавливаем параметры авторизации для клиента
		client.setUser("8J0sHd", "G4DfSK");
		// Устанавливаем таймаут клиента на чтение данных 6 секунд
		client.setTimeout(event::action_t::READ, 6000);
		// Регистрируем функцию обратного вызова на событие изменения статуса клиента
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие записи данных клиентом
		client.on <void (const size_t)> ("write", &Executor::write, &executor, _1);
		// Регистрируем функцию обратного вызова на событие подключения клиента к удалённому серверу
		client.on <void (const bool)> ("connect", &Executor::connect, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие запуска клиента
		client.on <void (const string &, const uint16_t)> ("launch", &Executor::launch, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие ошибок клиента
		client.on <void (const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие чтения данных клиентом
		client.on <void (const uint8_t *, const size_t)> ("read", &Executor::read, &executor, _1, _2, &client);
		// Регистрируем функцию обратного вызова на событие готовности клиента к работе
		client.on <void (const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3);
		// Запускаем событие клиента
		client.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
