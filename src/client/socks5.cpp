/**
 * @file: socks5.cpp
 * @date: 2026-05-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация клиента SOCKS5-прокси — согласование методов авторизации с прокси-сервером,
 *        выполнение команд CONNECT, BIND и UDP ASSOCIATE,
 *        установка туннеля и прозрачное проксирование прикладного трафика поверх базового клиента
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Если размер MTU для UDP сообщений в IPv4 не определён
 */
#ifndef AWH_MTU_UDP_IPV4_PAYLOAD_SIZE
	/**
	 * Устанавливаем размер полезной нагрузки UDP для IPv4 в 1400 байт
	 * Значение выбрано как безопасный запас относительно стандартного MTU Ethernet (1500)
	 *
	 * Теоретический максимум без фрагментации: 1500 - 20 (IPv4) - 8 (UDP) = 1472
	 * Дополнительный запас 72 байта оставляем под инкапсуляцию (туннели, служебные заголовки)
	 * Превышение может приводить к фрагментации и снижению надёжности доставки
	 */
	#define AWH_MTU_UDP_IPV4_PAYLOAD_SIZE 0x578
#endif

/**
 * Если размер MTU для UDP сообщений в IPv6 не определён
 */
#ifndef AWH_MTU_UDP_IPV6_PAYLOAD_SIZE
	/**
	 * Устанавливаем размер полезной нагрузки UDP для IPv6 в 1380 байт
	 * Значение выбрано как безопасный запас относительно стандартного MTU Ethernet (1500)
	 *
	 * Теоретический максимум без фрагментации: 1500 - 40 (IPv6) - 8 (UDP) = 1452
	 * Дополнительный запас 72 байта оставляем под инкапсуляцию
	 * В IPv6 маршрутизаторы не фрагментируют пакеты, поэтому запас особенно важен
	 */
	#define AWH_MTU_UDP_IPV6_PAYLOAD_SIZE 0x564
#endif

/**
 * Стандартный заголовочный файл
 */
#include <climits>

/**
 * Подключаем заголовочный файл проекта
 */
#include <client/socks5.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * @brief Размер данных в буфере
	 *
	 */
	thread_local size_t __awh_size__ = 0;

	/**
	 * @brief Нулевой MAC-адрес для сравнения
	 *
	 */
	static uint8_t __awh_zero_mac__[6] = {0};

	/**
	 * @brief Нулевой IPv6-адрес для сравнения
	 *
	 */
	static uint8_t __awh_zero_ipv6__[16] = {0};

	/**
	 * @brief Буфер временного хранения данных UDP сообщений
	 *
	 */
	thread_local uint8_t __awh_buffer__[AWH_MTU_UDP_IPV4_PAYLOAD_SIZE] = {0};
};

/**
 * @brief Конструктор
 *
 */
awh::client::Socks5::Endpoint::Endpoint() noexcept : attr(nullptr) {}

/**
 * @brief Метод изменения статуса клиента
 *
 * @param index  индекс очереди запускаемого события
 * @param status новый статус клиента
 *
 */
void awh::client::Socks5::status(const uint8_t index, const event::status_t status) noexcept {
	/**
	 * Временное состояние клиента
	 */
	switch(index){
		// Если мы получили статус события клиента
		case 0: {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Выполняем подключение клиента к удалённому серверу
					if(!this->_unit->client.connect(static_cast <event::id_t> (this->_id.eid))){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Failed to connect to remote server", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (index)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
							#endif
						}
					// Если подключение выполнено
					} else {
						// Выполняем запуск работы клиента, если клиент не запущен
						if(!this->_unit->client.launch(this->_id.eid)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (index)), log_t::flag_t::WARNING, this->_id.eid);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, this->_id.eid);
								#endif
							}
						// Если клиент запущен удачно, выполняем функцию обратного вызова
						} else this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_unit->client.getTarget(this->_id.eid), this->_unit->client.getTargetPort(this->_id.eid));
					}
				} break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("status", status);
				break;
			}
		} break;
		// Если мы получили статус события DNS-резолвера
		case 1: {
			/**
			 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие DNS-резолвера запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					/**
					 * Определяем принадлежит ли хост, к IP-адресу
					 */
					switch(static_cast <uint8_t> (this->_unit->addr.host(this->_host))){
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Устанавливаем адрес хоста целевой машины для клиента
							if(this->_unit->client.setTarget(this->_id.eid, this->_host)){
								/**
								 * В зависимости от статуса события клиента выполняем запуск
								 */
								switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (&this->_unit->client)->status(this->_id.eid))){
									// Если событие клиента не запущено, запускаем его
									case static_cast <uint8_t> (event::status_t::NONE): {
										// Если событие клиента не запущено, запускаем его
										if(this->_unit->client.commit(this->_id.eid)){
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", this->_unit->client.family(this->_id.eid), this->_host, this->_unit->client.getTarget(this->_id.eid));
											// Запускаем клиента
											this->_unit->client.start();
										}
									} break;
									// Если событие клиента инициализировано, запускаем его
									case static_cast <uint8_t> (event::status_t::INITIAL):
									// Если событие находится в состоянии успешного подключения
									case static_cast <uint8_t> (event::status_t::SUCCESS):
										// Запускаем клиента
										this->_unit->client.start();
									break;
								}
							}
							// Выходим из функции
							return;
						}
					}
					// Выполняем разрешение доменного имени
					if(!this->_dns.client->resolve(this->_dns.id, this->_unit->client.family(this->_id.eid), this->_host, this->_dns.alive.load(std::memory_order_acquire))){
						// Создаём текст ошибки разрешения доменного имени
						const string error = this->_fmk->format("It was not possible to obtain an IP address for the domain name \"%s\"", this->_host.c_str());
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (index)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						// Выполняем функцию обратного вызова
						} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::NOT_FOUND, error);
					}
				} break;
				// Если событие DNS-резолвера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Останавливаем клиента
					this->_unit->client.stop();
				break;
			}
		} break;
	}
}
/**
 * @brief Метод обработки событий подключения клиента к удалённому серверу
 *
 * @param eid идентификатор клиента
 * @param ok  результат подключения
 *
 */
void awh::client::Socks5::connect(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если подключение выполнено успешно
		if(ok){
			// Размер буфера данных
			size_t size = 0;
			// Буфер данных запроса
			uint8_t * buffer = nullptr;
			// Если извлечение буфера данных запроса выполнено успешно
			if(this->_socks5.buffer(&buffer, size, this->_ctx)){
				// Если отправка запроса на прокси-сервер не выполнена
				if(this->_unit->client.send(eid, buffer, size) != size){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
						#endif
					}
				}
			}
		// Если подключение не выполнено, выполняем функцию обратного вызова
		} else this->_callback.call <void (const bool)> ("connect", false);
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param      идентификатор клиента
 * @param size размер данных для записи
 *
 */
