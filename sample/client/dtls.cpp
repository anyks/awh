/**
 * @file dtls.cpp
 * @date 2026-05-20
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
 * @brief Пример клиента DTLS — демонстрация установки защищённого датаграммного соединения через фасад клиента,
 *        подписки на события подключения, чтения и записи и обмена данными с удалённым сервером
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <client/client.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

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
	public:
		/**
		 * @brief Метод обработки событий записи данных клиентом
		 *
		 * @param size размер данных для записи
		 *
		 */
		void write(const size_t size) noexcept {
			// Записываем в лог информацию о событии записи данных клиентом
			awh::log::print("Client write event: %zu bytes", awh::log::flag_t::INFO, size);
		}
		/**
		 * @brief Метод обработки событий чтения данных клиентом
		 *
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param client объект клиента
		 *
		 */
		void read(const uint8_t * data, const size_t size, client_t * client) noexcept {
			// Если данные получены
			if(size > 0)
				// Записываем данные в лог
				awh::log::print("%s", awh::log::flag_t::INFO, string(reinterpret_cast <const char *> (data), size).c_str());
			// Если данные не получены, то выводим сообщение об отсутствии данных
			else awh::log::print("No data received", awh::log::flag_t::WARNING);
			// Останавливаем событие клиента
			client->stop();
		}
		/**
		 * @brief Метод обработки событий изменения статуса клиента
		 *
		 * @param status новый статус клиента
		 * @param client объект клиента
		 *
		 */
		void status(const event::status_t status, client_t * client) noexcept {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Выполняем подключение клиента к удалённому серверу
					if(!client->connect())
						// Записываем ошибку в лог
						awh::log::print("Failed to connect to remote server", awh::log::flag_t::WARNING);
					// Если подключение выполнено, то выводим сообщение об успешном подключении клиента к удалённому серверу
					else awh::log::print("Successfully connected to remote server", awh::log::flag_t::INFO);
				} break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события клиента
					awh::log::print("DTLS client destroyed", awh::log::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий подключения клиента к удалённому серверу
		 *
		 * @param ok     результат подключения
		 * @param client объект клиента
		 *
		 */
		void connect(const bool ok, client_t * client) noexcept {
			// Если подключение выполнено
			if(ok){
				// Текст запроса к серверу
				const string request =
					"GET / HTTP/1.1\r\n"
					"Host: anyks.com\r\n"
					"Connection: close\r\n"
					"User-Agent: iouring-opentls-sample/1.0\r\n"
					"\r\n";
				// Если отправка данных данных клиентом на сервер не выполнена
				if(client->send(request.c_str(), request.size()) == 0)
					// Записываем ошибку в лог отправки данных клиентом на сервер
					awh::log::print("Failed to send data to remote server", awh::log::flag_t::WARNING);
			// Если подключение не выполнено, то выводим сообщение об ошибке подключения клиента к удалённому серверу
			} else awh::log::print("Failed to connect to remote server", awh::log::flag_t::WARNING);
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
			awh::log::print("Client is ready to connect to remote server: %s (%s)", awh::log::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки ошибок клиента
		 *
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 *
		 */
		void error([[maybe_unused]] const event::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог
			awh::log::print("Client error: %s", awh::log::flag_t::CRITICAL, message.c_str());
		}
		/**
		 * @brief Метод обработки ошибок транспортного уровня безопасности TLS
		 *
		 * @param error   код ошибки TLS
		 * @param message сообщение об ошибке TLS
		 *
		 */
		void errorTLS([[maybe_unused]] const tls::coder_t::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог TLS
			awh::log::print("TLS error: %s", awh::log::flag_t::CRITICAL, message.c_str());
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 */
		Executor() {}
};

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Создаём объект исполнителя для обработки событий клиента
	Executor executor;
	// Создаём объект транспортного уровня безопасности
	tls::coder_t tls;
	// Регистрируем объект транспортного уровня безопасности
	const tls::coder_t::id_t cts = tls.context(event::node_t::CLIENT, event::protocol_t::UDP);
	// Устанавливаем ALPN протоколы TLS
	tls.alpn(cts, {
		{0,"http/1.1"},
		{1,"h2"}
	});
	// Устанавливаем список доступных шифров TLS
	tls.ciphers(cts, {
		tls::cipher_t::TLS_AES_128_GCM_SHA256,
		tls::cipher_t::TLS_AES_256_GCM_SHA384,
		tls::cipher_t::TLS_CHACHA20_POLY1305_SHA256,
		tls::cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256,
		tls::cipher_t::ECDHE_RSA_AES128_GCM_SHA256,
		tls::cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384,
		tls::cipher_t::ECDHE_RSA_AES256_GCM_SHA384,
		tls::cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305,
		tls::cipher_t::ECDHE_RSA_CHACHA20_POLY1305,
		tls::cipher_t::ECDHE_RSA_AES128_SHA,
		tls::cipher_t::ECDHE_RSA_AES256_SHA,
		tls::cipher_t::AES128_GCM_SHA256,
		tls::cipher_t::AES256_GCM_SHA384,
		tls::cipher_t::AES128_SHA,
		tls::cipher_t::AES256_SHA
	});
	// Устанавливаем компрессор для транспортного уровня TLS
	tls.compressors(cts, {
		compressor::method_t::ZLIB,
		compressor::method_t::ZSTD,
		compressor::method_t::BROTLI
	});
	// Устанавливаем файл центра сертификации TLS
	tls.ca(cts, "../sh/certificates", "ca.pem");
	// Устанавливаем имя хоста TLS
	tls.serverNameIndication(cts, "anyks.com");
	// Включаем проверку имени хоста TLS
	tls.validateServerNameIndication(cts, false);
	// Создаём объект клиента
	client_t client(tls.transport(cts), &tls);
	// Создаём событие клиента и сохраняем его идентификатор
	[[maybe_unused]] const event::id_t eid = client.init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Устананавливаем опции события
	if(client.setOptions(event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set event options!" << endl;
	// Устанавливаем порт и целевой хост для клиента
	if(client.setTarget("127.0.0.1") && client.setTargetPort(2222)){
		// Устанавливаем таймаут клиента на чтение данных 6 секунд
		client.setTimeout(event::action_t::READ, 6000);
		// Регистрируем функцию обратного вызова на событие изменения статуса клиента
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие записи данных клиентом
		client.on <void (const size_t)> ("write", &Executor::write, &executor, _1);
		// Регистрируем функцию обратного вызова на событие подключения клиента к удалённому серверу
		client.on <void (const bool)> ("connect", &Executor::connect, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие чтения данных клиентом
		client.on <void (const uint8_t *, const size_t)> ("read", &Executor::read, &executor, _1, _2, &client);
		// Регистрируем функцию обратного вызова на событие ошибок клиента
		client.on <void (const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие ошибок транспортного уровня безопасности TLS
		client.on <void (const tls::coder_t::error_t, const string &)> ("error_tls", &Executor::errorTLS, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие готовности клиента к работе
		client.on <void (const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3);
		// Запускаем событие клиента
		client.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
