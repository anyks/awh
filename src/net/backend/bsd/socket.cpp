/**
 * @file: socket.cpp
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

/**
 * Подключаем системные заголовки
 */
#include <fcntl.h>
#include <signal.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

/**
 * Для операционной системы FreeBSD
 */
#if __FreeBSD__
	/**
	 * Подключаем заголовочные файлы для работы с SCTP протоколом
	 */
	#include <netinet/sctp.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/eth/socket.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения кода ошибки
 *
 * @param sock сетевой сокет
 * @return     код ошибки на сокете если присутствует
 */
int32_t awh::eth::Socket::error(const net::socket_t sock) const noexcept {
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
 * @brief Метод установки таймаута сокета
 *
 * @param sock  сетевой сокет
 * @param event событие сокета
 * @param msec  время таймаута в миллисекундах
 * @return      результат установки таймаута
 */
bool awh::eth::Socket::timeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept {
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
int32_t awh::eth::Socket::bufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept {
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
int32_t awh::eth::Socket::bufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept {
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
 * @brief Метод установки сетевого интерфейса для multicast пакетов
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param ifname имя сетевого интерфейса
 * @return       результат работы функции
 */
bool awh::eth::Socket::multicastIface(const net::socket_t sock, const event::family_t family, const string & ifname) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если название сетевого интерфейса не пустое
	if(!ifname.empty()){
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
						this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), ifname), log_t::flag_t::WARNING);
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
					if(this->_fmk->compare(ifa->ifa_name, ifname)){
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), ifname), log_t::flag_t::WARNING, ::strerror(errno));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
				const uint32_t index = ::if_nametoindex(ifname.c_str());
				// Устанавливаем сетевой интерфейс для multicast пакетов
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index, sizeof(index))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), ifname), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
			this->_log->debug("Interface name is empty", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (family), ifname), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Interface name is empty", log_t::flag_t::WARNING);
		#endif
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
bool awh::eth::Socket::keepalive(const net::socket_t sock, const int32_t cnt, const int32_t idle, const int32_t intvl) const noexcept {
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), log_t::flag_t::WARNING, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), log_t::flag_t::WARNING, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), log_t::flag_t::WARNING, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), log_t::flag_t::WARNING, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, cnt, idle, intvl), log_t::flag_t::WARNING, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки опций сокета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param mode   режим активации или деактивации
 * @param option опция сокета
 * @return       результат работы функции
 */
