/**
 * @file: if.cpp
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
#include <cstdio>
#include <cerrno>
#include <memory>
#include <cstring>
#include <cstdlib>

/**
 * Подключаем системные заголовки
 */
#include <netinet/if_ether.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <fcntl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/eth/if.hpp>

/**
 * Для операционной системы MacOS X, NetBSD, OpenBSD
 */
#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
	/**
	 * Подключаем заголовочные файлы для работы с UTUN интерфейсами
	 */
	#include <net/if_utun.h>
/**
 * Для операционной системы FreeBSD
 */
#elif __FreeBSD__
	/**
	 * Подключаем заголовочные файлы для работы с TUN интерфейсами
	 */
	#include <net/if_tun.h>
	#include <netinet/in_var.h>
#endif

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод удаления сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат удаления сетевого интерфейса
 */
bool awh::eth::Interface::destroy(const string & name) const noexcept {

}
/**
 * @brief Метод получения списка сетевых интерфейсов системы
 *
 * @return список сетевых интерфейсов системы
 */
unordered_set <string> awh::eth::Interface::available() const noexcept {
	// Результат работы функции
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
		for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next)
			// Добавляем имя сетевого интерфейса в результирующий список
			result.emplace(ifa->ifa_name);
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
	// Выводиим результат
	return result;
}
/**
 * @brief Метод создания TUN/TAP сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     дескриптор созданного TUN/TAP сетевого интерфейса
 */
