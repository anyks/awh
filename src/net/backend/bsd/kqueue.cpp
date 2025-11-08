/**
 * @file: kqueue.cpp
 * @date: 2025-11-06
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Если максимальное количество опрашиваемых событий за одну итерацию (64, 128, 256, 512, 1024)
 */
#ifndef AWH_MAX_POLL_EVENTS_COUNT
	/**
	 * Устанавливаем максимальное количество опрашиваемых событий за одну итерацию (64)
	 */
	#define AWH_MAX_POLL_EVENTS_COUNT 0x40
#endif

/**
 * Стандартные модули
 */
#include <cerrno>
#include <atomic>
#include <memory>
#include <cstdlib>
#include <iostream>

/**
 * Подключаем системные заголовки
 */
#include <sys/un.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/**
 * Подключаем наши заголовочные файлы
 */
#include <sys/os.hpp>

/**
 * Подключаем заголовочный файл асинхронного движка ввода-вывода
 */
#include <net/io.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем статические функции в пространство имён
 */
namespace {
	/**
	 * @brief Функция генерации уникального идентификатора
	 *
	 * @return уникальный идентификатор
	 */
	uint32_t identifier() noexcept {
		// Начинаем с 1 (0 можно оставить как "invalid")
		static atomic_uint32_t id{1};
		// Выводим новое значение идентификатора
		return id.fetch_add(1, memory_order_relaxed);
	}
	/**
	 * Глобальная переменная списка узлов событий
	 */
	unordered_map <awh::event::id_t, unique_ptr <awh::net::node_t>> __awh_nodes__;
}

/**
 * @brief Метод настройки события
 *
 * @param id идентификатор события
 * @return   результат выполнения настройки
 */
bool awh::IO::setup(const event::id_t id) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения порта события
 *
 * @param id идентификатор события
 * @return   порт события
 */
uint16_t awh::IO::port(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем семейство сокета
			 */
			switch(static_cast <uint8_t> (i->second->state.family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
				// Для семейства UDPv4
				case static_cast <uint8_t> (event::family_t::UDPV4):
				// Для семейства UDPv6
				case static_cast <uint8_t> (event::family_t::UDPV6): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER):
							// Возвращаем результат работы функции
							return awh_cast <net::attr_net_t *> (awh_cast <net::peer_t *> (i->second.get())->remote.get())->port;
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
							// Возвращаем результат работы функции
							return awh_cast <net::attr_net_t *> (awh_cast <net::client_t *> (i->second.get())->target.get())->port;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER):
							// Возвращаем результат работы функции
							return awh_cast <net::attr_net_t *> (awh_cast <net::server_t *> (i->second.get())->host.get())->port;
					}
				} break;
				// Для остальных семейств сокетов
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Port cannot be retrieved for events that are not network related", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Port cannot be retrieved for events that are not network related", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта события
 *
 * @param id   идентификатор события
 * @param port порт события
 * @return     результат выполнения установки
 */
bool awh::IO::port(const event::id_t id, const uint16_t port) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем семейство сокета
			 */
			switch(static_cast <uint8_t> (i->second->state.family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
				// Для семейства UDPv4
				case static_cast <uint8_t> (event::family_t::UDPV4):
				// Для семейства UDPv6
				case static_cast <uint8_t> (event::family_t::UDPV6): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Получаем объект адреса удалённого узла
							net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
							// Если объект адреса соседа не инициализирован
							if(peer->remote == nullptr){
								// Создаем новый объект адреса удалённого узла
								peer->remote = make_unique <net::attr_net_t> ();
								/**
								 * Определяем семейство сокета
								 */
								switch(static_cast <uint8_t> (i->second->state.family)){
									// Для семейства IPv4
									case static_cast <uint8_t> (event::family_t::IPV4):
									// Для семейства UDPv4
									case static_cast <uint8_t> (event::family_t::UDPV4):
										// Создаем новый объект адреса соседа IPv4
										awh_cast <net::attr_net_t *> (peer->remote.get())->ip = make_unique <net::addr_net_ipv4_t> ();
									break;
									// Для семейства IPv6
									case static_cast <uint8_t> (event::family_t::IPV6):
									// Для семейства UDPv6
									case static_cast <uint8_t> (event::family_t::UDPV6):
										// Создаем новый объект адреса соседа IPv6
										awh_cast <net::attr_net_t *> (peer->remote.get())->ip = make_unique <net::addr_net_ipv6_t> ();
									break;
								}
							}
							// Получаем объект хоста соседа
							awh_cast <net::attr_net_t *> (peer->remote.get())->port = port;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Получаем объект адреса удалённого узла
							net::client_t * client = awh_cast <net::client_t *> (i->second.get());
							// Если объект адреса клиента не инициализирован
							if(client->target == nullptr){
								// Создаем новый объект адреса удалённого узла
								client->target = make_unique <net::attr_net_t> ();
								/**
								 * Определяем семейство сокета
								 */
								switch(static_cast <uint8_t> (i->second->state.family)){
									// Для семейства IPv4
									case static_cast <uint8_t> (event::family_t::IPV4):
									// Для семейства UDPv4
									case static_cast <uint8_t> (event::family_t::UDPV4):
										// Создаем новый объект адреса клиента IPv4
										awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
									break;
									// Для семейства IPv6
									case static_cast <uint8_t> (event::family_t::IPV6):
									// Для семейства UDPv6
									case static_cast <uint8_t> (event::family_t::UDPV6):
										// Создаем новый объект адреса клиента IPv6
										awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
									break;
								}
							}
							// Получаем объект хоста клиента
							awh_cast <net::attr_net_t *> (client->target.get())->port = port;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Получаем объект адреса сервера
							net::server_t * server = awh_cast <net::server_t *> (i->second.get());
							// Если объект адреса сервера не инициализирован
							if(server->host == nullptr){
								// Создаем новый объект адреса сервера
								server->host = make_unique <net::attr_net_t> ();
								/**
								 * Определяем семейство сокета
								 */
								switch(static_cast <uint8_t> (i->second->state.family)){
									// Для семейства IPv4
									case static_cast <uint8_t> (event::family_t::IPV4):
									// Для семейства UDPv4
									case static_cast <uint8_t> (event::family_t::UDPV4):
										// Создаем новый объект адреса сервера IPv4
										awh_cast <net::attr_net_t *> (server->host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
									break;
									// Для семейства IPv6
									case static_cast <uint8_t> (event::family_t::IPV6):
									// Для семейства UDPv6
									case static_cast <uint8_t> (event::family_t::UDPV6):
										// Создаем новый объект адреса сервера IPv6
										awh_cast <net::attr_net_t *> (server->host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
									break;
								}
							}
							// Устанавливаем порт события
							awh_cast <net::attr_net_t *> (server->host.get())->port = port;
							// Возвращаем результат работы функции
							return true;
						}
					}
				} break;
				// Для остальных семейств сокетов
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Port cannot be set for events that are not network related",	 __PRETTY_FUNCTION__, std::make_tuple(id, port), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Port cannot be set for events that are not network related", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, port), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса события
 *
 * @param id идентификатор события
 * @return   сетевой интерфейс события
 */
string awh::IO::iface(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем текущее значение объекта клиента
					net::client_t * client = awh_cast <net::client_t *> (i->second.get());
					// Если объект источника сетевого адреса инициализирован
					if(client->source != nullptr){
						/**
						 * Определяем тип адреса
						 */
						switch(client->source->size){
							// Если адрес является IPv4
							case 4: {
								// Временный объект для извлечения сетевого интерфейса
								net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
								// Выполняем извлечение
								this->_eth.fillsource(client->source, source);
								// Возвращаем название сетевого интерфейса
								return source.iface;
							}
							// Если адрес является IPv6
							case 16: {
								// Временный объект для извлечения сетевого интерфейса
								net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
								// Выполняем извлечение
								this->_eth.fillsource(client->source, source);
								// Возвращаем название сетевого интерфейса
								return source.iface;
							}
						}
					}
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем текущее значение объекта сервера
					net::server_t * server = awh_cast <net::server_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
							// Выполняем получение нужного нам атрибута подключения
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
							// Если IP-адрес установлен и соответствует размеру IPv4
							if(host->ip->size == 4){
								// Извлекаем IP-адрес сети
								net::addr_net_ipv4_t * ip = awh_cast <net::addr_net_ipv4_t *> (host->ip.get());
								// Если префикс сети не максимальный
								if(ip->prefix < 32){
									// Устанавливаем IP-адрес сети в хостовом порядке
									this->_addr.v4(ip->address);
									// Выполняем наложение маски
									this->_addr.impose(ip->prefix, net_addr_t::addr_t::NETWORK, net_addr_t::type_t::IPV4);
									// IP-адрес сети IPv4 в формате little-endian
									unique_ptr <net::addr_t> network = make_unique <net::addr_net_ipv4_t> ();
									// Получаем адрес сети IPv4 в формате little-endian
									auto addr = awh_cast <net::addr_net_ipv4_t *> (network.get());
									// Устанавливаем префикс сети
									addr->prefix = ip->prefix;
									// Получаем значение IP-адреса сети
									addr->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
									// Выполняем извлечение
									this->_eth.fillsource(network, source);
								// Выполняем извлечение
								} else this->_eth.fillsource(host->ip, source);
								// Возвращаем название сетевого интерфейса
								return source.iface;
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve network interface name, desired IPv4 address does not match set IPv6", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve network interface name, desired IPv4 address does not match set IPv6", log_t::flag_t::WARNING);
								#endif
							}
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
							// Выполняем получение нужного нам атрибута подключения
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
							// Если IP-адрес установлен и соответствует размеру IPv6
							if(host->ip->size == 16){
								// Извлекаем IP-адрес сети
								net::addr_net_ipv6_t * ip = awh_cast <net::addr_net_ipv6_t *> (host->ip.get());
								// Если префикс сети не максимальный
								if(ip->prefix < 128){
									// Устанавливаем IP-адрес сети в хостовом порядке
									this->_addr.v6(ip->address);
									// Выполняем наложение маски
									this->_addr.impose(ip->prefix, net_addr_t::addr_t::NETWORK, net_addr_t::type_t::IPV6);
									// IP-адрес сети IPv6 в формате little-endian
									unique_ptr <net::addr_t> network = make_unique <net::addr_net_ipv6_t> ();
									// Получаем адрес сети IPv6 в формате little-endian
									auto addr = awh_cast <net::addr_net_ipv6_t *> (network.get());
									// Устанавливаем префикс сети
									addr->prefix = ip->prefix;
									// Получаем значение IP-адреса сети
									addr->address = ::move(this->_addr.v6(net_addr_t::endian_t::LITTLE));
									// Выполняем извлечение
									this->_eth.fillsource(network, source);
								// Выполняем извлечение
								} else this->_eth.fillsource(host->ip, source);
								// Возвращаем название сетевого интерфейса
								return source.iface;
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve network interface name, desired IPv6 address does not match set IPv4", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve network interface name, desired IPv6 address does not match set IPv4", log_t::flag_t::WARNING);
								#endif
							}
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::IO::iface(const event::id_t id, const string & name) noexcept {
	// Результат выполнения функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем текущее значение объекта клиента
					net::client_t * client = awh_cast <net::client_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
							// Устанавливаем имя сетевого интерфейса
							source.iface = this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE);
							// Выполняем извлечение сетевых параметров
							this->_eth.fillsource(source);
							// Если IP-адрес успешно получен
							if((result = (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0))){
								// Если источник сетевого адреса не инициализирован
								if(client->source == nullptr)
									// Инициализируем источник сетевого адреса
									client->source = make_unique <net::addr_net_ipv4_t> ();
								// Устанавливаем тип адреса
								client->state.address = event::address_t::IPV4;
								// Копируем IP-адрес в источник сетевого адреса
								awh_cast <net::addr_net_ipv4_t *> (client->source.get())->address = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
							// Если IP-адрес не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" is not found", log_t::flag_t::WARNING, source.iface.c_str());
								#endif
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
							// Устанавливаем имя сетевого интерфейса
							source.iface = this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE);
							// Выполняем извлечение сетевых параметров
							this->_eth.fillsource(source);
							// Если IP-адрес успешно получен
							if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0))){
								// Если источник сетевого адреса не инициализирован
								if(client->source == nullptr)
									// Инициализируем источник сетевого адреса
									client->source = make_unique <net::addr_net_ipv6_t> ();
								// Устанавливаем тип адреса
								client->state.address = event::address_t::IPV6;
								// Копируем IP-адрес в источник сетевого адреса
								awh_cast <net::addr_net_ipv6_t *> (client->source.get())->address = ::move(awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address);
							// Если IP-адрес не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" is not found", log_t::flag_t::WARNING, source.iface.c_str());
								#endif
							}
						} break;
					}
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем текущее значение объекта сервера
					net::server_t * server = awh_cast <net::server_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
							// Устанавливаем имя сетевого интерфейса
							source.iface = this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE);
							// Выполняем извлечение сетевых параметров
							this->_eth.fillsource(source);
							// Если IP-адрес успешно получен
							if((result = (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0))){
								// Устанавливаем тип адреса
								server->state.address = event::address_t::IPV4;
								// Копируем IP-адрес в хост сервера
								awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
							// Если IP-адрес не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" is not found", log_t::flag_t::WARNING, source.iface.c_str());
								#endif
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
							// Устанавливаем имя сетевого интерфейса
							source.iface = this->_fmk->transform(name, fmk_t::transform_t::LOWER_CASE);
							// Выполняем извлечение сетевых параметров
							this->_eth.fillsource(source);
							// Если IP-адрес успешно получен
							if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0))){
								// Устанавливаем тип адреса
								server->state.address = event::address_t::IPV6;
								// Копируем IP-адрес в хост сервера
								awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
							// Если IP-адрес не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" is not found", log_t::flag_t::WARNING, source.iface.c_str());
								#endif
							}
						} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод получения хоста целевой машины
 *
 * @param id идентификатор события
 * @return   хост целевой машины
 */