bool awh::eth::Socket::setoption(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode, const uint16_t option) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если сокет корректен
	if(sock != net::invalid_socket_t){
		/**
		 * Определяем опции сокета которые необходимо установить
		 */
		switch(option){
			// Если необходимо установить опцию HDRINCL
			case event::options::HDRINCL: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Определяем семейство события
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4): {
						// Активируем/деактивируем заголовки в сокете
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &flags, sizeof(flags))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6): {
						// Активируем/деактивируем заголовки в сокете (В Linux нужно использовать IPV6_HDRINCL)
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IP_HDRINCL, &flags, sizeof(flags))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
				}
			} break;
			// Если необходимо установить опцию TCP CORK
			case event::options::TCP_CORK: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Включаем/отключаем или отключаем алгоритм TCP/CORK
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NOPUSH, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо отключить алгоритм Нейгла
			case event::options::TCP_NO_DELAY: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
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
								if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags))))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
											sock,
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (mode),
											option
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
									#endif
								}
							} break;
							// Если протокол SCTP
							case IPPROTO_SCTP: {
								// Активируем/деактивируем алгоритм Нейгла
								if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_SCTP, SCTP_NODELAY, &flags, sizeof(flags))))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
											sock,
											static_cast <uint16_t> (family),
											static_cast <uint16_t> (mode),
											option
										), log_t::flag_t::WARNING,
										::strerror(errno)
									);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				/**
				 * Для остальных операционных систем
				 */
				#else
					// Активируем/деактивируем алгоритм Нейгла
					if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags))))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
								sock,
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (mode),
								option
							), log_t::flag_t::WARNING,
							::strerror(errno)
						);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
						#endif
					}
				#endif
			} break;
			// Если необходимо установить опцию IPV6 ONLY
			case event::options::IPV6_ONLY: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Разрешаем/запрещаем отображение IPv4 => IPv6
				if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо отключить сигнал SIGILL
			case event::options::NO_SIGILL: {
				// Создаем структуру активации сигнала
				struct sigaction act{0};
				// Устанавливаем флаги перезагрузки
				act.sa_flags = (SA_ONSTACK | SA_RESTART | SA_SIGINFO);
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED): {
						// Устанавливаем макрос игнорирования сигнала
						act.sa_handler = SIG_IGN;
						// Устанавливаем блокировку сигнала
						if(!(result = !static_cast <bool> (::sigaction(SIGILL, &act, nullptr)))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED): {
						// Устанавливаем макрос по умолчанию для сигнала
						act.sa_handler = SIG_DFL;
						// Снимаем блокировку сигнала
						if(!(result = !static_cast <bool> (::sigaction(SIGILL, &act, nullptr)))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
				}
			} break;
			// Если необходимо установить опцию BROADCAST
			case event::options::BROADCAST: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Активируем/деактивируем широковещательный адрес
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо отключить сигнал SIGPIPE
			case event::options::NO_SIGPIPE: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Устанавливаем/снимаем игнорирование отключения сигнала записи в убитый сокет
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо перевести сокет в неблокирующий режим
			case event::options::NO_IO_BLOCK:
			// Если необходимо перевести сокет в полублокирующий режим
			case event::options::SM_IO_BLOCK: {
				// Флаги установки опции
				int32_t flags = 0;
				// Получаем флаги сетевого сокета
				if(!(result = ((flags = ::fcntl(sock, F_GETFL, nullptr)) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						}
					} break;
				}
			} break;
			// Если необходимо установить опцию переиспользования адреса
			case event::options::REUSE_ADDR: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Разрешаем/запрещаем повторно использовать тот же сокет после отключения
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо установить опцию переиспользования порта
			case event::options::REUSE_PORT: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				// Разрешаем/запрещаем использовать один и тот же порт для нескольких сокетов
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &flags, sizeof(flags))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо установить опцию CLOSE ON EXEC
			case event::options::CLOSE_ON_EXEC: {
				// Флаги установки опции
				int32_t flags = 0;
				// Получаем флаги сетевого сокета
				if(!(result = ((flags = ::fcntl(sock, F_GETFD, nullptr)) >= 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							sock,
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (mode),
							option
						), log_t::flag_t::WARNING,
						::strerror(errno)
					);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
										sock,
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (mode),
										option
									), log_t::flag_t::WARNING,
									::strerror(errno)
								);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						}
					} break;
				}
			} break;
			// Если необходимо установить опцию MULTICAST LOOPBACK
			case event::options::MULTICAST_LOOPBACK: {
				// Флаги установки опции
				int32_t flags = 0;
				/**
				 * Определяем режим блокировки
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::ENABLED):
						// Устанавливаем флаг активации
						flags = 1;
					break;
					// Если необходимо деактивировать заголовки в сокете
					case static_cast <uint8_t> (net::socket_mode_t::DISABLED):
						// Устанавливаем флаг деактивации
						flags = 0;
					break;
				}
				/**
				 * Определяем семейство события
				 */
				switch(static_cast <uint8_t> (family)){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4): {
						// Активируем/деактивируем режим обратной петли для multicast пакетов
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &flags, sizeof(flags))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6): {
						// Активируем/деактивируем режим обратной петли для multicast пакетов
						if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &flags, sizeof(flags))))){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
									sock,
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (mode),
									option
								), log_t::flag_t::WARNING,
								::strerror(errno)
							);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
							#endif
						}
					} break;
				}
			} break;
		}
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
bool awh::eth::Socket::hops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const event::hops_t hops) const noexcept {
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), log_t::flag_t::WARNING, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), log_t::flag_t::WARNING, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), log_t::flag_t::WARNING, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (hops)), log_t::flag_t::WARNING, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
bool awh::eth::Socket::membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept {
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, ::strerror(errno));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
						// Если адрес является IPv6
						case 16: {
							// Формируем объект multicast request
							struct ipv6_mreq mreq;
							// Устанавливаем адрес multicast-группы
							::memcpy(&mreq.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(mreq.ipv6mr_multiaddr));
							// Устанавливаем индекс интерфейса по умолчанию
							mreq.ipv6mr_interface = 0;
							// Получаем список сетевых интерфейсов
							struct ifaddrs * ptr = nullptr;
							// Выполняем получение списка сетевых интерфейсов
							if(::getifaddrs(&ptr) == 0){
								// Перебираем все сетевые интерфейсы
								for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
									// Если не IPv6 адреса
									if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
										// Переходим к следующему интерфейсу
										continue;
									// Получаем указатель на структуру IPv6
									struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
									// Если адреса совпадают
									if(::memcmp(&sin->sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], sizeof(in6_addr)) == 0){
										// Получаем индекс интерфейса
										mreq.ipv6mr_interface = ::if_nametoindex(ifa->ifa_name);
										// Завершаем поиск
										break;
									}
								}
								// Освобождаем память списка сетевых интерфейсов
								::freeifaddrs(ptr);
							}
							// Добавляем новую multicast-группу к сокету
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, ::strerror(errno));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, ::strerror(errno));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
								#endif
							}
						} break;
						// Если адрес является IPv6
						case 16: {
							// Формируем объект multicast request
							struct ipv6_mreq mreq;
							// Устанавливаем адрес multicast-группы
							::memcpy(&mreq.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(mreq.ipv6mr_multiaddr));
							// Удаляем multicиндекс интерфейса по умолчанию
							mreq.ipv6mr_interface = 0;
							// Получаем список сетевых интерфейсов
							struct ifaddrs * ptr = nullptr;
							// Выполняем получение списка сетевых интерфейсов
							if(::getifaddrs(&ptr) == 0){
								// Перебираем все сетевые интерфейсы
								for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
									// Если не IPv6 адреса
									if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
										// Переходим к следующему интерфейсу
										continue;
									// Получаем указатель на структуру IPv6
									struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
									// Если адреса совпадают
									if(::memcmp(&sin->sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], sizeof(in6_addr)) == 0){
										// Получаем индекс интерфейса
										mreq.ipv6mr_interface = ::if_nametoindex(ifa->ifa_name);
										// Завершаем поиск
										break;
									}
								}
								// Освобождаем память списка сетевых интерфейсов
								::freeifaddrs(ptr);
							}
							// Удаляем multicast-группу из сокета
							if(!(result = !static_cast <bool> (::setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq))))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, ::strerror(errno));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
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
 * @brief Метод создания сокета
 *
 * @param family семейство протоколов сокета
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @return       созданный сокет
 */
