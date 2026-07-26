/**
 * @file: sctp.cpp
 * @date: 2026-05-20
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
 * Подключаем стандартные модули
 */
#include <cinttypes>

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
		 */
		void read([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size, [[maybe_unused]] void * ctx, server_t * server) noexcept {
			// Если данные получены
			if(size > 0)
				// Записываем данные в лог
				this->_log->print("%s", log_t::flag_t::INFO, string(reinterpret_cast <const char *> (data), size).c_str());
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
		 * @param tls  объект TLS кодера
		 * @param sctp   объект SCTP протокола
		 */
		void accept([[maybe_unused]] const event::id_t eid, const event::id_t cid, const tls::coder_t::id_t tid, server_t * server, tls::coder_t * tls, engine::sctp_t * sctp) noexcept {
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Connection established: " << server->getAddress(cid, event::address_t::IPV4) << ":" << server->getPort(cid) << ", MAC: " << server->getAddress(cid, event::address_t::MAC) << endl;
			// Устананавливаем опции события
			if(server->setOptions(cid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY | event::options::KEEPALIVE)){
				// Получаем информацию о сообщении SCTP-сокета
				const net::sctp::minfo_t & minfo = sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				cout << " SCTP Message Info1: " << endl;
				cout << "  - Stream Number: " << minfo.num << endl;
				cout << "  - Payload Protocol ID: " << (u_short) minfo.ppid << endl;
				cout << "  - Context: " << minfo.ctx << endl;
				cout << "  - Time to Live: " << minfo.ttl << endl;
				cout << "  - Flags: " << minfo.flags.size() << endl;
				// Получаем статус SCTP-сокета
				const net::sctp::status_t & status = sctp->status(cid);
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
				// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
				cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << tls->info(tid) << endl;
				cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << (u_short) tls->alpn(tid) << endl;
				cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << tls->serverNameIndication(tid) << endl << endl;
				cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
				cout << "Версия OpenSSL: " << tls->version() << endl << endl;
				cout << "Cipher: " << tls->cipherInfo(tid) << endl << endl;
				cout << "Certificate: " << tls->certificateInfo(tid) << endl << endl;
				cout << "CRL Info: " << tls->certificateRevocationListInfo(tid) << endl << endl;
				cout << "Certificate Validation: " << (tls->validateCertificate(tid) ? "Valid" : "Invalid") << endl << endl;
				// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
				this->_log->print("TLS handshake completed: ID=%" PRIu64 ", ALPN protocol=%d", log_t::flag_t::INFO, tid, tls->alpn(tid));
				// Выполняем подписку на SCTP события
				sctp->eventsSubscribe(cid, {
					net::sctp::event_type_t::ASSOC_CHANGE,
					net::sctp::event_type_t::SHUTDOWN_EVENT,
					net::sctp::event_type_t::SEND_FAILED_EVENT,
					net::sctp::event_type_t::REMOTE_ERROR
				});
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				sctp->on(cid, static_cast <engine::callback::sctp::minfo_t> (std::bind(&Executor::minfo, this, _1, _2)));
				// Устанавливаем функцию обратного вызова на создание события
				sctp->on(cid, static_cast <engine::callback::sctp::events_t> (std::bind(&Executor::sctp, this, _1, _2, sctp)));
			}
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
		 * @param eid    идентификатор клиента
		 * @param minfo  информация о сообщении SCTP-сокета
		 */
		void minfo(const event::id_t eid, const net::sctp::minfo_t & minfo) noexcept {
			// Записываем в лог информацию о сообщении SCTP-сокета
			this->_log->print(
				"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
				log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
			);
		}
		/**
		 * @brief Метод обработки событий готовности сервера к работе
		 *
		 * @param eid    идентификатор сервера
		 * @param family семейство адресов сервера
		 * @param domain доменное имя сервера
		 * @param ip     IP-адрес сервера
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
		 */
		void error([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & message, [[maybe_unused]] void * ctx) noexcept {
			// Записываем ошибку в лог
			this->_log->print("Server error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
		/**
		 * @brief Метод обработки ошибок транспортного уровня безопасности TLS
		 *
		 * @param id      идентификатор TLS
		 * @param error   код ошибки TLS
		 * @param message сообщение об ошибке TLS
		 */
		void errorTLS([[maybe_unused]] const tls::coder_t::id_t id, [[maybe_unused]] const tls::coder_t::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог TLS
			this->_log->print("TLS error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
		/**
		 * @brief Метод обработки TLS fingerprint клиента
		 *
		 * @param id      идентификатор TLS
		 * @param eid     идентификатор события сервера
		 * @param browser информация о браузере клиента
		 * @param fgp     объект отпечатка браузера
		 */
		void fingerprintTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::fgp_t::browser_t & browser, tls::fgp_t * fgp) noexcept {
			// Записываем в лог информацию о браузере клиента, который подключился к серверу
			this->_log->print("TLS fingerprint: ID=%" PRIu64 ", Event ID=%u, Browser=%s", log_t::flag_t::INFO, id, eid, fgp->print(browser).c_str());
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
	// Создаём объект исполнителя для обработки событий сервера
	Executor executor(&fmk, &log);
	// Создаём объект отпечатка браузера
	tls::fgp_t fgp(&fmk, &log);
	// Создаём объект транспортного уровня безопасности
	tls::coder_t tls(&fgp, &fmk, &log);
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Регистрируем объект транспортного уровня безопасности
	const tls::coder_t::id_t cts = tls.context(event::node_t::SERVER, event::protocol_t::TCP);
	// Создаём объект сервера
	server_t server(cts, &tls, &dns, &fmk, &log);
	// Устанавливаем список поддерживаемых DNS-серверов
	dns.setServers({"77.88.8.8", "77.88.8.1"});
	// Создаём событие сервера и сохраняем его идентификатор
	const event::id_t eid = server.init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::SCTP);
	// Устананавливаем опции события
	if(server.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
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
	// Устанавливаем ALPN протоколы TLS
	tls.alpn(cts, {
		{0,"h3"},
		{1,"http/1.1"}
	});
	// Устанавливаем файл центра сертификации TLS
	tls.ca(cts, "../sh/certificates", "ca.pem");
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
	// Включаем проверку имени хоста TLS
	tls.validateServerNameIndication(cts, false);
	// Устанавливаем клиентский сертификат TLS
	tls.certificate(cts, "../sh/certificates/server/cert.pem");
	// Устанавливаем приватный ключ TLS
	tls.privateKey(cts, "../sh/certificates/server/key.pem");
	// Устанавливаем порт и хост сервера
	if(server.setPort(3333) && server.setHost("localhost")){
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
		server.on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Executor::accept, &executor, _1, _2, _3, &server, &tls, &sctp);
		// Регистрируем функцию обратного вызова на событие ошибок транспортного уровня безопасности TLS
		server.on <void (const tls::coder_t::id_t, const tls::coder_t::error_t, const string &)> ("error_tls", &Executor::errorTLS, &executor, _1, _2, _3);
		// Регистрируем функцию обратного вызова на событие TLS fingerprint клиента
		server.on <void (const tls::coder_t::id_t, const event::id_t, const tls::fgp_t::browser_t &)> ("fingerprint_tls", &Executor::fingerprintTLS, &executor, _1, _2, _3, &fgp);
		// Запускаем событие сервера
		server.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