string awh::IO::target(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Получаем текущее значение объекта файловой системы
					net::fs_t * fs = awh_cast <net::fs_t *> (i->second.get());
					// Если объект адреса файловой системы не инициализирован
					if(fs->path == nullptr)
						// Прерываем выполнение
						break;
					// Возвращаем адрес файловой системы
					return awh_cast <net::addr_fs_t *> (fs->path.get())->address;
				}
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Получаем текущее значение объекта соседа
					net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
					// Если объект адреса соседа не инициализирован
					if(peer->remote == nullptr)
						// Прерываем выполнение
						break;
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							// Получаем объект хоста соседа
							net::attr_uds_t * remote = awh_cast <net::attr_uds_t *> (peer->remote.get());
							// Извлекаем и возвращаем адрес сокета
							return awh_cast <net::addr_fs_t *> (remote->path.get())->address;
						}
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							// Получаем объект хоста соседа
							net::attr_net_t * remote = awh_cast <net::attr_net_t *> (peer->remote.get());
							// Если IP-адрес установлен и соответствует размеру IPv4
							if(remote->ip->size == 4){
								// Устанавливаем полученный IP-адрес
								this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (remote->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Возвращаем результат работы функции
								return static_cast <string> (this->_addr);
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve target address, desired IPv4 address does not match set IPv6", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve target address, desired IPv4 address does not match set IPv6", log_t::flag_t::WARNING);
								#endif
							}
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Получаем объект хоста соседа
							net::attr_net_t * remote = awh_cast <net::attr_net_t *> (peer->remote.get());
							// Если IP-адрес установлен и соответствует размеру IPv6
							if(remote->ip->size == 16){
								// Устанавливаем полученный IP-адрес
								this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (remote->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Возвращаем результат работы функции
								return static_cast <string> (this->_addr);
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve target address, desired IPv6 address does not match set IPv4", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve target address, desired IPv6 address does not match set IPv4", log_t::flag_t::WARNING);
								#endif
							}
						}
					}
				} break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем текущее значение объекта клиента
					net::client_t * client = awh_cast <net::client_t *> (i->second.get());
					// Если объект адреса клиента не инициализирован
					if(client->target == nullptr)
						// Прерываем выполнение
						break;
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							// Получаем объект адреса целевой машины
							net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (client->target.get());
							// Извлекаем и возвращаем адрес сокета
							return awh_cast <net::addr_fs_t *> (target->path.get())->address;
						}
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							// Получаем объект адреса целевой машины
							net::attr_net_t * target = awh_cast <net::attr_net_t *> (client->target.get());
							// Если IP-адрес установлен и соответствует размеру IPv4
							if(target->ip->size == 4){
								// Устанавливаем полученный IP-адрес
								this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (target->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Возвращаем результат работы функции
								return static_cast <string> (this->_addr);
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve target address, desired IPv4 address does not match set IPv6", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve target address, desired IPv4 address does not match set IPv6", log_t::flag_t::WARNING);
								#endif
							}
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Получаем объект адреса целевой машины
							net::attr_net_t * target = awh_cast <net::attr_net_t *> (client->target.get());
							// Если IP-адрес установлен и соответствует размеру IPv6
							if(target->ip->size == 16){
								// Устанавливаем полученный IP-адрес
								this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (target->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Возвращаем результат работы функции
								return static_cast <string> (this->_addr);
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve target address, desired IPv6 address does not match set IPv4", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve target address, desired IPv6 address does not match set IPv4", log_t::flag_t::WARNING);
								#endif
							}
						}
					}
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем текущее значение объекта сервера
					net::server_t * server = awh_cast <net::server_t *> (i->second.get());
					// Если объект адреса сервера не инициализирован
					if(server->host == nullptr)
						// Прерываем выполнение
						break;
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							// Получаем объект адреса хоста сервера
							net::attr_uds_t * host = awh_cast <net::attr_uds_t *> (server->host.get());
							// Извлекаем и возвращаем адрес сокета
							return awh_cast <net::addr_fs_t *> (host->path.get())->address;
						}
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							// Получаем объект адреса хоста сервера
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
							// Если IP-адрес установлен и соответствует размеру IPv4
							if(host->ip->size == 4){
								// Устанавливаем полученный IP-адрес
								this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (host->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Возвращаем результат работы функции
								return static_cast <string> (this->_addr);
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve target address, desired IPv4 address does not match set IPv6", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve target address, desired IPv4 address does not match set IPv6", log_t::flag_t::WARNING);
								#endif
							}
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Получаем объект адреса хоста сервера
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
							// Если IP-адрес установлен и соответствует размеру IPv6
							if(host->ip->size == 16){
								// Устанавливаем полученный IP-адрес
								this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (host->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Возвращаем результат работы функции
								return static_cast <string> (this->_addr);
							// Если размер IP-адреса не соответствует IPv4
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("To retrieve target address, desired IPv6 address does not match set IPv4", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("To retrieve target address, desired IPv6 address does not match set IPv4", log_t::flag_t::WARNING);
								#endif
							}
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки хоста целевой машины
 *
 * @param id   идентификатор события
 * @param host хост целевой машины
 * @return     результат выполнения установки
 */
bool awh::IO::target(const event::id_t id, const string & target) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					/**
					 * Определяем проверку соответствует ли адрес
					 */
					switch(static_cast <uint8_t> (this->_addr.host(target))){
						// Если адрес соответствует адресу файловой системы
						case static_cast <uint8_t> (net_addr_t::type_t::FS): {
							// Получаем текущее значение объекта файловой системы
							net::fs_t * fs = awh_cast <net::fs_t *> (i->second.get());
							// Если объект адреса файловой системы не инициализирован
							if(fs->path == nullptr)
								// Инициализируем объект адреса файловой системы
								fs->path = make_unique <net::addr_fs_t> ();
							// Если тип адреса не установлен
							if(i->second->state.address == event::address_t::NONE)
								// Устанавливаем тип адреса как файл в файловой системе
								i->second->state.address = event::address_t::FILE;
							// Устанавливаем адрес файловой системы
							awh_cast <net::addr_fs_t *> (fs->path.get())->address = target;
							// Выводим результат работы функции
							return true;
						}
						// Если адрес не принадлежит к адресу файловой системы
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Address \"%s\" you are trying to add is not a filesystem address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Address \"%s\" you are trying to add is not a filesystem address", log_t::flag_t::WARNING, target.c_str());
							#endif
						}
					}
				} break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Получаем текущее значение объекта соседа
					net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует адресу файловой системы
								case static_cast <uint8_t> (net_addr_t::type_t::FS): {
									// Если объект адреса соседа не инициализирован
									if(peer->remote == nullptr)
										// Создаем новый объект адреса удалённого узла
										peer->remote = make_unique <net::attr_uds_t> ();
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как файловая система
										i->second->state.address = event::address_t::UDS;
									// Устанавливаем адрес uinix-доменного сокета
									awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (peer->remote.get())->path.get())->address = target;
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a filesystem address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a filesystem address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует IPv4-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Выполняем парсинг IPv4-адреса
									this->_addr.parse(target, net_addr_t::type_t::IPV4);
									// Если объект адреса соседа не инициализирован
									if(peer->remote == nullptr){
										// Создаем новый объект адреса удалённого узла
										peer->remote = make_unique <net::attr_net_t> ();
										// Создаем новый объект адреса соседа IPv4
										awh_cast <net::attr_net_t *> (peer->remote.get())->ip = make_unique <net::addr_net_ipv4_t> ();
									}
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как IPv4
										i->second->state.address = event::address_t::IPV4;
									// Устанавливаем полученный IP-адрес
									awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (peer->remote.get())->ip.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a IPv4 address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a IPv4 address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует IPv6-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Выполняем парсинг IPv6-адреса
									this->_addr.parse(target, net_addr_t::type_t::IPV6);
									// Если объект адреса соседа не инициализирован
									if(peer->remote == nullptr){
										// Создаем новый объект адреса удалённого узла
										peer->remote = make_unique <net::attr_net_t> ();
										// Создаем новый объект адреса соседа IPv6
										awh_cast <net::attr_net_t *> (peer->remote.get())->ip = make_unique <net::addr_net_ipv6_t> ();
									}
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как IPv6
										i->second->state.address = event::address_t::IPV6;
									// Устанавливаем полученный IP-адрес
									awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (peer->remote.get())->ip.get())->address = ::move(this->_addr.v6(net_addr_t::endian_t::LITTLE));
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a IPv6 address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a IPv6 address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
					}
				} break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем текущее значение объекта клиента
					net::client_t * client = awh_cast <net::client_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует адресу файловой системы
								case static_cast <uint8_t> (net_addr_t::type_t::FS): {
									// Если объект адреса соседа не инициализирован
									if(client->target == nullptr)
										// Создаем новый объект адреса удалённого узла
										client->target = make_unique <net::attr_uds_t> ();
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как файловая система
										i->second->state.address = event::address_t::UDS;
									// Устанавливаем адрес uinix-доменного сокета
									awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (client->target.get())->path.get())->address = target;
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a filesystem address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a filesystem address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует IPv4-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Выполняем парсинг IPv4-адреса
									this->_addr.parse(target, net_addr_t::type_t::IPV4);
									// Если объект адреса клиента не инициализирован
									if(client->target == nullptr){
										// Создаем новый объект адреса удалённого узла
										client->target = make_unique <net::attr_net_t> ();
										// Создаем новый объект адреса клиента IPv4
										awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
									}
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как IPv4
										i->second->state.address = event::address_t::IPV4;
									// Устанавливаем полученный IP-адрес
									awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (client->target.get())->ip.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a IPv4 address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a IPv4 address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует IPv6-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Выполняем парсинг IPv6-адреса
									this->_addr.parse(target, net_addr_t::type_t::IPV6);
									// Если объект адреса клиента не инициализирован
									if(client->target == nullptr){
										// Создаем новый объект адреса удалённого узла
										client->target = make_unique <net::attr_net_t> ();
										// Создаем новый объект адреса клиента IPv6
										awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
									}
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как IPv6
										i->second->state.address = event::address_t::IPV6;
									// Устанавливаем полученный IP-адрес
									awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (client->target.get())->ip.get())->address = ::move(this->_addr.v6(net_addr_t::endian_t::LITTLE));
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a IPv6 address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a IPv6 address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
					}
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем текущее значение объекта сервера
					net::server_t * server = awh_cast <net::server_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует адресу файловой системы
								case static_cast <uint8_t> (net_addr_t::type_t::FS): {
									// Если объект адреса сервера не инициализирован
									if(server->host == nullptr)
										// Создаем новый объект адреса сервера
										server->host = make_unique <net::attr_uds_t> ();
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как файловая система
										i->second->state.address = event::address_t::UDS;
									// Устанавливаем адрес uinix-доменного сокета
									awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (server->host.get())->path.get())->address = target;
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a filesystem address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a filesystem address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует IPv4-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Выполняем парсинг IPv4-адреса
									this->_addr.parse(target, net_addr_t::type_t::IPV4);
									// Если объект адреса сервера не инициализирован
									if(server->host == nullptr){
										// Создаем новый объект адреса сервера
										server->host = make_unique <net::attr_net_t> ();
										// Создаем новый объект адреса сервера IPv4
										awh_cast <net::attr_net_t *> (server->host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
									}
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как IPv4
										i->second->state.address = event::address_t::IPV4;
									// Устанавливаем полученный IP-адрес
									awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (server->host.get())->ip.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a IPv4 address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a IPv4 address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(target))){
								// Если адрес соответствует IPv6-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Выполняем парсинг IPv6-адреса
									this->_addr.parse(target, net_addr_t::type_t::IPV6);
									// Если объект адреса сервера не инициализирован
									if(server->host == nullptr){
										// Создаем новый объект адреса сервера
										server->host = make_unique <net::attr_net_t> ();
										// Создаем новый объект адреса сервера IPv6
										awh_cast <net::attr_net_t *> (server->host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
									}
									// Если тип адреса не установлен
									if(i->second->state.address == event::address_t::NONE)
										// Устанавливаем тип адреса как IPv6
										i->second->state.address = event::address_t::IPV6;
									// Устанавливаем полученный IP-адрес
									awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (server->host.get())->ip.get())->address = ::move(this->_addr.v6(net_addr_t::endian_t::LITTLE));
									// Выводим результат работы функции
									return true;
								}
								// Если адрес не принадлежит к адресу файловой системы
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a IPv6 address", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::WARNING, target.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a IPv6 address", log_t::flag_t::WARNING, target.c_str());
									#endif
								}
							}
						} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, target), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения типа узла события
 *
 * @param id идентификатор события
 * @return   тип узла события
 */
awh::event::node_t awh::IO::node(const event::id_t id) const noexcept {
	// Выполняем поиск идентификатора события
	auto i = ::__awh_nodes__.find(id);
	// Если идентификатор события найден
	if(i != ::__awh_nodes__.end())
		// Возвращаем тип узла события
		return i->second->state.node;
	// Выводим результат по умолчанию
	return event::node_t::NONE;
}
/**
 * @brief Метод установки типа узла события
 * @param id   идентификатор события
 * @param node тип узла события
 * @return     результат выполнения установки
 */
bool awh::IO::node(const event::id_t id, const event::node_t node) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (node)){
				// Если нода является пользовательским событием
				case static_cast <uint8_t> (event::node_t::USER): {
					// Выполняем создание нового объекта ноды
					unique_ptr <net::user_t> user = make_unique <net::user_t> ();
					// Выполняем перенос всей ноды
					i->second = ::move(user);
					// Устанавливаем тип узла события
					i->second->state.node = node;
				} break;
				// Если нода является таймером
				case static_cast <uint8_t> (event::node_t::TIMER): {
					// Выполняем создание нового объекта ноды
					unique_ptr <net::timer_t> timer = make_unique <net::timer_t> ();
					// Выполняем перенос всей ноды
					i->second = ::move(timer);
					// Устанавливаем тип узла события
					i->second->state.node = node;
				} break;
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Выполняем создание нового объекта ноды
					unique_ptr <net::fs_t> fs = make_unique <net::fs_t> (this->_fmk, this->_log);
					// Выполняем перенос состояний ноды
					fs->state = i->second->state;
					// Устанавливаем тип узла события
					fs->state.node = node;
					// Выполняем перенос всей ноды
					i->second = ::move(fs);
				} break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства директорий
						case static_cast <uint8_t> (event::family_t::DIR):
						// Для семейства файловой системы
						case static_cast <uint8_t> (event::family_t::FILE): {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to create a network node from a file node", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (node)), log_t::flag_t::CRITICAL);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to create a network node from a file node", log_t::flag_t::CRITICAL);
							#endif
							// Выводим отрицательный результат
							return false;
						} break;
						// Для семейства таймеров
						case static_cast <uint8_t> (event::family_t::TIMER):
						// Для семейства интервалов
						case static_cast <uint8_t> (event::family_t::INTERVAL): {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to create a network node from a timer node", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (node)), log_t::flag_t::CRITICAL);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to create a network node from a timer node", log_t::flag_t::CRITICAL);
							#endif
							// Выводим отрицательный результат
							return false;
						}
					}
					// Выполняем создание нового объекта ноды
					unique_ptr <net::peer_t> peer = make_unique <net::peer_t> (this->_fmk, this->_log);
					// Выполняем перенос состояний ноды
					peer->state = i->second->state;
					// Устанавливаем тип узла события
					peer->state.node = node;
					// Выполняем инициализацию объекта MAC-адреса
					peer->mac = make_unique <net::addr_mac_t> ();
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода ещё не определена
						case static_cast <uint8_t> (event::node_t::NONE):
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Получаем объект клиента
							auto client = awh_cast <net::client_t *> (i->second.get());
							// Устанавливаем значение сетевого сокета
							peer->socket = client->socket;
							// Выполняем перенос хоста ноды
							peer->remote = ::move(client->target);
						} break;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Получаем объект сервера
							auto server = awh_cast <net::server_t *> (i->second.get());
							// Устанавливаем значение сетевого сокета
							peer->socket = server->socket;
							// Выполняем перенос хоста ноды
							peer->remote = ::move(server->host);
						} break;
					}
					// Выполняем перенос всей ноды
					i->second = ::move(peer);
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства директорий
						case static_cast <uint8_t> (event::family_t::DIR):
						// Для семейства файловой системы
						case static_cast <uint8_t> (event::family_t::FILE): {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to create a network node from a file node", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (node)), log_t::flag_t::CRITICAL);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to create a network node from a file node", log_t::flag_t::CRITICAL);
							#endif
							// Выводим отрицательный результат
							return false;
						} break;
						// Для семейства таймеров
						case static_cast <uint8_t> (event::family_t::TIMER):
						// Для семейства интервалов
						case static_cast <uint8_t> (event::family_t::INTERVAL): {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to create a network node from a timer node", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (node)), log_t::flag_t::CRITICAL);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to create a network node from a timer node", log_t::flag_t::CRITICAL);
							#endif
							// Выводим отрицательный результат
							return false;
						}
					}
					// Выполняем создание нового объекта ноды
					unique_ptr <net::server_t> server = make_unique <net::server_t> (this->_fmk, this->_log);
					// Выполняем перенос состояний ноды
					server->state = i->second->state;
					// Устанавливаем тип узла события
					server->state.node = node;
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Получаем объект объект соседа
							auto peer = awh_cast <net::peer_t *> (i->second.get());
							// Устанавливаем значение сетевого сокета
							server->socket = peer->socket;
							// Выполняем перенос хоста ноды
							server->host = ::move(peer->remote);
						} break;
						// Если нода ещё не определена
						case static_cast <uint8_t> (event::node_t::NONE):
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Получаем объект объект клиента
							auto client = awh_cast <net::client_t *> (i->second.get());
							// Устанавливаем значение сетевого сокета
							server->socket = client->socket;
							// Выполняем перенос хоста ноды
							server->host = ::move(client->target);
						} break;
					}
					// Выполняем перенос всей ноды
					i->second = ::move(server);
				} break;
			}
			// Возвращаем результат работы функции
			return true;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (node)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @return        значение адреса события
 */