void awh::client::Socks5::write(const event::id_t, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const size_t)> ("write", size);
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param eid    идентификатор клиента
 * @param status новый статус клиента
 *
 */
void awh::client::Socks5::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::status_t)> ("state", status);
		// Если статус клиента изменился на "уничтожен"
		if((eid == this->_id.eid) && (status == event::status_t::DESTROYED)){
			// Обнуляем идентификатор клиента
			this->_id.eid = 0;
			// Если идентификатор UDP-клиента установлен
			if(this->_endpoint.udp.eid > 0){
				// Уничтожаем UDP-клиента
				this->_unit->client.destroy(this->_endpoint.udp.eid);
				// Обнуляем идентификатор UDP-клиента
				this->_endpoint.udp.eid = 0;
			}
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns.client->stop();
			// Останавливаем событие клиента
			else this->_unit->client.stop();
		}
	}
}
/**
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных клиента
 * @param size   размер данных клиента
 *
 */
void awh::client::Socks5::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если текущее состояние соответствует завершённому состоянию
			if(this->_ctx.state == proto::socks5_t::state_t::COMPLETED){
				// Если идентификатор клиента соответствует идентификатору socks5 клиента
				if(eid == this->_id.eid){
					// Если объект транспортного уровня безопасности установлен
					if((this->_coder != nullptr) && (this->_id.ctl > 0)){
						// Если данные не расшифрованы
						if(!this->_coder->decrypt(this->_id.ctl, buffer, size)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error_tls")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("TLS decryption data is failed", log_t::flag_t::WARNING);
								#endif
							}
						}
					// Если объект транспортного уровня безопасности не установлен, выполняем функцию обратного вызова
					} else this->_callback.call <void (const uint8_t *, const size_t)> ("read", buffer, size);
				// Если идентификатор клиента соответствует идентификатору UDP-клиента
				} else if(eid == this->_endpoint.udp.eid) {
					// Если парсинг данных от прокси-сервера выполнен успешно
					if(this->_socks5.parse(buffer, size, this->_endpoint.udp.ctx)){
						// Если установлен хост клиента, которому адресован UDP-пакет
						if(this->_endpoint.udp.ctx.host != nullptr){
							// Если объект транспортного уровня безопасности установлен
							if((this->_coder != nullptr) && (this->_id.ctl > 0)){
								// Если данные не расшифрованы
								if(!this->_coder->decrypt(this->_id.ctl, buffer + this->_endpoint.udp.ctx.size, size - this->_endpoint.udp.ctx.size)){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error_tls")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("TLS decryption data is failed", log_t::flag_t::WARNING);
										#endif
									}
								}
							// Если объект транспортного уровня безопасности не установлен, выполняем функцию обратного вызова
							} else this->_callback.call <void (const uint8_t *, const size_t)> ("read", buffer + this->_endpoint.udp.ctx.size, size - this->_endpoint.udp.ctx.size);
							// Выходим из функции
							return;
						}
					}
					// Создаём текст ошибки обработки UDP-пакета
					const string error = "Client for whom the UDP packet was received was not found";
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					// Выполняем функцию обратного вызова
					} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::NOT_FOUND, error);
				}
			// Если текущее состояние находится ещё в процессе общения с socks5 прокси-сервером
			} else {
				// Добавляем данные в буфер накопления SOCKS5-кадров
				this->_rx.insert(this->_rx.end(), buffer, buffer + size);
				// Если размер буфера превышает допустимый
				if(this->_rx.size() > proto::socks5_t::SOCKS5_RX_MAX_FRAME)
					// Выходим из функции
					return;
				// Определяем полный размер SOCKS5-кадра
				const size_t frame = this->_socks5.frameSize(this->_ctx.state, this->_rx.data(), this->_rx.size());
				// Если кадр ещё неполный
				if(frame == 0)
					// Выходим и ожидаем продолжение кадра
					return;
				// Если кадр некорректный
				if(frame == SIZE_MAX){
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const bool)> ("connect", false);
					// Выходим из функции
					return;
				}
				// Если парсинг данных от прокси-сервера выполнен успешно
				if(this->_socks5.parse(this->_rx.data(), frame, this->_ctx)){
					// Удаляем обработанный кадр из буфера
					this->_rx.erase(this->_rx.begin(), this->_rx.begin() + frame);
					/**
					 * Определяем состояние парсинга данных от прокси-сервера
					 */
					switch(static_cast <uint8_t> (this->_ctx.state)){
						// Если текущее состояние соответствует ошибке работе с прокси-сервером
						case static_cast <uint8_t> (proto::socks5_t::state_t::BROKEN): {
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status).c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status).c_str());
								#endif
							// Выполняем функцию обратного вызова
							} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::CONNECTION_FAIL, this->_socks5.statusMessage(this->_ctx.status).c_str());
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const bool)> ("connect", false);
						} break;
						// Если текущее состояние соответствует ожиданию выполнения подключения
						case static_cast <uint8_t> (proto::socks5_t::state_t::CONNECT): {
							// Устанавливаем пустую команду
							this->_ctx.command = proto::socks5_t::command_t::NONE;
							// Если клиент для работы с UDP-протоколом активирован
							if(this->_endpoint.udp.eid > 0){
								// Устанавливаем команду для UDP протокола
								this->_ctx.command = proto::socks5_t::command_t::UDP;
								// Получаем порт клиента для подключения, работающего через прокси
								uint16_t port = this->_unit->client.getSourcePort(this->_endpoint.udp.eid);
								/**
								 * Определяем тип данных сессии клиента, работающего через прокси
								 */
								switch(static_cast <uint8_t> (this->_unit->client.family(this->_endpoint.udp.eid))){
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (event::family_t::IPV4): {
										// Выполняем инициализацию объекта хоста
										this->_ctx.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										this->_ctx.host->type = net::type_t::IPV4;
										// Получаем объект хоста для подключения
										net::attr_net_t * host = awh_cast <net::attr_net_t *> (this->_ctx.host.get());
										// Устанавливаем внутренний IP-адрес клиента
										this->_unit->client.getAddress(this->_endpoint.udp.eid, event::address_t::IPV4, host->ip);
										// Если адрес клиента установлен а порт не установлен
										if((host->ip != nullptr) && (awh_cast <net::addr_net_ipv4_t *> (host->ip.get())->address > 0) && (port == 0)){
											// Получаем внутренний порт socks5-клиента
											port = this->_unit->client.getSourcePort(this->_id.eid);
											// Устанавливаем внутренний порт клиента
											this->_unit->client.setSourcePort(this->_endpoint.udp.eid, port);
										}
										// Устанавливаем внутренний порт клиента
										host->port = port;
									} break;
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (event::family_t::IPV6): {
										// Выполняем инициализацию объекта хоста
										this->_ctx.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										this->_ctx.host->type = net::type_t::IPV6;
										// Получаем объект хоста для подключения
										net::attr_net_t * host = awh_cast <net::attr_net_t *> (this->_ctx.host.get());
										// Устанавливаем внутренний IP-адрес клиента
										this->_unit->client.getAddress(this->_endpoint.udp.eid, event::address_t::IPV6, host->ip);
										// Если адрес клиента установлен а порт не установлен
										if((host->ip != nullptr) && (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (host->ip.get())->address[0], ::__awh_zero_ipv6__, 16) != 0) && (port == 0)){
											// Получаем внутренний порт socks5-клиента
											port = this->_unit->client.getSourcePort(this->_id.eid);
											// Устанавливаем внутренний порт клиента
											this->_unit->client.setSourcePort(this->_endpoint.udp.eid, port);
										}
										// Устанавливаем внутренний порт клиента
										host->port = port;
									} break;
								}
							// Если клиент для работы с UDP-протоколом не активирован
							} else if(this->_endpoint.attr != nullptr) {
								// Устанавливаем команду для TCP протокола
								this->_ctx.command = proto::socks5_t::command_t::CONNECT;
								/**
								 * Определяем тип данных сессии клиента, работающего через прокси
								 */
								switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
									// Если тип данных соответствует FQDN
									case static_cast <uint8_t> (net::type_t::FQDN): {
										// Выполняем инициализацию объекта хоста
										this->_ctx.host = make_unique <net::attr_fqdn_t> ();
										// Устанавливаем тип адреса события
										this->_ctx.host->type = net::type_t::FQDN;
										// Устанавливаем порт хоста для подключения
										awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port;
										// Устанавливаем доменное имя хоста для подключения
										awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain;
									} break;
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (net::type_t::IPV4): {
										// Выполняем инициализацию объекта хоста
										this->_ctx.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										this->_ctx.host->type = net::type_t::IPV4;
										// Устанавливаем порт хоста для подключения
										awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
										// Создаём новый объект адреса клиента IPv4
										awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
										// Устанавливаем IP-адрес хоста для подключения
										awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address;
									} break;
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (net::type_t::IPV6): {
										// Выполняем инициализацию объекта хоста
										this->_ctx.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										this->_ctx.host->type = net::type_t::IPV6;
										// Устанавливаем порт хоста для подключения
										awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
										// Создаём новый объект адреса клиента IPv6
										awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
										// Устанавливаем IP-адрес хоста для подключения
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address[0], 16);
									} break;
								}
							}
							// Если команда для работы с прокси-сервером установлена
							if(this->_ctx.command != proto::socks5_t::command_t::NONE){
								// Размер буфера данных
								size_t size = 0;
								// Буфер данных запроса
								uint8_t * buffer = nullptr;
								// Если извлечение буфера данных запроса выполнено успешно
								if(this->_socks5.buffer(&buffer, size, this->_ctx)){
									// Если отправка запроса на прокси-сервер не выполнена
									if(this->_unit->client.send(this->_id.eid, buffer, size) != size){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
											#endif
										}
									// Выходим из функции
									} else return;
								}
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const bool)> ("connect", false);
						} break;
						// Если текущее состояние соответствует успешному завершению рукопожатия
						case static_cast <uint8_t> (proto::socks5_t::state_t::HANDSHAKE): {
							// Порт хоста для подключения к удалённому серверу
							uint16_t port = 0;
							// Адрес хоста для подключения к удалённому серверу
							string target = "";
							// Если клиент для работы с UDP-протоколом активирован
							if(this->_endpoint.udp.eid > 0){
								// Если хост для подключения к удалённому серверу установлен
								if(this->_ctx.host != nullptr){
									/**
									 * Определяем тип данных адреса полученного от socks5 прокси-сервера
									 */
									switch(static_cast <uint8_t> (this->_ctx.host->type)){
										// Если тип данных соответствует FQDN
										case static_cast <uint8_t> (net::type_t::FQDN): {
											// Если DNS-резолвер подключён
											if(this->_dns.client != nullptr){
												// Выполняем разрешение хоста текущего сервера
												if(!this->_dns.client->resolve(this->_dns.id, this->_unit->client.family(this->_endpoint.udp.eid), awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->domain, this->_dns.alive.load(std::memory_order_acquire))){
													// Создаём текст ошибки разрешения хоста текущего сервера
													const string error = this->_fmk->format("It was not possible to obtain an IP address for the remote host \"%s\"", awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->domain.c_str());
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Записываем ошибку в лог
															this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Записываем ошибку в лог
															this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
														#endif
													// Выполняем функцию обратного вызова
													} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::NOT_FOUND, error);
												// Если разрешение хоста не выполнено, выходим
												} else return;
											// Если DNS-резолвер не подключён
											} else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Записываем ошибку в лог
													this->_log->debug("This client does not support working with domain names, since the DNS resolver is not found", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Записываем ошибку в лог
													this->_log->print("This client does not support working with domain names, since the DNS resolver is not found", log_t::flag_t::WARNING);
												#endif
											}
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const bool)> ("connect", false);
											// Выходим из функции
											return;
										}
										// Если тип данных соответствует IPv4
										case static_cast <uint8_t> (net::type_t::IPV4):
										// Если тип данных соответствует IPv6
										case static_cast <uint8_t> (net::type_t::IPV6): {
											// Устанавливаем порт и хост для подключения к удалённому серверу
											if(this->_unit->client.setTarget(this->_endpoint.udp.eid, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get()) &&
											   this->_unit->client.setTargetPort(this->_endpoint.udp.eid, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port)){
												// Выполняем фиксацию изменений для клиента, работающего через прокси
												if(this->_unit->client.commit(this->_endpoint.udp.eid)){
													// Выполняем запуск работы клиента, работающего через прокси
													if(this->_unit->client.launch(this->_endpoint.udp.eid)){
														// Получаем адрес хоста для подключения к удалённому серверу
														target = this->_unit->client.getTarget(this->_endpoint.udp.eid);
														// Получаем порт хоста для подключения к удалённому серверу
														port = this->_unit->client.getTargetPort(this->_endpoint.udp.eid);
													// Если запуск работы клиента, работающего через прокси, не выполнен
													} else {
														// Если функция обратного вызова не установлена
														if(!this->_callback.is("error")){
															/**
															 * Если включён режим отладки
															 */
															#if DEBUG_MODE
																// Записываем ошибку в лог
																this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
															/**
															 * Если режим отладки не включён
															 */
															#else
																// Записываем ошибку в лог
																this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, eid);
															#endif
														}
													}
												// Если фиксация настроек события сервера не выполнена
												} else {
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Записываем ошибку в лог
															this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Записываем ошибку в лог
															this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, eid);
														#endif
													}
												}
											// Если установка порта и адреса удалённого сервера для подключения не выполнена
											} else {
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Записываем ошибку в лог
														this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Записываем ошибку в лог
														this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, eid);
													#endif
												}
											}
										} break;
									}
								}
							// Если клиент для работы с UDP-протоколом не активирован
							} else if(this->_endpoint.attr != nullptr) {
								/**
								 * Определяем тип данных сессии клиента, работающего через прокси
								 */
								switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
									// Если тип данных соответствует FQDN
									case static_cast <uint8_t> (net::type_t::FQDN): {
										// Устанавливаем порт хоста для дальнейшего использования
										port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port;
										// Устанавливаем доменное имя хоста для дальнейшего использования
										target = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain;
									} break;
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (net::type_t::IPV4):
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (net::type_t::IPV6): {
										// Устанавливаем IP-адрес хоста для дальнейшего использования
										this->_unit->addr.source(awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get());
										// Устанавливаем IP-адрес хоста для дальнейшего использования
										target = ::move(static_cast <string> (this->_unit->addr));
										// Устанавливаем порт хоста для дальнейшего использования
										port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
									} break;
								}
							}
							// Если адрес хоста для подключения к удалённому серверу получен успешно
							if(!target.empty()){
								// Доменное имя хоста для callback ready
								string domain = target;
								// Если конечная точка клиента установлена
								if((this->_endpoint.attr != nullptr) && (this->_endpoint.attr->type == net::type_t::FQDN))
									// Устанавливаем доменное имя хоста
									domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain;
								// Устанавливаем состояние клиента как "завершённый"
								this->_ctx.state = proto::socks5_t::state_t::COMPLETED;
								// Очищаем буфер накопления SOCKS5-кадров
								this->_rx.clear();
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", this->_unit->client.family(eid), domain, target);
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const string &, const uint16_t)> ("launch", target, port);
								// Если объект транспортного уровня безопасности установлен
								if((this->_coder != nullptr) && (this->_id.ctl > 0)){
									// Если рукопожатие TLS не выполнено
									if(!this->_coder->handshake(this->_id.ctl)){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error_tls")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
											#endif
										}
									// Выходим из функции
									} else return;
								// Если объект транспортного уровня безопасности не установлен
								} else {
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const bool)> ("connect", true);
									// Выходим из функции
									return;
								}
							// Если адрес хоста для подключения к удалённому серверу не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Client event ID not found", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Client event ID not found", log_t::flag_t::WARNING);
								#endif
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const bool)> ("connect", false);
						} break;
						// В остальных случаях, проходим процедуру общения с сервером в автоматическом режиме
						default: {
							// Размер буфера данных
							size_t size = 0;
							// Буфер данных запроса
							uint8_t * buffer = nullptr;
							// Если извлечение буфера данных запроса выполнено успешно
							if(this->_socks5.buffer(&buffer, size, this->_ctx)){
								// Если отправка запроса на прокси-сервер не выполнена
								if(this->_unit->client.send(this->_id.eid, buffer, size) != size){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
										#endif
									}
								// Выходим из функции
								} else return;
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const bool)> ("connect", false);
						}
					}
				// Если парсинг полного SOCKS5-кадра не выполнен
				} else {
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Failed to parse data from proxy server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Failed to parse data from proxy server", log_t::flag_t::WARNING);
						#endif
					// Выполняем функцию обратного вызова
					} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::CONNECTION_FAIL, "Failed to parse data from proxy server");
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const bool)> ("connect", false);
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод разрешения доменного имени удалённого хоста в сетевой адрес
 *
 * @param        идентификатор DNS-запроса
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для разрешения
 * @param addr   указатель на структуру для хранения результата разрешения
 *
 */
