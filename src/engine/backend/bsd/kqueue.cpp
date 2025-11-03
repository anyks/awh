/**
 * @file: kqueue.cpp
 * @date: 2025-10-27
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
#include <unordered_map>

/**
 * Подключаем системные заголовки
 */
#include <fcntl.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/ethernet.h>
#include <sys/un.h>
#include <sys/event.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/**
 * Подключаем наши заголовочные файлы
 */
#include <sys/os.hpp>

/**
 * Подключаем заголовочный файл асинхронного движка ввода-вывода
 */
#include <engine/io.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Глобальная переменная списка предварительной настройки событий
 */
static unordered_map <awh::event::id_t, awh::sys_t::leadup_t> __awh_leadup__;

/**
 * Глобальная переменная списка узлов событий
 */
static unordered_map <awh::event::id_t, unique_ptr <awh::sys_t::node_t>> __awh_nodes__;

/**
 * @brief Функция генерации уникального идентификатора
 *
 * @return уникальный идентификатор
 */
static uint32_t identifier() noexcept {
	// Начинаем с 1 (0 можно оставить как "invalid")
	static atomic_uint32_t id{1};
	// Выводим новое значение идентификатора
	return id.fetch_add(1, memory_order_relaxed);
}

/**
 * @brief Метод опроса событий
 *
 * @param timeout таймаут опроса в миллисекундах
 * @return        результат выполнения опроса
 */
bool awh::IO::poll(const int32_t timeout) noexcept {

	return false;
}
/**
 * @brief Метод настройки события
 *
 * @param id    идентификатор события
 * @param delay задержка таймера события в миллисекундах
 * @return      результат выполнения настройки
 */
bool awh::IO::setup(const event::id_t id, const uint16_t delay) noexcept {
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
		if(i != ::__awh_nodes__.end()){
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
							return awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->port;
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
							// Возвращаем результат работы функции
							return awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->port;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER):
							// Возвращаем результат работы функции
							return awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->port;
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
	// Возвращаем результат работы функции
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
		if(i != ::__awh_nodes__.end()){
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
							// Получаем объект хоста соседа
							sys_t::host_ip_t * host = awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get());
							/**
							 * Определяем семейство сокета
							 */
							switch(static_cast <uint8_t> (i->second->state.family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
								// Для семейства UDPv4
								case static_cast <uint8_t> (event::family_t::UDPV4):
									// Инициализируем IPv4-адрес ноды
									host->ip = make_unique <sys_t::address_network_ipv4_t> ();
								break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
								// Для семейства UDPv6
								case static_cast <uint8_t> (event::family_t::UDPV6):
									// Инициализируем IPv6-адрес ноды
									host->ip = make_unique <sys_t::address_network_ipv6_t> ();
								break;
							}
							// Устанавливаем порт события
							host->port = port;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Получаем объект хоста клиента
							sys_t::host_ip_t * host = awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get());
							/**
							 * Определяем семейство сокета
							 */
							switch(static_cast <uint8_t> (i->second->state.family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
								// Для семейства UDPv4
								case static_cast <uint8_t> (event::family_t::UDPV4):
									// Инициализируем IPv4-адрес ноды
									host->ip = make_unique <sys_t::address_network_ipv4_t> ();
								break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
								// Для семейства UDPv6
								case static_cast <uint8_t> (event::family_t::UDPV6):
									// Инициализируем IPv6-адрес ноды
									host->ip = make_unique <sys_t::address_network_ipv6_t> ();
								break;
							}
							// Устанавливаем порт события
							host->port = port;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Получаем объект хоста сервера
							sys_t::host_ip_t * host = awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get());
							/**
							 * Определяем семейство сокета
							 */
							switch(static_cast <uint8_t> (i->second->state.family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
								// Для семейства UDPv4
								case static_cast <uint8_t> (event::family_t::UDPV4):
									// Инициализируем IPv4-адрес ноды
									host->ip = make_unique <sys_t::address_network_ipv4_t> ();
								break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
								// Для семейства UDPv6
								case static_cast <uint8_t> (event::family_t::UDPV6):
									// Инициализируем IPv6-адрес ноды
									host->ip = make_unique <sys_t::address_network_ipv6_t> ();
								break;
							}
							// Устанавливаем порт события
							host->port = port;
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
	// Возвращаем результат работы функции
	return false;
}
/**
 * @brief Метод получения хоста события
 *
 * @param id идентификатор события
 * @return   хост события
 */
string awh::IO::host(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end()){
			/**
			 * Определяем семейство сокета
			 */
			switch(static_cast <uint8_t> (i->second->state.family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
				// Для семейства UDPv4
				case static_cast <uint8_t> (event::family_t::UDPV4): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v4(awh_cast <sys_t::address_network_ipv4_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.v4(awh_cast <sys_t::address_network_ipv4_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v4(awh_cast <sys_t::address_network_ipv4_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
						}
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
				// Для семейства UDPv6
				case static_cast <uint8_t> (event::family_t::UDPV6): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v6(awh_cast <sys_t::address_network_ipv6_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_net);
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.v6(awh_cast <sys_t::address_network_ipv6_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_net);
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v6(awh_cast <sys_t::address_network_ipv6_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_net);
						}
					}
				} break;
				// Для семейства UNIX-доменных сокетов
				case static_cast <uint8_t> (event::family_t::UDS): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER):
							// Возвращаем адрес сокета в UNIX-домене
							return awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->path.get())->address;
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
							// Возвращаем адрес сокета в UNIX-домене
							return awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->path.get())->address;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER):
							// Возвращаем адрес сокета в UNIX-домене
							return awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->path.get())->address;
					}
				} break;
				// Для семейства директорий
				case static_cast <uint8_t> (event::family_t::DIR):
				// Для семейства файловой системы
				case static_cast <uint8_t> (event::family_t::FILE):
					// Возвращаем адрес файловой системы события
					return awh_cast <sys_t::address_fs_t *> (static_cast <sys_t::filesystem_t *> (i->second.get())->path.get())->address;
				// Для остальных семейств сокетов
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Host cannot be retrieved for events that are not network or filesystem related", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Host cannot be retrieved for events that are not network or filesystem related", log_t::flag_t::WARNING);
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
	// Возвращаем результат работы функции
	return "";
}
/**
 * @brief Метод установки хоста события
 *
 * @param id   идентификатор события
 * @param host хост события
 * @return     результат выполнения установки
 */