string awh::IO::address(const event::id_t id, const event::address_t address) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем тип адреса события
			 */
			switch(static_cast <uint8_t> (address)){
				// Если тип адреса принадлежит к MAC-адресам
				case static_cast <uint8_t> (event::address_t::MAC): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Получаем объект соседа
							auto peer = awh_cast <net::peer_t *> (i->second.get());
							// Если объект адреса соседа не инициализирован
							if(peer->remote == nullptr)
								// Прерываем выполнение
								break;
							/**
							 * Определяем семейство сокета
							 */
							switch(static_cast <uint8_t> (i->second->state.family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
								// Для семейства UDPv4
								case static_cast <uint8_t> (event::family_t::UDPV4): {
									// Получаем объект адреса хоста сервера
									net::attr_net_t * remote = awh_cast <net::attr_net_t *> (peer->remote.get());
									// Если IP-адрес установлен и соответствует размеру IPv4
									if(remote->ip->size == 4){
										// Временный объект для извлечения сетевого интерфейса
										net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
										// Устанавливаем полученный IP-адрес во временный объект
										awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (remote->ip.get())->address;
										// Выполняем извлечение сетевых параметров
										this->_eth.fillsource(i->second->state.node, source);
										// Если MAC-адрес успешно получен
										if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
											// Устанавливаем полученный MAC-адрес в объект события
											this->_addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
											// Выводим результат работы функции
											return static_cast <string> (this->_addr);
										// Если MAC-адрес не получен
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
											#endif
										}
									// Если размер IP-адреса не соответствует IPv4
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Desired IPv4 address does not match set IPv6", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Desired IPv4 address does not match set IPv6", log_t::flag_t::WARNING);
										#endif
									}
								} break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
								// Для семейства UDPv6
								case static_cast <uint8_t> (event::family_t::UDPV6): {
									// Получаем объект адреса хоста сервера
									net::attr_net_t * remote = awh_cast <net::attr_net_t *> (peer->remote.get());
									// Если IP-адрес установлен и соответствует размеру IPv6
									if(remote->ip->size == 16){
										// Временный объект для извлечения сетевого интерфейса
										net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
										// Устанавливаем полученный IP-адрес во временный объект
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (remote->ip.get())->address[0], 16);
										// Выполняем извлечение сетевых параметров
										this->_eth.fillsource(i->second->state.node, source);
										// Если MAC-адрес успешно получен
										if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
											// Устанавливаем полученный MAC-адрес в объект события
											this->_addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
											// Выводим результат работы функции
											return static_cast <string> (this->_addr);
										// Если MAC-адрес не получен
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
											#endif
										}
									// Если размер IP-адреса не соответствует IPv4
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Desired IPv6 address does not match set IPv4", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Desired IPv6 address does not match set IPv4", log_t::flag_t::WARNING);
										#endif
									}
								} break;
								// Если семейство сокета не определено
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
									#endif
								}
							}
						} break;
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Получаем объект клиента
							auto client = awh_cast <net::client_t *> (i->second.get());
							// Если объект адреса клиента не инициализирован
							if(client->source == nullptr)
								// Прерываем выполнение
								break;
							/**
							 * Определяем семейство сокета
							 */
							switch(static_cast <uint8_t> (i->second->state.family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
								// Для семейства UDPv4
								case static_cast <uint8_t> (event::family_t::UDPV4): {
									// Если IP-адрес установлен и соответствует размеру IPv4
									if((client->source != nullptr) && (client->source->size == 4)){
										// Временный объект для извлечения сетевого интерфейса
										net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
										// Устанавливаем полученный IP-адрес во временный объект
										awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (client->source.get())->address;
										// Выполняем извлечение сетевых параметров
										this->_eth.fillsource(i->second->state.node, source);
										// Если MAC-адрес успешно получен
										if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
											// Устанавливаем полученный MAC-адрес в объект события
											this->_addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
											// Выводим результат работы функции
											return static_cast <string> (this->_addr);
										// Если MAC-адрес не получен
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
											#endif
										}
									// Если размер IP-адреса не соответствует IPv4
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Desired IPv4 address does not match set IPv6", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Desired IPv4 address does not match set IPv6", log_t::flag_t::WARNING);
										#endif
									}
								} break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
								// Для семейства UDPv6
								case static_cast <uint8_t> (event::family_t::UDPV6): {
									// Если IP-адрес установлен и соответствует размеру IPv6
									if((client->source != nullptr) && (client->source->size == 16)){
										// Временный объект для извлечения сетевого интерфейса
										net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
										// Устанавливаем полученный IP-адрес во временный объект
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (client->source.get())->address[0], 16);
										// Выполняем извлечение сетевых параметров
										this->_eth.fillsource(i->second->state.node, source);
										// Если MAC-адрес успешно получен
										if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
											// Устанавливаем полученный MAC-адрес в объект события
											this->_addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
											// Выводим результат работы функции
											return static_cast <string> (this->_addr);
										// Если MAC-адрес не получен
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
											#endif
										}
									// Если размер IP-адреса не соответствует IPv4
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Desired IPv6 address does not match set IPv4", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Desired IPv6 address does not match set IPv4", log_t::flag_t::WARNING);
										#endif
									}
								} break;
								// Если семейство сокета не определено
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
									#endif
								}
							}
						} break;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Получаем объект сервера
							auto server = awh_cast <net::server_t *> (i->second.get());
							// Если объект адреса сервера не инициализирован
							if(server->host == nullptr)
								// Прерываем выполнение
								break;
							/**
							 * Определяем семейство сокета
							 */
							switch(static_cast <uint8_t> (i->second->state.family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
								// Для семейства UDPv4
								case static_cast <uint8_t> (event::family_t::UDPV4): {
									// Получаем объект адреса хоста сервера
									net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
									// Если IP-адрес установлен и соответствует размеру IPv4
									if(host->ip->size == 4){
										// Временный объект для извлечения сетевого интерфейса
										net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
										// Устанавливаем полученный IP-адрес во временный объект
										awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (host->ip.get())->address;
										// Выполняем извлечение сетевых параметров
										this->_eth.fillsource(i->second->state.node, source);
										// Если MAC-адрес успешно получен
										if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
											// Устанавливаем полученный MAC-адрес в объект события
											this->_addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
											// Выводим результат работы функции
											return static_cast <string> (this->_addr);
										// Если MAC-адрес не получен
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
											#endif
										}
									// Если размер IP-адреса не соответствует IPv4
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Desired IPv4 address does not match set IPv6", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Desired IPv4 address does not match set IPv6", log_t::flag_t::WARNING);
										#endif
									}
								} break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
								// Для семейства UDPv6
								case static_cast <uint8_t> (event::family_t::UDPV6): {
									// Получаем объект адреса хоста сервера
									net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
									// Если IP-адрес установлен и соответствует размеру IPv6
									if(host->ip->size == 16){
										// Временный объект для извлечения сетевого интерфейса
										net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
										// Устанавливаем полученный IP-адрес во временный объект
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (host->ip.get())->address[0], 16);
										// Выполняем извлечение сетевых параметров
										this->_eth.fillsource(i->second->state.node, source);
										// Если MAC-адрес успешно получен
										if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
											// Устанавливаем полученный MAC-адрес в объект события
											this->_addr.mac(awh_cast <net::addr_mac_t *> (source.mac.get())->address);
											// Выводим результат работы функции
											return static_cast <string> (this->_addr);
										// Если MAC-адрес не получен
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
											#endif
										}
									// Если размер IP-адреса не соответствует IPv4
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Desired IPv6 address does not match set IPv4", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Desired IPv6 address does not match set IPv4", log_t::flag_t::WARNING);
										#endif
									}
								} break;
								// Если семейство сокета не определено
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("MAC-address for specified event could not be retrieved", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("MAC-address for specified event could not be retrieved", log_t::flag_t::WARNING);
									#endif
								}
							}
						} break;
					}
				} break;
				// Если тип адреса принадлежит к Unix Domain Socket
				case static_cast <uint8_t> (event::address_t::UDS): {
					// Если типы адресов соответствуют
					if(address == i->second->state.address){
						/**
						 * Определяем чем является текущая нода
						 */
						switch(static_cast <uint8_t> (i->second->state.node)){
							// Если нода является соседом
							case static_cast <uint8_t> (event::node_t::PEER): {
								// Получаем объект адреса удалённого узла
								net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
								// Если объект адреса сервера не инициализирован
								if(peer->remote == nullptr)
									// Прерываем выполнение
									break;
								// Выводим результат работы функции
								return awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (peer->remote.get())->path.get())->address;
							}
							// Если нода является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								// Получаем объект адреса удалённого узла
								net::client_t * client = awh_cast <net::client_t *> (i->second.get());
								// Если объект адреса клиента не инициализирован
								if(client->target == nullptr)
									// Прерываем выполнение
									break;
								// Выводим результат работы функции
								return awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (client->target.get())->path.get())->address;
							}
							// Если нода является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Получаем объект адреса сервера
								net::server_t * server = awh_cast <net::server_t *> (i->second.get());
								// Если объект адреса сервера не инициализирован
								if(server->host == nullptr)
									// Прерываем выполнение
									break;
								// Выводим результат работы функции
								return awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (server->host.get())->path.get())->address;
							}
						}
					// Если типы адресов не соответствуют
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Requested UDS-address does not match current address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Requested UDS-address does not match current address", log_t::flag_t::WARNING);
						#endif
					}
				} break;
				// Если тип адреса принадлежит к дирректориям файловой системы
				case static_cast <uint8_t> (event::address_t::DIR):
				// Если тип адреса принадлежит к файлам файловой системы
				case static_cast <uint8_t> (event::address_t::FILE): {
					// Если типы адресов соответствуют
					if(address == i->second->state.address){
						// Получаем объект файловой системы
						net::fs_t * fs = awh_cast <net::fs_t *> (i->second.get());
						// Если объект адреса файловой системы не инициализирован
						if(fs->path == nullptr)
							// Прерываем выполнение
							break;
						// Выводим результат работы функции
						return awh_cast <net::addr_fs_t *> (fs->path.get())->address;
					// Если типы адресов не соответствуют
					}else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Requested filesystem address does not match current address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Requested filesystem does not match current address", log_t::flag_t::WARNING);
						#endif
					}
				} break;
				// Если тип адреса принадлежит к IPv4-адресам
				case static_cast <uint8_t> (event::address_t::IPV4): {
					// Если типы адресов соответствуют
					if(address == i->second->state.address){
						/**
						 * Определяем чем является текущая нода
						 */
						switch(static_cast <uint8_t> (i->second->state.node)){
							// Если нода является соседом
							case static_cast <uint8_t> (event::node_t::PEER): {
								// Получаем объект адреса удалённого узла
								net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
								// Если объект адреса соседа не инициализирован
								if(peer->remote == nullptr)
									// Прерываем выполнение
									break;
								// Устанавливаем полученный IPv4-адрес
								this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (peer->remote.get())->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
							// Если нода является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								// Получаем объект адреса удалённого узла
								net::client_t * client = awh_cast <net::client_t *> (i->second.get());
								// Если объект адреса клиента не инициализирован
								if(client->source == nullptr)
									// Прерываем выполнение
									break;
								// Устанавливаем полученный IPv4-адрес
								this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (client->source.get())->address, net_addr_t::endian_t::LITTLE);
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
							// Если нода является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Получаем объект адреса сервера
								net::server_t * server = awh_cast <net::server_t *> (i->second.get());
								// Если объект адреса сервера не инициализирован
								if(server->host == nullptr)
									// Прерываем выполнение
									break;
								// Устанавливаем полученный IPv4-адрес
								this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (server->host.get())->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
						}
					// Если типы адресов не соответствуют
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Requested IP-address does not match current IPv4-address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Requested IP-address does not match current IPv4-address", log_t::flag_t::WARNING);
						#endif
					}
				} break;
				// Если тип адреса принадлежит к IPv6-адресам
				case static_cast <uint8_t> (event::address_t::IPV6): {
					// Если типы адресов соответствуют
					if(address == i->second->state.address){
						/**
						 * Определяем чем является текущая нода
						 */
						switch(static_cast <uint8_t> (i->second->state.node)){
							// Если нода является соседом
							case static_cast <uint8_t> (event::node_t::PEER): {
								// Получаем объект адреса удалённого узла
								net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
								// Если объект адреса соседа не инициализирован
								if(peer->remote == nullptr)
									// Прерываем выполнение
									break;
								// Устанавливаем полученный IPv6-адрес
								this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (peer->remote.get())->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
							// Если нода является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								// Получаем объект адреса удалённого узла
								net::client_t * client = awh_cast <net::client_t *> (i->second.get());
								// Если объект адреса клиента не инициализирован
								if(client->source == nullptr)
									// Прерываем выполнение
									break;
								// Устанавливаем полученный IPv6-адрес
								this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (client->source.get())->address, net_addr_t::endian_t::LITTLE);
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
							// Если нода является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Получаем объект адреса сервера
								net::server_t * server = awh_cast <net::server_t *> (i->second.get());
								// Если объект адреса сервера не инициализирован
								if(server->host == nullptr)
									// Прерываем выполнение
									break;
								// Устанавливаем полученный IPv6-адрес
								this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (server->host.get())->ip.get())->address, net_addr_t::endian_t::LITTLE);
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
						}
					// Если типы адресов не соответствуют
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Requested IP-address does not match current IPv6-address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Requested IP-address does not match current IPv6-address", log_t::flag_t::WARNING);
						#endif
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @param value   значение адреса события
 * @return        результат выполнения установки
 */
