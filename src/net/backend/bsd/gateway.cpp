/**
 * @file: gateway.cpp
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
 * Макрос выравнивания структуры
 */
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/**
 * Стандартные модули
 */
#include <cerrno>
#include <memory>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iostream>

/**
 * Подключаем системные заголовки
 */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/eth/gateway.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем статичные функции в пространство имён
 */
namespace gw {
	/**
	 * @brief Функция преобразования префикса в маску подсети
	 *
	 * @param prefix префикс сети
	 * @return       маска подсети
	 */
	static in_addr_t prefix2mask(const uint8_t prefix) noexcept {
		// Если префикс равен нулю
		if(prefix == 0)
			// Возвращаем маску подсети
			return 0;
		// Выводим маску подсети
		return htonl((0xFFFFFFFFU) << (32 - static_cast <uint32_t> (prefix)));
	}
};

/**
 * @brief Метод получения маршрута для указанного адреса
 *
 * @param route объект для извлечения маршрута
 * @return      результат получения маршрута
 */
bool awh::Gateway::get(route_t & route) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип адреса
		 */
		switch(route.gateway->size){
			// Если адрес является IPv4
			case 4: {

			} break;
			// Если адрес является IPv6
			case 16: {

			} break;
			// Во всех остальных случаях
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unsupported address family", __PRETTY_FUNCTION__, std::make_tuple(route.gateway->size), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unsupported address family", log_t::flag_t::CRITICAL);
				#endif
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
 * @brief Метод добавления маршрута
 *
 * @param route объект маршрута для добавления
 * @return      результат добавления маршрута
 */
bool awh::Gateway::add(const route_t & route) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип адреса
		 */
		switch(route.gateway->size){
			// Если адрес является IPv4
			case 4: {

			} break;
			// Если адрес является IPv6
			case 16: {

			} break;
			// Во всех остальных случаях
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unsupported address family", __PRETTY_FUNCTION__, std::make_tuple(route.gateway->size), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unsupported address family", log_t::flag_t::CRITICAL);
				#endif
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
 * @brief Метод удаления маршрута
 *
 * @param route объект маршрута для удаления
 * @return      результат удаления маршрута
 */
