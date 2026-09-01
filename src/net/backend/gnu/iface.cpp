/**
 * @file iface.cpp
 * @date 2026-08-06
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
 * @brief Реализация бэкенда работы с сетевыми интерфейсами — перечисление интерфейсов машины, получение их адресов,
 *        флагов, MTU и состояния,
 *        создание и настройка TUN/TAP-устройств на каждой поддерживаемой операционной системе
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cerrno>
#include <memory>
#include <string>
#include <cstring>
#include <cstdlib>

/**
 * Системные заголовочные файлы
 */
#include <fcntl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <linux/if_tun.h>
#include <linux/ipv6.h>
#include <linux/sockios.h>

/**
 * Определяем константу времени жизни, если она не задана
 */
#ifndef ND6_INFINITE_LIFETIME
	/**
	 * Время жизни адреса IPv6 - бесконечность
	 */
	#define ND6_INFINITE_LIFETIME 0xFFFFFFFF
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/iface.hpp>
#include <net/backend/gnu/netlink.hpp>



/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статичные функции в пространство имён
 *
 * @warning Пространство зовётся здесь не `iface`, как у прочих систем, а `device`:
 *          именем `iface` у glibc занята структура, объявленная в `net/if.h`, и
 *          пространство с тем же именем ею перекрывается. Снимать чужое объявление
 *          ради своего имени значило бы менять окружение потребителя заголовка, потому
 *          расходится имя, а не окружение
 *
 */
