/**
 * @file: addr.cpp
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
 * Если стандартные DNS-серверы IPv4 не установлены
 */
#ifndef AWH_IPV4_NS
	/**
	 * Устанавливаем стандартные DNS-серверы IPv4
	 */
	#define AWH_IPV4_NS \
		"8.8.8.8", \
		"8.8.4.4", \
		"1.1.1.1", \
		"1.0.0.1", \
		"77.88.8.8", \
		"77.88.8.1"
#endif

/**
 * Если стандартные DNS-серверы IPv6 не установлены
 */
#ifndef AWH_IPV6_NS
	/**
	 * Устанавливаем стандартные DNS-серверы IPv6
	 */
	#define AWH_IPV6_NS \
		"2001:4860:4860::8888", \
		"2001:4860:4860::8844", \
		"2606:4700:4700::1111", \
		"2606:4700:4700::1001", \
		"2A02:6B8::FEED:0FF", \
		"2A02:6B8:0:1::FEED:0FF"
#endif

/**
 * Макрос выравнивания структуры
 */
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <cerrno>
#include <memory>
#include <vector>
#include <random>
#include <cstddef>
#include <cstring>
#include <cstdlib>

/**
 * Системные заголовочные файлы
 */
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <netinet/in.h> 
#include <net/route.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/addr.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Генератор случайных чисел для рандомизации DNS-серверов
	 *
	 */
	random_device __awh_randev__;

	/**
	 * @brief Нулевой MAC-адрес для сравнения
	 *
	 */
	constexpr uint8_t __awh_zero_mac__[6] = {0};

	/**
	 * @brief Нулевой IPv6-адрес для сравнения
	 *
	 */
	constexpr uint8_t __awh_zero_ipv6__[16] = {0};

	/**
	 * @brief Функция вычисления контрольной суммы
	 *
	 * @param data   указатель на данные
	 * @param length длина данных
	 * @return       вычисленная контрольная сумма
	 */
	uint16_t checksum(const void * data, size_t length) noexcept {
		// Получаем нужного вида буфер входящих данных
		const uint16_t * buffer = reinterpret_cast <const uint16_t *> (data);
		// Инициализируем сумму
		uint32_t sum = 0;
		/**
		 * Пока есть данные для обработки
		 */
		while(length > 1){
			// Добавляем к сумме очередные два байта данных
			sum += (* buffer++);
			// Уменьшаем длину данных на два байта
			length -= 2;
		}
		// Если остался один байт данных
		if(length == 1)
			// Добавляем к сумме последний байт данных
			sum += (* reinterpret_cast <const uint8_t *> (buffer));
		/**
		 * Складываем старшие 16 бит суммы с младшими 16 битами суммы
		 */
		while(sum >> 16)
			// Складываем старшие 16 бит суммы с младшими 16 битами суммы
			sum = ((sum & 0xFFFF) + (sum >> 16));
		// Возвращаем инвертированную сумму
		return static_cast <uint16_t> (~sum);
	}
};

/**
 * @brief Метод заполнения источника сетевых адресов по имени сетевого интерфейса
 *
 * @param source объект источника сетевых адресов
 */
void awh::eth::Network_Address::fillSource(net::src_t & source) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если название сетевого интерфейса передано
		if(!source.iface.empty()){
			// Если MAC-адрес ещё не заполнен
			if((::strncmp("lo", source.iface.c_str(), 2) != 0) &&
			   (::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], __awh_zero_mac__, 6) == 0)){
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0)
					// Выходим из функции
					return;
				// Переменная результата
				bool result = false;
				/**
				 * Перебираем все сетевые интерфейсы
				 */
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Пропускаем не совпадающие имена интерфейсов
					if((ifa->ifa_name == nullptr) || !this->_fmk->compare(ifa->ifa_name, source.iface))
						// Переходим к следующему интерфейсу
						continue;
					// Ищем MAC-адрес интерфейса
					if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)){
						// Получаем текущее значение аппаратного сетевого адреса
						struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
						// Проверяем длину MAC-адреса
						if((result = (sdl->sdl_alen == 6))){
							// Копируем MAC-адрес в результат
							const uint8_t * ptr = reinterpret_cast <const uint8_t *> (LLADDR(sdl));
							// Копируем MAC-адрес в результат
							::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6);
							// Завершаем поиск MAC-адреса
							break;
						}
					}
				}
				// Освобождаем память списка сетевых интерфейсов
				::freeifaddrs(ptr);
				// Если MAC-адрес не был найден
				if(!result)
					// Завершаем работу функции
					return;
			}
			/**
			 * Определяем тип адреса
			 */
			switch(source.ip->size){
				// Если адрес является IPv4
				case 4: {
					// Если IP-адрес ещё не заполнен
					if(awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address == 0){
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv4-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
								// Пропускаем интерфейсы, которые не являются IPv4
								continue;
							// Если интерфейс не активен
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Если имя интерфейса совпадает
							if(this->_fmk->compare(ifa->ifa_name, source.iface)){
								// Копируем IP-адрес в результат
								awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr)->sin_addr.s_addr;
								// Выходим из цикла
								break;
							}
						}
						// Освобождаем память от списка сетевых интерфейсов
						::freeifaddrs(ptr);
					}
				} break;
				// Если адрес является IPv6
				case 16: {
					// Если IPv6-адрес ещё не заполнен
					if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], __awh_zero_ipv6__, 16) == 0){
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv6-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
								// Пропускаем интерфейсы, которые не являются IPv6
								continue;
							// Пропускаем выключенные интерфейсы
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Если имя интерфейса совпадает
							if(this->_fmk->compare(ifa->ifa_name, source.iface)){
								// Копируем IP-адрес в результат
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr, sizeof(in6_addr));
								// Выходим из цикла
								break;
							}
						}
						// Освобождаем память списка сетевых интерфейсов
						::freeifaddrs(ptr);
					}
				} break;
			}
		// Загружаем данные по умолчанию
		} else this->fillSource(event::node_t::NONE, source);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод заполнения источника сетевых адресов по заданной сети
 *
 * @param net    сетевой адрес подсети (IP-адрес в сетевом порядке байт)
 * @param source объект источника сетевых адресов
 */