bool awh::IO::address(const event::id_t id, const event::address_t address, const string & value) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем тип адреса события
			 */
			switch(static_cast <uint8_t> (address)){
				// Если тип адреса не определён
				case static_cast <uint8_t> (event::address_t::NONE): {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Address \"%s\" type NONE cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Address \"%s\" type NONE cannot be set", log_t::flag_t::WARNING, value.c_str());
					#endif
				} break;
				// Если тип адреса принадлежит к MAC-адресам
				case static_cast <uint8_t> (event::address_t::MAC): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER):
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(value))){
								// Если адрес соответствует MAC-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::MAC): {
									/**
									 * Определяем чем является текущая нода
									 */
									switch(static_cast <uint8_t> (i->second->state.node)){
										// Если нода является соседом
										case static_cast <uint8_t> (event::node_t::PEER): {
											// Устанавливаем полученный MAC-адрес
											this->_addr.parse(value, net_addr_t::type_t::MAC);
											// Получаем объект соседа
											auto peer = awh_cast <net::peer_t *> (i->second.get());
											// Устанавливаем полученный MAC-адрес в объект события
											awh_cast <net::addr_mac_t *> (peer->mac.get())->address = ::move(this->_addr.mac());
											/**
											 * Определяем семейство сокета
											 */
											switch(static_cast <uint8_t> (i->second->state.family)){
												// Для семейства IPv4
												case static_cast <uint8_t> (event::family_t::IPV4):
												// Для семейства UDPv4
												case static_cast <uint8_t> (event::family_t::UDPV4): {
													// Временный объект для извлечения сетевого интерфейса
													net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
													// Устанавливаем полученный MAC-адрес во временный объект
													awh_cast <net::addr_mac_t *> (source.mac.get())->address = ::move(this->_addr.mac());
													// Выполняем извлечение сетевых параметров
													this->_eth.fillsource(i->second->state.node, source);
													// Если IP-адрес успешно получен
													if((result = (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0))){
														// Устанавливаем тип адреса
														i->second->state.address = event::address_t::IPV4;
														// Если объект адреса соседа не инициализирован
														if(peer->remote == nullptr)
															// Создаем новый объект адреса удалённого узла
															peer->remote = make_unique <net::attr_net_t> ();
														// Копируем IP-адрес в объект события
														awh_cast <net::attr_net_t *> (peer->remote.get())->ip = ::move(source.ip);
													// Если IP-адрес не получен
													}else {
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("MAC-address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
														/**
														* Если режим отладки не включён
														*/
														#else
															// Выводим сообщение об ошибке
															this->_log->print("MAC-address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
														#endif
													}
												} break;
												// Для семейства IPv6
												case static_cast <uint8_t> (event::family_t::IPV6):
												// Для семейства UDPv6
												case static_cast <uint8_t> (event::family_t::UDPV6): {
													// Временный объект для извлечения сетевого интерфейса
													net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
													// Устанавливаем полученный MAC-адрес во временный объект
													awh_cast <net::addr_mac_t *> (source.mac.get())->address = ::move(this->_addr.mac());
													// Выполняем извлечение сетевых параметров
													this->_eth.fillsource(i->second->state.node, source);
													// Если IP-адрес успешно получен
													if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0))){
														// Устанавливаем тип адреса
														i->second->state.address = event::address_t::IPV6;
														// Если объект адреса соседа не инициализирован
														if(peer->remote == nullptr)
															// Создаем новый объект адреса удалённого узла
															peer->remote = make_unique <net::attr_net_t> ();
														// Копируем IP-адрес в объект события
														awh_cast <net::attr_net_t *> (peer->remote.get())->ip = ::move(source.ip);
													// Если IP-адрес не получен
													} else {
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("MAC-address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
														/**
														* Если режим отладки не включён
														*/
														#else
															// Выводим сообщение об ошибке
															this->_log->print("MAC-address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
														#endif
													}
												} break;
											}
										} break;
										// Если нода является клиентом
										case static_cast <uint8_t> (event::node_t::CLIENT): {
											// Устанавливаем полученный MAC-адрес
											this->_addr.parse(value, net_addr_t::type_t::MAC);
											// Получаем объект клиента
											auto client = awh_cast <net::client_t *> (i->second.get());
											/**
											 * Определяем семейство сокета
											 */
											switch(static_cast <uint8_t> (i->second->state.family)){
												// Для семейства IPv4
												case static_cast <uint8_t> (event::family_t::IPV4):
												// Для семейства UDPv4
												case static_cast <uint8_t> (event::family_t::UDPV4): {
													// Временный объект для извлечения сетевого интерфейса
													net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
													// Устанавливаем полученный MAC-адрес во временный объект
													awh_cast <net::addr_mac_t *> (source.mac.get())->address = ::move(this->_addr.mac());
													// Выполняем извлечение сетевых параметров
													this->_eth.fillsource(i->second->state.node, source);
													// Если IP-адрес успешно получен
													if((result = (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0))){
														// Устанавливаем тип адреса
														i->second->state.address = event::address_t::IPV4;
														// Если объект адреса клиента не инициализирован
														if(client->target == nullptr){
															// Создаем новый объект адреса удалённого узла
															client->target = make_unique <net::attr_net_t> ();
															// Создаем новый объект адреса клиента IPv4
															awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
														}
														// Копируем IP-адрес в источник сетевого адреса клиента
														client->source = ::move(source.ip);
													// Если IP-адрес не получен
													} else {
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("MAC-address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
														/**
														* Если режим отладки не включён
														*/
														#else
															// Выводим сообщение об ошибке
															this->_log->print("MAC-address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
														#endif
													}
												} break;
												// Для семейства IPv6
												case static_cast <uint8_t> (event::family_t::IPV6):
												// Для семейства UDPv6
												case static_cast <uint8_t> (event::family_t::UDPV6): {
													// Временный объект для извлечения сетевого интерфейса
													net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
													// Устанавливаем полученный MAC-адрес во временный объект
													awh_cast <net::addr_mac_t *> (source.mac.get())->address = ::move(this->_addr.mac());
													// Выполняем извлечение сетевых параметров
													this->_eth.fillsource(i->second->state.node, source);
													// Если IP-адрес успешно получен
													if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0))){
														// Устанавливаем тип адреса
														i->second->state.address = event::address_t::IPV6;
														// Если объект адреса клиента не инициализирован
														if(client->target == nullptr){
															// Создаем новый объект адреса удалённого узла
															client->target = make_unique <net::attr_net_t> ();
															// Создаем новый объект адреса клиента IPv6
															awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
														}
														// Копируем IP-адрес в источник сетевого адреса клиента
														client->source = ::move(source.ip);
													// Если IP-адрес не получен
													} else {
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("MAC-address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
														/**
														* Если режим отладки не включён
														*/
														#else
															// Выводим сообщение об ошибке
															this->_log->print("MAC-address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
														#endif
													}
												} break;
											}
										} break;
										// Если нода является сервером
										case static_cast <uint8_t> (event::node_t::SERVER): {
											// Устанавливаем полученный MAC-адрес
											this->_addr.parse(value, net_addr_t::type_t::MAC);
											// Получаем объект сервера
											auto server = awh_cast <net::server_t *> (i->second.get());
											/**
											 * Определяем семейство сокета
											 */
											switch(static_cast <uint8_t> (i->second->state.family)){
												// Для семейства IPv4
												case static_cast <uint8_t> (event::family_t::IPV4):
												// Для семейства UDPv4
												case static_cast <uint8_t> (event::family_t::UDPV4): {
													// Временный объект для извлечения сетевого интерфейса
													net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
													// Устанавливаем полученный MAC-адрес во временный объект
													awh_cast <net::addr_mac_t *> (source.mac.get())->address = ::move(this->_addr.mac());
													// Выполняем извлечение сетевых параметров
													this->_eth.fillsource(i->second->state.node, source);
													// Если IP-адрес успешно получен
													if((result = (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0))){
														// Устанавливаем тип адреса
														i->second->state.address = event::address_t::IPV4;
														// Если объект адреса сервера не инициализирован
														if(server->host == nullptr)
															// Создаем новый объект адреса сервера
															server->host = make_unique <net::attr_net_t> ();
														// Копируем IP-адрес в источник сетевого адреса сервера
														awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
													// Если IP-адрес не получен
													} else {
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("MAC-address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
														/**
														* Если режим отладки не включён
														*/
														#else
															// Выводим сообщение об ошибке
															this->_log->print("MAC-address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
														#endif
													}
												} break;
												// Для семейства IPv6
												case static_cast <uint8_t> (event::family_t::IPV6):
												// Для семейства UDPv6
												case static_cast <uint8_t> (event::family_t::UDPV6): {
													// Временный объект для извлечения сетевого интерфейса
													net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
													// Устанавливаем полученный MAC-адрес во временный объект
													awh_cast <net::addr_mac_t *> (source.mac.get())->address = ::move(this->_addr.mac());
													// Выполняем извлечение сетевых параметров
													this->_eth.fillsource(i->second->state.node, source);
													// Если IP-адрес успешно получен
													if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0))){
														// Устанавливаем тип адреса
														i->second->state.address = event::address_t::IPV6;
														// Если объект адреса сервера не инициализирован
														if(server->host == nullptr)
															// Создаем новый объект адреса сервера
															server->host = make_unique <net::attr_net_t> ();
														// Копируем IP-адрес в источник сетевого адреса сервера
														awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
													// Если IP-адрес не получен
													} else {
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("MAC-address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
														/**
														* Если режим отладки не включён
														*/
														#else
															// Выводим сообщение об ошибке
															this->_log->print("MAC-address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
														#endif
													}
												} break;
											}
										} break;
									}
								} break;
								// Если адрес не принадлежит к MAC-адресу
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a MAC-address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a MAC-address", log_t::flag_t::WARNING, value.c_str());
									#endif
								}
							}
						} break;
						// Если нода имеет неподдерживаемый тип
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("MAC-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("MAC-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", log_t::flag_t::WARNING, value.c_str());
							#endif
						}
					}
				} break;
				// Если тип адреса принадлежит к Unix Domain Socket
				case static_cast <uint8_t> (event::address_t::UDS): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER):
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(value))){
								// Если адрес соответствует адресу файловой системы
								case static_cast <uint8_t> (net_addr_t::type_t::FS): {
									// Устанавливаем тип адреса
									i->second->state.address = address;
									/**
									 * Определяем чем является текущая нода
									 */
									switch(static_cast <uint8_t> (i->second->state.node)){
										// Если нода является соседом
										case static_cast <uint8_t> (event::node_t::PEER): {
											// Получаем объект адреса удалённого узла
											net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
											// Если объект адреса соседа не инициализирован
											if(peer->remote == nullptr)
												// Создаем новый объект адреса удалённого узла
												peer->remote = make_unique <net::attr_uds_t> ();
											// Устанавливаем адрес сокета в UNIX-домене
											awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (peer->remote.get())->path.get())->address = value;
											// Возвращаем результат работы функции
											return true;
										}
										// Если нода является клиентом
										case static_cast <uint8_t> (event::node_t::CLIENT): {
											// Получаем объект адреса удалённого узла
											net::client_t * client = awh_cast <net::client_t *> (i->second.get());
											// Если объект адреса клиента не инициализирован
											if(client->target == nullptr)
												// Создаем новый объект адреса удалённого узла
												client->target = make_unique <net::attr_uds_t> ();
											// Устанавливаем адрес сокета в UNIX-домене
											awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (client->target.get())->path.get())->address = value;
											// Возвращаем результат работы функции
											return true;
										}
										// Если нода является сервером
										case static_cast <uint8_t> (event::node_t::SERVER): {
											// Получаем объект адреса сервера
											net::server_t * server = awh_cast <net::server_t *> (i->second.get());
											// Если объект адреса сервера не инициализирован
											if(server->host == nullptr)
												// Создаем новый объект адреса сервера
												server->host = make_unique <net::attr_uds_t> ();
											// Устанавливаем адрес сокета в UNIX-домене
											awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (server->host.get())->path.get())->address = value;
											// Возвращаем результат работы функции
											return true;
										}
									}
								} break;
								// Если адрес не принадлежит к MAC-адресу
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a filesystem address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a filesystem address", log_t::flag_t::WARNING, value.c_str());
									#endif
								}
							}
						} break;
						// Если нода имеет неподдерживаемый тип
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Unix socket address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unix socket address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", log_t::flag_t::WARNING, value.c_str());
							#endif
						}
					}
				} break;
				// Если тип адреса принадлежит к дирректориям файловой системы
				case static_cast <uint8_t> (event::address_t::DIR):
				// Если тип адреса принадлежит к файлам файловой системы
				case static_cast <uint8_t> (event::address_t::FILE): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является файловой системой
						case static_cast <uint8_t> (event::node_t::FSYS): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(value))){
								// Если адрес соответствует адресу файловой системы
								case static_cast <uint8_t> (net_addr_t::type_t::FS): {
									// Устанавливаем тип адреса
									i->second->state.address = address;
									// Получаем объект файловой системы
									net::fs_t * fs = awh_cast <net::fs_t *> (i->second.get());
									// Если объект адреса файловой системы не инициализирован
									if(fs->path == nullptr)
										// Создаем новый объект адреса файловой системы
										fs->path = make_unique <net::addr_fs_t> ();
									// Устанавливаем адрес файловой системы события
									awh_cast <net::addr_fs_t *> (fs->path.get())->address = value;
									// Возвращаем результат работы функции
									return true;
								}
								// Если адрес не принадлежит к MAC-адресу
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a filesystem address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a filesystem address", log_t::flag_t::WARNING, value.c_str());
									#endif
								}
							}
						} break;
						// Если нода имеет неподдерживаемый тип
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Address \"%s\" can only be set for FILESYSTEM node", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Address \"%s\" can only be set for FILESYSTEM node", log_t::flag_t::WARNING, value.c_str());
							#endif
						}
					}
				} break;
				// Если тип адреса принадлежит к IPv4-адресам
				case static_cast <uint8_t> (event::address_t::IPV4): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER):
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(value))){
								// Если адрес соответствует адресу IPv4-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Устанавливаем полученный IPv4-адрес
									this->_addr.parse(value, net_addr_t::type_t::IPV4);
									// Временный объект для извлечения сетевого интерфейса
									net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
									// Устанавливаем полученный IP-адрес во временный объект
									awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = this->_addr.v4();
									// Выполняем извлечение сетевых параметров
									this->_eth.fillsource(i->second->state.node, source);
									// Если MAC-адрес успешно получен
									if((result = (::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0))){
										// Устанавливаем тип адреса
										i->second->state.address = address;
										/**
										 * Определяем чем является текущая нода
										 */
										switch(static_cast <uint8_t> (i->second->state.node)){
											// Если нода является соседом
											case static_cast <uint8_t> (event::node_t::PEER): {
												// Получаем объект соседа
												auto peer = awh_cast <net::peer_t *> (i->second.get());
												// Если объект адреса соседа не инициализирован
												if(peer->remote == nullptr)
													// Создаем новый объект адреса удалённого узла
													peer->remote = make_unique <net::attr_net_t> ();
												// Устанавливаем полученный IPv4-адрес в объект события
												awh_cast <net::attr_net_t *> (peer->remote.get())->ip = ::move(source.ip);
											} break;
											// Если нода является клиентом
											case static_cast <uint8_t> (event::node_t::CLIENT): {
												// Получаем объект клиента
												auto client = awh_cast <net::client_t *> (i->second.get());
												// Если объект адреса клиента не инициализирован
												if(client->target == nullptr){
													// Создаем новый объект адреса удалённого узла
													client->target = make_unique <net::attr_net_t> ();
													// Создаем новый объект адреса клиента IPv4
													awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
												}
												// Копируем IPv4-адрес в источник сетевого адреса клиента
												client->source = ::move(source.ip);
											} break;
											// Если нода является сервером
											case static_cast <uint8_t> (event::node_t::SERVER): {
												// Получаем объект сервера
												auto server = awh_cast <net::server_t *> (i->second.get());
												// Если объект адреса сервера не инициализирован
												if(server->host == nullptr)
													// Создаем новый объект адреса сервера
													server->host = make_unique <net::attr_net_t> ();
												// Копируем IPv4-адрес в хост сетевого адреса сервера
												awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
											} break;
										}
									// Если MAC-адрес не получен
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("IPv4-address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("IPv4-address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
										#endif
									}
								} break;
								// Если адрес не принадлежит к MAC-адресу
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"%s\" you are trying to add is not a IPv4-address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"%s\" you are trying to add is not a IPv4-address", log_t::flag_t::WARNING, value.c_str());
									#endif
								}
							}
						} break;
						// Если нода имеет неподдерживаемый тип
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("IP-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("IP-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", log_t::flag_t::WARNING, value.c_str());
							#endif
						}
					}
				} break;
				// Если тип адреса принадлежит к IPv6-адресам
				case static_cast <uint8_t> (event::address_t::IPV6): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER):
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							/**
							 * Определяем проверку соответствует ли адрес
							 */
							switch(static_cast <uint8_t> (this->_addr.host(value))){
								// Если адрес соответствует адресу IPv6-адресу
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Устанавливаем полученный IPv6-адрес
									this->_addr.parse(value, net_addr_t::type_t::IPV6);
									// Временный объект для извлечения сетевого интерфейса
									net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
									// Устанавливаем полученный IP-адрес во временный объект
									awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address = ::move(this->_addr.v6());
									// Выполняем извлечение сетевых параметров
									this->_eth.fillsource(i->second->state.node, source);
									// Если MAC-адрес успешно получен
									if((result = (::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0))){
										// Устанавливаем тип адреса
										i->second->state.address = address;
										/**
										 * Определяем чем является текущая нода
										 */
										switch(static_cast <uint8_t> (i->second->state.node)){
											// Если нода является соседом
											case static_cast <uint8_t> (event::node_t::PEER): {
												// Получаем объект соседа
												auto peer = awh_cast <net::peer_t *> (i->second.get());
												// Если объект адреса соседа не инициализирован
												if(peer->remote == nullptr)
													// Создаем новый объект адреса удалённого узла
													peer->remote = make_unique <net::attr_net_t> ();
												// Устанавливаем полученный IPv6-адрес в объект события
												awh_cast <net::attr_net_t *> (peer->remote.get())->ip = ::move(source.ip);
											} break;
											// Если нода является клиентом
											case static_cast <uint8_t> (event::node_t::CLIENT): {
												// Получаем объект клиента
												auto client = awh_cast <net::client_t *> (i->second.get());
												// Если объект адреса клиента не инициализирован
												if(client->target == nullptr){
													// Создаем новый объект адреса удалённого узла
													client->target = make_unique <net::attr_net_t> ();
													// Создаем новый объект адреса клиента IPv6
													awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
												}
												// Копируем IPv6-адрес в источник сетевого адреса клиента
												client->source = ::move(source.ip);
											} break;
											// Если нода является сервером
											case static_cast <uint8_t> (event::node_t::SERVER): {
												// Получаем объект сервера
												auto server = awh_cast <net::server_t *> (i->second.get());
												// Если объект адреса сервера не инициализирован
												if(server->host == nullptr)
													// Создаем новый объект адреса сервера
													server->host = make_unique <net::attr_net_t> ();
												// Копируем IPv6-адрес в хост сетевого адреса сервера
												awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
											} break;
										}
									// Если MAC-адрес не получен
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("IPv6-address \"[%s]\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("IPv6-address \"[%s]\" is not found", log_t::flag_t::WARNING, value.c_str());
										#endif
									}
								} break;
								// Если адрес не принадлежит к MAC-адресу
								default: {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Address \"[%s]\" you are trying to add is not a IPv6-address", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Address \"[%s]\" you are trying to add is not a IPv6-address", log_t::flag_t::WARNING, value.c_str());
									#endif
								}
							}
						} break;
						// Если нода имеет неподдерживаемый тип
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("IP-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("IP-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", log_t::flag_t::WARNING, value.c_str());
							#endif
						}
					}
				} break;
				// Если тип адреса принадлежит к сетям
				case static_cast <uint8_t> (event::address_t::NETWORK): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER):
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// IP-адрес сети
							string ip = "";
							// Маска сети
							string mask = "";
							// Тип сети
							net_addr_t::type_t type = net_addr_t::type_t::NONE;
							// Выполняем поиск разделителя сети
							auto pos = value.find('/');
							// Если разделитель найден
							if(pos != string::npos){
								// Получаем значение IP-адреса сети
								ip = value.substr(0, pos);
								// Получаем значение маски сети
								mask = value.substr(pos + 1);
							}
							/**
							 * Определяем тип полученного IP-адреса
							 */
							switch(static_cast <uint8_t> (this->_addr.host(ip))){
								// Для типа IPv4
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Если маска сети не указана
									if(mask.empty())
										// Устанавливаем маску по умолчанию для IPv4
										mask = "32";
									// Устанавливаем тип сети
									type = net_addr_t::type_t::IPV4;
								} break;
								// Для типа IPv6
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Если маска сети не указана
									if(mask.empty())
										// Устанавливаем маску по умолчанию для IPv6
										mask = "128";
									// Устанавливаем тип сети
									type = net_addr_t::type_t::IPV6;
								} break;
							}
							/**
							 * Определяем какой тип сети необходимо установить
							 */
							switch(static_cast <uint8_t> (type)){
								// Для типа IPv4
								case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
									// Выполняем парсинг IP-адреса сети
									this->_addr.parse(ip, net_addr_t::type_t::IPV4);
									// IP-адрес сети IPv4 в формате little-endian
									unique_ptr <net::addr_t> network = make_unique <net::addr_net_ipv4_t> ();
									// Получаем адрес сети IPv4 в формате little-endian
									auto addr = awh_cast <net::addr_net_ipv4_t *> (network.get());
									// Если маска является префиксом сети
									if(this->_fmk->is(mask, fmk_t::check_t::NUMBER))
										// Устанавливаем префикс сети
										addr->prefix = this->_fmk->atoi <uint8_t> (mask);
									// Если маска является стандартной маской сети
									else
										// Устанавливаем префикс сети
										addr->prefix = this->_addr.mask2Prefix(mask, type);
									// Выполняем наложение маски
									this->_addr.impose(addr->prefix, net_addr_t::addr_t::NETWORK, net_addr_t::type_t::IPV4);
									// Получаем значение IP-адреса сети
									addr->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
									// Временный объект для извлечения сетевого интерфейса
									net::src_t source(::make_unique <net::addr_net_ipv4_t> ());
									// Выполняем извлечение сетевых параметров
									this->_eth.fillsource(network, source);
									// Если IP-адрес успешно получен
									if((result = (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0))){
										// Устанавливаем тип адреса
										i->second->state.address = event::address_t::IPV4;
										/**
										 * Определяем чем является текущая нода
										 */
										switch(static_cast <uint8_t> (i->second->state.node)){
											// Если нода является соседом
											case static_cast <uint8_t> (event::node_t::PEER): {
												// Получаем текущее значение объекта соседа
												net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
												// Если объект адреса соседа не инициализирован
												if(peer->remote == nullptr)
													// Создаем новый объект адреса удалённого узла
													peer->remote = make_unique <net::attr_net_t> ();
												// Устанавливаем полученный IPv4-адрес в объект события
												awh_cast <net::attr_net_t *> (peer->remote.get())->ip = ::move(source.ip);
											} break;
											// Если нода является клиентом
											case static_cast <uint8_t> (event::node_t::CLIENT): {
												// Получаем текущее значение объекта клиента
												net::client_t * client = awh_cast <net::client_t *> (i->second.get());
												// Если объект адреса клиента не инициализирован
												if(client->target == nullptr){
													// Создаем новый объект адреса удалённого узла
													client->target = make_unique <net::attr_net_t> ();
													// Создаем новый объект адреса клиента IPv4
													awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
												}
												// Копируем IPv4-адрес в источник сетевого адреса клиента
												client->source = ::move(source.ip);
											} break;
											// Если нода является сервером
											case static_cast <uint8_t> (event::node_t::SERVER): {
												// Получаем текущее значение объекта сервера
												net::server_t * server = awh_cast <net::server_t *> (i->second.get());
												// Если объект адреса сервера не инициализирован
												if(server->host == nullptr)
													// Создаем новый объект адреса сервера
													server->host = make_unique <net::attr_net_t> ();
												// Копируем IPv4-адрес в хост сетевого адреса сервера
												awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
											} break;
										}
									// Если IP-адрес не получен
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Network address \"%s\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Network address \"%s\" is not found", log_t::flag_t::WARNING, value.c_str());
										#endif
									}
								} break;
								// Для типа IPv6
								case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
									// Выполняем парсинг IP-адреса сети
									this->_addr.parse(ip, net_addr_t::type_t::IPV6);
									// IP-адрес сети IPv6 в формате little-endian
									unique_ptr <net::addr_t> network = make_unique <net::addr_net_ipv6_t> ();
									// Получаем адрес сети IPv6 в формате little-endian
									auto addr = awh_cast <net::addr_net_ipv6_t *> (network.get());
									// Если маска является префиксом сети
									if(this->_fmk->is(mask, fmk_t::check_t::NUMBER))
										// Устанавливаем префикс сети
										addr->prefix = this->_fmk->atoi <uint8_t> (mask);
									// Если маска является стандартной маской сети
									else
										// Устанавливаем префикс сети
										addr->prefix = this->_addr.mask2Prefix(mask, type);
									// Выполняем наложение маски
									this->_addr.impose(addr->prefix, net_addr_t::addr_t::NETWORK, net_addr_t::type_t::IPV6);
									// Получаем значение IP-адреса сети
									addr->address = this->_addr.v6(net_addr_t::endian_t::LITTLE);
									// Временный объект для извлечения сетевого интерфейса
									net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
									// Выполняем извлечение сетевых параметров
									this->_eth.fillsource(network, source);
									// Если IP-адрес успешно получен
									if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0))){
										// Устанавливаем тип адреса
										i->second->state.address = event::address_t::IPV6;
										/**
										 * Определяем чем является текущая нода
										 */
										switch(static_cast <uint8_t> (i->second->state.node)){
											// Если нода является соседом
											case static_cast <uint8_t> (event::node_t::PEER): {
												// Получаем текущее значение объекта соседа
												net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
												// Если объект адреса соседа не инициализирован
												if(peer->remote == nullptr)
													// Создаем новый объект адреса удалённого узла
													peer->remote = make_unique <net::attr_net_t> ();
												// Устанавливаем полученный IPv6-адрес в объект события
												awh_cast <net::attr_net_t *> (peer->remote.get())->ip = ::move(source.ip);
											} break;
											// Если нода является клиентом
											case static_cast <uint8_t> (event::node_t::CLIENT): {
												// Получаем текущее значение объекта клиента
												net::client_t * client = awh_cast <net::client_t *> (i->second.get());
												// Если объект адреса клиента не инициализирован
												if(client->target == nullptr){
													// Создаем новый объект адреса удалённого узла
													client->target = make_unique <net::attr_net_t> ();
													// Создаем новый объект адреса клиента IPv6
													awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
												}
												// Копируем IPv6-адрес в источник сетевого адреса клиента
												client->source = ::move(source.ip);
											} break;
											// Если нода является сервером
											case static_cast <uint8_t> (event::node_t::SERVER): {
												// Получаем текущее значение объекта сервера
												net::server_t * server = awh_cast <net::server_t *> (i->second.get());
												// Если объект адреса сервера не инициализирован
												if(server->host == nullptr)
													// Создаем новый объект адреса сервера
													server->host = make_unique <net::attr_net_t> ();
												// Копируем IPv6-адрес в хост сетевого адреса сервера
												awh_cast <net::attr_net_t *> (server->host.get())->ip = ::move(source.ip);
											} break;
										}
									// Если IP-адрес не получен
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Network address \"[%s]\" is not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Network address \"[%s]\" is not found", log_t::flag_t::WARNING, value.c_str());
										#endif
									}
								} break;
							}
						} break;
						// Если нода имеет неподдерживаемый тип
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Network-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Network-address \"%s\" can only be set for PEER or CLIENT/SERVER nodes", log_t::flag_t::WARNING, value.c_str());
							#endif
						}
					}
				} break;
				// Для остальных типов адресов
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Unsupported address \"%s\" type cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::WARNING, value.c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unsupported address \"%s\" type cannot be set", log_t::flag_t::WARNING, value.c_str());
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address), value), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод удаления события
 *
 * @param id идентификатор события
 * @return   результат выполнения удаления
 */