awh::net::socket_t awh::eth::Socket::create(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Если сокет принадлежит к типу STREAM
					case static_cast <uint8_t> (event::type_t::STREAM):
						// Выводим созданный сокет
						return ::socket(AF_UNIX, SOCK_STREAM, 0);
					// Если сокет принадлежит к типу DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM):
						// Выводим созданный сокет
						return ::socket(AF_UNIX, SOCK_DGRAM, 0);
					// Если сокет принадлежит к типу SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						/**
						 * Для операционной системы MacOS X, NetBSD, OpenBSD
						 */
						#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
							// Выводим созданный сокет
							return ::socket(AF_UNIX, SOCK_DGRAM, 0);
						/**
						 * Для операционной системы FreeBSD
						 */
						#elif __FreeBSD__
							// Выводим созданный сокет
							return ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
						#endif
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"A socket for a Unix event cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("A socket for a Unix event cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
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
					// Если сокет принадлежит к типу RAW
					case static_cast <uint8_t> (event::type_t::RAW): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_RAW, 0);
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::RAW):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_RAW, 0);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_RAW, IPPROTO_TCP);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_RAW, IPPROTO_UDP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"RAW socket type only supports UDP or ICMP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("RAW socket type only supports UDP or ICMP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
						}
					} break;
					// Если сокет принадлежит к типу STREAM
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
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_STREAM, 0);
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_STREAM, 0);
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
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
					// Если сокет принадлежит к типу DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства IPv4
							case static_cast <uint8_t> (event::family_t::IPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_DGRAM, 0);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Выводим созданный сокет
										return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_DGRAM, 0);
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"DGRAM socket type only supports UDP, DTLS or ICMP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("DGRAM socket type only supports UDP, DTLS or ICMP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
							#endif
						}
					} break;
					// Если сокет принадлежит к типу SEQPACKET
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
								switch(static_cast <uint8_t> (proto)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										/**
										 * Для операционной системы MacOS X, NetBSD, OpenBSD
										 */
										#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
											// Выводим созданный сокет
											return ::socket(AF_INET, SOCK_DGRAM, 0);
										/**
										 *Для операционной системы FreeBSD
										 */
										#elif __FreeBSD__
											// Выводим созданный сокет
											return ::socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
										#endif
									} break;
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (proto)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP):
										// Выводим созданный сокет
										return ::socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
									// Если установлен другой протокол
									default: ok = false;
								}
							} break;
						}
						// Если сокет не создан
						if(!ok){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
									__PRETTY_FUNCTION__,
									std::make_tuple(
										static_cast <uint16_t> (family),
										static_cast <uint16_t> (type),
										static_cast <uint16_t> (proto)
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
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"A socket for an IP event cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("A socket for an IP event cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для неизвестного семейства
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(
						"A socket cannot be created, because family it belongs to is not defined",
						__PRETTY_FUNCTION__,
						std::make_tuple(
							static_cast <uint16_t> (family),
							static_cast <uint16_t> (type),
							static_cast <uint16_t> (proto)
						), log_t::flag_t::WARNING
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("A socket cannot be created, because family it belongs to is not defined", log_t::flag_t::WARNING);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__,
				std::make_tuple(
					static_cast <uint16_t> (family),
					static_cast <uint16_t> (type),
					static_cast <uint16_t> (proto)
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
	// Выводим результат по умолчанию
	return net::invalid_socket_t;
}
/**
 * @brief Метод создания пары сокетов
 *
 * @param family семейство протоколов сокета
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @return       созданный сокет
 */
array <awh::net::socket_t, 2> awh::eth::Socket::pair(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept {
	// Результат работы функции
	array <net::socket_t, 2> result = {
		net::invalid_socket_t,
		net::invalid_socket_t
	};
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства PIPE
			case static_cast <uint8_t> (event::family_t::PIPE): {
				// Выполняем инициализацию файловых дескрипторов
				if(::pipe(&result[0]) != 0){
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
								static_cast <uint16_t> (proto)
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
					// Если сокет принадлежит к типу STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_STREAM, 0, &result[0]) != 0){
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
										static_cast <uint16_t> (proto)
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
					// Если сокет принадлежит к типу DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_DGRAM, 0, &result[0]) != 0){
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
										static_cast <uint16_t> (proto)
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
					// Если сокет принадлежит к типу SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						/**
						 * Для операционной системы MacOS X, NetBSD, OpenBSD
						 */
						#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
							// Выполняем инициализацию файловых дескрипторов
							if(::socketpair(AF_UNIX, SOCK_DGRAM, 0, &result[0]) != 0){
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
											static_cast <uint16_t> (proto)
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
						/**
						 * Для остальных операционных систем
						 */
						#else
							// Выполняем инициализацию файловых дескрипторов
							if(::socketpair(AF_UNIX, SOCK_SEQPACKET, 0, &result[0]) != 0){
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
											static_cast <uint16_t> (proto)
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
						#endif
					} break;
					// Для неизвестного типа сокета
					default: {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"An event for a Unix event cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a Unix event cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Создаём нужное количество сокетов
				for(net::socket_t & socket : result)
					// Создаём сокет по указанным параметрам
					socket = this->create(family, type, proto);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__,
				std::make_tuple(
					static_cast <uint16_t> (family),
					static_cast <uint16_t> (type),
					static_cast <uint16_t> (proto)
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
awh::eth::Socket::Socket(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::eth::Socket::~Socket() noexcept {}