awh::net::socket_t awh::eth::Interface::tunnel(string & name) const noexcept {
	// Результат работы функции
	awh::net::socket_t result = net::invalid_socket_t;
	/**
	 * Для операционной системы MacOS X, NetBSD, OpenBSD
	 */
	#if __APPLE__ || __MACH__ || __NetBSD__ || __OpenBSD__
		// Объект контроллера
		struct ctl_info ctlInfo{0};
		// Устанавливаем имя контроллера UTUN
		::strncpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name));
		// Создаём сокет для управления UTUN интерфейсом
		result = ::socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
		// Если сокет не создан
		if(result == net::invalid_socket_t){
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
		// Получаем информацию о контроллере UTUN
		if(::ioctl(result, CTLIOCGINFO, &ctlInfo) == net::invalid_socket_t){
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
			// Закрываем сокет
			::close(result);
			// Выводим результат
			return result;
		}
		// Создаём структуру адреса управления сокетом
		struct sockaddr_ctl sc{0};
		// Устанавливаем идентификатор контроллера
		sc.sc_id = ctlInfo.ctl_id;
		// Устанавливаем длину структуры
		sc.sc_len = sizeof(sc);
		// Устанавливаем семейство адресов
		sc.sc_family = AF_SYSTEM;
		// Устанавливаем тип адреса управления сокетом
		sc.ss_sysaddr = AF_SYS_CONTROL;
		/**
		 * Попытайтесь найти свободный номер устройства вручную или позвольте ядру решить это.
		 * Если мы хотим реализовать логику «tun -> tun0, tun1»:
		 * При использовании UTUN привязка с sc_unit = 0 позволяет ядру выбрать следующий доступный «utunX».
		 * sc_unit = X + 1 запрос utunX.
		 */
		// Автоматический выбор номера интерфейса
		sc.sc_unit = 0;
		// Выполняем подключение к UTUN интерфейсу
		if(::connect(result, reinterpret_cast <struct sockaddr *> (&sc), sizeof(sc)) == net::invalid_socket_t){
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
			// Закрываем сокет
			::close(result);
			// Выводим результат
			return result;
		}
		// Выделяем буфер для имени интерфейса
		name.resize(IFNAMSIZ, '\0');
		// Размер буфера имени интерфейса
		socklen_t length = IFNAMSIZ;
		// Получаем имя созданного интерфейса
		if(::getsockopt(result, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, &name[0], &length) == net::invalid_socket_t){
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
			// Закрываем сокет
			::close(result);
			// Выводим результат
			return result;
		}
		// Обрезаем имя интерфейса по нулевому символу
		name.resize(length);
	/**
	 * Для операционной системы FreeBSD
	 */
	#elif __FreeBSD__
		// Создаём сокет для управления UTUN интерфейсом
		result = ::open("/dev/tun", O_RDWR);
		// Если сокет не создан
		if(result == net::invalid_socket_t){
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
		// Выделяем буфер для имени интерфейса
		name.resize(100, '\0');
		// Получаем имя созданного интерфейса
		if(::fdevname_r(result, &name[0], 100) == nullptr){
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
			// Закрываем сокет
			::close(result);
			// Выводим результат
			return net::invalid_socket_t;
		}
		/**
		 * Режим по умолчанию обычно включает заголовок семейства адресов (4 байта).
		 * При необходимости мы можем попытаться отключить его с помощью TUNSIFHEAD ioctl(fd, TUNSIFHEAD, &zero),
		 * но реализация MacOS X обрабатывает заголовок, поэтому при желании мы можем сохранить его согласованность.
		 */
		// Флаг активации заголовка
		int32_t flag = 1;
		// Включаем информацию о пакете TUN (заголовок)
		if(::ioctl(result, TUNSIFHEAD, &flag) != 0){
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
			// Закрываем сокет
			::close(result);
			// Выводим результат
			return net::invalid_socket_t;
		}
		// Обрезаем имя интерфейса по нулевому символу
		name.resize(::strlen(name.c_str()));
	#endif
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки доступности сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки доступности сетевого интерфейса
 */
bool awh::eth::Interface::isAvailable(const string & name) const noexcept {

}
/**
 * @brief Метод проверки туннельного сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки туннельного сетевого интерфейса
 */
bool awh::eth::Interface::isTunnel(const string & name) const noexcept {

}
/**
 * @brief Метод проверки туннельного сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     результат проверки туннельного сетевого интерфейса
 */
bool awh::eth::Interface::isTunnel(const unique_ptr <net::addr_t> & addr) const noexcept {

}
/**
 * @brief Метод проверки виртуального сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки виртуального сетевого интерфейса
 */
bool awh::eth::Interface::isVirtual(const string & name) const noexcept {

}
/**
 * @brief Метод проверки виртуального сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     результат проверки виртуального сетевого интерфейса
 */
bool awh::eth::Interface::isVirtual(const unique_ptr <net::addr_t> & addr) const noexcept {

}
/**
 * @brief Метод получения имени сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     имя сетевого интерфейса
 */
string awh::eth::Interface::name(const unique_ptr <net::addr_t> & addr) const noexcept {
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
					if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)){
						// Получаем текущее значение аппаратного сетевого адреса
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
 * @brief Метод получения режима сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     режим сетевого интерфейса
 */
awh::event::mode_t awh::eth::Interface::mode(const string & name) const noexcept {

}
/**
 * @brief Метод включения/выключения сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @param mode режим включения/выключения интерфейса
 * @param mtu  размер MTU интерфейса
 * @return     результат включения/выключения интерфейса
 */
bool awh::eth::Interface::mode(const string & name, const event::mode_t mode, const int32_t mtu) const noexcept {
	// Результат работы функции
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
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (mode), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			// Настраиваем интерфейс
			struct ifreq itr{0};
			// Копируем имя интерфейса
			::strncpy(itr.ifr_name, name.c_str(), IFNAMSIZ - 1);
			// Устанавливаем завершающий ноль
			itr.ifr_name[IFNAMSIZ - 1] = '\0';
			// Если не удалось получить флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCGIFFLAGS, &itr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (mode), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			 * Определяем режим работы интерфейса
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо включить интерфейс
				case static_cast <uint8_t> (event::mode_t::ENABLED):
					// Добавляем флаги UP и RUNNING
					itr.ifr_flags |= (IFF_UP | IFF_RUNNING);
				break;
				// Если необходимо выключить интерфейс
				case static_cast <uint8_t> (event::mode_t::DISABLED):
					// Сбросить флаги UP и RUNNING
					itr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
				break;
			}
			// Применяем новые флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCSIFFLAGS, &itr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (mode), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			// Если необходимо включить интерфейс
			if(mode == event::mode_t::ENABLED){
				// Устанавливаем MTU интерфейса
				itr.ifr_mtu = mtu;
				// Применяем новый MTU интерфейса
				if(!(result = (::ioctl(sock, SIOCSIFMTU, &itr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (mode), mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (mode), mtu), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения IP-адреса сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     IP-адрес сетевого интерфейса
 */
unique_ptr <awh::net::addr_t> awh::eth::Interface::ip(const string & name) const noexcept {

}
/**
 * @brief Метод получения IP-адреса сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @param type тип IP-адреса (локальный, глобальный, маска)
 * @return     IP-адрес сетевого интерфейса
 */
string awh::eth::Interface::ip(const string & name, const net::ip_type_t type) const noexcept {

}
/**
 * @brief Метод установки IP-адреса на сетевой интерфейс
 *
 * @param name   имя сетевого интерфейса
 * @param addr   адрес сетевого интерфейса для установки
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат установки IP-адреса
 */
bool awh::eth::Interface::ip(const string & name, const unique_ptr <net::addr_t> & addr, const uint8_t prefix) const noexcept {

}
/**
 * @brief Метод установки IP-адреса на сетевой интерфейс
 *
 * @param name   имя сетевого интерфейса
 * @param addr   адрес сетевого интерфейса для установки
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат установки IP-адреса
 */
bool awh::eth::Interface::ip(const string & name, const unique_ptr <net::addr_t> & addr, const unique_ptr <net::addr_t> & peer, const uint8_t prefix) const noexcept {

}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::eth::Interface::Interface(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::eth::Interface::~Interface() noexcept {}