bool awh::IO::host(const event::id_t id, const string & host) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end()){
			/**
			 * Определяем семейство сокета
			 */
			switch(static_cast <uint8_t> (i->second->state.family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
				// Для семейства UDPv4
				case static_cast <uint8_t> (event::family_t::UDPV4): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(host, net_t::type_t::IPV4);
							// Получаем указатель на объект соседа
							auto peer = awh_cast <sys_t::peer_t *> (i->second.get());
							// Получаем объект адреса для установки данных
							auto address = awh_cast <sys_t::address_network_ipv4_t *> (awh_cast <sys_t::host_ip_t *> (peer->host.get())->ip.get());
							// Копируем полученный IP-адрес в объект события
							address->address = this->_net.v4(net_t::endian_t::LITTLE);
							// Параметры сетей интерфейсов
							sys_t::addresses_t addresses{};
							// Устанавливаем хост сервера для получения MAC-адреса
							awh_cast <sys_t::address_network_ipv4_t *> (addresses.ip.get())->address = address->address;
							// Получаем MAC-адрес из системы по IP-адресу
							this->_sys.peerAddresses(addresses);
							// Получаем объект адреса для установки данных
							peer->mac = ::move(addresses.mac);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(host, net_t::type_t::IPV4);
							// Получаем указатель на объект клиента
							auto client = awh_cast <sys_t::client_t *> (i->second.get());
							// Получаем объект адреса для установки данных
							auto & address = awh_cast <sys_t::host_ip_t *> (client->host.get())->ip;
							// Копируем полученный IP-адрес в объект события
							awh_cast <sys_t::address_network_ipv4_t *> (address.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
							// Получаем название сетевого интерфейса по IP-адресу
							string iface = ::move(this->_sys.interfaceName(address));
							// Если название сетевого интерфейса не пустое
							if(!iface.empty())
								// Устанавливаем название сетевого интерфейса
								client->iface = ::move(iface);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(host, net_t::type_t::IPV4);
							// Получаем указатель на объект сервера
							auto server = awh_cast <sys_t::server_t *> (i->second.get());
							// Получаем объект адреса для установки данных
							auto & address = awh_cast <sys_t::host_ip_t *> (server->host.get())->ip;
							// Копируем полученный IP-адрес в объект события
							awh_cast <sys_t::address_network_ipv4_t *> (address.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
							// Получаем название сетевого интерфейса по IP-адресу
							string iface = ::move(this->_sys.interfaceName(address));
							// Если название сетевого интерфейса не пустое
							if(!iface.empty())
								// Устанавливаем название сетевого интерфейса
								server->iface = ::move(iface);
							// Параметры сетей интерфейсов
							sys_t::addresses_t addresses{};
							// Устанавливаем хост сервера для получения MAC-адреса
							awh_cast <sys_t::address_network_ipv4_t *> (addresses.ip.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
							// Получаем MAC-адрес из системы по IP-адресу
							this->_sys.nodeAddresses(addresses);
							// Получаем объект адреса для установки данных
							server->mac = ::move(addresses.mac);
							// Возвращаем результат работы функции
							return true;
						}
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
				// Для семейства UDPv6
				case static_cast <uint8_t> (event::family_t::UDPV6): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(host, net_t::type_t::IPV6);
							// Получаем указатель на объект соседа
							auto peer = awh_cast <sys_t::peer_t *> (i->second.get());
							// Получаем объект адреса для установки данных
							auto & address = awh_cast <sys_t::host_ip_t *> (peer->host.get())->ip;
							// Устанавливаем полученный IP-адрес в объект события
							awh_cast <sys_t::address_network_ipv6_t *> (address.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
							// Параметры сетей интерфейсов
							sys_t::addresses_t addresses(make_unique <sys_t::address_network_ipv6_t> ());
							// Устанавливаем IP-адрес для получения MAC-адреса
							awh_cast <sys_t::address_network_ipv6_t *> (addresses.ip.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
							// Получаем MAC-адрес из системы по IP-адресу
							this->_sys.peerAddresses(addresses);
							// Получаем объект адреса для установки данных
							peer->mac = ::move(addresses.mac);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(host, net_t::type_t::IPV6);
							// Получаем указатель на объект клиента
							auto client = awh_cast <sys_t::client_t *> (i->second.get());
							// Получаем объект адреса для установки данных
							auto & address = awh_cast <sys_t::host_ip_t *> (client->host.get())->ip;
							// Устанавливаем полученный IP-адрес в объект события
							awh_cast <sys_t::address_network_ipv6_t *> (address.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
							// Получаем название сетевого интерфейса по IP-адресу
							string iface = ::move(this->_sys.interfaceName(address));
							// Если название сетевого интерфейса не пустое
							if(!iface.empty())
								// Устанавливаем название сетевого интерфейса
								client->iface = ::move(iface);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(host, net_t::type_t::IPV6);
							// Получаем указатель на объект сервера
							auto server = awh_cast <sys_t::server_t *> (i->second.get());
							// Получаем объект адреса для установки данных
							auto & address = awh_cast <sys_t::host_ip_t *> (server->host.get())->ip;
							// Устанавливаем полученный IP-адрес в объект события
							awh_cast <sys_t::address_network_ipv6_t *> (address.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
							// Получаем название сетевого интерфейса по IP-адресу
							string iface = ::move(this->_sys.interfaceName(address));
							// Если название сетевого интерфейса не пустое
							if(!iface.empty())
								// Устанавливаем название сетевого интерфейса
								server->iface = ::move(iface);
							// Параметры сетей интерфейсов
							sys_t::addresses_t addresses(make_unique <sys_t::address_network_ipv6_t> ());
							// Устанавливаем IP-адрес для получения MAC-адреса
							awh_cast <sys_t::address_network_ipv6_t *> (addresses.ip.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
							// Получаем MAC-адрес из системы по IP-адресу
							this->_sys.nodeAddresses(addresses);
							// Получаем объект адреса для установки данных
							server->mac = ::move(addresses.mac);
							// Возвращаем результат работы функции
							return true;
						}
					}
				} break;
				// Для семейства UNIX-доменных сокетов
				case static_cast <uint8_t> (event::family_t::UDS): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем адрес сокета в UNIX-домене
							awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->path.get())->address = host;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем адрес сокета в UNIX-домене
							awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->path.get())->address = host;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем адрес сокета в UNIX-домене
							awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->path.get())->address = host;
							// Возвращаем результат работы функции
							return true;
						}
					}
				} break;
				// Для семейства директорий
				case static_cast <uint8_t> (event::family_t::DIR):
				// Для семейства файловой системы
				case static_cast <uint8_t> (event::family_t::FILE): {
					// Устанавливаем адрес файловой системы события
					awh_cast <sys_t::address_fs_t *> (static_cast <sys_t::filesystem_t *> (i->second.get())->path.get())->address = host;
					// Возвращаем результат работы функции
					return true;
				}
				// Для остальных семейств сокетов
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Host cannot be set for events that are not network or filesystem related", __PRETTY_FUNCTION__, std::make_tuple(id, host), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Host cannot be set for events that are not network or filesystem related", log_t::flag_t::WARNING);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, host), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
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
	// Возвращаем результат работы функции
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
		if(i != ::__awh_nodes__.end()){
			// Устанавливаем тип узла события
			i->second->state.node = node;
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Выполняем создание новой ноды
					unique_ptr <sys_t::peer_t> node = make_unique <sys_t::peer_t> ();
					// Выполняем перенос состояний ноды
					node->state = ::move(i->second->state);
					// Выполняем инициализацию объекта MAC-адреса
					node->mac = make_unique <sys_t::address_mac_t> ();
					// Выполняем перенос хоста ноды
					node->host = ::move(awh_cast <sys_t::client_t *> (i->second.get())->host);
					// Выполняем перенос всей ноды
					i->second = ::move(node);
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Выполняем создание новой ноды
					unique_ptr <sys_t::server_t> node = make_unique <sys_t::server_t> ();
					// Выполняем перенос состояний ноды
					node->state = ::move(i->second->state);
					// Выполняем инициализацию объекта MAC-адреса
					node->mac = make_unique <sys_t::address_mac_t> ();
					// Выполняем перенос хоста ноды
					node->host = ::move(awh_cast <sys_t::client_t *> (i->second.get())->host);
					// Выполняем перенос всей ноды
					i->second = ::move(node);
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
	// Возвращаем результат работы функции
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
		if(i != ::__awh_nodes__.end()){
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
							// Устанавливаем полученный MAC-адрес
							this->_net.mac(awh_cast <sys_t::address_mac_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->mac.get())->address);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
						} break;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный MAC-адрес
							this->_net.mac(awh_cast <sys_t::address_mac_t *> (awh_cast <sys_t::server_t *> (i->second.get())->mac.get())->address);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
						} break;
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
							// Возвращаем адрес сокета в UNIX-домене
							return awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->path.get())->address;
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT):
							// Возвращаем адрес сокета в UNIX-домене
							return awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->path.get())->address;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER):
							// Возвращаем адрес сокета в UNIX-домене
							return awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->path.get())->address;
					}
				} break;
				// Если тип адреса принадлежит к дирректориям файловой системы
				case static_cast <uint8_t> (event::address_t::DIR):
				// Если тип адреса принадлежит к файлам файловой системы
				case static_cast <uint8_t> (event::address_t::FILE):
					// Возвращаем адрес файловой системы события
					return awh_cast <sys_t::address_fs_t *> (static_cast <sys_t::filesystem_t *> (i->second.get())->path.get())->address;
				// Если тип адреса принадлежит к IPv4-адресам
				case static_cast <uint8_t> (event::address_t::IPV4): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v4(awh_cast <sys_t::address_network_ipv4_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.v4(awh_cast <sys_t::address_network_ipv4_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v4(awh_cast <sys_t::address_network_ipv4_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем хост события
							return static_cast <string> (this->_net);
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
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v6(awh_cast <sys_t::address_network_ipv6_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_net);
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.v6(awh_cast <sys_t::address_network_ipv6_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_net);
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.v6(awh_cast <sys_t::address_network_ipv6_t *> (awh_cast <sys_t::host_ip_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->ip.get())->address, net_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_net);
						}
					}
				} break;
				// Если тип адреса принадлежит к сетям
				case static_cast <uint8_t> (event::address_t::NETWORK): {
					// Выполняем поиск предварительных настроек события
					auto i = ::__awh_leadup__.find(id);
					// Если предварительные настройки события найдены
					if(i != ::__awh_leadup__.end()){
						// Если в предварительных настройках указана сеть
						if(!i->second.networks.empty()){
							// Получаем первую сеть из списка
							const auto & j = i->second.networks.begin();
							// Получаем параметры сети
							const sys_t::address_network_t & network = awh_cast <sys_t::address_network_t &> (* j->get());
							/**
							 * Определяем тип адреса сети
							 */
							switch(static_cast <uint8_t> (network.size)){
								// Если тип адреса сети является IPv4
								case 4: {
									// Устанавливаем полученный сетевой адрес
									this->_net.v4(static_cast <const sys_t::address_network_ipv4_t &> (network).address, net_t::endian_t::LITTLE);
									// Возвращаем хост события
									return (static_cast <string> (this->_net) + "/" + std::to_string(static_cast <const sys_t::address_network_ipv4_t &> (network).prefix));
								} break;
								// Если тип адреса сети является IPv6
								case 16: {
									// Устанавливаем полученный сетевой адрес
									this->_net.v6(static_cast <const sys_t::address_network_ipv6_t &> (network).address, net_t::endian_t::LITTLE);
									// Возвращаем результат работы функции
									return (static_cast <string> (this->_net) + "/" + std::to_string(static_cast <const sys_t::address_network_ipv6_t &> (network).prefix));
								} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
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
	// Если значение адреса для установки передано не пустым
	if(!value.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск идентификатора события
			auto i = ::__awh_nodes__.find(id);
			// Если идентификатор события найден
			if(i != ::__awh_nodes__.end()){
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
							this->_log->debug("Address type NONE cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Address type NONE cannot be set", log_t::flag_t::WARNING);
						#endif
					} break;
					// Если тип адреса принадлежит к MAC-адресам
					case static_cast <uint8_t> (event::address_t::MAC): {
						/**
						 * Определяем чем является текущая нода
						 */
						switch(static_cast <uint8_t> (i->second->state.node)){
							// Если нода является соседом
							case static_cast <uint8_t> (event::node_t::PEER): {
								// Устанавливаем полученный MAC-адрес
								this->_net.parse(value, net_t::type_t::MAC);
								// Получаем указатель на объект соседа
								auto peer = awh_cast <sys_t::peer_t *> (i->second.get());
								// Устанавливаем полученный MAC-адрес в объект события
								awh_cast <sys_t::address_mac_t *> (peer->mac.get())->address = ::move(this->_net.mac());
								// Параметры сетей интерфейсов
								sys_t::addresses_t addresses{};
								// Добавляем MAC-адрес для поиска
								awh_cast <sys_t::address_mac_t *> (addresses.mac.get())->address = ::move(this->_net.mac());
								// Получаем MAC-адрес из системы по IP-адресу
								this->_sys.peerAddresses(addresses);
								// Устанавливаем полученный IP-адрес в объект события
								awh_cast <sys_t::host_ip_t *> (peer->host.get())->ip = ::move(addresses.ip);
								// Возвращаем результат работы функции
								return true;
							} break;
							// Если нода является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Устанавливаем полученный MAC-адрес
								this->_net.parse(value, net_t::type_t::MAC);
								// Получаем указатель на объект сервера
								auto server = awh_cast <sys_t::server_t *> (i->second.get());
								// Устанавливаем полученный MAC-адрес в объект события
								awh_cast <sys_t::address_mac_t *> (server->mac.get())->address = ::move(this->_net.mac());
								// Получаем название сетевого интерфейса по MAC-адресу
								string iface = ::move(this->_sys.interfaceName(server->mac));
								// Если название сетевого интерфейса не пустое
								if(!iface.empty())
									// Устанавливаем название сетевого интерфейса
									server->iface = ::move(iface);
								/**
								 * Определяем семейство сокета
								 */
								switch(static_cast <uint8_t> (i->second->state.family)){
									// Для семейства IPv4
									case static_cast <uint8_t> (event::family_t::IPV4):
									// Для семейства UDPv4
									case static_cast <uint8_t> (event::family_t::UDPV4): {
										// Параметры сетей интерфейсов
										sys_t::addresses_t addresses{};
										// Устанавливаем хост сервера для получения MAC-адреса
										awh_cast <sys_t::address_mac_t *> (addresses.mac.get())->address = ::move(this->_net.mac());
										// Получаем IP-адрес из системы по MAC-адресу
										this->_sys.nodeAddresses(addresses);
										// Устанавливаем полученный IP-адрес
										awh_cast <sys_t::host_ip_t *> (server->host.get())->ip = ::move(addresses.ip);
									} break;
									// Для семейства IPv6
									case static_cast <uint8_t> (event::family_t::IPV6):
									// Для семейства UDPv6
									case static_cast <uint8_t> (event::family_t::UDPV6): {
										// Параметры сетей интерфейсов
										sys_t::addresses_t addresses(make_unique <sys_t::address_network_ipv6_t> ());
										// Устанавливаем хост сервера для получения MAC-адреса
										awh_cast <sys_t::address_mac_t *> (addresses.mac.get())->address = ::move(this->_net.mac());
										// Получаем IP-адрес из системы по MAC-адресу
										this->_sys.nodeAddresses(addresses);
										// Устанавливаем полученный IP-адрес
										awh_cast <sys_t::host_ip_t *> (server->host.get())->ip = ::move(addresses.ip);
									} break;
								}
								// Возвращаем результат работы функции
								return true;
							} break;
							// Если нода имеет неподдерживаемый тип
							default: {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("MAC address can only be set for PEER or SERVER nodes", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("MAC address can only be set for PEER or SERVER nodes", log_t::flag_t::WARNING);
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
							case static_cast <uint8_t> (event::node_t::PEER): {
								// Устанавливаем адрес сокета в UNIX-домене
								awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::peer_t *> (i->second.get())->host.get())->path.get())->address = value;
								// Возвращаем результат работы функции
								return true;
							}
							// Если нода является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								// Устанавливаем адрес сокета в UNIX-домене
								awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::client_t *> (i->second.get())->host.get())->path.get())->address = value;
								// Возвращаем результат работы функции
								return true;
							}
							// Если нода является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Устанавливаем адрес сокета в UNIX-домене
								awh_cast <sys_t::address_fs_t *> (awh_cast <sys_t::host_udc_t *> (awh_cast <sys_t::server_t *> (i->second.get())->host.get())->path.get())->address = value;
								// Возвращаем результат работы функции
								return true;
							}
						}
					} break;
					// Если тип адреса принадлежит к дирректориям файловой системы
					case static_cast <uint8_t> (event::address_t::DIR):
					// Если тип адреса принадлежит к файлам файловой системы
					case static_cast <uint8_t> (event::address_t::FILE): {
						// Устанавливаем адрес файловой системы события
						awh_cast <sys_t::address_fs_t *> (static_cast <sys_t::filesystem_t *> (i->second.get())->path.get())->address = value;
						// Возвращаем результат работы функции
						return true;
					} break;
					// Если тип адреса принадлежит к IPv4-адресам
					case static_cast <uint8_t> (event::address_t::IPV4): {
						/**
						 * Определяем чем является текущая нода
						 */
						switch(static_cast <uint8_t> (i->second->state.node)){
							// Если нода является соседом
							case static_cast <uint8_t> (event::node_t::PEER): {
								// Устанавливаем полученный IP-адрес
								this->_net.parse(value, net_t::type_t::IPV4);
								// Получаем указатель на объект соседа
								auto peer = awh_cast <sys_t::peer_t *> (i->second.get());
								// Получаем текущее значение адреса сокета в UNIX-домене
								sys_t::host_ip_t * host = awh_cast <sys_t::host_ip_t *> (peer->host.get());
								// Копируем полученный IP-адрес в объект события
								awh_cast <sys_t::address_network_ipv4_t *> (host->ip.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
								// Параметры сетей интерфейсов
								sys_t::addresses_t addresses{};
								// Устанавливаем хост сервера для получения MAC-адреса
								awh_cast <sys_t::address_network_ipv4_t *> (addresses.ip.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
								// Получаем MAC-адрес из системы по IP-адресу
								this->_sys.peerAddresses(addresses);
								// Получаем объект адреса для установки данных
								peer->mac = ::move(addresses.mac);
								// Возвращаем результат работы функции
								return true;
							}
							// Если нода является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								// Устанавливаем полученный IP-адрес
								this->_net.parse(value, net_t::type_t::IPV4);
								// Получаем указатель на объект клиента
								auto client = awh_cast <sys_t::client_t *> (i->second.get());
								// Получаем текущее значение адреса сокета в UNIX-домене
								sys_t::host_ip_t * host = awh_cast <sys_t::host_ip_t *> (client->host.get());
								// Копируем полученный IP-адрес в объект события
								awh_cast <sys_t::address_network_ipv4_t *> (host->ip.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
								// Получаем название сетевого интерфейса по IP-адресу
								string iface = ::move(this->_sys.interfaceName(host->ip));
								// Если название сетевого интерфейса не пустое
								if(!iface.empty())
									// Устанавливаем название сетевого интерфейса
									client->iface = ::move(iface);
								// Возвращаем результат работы функции
								return true;
							}
							// Если нода является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Устанавливаем полученный IP-адрес
								this->_net.parse(value, net_t::type_t::IPV4);
								// Получаем указатель на объект сервера
								auto server = awh_cast <sys_t::server_t *> (i->second.get());
								// Получаем текущее значение адреса сокета в UNIX-домене
								sys_t::host_ip_t * host = awh_cast <sys_t::host_ip_t *> (server->host.get());
								// Копируем полученный IP-адрес в объект события
								awh_cast <sys_t::address_network_ipv4_t *> (host->ip.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
								// Получаем название сетевого интерфейса по IP-адресу
								string iface = ::move(this->_sys.interfaceName(host->ip));
								// Если название сетевого интерфейса не пустое
								if(!iface.empty())
									// Устанавливаем название сетевого интерфейса
									server->iface = ::move(iface);
								// Параметры сетей интерфейсов
								sys_t::addresses_t addresses{};
								// Устанавливаем хост сервера для получения MAC-адреса
								awh_cast <sys_t::address_network_ipv4_t *> (addresses.ip.get())->address = this->_net.v4(net_t::endian_t::LITTLE);
								// Получаем MAC-адрес из системы по IP-адресу
								this->_sys.nodeAddresses(addresses);
								// Получаем объект адреса для установки данных
								server->mac = ::move(addresses.mac);
								// Возвращаем результат работы функции
								return true;
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
							case static_cast <uint8_t> (event::node_t::PEER): {
								// Устанавливаем полученный IP-адрес
								this->_net.parse(value, net_t::type_t::IPV6);
								// Получаем указатель на объект соседа
								auto peer = awh_cast <sys_t::peer_t *> (i->second.get());
								// Получаем объект адреса для установки данных
								auto & address = awh_cast <sys_t::host_ip_t *> (peer->host.get())->ip;
								// Устанавливаем полученный IP-адрес в объект события
								awh_cast <sys_t::address_network_ipv6_t *> (address.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
								// Параметры сетей интерфейсов
								sys_t::addresses_t addresses(make_unique <sys_t::address_network_ipv6_t> ());
								// Устанавливаем IP-адрес для получения MAC-адреса
								awh_cast <sys_t::address_network_ipv6_t *> (addresses.ip.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
								// Получаем MAC-адрес из системы по IP-адресу
								this->_sys.peerAddresses(addresses);
								// Получаем объект адреса для установки данных
								peer->mac = ::move(addresses.mac);
								// Возвращаем результат работы функции
								return true;
							}
							// Если нода является клиентом
							case static_cast <uint8_t> (event::node_t::CLIENT): {
								// Устанавливаем полученный IP-адрес
								this->_net.parse(value, net_t::type_t::IPV6);
								// Получаем указатель на объект клиента
								auto client = awh_cast <sys_t::client_t *> (i->second.get());
								// Получаем объект адреса для установки данных
								auto & address = awh_cast <sys_t::host_ip_t *> (client->host.get())->ip;
								// Устанавливаем полученный IP-адрес в объект события
								awh_cast <sys_t::address_network_ipv6_t *> (address.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
								// Получаем название сетевого интерфейса по IP-адресу
								string iface = ::move(this->_sys.interfaceName(address));
								// Если название сетевого интерфейса не пустое
								if(!iface.empty())
									// Устанавливаем название сетевого интерфейса
									client->iface = ::move(iface);
								// Возвращаем результат работы функции
								return true;
							}
							// Если нода является сервером
							case static_cast <uint8_t> (event::node_t::SERVER): {
								// Устанавливаем полученный IP-адрес
								this->_net.parse(value, net_t::type_t::IPV6);
								// Получаем указатель на объект сервера
								auto server = awh_cast <sys_t::server_t *> (i->second.get());
								// Получаем объект адреса для установки данных
								auto & address = awh_cast <sys_t::host_ip_t *> (server->host.get())->ip;
								// Устанавливаем полученный IP-адрес в объект события
								awh_cast <sys_t::address_network_ipv6_t *> (address.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
								// Получаем название сетевого интерфейса по IP-адресу
								string iface = ::move(this->_sys.interfaceName(address));
								// Если название сетевого интерфейса не пустое
								if(!iface.empty())
									// Устанавливаем название сетевого интерфейса
									server->iface = ::move(iface);
								// Параметры сетей интерфейсов
								sys_t::addresses_t addresses(make_unique <sys_t::address_network_ipv6_t> ());
								// Устанавливаем IP-адрес для получения MAC-адреса
								awh_cast <sys_t::address_network_ipv6_t *> (addresses.ip.get())->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
								// Получаем MAC-адрес из системы по IP-адресу
								this->_sys.nodeAddresses(addresses);
								// Получаем объект адреса для установки данных
								server->mac = ::move(addresses.mac);
								// Возвращаем результат работы функции
								return true;
							}
						}
					} break;
					// Если тип адреса принадлежит к сетям
					case static_cast <uint8_t> (event::address_t::NETWORK): {
						// IP-адрес сети
						string ip = "";
						// Маска сети
						string mask = "";
						// Тип сети
						net_t::type_t type = net_t::type_t::NONE;
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
						switch(static_cast <uint8_t> (this->_net.host(ip))){
							// Для типа IPv4
							case static_cast <uint8_t> (net_t::type_t::IPV4): {
								// Если маска сети не указана
								if(mask.empty())
									// Устанавливаем маску по умолчанию для IPv4
									mask = "32";
								// Устанавливаем тип сети
								type = net_t::type_t::IPV4;
							} break;
							// Для типа IPv6
							case static_cast <uint8_t> (net_t::type_t::IPV6): {
								// Если маска сети не указана
								if(mask.empty())
									// Устанавливаем маску по умолчанию для IPv6
									mask = "128";
								// Устанавливаем тип сети
								type = net_t::type_t::IPV6;
							} break;
						}
						/**
						 * Определяем какой тип сети необходимо установить
						 */
						switch(static_cast <uint8_t> (type)){
							// Для типа IPv4
							case static_cast <uint8_t> (net_t::type_t::IPV4): {
								// Выполняем парсинг IP-адреса сети
								this->_net.parse(ip, net_t::type_t::IPV4);
								// IP-адрес сети
								unique_ptr <sys_t::address_t> network = make_unique <sys_t::address_network_ipv4_t> ();
								// Получаем адрес сети
								auto addr = awh_cast <sys_t::address_network_ipv4_t *> (network.get());
								// Если маска является префиксом сети
								if(this->_fmk->is(mask, fmk_t::check_t::NUMBER))
									// Устанавливаем префикс сети
									addr->prefix = this->_fmk->atoi <uint8_t> (mask);
								// Если маска является стандартной маской сети
								else
									// Устанавливаем префикс сети
									addr->prefix = this->_net.mask2Prefix(mask, type);
								// Выполняем наложение маски
								this->_net.impose(addr->prefix, net_t::addr_t::NETWORK, net_t::type_t::IPV4);
								// Получаем значение IP-адреса сети
								addr->address = this->_net.v4(net_t::endian_t::LITTLE);
								// Параметры сетей интерфейсов
								sys_t::addresses_t addresses{};
								// Выполняем поиск интерфейса в указанной сети по префиксу
								this->_sys.addresses(network, addresses);
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является соседом
									case static_cast <uint8_t> (event::node_t::PEER): {
										// Получаем объект клиента
										sys_t::peer_t * peer = awh_cast <sys_t::peer_t *> (i->second.get());
										// Получаем объект адреса для установки данных
										peer->mac = ::move(addresses.mac);
										// Копируем полученный IP-адрес в объект события
										awh_cast <sys_t::host_ip_t *> (peer->host.get())->ip = ::move(addresses.ip);
										// Возвращаем результат работы функции
										return true;
									}
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем объект клиента
										auto client = awh_cast <sys_t::client_t *> (i->second.get());
										// Устанавливаем название сетевого интерфейса
										client->iface = ::move(addresses.iface);
										// Копируем полученный IP-адрес в объект события
										awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = ::move(addresses.ip);
									}
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем объект сервера
										auto server = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем объект адреса для установки данных
										server->mac = ::move(addresses.mac);
										// Устанавливаем название сетевого интерфейса
										server->iface = ::move(addresses.iface);
										// Копируем полученный IP-адрес в объект события
										awh_cast<sys_t::host_ip_t *> (server->host.get())->ip = ::move(addresses.ip);
									}
								}
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT):
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Выполняем поиск предварительных настроек события
										auto i = ::__awh_leadup__.find(id);
										// Если предварительные настройки события найдены
										if(i != ::__awh_leadup__.end())
											// Добавляем адрес сети для выхода в интернет
											i->second.networks.emplace(::move(network));
										// Если предварительные настройки события не найдены
										else {
											// Создаём предварительные настройки события
											auto i = ::__awh_leadup__.emplace(id, sys_t::leadup_t{});
											// Добавляем адрес сети для выхода в интернет
											i.first->second.networks.emplace(::move(network));
										}
										// Возвращаем результат работы функции
										return true;
									} break;
								}
							} break;
							// Для типа IPv6
							case static_cast <uint8_t> (net_t::type_t::IPV6): {
								// Выполняем парсинг IP-адреса сети
								this->_net.parse(ip, net_t::type_t::IPV6);
								// IP-адрес сети
								unique_ptr <sys_t::address_t> network = make_unique <sys_t::address_network_ipv6_t> ();
								// Получаем адрес сети
								auto addr = awh_cast <sys_t::address_network_ipv6_t *> (network.get());
								// Если маска является префиксом сети
								if(this->_fmk->is(mask, fmk_t::check_t::NUMBER))
									// Устанавливаем префикс сети
									addr->prefix = this->_fmk->atoi <uint8_t> (mask);
								// Если маска является стандартной маской сети
								else
									// Устанавливаем префикс сети
									addr->prefix = this->_net.mask2Prefix(mask, type);
								// Выполняем наложение маски
								this->_net.impose(addr->prefix, net_t::addr_t::NETWORK, net_t::type_t::IPV6);
								// Устанавливаем значение IP-адреса сети
								addr->address = ::move(this->_net.v6(net_t::endian_t::LITTLE));
								// Параметры сетей интерфейсов
								sys_t::addresses_t addresses(make_unique <sys_t::address_network_ipv6_t> ());
								// Выполняем поиск интерфейса в указанной сети по префиксу
								this->_sys.addresses(network, addresses);
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является соседом
									case static_cast <uint8_t> (event::node_t::PEER): {
										// Получаем объект клиента
										sys_t::peer_t * peer = awh_cast <sys_t::peer_t *> (i->second.get());
										// Получаем объект адреса для установки данных
										peer->mac = ::move(addresses.mac);
										// Копируем полученный IP-адрес в объект события
										awh_cast <sys_t::host_ip_t *> (peer->host.get())->ip = ::move(addresses.ip);
										// Возвращаем результат работы функции
										return true;
									}
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT): {
										// Получаем объект клиента
										auto client = awh_cast <sys_t::client_t *> (i->second.get());
										// Добавляем название сетевого интерфейса
										client->iface = ::move(addresses.iface);
										// Копируем полученный IP-адрес в объект события
										awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = ::move(addresses.ip);
									}
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем объект сервера
										auto server = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем объект адреса для установки данных
										server->mac = ::move(addresses.mac);
										// Устанавливаем название сетевого интерфейса
										server->iface = ::move(addresses.iface);
										// Копируем полученный IP-адрес в объект события
										awh_cast <sys_t::host_ip_t *> (server->host.get())->ip = ::move(addresses.ip);
									}
								}
								/**
								 * Определяем чем является текущая нода
								 */
								switch(static_cast <uint8_t> (i->second->state.node)){
									// Если нода является клиентом
									case static_cast <uint8_t> (event::node_t::CLIENT):
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Выполняем поиск предварительных настроек события
										auto i = ::__awh_leadup__.find(id);
										// Если предварительные настройки события найдены
										if(i != ::__awh_leadup__.end())
											// Добавляем адрес сети для выхода в интернет
											i->second.networks.emplace(::move(network));
										// Если предварительные настройки события не найдены
										else {
											// Создаём предварительные настройки события
											auto i = ::__awh_leadup__.emplace(id, sys_t::leadup_t{});
											// Добавляем адрес сети для выхода в интернет
											i.first->second.networks.emplace(::move(network));
										}
										// Возвращаем результат работы функции
										return true;
									} break;
								}
							} break;
						}
					} break;
					// Для остальных типов адресов
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Unsupported address type cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Unsupported address type cannot be set", log_t::flag_t::WARNING);
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
	}
	// Возвращаем результат работы функции
	return false;
}
/**
 * @brief Метод удаления события
 *
 * @param id идентификатор события
 * @return   результат выполнения удаления
 */
