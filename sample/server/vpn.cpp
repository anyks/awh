/**
 * @file: vpn.cpp
 * @date: 2026-05-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример VPN-сервера — демонстрация приёма подключений клиентов,
 *        создания виртуального интерфейса TUN и маршрутизации IP-пакетов между туннелем и клиентами
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем стандартные модули
 */
#include <netinet/in.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <server/server.hpp>
#include <units/tunnel.hpp>
#include <units/mediator.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * Базовая сеть 10.0.0.0 в host byte order
 */
#define BASE_IP_HOST 0x0A000000

/**
 * Счётчик хоста. Начинаем с 2, чтобы пропустить:
 * - 10.0.0.0 (сеть), 10.0.0.1 (сам сервер)
 */
static uint16_t nextHostId = 2;

/**
 * @brief Класс объекта исполнителя
 *
 */
class Executor {
	public:
		/**
		 * @brief Экшены запросов VPN-клиента
		 *
		 */
		enum class action_t : uint8_t {
			NONE      = 0x00,
			PAYLOAD   = 0x01,
			HANDSHAKE = 0x02
		};
	public:
		/**
		 * @brief Структура записи сообщения VPN-клиента
		 *
		 */
		typedef struct Record {
			action_t action; // Действие запроса VPN-клиента
			size_t size;     // Размер данных сообщения VPN-клиента
			/**
			 * @brief Конструктор
			 *
			 */
			Record() noexcept : action(action_t::NONE), size(0) {}
		} __attribute__((packed)) record_t;
	private:
		// Объект работы с сетевыми адресами
		net_addr_t _addr;
	private:
		// Сопоставление идентификаторов событий клиента и туннеля
		unordered_map <event::id_t, event::id_t> _clientToTunnelMap;
	private:
		// Объект туннеля
		unit::tunnel_t * _tunnel;
		// Объект посредника между сервером и туннелем
		unit::mediator_t * _mediator;
	private:
		// Объект фреймворка
		const fmk_t * _fmk;
		// Объект работы с логами
		const log_t * _log;
	public:
		/**
		 * @brief Функция генерации следующего IP-адреса для VPN-клиента (10.0.0.0/16)
		 *
		 * @return следующий IP-адрес в network byte order
		 *
		 */
		static uint32_t ip() noexcept {
			// Переменная результата
			uint32_t result = 0;
			/**
			 * Диапазон хостов для /16: 1..65534
			 * 0x0000 = 10.0.0.0 (сеть), 0xFFFF = 10.0.255.255 (broadcast)
			 */
			if(nextHostId >= 0xFFFE)
				// Зацикливаем диапазон
				nextHostId = 2;
			// Переводим в network byte order для struct sockaddr_in
			result = htonl(BASE_IP_HOST | nextHostId);
			// Увеличиваем счётчик хоста для следующего запроса
			nextHostId++;
			// Возвращаем результат
			return result;
		}
		/**
		 * @brief Метод обработки событий изменения статуса туннеля
		 *
		 * @param eid    идентификатор события
		 * @param status новый статус туннеля
		 *
		 */
		void statusVPN(const event::id_t eid, const event::status_t status) noexcept {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус уничтожения
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Tunnel event destroyed: ID=%u", log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Tunnel event initialized: ID=%u", log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Tunnel event launched: ID=%u", log_t::flag_t::INFO, eid);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий чтения данных туннеля
		 *
		 * @param eid    идентификатор клиента
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param server объект сервера
		 *
		 */
		void readVPN(const event::id_t eid, const uint8_t * data, const size_t size, server_t * server) noexcept {
			// Если данные получены
			if((data != nullptr) && (size > 0)){
				// Объект отправляемых данных
				record_t record{};
				// Устанавливаем размер данных полезной нагрузки
				record.size = size;
				// Устанавливаем действие ответа VPN-клиенту в полезную нагрузку
				record.action = action_t::PAYLOAD;
				// Буфер данных ответа для отправки клиенту
				vector <uint8_t> response;
				// Добавляем в ответ заголовок с информацией о действии и размере данных
				response.insert(response.end(), reinterpret_cast <const uint8_t *> (&record), reinterpret_cast <const uint8_t *> (&record) + sizeof(record_t));
				// Добавляем в ответ полезную нагрузку
				response.insert(response.end(), data, data + size);
				// Отправляем данные обратно клиенту
				if(server->send(eid, &response[0], response.size()) == 0)
					// Записываем ошибку в лог отправки данных клиентом на сервер
					this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
			}
		}
		/**
		 * @brief Метод обработки ошибок туннеля
		 *
		 * @param eid    идентификатор события туннеля
		 * @param error  код ошибки
		 * @param message сообщение об ошибке
		 *
		 */
		void errorVPN([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & message) noexcept {
			// Записываем ошибку в лог
			this->_log->print("Tunnel error: %s", log_t::flag_t::CRITICAL, message.c_str());
		}
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
		 * @param tid    идентификатор тоннеля
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param server объект сервера
		 *
		 */
		void read(const event::id_t eid, const event::id_t tid, const uint8_t * data, const size_t size, [[maybe_unused]] void * ctx, server_t * server) noexcept {
			// Если данные получены
			if(size >= sizeof(record_t)){
				// Объект получаемых данных
				record_t record{};
				// Устанавливаем смещение для чтения данных из буфера
				const size_t offset = sizeof(record);
				// Копируем данные в объект получаемых данных
				::memcpy(&record, data, offset);
				/**
				 * Определяем действие запроса VPN-клиента
				 */
				switch(static_cast <uint8_t> (record.action)){
					// Если действие является полезной нагрузкой
					case static_cast <uint8_t> (action_t::PAYLOAD): {
						// Если данные рукопожатия получены
						if(size >= (offset + record.size)){
							// Отправляем данные в туннель
							if(this->_tunnel->send(tid, data + offset, record.size))
								// Если данные успешно отправлены
								this->_log->print("Sent to tunnel: ID=%u, %zu bytes", log_t::flag_t::INFO, tid, record.size);
							// Если данные не отправлены
							else this->_log->print("Failed to send to tunnel: ID=%u", log_t::flag_t::CRITICAL, tid);
						}
					} break;
					// Если действие является рукопожатием
					case static_cast <uint8_t> (action_t::HANDSHAKE): {
						// Генерируем следующий IP-адрес для VPN-клиента
						const uint32_t clientIP = ip();
						// Устанавливаем размер данных ответа
						record.size = sizeof(clientIP);
						// Буфер данных ответа для отправки клиенту
						vector <uint8_t> response;
						// Добавляем в ответ заголовок с информацией о действии и размере данных
						response.insert(response.end(), reinterpret_cast <const uint8_t *> (&record), reinterpret_cast <const uint8_t *> (&record) + sizeof(record_t));
						// Добавляем в ответ IP-адрес клиента
						response.insert(response.end(), reinterpret_cast <const uint8_t *> (&clientIP), reinterpret_cast <const uint8_t *> (&clientIP) + sizeof(clientIP));
						// Отправляем данные обратно клиенту
						if(server->send(eid, &response[0], response.size()) == 0)
							// Записываем ошибку в лог отправки данных клиентом на сервер
							this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
						// Если данные успешно отправлены
						else {
							// Устанавливаем IP-адрес клиента в объект работы с сетевыми адресами
							this->_addr.v4(clientIP);
							// Создаём новый объект посредника между клиентом и туннелем
							const event::id_t mid = this->_mediator->issue(event::family_t::IPV4);
							// Устанавливаем адрес сервера назначения и фиксацию параметров посредника
							if(this->_mediator->setTarget(mid, this->_addr.source().get()) && this->_mediator->commit(mid)){
								// Подключаем посредника к клиенту
								this->_mediator->splice(mid, eid);
								// Сопоставляем идентификаторы событий клиента и туннеля
								this->_clientToTunnelMap.emplace(eid, mid);
							}
						}
					} break;
					// Если действие не определено
					default:
						// Записываем в лог сообщение о неизвестном действии
						this->_log->print("Received unknown action from client: %d", log_t::flag_t::WARNING, static_cast <uint16_t> (record.action));
					break;
				}
			// Если данные не получены или мусор
			} else this->_log->print("No data received or invalid data", log_t::flag_t::WARNING);
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
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Записываем в лог сообщение об успешном запуске события сервера
					this->_log->print("Server event successfully launched on port %d", log_t::flag_t::INFO, server->getPort());
				break;
				// Если событие сервера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события сервера
					this->_log->print("Server event destroyed", log_t::flag_t::INFO);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий изменения статуса клиентов
		 *
		 * @param eid    идентификатор клиента
		 * @param status новый статус клиента
		 *
		 */
		void state(const event::id_t eid, const event::status_t status, [[maybe_unused]] void * ctx) noexcept {
			/**
			 * Обрабатываем статус клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус уничтожения
				case static_cast <uint8_t> (event::status_t::DESTROYED): {
					// Ищем идентификатор туннеля, связанный с данным клиентом
					auto i = this->_clientToTunnelMap.find(eid);
					// Если сопоставление найдено
					if(i != this->_clientToTunnelMap.end()){
						// Уничтожаем событие туннеля, связанное с данным клиентом
						this->_mediator->destroy(i->second);
						// Удаляем сопоставление идентификаторов событий клиента и туннеля
						this->_clientToTunnelMap.erase(i);
					}
					// Записываем в лог сообщение об уничтожении клиента
					this->_log->print("Client destroyed: ID=%u", log_t::flag_t::INFO, eid);
				} break;
				// Если статус инициализации
				case static_cast <uint8_t> (event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации клиента
					this->_log->print("Client initialized: ID=%u", log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска клиента
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске клиента
					this->_log->print("Client launched: ID=%u", log_t::flag_t::INFO, eid);
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
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Connection established: " << server->getAddress(cid, event::address_t::IPV4) << ":" << server->getPort(cid) << ", MAC: " << server->getAddress(cid, event::address_t::MAC) << endl;
		}
		/**
		 * @brief Метод обработки событий запуска сервера
		 *
		 * @param address адрес сервера
		 * @param port    порт сервера
		 * @param server  объект сервера
		 *
		 */
		void launch(const string & address, const uint16_t port, server_t * server) noexcept {
			// Записываем в лог сообщение о запуске сервера
			this->_log->print("Server is launching to %s:%d", log_t::flag_t::INFO, address.c_str(), port);
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
		 * @param tun объект туннеля
		 * @param med объект посредника между сервером и туннелем
		 * @param fmk объект фреймворка
		 * @param log объект логирования
		 *
		 */
		Executor(unit::tunnel_t * tun, unit::mediator_t * med, const fmk_t * fmk, const log_t * log) noexcept :
		 _addr(fmk, log), _tunnel(tun), _mediator(med), _fmk(fmk), _log(log) {}
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
	// Создаём объект тоннеля
	unit::tunnel_t tunnel(&fmk, &log);
	// Создаём объект посредника между сервером и туннелем
	unit::mediator_t mediator(&fmk, &log);
	// Создаём объект сервера
	server_t server(&fmk, &log);
	// Создаём объект исполнителя для обработки событий сервера
	Executor executor(&tunnel, &mediator, &fmk, &log);
	// Создаём событие сервера и сохраняем его идентификатор
	const event::id_t eid = server.init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Устананавливаем опции события
	if(server.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set event options!" << endl;
	// Выполняем создание события туннеля
	const event::id_t tun = tunnel.issue(event::family_t::IPV4);
	// Устананавливаем опции события туннеля
	if(tunnel.setOptions(tun, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set tunnel event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set tunnel event options!" << endl;
	// Устанавливаем IP-адрес события туннеля и максимальный размер пакета для передачи данных по сети
	if(tunnel.setAddress(tun, event::address_t::IPV4, "10.0.0.1") && tunnel.setMaximumTransmissionUnit(tun, 1400)){
		// Регистрируем функцию обратного вызова на событие изменения статуса тоннеля
		tunnel.on <void (const event::id_t, const event::status_t)> ("state", &Executor::statusVPN, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие ошибок тоннеля
		tunnel.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Executor::errorVPN, &executor, _1, _2, _3);
		// Регистрируем функцию обратного вызова на событие изменения статуса посредника
		mediator.on <void (const event::id_t, const event::status_t)> ("state", &Executor::statusVPN, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие чтения данных посредником
		mediator.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &Executor::readVPN, &executor, _1, _2, _3, &server);
		// Регистрируем функцию обратного вызова на событие ошибок посредника
		mediator.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Executor::errorVPN, &executor, _1, _2, _3);
		// Выполняем фиксацию параметров туннеля
		if(tunnel.commit(tun)){
			// Устанавливаем порт и хост сервера
			if(server.setPort(3333) && server.setHost("0.0.0.0")){
				// Устанавливаем таймаут сервера на чтение данных 6 секунд
				server.setTimeout(event::action_t::READ, 6000);
				// Регистрируем функцию обратного вызова на событие изменения статуса сервера
				server.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &server);
				// Регистрируем функцию обратного вызова на событие записи данных сервером
				server.on <void (const event::id_t, const size_t, void *)> ("write", &Executor::write, &executor, _1, _2, _3);
				// Регистрируем функцию обратного вызова на событие изменения статуса клиентов
				server.on <void (const event::id_t, const event::status_t, void *)> ("state", &Executor::state, &executor, _1, _2, _3);
				// Регистрируем функцию обратного вызова на событие запуска сервера
				server.on <void (const string &, const uint16_t)> ("launch", &Executor::launch, &executor, _1, _2, &server);
				// Регистрируем функцию обратного вызова на событие ошибок сервера
				server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &Executor::error, &executor, _1, _2, _3, _4);
				// Регистрируем функцию обратного вызова на событие чтения данных сервером
				server.on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &Executor::read, &executor, _1, tun, _2, _3, _4, &server);
				// Регистрируем функцию обратного вызова на событие готовности сервера к работе
				server.on <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3, _4);
				// Регистрируем функцию обратного вызова на событие подключения клиента к серверу
				server.on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Executor::accept, &executor, _1, _2, _3, &server);
				// Нужно активировать порт 3333 в Firewall
				// sudo ipfw add 100 allow udp from any to me 3333 in
				// Удаление старого сетевого интерфейса тоннеля в FreeBSD
				// sudo ifconfig tun3 destroy
				// На сервере, чтобы получить ответ на пинг, нужно добавить роутинг, где (10.0.0.2 - адрес клиента)
				// sudo route add 10.0.0.2 -interface tun0
				// Запускаем событие сервера
				server.start();
			}
		}
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
