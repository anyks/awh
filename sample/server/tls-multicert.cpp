/**
 * @file tls-multicert.cpp
 * @date 2026-05-18
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
 * @brief Пример сервера TLS с несколькими сертификатами — демонстрация выбора сертификата по имени,
 *        запрошенному клиентом в расширении SNI, через фасад сервера
 *
 * @copyright Copyright © 2026
 *
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
		 * @param tls  объект TLS кодера
		 *
		 */
		void accept([[maybe_unused]] const event::id_t eid, const event::id_t cid, const tls::coder_t::id_t tid, server_t * server, tls::coder_t * tls) noexcept {
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Connection established: " << server->getAddress(cid, event::address_t::IPV4) << ":" << server->getPort(cid) << ", MAC: " << server->getAddress(cid, event::address_t::MAC) << endl;
			// Устананавливаем опции события
			if(server->setOptions(cid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY | event::options::AUTO_RECONNECT)){
				// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
				cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << tls->info(tid) << endl;
				cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << (u_short) tls->alpn(tid) << endl;
				cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << tls->serverNameIndication(tid) << endl << endl;
				cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
				cout << "OpenSSL version: " << tls->version() << endl << endl;
				cout << "Cipher: " << tls->cipherInfo(tid) << endl << endl;
				cout << "Certificate: " << tls->certificateInfo(tid) << endl << endl;
				cout << "CRL Info: " << tls->certificateRevocationListInfo(tid) << endl << endl;
				cout << "Certificate Validation: " << (tls->validateCertificate(tid) ? "Valid" : "Invalid") << endl << endl;
				// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
				this->_log->print("TLS handshake complete: ID=%" PRIu64 ", ALPN protocol=%d", log_t::flag_t::INFO, tid, tls->alpn(tid));
			}
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
		/**
		 * @brief Метод обработки ошибок транспортного уровня безопасности TLS
		 *
		 * @param id      идентификатор TLS
		 * @param error   код ошибки TLS
		 * @param message сообщение об ошибке TLS
		 *
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
		 *
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
	// Создаём объект отпечатка браузера
	tls::fgp_t fgp(&fmk, &log);
	// Создаём объект транспортного уровня безопасности
	tls::coder_t tls(&fgp, &fmk, &log);
	// Создаём объект сервера
	// server_t server(&tls, &fmk, &log);
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Регистрируем объект транспортного уровня безопасности для базового шаблона TLS
	const tls::coder_t::id_t cts1 = tls.context(event::node_t::SERVER, event::protocol_t::TCP);
	// Создаём объект сервера
	server_t server(cts1, &tls, &dns, &fmk, &log);
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
	// Регистрируем объект транспортного уровня безопасности для шаблона TLS с точным доменным именем
	const tls::coder_t::id_t cts2 = tls.context(event::node_t::SERVER, event::protocol_t::TCP);
	// Устанавливаем режим работы TLS для базового шаблона TLS
	tls.mode(cts1, tls::coder_t::mode_t::MULTICERT);
	// Устанавливаем режим работы TLS для шаблона TLS с точным доменным именем
	tls.mode(cts2, tls::coder_t::mode_t::MULTICERT);
	// Устанавливаем ALPN протоколы TLS базового шаблона TLS
	tls.alpn(cts1, {
		{0,"h2"},
		{1,"http/1.1"}
	});
	// Устанавливаем ALPN протоколы TLS для шаблона TLS с точным доменным именем
	tls.alpn(cts2, {
		{0,"h2"},
		{1,"h3"},
		{2,"http/1.1"}
	});
	// Устанавливаем файл центра сертификации TLS для базового шаблона TLS
	tls.ca(cts1, "../sh/certificates", "ca.pem");
	// Устанавливаем файл центра сертификации TLS для шаблона TLS с точным доменным именем
	tls.ca(cts2, "../sh/certificates", "ca.pem");
	// Устанавливаем имя хоста TLS (Указывать нужно после установки режима работы мультисертификатного TLS!!!!!!!)
	tls.serverNameIndication(cts2, "anyks.com");
	// Устанавливаем клиентский сертификат TLS для базового шаблона TLS
	tls.certificate(cts1, "../sh/certificates/example/cert.pem");
	// Устанавливаем приватный ключ TLS для базового шаблона TLS
	tls.privateKey(cts1, "../sh/certificates/example/key.pem");
	// Устанавливаем клиентский сертификат TLS для шаблона TLS с точным доменным именем
	tls.certificate(cts2, "../sh/certificates/server/cert.pem");
	// Устанавливаем приватный ключ TLS для шаблона TLS с точным доменным именем
	tls.privateKey(cts2, "../sh/certificates/server/key.pem");
	// Включаем проверку имени хоста TLS для базового шаблона TLS
	tls.validateServerNameIndication(cts1, false);
	// Включаем проверку имени хоста TLS для шаблона TLS с точным доменным именем
	tls.validateServerNameIndication(cts2, false);
	// Устанавливаем список доступных шифров TLS для базового шаблона TLS
	tls.ciphers(cts1, {
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
	// Устанавливаем список доступных шифров TLS для шаблона TLS с точным доменным именем
	tls.ciphers(cts2, {
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
	// Устанавливаем компрессор для транспортного уровня TLS для базового шаблона TLS
	tls.compressors(cts1, {
		compressor::method_t::ZLIB,
		compressor::method_t::ZSTD,
		compressor::method_t::BROTLI
	});
	// Устанавливаем компрессор для транспортного уровня TLS для шаблона TLS с точным доменным именем
	tls.compressors(cts2, {
		compressor::method_t::ZLIB,
		compressor::method_t::ZSTD,
		compressor::method_t::BROTLI
	});
	// Устанавливаем порт и хост сервера
	// if(server.setPort(2222) && server.setHost("127.0.0.1")){
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
		server.on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Executor::accept, &executor, _1, _2, _3, &server, &tls);
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