bool awh::IO::destroy(const event::id_t id) noexcept {
	
	return false;
}
/**
 * @brief Метод создания нового события на основе существующего
 *
 * @param id       идентификатор существующего события
 * @param protocol протокол сокета
 * @param mode     режим сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::id_t id, const event::protocol_t protocol, const event::mode_t mode) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end()){
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
								// Устанавливаем флаг режима сокета
								ret.first->second->state.mode = mode;
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
										sys_t::client_t * first = awh_cast <sys_t::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_udc_t> ();
										// Получаем объект хоста UDS-сокета
										sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (second->host.get());
										// Создаем сокет подключения
										host->fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										host->path = make_unique <sys_t::address_fs_t> ();
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
										sys_t::server_t * first = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_udc_t> ();
										// Получаем объект хоста UDS-сокета
										sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (second->host.get());
										// Создаем сокет подключения
										host->fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										host->path = make_unique <sys_t::address_fs_t> ();
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
								// Устанавливаем флаг режима сокета
								ret.first->second->state.mode = mode;
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
										sys_t::client_t * first = awh_cast <sys_t::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_udc_t> ();
										// Получаем объект хоста UDS-сокета
										sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (second->host.get());
										// Создаем сокет подключения
										host->fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										host->path = make_unique <sys_t::address_fs_t> ();
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
										sys_t::server_t * first = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_udc_t> ();
										// Получаем объект хоста UDS-сокета
										sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (second->host.get());
										// Создаем сокет подключения
										host->fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										host->path = make_unique <sys_t::address_fs_t> ();
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
								// Устанавливаем флаг режима сокета
								ret.first->second->state.mode = mode;
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
										sys_t::client_t * first = awh_cast <sys_t::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_udc_t> ();
										// Получаем объект хоста UDS-сокета
										sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (second->host.get());
										// Создаем сокет подключения
										host->fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										host->path = make_unique <sys_t::address_fs_t> ();
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
										sys_t::server_t * first = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_udc_t> ();
										// Получаем объект хоста UDS-сокета
										sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (second->host.get());
										// Создаем сокет подключения
										host->fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										host->path = make_unique <sys_t::address_fs_t> ();
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
									this->_log->debug(
										"An event for a Unix socket cannot be created because it has an invalid initialization type",
										__PRETTY_FUNCTION__, std::make_tuple(
											id, static_cast <uint16_t> (protocol),
											static_cast <uint16_t> (mode)
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
						switch(static_cast <uint8_t> (i->second->state.type)){
							// Для типа сокета RAW
							case static_cast <uint8_t> (event::type_t::RAW): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
								// Устанавливаем флаг режима сокета
								ret.first->second->state.mode = mode;
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
										sys_t::client_t * first = awh_cast <sys_t::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
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
														second->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
													break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
														second->host->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
													second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_RAW, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_RAW, IPPROTO_UDP);
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
													this->_log->debug(
														"RAW socket type only supports UDP protocol or Unix family socket with empty protocol",
														__PRETTY_FUNCTION__, std::make_tuple(
															id, static_cast <uint16_t> (protocol),
															static_cast <uint16_t> (mode)
														), log_t::flag_t::WARNING
													);
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
										sys_t::server_t * first = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
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
														second->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
													break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
														second->host->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
													second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_RAW, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_RAW, IPPROTO_UDP);
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
													this->_log->debug(
														"RAW socket type only supports UDP protocol or Unix family socket with empty protocol",
														__PRETTY_FUNCTION__, std::make_tuple(
															id, static_cast <uint16_t> (protocol),
															static_cast <uint16_t> (mode)
														), log_t::flag_t::WARNING
													);
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
								// Устанавливаем флаг режима сокета
								ret.first->second->state.mode = mode;
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
										sys_t::client_t * first = awh_cast <sys_t::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
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
														second->host->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->host->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
														second->host->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
													second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, IPPROTO_UDP);
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
													this->_log->debug(
														"DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol",
														__PRETTY_FUNCTION__, std::make_tuple(
															id, static_cast <uint16_t> (protocol),
															static_cast <uint16_t> (mode)
														), log_t::flag_t::WARNING
													);
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
										sys_t::server_t * first = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
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
														second->host->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->host->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
														second->host->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
													break;
													// Если установлен другой протокол
													default: ok = false;
												}
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
													second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, IPPROTO_UDP);
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
													this->_log->debug(
														"DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol",
														__PRETTY_FUNCTION__, std::make_tuple(
															id, static_cast <uint16_t> (protocol),
															static_cast <uint16_t> (mode)
														), log_t::flag_t::WARNING
													);
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
									this->_log->debug(
										"An event for a UDP socket cannot be created because it has an invalid initialization type",
										__PRETTY_FUNCTION__, std::make_tuple(
											id, static_cast <uint16_t> (protocol),
											static_cast <uint16_t> (mode)
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
						switch(static_cast <uint8_t> (i->second->state.type)){
							// Для типа сокета STREAM
							case static_cast <uint8_t> (event::type_t::STREAM): {
								// Выполняем создание события
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
								// Устанавливаем флаг режима сокета
								ret.first->second->state.mode = mode;
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
										sys_t::client_t * first = awh_cast <sys_t::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
											} break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, 0);
											break;
											// Если протокол определён как TCP
											case static_cast <uint8_t> (event::protocol_t::TCP):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_TCP);
											break;
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_SCTP);
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
												this->_log->debug(
													"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
													), log_t::flag_t::WARNING
												);
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
										sys_t::server_t * first = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
											} break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, 0);
											break;
											// Если протокол определён как TCP
											case static_cast <uint8_t> (event::protocol_t::TCP):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_TCP);
											break;
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_SCTP);
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
												this->_log->debug(
													"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
													), log_t::flag_t::WARNING
												);
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
								// Устанавливаем флаг режима сокета
								ret.first->second->state.mode = mode;
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
										sys_t::client_t * first = awh_cast <sys_t::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
											}break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.client.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
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
												this->_log->debug(
													"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									} break;
									// Если нода является сервером
									case static_cast <uint8_t> (event::node_t::SERVER): {
										// Получаем текущее значение объекта сервера
										sys_t::server_t * first = awh_cast <sys_t::server_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										sys_t::client_t * second = awh_cast <sys_t::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->host = make_unique <sys_t::host_ip_t> ();
										/**
										 * Определяем тип подключения
										 */
										switch(static_cast <uint8_t> (i->second->state.family)){
											// Для семейства IPv4
											case static_cast <uint8_t> (event::family_t::IPV4): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
											} break;
											// Для семейства IPv6
											case static_cast <uint8_t> (event::family_t::IPV6): {
												// Запоминаем размер структуры
												second->endpoint.size = sizeof(struct sockaddr_in6);
												// Выполняем инициализацию объекта IP-адреса клиента
												awh_cast <sys_t::host_ip_t *> (second->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
											} break;
										}
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->host->fd = ::socket(first->endpoint.server.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
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
												this->_log->debug(
													"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									this->_log->debug(
										"An event for a IP socket cannot be created because it has an invalid initialization type",
										__PRETTY_FUNCTION__, std::make_tuple(
											id, static_cast <uint16_t> (protocol),
											static_cast <uint16_t> (mode)
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::filesystem_t> ());
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = i->second->state.type;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = i->second->state.family;
						// Выполняем инициализацию объекта адреса файловой системы
						awh_cast <sys_t::filesystem_t &> (*ret.first->second).path = make_unique <sys_t::address_fs_t> ();
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для семейства таймеров
					case static_cast <uint8_t> (event::family_t::TIMER):
					// Для семейства интервалов
					case static_cast <uint8_t> (event::family_t::INTERVAL): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::timer_t> ());
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
							this->_log->debug(
								"Event cannot be created because the family it belongs to is not defined",
								__PRETTY_FUNCTION__, std::make_tuple(
									id, static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Event cannot be created because the family it belongs to is not defined", log_t::flag_t::WARNING);
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
					this->_log->debug(
						"Event ID=%u has not yet been initialized",
						__PRETTY_FUNCTION__, std::make_tuple(
							id, static_cast <uint16_t> (protocol),
							static_cast <uint16_t> (mode)
						), log_t::flag_t::WARNING, id
					);
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
				this->_log->debug(
					"Event ID=%u is not exist",
					__PRETTY_FUNCTION__, std::make_tuple(
						id, static_cast <uint16_t> (protocol),
						static_cast <uint16_t> (mode)
					), log_t::flag_t::WARNING, id
				);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (protocol), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @param mode     режим сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept {
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_udc_t> ();
						// Получаем объект хоста UDS-сокета
						sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (client->host.get());
						// Выполняем инициализацию объекта адреса файловой системы
						host->path = make_unique <sys_t::address_fs_t> ();
						// Создаем сокет подключения
						host->fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для типа сокета SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_udc_t> ();
						// Получаем объект хоста UDS-сокета
						sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (client->host.get());
						// Выполняем инициализацию объекта адреса файловой системы
						host->path = make_unique <sys_t::address_fs_t> ();
						// Создаем сокет подключения
						host->fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_udc_t> ();
						// Получаем объект хоста UDS-сокета
						sys_t::host_udc_t * host = awh_cast <sys_t::host_udc_t *> (client->host.get());
						// Выполняем инициализацию объекта адреса файловой системы
						host->path = make_unique <sys_t::address_fs_t> ();
						// Создаем сокет подключения
						host->fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
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
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_ip_t> ();
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
										client->host->fd = ::socket(AF_INET, SOCK_RAW, 0);
									break;
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::RAW):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
									break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
										client->host->fd = ::socket(AF_INET6, SOCK_RAW, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_UDP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_ip_t> ();
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
										client->host->fd = ::socket(AF_INET, SOCK_DGRAM, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
									break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
										client->host->fd = ::socket(AF_INET6, SOCK_DGRAM, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_ip_t> ();
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
										client->host->fd = ::socket(AF_INET, SOCK_STREAM, 0);
									break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
									break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
										client->host->fd = ::socket(AF_INET6, SOCK_STREAM, 0);
									break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
									break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->host->fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
						// Устанавливаем флаг семейства сокета
						ret.first->second->state.family = family;
						// Устанавливаем флаг протокола сокета
						ret.first->second->state.protocol = protocol;
						// Получаем объект клиента
						sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_ip_t> ();
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
										client->host->fd = ::socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
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
										client->host->fd = ::socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
									break;
									// Если установлен другой протокол
									default: ok = false;
								}
								// Выполняем инициализацию объекта IP-адреса клиента
								awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
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
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::filesystem_t> ());
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг режима сокета
				ret.first->second->state.mode = mode;
				// Устанавливаем флаг семейства сокета
				ret.first->second->state.family = family;
				// Устанавливаем флаг протокола сокета
				ret.first->second->state.protocol = protocol;
				// Выполняем инициализацию объекта адреса файловой системы
				awh_cast <sys_t::filesystem_t &> (* ret.first->second).path = make_unique <sys_t::address_fs_t> ();
				// Возвращаем идентификатор созданного события
				result = ret.first->first;
			} break;
			// Для семейства таймеров
			case static_cast <uint8_t> (event::family_t::TIMER):
			// Для семейства интервалов
			case static_cast <uint8_t> (event::family_t::INTERVAL): {
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::timer_t> ());
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг режима сокета
				ret.first->second->state.mode = mode;
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
						"Event cannot be created because the family it belongs to is not defined",
						__PRETTY_FUNCTION__, std::make_tuple(
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (type),
							static_cast <uint16_t> (protocol),
							static_cast <uint16_t> (mode)
						), log_t::flag_t::WARNING
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Event cannot be created because the family it belongs to is not defined", log_t::flag_t::WARNING);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @param mode     режим сокета
 * @return         пара идентификаторов созданных событий
 */
std::array <awh::event::id_t, 2> awh::IO::events(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept {
	// Результат работы функции
	std::array <awh::event::id_t, 2> result = {0,0};
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Список сокетов для инициализации
		int32_t fds[2] = {-1,-1};
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
								static_cast <uint16_t> (protocol),
								static_cast <uint16_t> (mode)
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
												static_cast <uint16_t> (protocol),
												static_cast <uint16_t> (mode)
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
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
													static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
													static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
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
		if((fds[0] != -1) && (fds[1] != -1)){
			// Переходим по всему списку идентификаторов событий
			for(uint8_t i = 0; i < 2; i++){
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <sys_t::client_t> ());
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг режима сокета
				ret.first->second->state.mode = mode;
				// Устанавливаем флаг семейства сокета
				ret.first->second->state.family = family;
				// Устанавливаем флаг протокола сокета
				ret.first->second->state.protocol = protocol;
				// Получаем объект клиента
				sys_t::client_t * client = awh_cast <sys_t::client_t *> (ret.first->second.get());
				/**
				 * Определяем семейство сокета
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства UNIX-доменных сокетов
					case static_cast <uint8_t> (event::family_t::UDS): {
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_udc_t> ();
						// Создаем сокет подключения
						client->host->fd = fds[i];
						// Выполняем инициализацию объекта адреса файловой системы
						awh_cast <sys_t::host_udc_t *> (client->host.get())->path = make_unique <sys_t::address_fs_t> ();
					} break;
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
					// Для семейства UDPv4
					case static_cast <uint8_t> (event::family_t::UDPV4): {
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_ip_t> ();
						// Создаем сокет подключения
						client->host->fd = fds[i];
						// Выполняем инициализацию объекта IP-адреса клиента
						awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv4_t> ();
					} break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
					// Для семейства UDPv6
					case static_cast <uint8_t> (event::family_t::UDPV6): {
						// Выполняем инициализацию объекта хоста клиента
						client->host = make_unique <sys_t::host_ip_t> ();
						// Создаем сокет подключения
						client->host->fd = fds[i];
						// Выполняем инициализацию объекта IP-адреса клиента
						awh_cast <sys_t::host_ip_t *> (client->host.get())->ip = make_unique <sys_t::address_network_ipv6_t> ();
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
awh::event::notify_t awh::IO::action(const event::id_t id, const event::action_t action) noexcept {

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

	return false;
}
/**
 * @brief Метод установки флага только IPv6 для события
 *
 * @param id     идентификатор события
 * @param enable флаг только IPv6
 * @return       результат выполнения установки
 */
bool awh::IO::onlyIPv6(const event::id_t id, const bool enable) noexcept {

	return false;
}
/**
 * @brief Метод установки опции события
 *
 * @param id     идентификатор события
 * @param option опция события
 * @param value  значение опции события
 * @return       результат выполнения установки
 */
bool awh::IO::option(const event::id_t id, const event::option_t option, const int32_t value) noexcept {
	
	return false;
}
/**
 * @brief Метод отключения события
 *
 * @param id идентификатор события
 * @return   результат выполнения отключения
 */
bool awh::IO::disconnect(const event::id_t id) noexcept {

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

	return false;
}
/**
 * @brief Метод принятия входящего соединения события
 *
 * @param id    идентификатор события
 * @param max   максимальное количество входящих соединений
 * @param async флаг асинхронного принятия соединения
 * @return      результат выполнения принятия соединения
 */
bool awh::IO::accept(const event::id_t id, const uint16_t max, const bool async) noexcept {

	return false;
}
/**
 * @brief Метод отправки события
 *
 * @param value значение события для отправки
 * @return      результат выполнения отправки
 */
bool awh::IO::post(const uint32_t value) noexcept {
	
	return false;
}
/**
 * @brief Метод отправки данных события
 *
 * @param id   идентификатор события
 * @param data указатель на данные для отправки
 * @param size размер данных для отправки
 * @return     результат выполнения отправки
 */
bool awh::IO::send(const event::id_t id, const char * data, const size_t size) noexcept {

	return false;
}
/**
 * @brief Метод очистки всех адресов сетей для выхода в интернет
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearNetworks(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод получения списка адресов сетей для выхода в интернет
 *
 * @param id идентификатор события
 * @return   список адресов сетей события
 */
std::unordered_set <string> awh::IO::networks(const event::id_t id) const noexcept {
	
	return {};
}
/**
 * @brief Метод добавления адреса сети для выхода в интернет
 *
 * @param id      идентификатор события
 * @param network адрес сети для добавления
 * @return        результат выполнения добавления
 */
bool awh::IO::addNetwork(const event::id_t id, const string & network) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления адреса сети для выхода в интернет
 *
 * @param id      идентификатор события
 * @param network адрес сети для удаления
 * @return        результат выполнения удаления
 */
bool awh::IO::removeNetwork(const event::id_t id, const string & network) noexcept {
	
	return false;
}
/**
 * @brief Метод добавления списка адресов сетей для выхода в интернет
 *
 * @param id       идентификатор события
 * @param networks список адресов сетей для добавления
 * @return         результат выполнения добавления
 */
bool awh::IO::addNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления списка адресов сетей для выхода в интернет
 *
 * @param id       идентификатор события
 * @param networks список адресов сетей для удаления
 * @return         результат выполнения удаления
 */
bool awh::IO::removeNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept {

	return false;
}
/**
 * @brief Метод получения сетевого интерфейса события
 *
 * @param id идентификатор события
 * @return   сетевой интерфейс события
 */
string awh::IO::networkInterface(const event::id_t id) const noexcept {

	return "";
}
/**
 * @brief Метод установки сетевого интерфейса события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::IO::setNetworkInterface(const event::id_t id, const string & name) noexcept {

	return false;
}
/**
 * @brief Метод очистки всех сетевых интерфейсов события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearNetworkInterfaces(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод получения списка сетевых интерфейсов события
 *
 * @param id идентификатор события
 * @return   список сетевых интерфейсов события
 */
std::unordered_set <string> awh::IO::networkInterfaces(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод добавления сетевого интерфейса для события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для добавления
 * @return     результат выполнения добавления
 */
bool awh::IO::addNetworkInterface(const event::id_t id, const string & name) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления сетевого интерфейса для события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для удаления
 * @return     результат выполнения удаления
 */
bool awh::IO::removeNetworkInterface(const event::id_t id, const string & name) noexcept {
	
	return false;
}
/**
 * @brief Метод добавления списка сетевых интерфейсов для события
 *
 * @param id    идентификатор события
 * @param names список сетевых интерфейсов для добавления
 * @return      результат выполнения добавления
 */
bool awh::IO::addNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления списка сетевых интерфейсов для события
 *
 * @param id    идентификатор события
 * @param names список сетевых интерфейсов для удаления
 * @return      результат выполнения удаления
 */
bool awh::IO::removeNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept {
	
	return false;
}
/**
 * @brief Метод присоединения события к мультикаст группе
 *
 * @param id               идентификатор события
 * @param multicastAddress адрес мультикаст группы для присоединения
 * @return                 результат выполнения присоединения
 */
bool awh::IO::multicastJoin(const event::id_t id, const string & multicastAddress) noexcept {

	return false;
}
/**
 * @brief Метод выхода события из мультикаст группы
 *
 * @param id               идентификатор события
 * @param multicastAddress адрес мультикаст группы для выхода
 * @return                 результат выполнения выхода
 */
bool awh::IO::multicastLeave(const event::id_t id, const string & multicastAddress) noexcept {

	return false;
}
/**
 * @brief Метод очистки чёрного списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearBlacklist(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод добавления адреса в чёрный список события
 *
 * @param id      идентификатор события
 * @param address адрес для добавления в чёрный список
 * @return        результат выполнения добавления
 */
bool awh::IO::addToBlacklist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод удаления адреса из чёрного списка события
 *
 * @param id      идентификатор события
 * @param address адрес для удаления из чёрного списка
 * @return        результат выполнения удаления
 */
bool awh::IO::removeFromBlacklist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод получения чёрного списка события
 *
 * @param id идентификатор события
 * @return   чёрный список события
 */
std::unordered_map <awh::event::address_t, string> awh::IO::blacklist(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод очистки белого списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearWhitelist(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод добавления адреса в белый список события
 * @param id      идентификатор события
 * @param address адрес для добавления в белый список
 * @return        результат выполнения добавления
 */
bool awh::IO::addToWhitelist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод удаления адреса из белого списка события
 *
 * @param id      идентификатор события
 * @param address адрес для удаления из белого списка
 * @return        результат выполнения удаления
 */
bool awh::IO::removeFromWhitelist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод получения белого списка события
 *
 * @param id идентификатор события
 * @return   белый список события
 */
std::unordered_map <awh::event::address_t, string> awh::IO::whitelist(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод установки таймаута на чтение события
 *
 * @param id      идентификатор события
 * @param timeout значение таймаута в миллисекундах
 */
void awh::IO::readTimeout(const event::id_t id, const uint16_t timeout) noexcept {

	
}
/**
 * @brief Метод установки таймаута на запись события
 *
 * @param id      идентификатор события
 * @param timeout значение таймаута в миллисекундах
 */
void awh::IO::writeTimeout(const event::id_t id, const uint16_t timeout) noexcept {

}
/**
 * @brief Метод установки глубины очереди принятия входящих соединений события
 *
 * @param id       идентификатор события
 * @param depth    глубина очереди принятия входящих соединений
 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
 */
void awh::IO::backlog(const event::id_t id, const uint16_t depth, const bool adaptive) noexcept {

}
/**
 * @brief Метод получения размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия с буфером
 * @return       размер буфера события
 */
size_t awh::IO::bufferSize(const event::id_t id, const event::action_t action) noexcept {

	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия с буфером
 * @param size   размер буфера события
 * @return       результат выполнения установки
 */
bool awh::IO::bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept {

	return false;
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

	return false;
}
/**
 * @brief Метод приостановки события
 *
 * @param id идентификатор события
 * @return   результат выполнения приостановки
 */
bool awh::IO::pause(const event::id_t id) noexcept {
	
	return false;
}
/**
 * @brief Метод возобновления события
 *
 * @param id идентификатор события
 * @return   результат выполнения возобновления
 */
bool awh::IO::resume(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод проверки состояния события
 *
 * @param id идентификатор события
 * @return   состояние события
 */
bool awh::IO::isAlive(const event::id_t id) const noexcept {

	return false;
}
/**
 * @brief Метод инициализации основного движка фреймворка
 *
 * @return результат выполнения инициализации
 */
bool awh::IO::initialize() noexcept {


	return false;
}
/**
 * @brief Метод деинициализации основного движка фреймворка
 *
 * @return результат выполнения деинициализации
 */
bool awh::IO::deinitialize() noexcept {

	return false;
}
/**
 * @brief Метод проверки состояния инициализации основного движка фреймворка
 *
 * @return состояние инициализации
 */
bool awh::IO::isInitialized() const noexcept {

	return false;
}
/**
 * @brief Метод получения режима события
 *
 * @param id идентификатор события
 * @return   режим события
 */
awh::event::mode_t awh::IO::mode(const event::id_t id) noexcept {
	
	return event::mode_t::NONE;
}
/**
 * @brief Метод получения типа события
 *
 * @param id идентификатор события
 * @return   тип события
 */
awh::event::type_t awh::IO::type(const event::id_t id) noexcept {

	return event::type_t::NONE;
}
/**
 * @brief Метод получения семейства события
 *
 * @param id идентификатор события
 * @return   семейство события
 */
awh::event::family_t awh::IO::family(const event::id_t id) noexcept {

	return event::family_t::NONE;
}
/**
 * @brief Метод получения статуса события
 *
 * @param id идентификатор события
 * @return   статус события
 */
awh::event::status_t awh::IO::status(const event::id_t id) noexcept {

	return event::status_t::NONE;
}
/**
 * @brief Методы установки функции обратного вызова на чтение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::read_t & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на запись события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::write_t & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на ошибку события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::error_t & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на изменение статуса события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::status_t & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на принятие события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::accept_t & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на подключение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::connect_t & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на получение пользовательского события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const event::callback::user_t & cb) noexcept {

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
