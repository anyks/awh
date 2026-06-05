/**
 * @file: socks5.cpp
 * @date: 2026-05-26
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
 * Если размер MTU для UDP сообщений в IPv4 не определён
 */
#ifndef AWH_MTU_UDP_IPV4_PAYLOAD_SIZE
	/**
	 * Устанавливаем размер MTU для UDP сообщений в 1500 байт
	 * Стандартный размер Ethernet MTU минус заголовки IP и UDP = 1472 - 72 = 1400 байт, с запасом на возможную инкапсуляцию
	 *
	 * 1500 - 20 (IP) - 8 (UDP) = 1472 максимум без фрагментации
	 * Запас 72 байта на возможную инкапсуляцию (туннели, провайдерские заголовки)
	 * Больше → фрагментация → потеря пакетов
	 */
	#define AWH_MTU_UDP_IPV4_PAYLOAD_SIZE 0x578
#endif

/**
 * Если размер MTU для UDP сообщений в IPv6 не определён
 */
#ifndef AWH_MTU_UDP_IPV6_PAYLOAD_SIZE
	/**
	 * Устанавливаем размер MTU для UDP сообщений в 1500 байт
	 * Стандартный размер Ethernet MTU минус заголовки IP и UDP = 1452 - 72 = 1380 байт, с запасом на возможную инкапсуляцию
	 *
	 * 1500 - 40 (IPv6) - 8 (UDP) = 1452 максимум без фрагментации
	 * Запас 72 байта на инкапсуляцию (туннели часто используют двойную инкапсуляцию)
	 * IPv6 не фрагментирует на маршрутизаторах → фрагментированные пакеты отбрасываются
	 */
	#define AWH_MTU_UDP_IPV6_PAYLOAD_SIZE 0x564
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <client/socks5.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Инкапсулируем статические параметры в пространство имён
 */
namespace {
	/**
	 * @brief Размер данных в буфере
	 *
	 */
	thread_local size_t __awh_size__ = 0;

	/**
	 * @brief Буфер временного хранения данных UDP сообщений
	 *
	 */
	thread_local uint8_t __awh_buffer__[AWH_MTU_UDP_IPV4_PAYLOAD_SIZE] = {0};
};

/**
 * @brief Метод изменения статуса клиента
 *
 * @param status новый статус клиента
 * @param state  новое временное состояние клиента
 */
void awh::client::Socks5::status(const event::status_t status, const state_t state) noexcept {
	/**
	 * Временное состояние клиента
	 */
	switch(static_cast <uint8_t> (state)){
		// Если мы получили статус события клиента
		case static_cast <uint8_t> (state_t::CLIENT): {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Выполняем подключение клиента к удалённому серверу
					if(!this->_client->connect(this->_eid)){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to connect to remote server", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (state)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
							#endif
						}
					// Если подключение выполнено
					} else {
						// Выполняем запуск работы клиента, если клиент не запущен
						if(!this->_client->launch(this->_eid)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (state)), log_t::flag_t::WARNING, this->_eid);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, this->_eid);
								#endif
							}
						// Если клиент запущен удачно
						} else {
							// Если функция обратного вызова установлена
							if(this->_callback.is("launch"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", this->_eid, this->_client->getTarget(this->_eid), this->_client->getPort(this->_eid));
							// Если DNS-резолвер подключён
							if(this->_dns != nullptr)
								// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
								this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &client::Socks5::resolve, this, _1, _2, _3, _4);
						}
					}
				} break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED): {
					// Если функция обратного вызова установлена
					if(this->_callback.is("status"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("status", status);
				} break;
			}
		} break;
		// Если мы получили статус события DNS-резолвера
		case static_cast <uint8_t> (state_t::RESOLVER): {
			/**
			 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие DNS-резолвера запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					/**
					 * Определяем принадлежит ли хост, к IP-адресу
					 */
					switch(static_cast <uint8_t> (this->_addr.host(this->_host))){
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Устанавливаем адрес хоста целевой машины для клиента
							if(this->_client->setTarget(this->_eid, this->_host)){
								/**
								 * В зависимости от статуса события клиента выполняем запуск
								 */
								switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_client)->status(this->_eid))){
									// Если событие клиента не запущено, запускаем его
									case static_cast <uint8_t> (event::status_t::NONE): {
										// Если событие клиента не запущено, запускаем его
										if(this->_client->commit(this->_eid)){
											// Если функция обратного вызова установлена
											if(this->_callback.is("ready"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, this->_client->family(this->_eid), this->_host, this->_client->getTarget(this->_eid));
											// Запускаем клиента
											this->_client->start();
										}
									} break;
									// Если событие клиента инициализировано, запускаем его
									case static_cast <uint8_t> (event::status_t::INITIAL):
									// Если событие находится в состоянии успешного подключения
									case static_cast <uint8_t> (event::status_t::SUCCESS):
										// Запускаем клиента
										this->_client->start();
									break;
								}
							}
							// Выходим из функции
							return;
						}
					}
					// Выполняем резолвинг доменного имени
					if(!this->_dns->resolve(this->_dns->issue(), awh_cast <unit::unit_t *> (this->_client)->family(this->_eid), this->_host, this->_timeoutDNS.load(std::memory_order_acquire))){
						// Создаём текст ошибки резолвинга доменного имени
						const string error = this->_fmk->format("It was not possible to obtain an IP address for the domain name \"%s\"", this->_host.c_str());
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (state)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						// Выполняем функцию обратного вызова
						} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_eid, event::error_t::NOT_FOUND, error);
					}
				} break;
				// Если событие DNS-резолвера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Останавливаем клиента
					this->_client->stop();
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
 */