void awh::client::Socks5::resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если DNS-резолвер находится в рабочем состоянии
		if(this->_dns.client->working()){
			/**
			 * В зависимости от статуса события клиента выполняем запуск
			 */
			switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (&this->_unit->client)->status(this->_id.eid))){
				// Если событие клиента не запущено, запускаем его
				case static_cast <uint8_t> (event::status_t::NONE): {
					// Устанавливаем адрес хоста целевой машины для клиента
					if(this->_unit->client.setTarget(this->_id.eid, addr)){
						// Если событие клиента не запущено, запускаем его
						if(this->_unit->client.commit(this->_id.eid)){
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", family, domain, this->_unit->client.getTarget(this->_id.eid));
							// Запускаем клиента
							this->_unit->client.start();
						}
					}
				} break;
				// Если клиент находится в подключённом состоянии
				case static_cast <uint8_t> (event::status_t::CONNECTED): {
					// Устанавливаем порт и хост для подключения к удалённому серверу
					if(this->_unit->client.setTarget(this->_endpoint.udp.eid, addr) &&
					   this->_unit->client.setTargetPort(this->_endpoint.udp.eid, awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->port)){
						// Выполняем фиксацию изменений для клиента, работающего через прокси
						if(this->_unit->client.commit(this->_endpoint.udp.eid)){
							// Выполняем запуск работы клиента, работающего через прокси
							if(this->_unit->client.launch(this->_endpoint.udp.eid)){
								// Устанавливаем состояние клиента как "завершённый"
								this->_ctx.state = proto::socks5_t::state_t::COMPLETED;
								// Очищаем буфер накопления SOCKS5-кадров
								this->_rx.clear();
								// Получаем адрес хоста для подключения к удалённому серверу
								const string & target = this->_unit->client.getTarget(this->_endpoint.udp.eid);
								// Получаем порт хоста для подключения к удалённому серверу
								const uint16_t port = this->_unit->client.getTargetPort(this->_endpoint.udp.eid);
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", family, domain, target);
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const string &, const uint16_t)> ("launch", target, port);
								// Если объект транспортного уровня безопасности установлен
								if((this->_coder != nullptr) && (this->_id.ctl > 0)){
									// Если рукопожатие TLS не выполнено
									if(!this->_coder->handshake(this->_id.ctl)){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error_tls")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
											#endif
										}
									}
								// Если объект транспортного уровня безопасности не установлен, выполняем функцию обратного вызова
								} else this->_callback.call <void (const bool)> ("connect", true);
							// Если запуск работы клиента, работающего через прокси, не выполнен
							} else {
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
									#endif
								}
							}
						// Если фиксация настроек события сервера не выполнена
						} else {
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
								#endif
							}
						}
					// Если установка порта и адреса удалённого сервера для подключения не выполнена
					} else {
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
							#endif
						}
					}
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param       идентификатор TLS
 * @param state состояние TLS
 *
 */
