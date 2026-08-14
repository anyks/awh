/**
 * @file vpn.cpp
 * @date 2026-05-22
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
 * @brief Пример VPN-клиента — демонстрация связывания виртуального сетевого интерфейса TUN с удалённым сервером через
 *        модуль посредника и передачи IP-пакетов по защищённому каналу
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <client/client.hpp>
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
		// Идентификатор события туннеля
		event::id_t _tun;
	private:
		// Объект работы с сетевыми адресами
		net_addr_t _addr;
		// Объект работы с шлюзами
		eth::gateway_t _gateway;
	private:
		// Флаг завершения рукопожатия
		bool _handshakeCompleted;
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
					this->_log->print("Tunnel destroyed: ID=%u", log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Tunnel initialized: ID=%u", log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Tunnel launched: ID=%u", log_t::flag_t::INFO, eid);
				break;
			}
		}
		/**
		 * @brief Метод обработки событий чтения данных туннеля
		 *
		 * @param eid    идентификатор клиента
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param client объект клиента
		 *
		 */
		void readVPN([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size, client_t * client) noexcept {
			// Если данные получены
			if((data != nullptr) && (size > 0)){
				// Объект отправляемых данных
				record_t record{};
				// Устанавливаем размер данных полезной нагрузки
				record.size = size;
				// Устанавливаем действие ответа VPN-серверу в полезную нагрузку
				record.action = action_t::PAYLOAD;
				// Буфер данных ответа для отправки серверу
				vector <uint8_t> response;
				// Добавляем в ответ заголовок с информацией о действии и размере данных
				response.insert(response.end(), reinterpret_cast <const uint8_t *> (&record), reinterpret_cast <const uint8_t *> (&record) + sizeof(record_t));
				// Добавляем в ответ полезную нагрузку
				response.insert(response.end(), data, data + size);
				// Отправляем данные обратно серверу
				if(client->send(&response[0], response.size()) == 0)
					// Записываем ошибку в лог отправки данных клиентом на сервер
					this->_log->print("Failed to send data to server", log_t::flag_t::WARNING);
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
		 * @param size размер данных для записи
		 *
		 */
		void write(const size_t size) noexcept {
			// Записываем в лог информацию о событии записи данных клиентом
			this->_log->print("Client write event: %zu bytes", log_t::flag_t::INFO, size);
		}
		/**
		 * @brief Метод обработки событий чтения данных клиентом
		 *
		 * @param eid    идентификатор клиента
		 * @param data   бинарный буфер данных
		 * @param size   размер данных
		 * @param client объект клиента
		 *
		 */
		void read(const event::id_t eid, const uint8_t * data, const size_t size, client_t * client) noexcept {
			// Если данные получены
			if(size >= sizeof(record_t)){
				// Объект получаемых данных
				record_t record{};
				// Устанавливаем смещение для чтения данных из буфера
				const size_t offset = sizeof(record);
				// Копируем данные в объект получаемых данных
				::memcpy(&record, data, offset);
				// Если данные рукопожатия получены
				if(size >= (offset + record.size)){
					/**
					 * Определяем действие запроса VPN-клиента
					 */
					switch(static_cast <uint8_t> (record.action)){
						// Если действие является полезной нагрузкой
						case static_cast <uint8_t> (action_t::PAYLOAD): {
							// Если данные рукопожатия получены
							if(size >= (offset + record.size)){
								// Отправляем данные в туннель
								if(this->_tunnel->send(this->_tun, data + offset, record.size))
									// Если данные успешно отправлены
									this->_log->print("Sent to tunnel: ID=%u, %zu bytes", log_t::flag_t::INFO, this->_tun, record.size);
								// Если данные не отправлены
								else this->_log->print("Failed to send to tunnel: ID=%u", log_t::flag_t::CRITICAL, this->_tun);
							}
						} break;
						// Если действие является рукопожатием
						case static_cast <uint8_t> (action_t::HANDSHAKE): {
							// IP-адрес клиента
							uint32_t clientIP = 0;
							// Извлекаем IP-адрес клиента из данных рукопожатия
							::memcpy(&clientIP, data + offset, record.size);
							// Устанавливаем IP-адрес клиента в объект работы с сетевыми адресами
							this->_addr.v4(clientIP);
							// Записываем в лог сообщение о получении рукопожатия с сервером и полученным IP-адресом клиента
							cout << "Received handshake from server with IP: " << static_cast <string> (this->_addr) << endl;
							// Выполняем создание события туннеля
							this->_tun = this->_tunnel->issue(event::family_t::IPV4);
							// Устананавливаем опции события туннеля
							if(this->_tunnel->setOptions(this->_tun, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
								// Записываем в лог сообщение об успешной установке опций события
								cout << " Successfully set tunnel event options!" << endl;
							// Записываем ошибку в лог установки опций события
							else cout << " Failed to set tunnel event options!" << endl;
							// Устанавливаем IP-адрес события туннеля и максимальный размер пакета для передачи данных по сети
							if(this->_tunnel->setAddress(this->_tun, event::address_t::IPV4, this->_addr.source().get()) && this->_tunnel->setMaximumTransmissionUnit(this->_tun, 1400)){
								// Создаём новый объект посредника между клиентом и туннелем
								const event::id_t mid = this->_mediator->issue(event::family_t::IPV4);
								// Устанавливаем адрес сервера назначения и фиксацию параметров посредника
								if(this->_mediator->setTarget(mid, "10.0.0.1") && this->_tunnel->setTarget(this->_tun, "10.0.0.1") && this->_mediator->commit(mid) && this->_tunnel->commit(this->_tun)){
									// Подключаем посредника к клиенту
									if(this->_mediator->splice(mid, eid)){
										// Маршрут туннеля
										eth::gateway_t::route_t route;
										// Устанавливаем интерфейс туннеля
										route.ifname = this->_tunnel->getIface(this->_tun);
										// Устанавливаем префикс маршрута туннеля
										route.prefix = 24;
										// Создаём шлюз маршрута туннеля
										route.gateway = make_unique <net::addr_net_ipv4_t> ();
										// Создаём адрес назначения маршрута туннеля
										route.destination = make_unique <net::addr_net_ipv4_t> ();
										// Выполням парсинг адреса назначения маршрута туннеля
										this->_addr = "10.0.0.0";
										// Устанавливаем адрес назначения маршрута туннеля
										awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
										// Устанавливаем маршрут туннеля (sudo route -n add -net 10.0.0.0/24 -interface utun7)
										if(this->_gateway.add(route))
											// Записываем в лог сообщение об успешной установке маршрута туннеля
											cout << " Tunnel route successfully added!" << endl;
										// Записываем в лог сообщение об успешном запуске события
										cout << " Server and tunnel event successfully launched!" << endl;
									}
								}
							}
						} break;
						// Если действие не определено
						default:
							// Записываем в лог сообщение о неизвестном действии
							this->_log->print("Received unknown action from server: %d", log_t::flag_t::WARNING, static_cast <uint16_t> (record.action));
						break;
					}
				// Записываем в лог сообщение о получении некорректных данных рукопожатия от клиента
				} else this->_log->print("Received invalid data from server", log_t::flag_t::WARNING);
			// Если данные не получены или мусор
			} else this->_log->print("No data received or invalid data", log_t::flag_t::WARNING);
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
						this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
					// Если подключение выполнено, то выводим сообщение об успешном подключении клиента к удалённому серверу
					else this->_log->print("Successfully connected to remote server", log_t::flag_t::INFO);
				} break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события клиента
					this->_log->print("Client destroyed", log_t::flag_t::INFO);
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
				// Объект получаемых данных
				record_t record{};
				// Устанавливаем действие запроса VPN-клиента в запрос на рукопожатие
				record.action = action_t::HANDSHAKE;
				// Если отправка данных данных клиентом на сервер не выполнена
				if(client->send(&record, sizeof(record)) == 0)
					// Записываем ошибку в лог отправки данных клиентом на сервер
					this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
			// Если подключение не выполнено, то выводим сообщение об ошибке подключения клиента к удалённому серверу
			} else this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
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
			this->_log->print("Client is ready to connect to remote server: %s (%s)", log_t::flag_t::INFO, domain.c_str(), ip.c_str());
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
			this->_log->print("Client error: %s", log_t::flag_t::CRITICAL, message.c_str());
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
		 _tun(0), _addr(fmk, log), _gateway(fmk, log),
		 _handshakeCompleted(false), _tunnel(tun), _mediator(med), _fmk(fmk), _log(log) {}
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
	// Создаём объект DNS-резолвера
	unit::dns_t dns(event::family_t::IPV4, &fmk, &log);
	// Создаём объект клиента
	client_t client(&dns, &fmk, &log);
	// Создаём объект исполнителя для обработки событий сервера
	Executor executor(&tunnel, &mediator, &fmk, &log);
	// Устанавливаем список поддерживаемых DNS-серверов
	dns.setServers({"77.88.8.8", "77.88.8.1"});
	// Создаём событие клиента и сохраняем его идентификатор
	const event::id_t eid = client.init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Устананавливаем опции события
	if(client.setOptions(event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
		// Записываем в лог сообщение об успешной установке опций события
		cout << " Successfully set event options!" << endl;
	// Записываем ошибку в лог установки опций события
	else cout << " Failed to set event options!" << endl;
	// Устанавливаем порт и целевой хост для клиента
	if(client.setTarget("anyks.com") && client.setTargetPort(3333)){
		// Регистрируем функцию обратного вызова на событие изменения статуса тоннеля
		tunnel.on <void (const event::id_t, const event::status_t)> ("state", &Executor::statusVPN, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие ошибок тоннеля
		tunnel.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Executor::errorVPN, &executor, _1, _2, _3);
		// Регистрируем функцию обратного вызова на событие изменения статуса посредника
		mediator.on <void (const event::id_t, const event::status_t)> ("state", &Executor::statusVPN, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие чтения данных посредником
		mediator.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &Executor::readVPN, &executor, _1, _2, _3, &client);
		// Регистрируем функцию обратного вызова на событие ошибок посредника
		mediator.on <void (const event::id_t, const event::error_t, const string &)> ("error", &Executor::errorVPN, &executor, _1, _2, _3);
		// Устанавливаем таймаут клиента на чтение данных 6 секунд
		client.setTimeout(event::action_t::READ, 6000);
		// Регистрируем функцию обратного вызова на событие изменения статуса клиента
		client.on <void (const event::status_t)> ("status", &Executor::status, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие записи данных клиентом
		client.on <void (const size_t)> ("write", &Executor::write, &executor, _1);
		// Регистрируем функцию обратного вызова на событие подключения клиента к удалённому серверу
		client.on <void (const bool)> ("connect", &Executor::connect, &executor, _1, &client);
		// Регистрируем функцию обратного вызова на событие чтения данных клиентом
		client.on <void (const uint8_t *, const size_t)> ("read", &Executor::read, &executor, eid, _1, _2, &client);
		// Регистрируем функцию обратного вызова на событие ошибок клиента
		client.on <void (const event::error_t, const string &)> ("error", &Executor::error, &executor, _1, _2);
		// Регистрируем функцию обратного вызова на событие готовности клиента к работе
		client.on <void (const event::family_t, const string &, const string &)> ("ready", &Executor::ready, &executor, _1, _2, _3);
		// Запускаем событие клиента
		client.start();
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
