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
#include <cerrno>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <iostream>

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
