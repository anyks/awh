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
#include <net/io2.hpp>

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
							awh_cast <net::attr_net_t *> (awh_cast <net::peer_t *> (i->second.get())->remote.get())->port = port;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Получаем объект хоста клиента
							awh_cast <net::attr_net_t *> (awh_cast <net::client_t *> (i->second.get())->target.get())->port = port;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем порт события
							awh_cast <net::attr_net_t *> (awh_cast <net::server_t *> (i->second.get())->host.get())->port = port;
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
		if(i != ::__awh_nodes__.end()){
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
							// Выполняем извлечение
							this->_eth.fillsource(host->ip, source);
							// Возвращаем название сетевого интерфейса
							return source.iface;
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t source(::make_unique <net::addr_net_ipv6_t> ());
							// Выполняем получение нужного нам атрибута подключения
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
							// Выполняем извлечение
							this->_eth.fillsource(host->ip, source);
							// Возвращаем название сетевого интерфейса
							return source.iface;
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
		if(i != ::__awh_nodes__.end()){
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
								// Копируем IP-адрес в источник сетевого адреса
								awh_cast <net::addr_net_ipv4_t *> (client->source.get())->address = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
							// Если IP-адрес не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" not found", log_t::flag_t::WARNING, source.iface.c_str());
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
								// Копируем IP-адрес в источник сетевого адреса
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (client->source.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], 16);
							// Если IP-адрес не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" not found", log_t::flag_t::WARNING, source.iface.c_str());
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
							if((result = (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0)))
								// Копируем IP-адрес в хост сервера
								awh_cast <net::addr_net_ipv4_t *> (server->host.get())->address = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
							// Если IP-адрес не получен
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" not found", log_t::flag_t::WARNING, source.iface.c_str());
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
							if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0)))
								// Копируем IP-адрес в хост сервера
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (server->host.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], 16);
							// Если IP-адрес не получен
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Network interface \"%s\" not found", __PRETTY_FUNCTION__, std::make_tuple(id, name), log_t::flag_t::WARNING, source.iface.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Network interface \"%s\" not found", log_t::flag_t::WARNING, source.iface.c_str());
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
		if(i != ::__awh_nodes__.end()){
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Получаем текущее значение объекта файловой системы
					net::fs_t * fs = awh_cast <net::fs_t *> (i->second.get());
					// Возвращаем адрес файловой системы
					return awh_cast <net::addr_fs_t *> (fs->path.get())->address;
				}
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
							// Устанавливаем полученный IP-адрес
							this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (remote->ip.get())->address, net_addr_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_addr);
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Получаем объект хоста соседа
							net::attr_net_t * remote = awh_cast <net::attr_net_t *> (peer->remote.get());
							// Устанавливаем полученный IP-адрес
							this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (remote->ip.get())->address, net_addr_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_addr);
						}
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
							// Устанавливаем полученный IP-адрес
							this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (target->ip.get())->address, net_addr_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_addr);
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Получаем объект адреса целевой машины
							net::attr_net_t * target = awh_cast <net::attr_net_t *> (client->target.get());
							// Устанавливаем полученный IP-адрес
							this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (target->ip.get())->address, net_addr_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_addr);
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
							// Устанавливаем полученный IP-адрес
							this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (host->ip.get())->address, net_addr_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_addr);
						}
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							// Получаем объект адреса хоста сервера
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (server->host.get());
							// Устанавливаем полученный IP-адрес
							this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (host->ip.get())->address, net_addr_t::endian_t::LITTLE);
							// Возвращаем результат работы функции
							return static_cast <string> (this->_addr);
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
		if(i != ::__awh_nodes__.end()){
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
							// Устанавливаем адрес файловой системы
							awh_cast <net::addr_fs_t *> (awh_cast <net::fs_t *> (i->second.get())->path.get())->address = target;
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
									// Устанавливаем полученный IP-адрес
									::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (peer->remote.get())->ip.get())->address[0], &this->_addr.v6(net_addr_t::endian_t::LITTLE)[0], 16);
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
									// Устанавливаем полученный IP-адрес
									::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (client->target.get())->ip.get())->address[0], &this->_addr.v6(net_addr_t::endian_t::LITTLE)[0], 16);
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
									// Устанавливаем полученный IP-адрес
									::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (server->host.get())->ip.get())->address[0], &this->_addr.v6(net_addr_t::endian_t::LITTLE)[0], 16);
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
		if(i != ::__awh_nodes__.end()){
			// Устанавливаем тип узла события
			i->second->state.node = node;
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является пользовательским событием
				case static_cast <uint8_t> (event::node_t::USER): {
					// Выполняем создание новой ноды
					unique_ptr <net::user_t> node = make_unique <net::user_t> ();
					// Выполняем перенос всей ноды
					i->second = ::move(node);
				} break;
				// Если нода является таймером
				case static_cast <uint8_t> (event::node_t::TIMER): {
					// Выполняем создание новой ноды
					unique_ptr <net::timer_t> node = make_unique <net::timer_t> ();
					// Выполняем перенос всей ноды
					i->second = ::move(node);
				} break;
				// Если нода является файловой системой
				case static_cast <uint8_t> (event::node_t::FSYS): {
					// Выполняем создание новой ноды
					unique_ptr <net::fs_t> node = make_unique <net::fs_t> (this->_fmk, this->_log);
					// Выполняем перенос состояний ноды
					node->state = ::move(i->second->state);
					// Выполняем перенос всей ноды
					i->second = ::move(node);
				} break;
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER): {
					// Выполняем создание новой ноды
					unique_ptr <net::peer_t> node = make_unique <net::peer_t> (this->_fmk, this->_log);
					// Выполняем перенос состояний ноды
					node->state = ::move(i->second->state);
					// Выполняем инициализацию объекта MAC-адреса
					node->mac = make_unique <net::addr_mac_t> ();
					// Выполняем перенос хоста ноды
					node->remote = ::move(awh_cast <net::client_t *> (i->second.get())->target);
					// Выполняем перенос всей ноды
					i->second = ::move(node);
				} break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Выполняем создание новой ноды
					unique_ptr <net::server_t> node = make_unique <net::server_t> ();
					// Выполняем перенос состояний ноды
					node->state = ::move(i->second->state);
					// Выполняем перенос хоста ноды
					node->host = ::move(awh_cast <net::client_t *> (i->second.get())->target);
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

	// Выводим результат по умолчанию
	return false;
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
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
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Создаем сокет подключения
										target->fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
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
										// Создаем сокет подключения
										target->fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
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
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Создаем сокет подключения
										target->fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
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
										// Создаем сокет подключения
										target->fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
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
										net::client_t * first = awh_cast <net::client_t *> (i->second.get());
										// Получаем текущее значение соседа получающего параметры
										net::client_t * second = awh_cast <net::client_t *> (ret.first->second.get());
										// Выполняем инициализацию объекта хоста клиента
										second->target = make_unique <net::attr_uds_t> ();
										// Получаем объект хоста UDS-сокета
										net::attr_uds_t * target = awh_cast <net::attr_uds_t *> (second->target.get());
										// Создаем сокет подключения
										target->fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
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
										// Создаем сокет подключения
										target->fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
										// Выполняем инициализацию объекта адреса файловой системы
										target->path = make_unique <net::addr_fs_t> ();
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
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
														second->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
													break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
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
														second->target->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
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
													second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_RAW, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_RAW, IPPROTO_UDP);
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
														second->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
													break;
													// Если протокол определён как ICMP
													case static_cast <uint8_t> (event::protocol_t::ICMP):
														// Создаем сокет подключения
														second->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
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
														second->target->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
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
													second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_RAW, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_RAW, IPPROTO_UDP);
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
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
														second->target->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->target->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
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
														second->target->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
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
													second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, IPPROTO_UDP);
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
														second->target->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
													break;
													// Если протокол определён как IGMP
													case static_cast <uint8_t> (event::protocol_t::IGMP):
														// Создаем сокет подключения
														second->target->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
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
														second->target->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
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
													second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, 0);
												break;
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP):
													// Создаем сокет подключения
													second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, IPPROTO_UDP);
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
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
												second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, 0);
											break;
											// Если протокол определён как TCP
											case static_cast <uint8_t> (event::protocol_t::TCP):
												// Создаем сокет подключения
												second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_TCP);
											break;
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_SCTP);
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
												second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, 0);
											break;
											// Если протокол определён как TCP
											case static_cast <uint8_t> (event::protocol_t::TCP):
												// Создаем сокет подключения
												second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_TCP);
											break;
											// Если протокол определён как SCTP
											case static_cast <uint8_t> (event::protocol_t::SCTP):
												// Создаем сокет подключения
												second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_SCTP);
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
								auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
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
												second->target->fd = ::socket(first->endpoint.client.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
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
												second->target->fd = ::socket(first->endpoint.server.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::fs_t> (this->_fmk, this->_log));
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
						target->fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для типа сокета SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
						target->fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					} break;
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						// Выполняем создание события
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
						target->fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
										client->target->fd = ::socket(AF_INET, SOCK_RAW, 0);
									break;
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::RAW):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
									break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
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
										client->target->fd = ::socket(AF_INET6, SOCK_RAW, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_UDP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
										client->target->fd = ::socket(AF_INET, SOCK_DGRAM, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
									break;
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
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
										client->target->fd = ::socket(AF_INET6, SOCK_DGRAM, 0);
									break;
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
									break;
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
										client->target->fd = ::socket(AF_INET, SOCK_STREAM, 0);
									break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
									break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
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
										client->target->fd = ::socket(AF_INET6, SOCK_STREAM, 0);
									break;
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
									break;
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Создаем сокет подключения
										client->target->fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
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
						auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
						// Устанавливаем флаг типа сокета
						ret.first->second->state.type = type;
						// Устанавливаем флаг режима сокета
						ret.first->second->state.mode = mode;
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
										client->target->fd = ::socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
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
										client->target->fd = ::socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
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
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::fs_t> (this->_fmk, this->_log));
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг режима сокета
				ret.first->second->state.mode = mode;
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
		if((fds[0] != net::invalid_socket_t) && (fds[1] != net::invalid_socket_t)){
			// Переходим по всему списку идентификаторов событий
			for(uint8_t i = 0; i < 2; i++){
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <net::client_t> (this->_fmk, this->_log));
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг режима сокета
				ret.first->second->state.mode = mode;
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
						client->target->fd = fds[i];
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
						client->target->fd = fds[i];
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
						client->target->fd = fds[i];
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
		if(i != ::__awh_nodes__.end()){
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
					fd = awh_cast <net::peer_t *> (i->second.get())->remote->fd;
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::client_t *> (i->second.get())->target->fd;
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::server_t *> (i->second.get())->host->fd;
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
		if(i != ::__awh_nodes__.end()){
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
					fd = awh_cast <net::peer_t *> (i->second.get())->remote->fd;
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::client_t *> (i->second.get())->target->fd;
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::server_t *> (i->second.get())->host->fd;
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
 * @param data указатель на данные для отправки
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
		if(i != ::__awh_nodes__.end()){
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
			if(i != ::__awh_nodes__.end()){
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
									this->_log->debug("Address being added to the blacklist does not match the file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Address being added to the blacklist does not match the file system address", log_t::flag_t::WARNING);
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
											this->_log->debug("Address being added to the blacklist does not match the file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to the blacklist does not match the file system address", log_t::flag_t::WARNING);
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
											this->_log->debug("Address being added to the blacklist does not match the MAC-address or IP-address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to the blacklist does not match the MAC-address or IP-address", log_t::flag_t::WARNING);
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
			if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
			if(i != ::__awh_nodes__.end()){
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
									this->_log->debug("Address being added to the whitelist does not match the file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Address being added to the whitelist does not match the file system address", log_t::flag_t::WARNING);
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
											this->_log->debug("Address being added to the whitelist does not match the file system address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to the whitelist does not match the file system address", log_t::flag_t::WARNING);
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
											this->_log->debug("Address being added to the whitelist does not match the MAC-address or IP-address", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
										/**
										* Если режим отладки не включён
										*/
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Address being added to the whitelist does not match the MAC-address or IP-address", log_t::flag_t::WARNING);
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
			if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
 * @param action тип действия с буфером
 * @return       размер буфера события
 */
size_t awh::IO::bufferSize(const event::id_t id, const event::action_t action) const noexcept {

	// Выводим результат по умолчанию
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

	// Выводим результат по умолчанию
	return false;
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
			/**
			 * Файловый дескриптор события
			 */
			net::socket_t fd = net::invalid_socket_t;
			/**
			 * Определяем чем является текущая нода
			 */
			switch(static_cast <uint8_t> (i->second->state.node)){
				// Если нода является соседом
				case static_cast <uint8_t> (event::node_t::PEER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::peer_t *> (i->second.get())->remote->fd;
				break;
				// Если нода является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::client_t *> (i->second.get())->target->fd;
				break;
				// Если нода является сервером
				case static_cast <uint8_t> (event::node_t::SERVER):
					// Получаем файловый дескриптор события
					fd = awh_cast <net::server_t *> (i->second.get())->host->fd;
				break;
				// Для других типов нод
				default: return false;
			}
			// Если файловый дескриптор события получен успешно
			if((result = (fd != net::invalid_socket_t))){
				// Устанавливаем параметры keep-alive для сокета события
				if((result = this->_eth.keepalive(fd, cnt, idle, intvl)))
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
 * @brief Метод получения режима события
 *
 * @param id идентификатор события
 * @return   режим события
 */
awh::event::mode_t awh::IO::mode(const event::id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end())
			// Возвращаем режим события
			return i->second->state.mode;
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
	return event::mode_t::NONE;
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
		if(i != ::__awh_nodes__.end()){
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