void awh::client::Socks5::connect(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если подключение выполнено успешно
		if(ok){
			// Размер буфера данных
			size_t size = 0;
			// Буфер данных запроса
			uint8_t * buffer = nullptr;
			// Если извлечение буфера данных запроса выполнено успешно
			if(this->_socks5.buffer(&buffer, size, this->_ctx)){
				// Если отправка запроса на прокси-сервер не выполнена
				if(this->_client->send(eid, buffer, size) != size){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
						#endif
					}
				}
			}
		// Если подключение не выполнено
		} else {
			// Если функция обратного вызова установлена
			if(this->_callback.is("connect"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, false);
		}
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid  идентификатор клиента
 * @param size размер данных для записи
 */
void awh::client::Socks5::write(const event::id_t eid, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("write"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const size_t)> ("write", this->_eid, size);
	}
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param eid    идентификатор клиента
 * @param status новый статус клиента
 */
void awh::client::Socks5::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
		// Если статус клиента изменился на "уничтожен"
		if((eid == this->_eid) && (status == event::status_t::DESTROYED)){
			// Если идентификатор UDP-клиента установлен
			if(this->_endpoint.udp.eid > 0)
				// Уничтожаем UDP-клиента
				this->_client->destroy(this->_endpoint.udp.eid);
			// Если объект DNS-резолвера установлен
			if(this->_dns != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns->stop();
			// Останавливаем событие клиента
			else this->_client->stop();
		}
	}
}
/**
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных клиента
 * @param size   размер данных клиента
 */