bool awh::IO::destroy(const event::id_t id) noexcept {
	
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод создания нового события на основе существующего
 *
 * @param id       идентификатор существующего события
 * @param protocol протокол сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::id_t id, const event::protocol_t protocol) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			// Если событие уже инициализированно
			if(i->second->state.status == event::status_t::INITIAL){
				/**
				 * Определяем семейство сокета
				 */
				switch(static_cast <uint8_t> (i->second->state.family)){
					// Для семейства UNIX-доменных сокетов
					case static_cast <uint8_t> (event::family_t::UDS): {
						/**
						 * Определяем тип сокета
						 */
						switch(static_cast <uint8_t> (i->second->state.type)){
							// Для типа сокета STREAM
							case static_cast <uint8_t> (event::type_t::STREAM): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
								// Устанавливаем флаг протокола сокета
								ret.first->second->state.protocol = protocol;
								// Устанавливаем флаг типа сокета
								ret.first->second->state.type = i->second->state.type;
								// Устанавливаем флаг семейства сокета
								ret.first->second->state.family = i->second->state.family;
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем текущее значение объекта клиента
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
										// Создаем сокет подключения
										second->socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
										// Запоминаем размер структуры
										second->endpoint.size = sizeof(struct sockaddr_un);
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										net::server_t * first = awh_cast <net::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
										// Создаем сокет подключения
										second->socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
										// Запоминаем размер структуры
										second->endpoint.size = sizeof(struct sockaddr_un);
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									} break;
								}
								// Возвращаем идентификатор созданного события
								result = ret.first->first;
							} break;
							// Для типа сокета SEQPACKET
							case static_cast <uint8_t> (event::type_t::SEQPACKET): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
								// Устанавливаем флаг протокола сокета
								ret.first->second->state.protocol = protocol;
								// Устанавливаем флаг типа сокета
								ret.first->second->state.type = i->second->state.type;
								// Устанавливаем флаг семейства сокета
								ret.first->second->state.family = i->second->state.family;
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем текущее значение объекта клиента
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
										// Создаем сокет подключения
										second->socket = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
										// Запоминаем размер структуры
										second->endpoint.size = sizeof(struct sockaddr_un);
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										net::server_t * first = awh_cast <net::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
										// Создаем сокет подключения
										second->socket = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
										// Запоминаем размер структуры
										second->endpoint.size = sizeof(struct sockaddr_un);
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									} break;
								}
								// Возвращаем идентификатор созданного события
								result = ret.first->first;
							} break;
							// Для типа сокета DATAGRAM
							case static_cast <uint8_t> (event::type_t::DATAGRAM): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
								// Устанавливаем флаг протокола сокета
								ret.first->second->state.protocol = protocol;
								// Устанавливаем флаг типа сокета
								ret.first->second->state.type = i->second->state.type;
								// Устанавливаем флаг семейства сокета
								ret.first->second->state.family = i->second->state.family;
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем текущее значение объекта клиента
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
										// Создаем сокет подключения
										second->socket = ::socket(AF_UNIX, SOCK_DGRAM, 0);
										// Запоминаем размер структуры
										second->endpoint.size = sizeof(struct sockaddr_un);
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										net::server_t * first = awh_cast <net::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
										// Создаем сокет подключения
										second->socket = ::socket(AF_UNIX, SOCK_DGRAM, 0);
										// Запоминаем размер структуры
										second->endpoint.size = sizeof(struct sockaddr_un);
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									} break;
								}
								// Возвращаем идентификатор созданного события
								result = ret.first->first;
							} break;
							// Для неизвестного типа сокета
							default: {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("An event for a Unix socket cannot be created because it has an invalid initialization type", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("An event for a Unix socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
								#endif
							}
						}
					} break;
					// Для семейства UDPv4
					case static_cast <uint8_t> (event::family_t::UDPV4):
					// Для семейства UDPv6
					case static_cast <uint8_t> (event::family_t::UDPV6): {
						// Флаг удачного выполнения объединение событий
						bool ok = true;
						/**
						 * Определяем тип сокета
						 */
						switch(static_cast <uint8_t> (i->second->state.type)){
							// Для типа сокета RAW
							case static_cast <uint8_t> (event::type_t::RAW): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
								// Устанавливаем флаг протокола сокета
								ret.first->second->state.protocol = protocol;
								// Устанавливаем флаг типа сокета
								ret.first->second->state.type = i->second->state.type;
								// Устанавливаем флаг семейства сокета
								ret.first->second->state.family = i->second->state.family;
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем текущее значение объекта клиента
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства UDPv4
											case static_cast <uint8_t> (event::family_t::UDPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как RAW
													case static_cast <uint8_t> (event::protocol_t::RAW):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
													break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства UDPv6
											case static_cast <uint8_t> (event::family_t::UDPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол определён как RAW
												case static_cast <uint8_t> (event::protocol_t::RAW):
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP): break;
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.client.ss_family, SOCK_RAW, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.client.ss_family, SOCK_RAW, IPPROTO_UDP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
											// Если всё хорошо, продолжаем работу
											if(ok){
												// Выполняем копирование объекта подключения клиента
												::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
												// Выполняем копирование объекта подключения сервера
												::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
											// Если протокол не определён
											} else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
												#endif
											}
										}
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										net::server_t * first = awh_cast <net::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства UDPv4
											case static_cast <uint8_t> (event::family_t::UDPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как RAW
													case static_cast <uint8_t> (event::protocol_t::RAW):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
													break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства UDPv6
											case static_cast <uint8_t> (event::family_t::UDPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол определён как RAW
												case static_cast <uint8_t> (event::protocol_t::RAW):
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP): break;
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.server.ss_family, SOCK_RAW, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.server.ss_family, SOCK_RAW, IPPROTO_UDP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
											// Если всё хорошо, продолжаем работу
											if(ok){
												// Выполняем копирование объекта подключения клиента
												::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
												// Выполняем копирование объекта подключения сервера
												::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
											// Если протокол не определён
											} else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
												#endif
											}
										}
									} break;
								}
								// Если всё прошло успешно
								if(ok)
									// Возвращаем идентификатор созданного события
									result = ret.first->first;
								// Если всё прошло не успешно
								else
									// Удаляем созданное событие
									::__awh_nodes__.erase(ret.first);
							} break;
							// Для типа сокета DATAGRAM
							case static_cast <uint8_t> (event::type_t::DATAGRAM): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
								// Устанавливаем флаг протокола сокета
								ret.first->second->state.protocol = protocol;
								// Устанавливаем флаг типа сокета
								ret.first->second->state.type = i->second->state.type;
								// Устанавливаем флаг семейства сокета
								ret.first->second->state.family = i->second->state.family;
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем текущее значение объекта клиента
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства UDPv4
											case static_cast <uint8_t> (event::family_t::UDPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства UDPv6
											case static_cast <uint8_t> (event::family_t::UDPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP): break;
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, IPPROTO_UDP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
											// Если всё хорошо, продолжаем работу
											if(ok){
												// Выполняем копирование объекта подключения клиента
												::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
												// Выполняем копирование объекта подключения сервера
												::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
											// Если протокол не определён
											} else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
												#endif
											}
										}
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										net::server_t * first = awh_cast <net::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства UDPv4
											case static_cast <uint8_t> (event::family_t::UDPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства UDPv6
											case static_cast <uint8_t> (event::family_t::UDPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												/**
												 * Определяем протокол
												 */
												switch(static_cast <uint8_t> (protocol)){
													// Если протокол не определён
													case static_cast <uint8_t> (event::protocol_t::NONE):
													// Если протокол определён как UDP
													case static_cast <uint8_t> (event::protocol_t::UDP): break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->socket = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP): break;
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->socket = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, IPPROTO_UDP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
											// Если всё хорошо, продолжаем работу
											if(ok){
												// Выполняем копирование объекта подключения клиента
												::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
												// Выполняем копирование объекта подключения сервера
												::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
											// Если протокол не определён
											} else {
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
												#endif
											}
										}
									} break;
								}
								// Если всё прошло успешно
								if(ok)
									// Возвращаем идентификатор созданного события
									result = ret.first->first;
								// Если всё прошло не успешно
								else
									// Удаляем созданное событие
									::__awh_nodes__.erase(ret.first);
							} break;
							// Для неизвестного типа сокета
							default: {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("An event for a UDP socket cannot be created because it has an invalid initialization type", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("An event for a UDP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
								#endif
							}
						}
					} break;
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6): {
						// Флаг удачного выполнения объединение событий
						bool ok = true;
						/**
						 * Определяем тип сокета
						 */
						switch(static_cast <uint8_t> (i->second->state.type)){
							// Для типа сокета STREAM
							case static_cast <uint8_t> (event::type_t::STREAM): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
								// Устанавливаем флаг протокола сокета
								ret.first->second->state.protocol = protocol;
								// Устанавливаем флаг типа сокета
								ret.first->second->state.type = i->second->state.type;
								// Устанавливаем флаг семейства сокета
								ret.first->second->state.family = i->second->state.family;
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем текущее значение объекта клиента
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, 0);
											break;
											// Если протокол определён как TCP
											case static_cast <uint8_t> (event::protocol_t::TCP):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_TCP);
											break;
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_SCTP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										net::server_t * first = awh_cast <net::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, 0);
											break;
											// Если протокол определён как TCP
											case static_cast <uint8_t> (event::protocol_t::TCP):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_TCP);
											break;
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_SCTP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									} break;
								}
								// Если всё прошло успешно
								if(ok)
									// Возвращаем идентификатор созданного события
									result = ret.first->first;
								// Если всё прошло не успешно
								else
									// Удаляем созданное событие
									::__awh_nodes__.erase(ret.first);
							} break;
							// Для типа сокета SEQPACKET
							case static_cast <uint8_t> (event::type_t::SEQPACKET): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
								// Устанавливаем флаг протокола сокета
								ret.first->second->state.protocol = protocol;
								// Устанавливаем флаг типа сокета
								ret.first->second->state.type = i->second->state.type;
								// Устанавливаем флаг семейства сокета
								ret.first->second->state.family = i->second->state.family;
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем текущее значение объекта клиента
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.client.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										net::server_t * first = awh_cast <net::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_net_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <net::attr_net_t *> (second->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
											} break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->socket = ::socket(first->endpoint.server.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									} break;
								}
								// Если всё прошло успешно
								if(ok)
									// Возвращаем идентификатор созданного события
									result = ret.first->first;
								// Если всё прошло не успешно
								else
									// Удаляем созданное событие
									::__awh_nodes__.erase(ret.first);
							} break;
							// Для неизвестного типа сокета
							default: {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("An event for a IP socket cannot be created because it has an invalid initialization type", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("An event for a IP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
								#endif
							}
						}
					} break;
					// Для семейства директорий
					case static_cast <uint8_t> (event::family_t::DIR):
					// Для семейства файловой системы
					case static_cast <uint8_t> (event::family_t::FILE): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::fs_t> (this->_fmk, this->_log));
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = i->second->state.type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = i->second->state.family;
						// Выполняем инициализацию объекта адреса файловой системы
						awh_cast <net::fs_t &> (*ret.first->second).path = make_unique <net::addr_fs_t> ();
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для семейства таймеров
					case static_cast <uint8_t> (event::family_t::TIMER):
					// Для семейства интервалов
					case static_cast <uint8_t> (event::family_t::INTERVAL): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::timer_t> ());
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = i->second->state.type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = i->second->state.family;
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для неизвестного семейства
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Event cannot be created because family it belongs to is not defined", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Event cannot be created because family it belongs to is not defined", log_t::flag_t::WARNING);
						#endif
					}
				}
			// Событие ещё не инициализированно
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Event ID=%u has not yet been initialized", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING, id);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Event ID=%u has not yet been initialized", log_t::flag_t::WARNING, id);
				#endif
			}
		// Если событие не найдено
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Event ID=%u is not exist", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING, id);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Event ID=%u is not exist", log_t::flag_t::WARNING, id);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод создания нового события
 *
 * @param family   семейство сокета
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем семейство сокета
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_uds_t> ();
						// Получаем объект хоста UDS-сокета
						net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (client->target.get());
						// Выполняем инициализацию объекта адреса файловой системы
						target->path = make_unique <net::addr_fs_t> ();
						// Создаем сокет подключения
						client->socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для типа сокета SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_uds_t> ();
						// Получаем объект хоста UDS-сокета
						net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (client->target.get());
						// Выполняем инициализацию объекта адреса файловой системы
						target->path = make_unique <net::addr_fs_t> ();
						// Создаем сокет подключения
						client->socket = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_uds_t> ();
						// Получаем объект хоста UDS-сокета
						net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (client->target.get());
						// Выполняем инициализацию объекта адреса файловой системы
						target->path = make_unique <net::addr_fs_t> ();
						// Создаем сокет подключения
						client->socket = ::socket(AF_UNIX, SOCK_DGRAM, 0);
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"An event for a Unix socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a Unix socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства UDPv4
			case static_cast <uint8_t> (event::family_t::UDPV4):
			// Для семейства UDPv6
			case static_cast <uint8_t> (event::family_t::UDPV6): {
				// Флаг удачного выполнения объединение событий
				bool ok = true;
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета RAW
					case static_cast <uint8_t> (event::type_t::RAW): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_net_t> ();
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства UDPv4
							case static_cast <uint8_t> (event::family_t::UDPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_RAW, 0);
									break;
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::RAW):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
									break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
							} break;
							// Для семейства UDPv6
							case static_cast <uint8_t> (event::family_t::UDPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_RAW, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_RAW, IPPROTO_UDP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							} break;
						}
						// Если всё прошло успешно
						if(ok)
							// Возвращаем идентификатор созданного события
							result = ret.first->first;
						// Если всё прошло не успешно
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"RAW socket type only supports UDP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__, std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (protocol)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
							// Удаляем созданное событие
							::__awh_nodes__.erase(ret.first);
						}
					} break;
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_net_t> ();
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства UDPv4
							case static_cast <uint8_t> (event::family_t::UDPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
									break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
							} break;
							// Для семейства UDPv6
							case static_cast <uint8_t> (event::family_t::UDPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_DGRAM, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							} break;
						}
						// Если всё прошло успешно
						if(ok)
							// Возвращаем идентификатор созданного события
							result = ret.first->first;
						// Если всё прошло не успешно
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__, std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (protocol)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
							// Удаляем созданное событие
							::__awh_nodes__.erase(ret.first);
						}
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"An event for a UDP socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a UDP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Флаг удачного выполнения объединение событий
				bool ok = true;
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_net_t> ();
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_STREAM, 0);
									break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
									break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_STREAM, 0);
									break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
									break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							} break;
						}
						// Если всё прошло успешно
						if(ok)
							// Возвращаем идентификатор созданного события
							result = ret.first->first;
						// Если всё прошло не успешно
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__, std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (protocol)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
							// Удаляем созданное событие
							::__awh_nodes__.erase(ret.first);
						}
					} break;
					// Для типа сокета SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_net_t> ();
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->socket = ::socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							} break;
						}
						// Если всё прошло успешно
						if(ok)
							// Возвращаем идентификатор созданного события
							result = ret.first->first;
						// Если всё прошло не успешно
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__, std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (protocol)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
							// Удаляем созданное событие
							::__awh_nodes__.erase(ret.first);
						}
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"An event for a IP socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a IP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства директорий
			case static_cast <uint8_t> (event::family_t::DIR):
			// Для семейства файловой системы
			case static_cast <uint8_t> (event::family_t::FILE): {
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::fs_t> (this->_fmk, this->_log));
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг семейства сокета
				ret.first->second->state.family = family;
				// Устанавливаем флаг протокола сокета
				ret.first->second->state.protocol = protocol;
				// Выполняем инициализацию объекта адреса файловой системы
				awh_cast <net::fs_t &> (* ret.first->second).path = make_unique <net::addr_fs_t> ();
				// Возвращаем идентификатор созданного события
				result = ret.first->first;
			} break;
			// Для семейства таймеров
			case static_cast <uint8_t> (event::family_t::TIMER):
			// Для семейства интервалов
			case static_cast <uint8_t> (event::family_t::INTERVAL): {
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::timer_t> ());
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг семейства сокета
				ret.first->second->state.family = family;
				// Устанавливаем флаг протокола сокета
				ret.first->second->state.protocol = protocol;
				// Возвращаем идентификатор созданного события
				result = ret.first->first;
			} break;
			// Для неизвестного семейства
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(
						"Event cannot be created because family it belongs to is not defined",
						__PRETTY_FUNCTION__, std::make_tuple(
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (type),
							static_cast <uint16_t> (protocol)
						), log_t::flag_t::WARNING
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Event cannot be created because family it belongs to is not defined", log_t::flag_t::WARNING);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод получения пары событий для сокета
 *
 * @param family   семейство сокета
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @return         пара идентификаторов созданных событий
 */
std::array <awh::event::id_t, 2> awh::IO::events(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Результат работы функции
	std::array <awh::event::id_t, 2> result = {0,0};
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Список сокетов для инициализации
		int32_t fds[2] = {
			net::invalid_socket_t,
			net::invalid_socket_t
		};
		/**
		 * Определяем семейство сокета
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPC
			case static_cast <uint8_t> (event::family_t::IPC): {
				// Выполняем инициализацию файловых дескрипторов
				if(::pipe(fds) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (type),
								static_cast <uint16_t> (protocol)
							),
							log_t::flag_t::CRITICAL, ::strerror(errno)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (protocol)
									),
									log_t::flag_t::CRITICAL, ::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					} break;
					// Для типа сокета SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"%s", __PRETTY_FUNCTION__,
									std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (protocol)
									),
									log_t::flag_t::CRITICAL, ::strerror(errno)
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
						}
					} break;
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						/**
						 * Определяем протокол
						 */
						switch(static_cast <uint8_t> (protocol)){
							// Если протокол не определён
							case static_cast <uint8_t> (event::protocol_t::NONE): {
								// Выполняем инициализацию файловых дескрипторов
								if(::socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) != 0){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug(
											"%s", __PRETTY_FUNCTION__,
											std::make_tuple(
												static_cast <uint16_t> (family),
												static_cast <uint16_t> (type),
												static_cast <uint16_t> (protocol)
											),
											log_t::flag_t::CRITICAL, ::strerror(errno)
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
									#endif
								}
							} break;
						}
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"An event for a Unix socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a Unix socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства UDPv4
			case static_cast <uint8_t> (event::family_t::UDPV4):
			// Для семейства UDPv6
			case static_cast <uint8_t> (event::family_t::UDPV6): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета RAW
					case static_cast <uint8_t> (event::type_t::RAW): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства UDPv4
							case static_cast <uint8_t> (event::family_t::UDPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, 0, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как RAW
									case static_cast <uint8_t> (event::protocol_t::RAW): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_RAW, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_UDP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_IGMP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_ICMP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
								}
							} break;
							// Для семейства UDPv6
							case static_cast <uint8_t> (event::family_t::UDPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_RAW, 0, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_RAW, IPPROTO_UDP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
								}
							} break;
						}
					} break;
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства UDPv4
							case static_cast <uint8_t> (event::family_t::UDPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, 0, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, IPPROTO_UDP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, IPPROTO_IGMP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, IPPROTO_ICMP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
								}
							} break;
							// Для семейства UDPv6
							case static_cast <uint8_t> (event::family_t::UDPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_DGRAM, 0, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
								}
							} break;
						}
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"An event for a UDP socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a UDP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_STREAM, 0, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_STREAM, IPPROTO_TCP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_STREAM, IPPROTO_SCTP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_STREAM, 0, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_STREAM, IPPROTO_TCP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_STREAM, IPPROTO_SCTP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
								}
							} break;
						}
					} break;
					// Для типа сокета SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если установлен другой протокол
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													static_cast <uint16_t> (family),
													static_cast <uint16_t> (type),
													static_cast <uint16_t> (protocol)
												), log_t::flag_t::WARNING
											);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
										#endif
									}
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, fds) != 0){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"%s", __PRETTY_FUNCTION__,
													std::make_tuple(
														static_cast <uint16_t> (family),
														static_cast <uint16_t> (type),
														static_cast <uint16_t> (protocol)
													),
													log_t::flag_t::CRITICAL, ::strerror(errno)
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если установлен другой протокол
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													static_cast <uint16_t> (family),
													static_cast <uint16_t> (type),
													static_cast <uint16_t> (protocol)
												), log_t::flag_t::WARNING
											);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
										#endif
									}
								}
							} break;
						}
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"An event for a IP socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a IP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
		}
		// Если пара сокетов создана удачно
		if((fds[0] != net::invalid_socket_t) && (fds[1] != net::invalid_socket_t)){
			// Переходим по всему списку идентификаторов событий
			for(uint8_t i = 0; i < 2; i++){
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг семейства сокета
				ret.first->second->state.family = family;
				// Устанавливаем флаг протокола сокета
				ret.first->second->state.protocol = protocol;
				// Получаем объект клиента
				net::client_t * client = awh_cast <net::client_t *> (ret.first->second.get());
				/**
				 * Определяем семейство сокета
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства UNIX-доменных сокетов
					case static_cast <uint8_t> (event::family_t::UDS): {
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_uds_t> ();
						// Создаем сокет подключения
						client->socket = fds[i];
						// Выполняем инициализацию объекта адреса файловой системы
						awh_cast <net::attr_uds_t *> (client->target.get())->path = make_unique <net::addr_fs_t> ();
					} break;
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
					// Для семейства UDPv4
					case static_cast <uint8_t> (event::family_t::UDPV4): {
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_net_t> ();
						// Создаем сокет подключения
						client->socket = fds[i];
						// Выполняем инициализацию объекта IP-адреса клиента
						awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv4_t> ();
					} break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
					// Для семейства UDPv6
					case static_cast <uint8_t> (event::family_t::UDPV6): {
						// Выполняем инициализацию объекта хоста клиента
						client->target = make_unique <net::attr_net_t> ();
						// Создаем сокет подключения
						client->socket = fds[i];
						// Выполняем инициализацию объекта IP-адреса клиента
						awh_cast <net::attr_net_t *> (client->target.get())->ip = make_unique <net::addr_net_ipv6_t> ();
					} break;
				}
				// Возвращаем идентификатор созданного события
				result[i] = ret.first->first;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод получения режима действия события
 *
 * @param id     идентификатор события
 * @param action действие события
 * @return       режим действия события
 */
awh::event::notify_t awh::IO::action(const event::id_t id, const event::action_t action) const noexcept {

	// Выводим результат по умолчанию
	return event::notify_t::DISABLED;
}
/**
 * @brief Метод установки режима действия события
 *
 * @param id     идентификатор события
 * @param action действие события
 * @param notify уведомления события
 * @return       результат выполнения установки
 */
bool awh::IO::action(const event::id_t id, const event::action_t action, const event::notify_t notify) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения опций события
 *
 * @param id идентификатор события
 * @return   опции события
 */
uint16_t awh::IO::options(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end())
			// Возвращаем опции события
			return i->second->state.options;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return event::options::NONE;
}
/**
 * @brief Метод установки опций события
 *
 * @param id      идентификатор события
 * @param options опции события для установки
 * @return        результат выполнения установки
 */
bool awh::IO::options(const event::id_t id, const uint16_t options) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Файловый дескриптор события
			 */
			net::socket_t fd = net::invalid_socket_t;
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::fs_t *> (i->second.get())->fd;
				break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::peer_t *> (i->second.get())->socket;
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::client_t *> (i->second.get())->socket;
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::server_t *> (i->second.get())->socket;
				break;
				// Для других типов нод
				default: return false;
			}
			// Если файловый дескриптор события получен успешно
			if((result = (fd != net::invalid_socket_t))){
				// Флаг установки опции
				bool isSetup = false;
				// Если опция передана как TCP_CORK
				if(event::options::TCPCORK & options){
					// Активируем алгоритм TCP/CORK
					if((isSetup = this->_eth.tcpcork(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::TCPCORK;
				// Если опция не передана как TCP_CORK
				} else {
					// Деактивируем алгоритм TCP/CORK
					if((isSetup = this->_eth.tcpcork(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::TCPCORK;
				}
				// Если опция не установлена
				if(!isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как IPV6_V6ONLY
				if(event::options::IPV6ONLY & options){
					// Устанавливаем режим отображения IPv4 => IPv6
					if((isSetup = this->_eth.ipv6only(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::IPV6ONLY;
				// Если опция не передана как IPV6_V6ONLY
				} else {
					// Снимаем режим отображения IPv4 => IPv6
					if((isSetup = this->_eth.ipv6only(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::IPV6ONLY;
				}
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как NO_SIGILL
				if(event::options::NOSIGILL & options){
					// Устанавливаем игнорирование сигнала SIGILL
					if((isSetup = this->_eth.nosigill()))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::NOSIGILL;
				// Если опция не передана как NOSIGILL
				} else i->second->state.options &= ~event::options::NOSIGILL;
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как NO_SIGPIPE
				if(event::options::NOSIGPIPE & options){
					// Устанавливаем игнорирование сигнала SIGPIPE
					if((isSetup = this->_eth.nosigpipe(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::NOSIGPIPE;
				// Если опция не передана как NO_SIGPIPE
				} else {
					// Снимаем игнорирование сигнала SIGPIPE
					if((isSetup = this->_eth.nosigpipe(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::NOSIGPIPE;
				}
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как NOIOBLOCK
				if(event::options::NOIOBLOCK & options){
					// Устанавливаем неблокирующий режим ввода/вывода
					if((isSetup = this->_eth.noblocking(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::NOIOBLOCK;
				// Если опция не передана как NOIOBLOCK
				} else {
					// Снимаем неблокирующий режим ввода/вывода
					if((isSetup = this->_eth.noblocking(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::NOIOBLOCK;
				}
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как REUSEADDR
				if(event::options::REUSEADDR & options){
					// Устанавливаем режим повторного использования адреса
					if((isSetup = this->_eth.reuseaddr(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::REUSEADDR;
				// Если опция не передана как REUSEADDR
				} else {
					// Снимаем режим повторного использования адреса
					if((isSetup = this->_eth.reuseaddr(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::REUSEADDR;
				}
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как REUSEPORT
				if(event::options::REUSEPORT & options){
					// Устанавливаем режим повторного использования порта
					if((isSetup = this->_eth.reuseport(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::REUSEPORT;
				// Если опция не передана как REUSEPORT
				} else {
					// Снимаем режим повторного использования порта
					if((isSetup = this->_eth.reuseport(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::REUSEPORT;
				}
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как TCP_NODELAY
				if(event::options::TCPNODELAY & options){
					// Устанавливаем режим отключения алгоритма Нейгла
					if((isSetup = this->_eth.tcpnodelay(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::TCPNODELAY;
				// Если опция не передана как TCP_NODELAY
				} else {
					// Снимаем режим отключения алгоритма Нейгла
					if((isSetup = this->_eth.tcpnodelay(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::TCPNODELAY;
				}
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
				// Если опция передана как CLOSE_ON_EXEC
				if(event::options::CLOSEONEXEC & options){
					// Устанавливаем режим закрытия дескриптора при выполнении exec
					if((isSetup = this->_eth.closeonexec(fd, net::socket_mode_t::ENABLED)))
						// Устанавливаем опцию события
						i->second->state.options |= event::options::CLOSEONEXEC;
				// Если опция не передана как CLOSE_ON_EXEC
				} else {
					// Снимаем режим закрытия дескриптора при выполнении exec
					if((isSetup = this->_eth.closeonexec(fd, net::socket_mode_t::DISABLED)))
						// Снимаем опцию события
						i->second->state.options &= ~event::options::CLOSEONEXEC;
				}
				// Если опция не установлена
				if(result && !isSetup)
					// Устанавливаем результат работы функции как ложь
					result = false;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (options)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод установки опции события
 *
 * @param id     идентификатор события
 * @param option опция события для установки
 * @param mode   режим установки опции события
 * @return       результат выполнения установки
 */
bool awh::IO::option(const event::id_t id, const uint16_t option, const bool mode) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Файловый дескриптор события
			 */
			net::socket_t fd = net::invalid_socket_t;
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::fs_t *> (i->second.get())->fd;
				break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::peer_t *> (i->second.get())->socket;
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::client_t *> (i->second.get())->socket;
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::server_t *> (i->second.get())->socket;
				break;
				// Для других типов нод
				default: return false;
			}
			// Если файловый дескриптор события получен успешно
			if((result = (fd != net::invalid_socket_t))){
				/**
				 * Определяем опцию события которую необходимо установить или снять
				 */
				switch(option){
					// Если опция передана как TCP_CORK
					case event::options::TCPCORK: {
						// Устанавливаем или снимаем режим алгоритма TCP/CORK
						if((result = this->_eth.tcpcork(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо активировать режим алгоритма TCP/CORK
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::TCPCORK;
							// Если необходимо деактивировать режим алгоритма TCP/CORK
							else i->second->state.options ^= event::options::TCPCORK;
						}
					} break;
					// Если опция передана как IPV6_V6ONLY
					case event::options::IPV6ONLY: {
						// Устанавливаем или снимаем режим отображения IPv4 => IPv6
						if((result = this->_eth.ipv6only(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо активировать режим отображения IPv4 => IPv6
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::IPV6ONLY;
							// Если необходимо деактивировать режим отображения IPv4 => IPv6
							else i->second->state.options ^= event::options::IPV6ONLY;
						}
					} break;
					// Если опция передана как NO_SIGILL
					case event::options::NOSIGILL: {
						// Отключаем или включаем генерацию сигнала SIGILL при записи в закрытый сокет
						if((result = this->_eth.nosigill())){
							// Если необходимо отключить генерацию сигнала SIGILL
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::NOSIGILL;
							// Если необходимо включить генерацию сигнала SIGILL
							else i->second->state.options ^= event::options::NOSIGILL;
						}
					} break;
					// Если опция передана как NO_SIGPIPE
					case event::options::NOSIGPIPE: {
						// Отключаем или включаем генерацию сигнала SIGPIPE при записи в закрытый сокет
						if((result = this->_eth.nosigpipe(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо отключить генерацию сигнала SIGPIPE
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::NOSIGPIPE;
							// Если необходимо включить генерацию сигнала SIGPIPE
							else i->second->state.options ^= event::options::NOSIGPIPE;
						}
					} break;
					// Если опция передана как NOIOBLOCK
					case event::options::NOIOBLOCK: {
						// Устанавливаем или снимаем режим неблокирующий режим сокета
						if((result = this->_eth.noblocking(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо активировать неблокирующий режим
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::NOIOBLOCK;
							// Если необходимо деактивировать режим неблокирующий режим
							else i->second->state.options ^= event::options::NOIOBLOCK;
						}
					} break;
					// Если опция передана как REUSEADDR
					case event::options::REUSEADDR: {
						// Устанавливаем или снимаем режим повторного использования адреса сокета
						if((result = this->_eth.reuseaddr(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо активировать режим повторного использования адреса сокета
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::REUSEADDR;
							// Если необходимо деактивировать режим повторного использования адреса сокета
							else i->second->state.options ^= event::options::REUSEADDR;
						}
					} break;
					// Если опция передана как REUSEPORT
					case event::options::REUSEPORT: {
						// Устанавливаем или снимаем режим повторного использования порта сокета
						if((result = this->_eth.reuseport(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо активировать режим повторного использования порта сокета
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::REUSEPORT;
							// Если необходимо деактивировать режим повторного использования порта сокета
							else i->second->state.options ^= event::options::REUSEPORT;
						}
					} break;
					// Если опция передана как TCP_NODELAY
					case event::options::TCPNODELAY: {
						// Устанавливаем или снимаем алгоритм Нейгла для TCP сокета
						if((result = this->_eth.tcpnodelay(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо активировать алгоритм Нейгла для TCP сокета
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::TCPNODELAY;
							// Если необходимо деактивировать алгоритм Нейгла для TCP сокета
							else i->second->state.options ^= event::options::TCPNODELAY;
						}
					} break;
					// Если опция передана как CLOSE_ON_EXEC
					case event::options::CLOSEONEXEC: {
						// Активируем или деактивируем режим закрытия сокета при выполнении exec()
						if((result = this->_eth.closeonexec(fd, (mode ? net::socket_mode_t::ENABLED : net::socket_mode_t::DISABLED)))){
							// Если необходимо активировать режим закрытия сокета при выполнении exec()
							if(mode)
								// Устанавливаем опцию события
								i->second->state.options |= event::options::CLOSEONEXEC;
							// Если необходимо деактивировать режим закрытия сокета при выполнении exec()
							else i->second->state.options ^= event::options::CLOSEONEXEC;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, option, mode), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод отключения события
 *
 * @param id идентификатор события
 * @return   результат выполнения отключения
 */
bool awh::IO::disconnect(const event::id_t id) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод подключения события к удалённому хосту
 *
 * @param id    идентификатор события
 * @param async флаг асинхронного подключения
 * @return      результат выполнения подключения
 */
bool awh::IO::connect(const event::id_t id, const bool async) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод перевода события в режим прослушивания входящих соединений
 *
 * @param id    идентификатор события
 * @param max   максимальное количество входящих соединений
 * @param async флаг асинхронного прослушивания
 * @return      результат выполнения перевода в режим прослушивания
 */
bool awh::IO::listen(const event::id_t id, const uint16_t max, const bool async) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отправки события
 *
 * @param value значение события для отправки
 * @return      результат выполнения отправки
 */
bool awh::IO::post(const uint32_t value) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных события
 *
 * @param id   идентификатор события
 * @param data буфер данных для отправки
 * @param size размер данных для отправки
 * @return     результат выполнения отправки
 */
bool awh::IO::send(const event::id_t id, const char * data, const size_t size) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод очистки чёрного списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearBlacklist(const event::id_t id) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Получаем объект ноды
					auto node = awh_cast <net::fs_t *> (i->second.get());
					// Очищаем чёрный список
					node->blacklist.clear();
					// Устанавливаем результат
					result = node->blacklist.empty();
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем объект ноды
					auto node = awh_cast <net::server_t *> (i->second.get());
					// Очищаем чёрный список
					node->blacklist.clear();
					// Устанавливаем результат
					result = node->blacklist.empty();
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат работы функции
	return result;
}
/**
 * @brief Метод добавления адреса в чёрный список события
 *
 * @param id    идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 */
bool awh::IO::addToBlacklist(const event::id_t id, const string & value) noexcept {
	// Если адрес для удаления передан
	if(!value.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск идентификатора события
			auto i = ::__awh_nodes__.find(id);
			// Если идентификатор события найден
			if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является файловой системой
					case static_cast <uint8_t> (event::node_t::FSYS): {
						/**
						 * Определяем проверку соответствует ли адрес
						 */
						switch(static_cast <uint8_t> (this->_addr.host(value))){
							// Если адрес соответствует файловой системе
							case static_cast <uint8_t> (net_addr_t::type_t::FS):
								// Выполняем добавление нового адреса в чёрный список
								return awh_cast <net::fs_t *> (i->second.get())->blacklist.emplace(value, i->second->state.address).second;
							// Если мы получили какой-то другой адрес
							default: {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Address being added to blacklist does not match file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Address being added to blacklist does not match file system address", log_t::flag_t::WARNING);
								#endif
							}
						}
					} break;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						/**
						 * Определяем тип адреса события
						 */
						switch(static_cast <uint8_t> (i->second->state.address)){
							// Если тип адреса принадлежит к Unix Domain Socket
							case static_cast <uint8_t> (event::address_t::UDS): {
								/**
								 * Определяем проверку соответствует ли адрес
								 */
								switch(static_cast <uint8_t> (this->_addr.host(value))){
									// Если адрес соответствует файловой системе
									case static_cast <uint8_t> (net_addr_t::type_t::FS):
										// Выполняем добавление нового адреса в чёрный список
										return awh_cast <net::server_t *> (i->second.get())->blacklist.emplace(value, i->second->state.address).second;
									// Если мы получили какой-то другой адрес
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Address being added to blacklist does not match file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to blacklist does not match file system address", log_t::flag_t::WARNING);
										#endif
									}
								}
							} break;
							// Если тип адреса принадлежит к IPv4-адресам
							case static_cast <uint8_t> (event::address_t::IPV4):
							// Если тип адреса принадлежит к IPv6-адресам
							case static_cast <uint8_t> (event::address_t::IPV6): {
								/**
								 * Определяем проверку соответствует ли адрес
								 */
								switch(static_cast <uint8_t> (this->_addr.host(value))){
									// Если адрес соответствует IPv4-адресу
									case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
										// Выполняем добавление нового адреса в чёрный список
										return awh_cast <net::server_t *> (i->second.get())->blacklist.emplace(value, i->second->state.address).second;
									// Если адрес соответствует MAC-адресу
									case static_cast <uint8_t> (net_addr_t::type_t::MAC):
									// Если адрес соответствует IPv6-адресу
									case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
										// Выполняем добавление нового адреса в чёрный список
										return awh_cast <net::server_t *> (i->second.get())->blacklist.emplace(this->_fmk->transform(value, fmk_t::transform_t::UPPER_CASE), i->second->state.address).second;
									// Если мы получили какой-то другой адрес
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Address being added to blacklist does not match MAC-address or IP-address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to blacklist does not match MAC-address or IP-address", log_t::flag_t::WARNING);
										#endif
									}
								}
							} break;
						}
					} break;
					// Для других типов нод
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Blacklist does not exist for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Blacklist does not exist for this event type", log_t::flag_t::WARNING);
						#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод удаления адреса из чёрного списка события
 *
 * @param id    идентификатор события
 * @param value адрес для удаления из чёрного списка
 * @return      результат выполнения удаления
 */
bool awh::IO::removeFromBlacklist(const event::id_t id, const string & value) noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес для удаления передан
	if(!value.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск идентификатора события
			auto i = ::__awh_nodes__.find(id);
			// Если идентификатор события найден
			if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является файловой системой
					case static_cast <uint8_t> (event::node_t::FSYS): {
						// Получаем объект ноды
						auto node = awh_cast <net::fs_t *> (i->second.get());
						// Если чёрный список не пустой
						if(!node->blacklist.empty()){
							// Выполняем поиск указанного адреса
							auto i = node->blacklist.find(value);
							// Если адрес найден, удаляем его
							if((result = (i != node->blacklist.end())))
								// Выполняем удаление указанного адреса
								node->blacklist.erase(i);
						}
					} break;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Получаем объект ноды
						auto node = awh_cast <net::server_t *> (i->second.get());
						// Если чёрный список не пустой
						if(!node->blacklist.empty()){
							/**
							 * Определяем тип адреса события
							 */
							switch(static_cast <uint8_t> (i->second->state.address)){
								// Если тип адреса принадлежит к Unix Domain Socket
								case static_cast <uint8_t> (event::address_t::UDS): {
									// Выполняем поиск указанного адреса
									auto i = node->blacklist.find(value);
									// Если адрес найден, удаляем его
									if((result = (i != node->blacklist.end())))
										// Выполняем удаление указанного адреса
										node->blacklist.erase(i);
								} break;
								// Если тип адреса принадлежит к IPv4-адресам
								case static_cast <uint8_t> (event::address_t::IPV4):
								// Если тип адреса принадлежит к IPv6-адресам
								case static_cast <uint8_t> (event::address_t::IPV6): {
									// Выполняем поиск указанного адреса
									auto i = node->blacklist.find(this->_fmk->transform(value, fmk_t::transform_t::UPPER_CASE));
									// Если адрес найден, удаляем его
									if((result = (i != node->blacklist.end())))
										// Выполняем удаление указанного адреса
										node->blacklist.erase(i);
								} break;
							}
						}
					} break;
					// Для других типов нод
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Blacklist does not exist for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Blacklist does not exist for this event type", log_t::flag_t::WARNING);
						#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим резульатат работы функции
	return result;
}
/**
 * @brief Метод получения чёрного списка события
 *
 * @param id идентификатор события
 * @return   чёрный список события
 */
const std::unordered_map <string, awh::event::address_t> & awh::IO::blacklist(const event::id_t id) const noexcept {
	// Результат работы функции
	static const std::unordered_map <string, event::address_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Выводим полученный результат
					return awh_cast <net::fs_t *> (i->second.get())->blacklist;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Выводим полученный результат
					return awh_cast <net::server_t *> (i->second.get())->blacklist;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Blacklist does not exist for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Blacklist does not exist for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим резульатат работы функции
	return result;
}
/**
 * @brief Метод очистки белого списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearWhitelist(const event::id_t id) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Получаем объект ноды
					auto node = awh_cast <net::fs_t *> (i->second.get());
					// Очищаем белый список
					node->whitelist.clear();
					// Устанавливаем результат
					result = node->whitelist.empty();
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем объект ноды
					auto node = awh_cast <net::server_t *> (i->second.get());
					// Очищаем белый список
					node->whitelist.clear();
					// Устанавливаем результат
					result = node->whitelist.empty();
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат работы функции
	return result;
}
/**
 * @brief Метод добавления адреса в белый список события
 *
 * @param id    идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 */
bool awh::IO::addToWhitelist(const event::id_t id, const string & value) noexcept {
	// Если адрес для удаления передан
	if(!value.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск идентификатора события
			auto i = ::__awh_nodes__.find(id);
			// Если идентификатор события найден
			if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является файловой системой
					case static_cast <uint8_t> (event::node_t::FSYS): {
						/**
						 * Определяем проверку соответствует ли адрес
						 */
						switch(static_cast <uint8_t> (this->_addr.host(value))){
							// Если адрес соответствует файловой системе
							case static_cast <uint8_t> (net_addr_t::type_t::FS):
								// Выполняем добавление нового адреса в белый список
								return awh_cast <net::fs_t *> (i->second.get())->whitelist.emplace(value, i->second->state.address).second;
							// Если мы получили какой-то другой адрес
							default: {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Address being added to whitelist does not match file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Address being added to whitelist does not match file system address", log_t::flag_t::WARNING);
								#endif
							}
						}
					} break;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						/**
						 * Определяем тип адреса события
						 */
						switch(static_cast <uint8_t> (i->second->state.address)){
							// Если тип адреса принадлежит к Unix Domain Socket
							case static_cast <uint8_t> (event::address_t::UDS): {
								/**
								 * Определяем проверку соответствует ли адрес
								 */
								switch(static_cast <uint8_t> (this->_addr.host(value))){
									// Если адрес соответствует файловой системе
									case static_cast <uint8_t> (net_addr_t::type_t::FS):
										// Выполняем добавление нового адреса в белый список
										return awh_cast <net::server_t *> (i->second.get())->whitelist.emplace(value, i->second->state.address).second;
									// Если мы получили какой-то другой адрес
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Address being added to whitelist does not match file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to whitelist does not match file system address", log_t::flag_t::WARNING);
										#endif
									}
								}
							} break;
							// Если тип адреса принадлежит к IPv4-адресам
							case static_cast <uint8_t> (event::address_t::IPV4):
							// Если тип адреса принадлежит к IPv6-адресам
							case static_cast <uint8_t> (event::address_t::IPV6): {
								/**
								 * Определяем проверку соответствует ли адрес
								 */
								switch(static_cast <uint8_t> (this->_addr.host(value))){
									// Если адрес соответствует IPv4-адресу
									case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
										// Выполняем добавление нового адреса в белый список
										return awh_cast <net::server_t *> (i->second.get())->whitelist.emplace(value, i->second->state.address).second;
									// Если адрес соответствует MAC-адресу
									case static_cast <uint8_t> (net_addr_t::type_t::MAC):
									// Если адрес соответствует IPv6-адресу
									case static_cast <uint8_t> (net_addr_t::type_t::IPV6):
										// Выполняем добавление нового адреса в белый список
										return awh_cast <net::server_t *> (i->second.get())->whitelist.emplace(this->_fmk->transform(value, fmk_t::transform_t::UPPER_CASE), i->second->state.address).second;
									// Если мы получили какой-то другой адрес
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Address being added to whitelist does not match MAC-address or IP-address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to whitelist does not match MAC-address or IP-address", log_t::flag_t::WARNING);
										#endif
									}
								}
							} break;
						}
					} break;
					// Для других типов нод
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Whitelist does not exist for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Whitelist does not exist for this event type", log_t::flag_t::WARNING);
						#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод удаления адреса из белого списка события
 *
 * @param id    идентификатор события
 * @param value адрес для удаления из белого списка
 * @return      результат выполнения удаления
 */
bool awh::IO::removeFromWhitelist(const event::id_t id, const string & value) noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес для удаления передан
	if(!value.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск идентификатора события
			auto i = ::__awh_nodes__.find(id);
			// Если идентификатор события найден
			if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является файловой системой
					case static_cast <uint8_t> (event::node_t::FSYS): {
						// Получаем объект ноды
						auto node = awh_cast <net::fs_t *> (i->second.get());
						// Если белый список не пустой
						if(!node->whitelist.empty()){
							// Выполняем поиск указанного адреса
							auto i = node->whitelist.find(value);
							// Если адрес найден, удаляем его
							if((result = (i != node->whitelist.end())))
								// Выполняем удаление указанного адреса
								node->whitelist.erase(i);
						}
					} break;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Получаем объект ноды
						auto node = awh_cast <net::server_t *> (i->second.get());
						// Если белый список не пустой
						if(!node->whitelist.empty()){
							/**
							 * Определяем тип адреса события
							 */
							switch(static_cast <uint8_t> (i->second->state.address)){
								// Если тип адреса принадлежит к Unix Domain Socket
								case static_cast <uint8_t> (event::address_t::UDS): {
									// Выполняем поиск указанного адреса
									auto i = node->whitelist.find(value);
									// Если адрес найден, удаляем его
									if((result = (i != node->whitelist.end())))
										// Выполняем удаление указанного адреса
										node->whitelist.erase(i);
								} break;
								// Если тип адреса принадлежит к IPv4-адресам
								case static_cast <uint8_t> (event::address_t::IPV4):
								// Если тип адреса принадлежит к IPv6-адресам
								case static_cast <uint8_t> (event::address_t::IPV6): {
									// Выполняем поиск указанного адреса
									auto i = node->whitelist.find(this->_fmk->transform(value, fmk_t::transform_t::UPPER_CASE));
									// Если адрес найден, удаляем его
									if((result = (i != node->whitelist.end())))
										// Выполняем удаление указанного адреса
										node->whitelist.erase(i);
								} break;
							}
						}
					} break;
					// Для других типов нод
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Whitelist does not exist for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Whitelist does not exist for this event type", log_t::flag_t::WARNING);
						#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим резульатат работы функции
	return result;
}
/**
 * @brief Метод получения белого списка события
 *
 * @param id идентификатор события
 * @return   белый список события
 */
const std::unordered_map <string, awh::event::address_t> & awh::IO::whitelist(const event::id_t id) const noexcept {
	// Результат работы функции
	static const std::unordered_map <string, event::address_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Выводим полученный результат
					return awh_cast <net::fs_t *> (i->second.get())->whitelist;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Выводим полученный результат
					return awh_cast <net::server_t *> (i->second.get())->whitelist;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Whitelist does not exist for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Whitelist does not exist for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим резульатат работы функции
	return result;
}
/**
 * @brief Метод установки глубины очереди принятия входящих соединений события
 *
 * @param id       идентификатор события
 * @param depth    глубина очереди принятия входящих соединений
 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
 */
void awh::IO::backlog(const event::id_t id, const uint16_t depth, const bool adaptive) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Выполняем создание новой ноды
					auto node = awh_cast <net::server_t *> (i->second.get());
					// Устанавливаем глубину очереди принятия входящих соединений
					node->backlog.depth = depth;
					// Устанавливаем режим адаптивной глубины очереди принятия входящих соединений
					node->backlog.adaptive = adaptive;
				} break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Event incoming connection accept queue depth cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Event incoming connection accept queue depth cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, depth, adaptive), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @return       размер буфера события
 */
size_t awh::IO::bufferSize(const event::id_t id, const event::action_t action) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end()){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Получаем текущее значение объекта файловой системы
					net::fs_t * fs = awh_cast <net::fs_t *> (i->second.get());
					/**
					 * Определяем тип действия события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Извлекаем размер буфера на чтение
							return fs->transfer.input.size;
						// Если действие не определено
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Buffer size can only be get for read action on file system events", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Buffer size can only be get for read action on file system events", log_t::flag_t::WARNING);
							#endif
						}
					}
				} break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Получаем текущее значение объекта соседа
					net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(peer->socket, net::socket_event_t::READ);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на чтение
										return static_cast <size_t> (length);
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE): {
									// Устанавливаем размер буфера для записи
									const int32_t length = this->_eth.bufferSize(peer->socket, net::socket_event_t::WRITE);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на запись
										return static_cast <size_t> (length);
								} break;
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(peer->socket, net::socket_event_t::READ);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на чтение
										return static_cast <size_t> (length);
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE): {
									// Устанавливаем размер буфера для записи
									const int32_t length = this->_eth.bufferSize(peer->socket, net::socket_event_t::WRITE);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на запись
										return static_cast <size_t> (length);
								} break;
							}
						} break;
					}
				} break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем текущее значение объекта клиента
					net::client_t * client = awh_cast <net::client_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(client->socket, net::socket_event_t::READ);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на чтение
										return static_cast <size_t> (length);
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE): {
									// Устанавливаем размер буфера для записи
									const int32_t length = this->_eth.bufferSize(client->socket, net::socket_event_t::WRITE);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на запись
										return static_cast <size_t> (length);
								} break;
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(client->socket, net::socket_event_t::READ);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на чтение
										return static_cast <size_t> (length);
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE): {
									// Устанавливаем размер буфера для записи
									const int32_t length = this->_eth.bufferSize(client->socket, net::socket_event_t::WRITE);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на запись
										return static_cast <size_t> (length);
								} break;
							}
						} break;
					}
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем текущее значение объекта сервера
					net::server_t * server = awh_cast <net::server_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(server->socket, net::socket_event_t::READ);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на чтение
										return static_cast <size_t> (length);
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE): {
									// Устанавливаем размер буфера для записи
									const int32_t length = this->_eth.bufferSize(server->socket, net::socket_event_t::WRITE);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на запись
										return static_cast <size_t> (length);
								} break;
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(server->socket, net::socket_event_t::READ);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на чтение
										return static_cast <size_t> (length);
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE): {
									// Устанавливаем размер буфера для записи
									const int32_t length = this->_eth.bufferSize(server->socket, net::socket_event_t::WRITE);
									// Если установка размера буфера прошла успешно
									if(length > 0)
										// Выводим размер буфера на запись
										return static_cast <size_t> (length);
								} break;
							}
						} break;
					}
				} break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Buffer size cannot be get for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Buffer size cannot be get for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 */
bool awh::IO::bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Получаем текущее значение объекта файловой системы
					net::fs_t * fs = awh_cast <net::fs_t *> (i->second.get());
					/**
					 * Определяем тип действия события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ): {
							// Устанавливаем размер буфера на чтение
							fs->transfer.input.size = size;
							// Выполняем создание нового буфера на чтение
							fs->transfer.input.data = make_unique <uint8_t []> (fs->transfer.input.size);
						} break;
						// Если действие не определено
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Buffer size can only be set for read action on file system events", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action), size), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Buffer size can only be set for read action on file system events", log_t::flag_t::WARNING);
							#endif
						}
					}
				} break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Получаем текущее значение объекта соседа
					net::peer_t * peer = awh_cast <net::peer_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(peer->socket, net::socket_event_t::READ, static_cast <int32_t> (size));
									// Если установка размера буфера прошла успешно
									if((result = (length > 0))){
										// Устанавливаем размер буфера на чтение
										peer->transfer.input.size = static_cast <size_t> (length);
										// Выполняем создание нового буфера на чтение
										peer->transfer.input.data = make_unique <uint8_t []> (peer->transfer.input.size);
									}
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE):
									// Устанавливаем размер буфера для записи
									result = (this->_eth.bufferSize(peer->socket, net::socket_event_t::WRITE, static_cast <int32_t> (size)) > 0);
								break;
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(peer->socket, net::socket_event_t::READ, static_cast <int32_t> (size));
									// Если установка размера буфера прошла успешно
									if((result = (length > 0))){
										// Устанавливаем размер буфера на чтение
										peer->transfer.input.size = static_cast <size_t> (length);
										// Выполняем создание нового буфера на чтение
										peer->transfer.input.data = make_unique <uint8_t []> (peer->transfer.input.size);
									}
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE):
									// Устанавливаем размер буфера для записи
									result = (this->_eth.bufferSize(peer->socket, net::socket_event_t::WRITE, static_cast <int32_t> (size)) > 0);
								break;
							}
						} break;
					}
				} break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем текущее значение объекта клиента
					net::client_t * client = awh_cast <net::client_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(client->socket, net::socket_event_t::READ, static_cast <int32_t> (size));
									// Если установка размера буфера прошла успешно
									if((result = (length > 0))){
										// Устанавливаем размер буфера на чтение
										client->transfer.input.size = static_cast <size_t> (length);
										// Выполняем создание нового буфера на чтение
										client->transfer.input.data = make_unique <uint8_t []> (client->transfer.input.size);
									}
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE):
									// Устанавливаем размер буфера для записи
									result = (this->_eth.bufferSize(client->socket, net::socket_event_t::WRITE, static_cast <int32_t> (size)) > 0);
								break;
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(client->socket, net::socket_event_t::READ, static_cast <int32_t> (size));
									// Если установка размера буфера прошла успешно
									if((result = (length > 0))){
										// Устанавливаем размер буфера на чтение
										client->transfer.input.size = static_cast <size_t> (length);
										// Выполняем создание нового буфера на чтение
										client->transfer.input.data = make_unique <uint8_t []> (client->transfer.input.size);
									}
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE):
									// Устанавливаем размер буфера для записи
									result = (this->_eth.bufferSize(client->socket, net::socket_event_t::WRITE, static_cast <int32_t> (size)) > 0);
								break;
							}
						} break;
					}
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем текущее значение объекта сервера
					net::server_t * server = awh_cast <net::server_t *> (i->second.get());
					/**
					 * Определяем семейство сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.family)){
						// Для семейства UNIX-доменных сокетов
						case static_cast <uint8_t> (event::family_t::UDS): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(server->socket, net::socket_event_t::READ, static_cast <int32_t> (size));
									// Если установка размера буфера прошла успешно
									if((result = (length > 0))){
										// Устанавливаем размер буфера на чтение
										server->transfer.input.size = static_cast <size_t> (length);
										// Выполняем создание нового буфера на чтение
										server->transfer.input.data = make_unique <uint8_t []> (server->transfer.input.size);
									}
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE):
									// Устанавливаем размер буфера для записи
									result = (this->_eth.bufferSize(server->socket, net::socket_event_t::WRITE, static_cast <int32_t> (size)) > 0);
								break;
							}
						} break;
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем тип действия события
							 */
							switch(static_cast <uint8_t> (action)){
								// Если действие является чтением
								case static_cast <uint8_t> (event::action_t::READ): {
									// Устанавливаем размер буфера для чтения
									const int32_t length = this->_eth.bufferSize(server->socket, net::socket_event_t::READ, static_cast <int32_t> (size));
									// Если установка размера буфера прошла успешно
									if((result = (length > 0))){
										// Устанавливаем размер буфера на чтение
										server->transfer.input.size = static_cast <size_t> (length);
										// Выполняем создание нового буфера на чтение
										server->transfer.input.data = make_unique <uint8_t []> (server->transfer.input.size);
									}
								} break;
								// Если действие является записью
								case static_cast <uint8_t> (event::action_t::WRITE):
									// Устанавливаем размер буфера для записи
									result = (this->_eth.bufferSize(server->socket, net::socket_event_t::WRITE, static_cast <int32_t> (size)) > 0);
								break;
							}
						} break;
					}
				} break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Buffer size cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action), size), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Buffer size cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action), size), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод получения таймаута события
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @return       значение таймаута в миллисекундах
 */
uint16_t awh::IO::timeout(const event::id_t id, const event::action_t action) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является таймером
				case static_cast <uint8_t> (event::node_t::TIMER):
					// Выводим значение задержки времени таймера
					return awh_cast <net::timer_t *> (i->second.get())->delay;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Получаем объект события соседа
					auto peer = awh_cast <net::peer_t *> (i->second.get());
					// Выполняем поиск таймаута для действия события
					auto i = peer->timeouts.find(action);
					// Если таймаут для действия события найден
					if(i != peer->timeouts.end())
						// Выводим значение таймаута для действия события
						return i->second;
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем объект события сервера
					auto server = awh_cast <net::server_t *> (i->second.get());
					// Выполняем поиск таймаута для действия события
					auto i = server->timeouts.find(action);
					// Если таймаут для действия события найден
					if(i != server->timeouts.end())
						// Выводим значение таймаута для действия события
						return i->second;
				} break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем объект события клиента
					auto client = awh_cast <net::client_t *> (i->second.get());
					// Выполняем поиск таймаута для действия события
					auto i = client->timeouts.find(action);
					// Если таймаут для действия события найден
					if(i != client->timeouts.end())
						// Выводим значение таймаута для действия события
						return i->second;
				} break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Unable to set timeout for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unable to set timeout for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута события
 *
 * @param id      идентификатор события
 * @param action  тип действия события
 * @param timeout значение таймаута в миллисекундах
 */
void awh::IO::timeout(const event::id_t id, const event::action_t action, const uint16_t timeout) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является таймером
				case static_cast <uint8_t> (event::node_t::TIMER):
					// Устанавливаем значение задержки времени таймера
					awh_cast <net::timer_t *> (i->second.get())->delay = timeout;
				break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Получаем объект события соседа
					auto peer = awh_cast <net::peer_t *> (i->second.get());
					// Устанавливаем значение таймаута для действия события
					peer->timeouts[action] = timeout;
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Получаем объект события сервера
					auto server = awh_cast <net::server_t *> (i->second.get());
					// Устанавливаем значение таймаута для действия события
					server->timeouts[action] = timeout;
				} break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Получаем объект события клиента
					auto client = awh_cast <net::client_t *> (i->second.get());
					// Устанавливаем значение таймаута для действия события
					client->timeouts[action] = timeout;
				} break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Unable to set timeout for this event type", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unable to set timeout for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (action), timeout), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки параметров keep-alive для события
 *
 * @param id    идентификатор события
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::IO::keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Файловый дескриптор события
			 */
			net::socket_t socket = net::invalid_socket_t;
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Получаем файловый дескриптор события
					socket = awh_cast <net::peer_t *> (i->second.get())->socket;
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Получаем файловый дескриптор события
					socket = awh_cast <net::client_t *> (i->second.get())->socket;
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Получаем файловый дескриптор события
					socket = awh_cast <net::server_t *> (i->second.get())->socket;
				break;
				// Для других типов нод
				default: return false;
			}
			// Если файловый дескриптор события получен успешно
			if((result = (socket != net::invalid_socket_t))){
				// Устанавливаем параметры keep-alive для сокета события
				if((result = this->_eth.keepalive(socket, cnt, idle, intvl)))
					// Устанавливаем флаг keep-alive в состояние включено
					i->second->state.options |= event::options::KEEPALIVE;
				// Если не удалось установить параметры keep-alive для сокета события
				else i->second->state.options &= ~event::options::KEEPALIVE;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, cnt, idle, intvl), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод приостановки события
 *
 * @param id идентификатор события
 * @return   результат выполнения приостановки
 */
bool awh::IO::pause(const event::id_t id) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод возобновления события
 *
 * @param id идентификатор события
 * @return   результат выполнения возобновления
 */
bool awh::IO::resume(const event::id_t id) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод проверки состояния события
 *
 * @param id идентификатор события
 * @return   состояние события
 */
bool awh::IO::isAlive(const event::id_t id) const noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод инициализации основного движка фреймворка
 *
 * @return результат выполнения инициализации
 */
bool awh::IO::initialize() noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод деинициализации основного движка фреймворка
 *
 * @return результат выполнения деинициализации
 */
bool awh::IO::deinitialize() noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод проверки состояния инициализации основного движка фреймворка
 *
 * @return состояние инициализации
 */
bool awh::IO::isInitialized() const noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения типа события
 *
 * @param id идентификатор события
 * @return   тип события
 */
awh::event::type_t awh::IO::type(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end())
			// Возвращаем тип события
			return i->second->state.type;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return event::type_t::NONE;
}
/**
 * @brief Метод получения семейства события
 *
 * @param id идентификатор события
 * @return   семейство события
 */
awh::event::family_t awh::IO::family(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end())
			// Возвращаем семейство события
			return i->second->state.family;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return event::family_t::NONE;
}
/**
 * @brief Метод получения статуса события
 *
 * @param id идентификатор события
 * @return   статус события
 */
awh::event::status_t awh::IO::status(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end())
			// Возвращаем статус события
			return i->second->state.status;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return event::status_t::NONE;
}
/**
 * @brief Метод опроса событий
 *
 * @param timeout таймаут опроса в миллисекундах
 * @return        результат выполнения опроса
 */
bool awh::IO::poll(const int32_t timeout) noexcept {

	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Методы установки функции обратного вызова на чтение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::read_t & cb) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Устанавливаем функцию обратного вызова на чтение события
					awh_cast <net::fs_t *> (i->second.get())->callbacks.read = ::move(cb);
				break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Устанавливаем функцию обратного вызова на чтение события
					awh_cast <net::peer_t *> (i->second.get())->callbacks.read = ::move(cb);
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Устанавливаем функцию обратного вызова на чтение события
					awh_cast <net::client_t *> (i->second.get())->callbacks.read = ::move(cb);
				break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("A data read callback cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("A data read callback cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Методы установки функции обратного вызова на запись события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::write_t & cb) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::fs_t *> (i->second.get())->callbacks.write = ::move(cb);
				break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::peer_t *> (i->second.get())->callbacks.write = ::move(cb);
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::client_t *> (i->second.get())->callbacks.write = ::move(cb);
				break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("A data write callback cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("A data write callback cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Методы установки функции обратного вызова на ошибку события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::error_t & cb) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::fs_t *> (i->second.get())->callbacks.error = ::move(cb);
				break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::peer_t *> (i->second.get())->callbacks.error = ::move(cb);
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::client_t *> (i->second.get())->callbacks.error = ::move(cb);
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::server_t *> (i->second.get())->callbacks.error = ::move(cb);
				break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("A error callback cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("A error callback cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Методы установки функции обратного вызова на изменение статуса события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::status_t & cb) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::fs_t *> (i->second.get())->callbacks.status = ::move(cb);
				break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::peer_t *> (i->second.get())->callbacks.status = ::move(cb);
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::client_t *> (i->second.get())->callbacks.status = ::move(cb);
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::server_t *> (i->second.get())->callbacks.status = ::move(cb);
				break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("A status callback cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("A status callback cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Методы установки функции обратного вызова на принятие события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::accept_t & cb) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::server_t *> (i->second.get())->callbacks.accept = ::move(cb);
				break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("A accept callback cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("A accept callback cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Методы установки функции обратного вызова на подключение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::connect_t & cb) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::client_t *> (i->second.get())->callbacks.connect = ::move(cb);
				break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("A connect callback cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("A connect callback cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Методы установки функции обратного вызова на получение пользовательского события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::user_t & cb) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if((i != ::__awh_nodes__.end()) && (i->second->state.status.load(std::memory_order_acquire) != event::status_t::DESTROYED)){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является пользовательским событием
				case static_cast <uint8_t> (event::node_t::USER):
					// Устанавливаем функцию обратного вызова на запись события
					awh_cast <net::user_t *> (i->second.get())->callback = ::move(cb);
				break;
				// Для других типов нод
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("A user callback cannot be set for this event type", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("A user callback cannot be set for this event type", log_t::flag_t::WARNING);
					#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::IO::IO(const fmk_t * fmk, const log_t * log) noexcept : engine_t(fmk, log) {

}
/**
 * @brief Деструктор
 *
 */
awh::IO::~IO() noexcept {

}
