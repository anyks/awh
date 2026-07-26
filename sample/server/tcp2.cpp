/**
 * @file: tcp2.cpp
 * @date: 2026-05-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример сервера TCP в режиме кластера — демонстрация запуска нескольких рабочих процессов,
 *        разрешения имени хоста через DNS-резолвер и обслуживания подключений каждым воркером
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <server/server.hpp>

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
		 * @brief Метод обработки событий записи данных клиентом
		 *
		 * @param eid  идентификатор клиента
		 * @param size размер данных для записи
		 *
		 */
		void write(const event::id_t eid, const size_t size, [[maybe_unused]] void * ctx) noexcept {
			// Записываем в лог информацию о событии записи данных клиентом
			this->_log->print("Client write event: %zu bytes", log_t::flag_t::INFO, size);
		}
		/**
		 * @brief Метод обработки событий чтения данных клиентом
		 *
		 * @param eid    идентификатор клиента
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param server объект сервера
		 *
		 */
		void read([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size, [[maybe_unused]] void * ctx, server_t * server) noexcept {
			// Если данные получены
			if(size > 0)
				// Записываем данные в лог
				this->_log->print("PID=%zu\n%s", log_t::flag_t::INFO, ::getpid(), string(reinterpret_cast <const char *> (data), size).c_str());
			// Если данные не получены, то выводим сообщение об отсутствии данных
			else this->_log->print("No data received", log_t::flag_t::WARNING);
			// Отправляем данные обратно клиенту
			if(server->send(eid, data, size) == 0)
				// Записываем ошибку в лог отправки данных клиентом на сервер
				this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
		}
		/**
		 * @brief Метод обработки событий изменения статуса сервера
		 *
		 * @param status новый статус сервера
		 * @param server объект сервера
		 *
		 */
		void status(const event::status_t status, server_t * server) noexcept {
			/**
			 * Определяем состояние сервера
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие сервера запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Выполняем прослушивание сервера на порту
					if(!server->listen(100))
						// Записываем ошибку в лог
						this->_log->print("Failed to listen on port %d", log_t::flag_t::WARNING, server->getPort());
					// Если подключение выполнено, то выводим сообщение об успешном прослушивании порта
					else this->_log->print("Successfully listening on port %d", log_t::flag_t::INFO, server->getPort());
				} break;
				// Если событие сервера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события сервера
					this->_log->print("Server destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий подключения клиента к серверу
		 *
		 * @param eid    идентификатор сервера
		 * @param cid    идентификатор клиента
		 * @param tid    идентификатор клиента TLS
		 * @param server объект сервера
		 *
		 */
		void accept([[maybe_unused]] const event::id_t eid, const event::id_t cid, const tls::coder_t::id_t tid, server_t * server) noexcept {
			// Устананавливаем опции события
			if(server->setOptions(cid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY | event::options::KEEPALIVE))
				// Записываем в лог сообщение об успешной установке опций события
				cout << " Connection established: " << server->getAddress(cid, event::address_t::IPV4) << ":" << server->getPort(cid) << ", MAC: " << server->getAddress(cid, event::address_t::MAC) << endl;
		}
		/**
		 * @brief Метод обработки событий готовности сервера к работе
		 *
		 * @param eid    идентификатор сервера
		 * @param family семейство адресов сервера
		 * @param domain доменное имя сервера
		 * @param ip     IP-адрес сервера
		 *
		 */
		void ready([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::family_t family, const string & domain, const string & ip) noexcept {
			// Записываем в лог сообщение о готовности сервера к работе
			this->_log->print("Server is ready to accept connections: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки ошибок сервера
		 *
		 * @param eid    идентификатор сервера
		 * @param error  код ошибки
		 * @param message сообщение об ошибке
		 *
		 */
		void error([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & message, [[maybe_unused]] void * ctx) noexcept {
			// Записываем ошибку в лог
			this->_log->print("Server error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 *
		 */
		Executor(const fmk_t * fmk, const log_t * log) : _fmk(fmk), _log(log) {}
};

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект исполнителя для обработки событий сервера
	Executor executor(&fmk, &log);
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Создаём объект сервера
	server_t server(&dns, &fmk, &log);
	// Устанавливаем список поддерживаемых DNS-серверов
	dns.setServers({"77.88.8.8", "77.88.8.1"});
	// Устанавливаем имя кластера для сервера
	server.clusterName("ANYKS");
	// Устанавливаем количество вокеров кластера для сервера
	server.clusterCount(0);
	// Включаем режим кластера для сервера
	server.clusterMode(event::mode_t::ENABLED);
	// Создаём событие сервера и сохраняем его идентификатор
	const event::id_t eid = server.init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Устананавливаем опции события
	if(server.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set event options!" << endl;
	// Устанавливаем порт и хост сервера
	if(server.setPort(2222) && server.setHost("localhost")){
		// Устанавливаем таймаут сервера на чтение данных 6 секунд
		server.setTimeout(event::action_t::READ, 6000);
		// Регистрируем функцию обратного вызова на событие изменения статуса сервера
		server.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &server);
		// Регистрируем функцию обратного вызова на событие записи данных сервером
		server.on <void (const event::id_t, const size_t, void *)> ("write", &Executor::write, &executor, _1, _2, _3);
		// Регистрируем функцию обратного вызова на событие чтения данных сервером
		server.on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &Executor::read, &executor, _1, _2, _3, _4, &server);
		// Регистрируем функцию обратного вызова на событие ошибок сервера
		server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &Executor::error, &executor, _1, _2, _3, _4);
		// Регистрируем функцию обратного вызова на событие готовности сервера к работе
		server.on <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3, _4);
		// Регистрируем функцию обратного вызова на событие подключения клиента к серверу
		server.on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Executor::accept, &executor, _1, _2, _3, &server);
		// Запускаем событие сервера
		server.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