void awh::eth::Network_Address::fillSource(const net::addr_t * net, net::src_t & source) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип адреса
		 */
		switch(source.ip->size){
			// Если адрес является IPv4
			case 4: {
				// Получаем сетевой адрес подсети
				const net::addr_net_ipv4_t * network = awh_cast <const net::addr_net_ipv4_t *> (net);
				// Проверяем корректность префикса сети
				if(network->prefix > 32)
					// Корректируем префикс сети
					const_cast <net::addr_net_ipv4_t *> (network)->prefix = 32;
				/**
				 * Блокируем работу ненужной проверки (пока непонятно что с этим делать)
				 * Проверка не работает на то, соответствует ли IP-адрес 192.168.7.249 маске 255.255.255.0
				 */
				#ifdef __AWH_DISABLED__
					// Проверка выравнивания сетевого адреса по маске
					const uint32_t mask = ((network->prefix == 0) ? 0 : (~((1U << (32 - network->prefix)) - 1)));
					// Если сетевой адрес не выровнен по маске
					if((htonl(network->address) & mask) != htonl(network->address)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"Network address %u is not aligned to prefix %u", __PRETTY_FUNCTION__,
								make_tuple(htonl(network->address), static_cast <uint16_t> (network->prefix)),
								log_t::flag_t::WARNING, htonl(network->address), static_cast <uint16_t> (network->prefix)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Network address %u is not aligned to prefix %u", log_t::flag_t::WARNING, htonl(network->address), static_cast <uint16_t> (network->prefix));
						#endif
						// Выходим из функции
						return;
					}
				#endif
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0){
					// Буфер временных данных для генерации IP-адреса
					char buffer[INET_ADDRSTRLEN];
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"Unable to get list of network interfaces", __PRETTY_FUNCTION__,
							make_tuple(
								::inet_ntop(AF_INET, &network->address, buffer, sizeof(buffer)),
								static_cast <uint16_t> (network->prefix)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
					#endif
					// Выходим из функции
					return;
				}
				// Устанавливаем префикс хостового адреса
				awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->prefix = network->prefix;
				/**
				 * Перебираем все сетевые интерфейсы
				 */
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Пропускаем не IPv4-интерфейсы
					if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
						// Пропускаем интерфейсы, которые не являются IPv4
						continue;
					// Пропускаем loopback и down-интерфейсы (опционально)
					// if(ifa->ifa_flags & IFF_LOOPBACK) continue;
					// Если интерфейс не активен
					if(!(ifa->ifa_flags & IFF_UP))
						// Пропускаем неактивные интерфейсы
						continue;
					// Получаем IP-адрес интерфейса
					struct sockaddr_in * sin = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr);
					// Преобразуем IP-адрес в хостовый порядок
					const uint32_t ip = sin->sin_addr.s_addr;
					// Проверяем принадлежность IP-адреса подсети
					if(this->isInSubnet(ntohl(ip), htonl(network->address), network->prefix)){
						// Устанавливаем название сетевого интерфейса
						source.iface = ifa->ifa_name;
						// Получаем MAC-адрес сетевого интерфейса
						this->fillSource(source);
						// Устанавливаем хост сети
						awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = ip;
						// Прерываем цикл поиска
						break;
					}
				}
				// Освобождаем память от списка сетевых интерфейсов
				::freeifaddrs(ptr);
			} break;
			// Если адрес является IPv6
			case 16: {
				// Получаем сетевой адрес подсети
				const net::addr_net_ipv6_t * network = awh_cast <const net::addr_net_ipv6_t *> (net);
				// Проверяем корректность префикса сети
				if(network->prefix > 128)
					// Корректируем префикс сети
					const_cast <net::addr_net_ipv6_t *> (network)->prefix = 128;
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0){
					// Буфер временных данных для генерации IP-адреса
					char buffer[INET_ADDRSTRLEN];
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"Unable to get list of network interfaces", __PRETTY_FUNCTION__,
							make_tuple(
								::inet_ntop(AF_INET6, &network->address[0], buffer, sizeof(buffer)),
								static_cast <uint16_t> (network->prefix)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
					#endif
					// Выходим из функции
					return;
				}
				// Устанавливаем префикс хостового адреса
				awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->prefix = network->prefix;
				/**
				 * Перебираем все сетевые интерфейсы
				 */
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Пропускаем не IPv6-интерфейсы
					if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
						// Пропускаем интерфейсы, которые не являются IPv6
						continue;
					// Пропускаем выключенные интерфейсы
					if(!(ifa->ifa_flags & IFF_UP))
						// Пропускаем неактивные интерфейсы
						continue;
					// Получаем указатель на IPv6-адрес
					struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
					// Получаем ссылку на IP-адрес
					const in6_addr & ip = sin->sin6_addr;
					// Пропускаем link-local, если не ищем их (опционально)
					// if(IN6_IS_ADDR_LINKLOCAL(&ip)) continue;
					// Проверяем принадлежность IP-адреса подсети
					if(this->ipv6PrefixEqual(ip.s6_addr, &network->address[0], network->prefix)){
						// Устанавливаем название сетевого интерфейса
						source.iface = ifa->ifa_name;
						// Получаем MAC-адрес сетевого интерфейса
						this->fillSource(source);
						// Хост: просто копируем найденный адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], ip.s6_addr, sizeof(ip.s6_addr));
						// Прерываем цикл поиска
						break;
					}
				}
				// Освобождаем память списка сетевых интерфейсов
				::freeifaddrs(ptr);
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
/**
 * @brief Метод заполнения источника сетевых адресов
 *
 * @param node   тип узла события
 * @param source объект источника сетевых адресов
 */
