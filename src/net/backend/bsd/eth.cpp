/**
 * @file: eth.cpp
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
 * Если максимальное количество файловых дескрипторов не передано
 */
#ifndef AWH_MAX_COUNT_FDS
	/**
	 * Устанавливаем максимальное количество доступных файловых дескрипторов 131072
	 */
	#define AWH_MAX_COUNT_FDS 0x20000
#endif

/**
 * Если стандартные DNS-серверы IPv4 не установлены
 */
#ifndef AWH_IPV4_RESOLVER
	/**
	 * Устанавливаем стандартные DNS-серверы IPv4
	 */
	#define AWH_IPV4_RESOLVER "8.8.8.8"
#endif

/**
 * Если стандартные DNS-серверы IPv6 не установлены
 */
#ifndef AWH_IPV6_RESOLVER
	/**
	 * Устанавливаем стандартные DNS-серверы IPv6
	 */
	#define AWH_IPV6_RESOLVER "2001:4860:4860::8888"
#endif

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
#include <netdb.h>
#include <signal.h>
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
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>

/**
 * Если операционной системой является FreeBSD
 */
#if __FreeBSD__
	// Подключаем заголовочный файл SCTP
	#include <netinet/sctp.h>
#endif

/**
 * Подключаем наши заголовочные файлы
 */
#include <sys/os.hpp>

/**
 * Подключаем заголовочный файл системных ресурсов
 */
#include <net/fds.hpp>
#include <net/eth.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
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
 * @brief Метод применения сетевой оптимизации операционной системы
 *
 */
void awh::Ethernet::netboost() const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем инициализацию объекта работы с операционноы системы
		os_t os(this->_log);
		// Выполняем инициализацию объекта работы с файловыми дескрипторами
		fds_t fds(this->_log);
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Структура лимитов дампов
			struct rlimit limit;
			// Устанавливаем текущий лимит равный бесконечности
			limit.rlim_cur = RLIM_INFINITY;
			// Устанавливаем максимальный лимит равный бесконечности
			limit.rlim_max = RLIM_INFINITY;
			// Выводим результат установки лимита дампов ядра
			if(::setrlimit(RLIMIT_CORE, &limit) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		#endif
		/**
		 * Выполняем установку нужного нам количества файловых дескрипторов
		 */
		if(!fds.limit(AWH_MAX_COUNT_FDS)){
			// Получаем лимиты файловых дескрипторов
			const auto & limits = fds.limit();
			// Если текущий лимит меньше желаемого
			if(limits.first < AWH_MAX_COUNT_FDS)
				// Выводим сообщение подсказки
				fds.help(limits.first, AWH_MAX_COUNT_FDS);
		}
		/**
		 * Если необходимо выполнить тюннинг операционной системы
		 */
		#if AWH_BOOSTING_NET
			/**
			 * Для операционной системы MacOS X
			 */
			#if __APPLE__ || __MACH__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					// Устанавливаем максимальное количество подключений
					os.sysctl("kern.ipc.somaxconn", 49152);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок MacOS X
					 */
					os.sysctl("kern.ipc.maxsockbuf", 6291456);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvspace", 1042560);
					// В MacOS X значение по умолчанию 3, что очень мало
					os.sysctl("net.inet.tcp.r", 8);
					// Увеличиваем максимумы автонастройки MacOS X TCP
					os.sysctl("net.inet.tcp.autorcvbufmax", 33554432);
					os.sysctl("net.inet.tcp.autosndbufmax", 33554432);
					// Устанавливаем прочие настройки
					os.sysctl("net.inet.tcp.slowstart_flightsize", 20);
					os.sysctl("net.inet.tcp.local_slowstart_flightsize", 20);
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Root privileges are required to apply network optimizations", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Root privileges are required to apply network optimizations", log_t::flag_t::WARNING);
					#endif
				}
			/**
			 * Для операционной системы FreeBSD, NetBSD или OpenBSD
			 */
			#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Если эффективный идентификатор пользователя принадлежит ROOT
				if(os.isAdmin()){
					/**
					 * Данные оптимизаций операционной системы берет от сюда: http://fasterdata.es.net/host-tuning/freebsd
					 */
					// Активируем контроль работы временной марки и масштабируемого окна
					os.sysctl("net.inet.tcp.rfc1323", 1);
					// Устанавливаем максимальное количество подключений
					os.sysctl("kern.ipc.somaxconn", 49152);
					// Активируем автоматическую отправку и получение
					os.sysctl("net.inet.tcp.sendbuf_auto", 1);
					os.sysctl("net.inet.tcp.recvbuf_auto", 1);
					// Увеличиваем размер шага автонастройки
					os.sysctl("net.inet.tcp.sendbuf_inc", 8192);
					os.sysctl("net.inet.tcp.recvbuf_inc", 16384);
					// Активируем нормальное нормальное TCP Reno
					os.sysctl("net.inet.tcp.inflight.enable", 0);
					// Активируем на хостах тестирования/измерений
					os.sysctl("net.inet.tcp.hostcache.expire", 1);
					/**
					 * Для хостов 10G было бы неплохо увеличить это значение,
					 * т.к. 4G, похоже, является пределом для некоторых установок FreeBSD
					 */
					os.sysctl("kern.ipc.maxsockbuf", 16777216);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendspace", 1042560);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvspace", 1042560);
					// Увеличиваем максимальный размер буферов для отправки
					os.sysctl("net.inet.tcp.sendbuf_max", 16777216);
					// Увеличиваем максимальный размер буферов для чтения
					os.sysctl("net.inet.tcp.recvbuf_max", 16777216);
					/**
					 * Вы можете проверить, какие доступны алгоритмы получения доступных сообщений, используя net.inet.tcp.cc.available
					 */
					const string & algorithm = os.sysctl <string> ("net.inet.tcp.cc.available");
					// Если выбран лучший доступны алгоритм
					if(!algorithm.empty()){
						// Если найден алгоритм cubic
						if(this->_fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.inet.tcp.cc.algorithm", "cubic");
						// Если же найден алгоритм htcp
						else if(this->_fmk->exists("htcp", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.inet.tcp.cc.algorithm", "htcp");
					}
				// Если пользователь не является суперпользователем
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Root privileges are required to apply network optimizations", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Root privileges are required to apply network optimizations", log_t::flag_t::WARNING);
					#endif
				}
			#endif
		#endif
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
/**
 * @brief Метод получения имени сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     имя сетевого интерфейса
 */
string awh::Ethernet::iface(const unique_ptr <net::addr_t> & addr) const noexcept {
	// Результат работы функции
	string result = "";
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
				// Выводим сообщение об ошибке
				this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
			#endif
			// Выводим пустой результат
			return result;
		}
		// Перебираем все сетевые интерфейсы
		for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
			/**
			 * Определяем тип адреса
			 */
			switch(addr->size){
				// Если адрес является MAC-адресом
				case 6: {
					// Ищем MAC-адрес интерфейса
					if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)) {
						// Получаем указатель на структуру sockaddr_dl
						struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
						// Проверяем длину MAC-адреса
						if(sdl->sdl_alen == 6){
							// Получаем указатель на MAC-адрес
							const uint8_t * ptr = reinterpret_cast <const uint8_t *> (LLADDR(sdl));
							// Сравниваем MAC-адреса
							if(::memcmp(&awh_cast <net::addr_mac_t *> (addr.get())->address[0], ptr, 6) == 0){
								// Устанавливаем результат
								result = ifa->ifa_name;
								// Завершаем поиск
								break;
							}
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
					if(sin->sin_addr.s_addr == awh_cast <const net::addr_net_ipv4_t *> (addr.get())->address){
						// Устанавливаем результат
						result = ifa->ifa_name;
						// Завершаем поиск
						break;
					}
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
					if(::memcmp(&sin->sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (addr.get())->address[0], sizeof(in6_addr)) == 0){
						// Устанавливаем результат
						result = ifa->ifa_name;
						// Завершаем поиск
						break;
					}
				} break;
			}
		}
		// Освобождаем память списка сетевых интерфейсов
		::freeifaddrs(ptr);
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
	// Выводим пустой результат
	return result;
}
/**
 * @brief Метод заполнения источника сетевых адресов по имени сетевого интерфейса
 *
 * @param source объект источника сетевых адресов
 */