void awh::client::Socks5::stateTLS(const tls::coder_t::id_t, const tls::coder_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const tls::coder_t::state_t)> ("state_tls", state);
		// Если состояние рукопожатия успешно завершено
		if(state == tls::coder_t::state_t::HANDSHAKED)
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const bool)> ("connect", true);
	}
}
/**
 * @brief Метод получения событий шифрования/дешифрования данных TLS
 *
 * @param        идентификатор TLS
 * @param event  тип события TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 *
 */
void awh::client::Socks5::processTLS(const tls::coder_t::id_t, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION): {
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0){
						// Сбрасываем размер буфера полезной нагрузки
						::__awh_size__ = 0;
						/**
						 * Определяем тип данных сессии клиента, работающего через прокси
						 */
						switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
							// Если тип данных соответствует FQDN
							case static_cast <uint8_t> (net::type_t::FQDN): {
								// Если хост для подключения к удалённому серверу не установлен или его тип данных не соответствует FQDN
								if((this->_endpoint.udp.ctx.host == nullptr) || (this->_endpoint.udp.ctx.host->type != net::type_t::FQDN)){
									// Выполняем инициализацию объекта хоста
									this->_endpoint.udp.ctx.host = make_unique <net::attr_fqdn_t> ();
									// Устанавливаем тип адреса события
									this->_endpoint.udp.ctx.host->type = net::type_t::FQDN;
								}
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_fqdn_t *> (this->_endpoint.udp.ctx.host.get())->port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port;
								// Устанавливаем доменное имя хоста для подключения
								awh_cast <net::attr_fqdn_t *> (this->_endpoint.udp.ctx.host.get())->domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain;
							} break;
							// Если тип данных соответствует IPv4
							case static_cast <uint8_t> (net::type_t::IPV4): {
								// Если хост для подключения к удалённому серверу не установлен или его тип данных не соответствует IPv4
								if((this->_endpoint.udp.ctx.host == nullptr) || (this->_endpoint.udp.ctx.host->type != net::type_t::IPV4)){
									// Выполняем инициализацию объекта хоста
									this->_endpoint.udp.ctx.host = make_unique <net::attr_net_t> ();
									// Устанавливаем тип адреса события
									this->_endpoint.udp.ctx.host->type = net::type_t::IPV4;
									// Создаём новый объект адреса клиента IPv4
									awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
								}
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
								// Устанавливаем IP-адрес хоста для подключения
								awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address;
							} break;
							// Если тип данных соответствует IPv6
							case static_cast <uint8_t> (net::type_t::IPV6): {
								// Если хост для подключения к удалённому серверу не установлен или его тип данных не соответствует IPv6
								if((this->_endpoint.udp.ctx.host == nullptr) || (this->_endpoint.udp.ctx.host->type != net::type_t::IPV6)){
									// Выполняем инициализацию объекта хоста
									this->_endpoint.udp.ctx.host = make_unique <net::attr_net_t> ();
									// Устанавливаем тип адреса события
									this->_endpoint.udp.ctx.host->type = net::type_t::IPV6;
									// Создаём новый объект адреса клиента IPv6
									awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
								}
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
								// Устанавливаем IP-адрес хоста для подключения
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address[0], 16);
							} break;
						}
						// Размер буфера данных
						size_t length = 0;
						// Буфер данных запроса
						uint8_t * data = nullptr;
						// Если извлечение буфера данных запроса выполнено успешно
						if(this->_socks5.buffer(&data, length, this->_endpoint.udp.ctx)){
							/**
							 * Определяем тип данных сессии клиента, работающего через прокси
							 */
							switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
								// Если тип данных соответствует FQDN
								case static_cast <uint8_t> (net::type_t::FQDN):
								// Если тип данных соответствует IPv6
								case static_cast <uint8_t> (net::type_t::IPV6):
									// Устанавливаем размер буфера полезной нагрузки для отправки
									::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV6_PAYLOAD_SIZE));
								break;
								// Если тип данных соответствует IPv4
								case static_cast <uint8_t> (net::type_t::IPV4):
									// Устанавливаем размер буфера полезной нагрузки для отправки
									::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
								break;
							}
							// Если размер буфера полезной нагрузки достаточен для отправки всех данных
							if(::__awh_size__ == (size + length)){
								// Копируем данные запроса в буфер полезной нагрузки
								::memcpy(&::__awh_buffer__[0], data, length);
								// Добавляем к буферу данных для отправки полезную нагрузку
								::memcpy(&::__awh_buffer__[length], buffer, size);
							// Если размер буфера полезной нагрузки недостаточен для отправки всех данных
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Message sent by the UDP is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
								#endif
								// Выходим из функции
								return;
							}
						// Если извлечение буфера данных запроса не выполнено
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Failed to generate buffer for UDP packet", log_t::flag_t::WARNING);
							#endif
						}
						// Если буфер полезной нагрузки для отправки не пустой
						if(::__awh_size__ > 0){
							// Если отправка запроса на прокси-сервер не выполнена
							if(this->_unit->client.send(this->_endpoint.udp.eid, ::__awh_buffer__, ::__awh_size__) != ::__awh_size__){
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
									#endif
								}
							}
						}
					// Если клиент для работы с UDP протоколом не инициализирован
					} else {
						// Отправляем данные обратно клиенту, которые были зашифрованы TLS
						if(!this->_unit->client.send(this->_id.eid, reinterpret_cast <const char *> (buffer), size)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
								#endif
							}
						}
					}
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION):
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const uint8_t *, const size_t)> ("read", buffer, size);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @return результат выполнения приостановки работы
 *
 */