void awh::eth::Network_Address::fillSource(const event::node_t node, net::src_t & source) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип узла
		 */
		switch(static_cast <uint8_t> (node)){
			// Если тип узла не установлен
			case static_cast <uint8_t> (event::node_t::NONE): {
				/**
				 * Определяем тип адреса
				 */
				switch(source.ip->size){
					// Если адрес является IPv4
					case 4: {
						// Создаем структуру подключения сервера
						struct sockaddr_in serv{0};
						// Указываем тип сетевого подключения IPv4
						serv.sin_family = AF_INET;
						// Устанавливаем порт DNS-сервера
						serv.sin_port = htons(53);
						// Создаём массив стандартных DNS-серверов IPv4
						const array <string_view, 6> resolvers = {AWH_IPV4_NS};
						// Указываем адреса IPv4 DNS-сервера
						::inet_pton(AF_INET, resolvers[::__awh_randev__() % resolvers.size()].data(), &serv.sin_addr);
						// Создаем сокет для проверки подключения
						const net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
						// Если сокет создать не удалось, выходим
						if(sock == net::invalid_socket_t)
							// Выходим из функции
							return;
						// Выполняем подключение к серверу
						int32_t conn = ::connect(sock, reinterpret_cast <const sockaddr *> (&serv), sizeof(serv));
						// Если подключение удачное
						if(conn > -1){
							// Создаем структуру имени
							struct sockaddr_in name{0};
							// Размер структуры
							socklen_t size = sizeof(name);
							// Запрашиваем имя сокета
							conn = ::getsockname(sock, reinterpret_cast <sockaddr *> (&name), &size);
							// Если ошибки нет
							if(conn > -1){
								// Устанавливаем хост сети
								awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = name.sin_addr.s_addr;
								// Устанавливаем название сетевого интерфейса
								source.iface = this->_iface.name(source.ip.get());
								// Если название сетевого интерфейса получено
								if(!source.iface.empty())
									// Получаем MAC-адрес сетевого интерфейса
									this->fillSource(source);
							}
						}
						// Закрываем сокет
						::close(sock);
					} break;
					// Если адрес является IPv6
					case 16: {
						// Создаем структуру подключения сервера
						struct sockaddr_in6 serv{0};
						// Указываем тип сетевого подключения IPv6
						serv.sin6_family = AF_INET6;
						// Устанавливаем порт DNS сервера
						serv.sin6_port = htons(53);
						// Создаём массив стандартных DNS-серверов IPv6
						const array <string_view, 6> resolvers = {AWH_IPV6_NS};
						// Указываем адреса IPv6 DNS-сервера
						::inet_pton(AF_INET6, resolvers[::__awh_randev__() % resolvers.size()].data(), &serv.sin6_addr);
						// Создаем сокет для проверки подключения
						const net::socket_t sock = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
						// Если сокет создать не удалось, выходим
						if(sock == net::invalid_socket_t)
							// Выходим из функции
							return;
						// Выполняем подключение к серверу
						int32_t conn = ::connect(sock, reinterpret_cast <const sockaddr *> (&serv), sizeof(serv));
						// Если подключение удачное
						if(conn > -1){
							// Создаем структуру имени
							struct sockaddr_in6 name{0};
							// Размер структуры
							socklen_t size = sizeof(name);
							// Запрашиваем имя сокета
							conn = ::getsockname(sock, reinterpret_cast <sockaddr *> (&name), &size);
							// Если ошибки нет
							if(conn > -1){
								// Хост: просто копируем найденный адрес
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], name.sin6_addr.s6_addr, sizeof(name.sin6_addr.s6_addr));
								// Устанавливаем название сетевого интерфейса
								source.iface = this->_iface.name(source.ip.get());
								// Если название сетевого интерфейса получено
								if(!source.iface.empty())
									// Получаем MAC-адрес сетевого интерфейса
									this->fillSource(source);
							}
						}
						// Закрываем сокет
						::close(sock);
					} break;
				}
			} break;
			// Если тип узла является одноранговым узлом
			case static_cast <uint8_t> (event::node_t::PEER): {
				/**
				 * Определяем тип адреса
				 */
				switch(source.ip->size){
					// Если адрес является IPv4
					case 4: {
						// Создаём массив параметров сетевого интерфейса
						int32_t mib[6];
						// Устанавливаем парарметры сетевого интерфейса
						mib[0] = CTL_NET;
						mib[1] = PF_ROUTE;
						mib[2] = 0;
						mib[3] = AF_INET;
						mib[4] = NET_RT_FLAGS;
						/**
						 * Если операционной системой является NetBSD или OpenBSD
						 */
						#if __NetBSD__ || __OpenBSD__
							mib[5] = RTF_LLDATA;
						/**
						 * Если операционной системой является macOS или FreeBSD
						 */
						#else
							mib[5] = RTF_LLINFO;
						#endif
						// Размер буфера данных
						size_t size = 0;
						// Выполняем получение размера буфера
						if(::sysctl(mib, 6, nullptr, &size, nullptr, 0) < 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Route sysctl estimate", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Route sysctl estimate", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Создаём буфер данных сетевого интерфейса
						vector <char> buffer(size);
						// Выполняем получение данных сетевого интерфейса
						if(::sysctl(mib, 6, &buffer[0], &size, nullptr, 0) < 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Actual retrieval of routing table", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Actual retrieval of routing table", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Начало итератора в буфере
						char * begin = nullptr;
						// Получаем конечное значение итератора
						char * end = (&buffer[0] + size);
						// Получаем числовое значение IP-адреса
						const uint32_t addr = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
						/**
						 * Переходим по всем сетевым интерфейсам
						 */
						for(begin = &buffer[0]; begin < end;){
							// Получаем указатель сетевого интерфейса
							struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
							// Переходим к следующему элементу
							begin += rtm->rtm_msglen;
							// Если версия RTM протокола не соответствует, пропускаем
							if(rtm->rtm_version != RTM_VERSION)
								// Выполняем пропуск
								continue;
							// Получаем текущее значение активного подключения (sockaddr идёт сразу после заголовка)
							struct sockaddr_inarp * sin = reinterpret_cast <struct sockaddr_inarp *> (rtm + 1);
							// Если сетевой интерфейс отличается от IPv4, пропускаем
							if(sin->sin_family != AF_INET)
								// Выполняем пропуск
								continue;
							// Получаем текущее значение аппаратного сетевого адреса (с учётом выравнивания sockaddr)
							struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (reinterpret_cast <char *> (sin) + ROUNDUP(sin->sin_len));
							// Если версия сетевого протокола отличается от MAC, пропускаем
							if(sdl->sdl_family != AF_LINK)
								// Выполняем пропуск
								continue;
							// Если IP-адрес установлен
							if(addr > 0){
								// Если искомый IP-адрес не совпадает, пропускаем
								if(addr != sin->sin_addr.s_addr)
									// Выполняем пропуск
									continue;
								// Если сетевой интерфейс получен
								if(sdl->sdl_alen > 0x00){
									// Копируем MAC-адрес в результат
									const uint8_t * ptr = reinterpret_cast <const uint8_t *> (LLADDR(sdl));
									// Копируем MAC-адрес в результат
									::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6);
									// Выходим из цикла
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Если сетевой интерфейс получен
								if(sdl->sdl_alen > 0x00){
									// Если MAC-адреса совпадают
									const uint8_t * ptr = reinterpret_cast <const uint8_t *> (LLADDR(sdl));
									// Сравниваем MAC-адреса
									if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6) == 0){
										// Копируем IP-адрес в результат
										awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = sin->sin_addr.s_addr;
										// Выходим из цикла
										break;
									}
								}
							}
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						// Создаём массив параметров сетевого интерфейса
						int32_t mib[6];
						// Устанавливаем парарметры сетевого интерфейса
						mib[0] = CTL_NET;
						mib[1] = PF_ROUTE;
						mib[2] = 0;
						mib[3] = AF_INET6;
						mib[4] = NET_RT_FLAGS;
						/**
						 * Если операционной системой является NetBSD или OpenBSD
						 */
						#if __NetBSD__ || __OpenBSD__
							mib[5] = RTF_LLDATA;
						/**
						 * Если операционной системой является macOS или FreeBSD
						 */
						#else
							mib[5] = RTF_LLINFO;
						#endif
						// Размер буфера данных
						size_t size = 0;
						// Выполняем получение размера буфера
						if(::sysctl(mib, 6, nullptr, &size, nullptr, 0) < 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Route sysctl estimate", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Route sysctl estimate", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Создаём буфер данных сетевого интерфейса
						vector <char> buffer(size);
						// Выполняем получение данных сетевого интерфейса
						if(::sysctl(mib, 6, &buffer[0], &size, nullptr, 0) < 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Actual retrieval of routing table", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Actual retrieval of routing table", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Начало итератора в буфере
						char * begin = nullptr;
						// Получаем конечное значение итератора
						char * end = (&buffer[0] + size);
						// Создаём объект подключения
						struct sockaddr_in6 addr{0};
						// Копируем IP-адрес в структуру подключения
						::memcpy(&addr.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(addr.sin6_addr));
						/**
						 * Переходим по всем сетевым интерфейсам
						 */
						for(begin = &buffer[0]; begin < end;){
							// Получаем указатель сетевого интерфейса
							struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
							// Переходим к следующему элементу
							begin += rtm->rtm_msglen;
							// Если версия RTM протокола не соответствует, пропускаем
							if(rtm->rtm_version != RTM_VERSION)
								// Выполняем пропуск
								continue;
							// Получаем текущее значение активного подключения (sockaddr идёт сразу после заголовка)
							struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (rtm + 1);
							// Если сетевой интерфейс отличается от IPv6, пропускаем
							if(sin->sin6_family != AF_INET6)
								// Выполняем пропуск
								continue;
							/**
							 * Если мы работаем с KAME
							 */
							#ifdef __KAME__
								{
									// Получаем текущий адрес IPv6
									struct in6_addr * in6 = &sin->sin6_addr;
									// Проверяем вид интерфейса, если интерфейс локальный и скоуп-ID не установлен
									if((IN6_IS_ADDR_LINKLOCAL(in6) || IN6_IS_ADDR_MC_LINKLOCAL(in6)) && (sin->sin6_scope_id == 0)){
										// Принудительно устанавливаем скоуп-ID
										sin->sin6_scope_id = static_cast <uint32_t> (ntohs(* reinterpret_cast <uint16_t *> (&in6->s6_addr[2])));
										// Выполняем зануление третьего хексета
										(* reinterpret_cast <uint16_t *> (&in6->s6_addr[2])) = 0;
									}
								}
							#endif
							// Получаем текущее значение аппаратного сетевого адреса
							struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (reinterpret_cast <char *> (sin) + ROUNDUP(sin->sin6_len));
							// Если версия сетевого протокола отличается от MAC, пропускаем
							if(sdl->sdl_family != AF_LINK)
								// Выполняем пропуск
								continue;
							// Если IP-адрес установлен
							if(::memcmp(&addr.sin6_addr, __awh_zero_ipv6__, 16) != 0){
								/*
								// Если RTM не соответствует хосту, пропускаем
								if(!(rtm->rtm_flags & RTF_HOST))
									// Выполняем пропуск
									continue;
								 */
								// Проверяем соответствует ли IP-адрес - тому, что мы ищем
								if(!IN6_ARE_ADDR_EQUAL(&addr.sin6_addr, &sin->sin6_addr))
									// Выполняем пропуск
									continue;
								// Если сетевой интерфейс получен
								if(sdl->sdl_alen > 0x00){
									// Копируем MAC-адрес в результат
									const uint8_t * ptr = reinterpret_cast <const uint8_t *> (LLADDR(sdl));
									// Копируем MAC-адрес в результат
									::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6);
									// Выходим из цикла
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Если сетевой интерфейс получен
								if(sdl->sdl_alen > 0x00){
									// Если MAC-адреса совпадают
									const uint8_t * ptr = reinterpret_cast <const uint8_t *> (LLADDR(sdl));
									// Сравниваем MAC-адреса
									if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6) == 0){
										// Копируем IP-адрес в результат
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &sin->sin6_addr, sizeof(in6_addr));
										// Выходим из цикла
										break;
									}
								}
							}
						}
					} break;
				}
			} break;
			// Если узел является клиентом
			case static_cast <uint8_t> (event::node_t::CLIENT):
			// Если узел является сервером
			case static_cast <uint8_t> (event::node_t::SERVER): {
				/**
				 * Определяем тип адреса
				 */
				switch(source.ip->size){
					// Если адрес является IPv4
					case 4: {
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Получаем числовое значение IP-адреса
						const uint32_t addr = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv4-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
								// Пропускаем интерфейсы, которые не являются IPv4
								continue;
							// Если интерфейс не активен
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Получаем IP-адрес интерфейса
							struct sockaddr_in * sin = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr);
							// Если IP-адрес установлен
							if(addr > 0){
								// Если IP-адрес совпадает с указанным IP-адресом
								if(addr == sin->sin_addr.s_addr){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Получаем MAC-адрес сетевого интерфейса
									this->fillSource(source);
									// Прерываем цикл поиска
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Признак того, что MAC-адрес найден
								bool found = false;
								// Буфер MAC-адреса текущего интерфейса
								uint8_t mac[6] = {0};
								/**
								 * Извлекаем MAC текущего интерфейса из уже полученного списка (без повторного системного вызова)
								 */
								for(struct ifaddrs * link = ptr; link != nullptr; link = link->ifa_next){
									// Ищем запись канального уровня с тем же именем интерфейса
									if((link->ifa_addr != nullptr) && (link->ifa_addr->sa_family == AF_LINK) && (link->ifa_name != nullptr) && (::strcmp(link->ifa_name, ifa->ifa_name) == 0)){
										// Получаем текущее значение аппаратного сетевого адреса
										struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (link->ifa_addr);
										// Если длина MAC-адреса корректна
										if(sdl->sdl_alen == 6){
											// Копируем MAC-адрес в буфер
											::memcpy(mac, LLADDR(sdl), 6);
											// Помечаем, что MAC-адрес найден
											found = true;
										}
										// Завершаем поиск
										break;
									}
								}
								// Сравниваем MAC-адреса
								if(found && (::memcmp(mac, &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6) == 0)){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Копируем IP-адрес в результат
									awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = sin->sin_addr.s_addr;
									// Выходим из цикла
									break;
								}
							}
						}
						// Освобождаем память от списка сетевых интерфейсов
						::freeifaddrs(ptr);
					} break;
					// Если адрес является IPv6
					case 16: {
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Создаём объект подключения
						struct sockaddr_in6 addr{0};
						// Копируем IP-адрес в структуру подключения
						::memcpy(&addr.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(addr.sin6_addr));
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv6-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
								// Пропускаем интерфейсы, которые не являются IPv6
								continue;
							// Пропускаем выключенные интерфейсы
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Получаем указатель на IPv6-адрес
							struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
							// Если IP-адрес установлен
							if(::memcmp(&addr.sin6_addr, __awh_zero_ipv6__, 16) != 0){
								// Если IP-адрес совпадает с указанным IP-адресом
								if(IN6_ARE_ADDR_EQUAL(&addr.sin6_addr, &sin->sin6_addr)){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Получаем MAC-адрес сетевого интерфейса
									this->fillSource(source);
									// Прерываем цикл поиска
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Признак того, что MAC-адрес найден
								bool found = false;
								// Буфер MAC-адреса текущего интерфейса
								uint8_t mac[6] = {0};
								/**
								 * Извлекаем MAC текущего интерфейса из уже полученного списка (без повторного системного вызова)
								 */
								for(struct ifaddrs * link = ptr; link != nullptr; link = link->ifa_next){
									// Ищем запись канального уровня с тем же именем интерфейса
									if((link->ifa_addr != nullptr) && (link->ifa_addr->sa_family == AF_LINK) && (link->ifa_name != nullptr) && (::strcmp(link->ifa_name, ifa->ifa_name) == 0)){
										// Получаем текущее значение аппаратного сетевого адреса
										struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (link->ifa_addr);
										// Если длина MAC-адреса корректна
										if(sdl->sdl_alen == 6){
											// Копируем MAC-адрес в буфер
											::memcpy(mac, LLADDR(sdl), 6);
											// Помечаем, что MAC-адрес найден
											found = true;
										}
										// Завершаем поиск
										break;
									}
								}
								// Сравниваем MAC-адреса
								if(found && (::memcmp(mac, &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6) == 0)){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Копируем IP-адрес в результат
									::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &sin->sin6_addr, sizeof(in6_addr));
									// Выходим из цикла
									break;
								}
							}
						}
						// Освобождаем память списка сетевых интерфейсов
						::freeifaddrs(ptr);
					} break;
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node), static_cast <uint16_t> (source.ip->size)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки принадлежности IP-адреса подсети
 *
 * @param ip     проверяемый IP-адрес в хостовом порядке
 * @param net    сетевой адрес подсети в хостовом порядке
 * @param prefix префикс подсети
 * @return       результат проверки
 */
