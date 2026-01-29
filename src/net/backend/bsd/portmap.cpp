/**
 * @file: portmap.cpp
 * @date: 2026-01-28
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
 * Стандартные модули
 */
#include <random>
#include <cerrno>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <iostream>

/**
 * Подключаем системные заголовки
 */
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

/**
 * Подключаем заголовочные файлы MiniUPnP
 */
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/eth/portmap.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения списка проброшенных портов на маршрутизаторе
 *
 * @return список параметров проброшенных портов на маршрутизаторе
 */
vector <awh::PortMapping::fwd_t> awh::PortMapping::mappings() const noexcept {
	// Результат работы функции
	vector <fwd_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Ищем устройства UPnP в локальной сети (3 секунды таймаут)
		UPNPDev * devlist = ::upnpDiscover(3000, nullptr, nullptr, 0, 0, 2, nullptr);
		// Если устройства не найдены
		if(devlist == nullptr)
			// Выводим пустой результат
			return result;
		// Действующий шлюз IGD
		UPNPUrls urls = {0};
		// Структура данных IGD
		IGDdatas data = {0};
		// Буфер для хранения внутреннего IP-адреса
		vector <char> internalAddr(64, 0);
		// Получаем действующий шлюз IGD
		int32_t status = ::UPNP_GetValidIGD(devlist, &urls, &data, &internalAddr[0], internalAddr.size(), nullptr, 0);
		// Освобождаем память списка устройств UPnP
		::freeUPNPDevlist(devlist);
		// Если не удалось получить действующий шлюз IGD
		if(status != 1){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, ::strupnperror(status));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, ::strupnperror(status));
			#endif
			// Освобождаем память URL-ов UPnP
			::FreeUPNPUrls(&urls);
			// Выводим пустой результат
			return result;
		}
		// Индекс перебора записей проброса портов
		size_t index = 0;
		/**
		 * Перебираем все записи проброса портов
		 */
		for(;;){
			// Буфер для хранения статуса включения/выключения порта
			char enabled[16] = {0};
			// Буфер для хранения протокола порта
			char protocol[16] = {0};
			// Буфер для хранения продолжительности аренды порта
			char duration[16] = {0};
			// Буфер для хранения внутреннего порта
			char internalPort[16] = {0};
			// Буфер для хранения внешнего порта
			char externalPort[16] = {0};
			// Буфер для хранения описания проброса порта
			char description[128] = {0};
			// Буфер для хранения внутреннего IP-адреса
			char internalAddress[64] = {0};
			// Буфер для хранения внешнего IP-адреса
			char externalAddress[64] = {0};
			// Получаем запись проброса порта по индексу
			status = ::UPNP_GetGenericPortMappingEntry(
				urls.controlURL, data.first.servicetype, std::to_string(index++).c_str(),
				externalPort, internalAddress, internalPort, protocol, description, enabled, externalAddress, duration
			);
			// Если записи с таким индексом нет
			if(status != 0)
				// Выходим из цикла перебора записей проброса портов
				break;
			// Добавляем новую запись проброса порта в результирующий список
			result.push_back(fwd_t());
			// Выполняем парсинг внутреннего IP-адреса
			if(this->_addr.parse(internalAddress)){
				/**
				 * В зависимости от типа IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если тип IP-адреса IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Устанавливаем внутренний IP-адрес в результирующую запись
						result.back().internalAddress = make_unique <net::addr_net_ipv4_t> ();
						// Присваиваем внутренний IP-адрес в результирующую запись
						awh_cast <net::addr_net_ipv4_t *> (result.back().internalAddress.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
					} break;
					// Если тип IP-адреса IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Устанавливаем внутренний IP-адрес в результирующую запись
						result.back().internalAddress = make_unique <net::addr_net_ipv6_t> ();
						// Присваиваем внутренний IP-адрес в результирующую запись
						awh_cast <net::addr_net_ipv6_t *> (result.back().internalAddress.get())->address = ::move(this->_addr.v6(net_addr_t::endian_t::LITTLE));
					} break;
					// Для остальных типов IP-адресов
					default: result.back().internalAddress = make_unique <net::addr_t> ();
				}
			// Если не удалось выполнить парсинг внутреннего IP-адреса
			} else result.back().internalAddress = make_unique <net::addr_t> ();
			// Выполняем парсинг внешнего IP-адреса
			if(this->_addr.parse(externalAddress)){
				/**
				 * В зависимости от типа IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если тип IP-адреса IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Устанавливаем внешний IP-адрес в результирующую запись
						result.back().externalAddress = make_unique <net::addr_net_ipv4_t> ();
						// Присваиваем внешний IP-адрес в результирующую запись
						awh_cast <net::addr_net_ipv4_t *> (result.back().externalAddress.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
					} break;
					// Если тип IP-адреса IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Устанавливаем внешний IP-адрес в результирующую запись
						result.back().externalAddress = make_unique <net::addr_net_ipv6_t> ();
						// Присваиваем внешний IP-адрес в результирующую запись
						awh_cast <net::addr_net_ipv6_t *> (result.back().externalAddress.get())->address = ::move(this->_addr.v6(net_addr_t::endian_t::LITTLE));
					} break;
					// Для остальных типов IP-адресов
					default: result.back().externalAddress = make_unique <net::addr_t> ();
				}
			// Если не удалось выполнить парсинг внешнего IP-адреса
			} else result.back().externalAddress = make_unique <net::addr_t> ();
			// Устанавливаем тип проброшенного порта в результирующую запись
			result.back().type = type_t::UPNP;
			// В зависимости от протокола проброшенного порта
			if(this->_fmk->compare("tcp", protocol))
				// Устанавливаем протокол проброшенного порта TCP в результирующую запись
				result.back().proto = proto_t::TCP;
			// Устанавливаем протокол проброшенного порта UDP в результирующую запись
			else result.back().proto = proto_t::UDP;
			// Устанавливаем описание проброшенного порта в результирующую запись
			::memcpy(result.back().description, description, ::strlen(description));
			// Устанавливаем время аренды проброшенного порта в результирующую запись
			result.back().lifetime = this->_fmk->atoi <uint32_t> (duration, ::strlen(duration));
			// Устанавливаем внутренний порт в результирующую запись
			result.back().internalPort = this->_fmk->atoi <uint16_t> (internalPort, ::strlen(internalPort));
			// Устанавливаем внешний порт в результирующую запись
			result.back().externalPort = this->_fmk->atoi <uint16_t> (externalPort, ::strlen(externalPort));
		}
		// Освобождаем память URL-ов UPnP
		::FreeUPNPUrls(&urls);
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки/удаления проброса портов на маршрутизаторе
 *
 * @param fwd  объект параметров проброса порта
 * @param mode режим включения/выключения проброса порта
 * @return     результат выполнения установки
 */
bool awh::PortMapping::mapping(const fwd_t & fwd, const event::mode_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип проброса порта
		 */
		switch(static_cast <uint8_t> (fwd.type)){
			// Если тип проброса порта является PCP
			case static_cast <uint8_t> (type_t::PCP): {
				// Семейство IP-адресов
				int32_t family = AF_UNSPEC;
				// Если указан внутренний IP-адрес
				if(fwd.internalAddress != nullptr){
					/**
					 * Определяем тип адреса
					 */
					switch(fwd.internalAddress->size){
						// Если адрес является IPv4
						case 4: family = AF_INET; break;
						// Если адрес является IPv6
						case 16: family = AF_INET6; break;
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC){
					// Если указан внешний IP-адрес
					if(fwd.externalAddress != nullptr){
						/**
						 * Определяем тип адреса
						 */
						switch(fwd.externalAddress->size){
							// Если адрес является IPv4
							case 4: family = AF_INET; break;
							// Если адрес является IPv6
							case 16: family = AF_INET6; break;
						}
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC)
					// Устанавливаем семейство адресов IPv4 по умолчанию
					family = AF_INET;
				// Структура маршрута
				gateway_t::route_t route{};
				/**
				 * Определяем семейство IP-адресов
				 */
				switch(family){
					// Если адрес является IPv4
					case AF_INET:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv4_t> ();
					break;
					// Если адрес является IPv6
					case AF_INET6:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv6_t> ();
					break;
				}
				// Если получаем маршрут для указанного адреса
				if(this->_gateway.get(route)){
					// Выполняем создание UDP сокета
					net::socket_t sock = ::socket(family, SOCK_DGRAM, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Выводим результат
						return result;
					}
					// Флаги установки опции
					const int32_t flags = 1;
					// Разрешаем/запрещаем повторно использовать тот же сокет после отключения
					if(static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Закрываем сокет
						::close(sock);
						// Выводим результат
						return result;
					}
					// PCP MAP request (RFC 6887)
					uint8_t request[60] = {0};
					// Устанавливаем значение версии
					request[0] = 2;
					// Устанавливаем опкод MAP (Client Request)
					request[1] = 1;
					// Размер объекта подключения
					socklen_t size = 0;
					// Параметры подключения к шлюзу
					struct sockaddr_storage addr{0};
					/**
					 * Определяем семейство IP-адресов
					 */
					switch(family){
						// Если адрес является IPv4
						case AF_INET: {
							/**
							 * Client IP Address (Header offset 8) -> ::ffff:192.168.x.x
							 * IPv4-mapped IPv6 address
							 */
							request[18] = 0xFF;
							request[19] = 0xFF;
							// Объект адреса шлюза
							struct sockaddr_in gw = {0};
							// Целевой адрес для "подключения" (чтобы узнать свой IP)
							struct sockaddr_in dst = {0};
							// Устанавливаем семейство адресов IPv4
							gw.sin_family = family;
							// Устанавливаем порт MDNS
							gw.sin_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							gw.sin_addr.s_addr = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
							// Выполняем "подключение" сокета к шлюзу
							if(::connect(sock, reinterpret_cast <struct sockaddr *> (&gw), sizeof(gw)) == 0){
								// Получаем локальный адрес сокета
								socklen_t length = sizeof(dst);
								// Извлекаем локальный адрес сокета
								::getsockname(sock, reinterpret_cast <struct sockaddr *> (&dst), &length);
								// Добавляем локальный IP-адрес клиента
								::memcpy(request + 20, &dst.sin_addr, 4);
							}
							// Закрываем сокет
							::close(sock);
							// Если не удалось определить локальный IP-адрес
							if(dst.sin_addr.s_addr == 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug(
										"Failed to determine local IP for PCP",
										__PRETTY_FUNCTION__,
										std::make_tuple(
											fwd.lifetime,
											fwd.description,
											fwd.internalPort,
											fwd.externalPort,
											static_cast <uint16_t> (fwd.type),
											static_cast <uint16_t> (fwd.proto),
											static_cast <uint16_t> (mode)
										),
										log_t::flag_t::CRITICAL
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Failed to determine local IP for PCP", log_t::flag_t::CRITICAL);
								#endif
								// Выводим результат
								return result;
							}
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in);
							// Копируем адрес шлюза
							::memcpy(&addr, &gw, size);
						} break;
						// Если адрес является IPv6
						case AF_INET6: {
							// Объект адреса шлюза
							struct sockaddr_in6 gw = {0};
							// Целевой адрес для "подключения" (чтобы узнать свой IP)
							struct sockaddr_in6 dst = {0};
							// Устанавливаем семейство адресов IPv6
							gw.sin6_family = family;
							// Устанавливаем порт MDNS
							gw.sin6_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							::memcpy(&gw.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 16);
							// Выполняем "подключение" сокета к шлюзу
							if(::connect(sock, reinterpret_cast <struct sockaddr *> (&gw), sizeof(gw)) == 0){
								// Получаем локальный адрес сокета
								socklen_t length = sizeof(dst);
								// Извлекаем локальный адрес сокета
								::getsockname(sock, reinterpret_cast <struct sockaddr *> (&dst), &length);
								// Добавляем локальный IP-адрес клиента
								::memcpy(request + 8, &dst.sin6_addr, 16);
							}
							// Закрываем сокет
							::close(sock);
							// Если не удалось определить локальный IP-адрес
							if(IN6_IS_ADDR_UNSPECIFIED(&dst.sin6_addr)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug(
										"Failed to determine local IP for PCP",
										__PRETTY_FUNCTION__,
										std::make_tuple(
											fwd.lifetime,
											fwd.description,
											fwd.internalPort,
											fwd.externalPort,
											static_cast <uint16_t> (fwd.type),
											static_cast <uint16_t> (fwd.proto),
											static_cast <uint16_t> (mode)
										),
										log_t::flag_t::CRITICAL
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Failed to determine local IP for PCP", log_t::flag_t::CRITICAL);
								#endif
								// Выводим результат
								return result;
							}
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in6);
							// Копируем адрес шлюза
							::memcpy(&addr, &gw, size);
						} break;
					}
					// Выполняем создание UDP сокета
					sock = ::socket(family, SOCK_DGRAM, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Выводим результат
						return result;
					}
					// Разрешаем/запрещаем повторно использовать тот же сокет после отключения
					if(static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Закрываем сокет
						::close(sock);
						// Выводим результат
						return result;
					}
					/**
					 * Определяем режим работы проброса порта
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо пробросить порт
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем время жизни (Header offset 4)
							* reinterpret_cast <uint32_t *> (request + 4) = htonl(fwd.lifetime);
						break;
						// Если необходимо убрать проброшенный порт
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Устанавливаем время жизни (Header offset 4)
							* reinterpret_cast <uint32_t *> (request + 4) = htonl(0);
						break;
					}
					// Если описание записи пустое
					if(::strlen(fwd.description) < 12){
						// Перебираем байты описания записи
						for(uint8_t i = 0; i < 12; ++i)
							// Заполняем описание записи случайными байтами
							const_cast <fwd_t &> (fwd).description[i] = request[24 + i] = (std::rand() % 255);
					// Если описание записи не пустое
					} else {
						// Перебираем байты описания записи
						for(uint8_t i = 0; i < 12; ++i)
							// Копируем описание записи в поле запроса
							request[24 + i] = fwd.description[i];
					}
					/**
					 * Определяем протокол проброса порта
					 */
					switch(static_cast <uint8_t> (fwd.proto)){
						// Если протокол проброса порта является TCP
						case static_cast <uint8_t> (proto_t::TCP):
							// Устанавливаем протокол TCP в запросе
							request[36] = 6; // протокол: 17 = UDP, 6 = TCP
						break;
						// Если протокол проброса порта является UDP
						case static_cast <uint8_t> (proto_t::UDP):
						default:
							// Устанавливаем протокол UDP в запросе
							request[36] = 17; // протокол: 17 = UDP, 6 = TCP
						break;
					}
					// Устанавливаем внутренний порт (Offset 40)
					* reinterpret_cast <uint16_t *> (request + 40) = htons(fwd.internalPort);
					// Устанавливаем внешний порт (Offset 42) (0 = авто)
					* reinterpret_cast <uint16_t *> (request + 42) = htons(fwd.externalPort);
					// Отправляем запрос на проброс порта
					ssize_t bytes = ::sendto(sock, reinterpret_cast <char *> (request), 60, 0, reinterpret_cast <struct sockaddr *> (&addr), size);
					// Если не удалось отправить запрос
					if(bytes != 60){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Закрываем сокет
						::close(sock);
						// Выводим результат
						return result;
					}
					// Буфер для приёма ответа
					char buffer[1024];
					// Получаем ответ от шлюза
					bytes = ::recvfrom(sock, buffer, sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&addr), &size);
					// Закрываем сокет
					::close(sock);
					// Если не удалось отправить запрос
					if(bytes <= 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Выводим результат
						return result;
					}
					// Устанавливаем терминальный нулевой символ в буфере
					buffer[bytes] = '\0';
					// Если получен ответ с корректным размером и опкодом (129 = MAP Opcode Response)
					if((bytes >= 60) && static_cast <uint8_t> (buffer[1]) == 129){
						// Получаем код результата
						const uint8_t code = buffer[3];
						// Если код результата не отрицательный
						if(code != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"PCP Error Code: %d",
									__PRETTY_FUNCTION__,
									std::make_tuple(
										fwd.lifetime,
										fwd.description,
										fwd.internalPort,
										fwd.externalPort,
										static_cast <uint16_t> (fwd.type),
										static_cast <uint16_t> (fwd.proto),
										static_cast <uint16_t> (mode)
									),
									log_t::flag_t::CRITICAL, code
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("PCP Error Code: %d", log_t::flag_t::CRITICAL, code);
							#endif
							// Выводим результат
							return result;
						}
						/**
						 * Определяем режим работы проброса порта
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если необходимо пробросить порт
							case static_cast <uint8_t> (event::mode_t::ENABLED): {
								// Проверяем совпадение внутреннего порта
								if((result = (fwd.internalPort == ntohs(* reinterpret_cast <const uint16_t *> (buffer + 42))))){
									// При получении ответа (возможно, позже):
									if(!(result = (::memcmp(buffer + 24, &fwd.description[0], 12) == 0))){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"Response was forged by an attacker on PCP",
												__PRETTY_FUNCTION__,
												std::make_tuple(
													fwd.lifetime,
													fwd.description,
													fwd.internalPort,
													fwd.externalPort,
													static_cast <uint16_t> (fwd.type),
													static_cast <uint16_t> (fwd.proto),
													static_cast <uint16_t> (mode)
												),
												log_t::flag_t::CRITICAL
											);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Response was forged by an attacker on PCP", log_t::flag_t::CRITICAL);
										#endif
									}
								// Если установленный порт не соответствует
								} else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug(
											"Port PCP mapping failed",
											__PRETTY_FUNCTION__,
											std::make_tuple(
												fwd.lifetime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Port PCP mapping failed", log_t::flag_t::CRITICAL);
									#endif
								}
							} break;
							// Если необходимо убрать проброшенный порт
							case static_cast <uint8_t> (event::mode_t::DISABLED): {
								// Проверяем совпадение внешнего порта
								if((result = (fwd.externalPort == ntohs(* reinterpret_cast <const uint16_t *> (buffer + 42))))){
									// При получении ответа (возможно, позже):
									if(!(result = (::memcmp(buffer + 24, &fwd.description[0], 12) == 0))){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"Response was forged by an attacker on PCP",
												__PRETTY_FUNCTION__,
												std::make_tuple(
													fwd.lifetime,
													fwd.description,
													fwd.internalPort,
													fwd.externalPort,
													static_cast <uint16_t> (fwd.type),
													static_cast <uint16_t> (fwd.proto),
													static_cast <uint16_t> (mode)
												),
												log_t::flag_t::CRITICAL
											);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Response was forged by an attacker on PCP", log_t::flag_t::CRITICAL);
										#endif
									}
								// Если установленный порт не соответствует
								} else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug(
											"Port PCP unmapping failed",
											__PRETTY_FUNCTION__,
											std::make_tuple(
												fwd.lifetime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Port PCP unmapping failed", log_t::flag_t::CRITICAL);
									#endif
								}
							} break;
						}
					}
				// Если маршрут не получен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Gateway address could not be obtained",
							__PRETTY_FUNCTION__,
							std::make_tuple(
								fwd.lifetime,
								fwd.description,
								fwd.internalPort,
								fwd.externalPort,
								static_cast <uint16_t> (fwd.type),
								static_cast <uint16_t> (fwd.proto),
								static_cast <uint16_t> (mode)
							),
							log_t::flag_t::CRITICAL
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Gateway address could not be obtained", log_t::flag_t::CRITICAL);
					#endif
				}
			} break;
			// Если тип проброса порта является UPNP
			case static_cast <uint8_t> (type_t::UPNP): {
				// Ищем устройства UPnP в локальной сети (3 секунды таймаут)
				UPNPDev * devlist = ::upnpDiscover(3000, nullptr, nullptr, 0, 0, 2, nullptr);
				// Если устройства не найдены
				if(devlist == nullptr)
					// Выводим пустой результат
					return result;
				// Действующий шлюз IGD
				UPNPUrls urls = {0};
				// Структура данных IGD
				IGDdatas data = {0};
				// Буфер для хранения внутреннего IP-адреса
				vector <char> internalAddress(64, 0);
				// Получаем действующий шлюз IGD
				int32_t status = ::UPNP_GetValidIGD(devlist, &urls, &data, &internalAddress[0], internalAddress.size(), nullptr, 0);
				// Освобождаем память списка устройств UPnP
				::freeUPNPDevlist(devlist);
				// Если не удалось получить действующий шлюз IGD
				if(status != 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								fwd.lifetime,
								fwd.description,
								fwd.internalPort,
								fwd.externalPort,
								static_cast <uint16_t> (fwd.type),
								static_cast <uint16_t> (fwd.proto),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING, ::strupnperror(status)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strupnperror(status));
					#endif
					// Освобождаем память URL-ов UPnP
					::FreeUPNPUrls(&urls);
					// Выводим пустой результат
					return result;
				}
				// Буфер для хранения внешнего IP-адреса
				char externalAddress[64] = {0};
				// Если указан внешний IP-адрес
				if(fwd.externalAddress != nullptr){
					/**
					 * Определяем тип адреса
					 */
					switch(fwd.externalAddress->size){
						// Если адрес является IPv4
						case 4: {
							// Устанавливаем полученный IPv4-адрес
							this->_addr.v4(awh_cast <net::addr_net_ipv4_t *> (fwd.externalAddress.get())->address, net_addr_t::endian_t::LITTLE);
							// Извлекаем IP-адрес в строковом формате
							string ip = ::move(static_cast <string> (this->_addr));
							// Если IP-адрес получен
							if(!ip.empty())
								// Копируем внешний IP-адрес в буфер
								::strncpy(&externalAddress[0], ip.c_str(), sizeof(externalAddress) - 1);
						} break;
						// Если адрес является IPv6
						case 16: {
							// Устанавливаем полученный IPv6-адрес
							this->_addr.v6(awh_cast <net::addr_net_ipv6_t *> (fwd.externalAddress.get())->address, net_addr_t::endian_t::LITTLE);
							// Извлекаем IP-адрес в строковом формате
							string ip = ::move(static_cast <string> (this->_addr));
							// Если IP-адрес получен
							if(!ip.empty())
								// Копируем внешний IP-адрес в буфер
								::strncpy(&externalAddress[0], ip.c_str(), sizeof(externalAddress) - 1);
						} break;
					}
				}
				/**
				 * Определяем режим работы проброса порта
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо пробросить порт
					case static_cast <uint8_t> (event::mode_t::ENABLED): {
						// Буфер для хранения внутреннего IP-адреса
						char internalPort[16];
						// Буфер для хранения внешнего IP-адреса
						char externalPort[16];
						// Устанавливаем внешний порт
						::snprintf(externalPort, sizeof(externalPort), "%d", fwd.externalPort);
						// Устанавливаем внутренний порт
						::snprintf(internalPort, sizeof(internalPort), "%d", fwd.internalPort);
						// Пробрасываем порт на маршрутизаторе
						status = ::UPNP_AddPortMapping(
							// Устанавливаем URL управления
							urls.controlURL,
							// Устанавливаем тип сервиса
							data.first.servicetype,
							// Устанавливаем внешний порт
							externalPort,
							// Устанавливаем внутренний порт
							internalPort,
							// Устанавливаем внутренний IP-адрес
							&internalAddress[0],
							// Устанавливаем описание проброса порта
							(::strlen(fwd.description) == 0 ? this->_fmk->format("%s (%s)", AWH_NAME, AWH_SHORT_NAME).c_str() : &fwd.description[0]),
							// Устанавливаем протокол порта
							(fwd.proto == proto_t::TCP ? "TCP" : "UDP"),
							// Устанавливаем внешний IP-адрес, если он указан
							&externalAddress[0],
							// Устанавливаем время жизни проброса порта
							(fwd.lifetime == 0 ? "0" : std::to_string(fwd.lifetime).c_str())
						);
					} break;
					// Если необходимо убрать проброшенный порт
					case static_cast <uint8_t> (event::mode_t::DISABLED): {
						// Буфер для хранения внешнего IP-адреса
						char externalPort[16];
						// Устанавливаем внешний порт
						::snprintf(externalPort, sizeof(externalPort), "%d", fwd.externalPort);
						// Удаляем проброс порта на маршрутизаторе
						status = ::UPNP_DeletePortMapping(
							// Устанавливаем URL управления
							urls.controlURL,
							// Устанавливаем тип сервиса
							data.first.servicetype,
							// Устанавливаем внешний порт
							externalPort,
							// Устанавливаем протокол порта
							(fwd.proto == proto_t::TCP ? "TCP" : "UDP"),
							// Устанавливаем внешний IP-адрес, если он указан
							&externalAddress[0]
						);
					} break;
				}
				// Очищаем память URL-ов UPnP
				::FreeUPNPUrls(&urls);
				// Если возникла ошибка
				if(!(result = (status == UPNPCOMMAND_SUCCESS))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								fwd.lifetime,
								fwd.description,
								fwd.internalPort,
								fwd.externalPort,
								static_cast <uint16_t> (fwd.type),
								static_cast <uint16_t> (fwd.proto),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING, ::strupnperror(status)
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strupnperror(status));
					#endif
				}
			} break;
			// Если тип проброса порта является NAT_PMP
			case static_cast <uint8_t> (type_t::NAT_PMP): {
				// Семейство IP-адресов
				int32_t family = AF_UNSPEC;
				// Если указан внутренний IP-адрес
				if(fwd.internalAddress != nullptr){
					/**
					 * Определяем тип адреса
					 */
					switch(fwd.internalAddress->size){
						// Если адрес является IPv4
						case 4: family = AF_INET; break;
						// Если адрес является IPv6
						case 16: family = AF_INET6; break;
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC){
					// Если указан внешний IP-адрес
					if(fwd.externalAddress != nullptr){
						/**
						 * Определяем тип адреса
						 */
						switch(fwd.externalAddress->size){
							// Если адрес является IPv4
							case 4: family = AF_INET; break;
							// Если адрес является IPv6
							case 16: family = AF_INET6; break;
						}
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC)
					// Устанавливаем семейство адресов IPv4 по умолчанию
					family = AF_INET;
				// Структура маршрута
				gateway_t::route_t route{};
				/**
				 * Определяем семейство IP-адресов
				 */
				switch(family){
					// Если адрес является IPv4
					case AF_INET:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv4_t> ();
					break;
					// Если адрес является IPv6
					case AF_INET6:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv6_t> ();
					break;
				}
				// Если получаем маршрут для указанного адреса
				if(this->_gateway.get(route)){
					// Выполняем создание UDP сокета
					net::socket_t sock = ::socket(family, SOCK_DGRAM, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Выводим результат
						return result;
					}
					// Флаги установки опции
					const int32_t flags = 1;
					// Разрешаем/запрещаем повторно использовать тот же сокет после отключения
					if(static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Закрываем сокет
						::close(sock);
						// Выводим результат
						return result;
					}
					// Размер объекта подключения
					socklen_t size = 0;
					// Параметры получения ответа от шлюза
					struct sockaddr_storage client{0};
					// Параметры подключения к шлюзу
					struct sockaddr_storage server{0};
					/**
					 * Определяем семейство IP-адресов
					 */
					switch(family){
						// Если адрес является IPv4
						case AF_INET: {
							// Объект адреса шлюза
							struct sockaddr_in gw = {0};
							// Устанавливаем семейство адресов IPv4
							gw.sin_family = family;
							// Устанавливаем порт MDNS
							gw.sin_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							gw.sin_addr.s_addr = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in);
							// Копируем адрес шлюза
							::memcpy(&server, &gw, size);
							// Копируем адрес клиента
							::memcpy(&client, &gw, size);
						} break;
						// Если адрес является IPv6
						case AF_INET6: {
							// Объект адреса шлюза
							struct sockaddr_in6 gw = {0};
							// Устанавливаем семейство адресов IPv6
							gw.sin6_family = family;
							// Устанавливаем порт MDNS
							gw.sin6_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							::memcpy(&gw.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 16);
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in6);
							// Копируем адрес клиента
							::memcpy(&client, &gw, size);
							// Копируем адрес шлюза
							::memcpy(&server, &gw, size);
						} break;
					}
					// === 3. Шаг 1: Запрос публичного IP (обязательный!) ===
					uint8_t request[12] = {0}; // версия=0, опкод=0
					// Отправляем запрос на проброс порта
					ssize_t bytes = ::sendto(sock, reinterpret_cast <char *> (request), 2, 0, reinterpret_cast <struct sockaddr *> (&server), size);
					// Если не удалось отправить запрос
					if(bytes <= 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Закрываем сокет
						::close(sock);
						// Выводим результат
						return result;
					}
					// Буфер для приёма ответа
					char buffer[1024];
					// Получаем ответ от шлюза
					bytes = ::recvfrom(sock, buffer, sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&client), &size);
					// Если не удалось отправить запрос
					if((bytes < 12) || (static_cast <uint8_t> (buffer[1]) != 128)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Закрываем сокет
						::close(sock);
						// Выводим результат
						return result;
					}
					// Устанавливаем терминальный нулевой символ в буфере
					buffer[bytes] = '\0';
					// === 4. Шаг 2: Запрос проброса порта ===
					::memset(request, 0, sizeof(request));
					// Устанавливаем версию протокола NAT-PMP
					request[0] = 0;
					/**
					 * Определяем протокол проброса порта
					 */
					switch(static_cast <uint8_t> (fwd.proto)){
						// Если протокол проброса порта является TCP
						case static_cast <uint8_t> (proto_t::TCP):
							// Устанавливаем опкод = 2 (map TCP)
							request[1] = 2;
						break;
						// Если протокол проброса порта является UDP
						case static_cast <uint8_t> (proto_t::UDP):
						default:
							// Устанавливаем опкод = 1 (map UDP)
							request[1] = 1;
						break;
					}
					// Устанавливаем зарезервировано
					request[2] = 0;
					request[3] = 0;
					/**
					 * Определяем режим работы проброса порта
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо пробросить порт
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем время жизни (секунды)
							* reinterpret_cast <uint32_t *> (request + 8) = htonl(fwd.lifetime);
						break;
						// Если необходимо убрать проброшенный порт
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Устанавливаем время жизни (секунды)
							* reinterpret_cast <uint32_t *> (request + 8) = htonl(0);
						break;
					}
					// Устанавливаем внутренний порт
					* reinterpret_cast <uint16_t *> (request + 4) = htons(fwd.internalPort);
					// Устанавливаем внешний порт = 0 (авто)
					* reinterpret_cast <uint16_t *> (request + 6) = htons(fwd.externalPort);
					// Отправляем запрос на проброс порта
					bytes = ::sendto(sock, reinterpret_cast <char *> (request), 12, 0, reinterpret_cast <struct sockaddr *> (&server), size);
					// Если не удалось отправить запрос
					if(bytes <= 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Закрываем сокет
						::close(sock);
						// Выводим результат
						return result;
					}
					// Получаем ответ от шлюза
					bytes = ::recvfrom(sock, buffer, sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&client), &size);
					// Закрываем сокет
					::close(sock);
					// Если не удалось отправить запрос
					if(bytes <= 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									fwd.lifetime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
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
						// Выводим результат
						return result;
					}
					// Устанавливаем терминальный нулевой символ в буфере
					buffer[bytes] = '\0';
					// === 5. Парсим ответ ===
					if((bytes >= 16) && (static_cast <uint8_t> (buffer[1]) == (request[1] | 128))){
						// Успех (Result code = 0) проверяем в map_resp[2..3]
						const uint16_t code = ntohs(* reinterpret_cast <const uint16_t *> (buffer + 2));
						// Если возникла ошибка
						if(code != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"NAT-PMP Error Code: %d",
									__PRETTY_FUNCTION__,
									std::make_tuple(
										fwd.lifetime,
										fwd.description,
										fwd.internalPort,
										fwd.externalPort,
										static_cast <uint16_t> (fwd.type),
										static_cast <uint16_t> (fwd.proto),
										static_cast <uint16_t> (mode)
									),
									log_t::flag_t::CRITICAL, code
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("NAT-PMP Error Code: %d", log_t::flag_t::CRITICAL, code);
							#endif
							// Выводим результат
							return result;
						}
						/**
						 * Определяем режим работы проброса порта
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если необходимо пробросить порт
							case static_cast <uint8_t> (event::mode_t::ENABLED): {
								// Если установленный порт не соответствует
								if(!(result = (fwd.externalPort == ntohs(* reinterpret_cast <const uint16_t *> (buffer + 10))))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug(
											"Port NAT-PMP mapping failed",
											__PRETTY_FUNCTION__,
											std::make_tuple(
												fwd.lifetime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Port NAT-PMP mapping failed", log_t::flag_t::CRITICAL);
									#endif
								}
							} break;
							// Если необходимо убрать проброшенный порт
							case static_cast <uint8_t> (event::mode_t::DISABLED): {
								// Проверяем совпадение внешнего порта
								if(!(result = (ntohs(* reinterpret_cast <const uint16_t *> (buffer + 10)) == 0))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug(
											"Port NAT-PMP unmapping failed",
											__PRETTY_FUNCTION__,
											std::make_tuple(
												fwd.lifetime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Port NAT-PMP unmapping failed", log_t::flag_t::CRITICAL);
									#endif
								}
							} break;
						}
					}
				// Если маршрут не получен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Gateway address could not be obtained",
							__PRETTY_FUNCTION__,
							std::make_tuple(
								fwd.lifetime,
								fwd.description,
								fwd.internalPort,
								fwd.externalPort,
								static_cast <uint16_t> (fwd.type),
								static_cast <uint16_t> (fwd.proto),
								static_cast <uint16_t> (mode)
							),
							log_t::flag_t::CRITICAL
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Gateway address could not be obtained", log_t::flag_t::CRITICAL);
					#endif
				}
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
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				std::make_tuple(
					fwd.lifetime,
					fwd.description,
					fwd.internalPort,
					fwd.externalPort,
					static_cast <uint16_t> (fwd.type),
					static_cast <uint16_t> (fwd.proto),
					static_cast <uint16_t> (mode)
				), log_t::flag_t::CRITICAL, error.what()
			);
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::PortMapping::PortMapping(const fmk_t * fmk, const log_t * log) noexcept :
 _gateway(fmk, log), _addr(fmk, log), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::PortMapping::~PortMapping() noexcept {}