bool awh::client::Socks5::pause() noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если идентификатор клиента установлен
			if(this->_id.eid > 0){
				// Приостанавливаем событие клиента
				if((result = this->_unit->client.pause(this->_id.eid))){
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0)
						// Приостанавливаем событие клиента для конечной точки
						result = this->_unit->client.pause(this->_endpoint.udp.eid);
				}
			// Если идентификатор клиента не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @return результат выполнения возобновления работы
 *
 */
bool awh::client::Socks5::resume() noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если идентификатор клиента установлен
			if(this->_id.eid > 0){
				// Возобновляем работу события клиента
				if((result = this->_unit->client.resume(this->_id.eid))){
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0)
						// Возобновляем работу события клиента для конечной точки
						result = this->_unit->client.resume(this->_endpoint.udp.eid);
				}
			// Если идентификатор клиента не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод мультиподключения клиентов к удалённым хостам (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения подключения
 *
 */
bool awh::client::Socks5::connect() noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения отключения
 *
 */
bool awh::client::Socks5::disconnect() noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения данных от сервера
 *
 * @return результат получения данных
 *
 */
bool awh::client::Socks5::recv() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если клиент для работы с UDP протоколом не инициализирован
			if(this->_endpoint.udp.eid == 0)
				// Получаем данные от сервера
				return this->_unit->client.recv(this->_id.eid);
			// Если клиент для работы с UDP протоколом инициализирован, получаем данные с него
			else return this->_unit->client.recv(this->_endpoint.udp.eid);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных серверу
 *
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных серверу
 *
 */
