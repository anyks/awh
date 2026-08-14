/**
 * @file quic.cpp
 * @date 2026-07-25
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
 * @brief Пример сервера QUIC — демонстрация приёма соединений по протоколу QUIC через фасад сервера,
 *        выполнения рукопожатия и обслуживания потоков клиентов
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
 * @brief Класс объекта исполнителя эхо-сервера QUIC
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
					// Переводим сервер в режим прослушивания входящих соединений
					if(!server->listen(100))
						// Записываем ошибку в лог
						this->_log->print("Failed to listen on port %d", log_t::flag_t::WARNING, server->getPort());
					// Если прослушивание успешно запущено
					else this->_log->print("QUIC server is listening on port %d", log_t::flag_t::INFO, server->getPort());
				} break;
				// Если событие сервера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события сервера
					this->_log->print("QUIC server destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий установленного соединения QUIC
		 *
		 * @param eid    идентификатор события сервера
		 * @param cid    идентификатор сессии соединения
		 * @param tid    идентификатор шаблона контекста безопасности
		 * @param server объект сервера
		 *
		 */
		void accept([[maybe_unused]] const event::id_t eid, const event::id_t cid, [[maybe_unused]] const tls::coder_t::id_t tid, server_t * server) noexcept {
			// Записываем в лог сообщение об установленном соединении
			this->_log->print("QUIC connection established: ID=%u, Address=%s", log_t::flag_t::INFO, cid, server->getAddress(cid, event::address_t::IPV4).c_str());
		}
		/**
		 * @brief Метод обработки собранных данных потока приложения QUIC
		 *
		 * @param cid    идентификатор сессии соединения
		 * @param sid    идентификатор потока приложения
		 * @param data   собранные данные потока
		 * @param fin    флаг завершения потока
		 * @param server объект сервера
		 *
		 */
		void stream(const event::id_t cid, const uint64_t sid, const string & data, const bool fin, server_t * server) noexcept {
			// Записываем в лог сведения о полученной части данных потока
			this->_log->print("Read: ID=%u, Stream=%" PRIu64 ", %zu bytes, FIN=%s", log_t::flag_t::INFO, cid, sid, data.size(), (fin ? "yes" : "no"));
			// Печатаем полученную от клиента полезную нагрузку, чтобы видеть её содержимое
			cout << endl << "==== SERVER RECEIVED (" << data.size() << " bytes) ====" << endl
			     << data
			     << "==== END OF RECEIVED ====" << endl;
			// Отправляем полученные данные обратно клиенту в тот же поток с сохранением флага завершения (эхо)
			if(server->send(cid, sid, data.data(), data.size(), fin))
				// Записываем в лог сообщение об отправке эхо-ответа
				this->_log->print("Echo: ID=%u, Stream=%" PRIu64 ", %zu bytes returned to client", log_t::flag_t::INFO, cid, sid, data.size());
			// Если отправка эхо-ответа не выполнена
			else this->_log->print("Failed to echo stream data to client", log_t::flag_t::WARNING);
		}
		/**
		 * @brief Метод обработки принятой датаграммы приложения QUIC (RFC 9221)
		 *
		 * @param cid    идентификатор сессии соединения
		 * @param data   данные датаграммы приложения
		 * @param server объект сервера
		 *
		 */
		void datagram(const event::id_t cid, const string & data, server_t * server) noexcept {
			// Записываем в лог сведения о принятой датаграмме приложения
			this->_log->print("Datagram: ID=%u, %zu bytes", log_t::flag_t::INFO, cid, data.size());
			// Печатаем принятую от клиента датаграмму, чтобы видеть её содержимое
			cout << endl << "==== SERVER RECEIVED DATAGRAM (" << data.size() << " bytes) ====" << endl
			     << data << endl
			     << "==== END OF RECEIVED DATAGRAM ====" << endl;
			// Отправляем датаграмму обратно клиенту (эхо)
			if(server->datagram(cid, data.data(), data.size()))
				// Записываем в лог сообщение об отправке эхо-датаграммы
				this->_log->print("Echo datagram: ID=%u, %zu bytes returned to client", log_t::flag_t::INFO, cid, data.size());
		}
		/**
		 * @brief Метод обработки завершённого соединения QUIC
		 *
		 * @param cid   идентификатор сессии соединения
		 * @param error код ошибки завершения соединения
		 *
		 */
		void disconnect(const event::id_t cid, const quic::error_t error) noexcept {
			// Записываем в лог сообщение о завершении соединения
			this->_log->print("QUIC connection closed: ID=%u, Error=%s", log_t::flag_t::INFO, cid, quic::errorName(error).data());
		}
		/**
		 * @brief Метод обработки готовности сервера к работе
		 *
		 * @param eid    идентификатор события сервера
		 * @param family семейство адресов сервера
		 * @param domain доменное имя сервера
		 * @param ip     IP-адрес сервера
		 *
		 */
		void ready([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::family_t family, const string & domain, const string & ip) noexcept {
			// Записываем в лог сообщение о готовности сервера к работе
			this->_log->print("QUIC server is ready: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки ошибок сервера
		 *
		 * @param eid     идентификатор события сервера
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 *
		 */
		void error([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & message, [[maybe_unused]] void * ctx) noexcept {
			// Записываем ошибку в лог
			this->_log->print("QUIC server error: %s", log_t::flag_t::CRITICAL, message.c_str());
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
int32_t main([[maybe_unused]] int32_t argc, [[maybe_unused]] char * argv[]){
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
	// Регистрируем шаблон контекста безопасности для транспорта QUIC
	const tls::coder_t::id_t cts = tls.context(event::node_t::SERVER, event::protocol_t::QUIC);
	// Если шаблон контекста безопасности не создан
	if(cts == 0){
		// Записываем в лог сообщение об ошибке
		log.print("QUIC security context is not created", log_t::flag_t::CRITICAL);
		// Выходим из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Устанавливаем список поддерживаемых ALPN-протоколов (RFC 9001 §8.1)
	tls.alpn(cts, {tls::coder_t::alpn_t{0, "h3"}});
	// Устанавливаем сертификат сервера
	tls.certificate(cts, "../sh/certificates/server/cert.pem");
	// Устанавливаем приватный ключ сервера
	tls.privateKey(cts, "../sh/certificates/server/key.pem");
	/**
	 * Снимаем проверку сертификата удалённого узла: на серверном узле это означает
	 * требование клиентского сертификата (взаимная аутентификация), для эхо-сервера
	 * оно не нужно
	 */
	tls.validateServerNameIndication(cts, false);
	// Создаём объект сервера QUIC на шаблоне контекста безопасности
	server_t server(cts, &tls, &fmk, &log);
	// Создаём событие сервера транспорта QUIC поверх UDP
	server.init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::QUIC);
	/**
	 * Настройки кластера задаются ПОСЛЕ init(): фасад маршрутизирует вызовы на
	 * юнит QUIC только когда протокол транспорта уже установлен методом init()
	 */
	// Устанавливаем имя кластера для сервера
	server.clusterName("ANYKS");
	// Устанавливаем количество вокеров кластера для сервера
	server.clusterCount(4);
	// Устанавливаем диапазон портов кластера для сервера
	server.clusterRange(2222, 2232);
	// Включаем режим кластера для сервера
	server.clusterMode(event::mode_t::ENABLED);
	// Локальные транспортные параметры соединений QUIC (RFC 9000 §7.4)
	quic::params::params_t params;
	// Устанавливаем таймаут простоя соединения в миллисекундах
	params.maxIdleTimeout = 30000;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiLocal = 262144;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит данных однонаправленных потоков
	params.initialMaxStreamDataUni = 262144;
	// Устанавливаем лимит числа двунаправленных потоков
	params.initialMaxStreamsBidi = 100;
	// Устанавливаем лимит числа однонаправленных потоков
	params.initialMaxStreamsUni = 100;
	// Устанавливаем предельный размер принимаемой датаграммы приложения (RFC 9221 §3)
	params.maxDatagramFrameSize = 1200;
	// Устанавливаем локальные транспортные параметры соединений QUIC
	server.params(params);
	// Включаем проверку адреса клиента через пакет Retry (RFC 9000 §8.1.2)
	server.retry(true);
	// Включаем уведомление о перегрузке пути (RFC 9000 §13.4)
	server.ecn(true);
	// Устанавливаем хост и порт сервера
	// if(server.setPort(2222) && server.setHost("127.0.0.1")){
	if(server.setHost("127.0.0.1")){
		// Регистрируем функцию обратного вызова на событие изменения статуса сервера
		server.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &server);
		// Регистрируем функцию обратного вызова на событие установленного соединения QUIC
		server.on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Executor::accept, &executor, _1, _2, _3, &server);
		// Регистрируем функцию обратного вызова на собранные данные потока приложения QUIC
		server.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("stream", &Executor::stream, &executor, _1, _2, _3, _4, &server);
		// Регистрируем функцию обратного вызова на принятую датаграмму приложения QUIC
		server.on <void (const event::id_t, const string &)> ("datagram", &Executor::datagram, &executor, _1, _2, &server);
		// Регистрируем функцию обратного вызова на завершённое соединение QUIC
		server.on <void (const event::id_t, const quic::error_t)> ("disconnect", &Executor::disconnect, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие готовности сервера к работе
		server.on <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3, _4);
		// Регистрируем функцию обратного вызова на событие ошибок сервера
		server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &Executor::error, &executor, _1, _2, _3, _4);
		// Запускаем событие сервера
		server.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
