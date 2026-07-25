/**
 * @file: sctp.cpp
 * @date: 2026-05-20
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
		void read(const uint8_t * data, const size_t size, client_t * client) noexcept {
			// Если данные получены
			if(size > 0)
				// Записываем данные в лог
				this->_log->print("%s", log_t::flag_t::INFO, string(reinterpret_cast <const char *> (data), size).c_str());
			// Если данные не получены, то выводим сообщение об отсутствии данных
			else this->_log->print("No data received", log_t::flag_t::WARNING);
			// Останавливаем событие клиента
			client->stop();
		}
		/**
		 * @brief Метод обработки событий изменения статуса клиента
		 *
		 * @param status новый статус клиента
		 * @param client объект клиента
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
						this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
					// Если подключение выполнено, то выводим сообщение об успешном подключении клиента к удалённому серверу
					else this->_log->print("Successfully connected to remote server", log_t::flag_t::INFO);
				} break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события клиента
					this->_log->print("SCTP client destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки состояния клиента
		 *
		 * @param eid    идентификатор клиента
		 * @param status новый статус клиента
		 * @param sctp   объект SCTP
		 */
		void state(const event::id_t eid, const event::status_t status, engine::sctp_t * sctp) noexcept {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус возрождения события
				case static_cast <uint8_t> (event::status_t::REBIRTHED): {
					// Записываем в лог сообщение об возрождении события
					this->_log->print("SCTP client rebirthed: ID=%u", log_t::flag_t::INFO, eid);
					// Выполняем подписку на SCTP события
					sctp->eventsSubscribe(eid, {
						net::sctp::event_type_t::ASSOC_CHANGE,
						net::sctp::event_type_t::SHUTDOWN_EVENT,
						net::sctp::event_type_t::SEND_FAILED_EVENT,
						net::sctp::event_type_t::REMOTE_ERROR
					});
				} break;
			}
		}
		/**
		 * @brief Метод обработки событий подключения клиента к удалённому серверу
		 *
		 * @param ok     результат подключения
		 * @param client объект клиента
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
					this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
			// Если подключение не выполнено, то выводим сообщение об ошибке подключения клиента к удалённому серверу
			} else this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
		}
		/**
		 * @brief Метод получения событий SCTP
		 *
		 * @param eid   идентификатор клиента
		 * @param event событие SCTP
		 * @param sctp  объект SCTP
		 */
		void sctp([[maybe_unused]] const event::id_t eid, net::sctp_event_t event, engine::sctp_t * sctp) noexcept {
			// Записываем в лог сообщение с идентификатором событий SCTP
			cout << " SCTP EVENT ID: " << event->id << endl;
			/**
			 * Определяем тип события SCTP
			 */
			switch(static_cast <uint8_t> (event->type)){
				// Если требуется уведомление о каждом входящем DATA-пакете
				case static_cast <uint8_t> (net::sctp::event_type_t::DATA_IO):
					// Записываем в лог сообщение о событии DATA IO
					cout << "  - DATA IO EVENT " << endl;
				break;
				// Если ошибка удалённого узла
				case static_cast <uint8_t> (net::sctp::event_type_t::REMOTE_ERROR):
					// Записываем в лог сообщение о событии REMOTE ERROR
					cout << "  - REMOTE ERROR EVENT " << endl;
				break;
				// Если изменение ассоциации
				case static_cast <uint8_t> (net::sctp::event_type_t::ASSOC_CHANGE):
					// Записываем в лог сообщение о событии ASSOC CHANGE
					cout << "  - ASSOC CHANGE EVENT " << endl;
				break;
				// Если событие завершения работы
				case static_cast <uint8_t> (net::sctp::event_type_t::SHUTDOWN_EVENT):
					// Записываем в лог сообщение о событии SHUTDOWN EVENT
					cout << "  - SHUTDOWN EVENT " << endl;
				break;
				// Если событие "отправитель сухой"
				case static_cast <uint8_t> (net::sctp::event_type_t::SENDER_DRY_EVENT):
					// Записываем в лог сообщение о событии SENDER DRY EVENT
					cout << "  - SENDER DRY EVENT " << endl;
				break;
				// Если изменение адреса однорангового узла
				case static_cast <uint8_t> (net::sctp::event_type_t::PEER_ADDR_CHANGE):
					// Записываем в лог сообщение о событии PEER ADDR CHANGE
					cout << "  - PEER ADDR CHANGE EVENT " << endl;
				break;
				// Если событие ошибки отправки
				case static_cast <uint8_t> (net::sctp::event_type_t::SEND_FAILED_EVENT):
					// Записываем в лог сообщение о событии SEND FAILED EVENT
					cout << "  - SEND FAILED EVENT " << endl;
				break;
				// Если событие сброса потока
				case static_cast <uint8_t> (net::sctp::event_type_t::STREAM_RESET_EVENT):
					// Записываем в лог сообщение о событии STREAM RESET EVENT
					cout << "  - STREAM RESET EVENT " << endl;
				break;
				// Если событие аутентификации
				case static_cast <uint8_t> (net::sctp::event_type_t::AUTHENTICATION_EVENT):
					// Записываем в лог сообщение о событии AUTHENTICATION EVENT
					cout << "  - AUTHENTICATION EVENT " << endl;
				break;
				// Если событие адаптационное указание
				case static_cast <uint8_t> (net::sctp::event_type_t::ADAPTATION_INDICATION):
					// Записываем в лог сообщение о событии ADAPTATION INDICATION
					cout << "  - ADAPTATION INDICATION EVENT " << endl;
				break;
				// Если событие частичной доставки
				case static_cast <uint8_t> (net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
					// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
					cout << "  - PARTIAL DELIVERY EVENT " << endl;
				break;
			}
			// Получаем статус SCTP-сокета
			const net::sctp::status_t & status = sctp->status(eid);
			// Возвращаем статус SCTP-сокета
			cout << " SCTP Status: " << endl;
			cout << "  - ID: " << status.id << endl;
			cout << "  - State: " << (u_short) status.state << endl;
			cout << "  - Outbound Streams: " << status.ostreams << endl;
			cout << "  - Inbound Streams: " << status.istreams << endl;
			cout << "  - Fragmentation Point: " << status.fragpoint << endl;
			cout << "  - Rate Window: " << status.ratewind << endl;
			cout << "  - Unpack Data: " << status.unackdata << endl;
			cout << "  - Pending Data: " << status.penddata << endl;
		}
		/**
		 * @brief Метод обработки событий получения информации о сообщении SCTP-сокета
		 *
		 * @param eid   идентификатор клиента
		 * @param minfo информация о сообщении SCTP-сокета
		 */
		void minfo(const event::id_t eid, const net::sctp::minfo_t & minfo) noexcept {
			// Записываем в лог информацию о сообщении SCTP-сокета
			this->_log->print(
				"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
				log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
			);
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
		/**
		 * @brief Метод обработки ошибок транспортного уровня безопасности TLS
		 *
		 * @param error   код ошибки TLS
		 * @param message сообщение об ошибке TLS
		 */
		void errorTLS([[maybe_unused]] const tls::coder_t::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог TLS
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
	// Создаём объект SCTP протокола
	engine::sctp_t sctp(&fmk, &log);
	// Создаём объект исполнителя для обработки событий клиента
	Executor executor(&fmk, &log);
	// Создаём объект транспортного уровня безопасности
	tls::coder_t tls(&fmk, &log);
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Регистрируем объект транспортного уровня безопасности
	const tls::coder_t::id_t cts = tls.context(event::node_t::CLIENT, event::protocol_t::TCP);
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
	client_t client(tls.transport(cts), &tls, &dns, &fmk, &log);
	// Устанавливаем список поддерживаемых DNS-серверов
	dns.setServers({"77.88.8.8", "77.88.8.1"});
	// Создаём событие клиента и сохраняем его идентификатор
	const event::id_t eid = client.init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::SCTP);
	// Устананавливаем опции события
	if(client.setOptions(event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set event options!" << endl;
	// Выполняем подписку на SCTP события
	sctp.eventsSubscribe(eid, {
		net::sctp::event_type_t::ASSOC_CHANGE,
		net::sctp::event_type_t::SHUTDOWN_EVENT,
		net::sctp::event_type_t::SEND_FAILED_EVENT,
		net::sctp::event_type_t::REMOTE_ERROR
	});
	// Устанавливаем порт и целевой хост для клиента
	if(client.setTarget("localhost") && client.setTargetPort(3333)){
		// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
		sctp.on(eid, static_cast <engine::callback::sctp::minfo_t> (std::bind(&Executor::minfo, &executor, _1, _2)));
		// Устанавливаем функцию обратного вызова на создание события
		sctp.on(eid, static_cast <engine::callback::sctp::events_t> (std::bind(&Executor::sctp, &executor, _1, _2, &sctp)));
		// Устанавливаем таймаут клиента на чтение данных 6 секунд
		client.setTimeout(event::action_t::READ, 6000);
		// Регистрируем функцию обратного вызова на событие изменения статуса клиента
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие записи данных клиентом
		client.on <void (const size_t)> ("write", &Executor::write, &executor, _1);
		// Регистрируем функцию обратного вызова на событие подключения клиента к удалённому серверу
		client.on <void (const bool)> ("connect", &Executor::connect, &executor, _1, &client);
		// Регистрируем функцию обратного вызова получения состояния клиента
		client.on <void (const event::status_t)> ("state", &Executor::state, &executor, eid, _1, _2, &sctp);
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