bool awh::Gateway::remove(const route_t & route) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип адреса
		 */
		switch(route.gateway->size){
			// Если адрес является IPv4
			case 4: {
				// Читаем ВСЕ IPv4-маршруты
				int32_t mib[6] = {
					CTL_NET,     // Сетевой уровень
					PF_ROUTE,    // Протокол маршрутизации
					0,           // Производитель
					AF_INET,     // Адресное семейство IPv4
					NET_RT_DUMP, // Чтение маршрутов
					0            // Флаги поиска
				};
				// Размер буфера
				size_t length = 0;
				// Получаем размер буфера
				if(::sysctl(mib, 6, nullptr, &length, nullptr, 0) < 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
				// Буфер данных для получения маршрутов
				vector <uint8_t> buffer(length, 0);
				// Извлекаем маршруты в буфер
				if(::sysctl(mib, 6, buffer.data(), &length, nullptr, 0) < 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
				// Создаём сокет для удаления маршрутов
				net::socket_t sock = ::socket(PF_ROUTE, SOCK_RAW, 0);
				// Если сокет не создан
				if(sock == net::invalid_socket_t){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
				// Получаем итератор следующего маршрута
				uint8_t * begin = &buffer[0];
				// Получаем конец всех маршрутов
				uint8_t * end = (begin + length);
				// Получаем маску подсети из переданного префикса
				const in_addr_t netmsk = ::gw::prefix2mask(route.prefix);
				/**
				 * @brief Функция получения следующего адреса маршрута
				 *
				 * @param addr объект текущего адреса маршрута
				 * @return     объект следующего адреса маршрута
				 */
				auto advance = [](struct sockaddr * addr) noexcept -> struct sockaddr * {
					// Получаем длину структуры адреса
					const uint32_t length = static_cast <uint32_t> (addr->sa_len ? addr->sa_len : sizeof(long));
					// Извлекаем объект текущего адреса маршрута
					return reinterpret_cast <struct sockaddr *> (reinterpret_cast <uint8_t *> (addr) + ROUNDUP(length));
				};
				// Индекс текущего маршрута
				int32_t index = 0;
				/**
				 * Перебираем все маршруты
				 */
				while(begin < end){
					// Приводим к структуре маршрута
					struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
					// Если версия маршрута не совпадает
					if(rtm->rtm_version != RTM_VERSION)
						// Выходим из цикла
						break;
					// Объект текущего маршрута
					struct sockaddr_in * gw = nullptr;
					// Объект назначения маршрута
					struct sockaddr_in * dst = nullptr;
					// Объект маски подсети маршрута
					struct sockaddr_in * mask = nullptr;
					// Объект сетевого интерфейса маршрута
					struct sockaddr_dl * ifp = nullptr;
					// Объект текущего адреса маршрута
					struct sockaddr * sa = reinterpret_cast <struct sockaddr *> (rtm + 1);
					// Если присутствуют адреса в маршруте
					if(rtm->rtm_addrs & RTA_DST){
						// Если адрес назначения является IPv4
						if(sa->sa_family == AF_INET)
							// Извлекаем адрес назначения маршрута
							dst = reinterpret_cast <struct sockaddr_in *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Если присутствует шлюз в маршруте
					if(rtm->rtm_addrs & RTA_GATEWAY){
						// Если адрес шлюза является IPv4
						if(sa->sa_family == AF_INET)
							// Извлекаем адрес шлюза маршрута
							gw = reinterpret_cast <struct sockaddr_in *> (sa);
						// Если адрес шлюза является ссылочным
						else if(sa->sa_family == AF_LINK)
							// Извлекаем сетевой интерфейс маршрута
							ifp = reinterpret_cast <struct sockaddr_dl *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Если присутствует маска подсети в маршруте
					if(rtm->rtm_addrs & RTA_NETMASK){
						// Если адрес маски подсети является IPv4
						if(sa->sa_family == AF_INET)
							// Извлекаем маску подсети маршрута
							mask = reinterpret_cast <struct sockaddr_in *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Если присутствует сетевой интерфейс в маршруте
					if((rtm->rtm_addrs & RTA_IFP) && (ifp == nullptr)){
						// Если адрес сетевого интерфейса является ссылочным
						if(sa->sa_family == AF_LINK)
							// Извлекаем сетевой интерфейс маршрута
							ifp = reinterpret_cast <struct sockaddr_dl *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Флаг совпадения маршрута
					bool match = true;
					/**
					 * РЕЖИМ 1:
					 * Удалить ЛЮБОЙ default route (dst_str == "0.0.0.0" или nullptr + dst=0.0.0.0)
					 */
					// Проверяем, является ли адрес назначения маршрута default route
					const bool isDefault = ((dst != nullptr) && (dst->sin_addr.s_addr == INADDR_ANY));
					// Если требуется удалить ЛЮБОЙ default route
					if(isDefault && (awh_cast <net::addr_net_ipv4_t *> (route.dest.get())->address == 0) &&
					                (awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address == 0) && (netmsk == 0))
						// Удаляем ЛЮБОЙ default route
						goto DeleteRoute;
					/**
					 * РЕЖИМ 2:
					 * Точное совпадение по заданным параметрам
					 */
					// Если маска подсети задана
					if(netmsk > 0)
						// Устанавливаем флаг совпадения по маске подсети маршрута
						match = ((mask != nullptr) && (mask->sin_addr.s_addr == netmsk));
					// Если адрес назначения маршрута задан
					if(awh_cast <net::addr_net_ipv4_t *> (route.dest.get())->address > 0)
						// Устанавливаем флаг совпадения по адресу назначения маршрута
						match = ((dst != nullptr) && (dst->sin_addr.s_addr == awh_cast <net::addr_net_ipv4_t *> (route.dest.get())->address));
					// Если адрес шлюза маршрута задан
					if(awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address > 0)
						// Устанавливаем флаг совпадения по адресу шлюза маршрута
						match = ((gw != nullptr) && (gw->sin_addr.s_addr == awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address));
					// Если имя сетевого интерфейса задано
					if(!route.ifname.empty()){
						// Получаем индекс сетевого интерфейса
						const uint32_t idx = ::if_nametoindex(route.ifname.c_str());
						// Устанавливаем флаг совпадения по имени сетевого интерфейса маршрута
						match = ((ifp != nullptr) && (ifp->sdl_index == idx));
					}
					// Если адрес не совпадает
					if(!match){
						// Переходим к следующему маршруту
						begin += rtm->rtm_msglen;
						// Продолжаем перебор дальше
						continue;
					}
					// Устанавливаем метку удаления маршрута
					DeleteRoute:
					{
						// Буфер для удаления маршрута
						char buffer[1024];
						// Зануляем буфер для удаления маршрута
						::memset(buffer, 0, sizeof(buffer));
						// Получаем буфер полезной нагрузки текущего адреса маршрута
						char * payload = (buffer + sizeof(struct rt_msghdr));
						// Получаем объект текущего адреса маршрута
						struct rt_msghdr * rtd = reinterpret_cast <struct rt_msghdr *> (buffer);
						// Устанавливаем сметку маршрута
						rtd->rtm_seq = ++index;
						// Устанавливаем идентификатор процесса
						rtd->rtm_pid = ::getpid();
						// Устанавливаем тип сообщения на удаление маршрута
						rtd->rtm_type = RTM_DELETE;
						// Устанавливаем версию маршрута
						rtd->rtm_version = RTM_VERSION;
						// Устанавливаем флаги маршрута
						rtd->rtm_flags = rtm->rtm_flags;
						// Устанавливаем адреса маршрута
						rtd->rtm_addrs = rtm->rtm_addrs;
						// Объект для чтения адресов из исходного маршрута
						struct sockaddr * src = reinterpret_cast <struct sockaddr *> (rtm + 1);
						// Флаг, указывающий, что шлюз является ссылочным (AF_LINK)
						bool isGatewayLink = false;
						// Если присутствуют адреса в маршруте
						if(rtm->rtm_addrs & RTA_DST){
							// Копируем адрес назначения маршрута
							::memcpy(payload, src, src->sa_len);
							// Смещаем указатель полезной нагрузки
							payload += ROUNDUP(src->sa_len);
							// Устанавливаем текущий адрес маршрута
							src = advance(src);
						}
						// Если присутствует шлюз в маршруте
						if(rtm->rtm_addrs & RTA_GATEWAY){
							// Если адрес шлюза является ссылочным
							if(src->sa_family == AF_LINK)
								// Устанавливаем флаг маршрута
								isGatewayLink = true;
							// Копируем адрес шлюза маршрута
							::memcpy(payload, src, src->sa_len);
							// Смещаем указатель полезной нагрузки
							payload += ROUNDUP(src->sa_len);
							// Устанавливаем текущий адрес маршрута
							src = advance(src);
						}
						// Если присутствует маска подсети в маршруте
						if(rtm->rtm_addrs & RTA_NETMASK){
							// Копируем маску подсети маршрута
							::memcpy(payload, src, src->sa_len);
							// Смещаем указатель полезной нагрузки
							payload += ROUNDUP(src->sa_len);
							// Устанавливаем текущий адрес маршрута
							src = advance(src);
						}
						// Если присутствует сетевой интерфейс в маршруте
						if(rtm->rtm_addrs & RTA_IFP){
							// Проверяем, не скопирован ли уже как GATEWAY (AF_LINK)
							if(!isGatewayLink){
								// Копируем сетевой интерфейс маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
							}
						}
						// Устанавливаем длину сообщения маршрута
						rtd->rtm_msglen = (payload - buffer);
						// Записываем сообщение в сокет
						if(!(result = (::write(sock, buffer, rtd->rtm_msglen) > 0))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					}
					// Переходим к следующему маршруту
					begin += rtm->rtm_msglen;
				}
				// Закрываем сокет
				::close(sock);
			} break;
			// Если адрес является IPv6
			case 16: {
				// Читаем ВСЕ IPv6-маршруты
				int32_t mib[6] = {
					CTL_NET,     // Сетевой уровень
					PF_ROUTE,    // Протокол маршрутизации
					0,           // Производитель
					AF_INET6,    // Адресное семейство IPv6
					NET_RT_DUMP, // Чтение маршрутов
					0            // Флаги поиска
				};
				// Размер буфера
				size_t length = 0;
				// Получаем размер буфера
				if(::sysctl(mib, 6, nullptr, &length, nullptr, 0) < 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
				// Буфер данных для получения маршрутов
				vector <uint8_t> buffer(length, 0);
				// Извлекаем маршруты в буфер
				if(::sysctl(mib, 6, buffer.data(), &length, nullptr, 0) < 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
				// Создаём сокет для удаления маршрутов
				net::socket_t sock = ::socket(PF_ROUTE, SOCK_RAW, 0);
				// Если сокет не создан
				if(sock == net::invalid_socket_t){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
				// Получаем итератор следующего маршрута
				uint8_t * begin = &buffer[0];
				// Получаем конец всех маршрутов
				uint8_t * end = (begin + length);
				// Получаем маску подсети из переданного префикса
				struct in6_addr netmsk;
				// Зануляем маску
				::memset(&netmsk, 0, sizeof(netmsk));
				// Если префикс задан
				if(route.prefix > 0){
					// Текущий префикс
					uint32_t prefix = static_cast <uint32_t> (route.prefix);
					// Проходим по байтам
					for(uint8_t i = 0; i < 16; ++i){
						// Если префикс больше либо равен 8
						if(prefix >= 8){
							// Устанавливаем байт маски подсети
							netmsk.s6_addr[i] = 0xff;
							// Уменьшаем префикс на 8
							prefix -= 8;
						// Если префикс меньше 8, но больше нуля
						} else if(prefix > 0) {
							// Устанавливаем байт маски подсети
							netmsk.s6_addr[i] = static_cast <uint8_t> (0xff << (8 - prefix));
							// Обнуляем префикс
							prefix = 0;
						// Зануляем байт маски подсети
						} else netmsk.s6_addr[i] = 0;
					}
				}
				/**
				 * @brief Функция получения следующего адреса маршрута
				 *
				 * @param addr объект текущего адреса маршрута
				 * @return     объект следующего адреса маршрута
				 */
				auto advance = [](struct sockaddr * addr) noexcept -> struct sockaddr * {
					// Получаем длину структуры адреса
					const uint32_t length = static_cast <uint32_t> (addr->sa_len ? addr->sa_len : sizeof(long));
					// Извлекаем объект текущего адреса маршрута
					return reinterpret_cast <struct sockaddr *> (reinterpret_cast <uint8_t *> (addr) + ROUNDUP(length));
				};
				// Индекс текущего маршрута
				int32_t index = 0;
				/**
				 * Перебираем все маршруты
				 */
				while(begin < end){
					// Приводим к структуре маршрута
					struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
					// Если версия маршрута не совпадает
					if(rtm->rtm_version != RTM_VERSION)
						// Выходим из цикла
						break;
					// Объект текущего маршрута
					struct sockaddr_in6 * gw = nullptr;
					// Объект назначения маршрута
					struct sockaddr_in6 * dst = nullptr;
					// Объект маски подсети маршрута
					struct sockaddr_in6 * mask = nullptr;
					// Объект сетевого интерфейса маршрута
					struct sockaddr_dl * ifp = nullptr;
					// Объект текущего адреса маршрута
					struct sockaddr * sa = reinterpret_cast <struct sockaddr *> (rtm + 1);
					// Если присутствуют адреса в маршруте
					if(rtm->rtm_addrs & RTA_DST){
						// Если адрес назначения является IPv6
						if(sa->sa_family == AF_INET6)
							// Извлекаем адрес назначения маршрута
							dst = reinterpret_cast <struct sockaddr_in6 *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Если присутствует шлюз в маршруте
					if(rtm->rtm_addrs & RTA_GATEWAY){
						// Если адрес шлюза является IPv6
						if(sa->sa_family == AF_INET6)
							// Извлекаем адрес шлюза маршрута
							gw = reinterpret_cast <struct sockaddr_in6 *> (sa);
						// Если адрес шлюза является ссылочным
						else if(sa->sa_family == AF_LINK)
							// Извлекаем сетевой интерфейс маршрута
							ifp = reinterpret_cast <struct sockaddr_dl *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Если присутствует маска подсети в маршруте
					if(rtm->rtm_addrs & RTA_NETMASK){
						// Если адрес маски подсети является IPv6
						if(sa->sa_family == AF_INET6)
							// Извлекаем маску подсети маршрута
							mask = reinterpret_cast <struct sockaddr_in6 *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Если присутствует сетевой интерфейс в маршруте
					if((rtm->rtm_addrs & RTA_IFP) && (ifp == nullptr)){
						// Если адрес сетевого интерфейса является ссылочным
						if(sa->sa_family == AF_LINK)
							// Извлекаем сетевой интерфейс маршрута
							ifp = reinterpret_cast <struct sockaddr_dl *> (sa);
						// Устанавливаем текущий адрес маршрута
						sa = advance(sa);
					}
					// Флаг совпадения маршрута
					bool match = true;
					/**
					 * РЕЖИМ 1:
					 * Удалить ЛЮБОЙ default route
					 */
					// Проверяем, является ли адрес назначения маршрута default route
					const bool isDefault = ((dst != nullptr) && (IN6_IS_ADDR_UNSPECIFIED(&dst->sin6_addr)));
					// Нулевой IPv6 адрес
					const uint8_t zeroAddr[16] = {0};
					// Если требуется удалить ЛЮБОЙ default route
					if(isDefault && (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.dest.get())->address[0], zeroAddr, 16) == 0) &&
					                (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], zeroAddr, 16) == 0) && (route.prefix == 0))
						// Удаляем ЛЮБОЙ default route
						goto DeleteRouteIPv6;
					/**
					 * РЕЖИМ 2:
					 * Точное совпадение по заданным параметрам
					 */
					// Если маска подсети задана
					if(route.prefix > 0)
						// Устанавливаем флаг совпадения по маске подсети маршрута
						match = ((mask != nullptr) && (::memcmp(&mask->sin6_addr, &netmsk, 16) == 0));
					// Если адрес назначения маршрута задан
					if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.dest.get())->address[0], zeroAddr, 16) != 0)
						// Устанавливаем флаг совпадения по адресу назначения маршрута
						match = match && ((dst != nullptr) && (::memcmp(&dst->sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.dest.get())->address[0], 16) == 0));
					// Если адрес шлюза маршрута задан
					if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], zeroAddr, 16) != 0)
						// Устанавливаем флаг совпадения по адресу шлюза маршрута
						match = match && ((gw != nullptr) && (::memcmp(&gw->sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 16) == 0));
					// Если имя сетевого интерфейса задано
					if(!route.ifname.empty()){
						// Получаем индекс сетевого интерфейса
						const uint32_t idx = ::if_nametoindex(route.ifname.c_str());
						// Устанавливаем флаг совпадения по имени сетевого интерфейса маршрута
						match = match && ((ifp != nullptr) && (ifp->sdl_index == idx));
					}
					// Если адрес не совпадает
					if(!match){
						// Переходим к следующему маршруту
						begin += rtm->rtm_msglen;
						// Продолжаем перебор дальше
						continue;
					}
					// Устанавливаем метку удаления маршрута
					DeleteRouteIPv6:
					{
						// Буфер для удаления маршрута
						char buffer[1024];
						// Зануляем буфер для удаления маршрута
						::memset(buffer, 0, sizeof(buffer));
						// Получаем буфер полезной нагрузки текущего адреса маршрута
						char * payload = (buffer + sizeof(struct rt_msghdr));
						// Получаем объект текущего адреса маршрута
						struct rt_msghdr * rtd = reinterpret_cast <struct rt_msghdr *> (buffer);
						// Устанавливаем сметку маршрута
						rtd->rtm_seq = ++index;
						// Устанавливаем идентификатор процесса
						rtd->rtm_pid = ::getpid();
						// Устанавливаем тип сообщения на удаление маршрута
						rtd->rtm_type = RTM_DELETE;
						// Устанавливаем версию маршрута
						rtd->rtm_version = RTM_VERSION;
						// Устанавливаем флаги маршрута
						rtd->rtm_flags = rtm->rtm_flags;
						// Устанавливаем адреса маршрута
						rtd->rtm_addrs = rtm->rtm_addrs;
						// Объект для чтения адресов из исходного маршрута
						struct sockaddr * src = reinterpret_cast <struct sockaddr *> (rtm + 1);
						// Флаг, указывающий, что шлюз является ссылочным (AF_LINK)
						bool isGatewayLink = false;
						// Если присутствуют адреса в маршруте
						if(rtm->rtm_addrs & RTA_DST){
							// Копируем адрес назначения маршрута
							::memcpy(payload, src, src->sa_len);
							// Смещаем указатель полезной нагрузки
							payload += ROUNDUP(src->sa_len);
							// Устанавливаем текущий адрес маршрута
							src = advance(src);
						}
						// Если присутствует шлюз в маршруте
						if(rtm->rtm_addrs & RTA_GATEWAY){
							// Если адрес шлюза является ссылочным
							if(src->sa_family == AF_LINK)
								// Устанавливаем флаг маршрута
								isGatewayLink = true;
							// Копируем адрес шлюза маршрута
							::memcpy(payload, src, src->sa_len);
							// Смещаем указатель полезной нагрузки
							payload += ROUNDUP(src->sa_len);
							// Устанавливаем текущий адрес маршрута
							src = advance(src);
						}
						// Если присутствует маска подсети в маршруте
						if(rtm->rtm_addrs & RTA_NETMASK){
							// Копируем маску подсети маршрута
							::memcpy(payload, src, src->sa_len);
							// Смещаем указатель полезной нагрузки
							payload += ROUNDUP(src->sa_len);
							// Устанавливаем текущий адрес маршрута
							src = advance(src);
						}
						// Если присутствует сетевой интерфейс в маршруте
						if(rtm->rtm_addrs & RTA_IFP){
							// Проверяем, не скопирован ли уже как GATEWAY (AF_LINK)
							if(!isGatewayLink){
								// Копируем сетевой интерфейс маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
							}
						}
						// Устанавливаем длину сообщения маршрута
						rtd->rtm_msglen = (payload - buffer);
						// Записываем сообщение в сокет
						if(!(result = (::write(sock, buffer, rtd->rtm_msglen) > 0))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					}
					// Переходим к следующему маршруту
					begin += rtm->rtm_msglen;
				}
				// Закрываем сокет
				::close(sock);
			} break;
			// Во всех остальных случаях
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unsupported address family", __PRETTY_FUNCTION__, std::make_tuple(route.gateway->size), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unsupported address family", log_t::flag_t::CRITICAL);
				#endif
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