void awh::Ethernet::fillsource(net::src_t & source) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если название сетевого интерфейса передано
		if(!source.iface.empty()){
			// Если MAC-адрес ещё не заполнен
			if((::strncmp("lo", source.iface.c_str(), 2) != 0) &&
			   (::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], (uint8_t[6]){0}, 6) == 0)){
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0)
					// Выходим из функции
					return;
				// Результат работы функции
				bool result = false;
				// Перебираем все сетевые интерфейсы
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Пропускаем не совпадающие имена интерфейсов
					if((ifa->ifa_name == nullptr) || (::strcmp(ifa->ifa_name, source.iface.c_str()) != 0))
						// Переходим к следующему интерфейсу
						continue;
					// Ищем MAC-адрес интерфейса
					if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)) {
						// Получаем указатель на структуру sockaddr_dl
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
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Перебираем все сетевые интерфейсы
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
							// Если имя интерфейса совпадает
							if(this->_fmk->compare(ifa->ifa_name, source.iface)){
								// Копируем IP-адрес в результат
								awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = sin->sin_addr.s_addr;
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
					if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], (uint8_t[16]){0}, 16) == 0){
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (source.ip->size)), awh::log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to get list of network interfaces", awh::log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Перебираем все сетевые интерфейсы
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
							// Если имя интерфейса совпадает
							if(this->_fmk->compare(ifa->ifa_name, source.iface)){
								// Копируем IP-адрес в результат
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &sin->sin6_addr, sizeof(in6_addr));
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
		} else this->fillsource(event::node_t::NONE, source);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод заполнения источника сетевых адресов
 *
 * @param node   тип узла события
 * @param source объект источника сетевых адресов
 */