namespace device {
	/**
	 * @brief Функция преобразования префикса в маску подсети
	 *
	 * @param prefix префикс сети
	 * @return       маска подсети
	 *
	 */
	static in_addr_t prefix2mask(const uint8_t prefix) noexcept {
		// Если префикс равен нулю
		if(prefix == 0)
			// Возвращаем маску подсети
			return 0;
		// Возвращаем маску подсети
		return htonl((0xFFFFFFFFU) << (32 - static_cast <uint32_t> (prefix)));
	}
	/**
	 * @brief Функция преобразования маски подсети в префикс
	 *
	 * @param mask маска подсети
	 * @return     префикс сети
	 *
	 */
	static uint8_t mask2prefix(const struct in_addr & mask) noexcept {
		// Переменная результата
		uint8_t result = 0;
		// Преобразуем маску подсети в префикс
		uint32_t value = ntohl(mask.s_addr);
		/**
		 * Пока старший бит равен единице
		 */
		while(value & 0x80000000){
			// Увеличиваем префикс
			result++;
			// Сдвигаем значение маски подсети влево
			value <<= 1;
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция безопасного копирования имени интерфейса в фиксированный буфер
	 *
	 * @param buffer буфер назначения фиксированного размера IFNAMSIZ
	 * @param name   имя сетевого интерфейса (может быть не нуль-терминированным string_view)
	 *
	 */
	static void copyName(char (& buffer)[IFNAMSIZ], const string_view name) noexcept {
		// Определяем количество копируемых байт с учётом завершающего нуля
		const size_t length = (name.size() < static_cast <size_t> (IFNAMSIZ - 1)) ? name.size() : static_cast <size_t> (IFNAMSIZ - 1);
		// Копируем имя интерфейса ровно на длину переданного представления
		::memcpy(buffer, name.data(), length);
		// Устанавливаем завершающий ноль
		buffer[length] = '\0';
	}
	/**
	 * @brief Функция проверки типа канального уровня на принадлежность туннелю
	 *
	 * @param type тип интерфейса канального уровня (sll_hatype)
	 * @return     результат проверки
	 *
	 */
	static bool isTunnelLinkType(const uint16_t type) noexcept {
		/**
		 * Определяем тип интерфейса
		 */
		switch(type){
			// Если это туннельный интерфейс (IPIP)
			case ARPHRD_TUNNEL:
			// Если это туннельный интерфейс (IPIP поверх IPv6)
			case ARPHRD_TUNNEL6:
			// Если это туннельный интерфейс (SIT, IPv6 поверх IPv4)
			case ARPHRD_SIT:
			// Если это туннельный интерфейс (IPv6 поверх IPv6)
			// Если это туннельный интерфейс (GRE)
			case ARPHRD_IPGRE:
			// Если это туннельный интерфейс (PPP)
			case ARPHRD_PPP:
				// Сообщаем, что это туннельный интерфейс
				return true;
		}
		// Сообщаем, что это не туннельный интерфейс
		return false;
	}
	/**
	 * @brief Функция проверки типа канального уровня на принадлежность виртуальному интерфейсу
	 *
	 * @param type тип интерфейса канального уровня (sll_hatype)
	 * @return     результат проверки
	 *
	 */
	static bool isVirtualLinkType(const uint16_t type) noexcept {
		/**
		 * Определяем тип интерфейса
		 */
		switch(type){
			/**
			 * Определяем виртуальные интерфейсы
			 *
			 * @note Отдельного типа канального уровня у мостов и разделения на логические
			 *       сегменты Linux не заводит: и мост, и сегмент 802.1Q числятся обычным
			 *       Ethernet. Отличить их по типу нельзя, и разбор ведётся выше - по имени
			 *       устройства и по его признакам
			 */
			// Если это виртуальный интерфейс (Loopback)
			case ARPHRD_LOOPBACK:
			// Если это виртуальный интерфейс (PPP)
			case ARPHRD_PPP:
			// Если это виртуальный интерфейс (IPIP)
			case ARPHRD_TUNNEL:
			// Если это виртуальный интерфейс (IPIP поверх IPv6)
			case ARPHRD_TUNNEL6:
			// Если это виртуальный интерфейс (SIT)
			case ARPHRD_SIT:
			// Если это виртуальный интерфейс (GRE)
			case ARPHRD_IPGRE:
			// Если это виртуальный интерфейс без канального уровня вовсе
			case ARPHRD_NONE:
				// Сообщаем, что это виртуальный интерфейс
				return true;
		}
		// Сообщаем, что это не виртуальный интерфейс
		return false;
	}
	/**
	 * @brief Функция поиска имени сетевого интерфейса по адресу в уже полученном списке
	 *
	 * @param list список сетевых интерфейсов
	 * @param addr адрес сетевого подключения
	 * @return     имя найденного сетевого интерфейса
	 *
	 */
	static string findNameByAddr(struct ifaddrs * list, const awh::net::addr_t * addr) noexcept {
		/**
		 * Перебираем все сетевые интерфейсы
		 */
		for(struct ifaddrs * ifa = list; ifa != nullptr; ifa = ifa->ifa_next){
			/**
			 * Определяем тип адреса
			 */
			switch(addr->size){
				// Если адрес является MAC-адресом
				case 6: {
					/**
					 * Ищем MAC-адрес интерфейса
					 *
					 * @note Записи канального уровня Linux выдаёт под семейством AF_PACKET
					 *       и описывает структурой sockaddr_ll, тогда как BSD и macOS
					 *       пользуются AF_LINK и sockaddr_dl
					 */
					if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_PACKET)){
						// Получаем текущее значение аппаратного сетевого адреса
						struct sockaddr_ll * sll = reinterpret_cast <struct sockaddr_ll *> (ifa->ifa_addr);
						// Проверяем длину MAC-адреса
						if(sll->sll_halen == 6){
							// Получаем указатель на MAC-адрес
							const uint8_t * mac = reinterpret_cast <const uint8_t *> (sll->sll_addr);
							// Сравниваем MAC-адреса
							if(::memcmp(&awh_cast <const awh::net::addr_mac_t *> (addr)->address[0], mac, 6) == 0)
								// Возвращаем найденное имя интерфейса
								return string(ifa->ifa_name);
						}
					}
				} break;
				// Если адрес является IPv4
				case 4: {
					// Если не IPv4 адреса
					if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
						// Переходим к следующему интерфейсу
						continue;
					// Получаем указатель на структуру IPv4
					struct sockaddr_in * sin = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr);
					// Если адреса совпадают
					if(sin->sin_addr.s_addr == awh_cast <const awh::net::addr_net_ipv4_t *> (addr)->address)
						// Возвращаем найденное имя интерфейса
						return string(ifa->ifa_name);
				} break;
				// Если адрес является IPv6
				case 16: {
					// Если не IPv6 адреса
					if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
						// Переходим к следующему интерфейсу
						continue;
					// Получаем указатель на структуру IPv6
					struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
					// Если адреса совпадают
					if(::memcmp(&sin->sin6_addr, &awh_cast <const awh::net::addr_net_ipv6_t *> (addr)->address[0], sizeof(in6_addr)) == 0)
						// Возвращаем найденное имя интерфейса
						return string(ifa->ifa_name);
				} break;
			}
		}
		// Возвращаем пустое имя интерфейса
		return string{};
	}
	/**
	 * @brief Функция классификации интерфейса (туннельный/виртуальный) в уже полученном списке
	 *
	 * @param list список сетевых интерфейсов
	 * @param name имя сетевого интерфейса
	 * @param virt режим классификации: true - виртуальный, false - туннельный
	 * @param fmk  объект фреймворка
	 * @return     результат классификации интерфейса
	 *
	 */
	static bool classify(struct ifaddrs * list, const string_view name, const bool virt, const awh::fmk_t * fmk) noexcept {
		// Переменная результата
		bool result = false;
		/**
		 * Перебираем все сетевые интерфейсы
		 */
		for(struct ifaddrs * ifa = list; ifa != nullptr; ifa = ifa->ifa_next){
			// Если имя интерфейса не совпадает
			if(!fmk->compare(ifa->ifa_name, name))
				// Переходим к следующему интерфейсу
				continue;
			// Применяем эвристику по флагам интерфейса
			if(virt)
				// Виртуальный интерфейс обычно Point-to-Point или Loopback
				result = ((ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_flags & IFF_LOOPBACK));
			// Туннель обычно Point-to-Point и не Broadcast
			else result = ((ifa->ifa_flags & IFF_POINTOPOINT) && !(ifa->ifa_flags & IFF_BROADCAST));
			// Дополнительная точная проверка через записи канального уровня
			if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_PACKET)){
				// Получаем структуру адреса канального уровня
				struct sockaddr_ll * sll = reinterpret_cast <struct sockaddr_ll *> (ifa->ifa_addr);
				// Если тип интерфейса точно определён, возвращаем результат окончательно
				if(virt ? ::device::isVirtualLinkType(static_cast <uint16_t> (sll->sll_hatype)) : ::device::isTunnelLinkType(static_cast <uint16_t> (sll->sll_hatype)))
					// Сообщаем, что интерфейс точно классифицирован
					return true;
			}
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция заведения маршрутов до встречных сторон устройства
	 *
	 * @details Маршрут до другого конца точки-точки ядро заводит лишь при назначении
	 *          адреса на ПОДНЯТОЕ устройство. Движок же адреса назначает раньше, чем
	 *          устройство поднимут, и маршрут не появляется вовсе: обратный путь
	 *          туннеля молчит при исправных адресах с обеих сторон
	 *
	 * @note Адрес встречной стороны берётся у самого ядра: назначение его сохранило,
	 *       и держать эти сведения ещё и у себя незачем
	 *
	 * @param name название сетевого интерфейса
	 * @param log  объект для работы с логами
	 *
	 */
	static void routePeers(const string_view name, const awh::log_t * log) noexcept {
		// Список сетевых интерфейсов машины
		struct ifaddrs * addresses = nullptr;
		// Если список сетевых интерфейсов получить не удалось
		if(::getifaddrs(&addresses) != 0)
			// Выходим из функции
			return;
		// Получаем номер сетевого интерфейса по его имени
		const uint32_t index = ::if_nametoindex(string(name).c_str());
		// Если номер сетевого интерфейса получен
		if(index > 0){
			// Объект работы с ядром через сокет управления сетью
			const awh::gnu::netlink_t netlink(log);
			// Проходим по всему списку сетевых интерфейсов
			for(struct ifaddrs * i = addresses; i != nullptr; i = i->ifa_next){
				// Если запись принадлежит другому устройству
				if((i->ifa_name == nullptr) || (name.compare(i->ifa_name) != 0))
					// Переходим к следующей записи
					continue;
				// Если запись не является адресом IPv6 со встречной стороной
				if((i->ifa_addr == nullptr) || (i->ifa_addr->sa_family != AF_INET6) ||
				   (i->ifa_dstaddr == nullptr) || (i->ifa_dstaddr->sa_family != AF_INET6))
					// Переходим к следующей записи
					continue;
				// Получаем адрес встречной стороны
				const struct sockaddr_in6 * peer = reinterpret_cast <const struct sockaddr_in6 *> (i->ifa_dstaddr);
				// Если адрес встречной стороны совпадает со своим адресом
				if(::memcmp(&peer->sin6_addr, &reinterpret_cast <const struct sockaddr_in6 *> (i->ifa_addr)->sin6_addr, 16) == 0)
					// Переходим к следующей записи: встречной стороны у адреса нет
					continue;
				/**
				 * @brief Структура сообщения заведения маршрута
				 *
				 */
				struct {
					// Заголовок сообщения
					struct nlmsghdr header;
					// Описание маршрута
					struct rtmsg route;
					// Место под свойства маршрута
					uint8_t buffer[64];
				} message{};
				// Устанавливаем размер сообщения
				message.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
				// Устанавливаем тип запроса
				message.header.nlmsg_type = RTM_NEWROUTE;
				// Устанавливаем признаки запроса заведения с подтверждением
				message.header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK);
				// Устанавливаем порядковый номер запроса
				message.header.nlmsg_seq = 1;
				// Устанавливаем семейство маршрута
				message.route.rtm_family = AF_INET6;
				// Устанавливаем длину приставки назначения: маршрут ведёт к одному адресу
				message.route.rtm_dst_len = 128;
				// Устанавливаем таблицу маршрута
				message.route.rtm_table = RT_TABLE_MAIN;
				// Устанавливаем способ заведения маршрута
				message.route.rtm_protocol = RTPROT_STATIC;
				// Устанавливаем область действия маршрута
				message.route.rtm_scope = RT_SCOPE_UNIVERSE;
				// Устанавливаем вид маршрута
				message.route.rtm_type = RTN_UNICAST;
				// Получаем место под свойство адреса назначения маршрута
				struct rtattr * attribute = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (&message) + NLMSG_ALIGN(message.header.nlmsg_len));
				// Устанавливаем вид свойства
				attribute->rta_type = RTA_DST;
				// Устанавливаем размер свойства
				attribute->rta_len = RTA_LENGTH(16);
				// Устанавливаем адрес назначения маршрута
				::memcpy(RTA_DATA(attribute), &peer->sin6_addr, 16);
				// Увеличиваем размер сообщения на размер свойства
				message.header.nlmsg_len = (NLMSG_ALIGN(message.header.nlmsg_len) + RTA_ALIGN(attribute->rta_len));
				// Получаем место под свойство устройства маршрута
				attribute = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (&message) + NLMSG_ALIGN(message.header.nlmsg_len));
				// Устанавливаем вид свойства
				attribute->rta_type = RTA_OIF;
				// Устанавливаем размер свойства
				attribute->rta_len = RTA_LENGTH(sizeof(uint32_t));
				// Устанавливаем устройство маршрута
				::memcpy(RTA_DATA(attribute), &index, sizeof(index));
				// Увеличиваем размер сообщения на размер свойства
				message.header.nlmsg_len = (NLMSG_ALIGN(message.header.nlmsg_len) + RTA_ALIGN(attribute->rta_len));
				// Выполняем отправку сообщения ядру
				netlink.commit(&message, message.header.nlmsg_len);
			}
		}
		// Освобождаем список сетевых интерфейсов
		::freeifaddrs(addresses);
	}
	/**
	 * @brief Функция применения IP-адреса (и при необходимости адреса пира) к интерфейсу через указанный сокет
	 *
	 * @param sock   управляющий сокет
	 * @param name   имя сетевого интерфейса
	 * @param ip     адрес сетевого интерфейса для установки
	 * @param peer   адрес удалённого пира (для точка-точка) либо nullptr
	 * @param prefix префикс подсети
	 * @param log    объект работы с логами
	 * @return       результат применения адреса
	 *
	 */
	static bool applyAddress(const awh::net::socket_t sock, const string_view name, const awh::net::addr_t * ip, const awh::net::addr_t * peer, const uint8_t prefix, const awh::log_t * log) noexcept {
		// Переменная результата
		bool result = false;
		/**
		 * Определяем тип адреса
		 */
		switch(ip->size){
			// Если адрес является IPv4
			case 4: {
				/**
				 * Назначаем адрес и маску подсети
				 *
				 * @details У BSD и macOS адрес, маска и широковещательный адрес ставятся
				 *          разом, одним запросом SIOCAIFADDR со сборной структурой. У Linux
				 *          такого запроса нет: каждое из значений ставится своим запросом, и
				 *          порядок здесь существенен - маску следует задавать после адреса,
				 *          иначе ядро подставит ей своё умолчание по классу сети
				 *
				 * @warning Атомарности у такой установки нет: между запросами устройство
				 *          некоторое время держит адрес с чужой маской. Обойти это средствами
				 *          ioctl нельзя - иного способа Linux не даёт вовсе
				 *
				 */
				// Объект запроса сетевого интерфейса
				struct ifreq ifr{};
				// Копируем имя сетевого интерфейса
				::device::copyName(ifr.ifr_name, name);
				// Получаем указатель на адрес запроса
				struct sockaddr_in * sin = reinterpret_cast <struct sockaddr_in *> (&ifr.ifr_addr);
				// Устанавливаем семейство адресов IPv4
				sin->sin_family = AF_INET;
				// Устанавливаем IP-адрес интерфейса
				sin->sin_addr.s_addr = awh_cast <const awh::net::addr_net_ipv4_t *> (ip)->address;
				// Применяем новый IP-адрес интерфейса
				if(!(result = (::ioctl(sock, SIOCSIFADDR, &ifr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				// Устанавливаем семейство маски подсети
				sin->sin_family = AF_INET;
				// Если префикс подсети больше 32 или равен 0
				if((prefix > 32) || (prefix == 0))
					// Устанавливаем маску подсети интерфейса как /32
					sin->sin_addr.s_addr = 0xFFFFFFFF;
				// Устанавливаем маску подсети интерфейса
				else sin->sin_addr.s_addr = ::device::prefix2mask(prefix);
				// Применяем маску подсети интерфейса
				if(!(result = (::ioctl(sock, SIOCSIFNETMASK, &ifr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				// Если задан адрес удалённого пира (точка-точка)
				if(peer != nullptr){
					// Устанавливаем семейство адреса пира
					sin->sin_family = AF_INET;
					// Устанавливаем IP-адрес удалённого пира
					sin->sin_addr.s_addr = awh_cast <const awh::net::addr_net_ipv4_t *> (peer)->address;
					// Применяем адрес удалённого пира
					if(!(result = (::ioctl(sock, SIOCSIFDSTADDR, &ifr) == 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				}
			} break;
			// Если адрес является IPv6
			case 16: {
				/**
				 * Назначаем адрес IPv6
				 *
				 * @details Запрос здесь тот же, что и у IPv4, - SIOCSIFADDR, - но описывается
				 *          он своей структурой: вместо имени устройства она несёт его номер, а
				 *          длина приставки задаётся прямо в ней, отдельного запроса маске не
				 *          требуется. Сокет для запроса обязан быть семейства IPv6
				 *
				 * @note Адрес другого конца у точки-точки для IPv6 средствами ioctl не
				 *       задаётся вовсе: у Linux его ставит подсистема маршрутов. Довод этот
				 *       здесь принимается и оставляется без применения - осознанно, а не по
				 *       недосмотру
				 *
				 */
				// Объект запроса сетевого интерфейса IPv6
				struct in6_ifreq ifr6{};
				// Получаем номер сетевого интерфейса по его имени
				ifr6.ifr6_ifindex = static_cast <int32_t> (::if_nametoindex(string(name).c_str()));
				// Если номер сетевого интерфейса получить не удалось
				if(ifr6.ifr6_ifindex == 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				// Устанавливаем IP-адрес интерфейса
				::memcpy(&ifr6.ifr6_addr, &awh_cast <const awh::net::addr_net_ipv6_t *> (ip)->address[0], 16);
				// Если префикс подсети больше 128 или равен 0
				if((prefix > 128) || (prefix == 0))
					// Устанавливаем длину приставки как /128
					ifr6.ifr6_prefixlen = 128;
				// Устанавливаем длину приставки подсети
				else ifr6.ifr6_prefixlen = prefix;
				/**
				 * Задаём адрес вместе с адресом встречной стороны
				 *
				 * @details Запрос ioctl несёт только свой адрес, и точка-точка у IPv6
				 *          остаётся без другого конца: ядро не знает, которым устройством
				 *          достигать встречную сторону, и обратный путь туннеля молчит.
				 *          Сокет управления сетью принимает оба адреса разом, и маршрут до
				 *          встречной стороны ядро заводит само - ровно как у систем BSD
				 *
				 * @warning Маршрут ядро заводит лишь на поднятом устройстве, оттого сам
				 *          маршрут тут НЕ задаётся: ядро поставит его, когда устройство
				 *          поднимут, а заведи мы его сами - получили бы отказ «сеть опущена»
				 */
				if((peer != nullptr) && (peer->size == 16)){
					/**
					 * @brief Структура сообщения назначения адреса
					 *
					 */
					struct {
						// Заголовок сообщения
						struct nlmsghdr header;
						// Описание адреса
						struct ifaddrmsg address;
						// Место под свойства адреса
						uint8_t buffer[64];
					} message{};
					// Устанавливаем размер сообщения
					message.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
					// Устанавливаем тип запроса
					message.header.nlmsg_type = RTM_NEWADDR;
					// Устанавливаем признаки запроса назначения с подтверждением
					message.header.nlmsg_flags = (NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK);
					// Устанавливаем порядковый номер запроса
					message.header.nlmsg_seq = 1;
					// Устанавливаем семейство адреса
					message.address.ifa_family = AF_INET6;
					// Устанавливаем длину приставки адреса
					message.address.ifa_prefixlen = ifr6.ifr6_prefixlen;
					// Устанавливаем номер устройства адреса
					message.address.ifa_index = static_cast <uint32_t> (ifr6.ifr6_ifindex);
					// Получаем место под свойство своего адреса
					struct rtattr * attribute = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (&message) + NLMSG_ALIGN(message.header.nlmsg_len));
					// Устанавливаем вид свойства
					attribute->rta_type = IFA_LOCAL;
					// Устанавливаем размер свойства
					attribute->rta_len = RTA_LENGTH(16);
					// Устанавливаем свой адрес устройства
					::memcpy(RTA_DATA(attribute), &awh_cast <const awh::net::addr_net_ipv6_t *> (ip)->address[0], 16);
					// Увеличиваем размер сообщения на размер свойства
					message.header.nlmsg_len = (NLMSG_ALIGN(message.header.nlmsg_len) + RTA_ALIGN(attribute->rta_len));
					// Получаем место под свойство адреса встречной стороны
					attribute = reinterpret_cast <struct rtattr *> (reinterpret_cast <uint8_t *> (&message) + NLMSG_ALIGN(message.header.nlmsg_len));
					// Устанавливаем вид свойства
					attribute->rta_type = IFA_ADDRESS;
					// Устанавливаем размер свойства
					attribute->rta_len = RTA_LENGTH(16);
					// Устанавливаем адрес встречной стороны
					::memcpy(RTA_DATA(attribute), &awh_cast <const awh::net::addr_net_ipv6_t *> (peer)->address[0], 16);
					// Увеличиваем размер сообщения на размер свойства
					message.header.nlmsg_len = (NLMSG_ALIGN(message.header.nlmsg_len) + RTA_ALIGN(attribute->rta_len));
					// Объект работы с ядром через сокет управления сетью
					const awh::gnu::netlink_t netlink(log);
					// Выполняем отправку сообщения ядру
					result = netlink.commit(&message, message.header.nlmsg_len);
				}
				/**
				 * Применяем новый IP-адрес интерфейса
				 *
				 * @note Запрос сюда доходит лишь тогда, когда встречной стороны нет либо
				 *       сокет управления сетью отказал: назначение с двумя адресами уже
				 *       сделано выше и повторять его запросом ioctl незачем
				 */
				if(!result && !(result = (::ioctl(sock, SIOCSIFADDR, &ifr6) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция создания клонируемого интерфейса
	 *
	 * @param driver имя драйвера интерфейса
	 * @param name   имя сетевого интерфейса
	 * @param log    объект работы с логами
	 * @return       дескриптор созданного сетевого интерфейса
	 *
	 */
	static awh::net::socket_t clonable(string_view driver, string & name, const awh::log_t * log) noexcept {
		// Если название драйвера передано
		if(!driver.empty()){
			/**
			 * Выполняем перехват ошибок
			 */
			try {
				// Создаём сокет для управления интерфейсом
				const awh::net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
				// Если создание сокета прошло неудачно
				if(sock == awh::net::invalid_socket_t){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(driver, name), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Возвращаем результат
					return awh::net::invalid_socket_t;
				}
				/**
				 * Заводим устройство сообщением ядру
				 *
				 * @details Клонирующих драйверов, которыми это делается у BSD запросом
				 *          SIOCIFCREATE, у Linux нет вовсе: устройство заводится сообщением
				 *          RTM_NEWLINK с указанием рода, а имя драйвера родом и служит
				 *
				 * @note Сокет управления здесь всё равно нужен: наружу метод отдаёт именно
				 *       его, и потребитель настраивает через него заведённое устройство
				 */
				// Выполняем инициализацию объекта опроса ядра
				const awh::gnu::netlink_t netlink(log);
				// Структура запроса
				struct ifreq ifr{};
				// Если имя интерфейса задано
				if(!name.empty()){
					// Копируем имя интерфейса
					::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
					// Устанавливаем завершающий ноль
					ifr.ifr_name[IFNAMSIZ - 1] = '\0';
					// Создаём интерфейс
					if(!netlink.link(name, driver)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("%s", __PRETTY_FUNCTION__, make_tuple(driver, name), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return awh::net::invalid_socket_t;
					}
					// Возвращаем сокет созданного интерфейса
					return sock;
				// Если имя интерфейса не задано
				} else {
					/**
					 * Перебираем возможные индексы
					 */
					for(size_t i = 0; i < 128; ++i){
						// Формируем имя интерфейса
						::snprintf(ifr.ifr_name, IFNAMSIZ, "%s%zu", string(driver).c_str(), i);
						// Пытаемся создать интерфейс
						if(netlink.link(ifr.ifr_name, driver)){
							// Сохраняем имя
							name = ifr.ifr_name;
							// Возвращаем сокет
							return sock;
						}
						// Если ошибка не EEXIST, прерываем
						if(errno != EEXIST)
							// Прерываем цикл
							break;
					}
				}
				// Закрываем сокет и возвращаем ошибку
				::close(sock);
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(driver, name), awh::log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
		// Возвращаем значение по умолчанию
		return awh::net::invalid_socket_t;
	};
};

/**
 * @brief Метод удаления сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат удаления сетевого интерфейса
 *
 */
bool awh::eth::Interface::destroy(string_view name) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого интерфейса передано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём сокет для управления интерфейсом
			net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(!(result = (sock != net::invalid_socket_t))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			/**
			 * Снимаем устройство сообщением ядру
			 *
			 * @note Запроса SIOCIFDESTROY у Linux нет: снятие ведётся сообщением
			 *       RTM_DELLINK, тем же путём, что и заведение
			 */
			// Выполняем инициализацию объекта опроса ядра
			const gnu::netlink_t netlink(this->_log);
			// Удаляем интерфейс
			if(!(result = netlink.unlink(name))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
			// Закрываем сокет
			::close(sock);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения списка сетевых интерфейсов системы
 *
 * @return список сетевых интерфейсов системы
 *
 */
unordered_set <string> awh::eth::Interface::available() const noexcept {
	// Переменная результата
	unordered_set <string> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем список сетевых интерфейсов
		struct ifaddrs * ptr = nullptr;
		// Выполняем получение списка сетевых интерфейсов
		if(::getifaddrs(&ptr) != 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
			#endif
			// Возвращаем пустой результат
			return result;
		}
		// Гарантируем освобождение списка интерфейсов при любом выходе
		const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
		/**
		 * Перебираем все сетевые интерфейсы
		 */
		for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next)
			// Добавляем имя сетевого интерфейса в результирующий список
			result.emplace(ifa->ifa_name);
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
	// Выводиим результат
	return result;
}
/**
 * @brief Метод проверки доступности сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки доступности сетевого интерфейса
 *
 */
bool awh::eth::Interface::isAvailable(string_view name) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если название сетевого интерфейса передано
		if(!name.empty()){
			// Получаем список сетевых интерфейсов
			struct ifaddrs * ptr = nullptr;
			// Выполняем получение списка сетевых интерфейсов
			if(::getifaddrs(&ptr) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
				#endif
				// Возвращаем пустой результат
				return result;
			}
			// Гарантируем освобождение списка интерфейсов при любом выходе
			const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
			/**
			 * Перебираем все сетевые интерфейсы
			 */
			for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next)
				// Добавляем имя сетевого интерфейса в результирующий список
				if((result = this->_fmk->compare(ifa->ifa_name, name)))
					// Завершаем поиск
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводиим результат
	return result;
}
/**
 * @brief Метод проверки туннельного сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки туннельного сетевого интерфейса
 *
 */
bool awh::eth::Interface::isTunnel(string_view name) const noexcept {
	// Переменная результата
	bool result = false;
	// Если имя интерфейса задано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем список сетевых интерфейсов
			struct ifaddrs * ptr = nullptr;
			// Выполняем получение списка сетевых интерфейсов
			if(::getifaddrs(&ptr) == 0){
				// Гарантируем освобождение списка интерфейсов при любом выходе
				const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
				// Выполняем классификацию интерфейса как туннельного
				result = ::device::classify(ptr, name, false, this->_fmk);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки туннельного сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     результат проверки туннельного сетевого интерфейса
 *
 */
bool awh::eth::Interface::isTunnel(const net::addr_t * addr) const noexcept {
	// Переменная результата
	bool result = false;
	// Если адрес передан
	if(addr != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем список сетевых интерфейсов
			struct ifaddrs * ptr = nullptr;
			// Выполняем получение списка сетевых интерфейсов (единственный проход)
			if(::getifaddrs(&ptr) == 0){
				// Гарантируем освобождение списка интерфейсов при любом выходе
				const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
				// Определяем имя интерфейса по адресу в полученном списке
				const string name = ::device::findNameByAddr(ptr, addr);
				// Если имя интерфейса найдено, выполняем классификацию по тому же списку
				if(!name.empty())
					// Выполняем классификацию интерфейса как туннельного
					result = ::device::classify(ptr, name, false, this->_fmk);
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
 * @brief Метод проверки виртуального сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки виртуального сетевого интерфейса
 *
 */
bool awh::eth::Interface::isVirtual(string_view name) const noexcept {
	// Переменная результата
	bool result = false;
	// Если имя интерфейса задано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем список сетевых интерфейсов
			struct ifaddrs * ptr = nullptr;
			// Выполняем получение списка сетевых интерфейсов
			if(::getifaddrs(&ptr) == 0){
				// Гарантируем освобождение списка интерфейсов при любом выходе
				const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
				// Выполняем классификацию интерфейса как виртуального
				result = ::device::classify(ptr, name, true, this->_fmk);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки виртуального сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     результат проверки виртуального сетевого интерфейса
 *
 */
bool awh::eth::Interface::isVirtual(const net::addr_t * addr) const noexcept {
	// Переменная результата
	bool result = false;
	// Если адрес передан
	if(addr != nullptr){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем список сетевых интерфейсов
			struct ifaddrs * ptr = nullptr;
			// Выполняем получение списка сетевых интерфейсов (единственный проход)
			if(::getifaddrs(&ptr) == 0){
				// Гарантируем освобождение списка интерфейсов при любом выходе
				const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
				// Определяем имя интерфейса по адресу в полученном списке
				const string name = ::device::findNameByAddr(ptr, addr);
				// Если имя интерфейса найдено, выполняем классификацию по тому же списку
				if(!name.empty())
					// Выполняем классификацию интерфейса как виртуального
					result = ::device::classify(ptr, name, true, this->_fmk);
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
 * @brief Метод получения имени сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     имя сетевого интерфейса
 *
 */
string awh::eth::Interface::name(const net::addr_t * addr) const noexcept {
	// Переменная результата
	string result = "";
	// Если адрес не передан
	if(addr == nullptr)
		// Возвращаем пустой результат
		return result;
	/**
	 * Неопределённый адрес не называет НИЧЕГО
	 *
	 * @details Нулевой адрес - это `INADDR_ANY`, «любой», а не адрес какого-то
	 *          устройства. Искать его среди связей нельзя: система выдаёт нулевой
	 *          адрес IPv4 всякой связи, у которой адреса ещё нет, и поиск возвращал
	 *          имя первой попавшейся из них
	 *
	 * @warning Прежде этого разбора не было, и держалось всё лишь на том, что у машины
	 *          не случалось связей без адреса. Установлено 23.08.2026 на стендах
	 *          Solaris и OpenIndiana: там заведены связи `awh_tun0`...`awh_tun7` под
	 *          туннели, и `EthSuiteTest` падал на обеих машинах, требуя от нулевого
	 *          адреса пустого имени
	 */
	switch(addr->size){
		// Если адрес является MAC-адресом
		case 6: {
			// Признак нулевого аппаратного адреса
			bool empty = true;
			/**
			 * Проходим по всем октетам аппаратного адреса
			 */
			for(uint8_t i = 0; (i < 6) && empty; i++)
				// Снимаем признак у первого ненулевого октета
				empty = (awh_cast <const awh::net::addr_mac_t *> (addr)->address[i] == 0);
			// Если аппаратный адрес нулевой, возвращаем пустой результат
			if(empty)
				// Выводим пустой результат
				return result;
		} break;
		// Если адрес является IPv4
		case 4: {
			// Если адрес нулевой, возвращаем пустой результат
			if(awh_cast <const awh::net::addr_net_ipv4_t *> (addr)->address == 0)
				// Выводим пустой результат
				return result;
		} break;
		// Если адрес является IPv6
		case 16: {
			// Признак нулевого адреса IPv6
			bool empty = true;
			/**
			 * Проходим по всем октетам адреса IPv6
			 */
			for(uint8_t i = 0; (i < 16) && empty; i++)
				// Снимаем признак у первого ненулевого октета
				empty = (awh_cast <const awh::net::addr_net_ipv6_t *> (addr)->address[i] == 0);
			// Если адрес нулевой, возвращаем пустой результат
			if(empty)
				// Выводим пустой результат
				return result;
		} break;
	}
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Получаем список сетевых интерфейсов
		struct ifaddrs * ptr = nullptr;
		// Выполняем получение списка сетевых интерфейсов
		if(::getifaddrs(&ptr) != 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
			#endif
			// Возвращаем пустой результат
			return result;
		}
		// Гарантируем освобождение списка интерфейсов при любом выходе
		const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
		// Определяем имя интерфейса по адресу в полученном списке
		result = ::device::findNameByAddr(ptr, addr);
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
	// Возвращаем пустой результат
	return result;
}
/**
 * @brief Метод создания сетевого интерфейса
 *
 * @param type тип сетевого интерфейса
 * @param name имя сетевого интерфейса
 * @return     дескриптор созданного сетевого интерфейса
 *
 */
awh::net::socket_t awh::eth::Interface::create(const event::eth_t type, string & name) const noexcept {
	// Переменная результата
	net::socket_t result = net::invalid_socket_t;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип создаваемого интерфейса
		 */
		switch(static_cast <uint8_t> (type)){
			// Если создаётся прямое подключение к сети через драйвер сетевой карты
			case static_cast <uint8_t> (event::eth_t::NET): {
				// Если имя интерфейса не задано, пытаемся его определить
				if(name.empty()){
					// Получаем список интерфейсов
					struct ifaddrs * ifap = nullptr;
					// Если список получен успешно
					if(::getifaddrs(&ifap) == 0){
						// Гарантируем освобождение списка интерфейсов при любом выходе
						const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ifap, &::freeifaddrs);
						/**
						 * Перебираем интерфейсы
						 */
						for(struct ifaddrs * ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем интерфейсы без записи канального уровня
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_PACKET))
								// Переходим к следующему интерфейсу
								continue;
							// Получаем структуру адреса канального уровня
							struct sockaddr_ll * sll = reinterpret_cast <struct sockaddr_ll *> (ifa->ifa_addr);
							// Проверяем тип интерфейса (должен быть Ethernet)
							if(sll->sll_hatype == ARPHRD_ETHER){
								// Проверяем флаги: не петля, и (активный или можно активировать)
								if(!(ifa->ifa_flags & IFF_LOOPBACK)){
									// Сохраняем имя
									name = ifa->ifa_name;
									// Прерываем поиск
									break;
								}
							}
						}
					}
				}
				// Если имя интерфейса определено
				if(!name.empty()){
					// Создаём сокет для управления для поднятия интерфейса
					result = ::socket(AF_INET, SOCK_DGRAM, 0);
					// Если сокет управления создан
					if(result != net::invalid_socket_t){
						// Структура запроса
						struct ifreq ifr{};
						// Копируем имя интерфейса
						::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
						// Устанавливаем завершающий ноль
						ifr.ifr_name[IFNAMSIZ - 1] = '\0';
						// Получаем текущие флаги
						if(::ioctl(result, SIOCGIFFLAGS, &ifr) == 0){
							// Устанавливаем флаг UP
							ifr.ifr_flags |= IFF_UP;
							// Применяем флаги
							::ioctl(result, SIOCSIFFLAGS, &ifr);
						}
						// Закрываем сокет управления
						::close(result);
					}
					/**
					 * Заводим сокет чтения кадров канального уровня
					 *
					 * @details Устройств `/dev/bpf*`, которыми это делается у BSD и macOS, у
					 *          Linux нет вовсе. Взамен заводится сокет семейства AF_PACKET и
					 *          привязывается к устройству по его номеру - тем же вызовом
					 *          привязки, что и у обычного сокета
					 *
					 * @note Немедленной выдачи, которую у BSD приходится включать отдельным
					 *       запросом, здесь включать не нужно: сокет отдаёт каждый кадр сразу
					 *       по приходу, накопления кадров у Linux нет
					 *
					 * @warning Права на такой сокет нужны надзорные, либо возможность
					 *          CAP_NET_RAW: чтение кадров минует всю обработку сети
					 *
					 */
					// Создаём сокет чтения кадров канального уровня
					result = ::socket(AF_PACKET, SOCK_RAW, static_cast <int32_t> (htons(ETH_P_ALL)));
					// Если сокет чтения кадров создан
					if(result != net::invalid_socket_t){
						// Структура привязки сокета к устройству
						struct sockaddr_ll sll{};
						// Устанавливаем семейство канального уровня
						sll.sll_family = AF_PACKET;
						// Устанавливаем разбор кадров любого протокола
						sll.sll_protocol = htons(ETH_P_ALL);
						// Получаем номер сетевого интерфейса по его имени
						sll.sll_ifindex = static_cast <int32_t> (::if_nametoindex(name.c_str()));
						// Если номер устройства получен, выполняем привязку к нему
						if((sll.sll_ifindex == 0) || (::bind(result, reinterpret_cast <struct sockaddr *> (&sll), sizeof(sll)) != 0)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
							#endif
							// Закрываем сокет чтения кадров
							::close(result);
							// Сбрасываем результат
							result = net::invalid_socket_t;
						}
					}
				}
			} break;
			// Если необходимо создать интерфейс передачи сырых IP-пакетов
			case static_cast <uint8_t> (event::eth_t::TUN):
			// Если необходимо создать интерфейс передачи кадров Ethernet
			case static_cast <uint8_t> (event::eth_t::TAP): {
				/**
				 * Заводим устройство передачи
				 *
				 * @details У Linux оба вида устройств заводятся одним и тем же путём: через
				 *          управляющее устройство `/dev/net/tun`, а вид задаётся признаком в
				 *          запросе - IFF_TUN для сырых IP-пакетов, IFF_TAP для кадров
				 *          Ethernet. Отдельных устройств `/dev/tunN` и `/dev/tapN`, как у
				 *          BSD, здесь нет вовсе, и перебор свободного номера не нужен
				 *
				 * @note Имя интерфейса ядро выдаёт само, когда в запросе оставлено пустое
				 *       место: подставляется первое свободное - tun0, tap0 и далее. Имя это
				 *       ядро записывает обратно в тот же запрос, оттуда оно и берётся
				 *
				 * @warning Признак IFF_NO_PI выставляется намеренно: без него ядро
				 *          предваряет каждый пакет своим четырёхоктетным заголовком, и
				 *          вычитанное перестаёт быть тем, чем его считает вызывающий -
				 *          пакетом либо кадром
				 *
				 */
				// Открываем управляющее устройство
				result = ::open("/dev/net/tun", O_RDWR);
				// Если управляющее устройство открыть не удалось
				if(result == net::invalid_socket_t){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Возвращаем результат
					return result;
				}
				// Создаём объект запроса сетевого интерфейса
				struct ifreq ifr{};
				// Устанавливаем вид заводимого устройства
				ifr.ifr_flags = ((type == event::eth_t::TUN) ? IFF_TUN : IFF_TAP);
				// Убираем служебный заголовок ядра перед каждым пакетом
				ifr.ifr_flags |= IFF_NO_PI;
				// Если имя интерфейса передано, задаём его ядру
				if(!name.empty())
					// Устанавливаем название сетевого интерфейса
					::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
				// Заводим сетевой интерфейс
				if(::ioctl(result, TUNSETIFF, &ifr) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Закрываем описатель устройства
					::close(result);
					// Возвращаем результат
					return net::invalid_socket_t;
				}
				// Запоминаем имя интерфейса, выданное ядром
				name = ifr.ifr_name;
			} break;
			// Если создаётся общий туннельный интерфейс (IPv6-in-IPv4, IPv4-in-IPv6, IPv6-in-IPv6)
			case static_cast <uint8_t> (event::eth_t::GIF):
				// Создаём клонирующий интерфейс GIF
				result = ::device::clonable("gif", name, this->_log);
			break;
			// Если создаётся GRE-туннель (включая с ключом)
			case static_cast <uint8_t> (event::eth_t::GRE):
				// Создаём клонирующий интерфейс GRE
				result = ::device::clonable("gre", name, this->_log);
			break;
			// Если создаётся беспроводной интерфейс
			case static_cast <uint8_t> (event::eth_t::WLAN):
				// Создаём клонирующий интерфейс WLAN
				result = ::device::clonable("wlan", name, this->_log);
			break;
			// Если создаётся интерфейс логической сегментации на основе 802.1Q
			case static_cast <uint8_t> (event::eth_t::VLAN):
				// Создаём клонирующий интерфейс VLAN
				result = ::device::clonable("vlan", name, this->_log);
			break;
			// Если создаётся интерфейс агрегации каналов
			case static_cast <uint8_t> (event::eth_t::BOND):
				/**
				 * Создаём клонирующий интерфейс агрегации каналов
				 *
				 * @warning Род тут «bond», а НЕ «lagg»: агрегацию каналов Linux зовёт
				 *          своим именем, и «lagg» - имя из наречия BSD. Устройство
				 *          заводится сообщением RTM_NEWLINK, где имя драйвера служит
				 *          родом, и род «lagg» ядро Linux отвергает всегда: такого
				 *          рода у него нет вовсе. Проверено опросом ядра 6.1 на
				 *          стенде Debian 31.08.2026 - «bond» в перечне родов есть,
				 *          «lagg» отсутствует
				 */
				result = ::device::clonable("bond", name, this->_log);
			break;
			// Если создаётся интерфейс объединения интерфейсов на уровне L2
			case static_cast <uint8_t> (event::eth_t::BRIDGE):
				// Создаём клонирующий интерфейс Bridge
				result = ::device::clonable("bridge", name, this->_log);
			break;
			// Если создаётся неизвестный тип интерфейса
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unsupported network interface type", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unsupported network interface type", log_t::flag_t::WARNING);
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     MTU сетевого интерфейса
 *
 */
uint32_t awh::eth::Interface::mtu(string_view name) const noexcept {
	// Если название сетевого интерфейса передано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём сокет для управления интерфейсом
			net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(sock == net::invalid_socket_t){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем значение по умолчанию
				return 0;
			}
			// Настраиваем интерфейс
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::device::copyName(ifr.ifr_name, name);
			// Извлекаем MTU из интерфейса
			if(::ioctl(sock, SIOCGIFMTU, &ifr) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Закрываем сокет
				::close(sock);
				// Возвращаем значение по умолчанию
				return 0;
			}
			// Закрываем сокет
			::close(sock);
			// Возвращаем результат
			return static_cast <uint32_t> (ifr.ifr_mtu);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @param mtu  размер MTU интерфейса
 * @return     результат установки MTU сетевого интерфейса
 *
 */
bool awh::eth::Interface::mtu(string_view name, const uint32_t mtu) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого интерфейса передано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём сокет для управления интерфейсом
			net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(!(result = (sock != net::invalid_socket_t))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Настраиваем интерфейс
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::device::copyName(ifr.ifr_name, name);
			// Если не удалось получить флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCGIFFLAGS, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Закрываем сокет
				::close(sock);
				// Возвращаем результат
				return result;
			}
			// Устанавливаем MTU интерфейса
			ifr.ifr_mtu = static_cast <int32_t> (mtu);
			// Применяем новый MTU интерфейса
			if(!(result = (::ioctl(sock, SIOCSIFMTU, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
			// Закрываем сокет
			::close(sock);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, mtu), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения установленных флагов сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     флаги сетевого интерфейса
 *
 */
unordered_set <awh::event::eth_flag_t> awh::eth::Interface::flags(string_view name) const noexcept {
	// Переменная результата
	unordered_set <event::eth_flag_t> result;
	// Если название сетевого интерфейса передано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём сокет для управления интерфейсом
			net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(sock == net::invalid_socket_t){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Настраиваем интерфейс
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::device::copyName(ifr.ifr_name, name);
			// Если не удалось получить флаги интерфейса
			if(::ioctl(sock, SIOCGIFFLAGS, &ifr) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Закрываем сокет
				::close(sock);
				// Возвращаем результат
				return result;
			}
			// Закрываем сокет
			::close(sock);
			// Если сетевой интерфейс в режиме поднят
			if(ifr.ifr_flags & IFF_UP)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::UP);
			// Если сетевой интерфейс принимает все multicast-пакеты
			if(ifr.ifr_flags & IFF_ALLMULTI)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::ALLMULTI);
			// Если сетевой интерфейс поддерживает broadcast
			if(ifr.ifr_flags & IFF_BROADCAST)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::BROADCAST);
			// Если сетевой интерфейс в режиме debug
			if(ifr.ifr_flags & IFF_DEBUG)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::DEBUG);
			// Если сетевой интерфейс поддерживает multicast
			if(ifr.ifr_flags & IFF_MULTICAST)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::MULTICAST);
			// Если сетевой интерфейс отключил ARP
			if(ifr.ifr_flags & IFF_NOARP)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::NOARP);
			// Если сетевой интерфейс в режиме запущен
			if(ifr.ifr_flags & IFF_RUNNING)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::RUNNING);
			// Если сетевой интерфейс в режиме promiscuous
			if(ifr.ifr_flags & IFF_PROMISC)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::PROMISC);
			// Если сетевой интерфейс является loopback интерфейсом
			if(ifr.ifr_flags & IFF_LOOPBACK)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::LOOPBACK);
			// Если сетевой интерфейс является point-to-point интерфейсом
			if(ifr.ifr_flags & IFF_POINTOPOINT)
				// Добавляем флаг интерфейса в результат
				result.emplace(event::eth_flag_t::POINTTOPOINT);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки флага сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @param flag флаг сетевого интерфейса
 * @param mode режим включения/выключения флага
 * @return     результат установки флага сетевого интерфейса
 *
 */
bool awh::eth::Interface::flag(string_view name, const event::eth_flag_t flag, const event::mode_t mode) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого интерфейса передано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём сокет для управления интерфейсом
			net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(!(result = (sock != net::invalid_socket_t))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Настраиваем интерфейс
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::device::copyName(ifr.ifr_name, name);
			// Если не удалось получить флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCGIFFLAGS, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Закрываем сокет
				::close(sock);
				// Возвращаем результат
				return result;
			}
			/**
			 * Устанавливаем или снимаем флаг интерфейса
			 */
			switch(static_cast <uint8_t> (flag)){
				// Если нужно установить флаг поднятия интерфейса
				case static_cast <uint8_t> (event::eth_flag_t::UP): {
					/**
					 * Определяем режим работы интерфейса
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо включить интерфейс
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем флаг интерфейса
							ifr.ifr_flags |= IFF_UP;
						break;
						// Если необходимо выключить интерфейс
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Снимаем флаг интерфейса
							ifr.ifr_flags &= ~IFF_UP;
						break;
					}
				} break;
				// Если нужно установить флаг promiscuous режима
				case static_cast <uint8_t> (event::eth_flag_t::PROMISC): {
					/**
					 * Определяем режим работы интерфейса
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо включить интерфейс
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем флаг интерфейса
							ifr.ifr_flags |= IFF_PROMISC;
						break;
						// Если необходимо выключить интерфейс
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Снимаем флаг интерфейса
							ifr.ifr_flags &= ~IFF_PROMISC;
						break;
					}
				} break;
				// Если нужно установить флаг отключения ARP
				case static_cast <uint8_t> (event::eth_flag_t::NOARP): {
					/**
					 * Определяем режим работы интерфейса
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо включить интерфейс
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем флаг интерфейса
							ifr.ifr_flags |= IFF_NOARP;
						break;
						// Если необходимо выключить интерфейс
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Снимаем флаг интерфейса
							ifr.ifr_flags &= ~IFF_NOARP;
						break;
					}
				} break;
				// Если нужно установить флаг debug режима
				case static_cast <uint8_t> (event::eth_flag_t::DEBUG): {
					/**
					 * Определяем режим работы интерфейса
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо включить интерфейс
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем флаг интерфейса
							ifr.ifr_flags |= IFF_DEBUG;
						break;
						// Если необходимо выключить интерфейс
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Снимаем флаг интерфейса
							ifr.ifr_flags &= ~IFF_DEBUG;
						break;
					}
				} break;
				// Если нужно установить флаг приёма всех multicast-пакетов
				case static_cast <uint8_t> (event::eth_flag_t::ALLMULTI): {
					/**
					 * Определяем режим работы интерфейса
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо включить интерфейс
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем флаг интерфейса
							ifr.ifr_flags |= IFF_ALLMULTI;
						break;
						// Если необходимо выключить интерфейс
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Снимаем флаг интерфейса
							ifr.ifr_flags &= ~IFF_ALLMULTI;
						break;
					}
				} break;
				// Если флаг не поддерживается
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Passed network interface flag cannot be modified", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Passed network interface flag cannot be modified", log_t::flag_t::WARNING);
					#endif
					// Закрываем сокет
					::close(sock);
					// Возвращаем результат
					return result;
				}
			}
			// Применяем новые флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCSIFFLAGS, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
			/**
			 * Заводим маршруты до встречных сторон поднятого устройства
			 *
			 * @note Порядок тут обратный привычному: адреса на устройство ставит движок
			 *       ЗАРАНЕЕ, а поднимает его потребитель, и ядру заводить маршрут в тот
			 *       миг ещё не по чему. Наверстываем это здесь, сразу по поднятии
			 */
			if(result && (flag == event::eth_flag_t::UP) && (mode == event::mode_t::ENABLED))
				// Заводим маршруты до встречных сторон устройства
				::device::routePeers(name, this->_log);
			// Закрываем сокет
			::close(sock);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки IP-адреса на сетевой интерфейс
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для установки
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат установки IP-адреса
 *
 */
bool awh::eth::Interface::setAddress(string_view name, const net::addr_t * ip, const uint8_t prefix) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого интерфейса и адрес для установки переданы
	if(!name.empty() && (ip != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём сокет для управления интерфейсом
			/**
			 * Семейство управляющего сокета следует виду адреса
			 *
			 * @warning Запрос назначения адреса IPv6 сокет семейства IPv4 отвергает
			 *          с EINVAL: адрес на устройство не встаёт вовсе, а отказ выглядит
			 *          неверным доводом запроса
			 */
			net::socket_t sock = ::socket(((ip->size == 16) ? AF_INET6 : AF_INET), SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(!(result = (sock != net::invalid_socket_t))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Применяем IP-адрес и маску интерфейса через управляющий сокет
			result = ::device::applyAddress(sock, name, ip, nullptr, prefix, this->_log);
			// Закрываем сокет
			::close(sock);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения IP-адреса сетевого интерфейса
 *
 * @param name   имя сетевого интерфейса
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       IP-адрес сетевого интерфейса
 *
 */
unique_ptr <awh::net::addr_t> awh::eth::Interface::getAddress(string_view name, const event::family_t family) const noexcept {
	// Переменная результата
	unique_ptr <awh::net::addr_t> result = nullptr;
	// Если название сетевого интерфейса передано
	if(!name.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем список сетевых интерфейсов
			struct ifaddrs * ptr = nullptr;
			// Выполняем получение списка сетевых интерфейсов
			if(::getifaddrs(&ptr) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
				#endif
				// Выходим из функции
				return result;
			}
			// Гарантируем освобождение списка интерфейсов при любом выходе
			const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
			/**
			 * Перебираем все сетевые интерфейсы
			 */
			for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
				// Пропускаем не IPv4-интерфейсы
				if(ifa->ifa_addr == nullptr)
					// Пропускаем интерфейсы, которые не являются IPv4
					continue;
				/**
				 * Определяем семейство протоколов
				 */
				switch(static_cast <uint8_t> (family)){
					// Если необходимо получить IPv4-адрес
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Пропускаем не IPv4-интерфейсы
						if(ifa->ifa_addr->sa_family != AF_INET)
							// Переходим к следующему интерфейсу
							continue;
					break;
					// Если необходимо получить IPv6-адрес
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Пропускаем не IPv6-интерфейсы
						if(ifa->ifa_addr->sa_family != AF_INET6)
							// Переходим к следующему интерфейсу
							continue;
					break;
				}
				// Если интерфейс не активен
				if(!(ifa->ifa_flags & IFF_UP))
					// Пропускаем неактивные интерфейсы
					continue;
				// Если имя интерфейса совпадает
				if(this->_fmk->compare(ifa->ifa_name, name)){
					/**
					 * Определяем тип адреса интерфейса
					 */
					switch(ifa->ifa_addr->sa_family){
						// Если интерфейс является IPv4
						case AF_INET: {
							// Создаём объект для хранения IPv4-адреса
							result = make_unique <net::addr_net_ipv4_t> ();
							// Копируем IP-адрес в результат
							awh_cast <net::addr_net_ipv4_t *> (result.get())->address = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr)->sin_addr.s_addr;
							// Завершаем поиск (для IPv4 берём первый найденный адрес)
							goto End;
						}
						// Если интерфейс является IPv6
						case AF_INET6: {
							// Получаем структуру IPv6-адреса
							struct sockaddr_in6 * sin6 = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
							// Определяем, является ли адрес Link-Local
							const bool isLinkLocal = IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr);
							// Если результат ещё не установлен или найден глобальный адрес (перезаписываем Link-Local)
							if((result == nullptr) || !isLinkLocal){
								// Создаём объект для хранения IPv6-адреса
								result = make_unique <net::addr_net_ipv6_t> ();
								// Копируем IP-адрес в результат
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (result.get())->address[0], &sin6->sin6_addr, sizeof(in6_addr));
								// Если найден глобальный адрес, то это наш лучший выбор
								if(!isLinkLocal)
									// Завершаем поиск
									goto End;
							}
						} break;
						// В остальных случаях пропускаем интерфейс
						default: continue;
					}
				}
			}
			/**
			 * Метка завершения поиска
			 */
			End:;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки параметров сетевого интерфейса точка-точка
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для установки
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат установки параметров сетевого интерфейса точка-точка
 *
 */
bool awh::eth::Interface::setAddress(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого интерфейса и адреса для установки переданы
	if(!name.empty() && (ip != nullptr) && (peer != nullptr) && (ip->size == peer->size)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём сокет для управления интерфейсом
			/**
			 * Семейство управляющего сокета следует виду адреса
			 *
			 * @warning Запрос назначения адреса IPv6 сокет семейства IPv4 отвергает
			 *          с EINVAL: адрес на устройство не встаёт вовсе, а отказ выглядит
			 *          неверным доводом запроса
			 */
			net::socket_t sock = ::socket(((ip->size == 16) ? AF_INET6 : AF_INET), SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(!(result = (sock != net::invalid_socket_t))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			// Применяем IP-адрес, маску и адрес удалённого пира интерфейса через управляющий сокет
			result = ::device::applyAddress(sock, name, ip, peer, prefix, this->_log);
			// Закрываем сокет
			::close(sock);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод изменения параметров сетевого интерфейса точка-точка
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для получения
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат изменения параметров сетевого интерфейса точка-точка
 *
 */
bool awh::eth::Interface::getAddress(string_view name, unique_ptr <net::addr_t> & ip, unique_ptr <net::addr_t> & peer, uint8_t & prefix) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого интерфейса передано
	if(!name.empty() && (ip != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Список сетевых интерфейсов
			struct ifaddrs * ptr = nullptr;
			// Выполняем получение списка сетевых интерфейсов
			if(::getifaddrs(&ptr) == 0){
				// Гарантируем освобождение списка интерфейсов при любом выходе
				const unique_ptr <struct ifaddrs, void (*)(struct ifaddrs *)> guard(ptr, &::freeifaddrs);
				/**
				 * Перебираем все сетевые интерфейсы
				 */
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Если интерфейс не имеет адреса или имя не совпадает
					if((ifa->ifa_addr == nullptr) || !this->_fmk->compare(ifa->ifa_name, name))
						// Переходим к следующему
						continue;
					/**
					 * Определяем тип адреса который мы ищем
					 */
					switch(ip->size){
						// Если ищем IPv4
						case 4: {
							// Если семейство адресов совпадает
							if(ifa->ifa_addr->sa_family == AF_INET){
								// Преобразуем адрес интерфейса
								struct sockaddr_in * sin = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr);
								// Извлекаем IP-адрес интерфейса
								awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = sin->sin_addr.s_addr;
								// Если адрес удалённого пира доступен (P2P интерфейс)
								if((peer != nullptr) && (ifa->ifa_dstaddr != nullptr) && (ifa->ifa_dstaddr->sa_family == AF_INET)){
									// Получаем адрес пира
									struct sockaddr_in * dst = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_dstaddr);
									// Извлекаем IP-адрес удалённого пира
									awh_cast <net::addr_net_ipv4_t *> (peer.get())->address = dst->sin_addr.s_addr;
								}
								// Если маска подсети доступна
								if(ifa->ifa_netmask != nullptr){
									// Преобразуем маску
									struct sockaddr_in * msk = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_netmask);
									// Извлекаем префикс подсети
									prefix = ::device::mask2prefix(msk->sin_addr);
								// Если маска подсети недоступна, устанавливаем префикс /32
								} else prefix = 32;
								// Устанавливаем флаг успеха
								result = true;
								// Прерываем поиск (для IPv4 берем первый попавшийся)
								goto End;
							}
						} break;
						// Если ищем IPv6
						case 16: {
							// Если семейство адресов совпадает
							if(ifa->ifa_addr->sa_family == AF_INET6){
								// Получаем адрес интерфейса
								struct sockaddr_in6 * sin6 = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
								// Если это не Link-Local адрес, или мы еще не нашли никакого адреса
								bool isLinkLocal = IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr);
								// Если результат еще не установлен или мы нашли глобальный адрес (перезаписываем Link-Local)
								if(!result || !isLinkLocal){
									// Копируем IP-адрес
									::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &sin6->sin6_addr, sizeof(in6_addr));
									// Сбрасываем префикс перед расчетом
									prefix = 0;
									// Если маска подсети доступна
									if(ifa->ifa_netmask != nullptr){
										// Получаем маску подсети
										struct sockaddr_in6 * msk6 = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_netmask);
										/**
										 * Вычисляем префикс
										 */
										for(size_t i = 0; i < 16; ++i){
											// Получаем байт маски
											uint8_t byte = msk6->sin6_addr.s6_addr[i];
											// Считаем биты
											while(byte & 0x80){
												// Увеличиваем префикс
												prefix++;
												// Сдвигаем байт
												byte <<= 1;
											}
										}
									// Если маска подсети недоступна, устанавливаем префикс /128
									} else prefix = 128;
									// Если пир задан и доступен
									if((peer != nullptr) && (ifa->ifa_dstaddr != nullptr) && (ifa->ifa_dstaddr->sa_family == AF_INET6)){
										// Получаем адрес пира
										struct sockaddr_in6 * dst6 = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_dstaddr);
										// Копируем адрес пира
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (peer.get())->address[0], &dst6->sin6_addr, sizeof(in6_addr));
									}
									// Устанавливаем флаг успеха
									result = true;
									// Если найден глобальный адрес, то это наш лучший выбор
									if(!isLinkLocal)
										// Прерываем поиск
										goto End;
								}
							}
						} break;
					}
				}
				/**
				 * Метка завершения поиска
				 */
				End:;
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод комплексной настройки сетевого интерфейса (адрес + MTU + поднятие) за один управляющий сокет
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для установки
 * @param prefix префикс подсети
 * @param mtu    размер MTU интерфейса (0 - не изменять)
 * @return       результат комплексной настройки сетевого интерфейса
 *
 */
bool awh::eth::Interface::configure(string_view name, const net::addr_t * ip, const uint8_t prefix, const uint32_t mtu) const noexcept {
	// Делегируем выполнение комплексной настройке без адреса удалённого пира
	return this->configure(name, ip, nullptr, prefix, mtu);
}
/**
 * @brief Метод комплексной настройки сетевого интерфейса точка-точка (адрес + пир + MTU + поднятие) за один управляющий сокет
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для установки
 * @param peer   адрес удалённого пира (для точка-точка) либо nullptr
 * @param prefix префикс подсети
 * @param mtu    размер MTU интерфейса (0 - не изменять)
 * @return       результат комплексной настройки сетевого интерфейса
 *
 */
bool awh::eth::Interface::configure(string_view name, const net::addr_t * ip, const net::addr_t * peer, const uint8_t prefix, const uint32_t mtu) const noexcept {
	// Переменная результата
	bool result = false;
	// Если имя интерфейса и адрес переданы, а адрес пира (если задан) совпадает по типу
	if(!name.empty() && (ip != nullptr) && ((peer == nullptr) || (ip->size == peer->size))){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Создаём единственный управляющий сокет для всех операций
			/**
			 * Семейство управляющего сокета следует виду адреса
			 *
			 * @warning Запрос назначения адреса IPv6 сокет семейства IPv4 отвергает
			 *          с EINVAL: адрес на устройство не встаёт вовсе, а отказ выглядит
			 *          неверным доводом запроса
			 */
			net::socket_t sock = ::socket(((ip->size == 16) ? AF_INET6 : AF_INET), SOCK_DGRAM, 0);
			// Если создание сокета прошло неудачно
			if(sock == net::invalid_socket_t){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Возвращаем результат
				return result;
			}
			/**
			 * Шаг 1. Применяем IP-адрес (и при необходимости адрес пира) через управляющий сокет
			 */
			result = ::device::applyAddress(sock, name, ip, peer, prefix, this->_log);
			/**
			 * Шаг 2. Устанавливаем MTU интерфейса (если задан)
			 */
			if(result && (mtu > 0)){
				// Настраиваем интерфейс
				struct ifreq ifr{0};
				// Копируем имя интерфейса
				::device::copyName(ifr.ifr_name, name);
				// Устанавливаем MTU интерфейса
				ifr.ifr_mtu = static_cast <int32_t> (mtu);
				// Применяем новый MTU интерфейса
				if(!(result = (::ioctl(sock, SIOCSIFMTU, &ifr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			}
			/**
			 * Шаг 3. Поднимаем интерфейс (устанавливаем флаг IFF_UP)
			 */
			if(result){
				// Настраиваем интерфейс
				struct ifreq ifr{0};
				// Копируем имя интерфейса
				::device::copyName(ifr.ifr_name, name);
				// Если удалось получить текущие флаги интерфейса
				if((result = (::ioctl(sock, SIOCGIFFLAGS, &ifr) == 0))){
					// Устанавливаем флаг поднятия интерфейса
					ifr.ifr_flags |= IFF_UP;
					// Применяем новые флаги интерфейса
					if(!(result = (::ioctl(sock, SIOCSIFFLAGS, &ifr) == 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				// Если получить флаги интерфейса не удалось
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			}
			// Закрываем сокет
			::close(sock);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, static_cast <uint16_t> (prefix), mtu), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки безопасности работы потоков
 *
 * @details Общего учёта, который приходилось бы защищать, у модуля на этой системе
 *          нет: перебор связей для туннелей ведёт только Solaris, и замок заведён
 *          там же. Оттого вызов ничего не делает
 *
 * @note Пустота эта НАМЕРЕННА и не подлежит переоткрытию как недоделка. Настройка
 *       общая на все системы, и вычёркивать её у тех, кому защищать нечего, значило
 *       бы разводить договор модуля по системам
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::eth::Interface::threadSafety([[maybe_unused]] const bool mode) noexcept {
	// Защищать на этой системе нечего
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
awh::eth::Interface::Interface(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::eth::Interface::~Interface() noexcept {}
