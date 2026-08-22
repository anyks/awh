/**
 * @file quic-proxy.cpp
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
 * @brief Пример прокси-сервера QUIC — демонстрация приёма QUIC-соединений и туннелирования прикладного трафика
 *        клиентов в исходящие TCP-подключения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем стандартные модули
 */
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <server/server.hpp>
#include <unit/client.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Класс объекта исполнителя QUIC-прокси на объединении данных (splice)
 *
 * @details Прокси принимает соединения QUIC (фронтенд) и на каждую сессию открывает
 *          отдельное исходящее TCP-соединение к бэкенду (client-юнит), после чего
 *          связывает их в обоих направлениях объединением данных:
 *          - server.splice(cid, bid): расшифрованные данные потоков сессии QUIC
 *            перенаправляются сырьём в сокет бэкенда (см. relay);
 *          - client.splice(bid, cid): ответ бэкенда возвращается в сессию QUIC
 *            туннельным потоком с шифрованием на уровне соединения (см. inject).
 *
 *          Такой же приём (client-юнит + двусторонний splice) использует прокси SOCKS5:
 *          посредник (mediator) для этого не годится, так как рассчитан на raw-IP/TUN
 *          и не имеет понятия порта назначения
 *
 */
class Proxy {
	private:
		// Объект фреймворка
		const fmk_t * _fmk;
		// Объект работы с логами
		const log_t * _log;
	private:
		// Объект QUIC-сервера (фронтенд прокси)
		server_t * _server;
		// Объект клиента (исходящие TCP-соединения к бэкенду)
		unit::client_t * _client;
	private:
		// Адрес хоста бэкенда
		string _host;
		// Порт хоста бэкенда
		uint16_t _port;
	private:
		// Сопоставление идентификатора сессии QUIC и идентификатора события бэкенда
		unordered_map <event::id_t, event::id_t> _sessionToBackend;
		// Сопоставление идентификатора события бэкенда и идентификатора сессии QUIC
		unordered_map <event::id_t, event::id_t> _backendToSession;
	public:
		/**
		 * @brief Метод обработки событий изменения статуса QUIC-сервера
		 *
		 * @param status новый статус сервера
		 *
		 */
		void status(const event::status_t status) noexcept {
			/**
			 * Определяем состояние сервера
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие сервера запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Переводим сервер в режим прослушивания входящих соединений
					if(!this->_server->listen(100))
						// Записываем ошибку в лог
						this->_log->print("Failed to listen on port %d", log_t::flag_t::WARNING, this->_server->getPort());
					// Если прослушивание успешно запущено
					else this->_log->print("QUIC proxy is listening on port %d (backend %s:%d)", log_t::flag_t::INFO, this->_server->getPort(), this->_host.c_str(), this->_port);
				} break;
				// Если событие сервера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события сервера
					this->_log->print("QUIC proxy destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки установленного соединения QUIC (фронтенд прокси)
		 *
		 * @param eid идентификатор события сервера
		 * @param cid идентификатор сессии соединения QUIC
		 * @param tid идентификатор шаблона контекста безопасности
		 *
		 */
		void accept([[maybe_unused]] const event::id_t eid, const event::id_t cid, [[maybe_unused]] const tls::coder_t::id_t tid) noexcept {
			// Записываем в лог сообщение об установленном соединении QUIC
			this->_log->print("QUIC connection established: ID=%u, Address=%s", log_t::flag_t::INFO, cid, this->_server->getAddress(cid, event::address_t::IPV4).c_str());
			// Открываем исходящее TCP-соединение к бэкенду для этой сессии QUIC
			const event::id_t bid = this->_client->issue(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Если событие бэкенда создать не удалось
			if(bid == 0){
				// Записываем ошибку в лог
				this->_log->print("Failed to create backend connection for session ID=%u", log_t::flag_t::CRITICAL, cid);
				// Выходим из метода
				return;
			}
			// Устанавливаем опции события бэкенда (неблокирующий сокет обязателен для событийной модели)
			if(!this->_client->setOptions(bid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
				// Записываем ошибку в лог
				this->_log->print("Failed to set backend options for session ID=%u", log_t::flag_t::CRITICAL, cid);
				// Уничтожаем событие бэкенда
				this->_client->destroy(bid);
				// Выходим из метода
				return;
			}
			// Устанавливаем адрес и порт хоста бэкенда
			if(!(this->_client->setTarget(bid, this->_host) && this->_client->setTargetPort(bid, this->_port))){
				// Записываем ошибку в лог
				this->_log->print("Failed to set backend target for session ID=%u", log_t::flag_t::CRITICAL, cid);
				// Уничтожаем событие бэкенда
				this->_client->destroy(bid);
				// Выходим из метода
				return;
			}
			// Сопоставляем идентификаторы сессии QUIC и события бэкенда
			this->_sessionToBackend.emplace(cid, bid);
			// Сопоставляем идентификаторы события бэкенда и сессии QUIC
			this->_backendToSession.emplace(bid, cid);
			/**
			 * Устанавливаем объединение данных СРАЗУ, до подключения бэкенда: у QUIC-сервера
			 * приёмник данных потоков всегда активен, поэтому данные, пришедшие до установки
			 * splice, были бы выданы (и потеряны) вместо перенаправления. Событие бэкенда уже
			 * создано issue(), а сессия QUIC актуальна - обе привязки допустимы сейчас.
			 * Перенаправляемые в неподключённый бэкенд данные ставятся в его очередь отправки
			 * и уходят по факту подключения
			 */
			// Обратное направление: ответ бэкенда инъецируется в туннельный поток сессии
			this->_server->splice(bid, cid);
			// Прямое направление: данные потоков сессии перенаправляются в сокет бэкенда
			this->_server->splice(cid, bid);
			/**
			 * Фиксируем параметры, инициируем подключение и запускаем событие бэкенда:
			 * launch включает отслеживание событий сокета в event loop, без него
			 * функция обратного вызова на подключение не сработает (см. SOCKS5)
			 */
			if(!(this->_client->commit(bid) && this->_client->connect(bid) && this->_client->launch(bid))){
				// Записываем ошибку в лог
				this->_log->print("Failed to connect backend %s:%d for session ID=%u", log_t::flag_t::CRITICAL, this->_host.c_str(), this->_port, cid);
				// Снимаем сопоставления и уничтожаем событие бэкенда
				this->closeBackend(bid);
			}
		}
		/**
		 * @brief Метод обработки завершённого соединения QUIC (фронтенд прокси)
		 *
		 * @param cid   идентификатор сессии соединения QUIC
		 * @param error код ошибки завершения соединения
		 *
		 */
		void disconnect(const event::id_t cid, const quic::error_t error) noexcept {
			// Записываем в лог сообщение о завершении соединения QUIC
			this->_log->print("QUIC connection closed: ID=%u, Error=%s", log_t::flag_t::INFO, cid, quic::errorName(error).data());
			// Выполняем поиск события бэкенда, связанного с этой сессией QUIC
			auto i = this->_sessionToBackend.find(cid);
			// Если событие бэкенда найдено
			if(i != this->_sessionToBackend.end()){
				// Идентификатор события бэкенда
				const event::id_t bid = i->second;
				// Снимаем сопоставление сессии QUIC
				this->_sessionToBackend.erase(i);
				// Снимаем сопоставление события бэкенда
				this->_backendToSession.erase(bid);
				// Уничтожаем событие бэкенда
				this->_client->destroy(bid);
			}
		}
		/**
		 * @brief Метод обработки события готовности QUIC-сервера к работе
		 *
		 * @param eid    идентификатор события сервера
		 * @param family семейство адресов сервера
		 * @param domain доменное имя сервера
		 * @param ip     IP-адрес сервера
		 *
		 */
		void ready([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::family_t family, const string & domain, const string & ip) noexcept {
			// Записываем в лог сообщение о готовности сервера к работе
			this->_log->print("QUIC proxy is ready: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
		}
		/**
		 * @brief Метод обработки ошибок QUIC-сервера
		 *
		 * @param eid     идентификатор события сервера
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 *
		 */
		void error([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & message, [[maybe_unused]] void * ctx) noexcept {
			// Записываем ошибку в лог
			this->_log->print("QUIC proxy error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
	public:
		/**
		 * @brief Метод обработки события подключения к бэкенду (исходящее соединение)
		 *
		 * @param bid идентификатор события бэкенда
		 * @param ok  результат подключения к бэкенду
		 *
		 */
		void connectBackend(const event::id_t bid, const bool ok) noexcept {
			// Выполняем поиск сессии QUIC, связанной с этим событием бэкенда
			auto i = this->_backendToSession.find(bid);
			// Если сессия QUIC для этого события бэкенда не найдена
			if(i == this->_backendToSession.end())
				// Выходим из метода
				return;
			// Идентификатор сессии QUIC
			const event::id_t cid = i->second;
			// Если подключение к бэкенду не выполнено
			if(!ok){
				// Записываем ошибку в лог
				this->_log->print("Backend connection failed for session ID=%u", log_t::flag_t::CRITICAL, cid);
				// Снимаем сопоставления и уничтожаем событие бэкенда
				this->closeBackend(bid);
				// Выходим из метода
				return;
			}
			/**
			 * Объединение данных уже установлено в accept() (до подключения бэкенда),
			 * поэтому здесь только фиксируем факт установленного соединения. Данные,
			 * перенаправленные в очередь отправки бэкенда до его подключения, уходят сейчас
			 */
			this->_log->print("Backend connected for session ID=%u: %s:%d", log_t::flag_t::INFO, cid, this->_host.c_str(), this->_port);
		}
		/**
		 * @brief Метод обработки событий изменения статуса бэкенда
		 *
		 * @param bid    идентификатор события бэкенда
		 * @param status новый статус бэкенда
		 *
		 */
		void stateBackend(const event::id_t bid, const event::status_t status) noexcept {
			// Если бэкенд подлежит уничтожению
			if(status == event::status_t::DESTROYED){
				// Выполняем поиск сессии QUIC, связанной с этим событием бэкенда
				auto i = this->_backendToSession.find(bid);
				// Если сессия QUIC найдена
				if(i != this->_backendToSession.end()){
					// Снимаем сопоставление сессии QUIC
					this->_sessionToBackend.erase(i->second);
					// Снимаем сопоставление события бэкенда
					this->_backendToSession.erase(i);
				}
			}
		}
		/**
		 * @brief Метод обработки ошибок бэкенда
		 *
		 * @param bid     идентификатор события бэкенда
		 * @param error   код ошибки
		 * @param message сообщение об ошибке
		 *
		 */
		void errorBackend([[maybe_unused]] const event::id_t bid, [[maybe_unused]] const event::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог
			this->_log->print("Backend error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
	private:
		/**
		 * @brief Метод снятия сопоставлений и уничтожения события бэкенда
		 *
		 * @param bid идентификатор события бэкенда
		 *
		 */
		void closeBackend(const event::id_t bid) noexcept {
			// Выполняем поиск сессии QUIC, связанной с этим событием бэкенда
			auto i = this->_backendToSession.find(bid);
			// Если сессия QUIC найдена
			if(i != this->_backendToSession.end()){
				// Снимаем сопоставление сессии QUIC
				this->_sessionToBackend.erase(i->second);
				// Снимаем сопоставление события бэкенда
				this->_backendToSession.erase(i);
			}
			// Уничтожаем событие бэкенда
			this->_client->destroy(bid);
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param server объект QUIC-сервера (фронтенд)
		 * @param client объект клиента (исходящие соединения к бэкенду)
		 * @param host   адрес хоста бэкенда
		 * @param port   порт хоста бэкенда
		 * @param fmk    объект фреймворка
		 * @param log    объект логирования
		 *
		 */
		Proxy(server_t * server, unit::client_t * client, string_view host, const uint16_t port, const fmk_t * fmk, const log_t * log) noexcept :
		 _fmk(fmk), _log(log), _server(server), _client(client), _host(host), _port(port) {}
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
	// Снимаем требование клиентского сертификата (взаимная аутентификация не нужна)
	tls.validateServerNameIndication(cts, false);
	// Создаём объект QUIC-сервера (фронтенд прокси) на шаблоне контекста безопасности
	server_t server(cts, &tls, &fmk, &log);
	// Создаём объект клиента для исходящих TCP-соединений к бэкенду
	unit::client_t client(&fmk, &log);
	// Создаём объект прокси (бэкенд - внешний TCP-эхо на 127.0.0.1:9999)
	Proxy proxy(&server, &client, "127.0.0.1", 9999, &fmk, &log);
	// Создаём событие сервера транспорта QUIC поверх UDP
	server.init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::QUIC);
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
	// Устанавливаем хост и порт QUIC-прокси
	if(server.setHost("127.0.0.1") && server.setPort(2222)){
		// Регистрируем функцию обратного вызова на событие изменения статуса сервера
		server.on <void (const event::status_t)> ("status", &Proxy::status, &proxy, _1);
		// Регистрируем функцию обратного вызова на событие установленного соединения QUIC
		server.on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Proxy::accept, &proxy, _1, _2, _3);
		// Регистрируем функцию обратного вызова на завершённое соединение QUIC
		server.on <void (const event::id_t, const quic::error_t)> ("disconnect", &Proxy::disconnect, &proxy, _1, _2);
		// Регистрируем функцию обратного вызова на событие готовности сервера к работе
		server.on <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", &Proxy::ready, &proxy, _1, _2, _3, _4);
		// Регистрируем функцию обратного вызова на событие ошибок сервера
		server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &Proxy::error, &proxy, _1, _2, _3, _4);
		// Регистрируем функцию обратного вызова на подключение к бэкенду
		client.on <void (const event::id_t, const bool)> ("connect", &Proxy::connectBackend, &proxy, _1, _2);
		// Регистрируем функцию обратного вызова на изменение статуса бэкенда
		client.on <void (const event::id_t, const event::status_t)> ("state", &Proxy::stateBackend, &proxy, _1, _2);
		// Регистрируем функцию обратного вызова на ошибки бэкенда
		client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Proxy::errorBackend, &proxy, _1, _2, _3);
		// Запускаем событие сервера
		server.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
