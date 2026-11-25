/**
 * @file httpproxy-forward.cpp
 * @date 2026-11-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Пример клиента, работающего через HTTP-прокси в режиме пересылки — демонстрация
 *        пересылки обычного HTTP-запроса в absolute-form и получения ответа через прокси-сервер
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <sys/fmk.hpp>
#include <client/http/proxy.hpp>

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
	public:
		/**
		 * @brief Метод обработки событий записи данных клиентом
		 *
		 * @param size размер данных для записи
		 *
		 */
		void write(const size_t size) noexcept {
			// Записываем в лог информацию о событии записи данных клиентом
			log::print("Client write event: %zu bytes", log::flag_t::INFO, size);
		}
		/**
		 * @brief Метод обработки событий чтения данных клиентом
		 *
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param client объект клиента
		 *
		 */
		void read(const uint8_t * data, const size_t size, client::http_proxy_t * client) noexcept {
			// Если данные получены
			if(size > 0)
				// Записываем данные в лог
				log::print("%s", log::flag_t::INFO, string(reinterpret_cast <const char *> (data), size).c_str());
			// Если данные не получены, то выводим сообщение об отсутствии данных
			else log::print("No data received", log::flag_t::WARNING);
			// Останавливаем событие клиента
			client->stop();
		}
		/**
		 * @brief Метод обработки событий готовности головы ответа прокси
		 *
		 * @param code    код состояния ответа
		 * @param headers заголовки ответа
		 *
		 */
		void response(const uint16_t code, const vector <pair <string, string>> & headers) noexcept {
			// Записываем в лог код состояния ответа
			log::print("Response status: %u", log::flag_t::INFO, code);
			/**
			 * Перебираем заголовки ответа
			 */
			for(const auto & header : headers){
				// Записываем заголовок ответа в лог
				log::print("Response header: %s: %s", log::flag_t::INFO, header.first.c_str(), header.second.c_str());
			}
		}
		/**
		 * @brief Метод обработки событий изменения статуса клиента
		 *
		 * @param status новый статус клиента
		 * @param client объект клиента
		 *
		 */
		void status(const event::status_t status, [[maybe_unused]] client::http_proxy_t * client) noexcept {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Если подключение выполнено, то выводим сообщение об успешном подключении клиента к прокси-серверу
					log::print("Successfully launched HTTP proxy client", log::flag_t::INFO);
				break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события клиента
					log::print("HTTP proxy client destroyed", log::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий подключения клиента к прокси-серверу
		 *
		 * @param ok     результат подключения
		 * @param client объект клиента
		 *
		 */
		void connect(const bool ok, client::http_proxy_t * client) noexcept {
			// Если подключение выполнено
			if(ok){
				// Текст запроса пользователя в origin-form
				const string request =
					"GET / HTTP/1.1\r\n"
					"Host: anyks.com\r\n"
					"User-Agent: awh-httpproxy-sample/1.0\r\n"
					"\r\n";
				// Если отправка запроса на пересылку не выполнена
				if(client->send(request.c_str(), request.size()) == 0)
					// Записываем ошибку в лог отправки запроса на пересылку
					log::print("Failed to send the request for forwarding", log::flag_t::WARNING);
				// Если отправка запроса на пересылку выполнена, то выводим сообщение об успешной отправке
				else log::print("Sent the request for forwarding through the proxy", log::flag_t::INFO);
			// Если подключение не выполнено, то выводим сообщение об ошибке подключения к прокси-серверу
			} else log::print("Failed to connect to the proxy server", log::flag_t::WARNING);
		}
		/**
		 * @brief Метод обработки событий готовности клиента к работе
		 *
		 * @param family семейство адресов клиента
		 * @param domain доменное имя клиента
		 * @param ip     IP-адрес клиента
		 *
		 */
		void ready([[maybe_unused]] const event::family_t family, const string & domain, const string & ip) noexcept {
			// Записываем в лог сообщение о готовности клиента к работе
			log::print("Client is ready to connect to the proxy server: %s (%s)", log::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки ошибок клиента
		 *
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 * @param client  объект клиента
		 *
		 */
		void error([[maybe_unused]] const event::error_t error, const string & message, client::http_proxy_t * client) noexcept {
			// Записываем ошибку в лог
			log::print("Client error: %s", log::flag_t::CRITICAL, message.c_str());
			// Останавливаем событие клиента
			client->stop();
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 */
		Executor() {}
};

/**
 * @brief Функция главного входа в приложение
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	/**
	 * Заводим модуль ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 *
	 */
	fmk::initialize();
	// Создаём объект исполнителя для обработки событий клиента
	Executor executor;
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4);
	// Устанавливаем хост прокси-сервера для подключения клиента
	const string proxy = "localhost";
	// Создаём объект клиента HTTP-прокси
	client::http_proxy_t client(&dns);
	// Устанавливаем режим пересылки обычных HTTP-запросов
	client.mode(http::proxy::client_t::mode_t::FORWARD);
	// Устанавливаем список поддерживаемых DNS-серверов
	dns.setServers(vector <string> {"77.88.8.8", "77.88.8.1"});
	// Создаём событие клиента и сохраняем его идентификатор
	[[maybe_unused]] const event::id_t eid = client.init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Устанавливаем опции события
	if(client.setOptions(event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set event options!" << endl;
	// Устанавливаем порт и хост прокси-сервера
	if(client.setTargetPort(3128) && client.setTarget(proxy)){
		// Устанавливаем таймаут клиента на чтение данных 6 секунд
		client.setTimeout(event::action_t::READ, 6000);
		// Регистрируем функцию обратного вызова на событие изменения статуса клиента
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие записи данных клиентом
		client.on <void (const size_t)> ("write", &Executor::write, &executor, _1);
		// Регистрируем функцию обратного вызова на событие подключения клиента к прокси-серверу
		client.on <void (const bool)> ("connect", &Executor::connect, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие чтения данных клиентом
		client.on <void (const uint8_t *, const size_t)> ("read", &Executor::read, &executor, _1, _2, &client);
		// Регистрируем функцию обратного вызова на событие готовности головы ответа прокси
		client.on <void (const uint16_t, const vector <pair <string, string>> &)> ("response", &Executor::response, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие ошибок клиента
		client.on <void (const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2, &client);
		// Регистрируем функцию обратного вызова на событие готовности клиента к работе
		client.on <void (const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3);
		// Запускаем событие клиента
		client.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