bool awh::eth::Network_Address::isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если префикс равен нулю, то любой IP-адрес принадлежит подсети
		if(prefix == 0)
			// Возвращаем результат проверки
			return true;
		// Вычисляем маску подсети
		uint32_t mask = (~((1U << (32 - prefix)) - 1));
		// Проверяем принадлежность IP-адреса подсети
		return ((ip & mask) == (net & mask));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(ip, net, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод сравнения двух IPv6-адресов по префиксу (в битах)
 *
 * @param first  Первый IPv6-адрес
 * @param second Второй IPv6-адрес
 * @param length Длина префикса в битах
 * @return       Результат сравнения
 */
bool awh::eth::Network_Address::ipv6PrefixEqual(const uint8_t * first, const uint8_t * second, const uint8_t length) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если длина префикса равна нулю, адреса считаются равными
		if(length == 0)
			// Возвращаем результат сравнения
			return true;
		// Вычисляем количество полных байтов и оставшихся битов
		size_t fullBytes = (length / 8);
		// Вычисляем количество битов в последнем байте
		uint8_t bitsInLast = (length % 8);
		// Сравниваем полные байты
		if(::memcmp(first, second, fullBytes) != 0)
			// Возвращаем результат сравнения
			return false;
		// Если нет оставшихся битов, адреса равны
		if(bitsInLast == 0)
			// Возвращаем результат сравнения
			return true;
		// Сравниваем оставшиеся биты в последнем байте
		const uint8_t mask = ((0xFF << (8 - bitsInLast)) & 0xFF);
		// Возвращаем результат сравнения
		return ((first[fullBytes] & mask) == (second[fullBytes] & mask));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(first, second, static_cast <uint16_t> (length)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод вычисления контрольной суммы транспортного уровня
 *
 * @param family    семейство протоколов (IPv4 или IPv6)
 * @param protocol  протокол транспортного уровня
 * @param src       указатель на источник данных
 * @param dst       указатель на приёмник данных
 * @param transport указатель на данные транспортного уровня
 * @param length    длина данных транспортного уровня
 * @return          вычисленная контрольная сумма
 */
uint16_t awh::eth::Network_Address::checksum(const event::family_t family, const event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length) const noexcept {
	// Переменная результата
	uint16_t result = 0;
	// Проверяем корректность входных данных
	if((src != nullptr) && (dst != nullptr) && (transport != nullptr) && (length > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Общий размер данных
			size_t totalSize = 0;
			// Размер псевдозаголовка
			size_t pseudoSize = 0;
			// Смещение поля контрольной суммы в транспортном заголовке
			size_t checksumOffset = 0;
			// Псевдозаголовок
			unique_ptr <uint8_t []> pseudo = nullptr;
			/**
			 * Определяем протокол
			 */
			switch(static_cast <uint8_t> (protocol)){
				// Если протокол определён как TCP
				case static_cast <uint8_t> (event::protocol_t::TCP):
					// Запоминаем смещение поля контрольной суммы в TCP-заголовке
					checksumOffset = offsetof(struct tcphdr, th_sum);
				break;
				// Если протокол определён как UDP
				case static_cast <uint8_t> (event::protocol_t::UDP):
					// Запоминаем смещение поля контрольной суммы в UDP-заголовке
					checksumOffset = offsetof(struct udphdr, uh_sum);
				break;
				// Для неподдерживаемого протокола
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unsupported protocol for checksum calculation", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unsupported protocol for checksum calculation", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из функции
					return result;
				}
			}
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					/**
					 * @brief Структура псевдозаголовка IPv4
					 *
					 */
					struct {
						uint32_t src;    // IP-адрес источника
						uint32_t dst;    // IP-адрес назначения
						uint8_t zero;    // Зарезервировано, должно быть равно 0
						uint8_t proto;   // Протокол транспортного уровня
						uint16_t length; // Длина транспортного уровня
					} hdr;
					// Устанавливаем ноль в зарезервированное поле
					hdr.zero = 0;
					/**
					 * Определяем протокол
					 */
					switch(static_cast <uint8_t> (protocol)){
						// Если протокол определён как TCP
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Устанавливаем протокол транспортного уровня
							hdr.proto = IPPROTO_TCP;
						break;
						// Если протокол определён как UDP
						case static_cast <uint8_t> (event::protocol_t::UDP):
							// Устанавливаем протокол транспортного уровня
							hdr.proto = IPPROTO_UDP;
						break;
					}
					// Устанавливаем IP-адреса источника и назначения
					hdr.src = (* reinterpret_cast <const uint32_t *> (src));
					hdr.dst = (* reinterpret_cast <const uint32_t *> (dst));
					// Устанавливаем длину транспортного уровня
					hdr.length = htons(static_cast <uint16_t> (length));
					// Вычисляем размеры псевдозаголовка
					pseudoSize = sizeof(hdr);
					// Вычисляем общий размер данных
					totalSize = (pseudoSize + length);
					// Выделяем память под псевдозаголовок
					pseudo = make_unique <uint8_t []> (totalSize);
					// Формируем псевдозаголовок
					::memcpy(pseudo.get(), &hdr, pseudoSize);
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// IPv6
					struct {
						// IP-адрес источника
						struct in6_addr src;
						// IP-адрес назначения
						struct in6_addr dst;
						// Длина транспортного уровня
						uint32_t length;
						// Зарезервировано, должно быть равно 0
						uint8_t zero[3];
						// Следующий заголовок
						uint8_t next_hdr;
					} hdr;
					/**
					 * Определяем протокол
					 */
					switch(static_cast <uint8_t> (protocol)){
						// Если протокол определён как TCP
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Устанавливаем протокол транспортного уровня
							hdr.next_hdr = IPPROTO_TCP;
						break;
						// Если протокол определён как UDP
						case static_cast <uint8_t> (event::protocol_t::UDP):
							// Устанавливаем протокол транспортного уровня
							hdr.next_hdr = IPPROTO_UDP;
						break;
					}
					// Устанавливаем нули в зарезервированное поле
					hdr.zero[0] = hdr.zero[1] = hdr.zero[2] = 0;
					// Устанавливаем IP-адреса источника и назначения
					hdr.src = (* reinterpret_cast <const struct in6_addr *> (src));
					hdr.dst = (* reinterpret_cast <const struct in6_addr *> (dst));
					// Устанавливаем длину транспортного уровня (да, 32-bit, но значение 16-bit)
					hdr.length = htonl(static_cast <uint32_t> (length));
					// Вычисляем размеры псевдозаголовка
					pseudoSize = sizeof(hdr);
					// Вычисляем общий размер данных
					totalSize = (pseudoSize + length);
					// Выделяем память под псевдозаголовок
					pseudo = make_unique <uint8_t []> (totalSize);
					// Формируем псевдозаголовок
					::memcpy(pseudo.get(), &hdr, pseudoSize);
				} break;
			}
			// Если семейство протокола не поддержано, псевдозаголовок не сформирован
			if(pseudo == nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unsupported address family for checksum calculation", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unsupported address family for checksum calculation", log_t::flag_t::CRITICAL);
				#endif
				// Выходим из функции
				return result;
			}
			// Копируем транспортный заголовок + данные
			::memcpy(pseudo.get() + pseudoSize, transport, length);
			// Обнуляем контрольную сумму в копии транспортного заголовка (исходный буфер не модифицируется)
			if((checksumOffset + sizeof(uint16_t)) <= length)
				// Зануляем поле контрольной суммы в копии
				::memset(pseudo.get() + pseudoSize + checksumOffset, 0, sizeof(uint16_t));
			// Вычисляем контрольную сумму
			result = ::checksum(pseudo.get(), totalSize);
			// Для UDP нулевая контрольная сумма передаётся как 0xFFFF (RFC 768)
			if((static_cast <uint8_t> (protocol) == static_cast <uint8_t> (event::protocol_t::UDP)) && (result == 0))
				// Корректируем нулевую контрольную сумму UDP
				result = 0xFFFF;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::eth::Network_Address::Network_Address(const fmk_t * fmk, const log_t * log) noexcept :
 _iface(fmk, log), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::eth::Network_Address::~Network_Address() noexcept {}