void awh::Ethernet::fillsource(const event::node_t node, net::src_t & source) const noexcept {
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
						struct sockaddr_in serv;
						// Обнуляем структуру подключения
						::memset(&serv, 0, sizeof(serv));
						// Указываем тип сетевого подключения IPv4
						serv.sin_family = AF_INET;
						// Устанавливаем порт DNS-сервера
						serv.sin_port = htons(53);
						// Указываем адреса IPv4 DNS-сервера
						::inet_pton(AF_INET, AWH_IPV4_RESOLVER, &serv.sin_addr);
						// Создаем сокет для проверки подключения
						const net::socket_t sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
						// Выполняем подключение к серверу
						int32_t conn = ::connect(sock, reinterpret_cast <const sockaddr *> (&serv), sizeof(serv));
						// Если подключение удачное
						if(conn > -1){
							// Создаем структуру имени
							struct sockaddr_in name;
							// Размер структуры
							socklen_t size = sizeof(name);
							// Запрашиваем имя сокета
							conn = ::getsockname(sock, reinterpret_cast <sockaddr *> (&name), &size);
							// Если ошибки нет
							if(conn > -1){
								// Устанавливаем хост сети
								awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = name.sin_addr.s_addr;
								// Устанавливаем название сетевого интерфейса
								source.iface = this->iface(source.ip);
								// Если название сетевого интерфейса получено
								if(!source.iface.empty())
									// Получаем MAC-адрес сетевого интерфейса
									this->fillsource(source);
							}
						}
						// Закрываем сокет
						::close(sock);
					} break;
					// Если адрес является IPv6
					case 16: {
						// Создаем структуру подключения сервера
						struct sockaddr_in6 serv;
						// Обнуляем структуру подключения
						::memset(&serv, 0, sizeof(serv));
						// Указываем тип сетевого подключения IPv4
						serv.sin6_family = AF_INET6;
						// Устанавливаем порт DNS сервера
						serv.sin6_port = htons(53);
						// Указываем адреса IPv6 DNS-сервера
						::inet_pton(AF_INET6, AWH_IPV6_RESOLVER, &serv.sin6_addr);
						// Создаем сокет для проверки подключения
						const net::socket_t sock = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
						// Выполняем подключение к серверу
						int32_t conn = ::connect(sock, reinterpret_cast <const sockaddr *> (&serv), sizeof(serv));
						// Если подключение удачное
						if(conn > -1){
							// Создаем структуру имени
							struct sockaddr_in6 name;
							// Размер структуры
							socklen_t size = sizeof(name);
							// Запрашиваем имя сокета
							conn = ::getsockname(sock, reinterpret_cast <sockaddr *> (&name), &size);
							// Если ошибки нет
							if(conn > -1){
								// Хост: просто копируем найденный адрес
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], name.sin6_addr.s6_addr, sizeof(name.sin6_addr.s6_addr));
								// Устанавливаем название сетевого интерфейса
								source.iface = this->iface(source.ip);
								// Если название сетевого интерфейса получено
								if(!source.iface.empty())
									// Получаем MAC-адрес сетевого интерфейса
									this->fillsource(source);
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
						 * Если операционной системой является MacOS X или FreeBSD
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
								// Выводим сообщение об ошибке
								this->_log->debug("Route sysctl estimate", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
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
								// Выводим сообщение об ошибке
								this->_log->debug("Actual retrieval of routing table", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
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
						// Переходим по всем сетевым интерфейсам
						for(begin = &buffer[0]; begin < end;){
							// Получаем указатель сетевого интерфейса
							struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
							// Переходим к следующему элементу
							begin += rtm->rtm_msglen;
							// Получаем текущее значение активного подключения
							struct sockaddr_inarp * sin = reinterpret_cast <struct sockaddr_inarp *> (rtm + 1);
							// Если сетевой интерфейс отличается от IPv4, пропускаем
							if(sin->sin_family != AF_INET)
								// Выполняем пропуск
								continue;
							// Получаем текущее значение аппаратного сетевого адреса
							struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (sin + 1);
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
						 * Если операционной системой является MacOS X или FreeBSD
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
								// Выводим сообщение об ошибке
								this->_log->debug("Route sysctl estimate", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
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
								// Выводим сообщение об ошибке
								this->_log->debug("Actual retrieval of routing table", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
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
						struct sockaddr_in6 addr;
						// Копируем IP-адрес в структуру подключения
						::memcpy(&addr.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(addr.sin6_addr));
						// Переходим по всем сетевым интерфейсам
						for(begin = &buffer[0]; begin < end;){
							// Получаем указатель сетевого интерфейса
							struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
							// Переходим к следующему элементу
							begin += rtm->rtm_msglen;
							// Если версия RTM протокола не соответствует, пропускаем
							if(rtm->rtm_version != RTM_VERSION)
								// Выполняем пропуск
								continue;
							// Получаем текущее значение активного подключения
							struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (begin + sizeof(rt_msghdr));
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
							if(::memcmp(&addr.sin6_addr, (uint8_t[16]){0}, 16) != 0){
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
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Временный объект для извлечения MAC-адреса
						net::src_t temp(::make_unique <net::addr_net_ipv4_t> ());
						// Получаем числовое значение IP-адреса
						const uint32_t addr = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
						// Перебираем все сетевые интерфейсы
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
									this->fillsource(source);
									// Прерываем цикл поиска
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Устанавливаем название сетевого интерфейса
								temp.iface = ifa->ifa_name;
								// Устанавливаем IP-адрес временного объекта
								awh_cast <net::addr_net_ipv4_t *> (temp.ip.get())->address = sin->sin_addr.s_addr;
								// Выполняем извлечение MAC-адреса временного объекта
								this->fillsource(temp);
								// Сравниваем MAC-адреса
								if(::memcmp(&awh_cast <net::addr_mac_t *> (temp.mac.get())->address[0], &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6) == 0){
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
								// Выводим сообщение об ошибке
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (node)), awh::log_t::flag_t::WARNING);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unable to get list of network interfaces", awh::log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Временный объект для извлечения MAC-адреса
						net::src_t temp(::make_unique <net::addr_net_ipv6_t> ());
						// Создаём объект подключения
						struct sockaddr_in6 addr;
						// Копируем IP-адрес в структуру подключения
						::memcpy(&addr.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(addr.sin6_addr));
						// Перебираем все сетевые интерфейсы
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
							if(::memcmp(&addr.sin6_addr, (uint8_t[16]){0}, 16) != 0){
								// Если IP-адрес совпадает с указанным IP-адресом
								if(IN6_ARE_ADDR_EQUAL(&addr.sin6_addr, &sin->sin6_addr)){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Получаем MAC-адрес сетевого интерфейса
									this->fillsource(source);
									// Прерываем цикл поиска
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Устанавливаем название сетевого интерфейса
								temp.iface = ifa->ifa_name;
								// Устанавливаем IP-адрес временного объекта
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (temp.ip.get())->address[0], &sin->sin6_addr, sizeof(in6_addr));
								// Выполняем извлечение MAC-адреса временного объекта
								this->fillsource(temp);
								// Сравниваем MAC-адреса
								if(::memcmp(&awh_cast <net::addr_mac_t *> (temp.mac.get())->address[0], &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6) == 0){
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (node), static_cast <uint16_t> (source.ip->size)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод заполнения источника сетевых адресов по заданной сети
 *
 * @param net    сетевой адрес подсети в хостовом порядке
 * @param source объект источника сетевых адресов
 */
void awh::Ethernet::fillsource(const unique_ptr <net::addr_t> & net, net::src_t & source) const noexcept {
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
				const net::addr_net_ipv4_t * network = awh_cast <const net::addr_net_ipv4_t *> (net.get());
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
							// Выводим сообщение об ошибке
							this->_log->debug(
								"Network address %u is not aligned to prefix %u", __PRETTY_FUNCTION__,
								std::make_tuple(htonl(network->address), static_cast <uint16_t> (network->prefix)),
								log_t::flag_t::WARNING, htonl(network->address), static_cast <uint16_t> (network->prefix)
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
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
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Unable to get list of network interfaces", __PRETTY_FUNCTION__,
							std::make_tuple(
								::inet_ntop(AF_INET, &network->address, buffer, sizeof(buffer)),
								static_cast <uint16_t> (network->prefix)
							), log_t::flag_t::WARNING
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
					#endif
					// Выходим из функции
					return;
				}
				// Устанавливаем префикс хостового адреса
				awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->prefix = network->prefix;
				// Перебираем все сетевые интерфейсы
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
						this->fillsource(source);
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
				const net::addr_net_ipv6_t * network = awh_cast <const net::addr_net_ipv6_t *> (net.get());
				// Проверяем корректность префикса сети
				if(network->prefix > 128)
					// Корректируем префикс сети
					awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->prefix = 128;
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
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Unable to get list of network interfaces", __PRETTY_FUNCTION__,
							std::make_tuple(
								::inet_ntop(AF_INET6, &network->address[0], buffer, sizeof(buffer)),
								static_cast <uint16_t> (network->prefix)
							), awh::log_t::flag_t::WARNING
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unable to get list of network interfaces", awh::log_t::flag_t::WARNING);
					#endif
					// Выходим из функции
					return;
				}
				// Устанавливаем префикс хостового адреса
				awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->prefix = network->prefix;
				// Временный IPv6-адрес
				struct in6_addr addr;
				// Перебираем все сетевые интерфейсы
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
						this->fillsource(source);
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
/**
 * @brief Метод проверки принадлежности IP-адреса подсети
 *
 * @param ip     проверяемый IP-адрес в хостовом порядке
 * @param net    сетевой адрес подсети в хостовом порядке
 * @param prefix префикс подсети
 * @return       результат проверки
 */
bool awh::Ethernet::isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если префикс равен нулю, то любой IP-адрес принадлежит подсети
		if(prefix == 0)
			// Выводим результат проверки
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(ip, net, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод сравнения двух IPv6-адресов по префиксу (в битах)
 *
 * @param a      Первый IPv6-адрес
 * @param b      Второй IPv6-адрес
 * @param length Длина префикса в битах
 * @return       Результат сравнения
 */
bool awh::Ethernet::ipv6PrefixEqual(const uint8_t * a, const uint8_t * b, const uint8_t length) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если длина префикса равна нулю, адреса считаются равными
		if(length == 0)
			// Выводим результат сравнения
			return true;
		// Вычисляем количество полных байтов и оставшихся битов
		size_t fullBytes = (length / 8);
		// Вычисляем количество битов в последнем байте
		uint8_t bitsInLast = (length % 8);
		// Сравниваем полные байты
		if(::memcmp(a, b, fullBytes) != 0)
			// Выводим результат сравнения
			return false;
		// Если нет оставшихся битов, адреса равны
		if(bitsInLast == 0)
			// Выводим результат сравнения
			return true;
		// Сравниваем оставшиеся биты в последнем байте
		const uint8_t mask = ((0xFF << (8 - bitsInLast)) & 0xFF);
		// Выводим результат сравнения
		return ((a[fullBytes] & mask) == (b[fullBytes] & mask));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(a, b, static_cast <uint16_t> (length)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки таймаута сокета
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @param msec  время таймаута в миллисекундах
 * @return      результат установки таймаута
 */
bool awh::Ethernet::timeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект таймаута
	struct timeval timeout;
	// Устанавливаем время в секундах
	timeout.tv_sec = (msec > 0 ? (msec / 1000) : 0);
	// Устанавливаем время счётчика (микросекунды)
	timeout.tv_usec = (msec > 0 ? ((msec % 1000) * 1000) : 0);
	/**
	 * Определяем флаг блокировки
	 */
	switch(static_cast <uint8_t> (event)){
		// Если необходимо установить таймаут на чтение
		case static_cast <uint8_t> (net::socket_event_t::READ): {
			// Выполняем установку таймаута на чтение данных из сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event), msec), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо установить таймаут на запись
		case static_cast <uint8_t> (net::socket_event_t::WRITE): {
			// Выполняем установку таймаута на запись данных в сокет
			if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event), msec), log_t::flag_t::CRITICAL, ::strerror(errno));
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
	// Все удачно
	return result;
}
/**
 * @brief Метод получения размера буфера
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @return      размер буфера сокета
 */
int32_t awh::Ethernet::bufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept {
	// Результат работы функции
	int32_t result = 0;
	/**
	 * Определяем флаг блокировки
	 */
	switch(static_cast <uint8_t> (event)){
		// Если необходимо получить размер буфера на чтение
		case static_cast <uint8_t> (net::socket_event_t::READ): {
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера
			if(::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо получить размер буфера на запись
		case static_cast <uint8_t> (net::socket_event_t::WRITE): {
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера
			if(::getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, ::strerror(errno));
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки размеров буфера
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @param size  размер буфера сокета
 * @return      установленный размер буфера сокета
 */
int32_t awh::Ethernet::bufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept {
	// Результат работы функции
	int32_t result = -1;
	/**
	 * Определяем флаг блокировки
	 */
	switch(static_cast <uint8_t> (event)){
		// Если необходимо установить размер буфера на чтение
		case static_cast <uint8_t> (net::socket_event_t::READ): {
			// Устанавливаем размер буфера на чтение
			if(::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event), size), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера на чтение
			if(::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event), size), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		} break;
		// Если необходимо установить размер буфера на запись
		case static_cast <uint8_t> (net::socket_event_t::WRITE): {
			// Устанавливаем размер буфера на запись
			if(::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event), size), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			// Получаем размер установленного размера буфера
			socklen_t length = sizeof(result);
			// Считываем установленный размер буфера
			if(::getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &result, &length) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (event), size), log_t::flag_t::CRITICAL, ::strerror(errno));
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод блокировки сигнала SIGILL
 *
 * @return результат работы функции
 */
bool awh::Ethernet::nosigill() const noexcept {
	// Результат работы функции
	bool result = false;
	// Создаем структуру активации сигнала
	struct sigaction act;
	// Зануляем структуру
	::memset(&act, 0, sizeof(act));
	// Устанавливаем макрос игнорирования сигнала
	act.sa_handler = SIG_IGN;
	// Устанавливаем флаги перезагрузки
	act.sa_flags = (SA_ONSTACK | SA_RESTART | SA_SIGINFO);
	// Устанавливаем блокировку сигнала
	if(!(result = !static_cast <bool> (::sigaction(SIGILL, &act, nullptr)))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Все удачно
	return result;
}
/**
* @brief Метод активации получения SCTP-событий для сокета
*
* @param sock    сетевой сокет
* @param options опции активации событий SCTP
* @return        результат работы функции
*/
bool awh::Ethernet::sctpEvents([[maybe_unused]] const net::socket_t sock, [[maybe_unused]] const uint16_t options) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Если операционной системой является FreeBSD
	 */
	#if __FreeBSD__
		/**
		 * Блокируем работу ненужной проверки (пока непонятно что с этим делать)
		 */
		#ifdef __AWH_DISABLED__
			// Создаём объект подписки на события
			struct sctp_event event;
			// Зануляем объект события
			::memset(&event, 0, sizeof(event));
			// Активируем получение входящих событий
			event.se_on = 1;
			// Устанавливаем тип события
			event.se_type = SCTP_ASSOC_CHANGE;
			// Устанавливаем идентификатор ассоциации
			event.se_assoc_id = SCTP_FUTURE_ASSOC;
			// Выполняем активацию получения событий SCTP для сокета
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_EVENT, &event, sizeof(event))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
			}
		#endif
		// Создаём объект подписки на события
		struct sctp_event_subscribe events;
		// Зануляем объект события
		::memset(&events, 0, sizeof(events));
		// Если необходимо установить уведомления о каждом входящем DATA-пакете
		if(options & event::sctp::notify::AWH_DATA_IO)
			// Устанавливаем уведомления о каждом входящем DATA-пакете
			events.sctp_data_io_event = 1;
		// Отключаем уведомления о каждом входящем DATA-пакете
		else events.sctp_data_io_event = 0;
		// Если необходимо установить события SCTP_ADDR_CHANGE
		if(options & event::sctp::notify::AWH_ADDRESS)
			// Устанавливаем события SCTP_ADDR_CHANGE
			events.sctp_address_event = 1;
		// Отключаем события SCTP_ADDR_CHANGE
		else events.sctp_address_event = 0;
		// Если необходимо установить события SCTP_SHUTDOWN_EVENT
		if(options & event::sctp::notify::AWH_SHUTDOWN)
			// Устанавливаем события SCTP_SHUTDOWN_EVENT
			events.sctp_shutdown_event = 1;
		// Отключаем события SCTP_SHUTDOWN_EVENT
		else events.sctp_shutdown_event = 0;
		// Если необходимо установить события SCTP_PEER_ERROR_EVENT
		if(options & event::sctp::notify::AWH_PEER_ERROR)
			// Устанавливаем события SCTP_PEER_ERROR_EVENT
			events.sctp_peer_error_event = 1;
		// Отключаем события SCTP_PEER_ERROR_EVENT
		else events.sctp_peer_error_event = 0;
		// Если необходимо установить события SCTP_SENDER_DRY_EVENT
		if(options & event::sctp::notify::AWH_SENDER_DRY)
			// Устанавливаем события SCTP_SENDER_DRY_EVENT
			events.sctp_sender_dry_event = 1;
		// Отключаем события SCTP_SENDER_DRY_EVENT
		else events.sctp_sender_dry_event = 0;
		// Если необходимо установить асоциационные события SCTP_ASSOC_CHANGE
		if(options & event::sctp::notify::AWH_ASSOCIATION)
			// Устанавливаем асоциационные события SCTP_ASSOC_CHANGE
			events.sctp_association_event = 1;
		// Отключаем асоциационные события SCTP_ASSOC_CHANGE
		else events.sctp_association_event = 0;
		/*
		// Если необходимо установить асоциационные события SCTP_ASSOC_RESET
		if(options & event::sctp::notify::AWH_ASSOC_RESET)
			// Устанавливаем асоциационные события SCTP_ASSOC_CHANGE
			events.sctp_assoc_reset_event = 1;
		// Отключаем асоциационные события SCTP_ASSOC_RESET
		else events.sctp_assoc_reset_event = 0;
		*/
		// Если необходимо установить события SCTP_SEND_FAILED_EVENT
		if(options & event::sctp::notify::AWH_SEND_FAILURE)
			// Устанавливаем события SCTP_SEND_FAILED_EVENT
			events.sctp_send_failure_event = 1;
		// Отключаем события SCTP_SEND_FAILED_EVENT
		else events.sctp_send_failure_event = 0;
		/*
		// Если необходимо установить асоциационные события SCTP_TERMINATION_EVENT
		if(options & event::sctp::notify::AWH_TERMINATION)
			// Устанавливаем асоциационные события SCTP_ASSOC_CHANGE
			events.sctp_termination_event = 1;
		// Отключаем асоциационные события SCTP_ASSOC_CHANGE
		else events.sctp_termination_event = 0;
		*/
		// Если необходимо установить асоциационные события SCTP_STREAM_RESET_EVENT
		if(options & event::sctp::notify::AWH_STREAM_RESET)
			// Устанавливаем асоциационные события SCTP_ASSOC_CHANGE
			events.sctp_stream_reset_event = 1;
		// Отключаем асоциационные события SCTP_ASSOC_CHANGE
		else events.sctp_stream_reset_event = 0;
		// Если необходимо установить события SCTP_AUTHENTICATION_INDICATION
		if(options & event::sctp::notify::AWH_AUTHENTICATION)
			// Устанавливаем события SCTP_AUTHENTICATION_INDICATION
			events.sctp_authentication_event = 1;
		// Отключаем события SCTP_AUTHENTICATION_INDICATION
		else events.sctp_authentication_event = 0;
		// Если необходимо установить события SCTP_PARTIAL_DELIVERY_EVENT
		if(options & event::sctp::notify::AWH_PARTIAL_DELIVERY)
			// Устанавливаем события SCTP_PARTIAL_DELIVERY_EVENT
			events.sctp_partial_delivery_event = 1;
		// Отключаем события SCTP_PARTIAL_DELIVERY_EVENT
		else events.sctp_partial_delivery_event = 0;
		// Если необходимо установить события SCTP_ADAPTATION_INDICATION
		if(options & event::sctp::notify::AWH_ADAPTATION_LAYER)
			// Устанавливаем события SCTP_ADAPTATION_INDICATION
			events.sctp_adaptation_layer_event = 1;
		// Отключаем события SCTP_ADAPTATION_INDICATION
		else events.sctp_adaptation_layer_event = 0;
		// Выполняем активацию получения событий SCTP для сокета
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения статуса SCTP сокета
 *
 * @param sock   сетевой сокет
 * @param status объект для извлечения статуса инициализации SCTP сокета
 * @return       результат работы функции
 */
bool awh::Ethernet::sctpStatus([[maybe_unused]] const net::socket_t sock, [[maybe_unused]] net::sctp::status_t & status) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Если операционной системой является FreeBSD
	 */
	#if __FreeBSD__
		// Размер структуры статуса SCTP сокета
		socklen_t length = sizeof(status);
		// Выполняем инициализацию SCTP сокета
		if(!(result = !static_cast <bool> (::getsockopt(sock, IPPROTO_SCTP, SCTP_STATUS, &status, &length)))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * @brief Метод инициализации SCTP сокета
 *
 * @param sock   сетевой сокет
 * @param intmes параметры инициализации SCTP сокета
 * @return       результат работы функции
 */
bool awh::Ethernet::sctpInitMessages([[maybe_unused]] const net::socket_t sock, [[maybe_unused]] const net::sctp::intmes_t & intmes) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Если операционной системой является FreeBSD
	 */
	#if __FreeBSD__
		// Выполняем инициализацию SCTP сокета
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &intmes, sizeof(intmes))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
* @brief Метод получения кода ошибки
*
* @param sock сетевой сокет
* @return     код ошибки на сокете если присутствует
*/
int32_t awh::Ethernet::error(const net::socket_t sock) const noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Размер кода ошибки
	socklen_t size = sizeof(result);
	// Если мы получили ошибку, выходим сообщение
	if(::getsockopt(sock, SOL_SOCKET, SO_ERROR, &result, &size) != 0){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод активации TCP/CORK
 *
 * @param sock сетевой сокет
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Ethernet::cork(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать алгоритм TCP/CORK
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать алгоритм TCP/CORK
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	// Включаем/отключаем или отключаем алгоритм TCP/CORK
	if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NOPUSH, &on, sizeof(on))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Все удачно
	return result;
}
/**
 * @brief Метод отключения алгоритма Нейгла
 *
 * @param sock сетевой сокет
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Ethernet::nodelay(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать алгоритм Нейгла
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать алгоритм Нейгла
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	/**
	 * Если операционной системой является FreeBSD
	 */
	#if __FreeBSD__
		// Переменная для хранения протокола сокета
		int32_t protocol = 0;
		// Длина протокола сокета
		socklen_t length = sizeof(protocol);
		// Получаем протокол сокета
		if(::getsockopt(sock, SOL_SOCKET, SO_PROTOCOL, &protocol, &length) == 0){
			/**
			 * Определяем протокол сокета
			 */
			switch(protocol){
				// Если протокол TCP
				case IPPROTO_TCP: {
					// Активируем/деактивируем алгоритм Нейгла
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
				// Если протокол SCTP
				case IPPROTO_SCTP: {
					// Активируем/деактивируем алгоритм Нейгла
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_NODELAY, &on, sizeof(on))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
			}
		// Если возникает ошибка получения протокола сокета
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
			#endif
		}
	/**
	 * Для остальных операционных систем
	 */
	#else
		// Активируем/деактивируем алгоритм Нейгла
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * @brief Метод включающий или отключающий режим отображения IPv4 => IPv6
 *
 * @param sock сетевой сокет
 * @param mode режим активации или деактивации
 * @return     результат работы функции
 */
bool awh::Ethernet::ipv6only(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо включить режим отображения IPv4 => IPv6
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо отключить режим отображения IPv4 => IPv6
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	// Разрешаем/запрещаем отображение IPv4 => IPv6
	if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод разрешающий повторно использовать сокет после его удаления
 *
 * @param sock сетевой сокет
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Ethernet::reuseaddr(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать использовать тот же сокет после его удаления
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать использовать тот же сокет после его удаления
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	// Разрешаем/запрещаем повторно использовать тот же сокет после отключения
	if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод разрешающий повторно использовать один и тот же порт для нескольких сокетов
 *
 * @param sock сетевой сокет
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Ethernet::reuseport(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать использование одного и того же порта для нескольких сокетов
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать использование одного и того же порта для нескольких сокетов
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	// Разрешаем/запрещаем использовать один и тот же порт для нескольких сокетов
	if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод игнорирования отключения сигнала записи в убитый сокет
 *
 * @param sock сетевой сокет
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Ethernet::nosigpipe(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо игнорировать отключение сигнала записи в убитый сокет
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать игнорирование отключения сигнала записи в убитый сокет
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	// Устанавливаем/снимаем игнорирование отключения сигнала записи в убитый сокет
	if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод разрешения широковещательного адреса
 *
 * @param sock сетевой сокет
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Ethernet::broadcast(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать широковещательный адрес
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать широковещательный адрес
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	// Активируем/деактивируем широковещательный адрес
	if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки блокирующего сокета
 *
 * @param sock сетевого сокета
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Ethernet::noblocking(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Флаги сетевого сокета
	int32_t flags = 0;
	// Результат работы функции
	bool result = false;
	// Получаем флаги сетевого сокета
	if(!(result = ((flags = ::fcntl(sock, F_GETFL, nullptr)) >= 0))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return result;
	}
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо перевести сокет в блокирующий режим
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
			// Если флаг ещё не установлен
			if(!(result = (flags & O_NONBLOCK))){
				// Устанавливаем неблокирующий режим
				if(!(result = (::fcntl(sock, F_SETFL, flags | O_NONBLOCK) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		} break;
		// Если необходимо перевести сокет в неблокирующий режим
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
			// Если флаг уже установлен
			if(!(result = !(flags & O_NONBLOCK))){
				// Снимаем неблокирующий режим
				if(!(result = (::fcntl(sock, F_SETFL, flags ^ O_NONBLOCK) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки режима автоматического закрытия файлового дескриптора при вызове exec
 *
 * @param sock сетевой сокет
 * @param mode режим активации или деактивации
 * @return     результат работы функции
 */
bool awh::Ethernet::closeonexec(const net::socket_t sock, const net::socket_mode_t mode) const noexcept {
	// Флаги сетевого сокета
	int32_t flags = 0;
	// Результат работы функции
	bool result = false;
	// Получаем флаги сетевого сокета
	if(!(result = ((flags = ::fcntl(sock, F_GETFD, nullptr)) >= 0))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return result;
	}
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать режим закрытия сокета после запуска
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
			// Если флаг ещё не установлен
			if(!(result = (flags & FD_CLOEXEC))){
				// Устанавливаем режим закрытия сокета после запуска
				if(!(result = (::fcntl(sock, F_SETFD, flags | FD_CLOEXEC) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		} break;
		// Если необходимо деактивировать режим закрытия сокета после запуска
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
			// Если флаг уже установлен
			if(!(result = !(flags & FD_CLOEXEC))){
				// Снимаем режим закрытия сокета после запуска
				if(!(result = (::fcntl(sock, F_SETFD, flags ^ FD_CLOEXEC) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод устанавливает постоянное подключение на сокет
 *
 * @param sock  сетевой сокет
 * @param cnt   максимальное количество попыток
 * @param idle  время через которое происходит проверка подключения
 * @param intvl время между попытками
 * @return      результат работы функции
 */
bool awh::Ethernet::keepalive(const net::socket_t sock, const int32_t cnt, const int32_t idle, const int32_t intvl) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если максимальное количество попыток передано неправильно
	if(cnt < 0)
		// Выполняем компенсацию
		const_cast <int32_t &> (cnt) = 0;
	// Если время через которое происходит проверка подключения передано неправильно
	if(idle < 0)
		// Выполняем компенсацию
		const_cast <int32_t &> (idle) = 0;
	// Если время между попытками передано неправильно
	if(intvl < 0)
		// Выполняем компенсацию
		const_cast <int32_t &> (intvl) = 0;
	// Устанавливаем параметр
	int32_t keepAlive = 1;
	// Активация постоянного подключения
	if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return result;
	}
	// Максимальное количество попыток
	if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
		// Выходим из функции
		return result;
	}
	/**
	 * Если мы работаем в MacOS X
	 */
	#if __APPLE__
		// Время через которое происходит проверка подключения
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), awh::log_t::flag_t::WARNING, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
			#endif
			// Выходим из функции
			return result;
		}
	/**
	 * Если мы работаем в Linux, FreeBSD, NetBSD или OpenBSD или Sun Solaris
	 */
	#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Время через которое происходит проверка подключения
		if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), awh::log_t::flag_t::WARNING, ::strerror(errno));
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
			#endif
			// Выходим из функции
			return result;
		}
	#endif
	// Время между попытками
	if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl))))){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), awh::log_t::flag_t::WARNING, ::strerror(errno));
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки включения/отключения заголовков в сокете
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param mode   режим активации или деактивации
 * @return       результат работы функции
 */
bool awh::Ethernet::hdrinclude(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать заголовки в сокете
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать заголовки в сокете
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Активируем/деактивируем заголовки в сокете
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Активируем/деактивируем заголовки в сокете (В Linux нужно использовать IPV6_HDRINCL)
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IP_HDRINCL, &on, sizeof(on))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки сетевого интерфейса для multicast пакетов
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param name   имя сетевого интерфейса
 * @return       результат работы функции
 */
bool awh::Ethernet::multicastIface(const net::socket_t sock, const event::family_t family, const string & name) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если название сетевого интерфейса не пустое
	if(!name.empty()){
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), name), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
					#endif
					// Выводим пустой результат
					return result;
				}
				// Перебираем все сетевые интерфейсы
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
					// Если имя интерфейса совпадает
					if(this->_fmk->compare(ifa->ifa_name, name)){
						// Создаём объект сетевого интерфейса
						struct in_addr iface = {};
						// Присваиваем найденный IP-адрес
						iface.s_addr = sin->sin_addr.s_addr;
						// Устанавливаем сетевой интерфейс для multicast пакетов
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), name), awh::log_t::flag_t::WARNING, ::strerror(errno));
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
						// Выходим из цикла
						break;
					}
				}
				// Освобождаем память от списка сетевых интерфейсов
				::freeifaddrs(ptr);
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Получаем индекс сетевого интерфейса по его имени
				const uint32_t index = ::if_nametoindex(name.c_str());
				// Устанавливаем сетевой интерфейс для multicast пакетов
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index, sizeof(index))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), name), awh::log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
		}
	// Если название сетевого интерфейса пустое
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Interface name is empty", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), name), awh::log_t::flag_t::WARNING);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Interface name is empty", awh::log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки режима обратной петли для multicast пакетов
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param mode   режим активации или деактивации
 * @return       результат работы функции
 */
bool awh::Ethernet::multicastLoopback(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept {
	// Параметр установки типа сокета
	int32_t on = 0;
	/**
	 * Определяем режим блокировки
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать режим обратной петли для multicast пакетов
		case static_cast <uint8_t> (net::socket_mode_t::ENABLED): on = 1; break;
		// Если необходимо деактивировать режим обратной петли для multicast пакетов
		case static_cast <uint8_t> (net::socket_mode_t::DISABLED): on = 0; break;
	}
	// Результат работы функции
	bool result = false;
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Активируем/деактивируем режим обратной петли для multicast пакетов
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Активируем/деактивируем режим обратной петли для multicast пакетов
			if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &on, sizeof(on))))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
				#endif
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param sock     сетевой сокет
 * @param family   семейство протоколов (IPv4 или IPv6)
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @param hops     максимальное количество хопов
 * @return         результат работы функции
 */
bool awh::Ethernet::hops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const event::hops_t hops) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			/**
			 * Определяем режим трансляции пакетов
			 */
			switch(static_cast <uint8_t> (delivery)){
				// Если необходимо установить максимальное количество хопов для unicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::UNICAST):
				// Если необходимо установить максимальное количество хопов для broadcast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::BROADCAST): {
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_TTL, &hops, sizeof(hops))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), awh::log_t::flag_t::WARNING, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
				// Если необходимо установить максимальное количество хопов для multicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::MULTICAST): {
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &hops, sizeof(hops))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), awh::log_t::flag_t::WARNING, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
			}
		} break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			/**
			 * Определяем режим трансляции пакетов
			 */
			switch(static_cast <uint8_t> (delivery)){
				// Если необходимо установить максимальное количество хопов для unicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::UNICAST):
				// Если необходимо установить максимальное количество хопов для broadcast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::BROADCAST): {
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), awh::log_t::flag_t::WARNING, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
				// Если необходимо установить максимальное количество хопов для multicast пакетов
				case static_cast <uint8_t> (event::delivery_mode_t::MULTICAST): {
					// Устанавливаем максимальное количество хопов, через которые может пройти пакет
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), awh::log_t::flag_t::WARNING, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				} break;
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод активации/деактивации мультикаст группы события
 *
 * @param sock   сетевой сокет
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @return       результат работы функции
 */
bool awh::Ethernet::membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если тип IP-адресов совпадает
		if(group->size == source->size){
			/**
			 * Определяем режим блокировки
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо активировать заголовки в сокете
				case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
					/**
					 * Определяем тип адреса
					 */
					switch(group->size){
						// Если адрес является IPv4
						case 4: {
							// Формируем объект multicast request
							struct ip_mreq mreq;
							// Устанавливаем адрес multicast-группы
							mreq.imr_multiaddr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (group)->address;
							// Устанавливаем адрес сетевого интерфейса
							mreq.imr_interface.s_addr = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
							// Добавляем новую multicast-группу к сокету
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
						// Если адрес является IPv6
						case 16: {
							// Формируем объект multicast request
							struct ipv6_mreq mreq;
							// Устанавливаем адрес multicast-группы
							::memcpy(&mreq.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(mreq.ipv6mr_multiaddr));
							// Устанавливаем адрес сетевого интерфейса
							::memcpy(&mreq.ipv6mr_interface, &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], sizeof(mreq.ipv6mr_interface));
							// Добавляем новую multicast-группу к сокету
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
					}
				} break;
				// Если необходимо деактивировать заголовки в сокете
				case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
					/**
					 * Определяем тип адреса
					 */
					switch(group->size){
						// Если адрес является IPv4
						case 4: {
							// Формируем объект multicast request
							struct ip_mreq mreq;
							// Устанавливаем адрес multicast-группы
							mreq.imr_multiaddr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (group)->address;
							// Устанавливаем адрес сетевого интерфейса
							mreq.imr_interface.s_addr = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
							// Удаляем multicast-группу из сокета
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
						// Если адрес является IPv6
						case 16: {
							// Формируем объект multicast request
							struct ipv6_mreq mreq;
							// Устанавливаем адрес multicast-группы
							::memcpy(&mreq.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(mreq.ipv6mr_multiaddr));
							// Устанавливаем адрес сетевого интерфейса
							::memcpy(&mreq.ipv6mr_interface, &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], sizeof(mreq.ipv6mr_interface));
							// Удаляем multicast-группу из сокета
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), awh::log_t::flag_t::WARNING, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
					}
				} break;
			}
		// Если IP-адреса отличаются
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("It is impossible to work with a multicast group because the IP address types are different", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("It is impossible to work with a multicast group because the IP address types are different", log_t::flag_t::CRITICAL, error.what());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
uint16_t awh::Ethernet::checksum(const event::family_t family, const event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length) const noexcept {
	// Результат работы функции
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
			// Оригинальный checksum
			uint16_t original = 0;
			// Указатель на checksum в транспортном заголовке
			uint16_t * checksum = nullptr;
			// Псевдозаголовок
			unique_ptr <uint8_t []> pseudo = nullptr;
			/**
			 * Определяем протокол
			 */
			switch(static_cast <uint8_t> (protocol)){
				// Если протокол определён как TCP
				case static_cast <uint8_t> (event::protocol_t::TCP):
					// Сохраняем оригинальный checksum
					checksum = const_cast <uint16_t *> (&(reinterpret_cast <const struct tcphdr *> (transport))->th_sum);
				break;
				// Если протокол определён как UDP
				case static_cast <uint8_t> (event::protocol_t::UDP):
					// Сохраняем оригинальный checksum
					checksum = const_cast <uint16_t *> (&(reinterpret_cast <const struct udphdr *> (transport))->uh_sum);
				break;
				// Для неподдерживаемого протокола
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Unsupported protocol for checksum calculation", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unsupported protocol for checksum calculation", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из функции
					return result;
				}
			}
			// Сохраняем оригинал
			original = (* checksum);
			// Обнуляем checksum в транспортном заголовке
			(* checksum) = 0;
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
			// Копируем транспортный заголовок + данные
			::memcpy(pseudo.get() + pseudoSize, transport, length);
			// Вычисляем контрольную сумму
			result = ::checksum(pseudo.get(), totalSize);
			// Восстанавливаем оригинал (опционально — обычно вызывающий сам запишет результат)
			(* checksum) = original;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::Ethernet::Ethernet(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
	/**
	 * Выполняем настройку сетевых параметров
	 */
	this->netboost();
}
/**
 * @brief Деструктор
 *
 */
awh::Ethernet::~Ethernet() noexcept {}