size_t awh::client::Socks5::send(const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если клиент для работы с UDP протоколом не инициализирован
			if(this->_endpoint.udp.eid == 0){
				// Если идентификатор TLS и объект TLS установлены
				if((this->_id.ctl > 0) && (this->_coder != nullptr)){
					// Если шифрование данных TLS выполнено успешно
					if(this->_coder->encrypt(this->_id.ctl, buffer, size))
						// Возвращаем размер отправленных данных
						return size;
					// Возвращаем значение по умолчанию
					return 0;
				}
				// Выполняем отправку данных серверу
				return this->_unit->client.send(this->_id.eid, buffer, size);
			// Если клиент для работы с UDP протоколом инициализирован
			} else {
				// Если идентификатор TLS и объект TLS установлены
				if((this->_id.ctl > 0) && (this->_coder != nullptr)){
					// Если шифрование данных TLS выполнено успешно
					if(this->_coder->encrypt(this->_id.ctl, buffer, size))
						// Возвращаем размер отправленных данных
						return size;
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * Определяем тип данных сессии клиента, работающего через прокси
				 */
				switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
					// Если тип данных соответствует FQDN
					case static_cast <uint8_t> (net::type_t::FQDN): {
						// Если объект хоста для подключения не инициализирован или тип данных сессии клиента, работающего через прокси, не соответствует FQDN
						if((this->_endpoint.udp.ctx.host == nullptr) || (this->_endpoint.udp.ctx.host->type != net::type_t::FQDN)){
							// Выполняем инициализацию объекта хоста
							this->_endpoint.udp.ctx.host = make_unique <net::attr_fqdn_t> ();
							// Устанавливаем тип адреса события
							this->_endpoint.udp.ctx.host->type = net::type_t::FQDN;
						}
						// Устанавливаем порт хоста для подключения
						awh_cast <net::attr_fqdn_t *> (this->_endpoint.udp.ctx.host.get())->port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port;
						// Устанавливаем доменное имя хоста для подключения
						awh_cast <net::attr_fqdn_t *> (this->_endpoint.udp.ctx.host.get())->domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain;
					} break;
					// Если тип данных соответствует IPv4
					case static_cast <uint8_t> (net::type_t::IPV4): {
						// Если объект хоста для подключения не инициализирован или тип данных сессии клиента, работающего через прокси, не соответствует IPv4
						if((this->_endpoint.udp.ctx.host == nullptr) || (this->_endpoint.udp.ctx.host->type != net::type_t::IPV4)){
							// Выполняем инициализацию объекта хоста
							this->_endpoint.udp.ctx.host = make_unique <net::attr_net_t> ();
							// Устанавливаем тип адреса события
							this->_endpoint.udp.ctx.host->type = net::type_t::IPV4;
							// Создаём новый объект адреса клиента IPv4
							awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
						}
						// Устанавливаем порт хоста для подключения
						awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
						// Устанавливаем IP-адрес хоста для подключения
						awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address;
					} break;
					// Если тип данных соответствует IPv6
					case static_cast <uint8_t> (net::type_t::IPV6): {
						// Если объект хоста для подключения не инициализирован или тип данных сессии клиента, работающего через прокси, не соответствует IPv6
						if((this->_endpoint.udp.ctx.host == nullptr) || (this->_endpoint.udp.ctx.host->type != net::type_t::IPV6)){
							// Выполняем инициализацию объекта хоста
							this->_endpoint.udp.ctx.host = make_unique <net::attr_net_t> ();
							// Устанавливаем тип адреса события
							this->_endpoint.udp.ctx.host->type = net::type_t::IPV6;
							// Создаём новый объект адреса клиента IPv6
							awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
						}
						// Устанавливаем порт хоста для подключения
						awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
						// Устанавливаем IP-адрес хоста для подключения
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.udp.ctx.host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address[0], 16);
					} break;
				}
				// Размер буфера данных
				size_t length = 0;
				// Буфер данных запроса
				uint8_t * data = nullptr;
				// Если извлечение буфера данных запроса выполнено успешно
				if(this->_socks5.buffer(&data, length, this->_endpoint.udp.ctx)){
					/**
					 * Определяем тип данных сессии клиента, работающего через прокси
					 */
					switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
						// Если тип данных соответствует FQDN
						case static_cast <uint8_t> (net::type_t::FQDN):
						// Если тип данных соответствует IPv6
						case static_cast <uint8_t> (net::type_t::IPV6):
							// Устанавливаем размер буфера полезной нагрузки для отправки
							::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV6_PAYLOAD_SIZE));
						break;
						// Если тип данных соответствует IPv4
						case static_cast <uint8_t> (net::type_t::IPV4):
							// Устанавливаем размер буфера полезной нагрузки для отправки
							::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
						break;
					}
					// Если размер буфера полезной нагрузки достаточен для отправки всех данных
					if(::__awh_size__ == (size + length)){
						// Копируем данные запроса в буфер полезной нагрузки
						::memcpy(&::__awh_buffer__[0], data, length);
						// Добавляем к буферу данных для отправки полезную нагрузку
						::memcpy(&::__awh_buffer__[length], buffer, size);
						// Выполняем отправку данных серверу
						return this->_unit->client.send(this->_endpoint.udp.eid, ::__awh_buffer__, ::__awh_size__);
					// Если размер буфера полезной нагрузки недостаточен для отправки всех данных
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Message sent by the UDP is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
						#endif
					}
				// Если извлечение буфера данных запроса не выполнено
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Failed to generate buffer for UDP packet", log_t::flag_t::WARNING);
					#endif
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод объединения данных между клиентом и другим событием (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения объединения
 *
 */
bool awh::client::Socks5::splice(const event::id_t, const event::direct_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 */
bool awh::client::Socks5::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0){
			// Устанавливаем пропускную способность клиента
			if((result = this->_unit->client.bandwidth(this->_id.eid, limiting, bandwidth))){
				// Если клиент для работы с UDP протоколом инициализирован
				if(this->_endpoint.udp.eid > 0)
					// Устанавливаем пропускную способность клиента для конечной точки
					result = this->_unit->client.bandwidth(this->_endpoint.udp.eid, limiting, bandwidth);
			}
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения установки
 *
 */
bool awh::client::Socks5::membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения установки
 *
 */
bool awh::client::Socks5::membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров авторизации
 *
 * @param username имя пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 *
 */
void awh::client::Socks5::setUser(const string & username, const string & password) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working())
		// Устанавливаем параметры авторизации для объекта клиента
		this->_socks5.setUser(username, password);
}
/**
 * @brief Метод установки исходящего адреса для UDP-клиента
 *
 * @param addr исходящий адрес для UDP-клиента
 * @return 	   результат выполнения установки исходящего адреса для UDP-клиента
 *
 */