void awh::client::Socks5::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если текущее состояние соответствует завершённому состоянию
			if(this->_ctx.state == proto::socks5_t::state_t::COMPLETED){
				// Если идентификатор клиента соответствует идентификатору socks5 клиента
				if(eid == this->_eid){
					// Если объект транспортного уровня безопасности установлен
					if((this->_tls != nullptr) && (this->_secId > 0)){
						// Если данные не расшифрованы
						if(!this->_tls->decrypt(this->_secId, buffer, size)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error_tls")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("TLS decryption data is failed", log_t::flag_t::WARNING);
								#endif
							}
						}
					// Если объект транспортного уровня безопасности не установлен
					} else {
						// Если функция обратного вызова установлена
						if(this->_callback.is("read"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, buffer, size);
					}
				// Если идентификатор клиента соответствует идентификатору UDP-клиента
				} else if(eid == this->_endpoint.udp.eid) {
					// Инициализируем объект заголовка UDP пакета
					proto::socks5_t::udp_head_t udp{};
					// Если парсинг данных от прокси-сервера выполнен успешно
					if(this->_socks5.parse(buffer, size, udp)){
						// Если хост клиента которому адресован UDP пакет установлен
						if(udp.host != nullptr){
							// Если объект транспортного уровня безопасности установлен
							if((this->_tls != nullptr) && (this->_secId > 0)){
								// Если данные не расшифрованы
								if(!this->_tls->decrypt(this->_secId, buffer + udp.size, size - udp.size)){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error_tls")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("TLS decryption data is failed", log_t::flag_t::WARNING);
										#endif
									}
								}
							// Если объект транспортного уровня безопасности не установлен
							} else {
								// Если функция обратного вызова установлена
								if(this->_callback.is("read"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, buffer + udp.size, size - udp.size);
							}
							// Выходим из функции
							return;
						}
					}
					// Создаём текст ошибки резолвинга доменного имени
					const string error = "Client for whom the UDP packet was received was not found";
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					// Выполняем функцию обратного вызова
					} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::NOT_FOUND, error);
				}
			// Если текущее состояние находится ещё в процессе общения с socks5 прокси-сервером
			} else {
				// Если парсинг данных от прокси-сервера выполнен успешно
				if(this->_socks5.parse(buffer, size, this->_ctx)){
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
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status));
								#endif
							// Выполняем функцию обратного вызова
							} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, this->_socks5.statusMessage(this->_ctx.status));
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, false);
						} break;
						// Если текущее состояние соответствует ожиданию выполнения подключения
						case static_cast <uint8_t> (proto::socks5_t::state_t::CONNECT): {
							// Если клиент для работы с UDP-протоколом активирован
							if(this->_endpoint.udp.eid > 0){
								// Устанавливаем команду для UDP протокола
								this->_ctx.command = proto::socks5_t::command_t::UDP;
								// Получаем порт клиента для подключения, работающего через прокси
								uint16_t port = this->_client->getInternalPort(this->_endpoint.udp.eid);
								/**
								 * Определяем тип данных сесии клиента, работающего через прокси
								 */
								switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (net::type_t::IPV4): {
										// Выполняем инициализацию объекта хоста
										this->_ctx.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										this->_ctx.host->type = net::type_t::IPV4;
										// Устанавливаем внутренний IP-адрес клиента
										this->_client->getAddress(this->_endpoint.udp.eid, event::address_t::IPV4, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip);
										// Если адрес клиента установлен а порт не установлен
										if((awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address > 0) && (port == 0)){
											// Получаем внутренний порт socks5-клиента
											port = this->_client->getInternalPort(this->_eid);
											// Устанавливаем внутренний порт клиента
											this->_client->setInternalPort(this->_endpoint.udp.eid, port);
										}
									} break;
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (net::type_t::IPV6): {
										// Выполняем инициализацию объекта хоста
										this->_ctx.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										this->_ctx.host->type = net::type_t::IPV6;
										// Устанавливаем внутренний IP-адрес клиента
										this->_client->getAddress(this->_endpoint.udp.eid, event::address_t::IPV6, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip);
										// Если адрес клиента установлен а порт не установлен
										if((::memcmp(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address[0], (uint8_t[16]){0}, 16) != 0) && (port == 0)){
											// Получаем внутренний порт socks5-клиента
											port = this->_client->getInternalPort(this->_eid);
											// Устанавливаем внутренний порт клиента
											this->_client->setInternalPort(this->_endpoint.udp.eid, port);
										}
									} break;
								}
								// Устанавливаем внутренний порт клиента
								awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = port;
							// Если клиент для работы с UDP-протоколом не активирован
							} else {
								// Устанавливаем команду для TCP протокола
								this->_ctx.command = proto::socks5_t::command_t::CONNECT;
								/**
								 * Определяем тип данных сесии клиента, работающего через прокси
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
							// Размер буфера данных
							size_t size = 0;
							// Буфер данных запроса
							uint8_t * buffer = nullptr;
							// Если извлечение буфера данных запроса выполнено успешно
							if(this->_socks5.buffer(&buffer, size, this->_ctx)){
								// Если отправка запроса на прокси-сервер не выполнена
								if(this->_client->send(this->_eid, buffer, size) != size){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
										#endif
									}
								// Выходим из функции
								} else return;
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, false);
						} break;
						// Если текущее состояние соответствует выполненному рукопожатию
						case static_cast <uint8_t> (proto::socks5_t::state_t::HANDSHAKE): {
							// Порт хоста для подключения к удалённому серверу
							uint16_t port = 0;
							// Адрес хоста для подключения к удалённому серверу
							string target = "";
							// Если клиент для работы с UDP-протоколом активирован
							if(this->_endpoint.udp.eid > 0){
								/**
								 * Определяем тип данных адреса полученного от socks5 прокси-сервера
								 */
								switch(static_cast <uint8_t> (this->_ctx.host->type)){
									// Если тип данных соответствует FQDN
									case static_cast <uint8_t> (net::type_t::FQDN): {
										// Если DNS-резолвер подключён
										if(this->_dns != nullptr){
											// Выполняем резолвинг хоста текущего сервера
											if(!this->_dns->resolve(this->_dns->issue(), this->_client->family(this->_endpoint.udp.eid), awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->domain)){
												// Создаём текст ошибки резолвинга хоста текущего сервера
												const string error = this->_fmk->format("It was not possible to obtain an IP address for the remote host \"%s\"", awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->domain.c_str());
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
													#endif
												// Выполняем функцию обратного вызова
												} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::NOT_FOUND, error);
											// Если резолвинг хоста не выполнен, выходим
											} else return;
										// Если DNS-резолвер не подключён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("This client does not support working with domain names, since the DNS resolver is not found", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("This client does not support working with domain names, since the DNS resolver is not found", log_t::flag_t::WARNING);
											#endif
										}
										// Если функция обратного вызова установлена
										if(this->_callback.is("connect"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, false);
										// Выходим из фукнции
										return;
									}
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (net::type_t::IPV4):
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (net::type_t::IPV6): {
										// Устанавливаем порт и хост для подключения к удалённому серверу
										if(this->_client->setPort(this->_endpoint.udp.eid, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port) &&
										   this->_client->setTarget(this->_endpoint.udp.eid, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())){
											// Выполняем фиксацию изменений для клиента, работающего через прокси
											if(this->_client->commit(this->_endpoint.udp.eid)){
												// Выполняем запуск работы клиента, работающего через прокси
												if(this->_client->launch(this->_endpoint.udp.eid)){
													// Получаем порт хоста для подключения к удалённому серверу
													port = this->_client->getPort(this->_endpoint.udp.eid);
													// Получаем адрес хоста для подключения к удалённому серверу
													target = this->_client->getTarget(this->_endpoint.udp.eid);
												// Если запуск работы клиента, работающего через прокси, не выполнен
												} else {
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Выводим сообщение об ошибке
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
														// Выводим сообщение об ошибке
														this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
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
													// Выводим сообщение об ошибке
													this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, eid);
												#endif
											}
										}
									} break;
								}
							// Если клиент для работы с UDP-протоколом не активирован
							} else {
								/**
								 * Определяем тип данных сесии клиента, работающего через прокси
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
										this->_addr.source(awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get());
										// Устанавливаем IP-адрес хоста для дальнейшего использования
										target = ::move(static_cast <string> (this->_addr));
										// Устанавливаем порт хоста для дальнейшего использования
										port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
									} break;
								}
							}
							// Если адрес хоста для подключения к удалённому серверу получен успешно
							if(!target.empty()){
								// Устанавливаем состояние клиента как "завершённый"
								this->_ctx.state = proto::socks5_t::state_t::COMPLETED;
								// Если функция обратного вызова установлена
								if(this->_callback.is("ready"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", eid, this->_client->family(eid), target, target);
								// Если функция обратного вызова установлена
								if(this->_callback.is("launch"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", eid, target, port);
								// Если объект транспортного уровня безопасности установлен
								if((this->_tls != nullptr) && (this->_secId > 0)){
									// Если рукопожатие TLS не выполнено
									if(!this->_tls->handshake(this->_secId)){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error_tls")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
											#endif
										}
									// Выходим из функции
									} else return;
								// Если объект транспортного уровня безопасности не установлен
								} else {
									// Если функция обратного вызова установлена
									if(this->_callback.is("connect"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, true);
									// Выходим из функции
									return;
								}
							// Если адрес хоста для подключения к удалённому серверу не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Client event ID not found", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Client event ID not found", log_t::flag_t::WARNING);
								#endif
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, false);
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
								if(this->_client->send(this->_eid, buffer, size) != size){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
										#endif
									}
								// Выходим из функции
								} else return;
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, false);
						}
					}
				// Если парсинг данных от прокси-сервера не выполнен
				} else {
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Failed to parse data from proxy server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to parse data from proxy server", log_t::flag_t::WARNING);
						#endif
					// Выполняем функцию обратного вызова
					} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, "Failed to parse data from proxy server");
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод резолвинга доменного имени удалённого хоста в сетевой адрес
 *
 * @param id     идентификатор DNS-запроса
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для резолвинга
 * @param addr   указатель на структуру для хранения результата резолвинга
 */
void awh::client::Socks5::resolve(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем порт и хост для подключения к удалённому серверу
		if(this->_client->setTarget(this->_endpoint.udp.eid, addr) &&
		   this->_client->setPort(this->_endpoint.udp.eid, awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->port)){
			// Выполняем фиксацию изменений для клиента, работающего через прокси
			if(this->_client->commit(this->_endpoint.udp.eid)){
				// Выполняем запуск работы клиента, работающего через прокси
				if(this->_client->launch(this->_endpoint.udp.eid)){
					// Устанавливаем состояние клиента как "завершённый"
					this->_ctx.state = proto::socks5_t::state_t::COMPLETED;
					// Получаем порт хоста для подключения к удалённому серверу
					const uint16_t port = this->_client->getPort(this->_endpoint.udp.eid);
					// Получаем адрес хоста для подключения к удалённому серверу
					const string & target = this->_client->getTarget(this->_endpoint.udp.eid);
					// Если функция обратного вызова установлена
					if(this->_callback.is("ready"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, family, target, target);
					// Если функция обратного вызова установлена
					if(this->_callback.is("launch"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", this->_eid, target, port);
					// Если объект транспортного уровня безопасности установлен
					if((this->_tls != nullptr) && (this->_secId > 0)){
						// Если рукопожатие TLS не выполнено
						if(!this->_tls->handshake(this->_secId)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error_tls")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
								#endif
							}
						}
					// Если объект транспортного уровня безопасности не установлен
					} else {
						// Если функция обратного вызова установлена
						if(this->_callback.is("connect"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, true);
					}
				// Если запуск работы клиента, работающего через прокси, не выполнен
				} else {
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
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
						// Выводим сообщение об ошибке
						this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
					// Выводим сообщение об ошибке
					this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param id    идентификатор TLS
 * @param state состояние TLS
 */
void awh::client::Socks5::stateTLS(const tls::coder_t::id_t id, const tls::coder_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state_tls"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const tls::coder_t::id_t, const tls::coder_t::state_t)> ("state_tls", id, state);
		// Если состояние рукопожатия успешно завершено
		if(state == tls::coder_t::state_t::HANDSHAKED){
			// Если функция обратного вызова установлена
			if(this->_callback.is("connect"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, true);
		}
	}
}
/**
 * @brief Метод получения событий шифрования/дешифрования данных TLS
 *
 * @param id     идентификатор TLS
 * @param event  тип события TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 */
void awh::client::Socks5::processTLS(const tls::coder_t::id_t id, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
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
						// Инициализируем объект заголовка UDP пакета
						proto::socks5_t::udp_head_t udp{};
						/**
						 * Определяем тип данных сесии клиента, работающего через прокси
						 */
						switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
							// Если тип данных соответствует FQDN
							case static_cast <uint8_t> (net::type_t::FQDN): {
								// Выполняем инициализацию объекта хоста
								udp.host = make_unique <net::attr_fqdn_t> ();
								// Устанавливаем тип адреса события
								udp.host->type = net::type_t::FQDN;
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_fqdn_t *> (udp.host.get())->port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port;
								// Устанавливаем доменное имя хоста для подключения
								awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain;
							} break;
							// Если тип данных соответствует IPv4
							case static_cast <uint8_t> (net::type_t::IPV4): {
								// Выполняем инициализацию объекта хоста
								udp.host = make_unique <net::attr_net_t> ();
								// Устанавливаем тип адреса события
								udp.host->type = net::type_t::IPV4;
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_net_t *> (udp.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
								// Устанавливаем IP-адрес хоста для подключения
								awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address;
							} break;
							// Если тип данных соответствует IPv6
							case static_cast <uint8_t> (net::type_t::IPV6): {
								// Выполняем инициализацию объекта хоста
								udp.host = make_unique <net::attr_net_t> ();
								// Устанавливаем тип адреса события
								udp.host->type = net::type_t::IPV6;
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_net_t *> (udp.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
								// Создаём новый объект адреса клиента IPv6
								awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
								// Устанавливаем IP-адрес хоста для подключения
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address[0], 16);
							} break;
						}
						// Размер буфера данных
						size_t length = 0;
						// Буфер данных запроса
						uint8_t * data = nullptr;
						// Если извлечение буфера данных запроса выполнено успешно
						if(this->_socks5.buffer(&data, length, udp)){
							/**
							 * Определяем тип данных сесии клиента, работающего через прокси
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
							// Если размер буфера полезной нагрузки достаточно для отправки всех данных
							if(::__awh_size__ == (size + length)){
								// Копируем данные запроса в буфер полезной нагрузки
								::memcpy(&::__awh_buffer__[0], data, length);
								// Добавляем к буферу данных для отправки полезную нагрузку
								::memcpy(&::__awh_buffer__[length], buffer, size);
							// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
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
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to generate buffer for UDP packet", log_t::flag_t::WARNING);
							#endif
						}
						// Если буфер полезной нагрузки для отправки не пустой
						if(::__awh_size__ > 0){
							// Если отправка запроса на прокси-сервер не выполнена
							if(this->_client->send(this->_endpoint.udp.eid, ::__awh_buffer__, ::__awh_size__) != ::__awh_size__){
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
									#endif
								}
							}
						}
					// Если клиент для работы с UDP протоколом не инициализирован
					} else {
						// Отправляем данные обратно клиенту, которые были зашифрованы TLS
						if(!this->_client->send(this->_eid, reinterpret_cast <const char *> (buffer), size)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
								#endif
							}
						}
					}
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION): {
					// Если функция обратного вызова установлена
					if(this->_callback.is("read"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, buffer, size);
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @return результат выполнения приостановки работы
 */
bool awh::client::Socks5::pause() noexcept {
	// Результат работы функции
	bool result = false;
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если идентификатор клиента установлен
			if(this->_eid > 0){
				// Приостанавливаем событие клиента
				if((result = this->_client->pause(this->_eid))){
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0)
						// Приостанавливаем событие клиента для конечной точки
						result = this->_client->pause(this->_endpoint.udp.eid);
				}
			// Если идентификатор клиента не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @return результат выполнения возобновления работы
 */
bool awh::client::Socks5::resume() noexcept {
	// Результат работы функции
	bool result = false;
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если идентификатор клиента установлен
			if(this->_eid > 0){
				// Возобновляем работу события клиента
				if((result = this->_client->resume(this->_eid))){
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0)
						// Возобновляем работу события клиента для конечной точки
						result = this->_client->resume(this->_endpoint.udp.eid);
				}
			// Если идентификатор клиента не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод мультиподключения клиентов к удалённым хостам (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения подключения
 */
bool awh::client::Socks5::connect() noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения отключения
 */
bool awh::client::Socks5::disconnect() noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения данных от сервера
 *
 * @return результат получения данных
 */
bool awh::client::Socks5::recv() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если клиент для работы с UDP протоколом не инициализирован
			if(this->_endpoint.udp.eid == 0)
				// Получаем данные от сервера
				return this->_client->recv(this->_eid);
			// Если клиент для работы с UDP протоколом инициализирован, получаем данные с него
			else return this->_client->recv(this->_endpoint.udp.eid);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных клиенту
 *
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных клиенту
 */
size_t awh::client::Socks5::send(const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если клиент для работы с UDP протоколом не инициализирован
			if(this->_endpoint.udp.eid == 0){
				// Если идентификатор TLS и объект TLS установлены
				if((this->_secId > 0) && (this->_tls != nullptr)){
					// Если шифрование данных TLS выполнено успешно
					if(this->_tls->encrypt(this->_secId, buffer, size))
						// Возвращаем размер отправленных данных
						return size;
					// Выводим результат по умолчанию
					return 0;
				}
				// Выполняем отправку данных серверу
				return this->_client->send(this->_eid, buffer, size);
			// Если клиент для работы с UDP протоколом инициализирован
			} else {
				// Если идентификатор TLS и объект TLS установлены
				if((this->_secId > 0) && (this->_tls != nullptr)){
					// Если шифрование данных TLS выполнено успешно
					if(this->_tls->encrypt(this->_secId, buffer, size))
						// Возвращаем размер отправленных данных
						return size;
					// Выводим результат по умолчанию
					return 0;
				}
				// Инициализируем объект заголовка UDP пакета
				proto::socks5_t::udp_head_t udp{};
				/**
				 * Определяем тип данных сесии клиента, работающего через прокси
				 */
				switch(static_cast <uint8_t> (this->_endpoint.attr->type)){
					// Если тип данных соответствует FQDN
					case static_cast <uint8_t> (net::type_t::FQDN): {
						// Выполняем инициализацию объекта хоста
						udp.host = make_unique <net::attr_fqdn_t> ();
						// Устанавливаем тип адреса события
						udp.host->type = net::type_t::FQDN;
						// Устанавливаем порт хоста для подключения
						awh_cast <net::attr_fqdn_t *> (udp.host.get())->port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port;
						// Устанавливаем доменное имя хоста для подключения
						awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain;
					} break;
					// Если тип данных соответствует IPv4
					case static_cast <uint8_t> (net::type_t::IPV4): {
						// Выполняем инициализацию объекта хоста
						udp.host = make_unique <net::attr_net_t> ();
						// Устанавливаем тип адреса события
						udp.host->type = net::type_t::IPV4;
						// Устанавливаем порт хоста для подключения
						awh_cast <net::attr_net_t *> (udp.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
						// Устанавливаем IP-адрес хоста для подключения
						awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address;
					} break;
					// Если тип данных соответствует IPv6
					case static_cast <uint8_t> (net::type_t::IPV6): {
						// Выполняем инициализацию объекта хоста
						udp.host = make_unique <net::attr_net_t> ();
						// Устанавливаем тип адреса события
						udp.host->type = net::type_t::IPV6;
						// Устанавливаем порт хоста для подключения
						awh_cast <net::attr_net_t *> (udp.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port;
						// Создаём новый объект адреса клиента IPv6
						awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем IP-адрес хоста для подключения
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address[0], 16);
					} break;
				}
				// Размер буфера данных
				size_t length = 0;
				// Буфер данных запроса
				uint8_t * data = nullptr;
				// Если извлечение буфера данных запроса выполнено успешно
				if(this->_socks5.buffer(&data, length, udp)){
					/**
					 * Определяем тип данных сесии клиента, работающего через прокси
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
					// Если размер буфера полезной нагрузки достаточно для отправки всех данных
					if(::__awh_size__ == (size + length)){
						// Копируем данные запроса в буфер полезной нагрузки
						::memcpy(&::__awh_buffer__[0], data, length);
						// Добавляем к буферу данных для отправки полезную нагрузку
						::memcpy(&::__awh_buffer__[length], buffer, size);
						// Выполняем отправку данных серверу
						return this->_client->send(this->_endpoint.udp.eid, ::__awh_buffer__, ::__awh_size__);
					// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Message sent by the UDP is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
						#endif
					}
				// Если извлечение буфера данных запроса не выполнено
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 */
bool awh::client::Socks5::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор клиента установлен
		if(this->_eid > 0){
			// Устанавливаем пропускную способность клиента
			if((result = this->_client->bandwidth(this->_eid, limiting, bandwidth))){
				// Если клиент для работы с UDP протоколом инициализирован
				if(this->_endpoint.udp.eid > 0)
					// Устанавливаем пропускную способность клиента для конечной точки
					result = this->_client->bandwidth(this->_endpoint.udp.eid, limiting, bandwidth);
			}
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки параметров авторизации
 *
 * @param username имя пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Socks5::setUser(const string & username, const string & password) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working())
		// Устанавливаем параметры авторизации для объекта клиента
		this->_socks5.setUser(username, password);
}
/**
 * @brief Метод установки исходящего адреса для UDP-клиента
 *
 * @param addr искходящий адрес для UDP-клиента
 * @return 	   результат выполнения установки исходящего адреса для UDP-клиента
 */
bool awh::client::Socks5::udp(const net::attr_net_t * addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если исходящий адрес для UDP-клиента не пустой
			if(addr != nullptr){
				// Если клиент для работы с UDP протоколом инициализирован
				if(this->_endpoint.udp.eid > 0)
					// Удаляем клиента принадлежащего пиру
					this->_client->destroy(this->_endpoint.udp.eid);
				/**
				 * Определяем тип полученного IP-адреса
				 */
				switch(addr->ip->size){
					// Для типа IPv4
					case 4:
						// Выполняем создание клиента для подключения к удалённому серверу
						this->_endpoint.udp.eid = this->_client->issue(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
					break;
					// Для типа IPv6
					case 16:
						// Выполняем создание клиента для подключения к удалённому серверу
						this->_endpoint.udp.eid = this->_client->issue(event::family_t::IPV6, event::type_t::DATAGRAM, event::protocol_t::UDP);
					break;
				}
				// Если клиент для работы с UDP протоколом инициализирован
				if(this->_endpoint.udp.eid > 0){
					// Устананавливаем опции события
					if(this->_client->setOptions(this->_endpoint.udp.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
						// Устанавливаем адрес с которого будет выполняться подключение к удалённому серверу
						if(!(result = (this->_client->setAddress(this->_endpoint.udp.eid, event::address_t::IPV4, addr->ip.get()) && this->_client->setInternalPort(this->_endpoint.udp.eid, addr->port)))){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								// Устанавливаем исходящий адрес для UDP-клиента
								this->_addr.source(addr->ip.get());
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, static_cast <string> (this->_addr).c_str(), this->_endpoint.udp.eid);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, static_cast <string> (this->_addr).c_str(), this->_endpoint.udp.eid);
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
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, this->_endpoint.udp.eid);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
							#endif
						}
					}
					// Удаляем клиента принадлежащего пиру
					this->_client->destroy(this->_endpoint.udp.eid);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки исходящего адреса для UDP-клиента
 *
 * @param addr искходящий адрес для UDP-клиента
 * @param port исходящий порт для UDP-клиента
 * @return     результат выполнения установки исходящего адреса для UDP-клиента
 */
bool awh::client::Socks5::udp(string_view addr, const uint16_t port) noexcept {
	// Результат работы функции
	bool result = false;
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если исходящий адрес для UDP-клиента не пустой
			if(!addr.empty()){
				// Выполняем парсинг IP-адреса
				if(this->_addr.parse(addr)){
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0)
						// Удаляем клиента принадлежащего пиру
						this->_client->destroy(this->_endpoint.udp.eid);
					/**
					 * Определяем тип полученного IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_addr.type())){
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
							// Выполняем создание клиента для подключения к удалённому серверу
							this->_endpoint.udp.eid = this->_client->issue(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
						break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
							// Выполняем создание клиента для подключения к удалённому серверу
							this->_endpoint.udp.eid = this->_client->issue(event::family_t::IPV6, event::type_t::DATAGRAM, event::protocol_t::UDP);
						break;
					}
					// Если клиент для работы с UDP протоколом инициализирован
					if(this->_endpoint.udp.eid > 0){
						// Устананавливаем опции события
						if(this->_client->setOptions(this->_endpoint.udp.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
							// Устанавливаем адрес с которого будет выполняться подключение к удалённому серверу
							if(!(result = (this->_client->setAddress(this->_endpoint.udp.eid, event::address_t::IPV4, this->_addr.source().get()) && this->_client->setInternalPort(this->_endpoint.udp.eid, port)))){
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::WARNING, addr, this->_endpoint.udp.eid);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
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
									// Выводим сообщение об ошибке
									this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::WARNING, this->_endpoint.udp.eid);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, this->_endpoint.udp.eid);
								#endif
							}
						}
						// Удаляем клиента принадлежащего пиру
						this->_client->destroy(this->_endpoint.udp.eid);
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки конечной точки клиента
 *
 * @param attr параметры подключения для установки конечной точки
 * @return     результат выполнения установки конечной точки
 */
bool awh::client::Socks5::endpoint(const net::attr_t * attr) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если адрес для установки конечной точки не пустой
			if(attr != nullptr){
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
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_endpoint.attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения для идентификатора события клиента
						this->_endpoint.attr->type = net::type_t::IPV4;
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port = awh_cast <const net::attr_net_t *> (attr)->port;
						// Устанавливаем полученный IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (attr)->ip.get())->address;
					} break;
					// Для типа IPv6
					case static_cast <uint8_t> (net::type_t::IPV6): {
						// Создаём объект параметров подключения для идентификатора события клиента
						this->_endpoint.attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения для идентификатора события клиента
						this->_endpoint.attr->type = net::type_t::IPV6;
						// Создаём новый объект адреса клиента IPv6
						awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->port = awh_cast <const net::attr_net_t *> (attr)->port;
						// Устанавливаем полученный IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (attr)->ip.get())->address[0], 16);
					} break;
				}
				// Выводим результат наличия объекта атрибутов конечной точки для идентификатора события клиента
				return (this->_endpoint.attr->type != net::type_t::NONE);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки конечной точки клиента
 *
 * @param addr адрес хоста для установки
 * @param port порт хоста для установки
 * @return     результат выполнения установки конечной точки
 */
bool awh::client::Socks5::endpoint(string_view addr, const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если адрес для установки конечной точки не пустой
			if(!addr.empty()){
				// Сбрасываем объект атрибутов конечной точки для идентификатора события клиента
				this->_endpoint.attr.reset(nullptr);
				// Выполняем парсинг IP-адреса
				if(this->_addr.parse(addr)){
					/**
					 * Определяем тип полученного IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_addr.type())){
						// Для типа FQDN
						case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
							// Создаём объект параметров подключения для идентификатора события клиента
							this->_endpoint.attr = make_unique <net::attr_fqdn_t> ();
							// Устанавливаем тип параметров подключения для идентификатора события клиента
							this->_endpoint.attr->type = net::type_t::FQDN;
							// Устанавливаем полученный порт
							awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->port = port;
							// Устанавливаем полученный доменное имя хоста для подключения
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
							awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
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
							awh_cast <net::attr_net_t *> (this->_endpoint.attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
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
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (this->_endpoint.attr.get())->domain = addr;
				}
				// Выводим результат наличия объекта атрибутов конечной точки для идентификатора события клиента
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(addr, port), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, fmk, log), _socks5(fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param tls    объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, tls::coder_t * tls, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, tls, fmk, log), _socks5(fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param dns    объект DNS-резолвера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, dns, fmk, log), _socks5(fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param dns    объект DNS-резолвера
 * @param tls    объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, unit::dns_t * dns, tls::coder_t * tls, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, dns, tls, fmk, log), _socks5(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::client::Socks5::~Socks5() noexcept {}