bool awh::client::Socks5::udp(const net::attr_net_t * addr) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если исходящий адрес для UDP-клиента не пустой
			if((addr != nullptr) && (addr->ip != nullptr)){
				// Если клиент для работы с UDP протоколом инициализирован
				if(this->_endpoint.udp.eid > 0)
					// Удаляем клиент, принадлежащий пиру
					this->_unit->client.destroy(this->_endpoint.udp.eid);
				/**
				 * Определяем тип полученного IP-адреса
				 */
				switch(addr->ip->size){
					// Для типа IPv4
					case 4: {
						// Выполняем создание клиента для подключения к удалённому серверу
						this->_endpoint.udp.eid = this->_unit->client.issue(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
						// Если клиент для работы с UDP протоколом инициализирован
						if(this->_endpoint.udp.eid > 0){
							// Устанавливаем опции события
							if(this->_unit->client.setOptions(this->_endpoint.udp.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
								// Устанавливаем адрес с которого будет выполняться подключение к удалённому серверу
								if(!(result = (this->_unit->client.setAddress(this->_endpoint.udp.eid, event::address_t::IPV4, addr->ip.get()) && this->_unit->client.setSourcePort(this->_endpoint.udp.eid, addr->port)))){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										// Устанавливаем исходящий адрес для UDP-клиента
										this->_unit->addr.source(addr->ip.get());
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, static_cast <string> (this->_unit->addr).c_str(), this->_endpoint.udp.eid);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, static_cast <string> (this->_unit->addr).c_str(), this->_endpoint.udp.eid);
										#endif
									}
								// Если установка опций события выполнена, возвращаем положительный результат
								} else return result;
							// Если установка опций события не выполнена
							} else {
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, this->_endpoint.udp.eid);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
									#endif
								}
							}
							// Удаляем клиент, принадлежащий пиру
							this->_unit->client.destroy(this->_endpoint.udp.eid);
						}
					} break;
					// Для типа IPv6
					case 16: {
						// Выполняем создание клиента для подключения к удалённому серверу
						this->_endpoint.udp.eid = this->_unit->client.issue(event::family_t::IPV6, event::type_t::DATAGRAM, event::protocol_t::UDP);
						// Если клиент для работы с UDP протоколом инициализирован
						if(this->_endpoint.udp.eid > 0){
							// Устанавливаем опции события
							if(this->_unit->client.setOptions(this->_endpoint.udp.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
								// Устанавливаем адрес с которого будет выполняться подключение к удалённому серверу
								if(!(result = (this->_unit->client.setAddress(this->_endpoint.udp.eid, event::address_t::IPV6, addr->ip.get()) && this->_unit->client.setSourcePort(this->_endpoint.udp.eid, addr->port)))){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										// Устанавливаем исходящий адрес для UDP-клиента
										this->_unit->addr.source(addr->ip.get());
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, static_cast <string> (this->_unit->addr).c_str(), this->_endpoint.udp.eid);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, static_cast <string> (this->_unit->addr).c_str(), this->_endpoint.udp.eid);
										#endif
									}
								// Если установка опций события выполнена, возвращаем положительный результат
								} else return result;
							// Если установка опций события не выполнена
							} else {
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, this->_endpoint.udp.eid);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
									#endif
								}
							}
							// Удаляем клиент, принадлежащий пиру
							this->_unit->client.destroy(this->_endpoint.udp.eid);
						}
					} break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки исходящего адреса для UDP-клиента
 *
 * @param addr исходящий адрес для UDP-клиента
 * @param port исходящий порт для UDP-клиента
 * @return     результат выполнения установки исходящего адреса для UDP-клиента
 *
 */
bool awh::client::Socks5::udp(string_view addr, const uint16_t port) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если исходящий адрес для UDP-клиента не пустой
			if(!addr.empty()){
				// Выполняем парсинг IP-адреса
				if(this->_unit->addr.parse(addr)){
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0)
						// Удаляем клиент, принадлежащий пиру
						this->_unit->client.destroy(this->_endpoint.udp.eid);
					/**
					 * Определяем тип полученного IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_unit->addr.type())){
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
							// Выполняем создание клиента для подключения к удалённому серверу
							this->_endpoint.udp.eid = this->_unit->client.issue(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
							// Если клиент для работы с UDP протоколом инициализирован
							if(this->_endpoint.udp.eid > 0){
								// Устанавливаем опции события
								if(this->_unit->client.setOptions(this->_endpoint.udp.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
									// Устанавливаем адрес с которого будет выполняться подключение к удалённому серверу
									if(!(result = (this->_unit->client.setAddress(this->_endpoint.udp.eid, event::address_t::IPV4, this->_unit->addr.source().get()) && this->_unit->client.setSourcePort(this->_endpoint.udp.eid, port)))){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::WARNING, addr, this->_endpoint.udp.eid);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, addr, this->_endpoint.udp.eid);
											#endif
										}
									// Если установка опций события выполнена, возвращаем положительный результат
									} else return result;
								// Если установка опций события не выполнена
								} else {
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
										#endif
									}
								}
								// Удаляем клиент, принадлежащий пиру
								this->_unit->client.destroy(this->_endpoint.udp.eid);
							}
						} break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Выполняем создание клиента для подключения к удалённому серверу
							this->_endpoint.udp.eid = this->_unit->client.issue(event::family_t::IPV6, event::type_t::DATAGRAM, event::protocol_t::UDP);
							// Если клиент для работы с UDP протоколом инициализирован
							if(this->_endpoint.udp.eid > 0){
								// Устанавливаем опции события
								if(this->_unit->client.setOptions(this->_endpoint.udp.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC)){
									// Устанавливаем адрес с которого будет выполняться подключение к удалённому серверу
									if(!(result = (this->_unit->client.setAddress(this->_endpoint.udp.eid, event::address_t::IPV6, this->_unit->addr.source().get()) && this->_unit->client.setSourcePort(this->_endpoint.udp.eid, port)))){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Записываем ошибку в лог
												this->_log->debug("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::WARNING, addr, this->_endpoint.udp.eid);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												this->_log->print("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, addr, this->_endpoint.udp.eid);
											#endif
										}
									// Если установка опций события выполнена, возвращаем положительный результат
									} else return result;
								// Если установка опций события не выполнена
								} else {
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
										#endif
									}
								}
								// Удаляем клиент, принадлежащий пиру
								this->_unit->client.destroy(this->_endpoint.udp.eid);
							}
						} break;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки конечной точки клиента
 *
 * @param attr параметры подключения для установки конечной точки
 * @return     результат выполнения установки конечной точки
 *
 */
bool awh::client::Socks5::endpoint(const net::attr_t * attr) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если указатель на адрес конечной точки не пустой
			if(attr != nullptr){
				// Сбрасываем объект атрибутов конечной точки для идентификатора события клиента
				this->_endpoint.attr.reset(nullptr);
				/**
				 * Определяем тип полученного IP-адреса
				 */
				switch(static_cast <uint8_t> (attr->type)){
					// Для типа FQDN
					case static_cast <uint8_t> (net::type_t::FQDN): {
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_endpoint.attr = make_unique <net::attr_fqdn_t> ();
						// Устанавливаем тип параметров подключения для идентификатора события клиента
						this->_endpoint.attr->type = net::type_t::FQDN;
						// Устанавливаем полученный порт
						awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port = awh_cast <const net::attr_fqdn_t *> (attr)->port;
						// Устанавливаем полученное доменное имя
						awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain = awh_cast <const net::attr_fqdn_t *> (attr)->domain;
					} break;
					// Для типа IPv4
					case static_cast <uint8_t> (net::type_t::IPV4): {
						// Получаем объект переданных атрибутов сетевого адреса
						const net::attr_net_t * source = awh_cast <const net::attr_net_t *> (attr);
						// Если в переданных атрибутах IP-адрес не установлен
						if(source->ip == nullptr)
							// Прерываем выполнение
							break;
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_endpoint.attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения для идентификатора события клиента
						this->_endpoint.attr->type = net::type_t::IPV4;
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port = source->port;
						// Создаём новый объект адреса клиента IPv4
						awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем полученный IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (source->ip.get())->address;
					} break;
					// Для типа IPv6
					case static_cast <uint8_t> (net::type_t::IPV6): {
						// Получаем объект переданных атрибутов сетевого адреса
						const net::attr_net_t * source = awh_cast <const net::attr_net_t *> (attr);
						// Если в переданных атрибутах IP-адрес не установлен
						if(source->ip == nullptr)
							// Прерываем выполнение
							break;
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_endpoint.attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения для идентификатора события клиента
						this->_endpoint.attr->type = net::type_t::IPV6;
						// Создаём новый объект адреса клиента IPv6
						awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port = source->port;
						// Устанавливаем полученный IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (source->ip.get())->address[0], 16);
					} break;
				}
				// Возвращаем результат наличия объекта атрибутов конечной точки для идентификатора события клиента
				return ((this->_endpoint.attr != nullptr) && (this->_endpoint.attr->type != net::type_t::NONE));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки конечной точки клиента
 *
 * @param addr адрес хоста для установки
 * @param port порт хоста для установки
 * @return     результат выполнения установки конечной точки
 *
 */
bool awh::client::Socks5::endpoint(string_view addr, const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если адрес для установки конечной точки не пустой
			if(!addr.empty()){
				// Сбрасываем объект атрибутов конечной точки для идентификатора события клиента
				this->_endpoint.attr.reset(nullptr);
				// Выполняем парсинг IP-адреса
				if(this->_unit->addr.parse(addr)){
					/**
					 * Определяем тип полученного IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_unit->addr.type())){
						// Для типа FQDN
						case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
							// Создаём объект параметров подключения для идентификатора события клиента
							this->_endpoint.attr = make_unique <net::attr_fqdn_t> ();
							// Устанавливаем тип параметров подключения для идентификатора события клиента
							this->_endpoint.attr->type = net::type_t::FQDN;
							// Устанавливаем полученный порт
							awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port = port;
							// Устанавливаем полученное доменное имя хоста для подключения
							awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain = addr;
						} break;
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
							// Создаём объект параметров подключения для идентификатора события клиента
							this->_endpoint.attr = make_unique <net::attr_net_t> ();
							// Устанавливаем тип параметров подключения для идентификатора события клиента
							this->_endpoint.attr->type = net::type_t::IPV4;
							// Устанавливаем полученный порт
							awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port = port;
							// Устанавливаем полученный IP-адрес
							awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip = ::move(this->_unit->addr.source(net_addr_t::endian_t::LITTLE));
						} break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Создаём объект параметров подключения для идентификатора события клиента
							this->_endpoint.attr = make_unique <net::attr_net_t> ();
							// Устанавливаем тип параметров подключения для идентификатора события клиента
							this->_endpoint.attr->type = net::type_t::IPV6;
							// Устанавливаем полученный порт
							awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port = port;
							// Устанавливаем полученный IP-адрес
							awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip = ::move(this->_unit->addr.source(net_addr_t::endian_t::LITTLE));
						} break;
					}
				// Если распарсить адрес не удалось, значит будем считать, что это FQDN
				} else {
					// Создаём объект параметров подключения для идентификатора события клиента
					this->_endpoint.attr = make_unique <net::attr_fqdn_t> ();
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					this->_endpoint.attr->type = net::type_t::FQDN;
					// Устанавливаем полученный порт
					awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port = port;
					// Устанавливаем полученное доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain = addr;
				}
				// Возвращаем результат наличия объекта атрибутов конечной точки для идентификатора события клиента
				return (this->_endpoint.attr != nullptr);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::client::Socks5::Socks5(const fmk_t * fmk, const log_t * log) noexcept :
 client_t(fmk, log), _socks5(fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::client::Socks5::Socks5(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(dns, fmk, log), _socks5(fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param ctl   идентификатор контекста безопасности
 * @param coder объект транспортного уровня безопасности
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 *
 */
awh::client::Socks5::Socks5(const tls::coder_t::id_t ctl, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(ctl, coder, fmk, log), _socks5(fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param ctl   идентификатор контекста безопасности
 * @param coder объект транспортного уровня безопасности
 * @param dns   объект DNS-резолвера
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 *
 */
awh::client::Socks5::Socks5(const tls::coder_t::id_t ctl, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(ctl, coder, dns, fmk, log), _socks5(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::client::Socks5::~Socks5() noexcept {}
