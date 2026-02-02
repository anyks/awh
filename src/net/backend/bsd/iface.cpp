/**
 * @file: iface.cpp
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
#include <string>
#include <cstring>
#include <cstdlib>

/**
 * Подключаем системные заголовки
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
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet6/in6_var.h>

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
 * Подключаем заголовочные файлы проекта
 */
#include <net/eth/iface.hpp>

/**
 * Для операционной системы MacOS X
 */
#if __APPLE__ || __MACH__
	/**
	 * Подключаем заголовочные файлы для работы с UTUN интерфейсами
	 */
	#include <net/if_utun.h>
	#include <sys/kern_control.h>
/**
 * Для операционной системы FreeBSD, NetBSD, OpenBSD
 */
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Подключаем заголовочные файлы для работы с TUN интерфейсами
	 */
	#include <net/if_tun.h>
#endif

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем статичные функции в пространство имён
 */
namespace iface {
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
	/**
	 * @brief Функция преобразования маски подсети в префикс
	 *
	 * @param mask маска подсети
	 * @return     префикс сети
	 */
	static uint8_t mask2prefix(const struct in_addr & mask) noexcept {
		// Результат работы функции
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
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция создания клонируемого интерфейса
	 *
	 * @param driver имя драйвера интерфейса
	 * @param name   имя сетевого интерфейса
	 * @param log    объект работы с логами
	 * @return       дескриптор созданного сетевого интерфейса
	 */
	static awh::net::socket_t clonable(const string & driver, string & name, const awh::log_t * log) noexcept {
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
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(driver, name), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выводим результат
					return awh::net::invalid_socket_t;
				}
				// Структура запроса
				struct ifreq ifr{0};
				// Если имя интерфейса задано
				if(!name.empty()){
					// Копируем имя интерфейса
					::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
					// Устанавливаем завершающий ноль
					ifr.ifr_name[IFNAMSIZ - 1] = '\0';
					// Удаляем интерфейс
					if(::ioctl(sock, SIOCIFCREATE, &ifr) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(driver, name), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Выводим результат
						return awh::net::invalid_socket_t;
					}
					// Выводим сокет созданного интерфейса
					return sock;
				// Если имя интерфейса не задано
				} else {
					/**
					 * Перебираем возможные индексы
					 */
					for(size_t i = 0; i < 128; ++i){
						// Формируем имя интерфейса
						::snprintf(ifr.ifr_name, IFNAMSIZ, "%s%zu", driver.c_str(), i);
						// Пытаемся создать интерфейс
						if(::ioctl(sock, SIOCIFCREATE, &ifr) == 0){
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
					// Выводим сообщение об ошибке
					log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(driver, name), awh::log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		}
		// Выводим результат по умолчанию
		return awh::net::invalid_socket_t;
	};
};

/**
 * @brief Метод удаления сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат удаления сетевого интерфейса
 */
bool awh::eth::Interface::destroy(const string & name) const noexcept {
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
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
			// Устанавливаем завершающий ноль
			ifr.ifr_name[IFNAMSIZ - 1] = '\0';
			// Удаляем интерфейс
			if(!(result = (::ioctl(sock, SIOCIFDESTROY, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки доступности сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки доступности сетевого интерфейса
 */
bool awh::eth::Interface::isAvailable(const string & name) const noexcept {
	// Результат работы функции
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
					// Выводим сообщение об ошибке
					this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::WARNING);
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
				if((result = this->_fmk->compare(ifa->ifa_name, name)))
					// Завершаем поиск
					break;
			// Освобождаем память списка сетевых интерфейсов
			::freeifaddrs(ptr);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки туннельного сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки туннельного сетевого интерфейса
 */
bool awh::eth::Interface::isTunnel(const string & name) const noexcept {
	// Результат работы функции
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
				// Перебираем все сетевые интерфейсы
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Если имя интерфейса совпадает
					if(this->_fmk->compare(ifa->ifa_name, name)){
						/**
						 * Проверяем флаги на любой записи интерфейса (IPv4/IPv6/Link)
						 * Туннель обычно Point-to-Point и не Broadcast
						 */
						result = ((ifa->ifa_flags & IFF_POINTOPOINT) && !(ifa->ifa_flags & IFF_BROADCAST));
						// Дополнительная точная проверка через AF_LINK
						if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)){
							// Получаем структуру адреса канального уровня
							struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
							/**
							 * Определяем тип интерфейса
							 */
							switch(sdl->sdl_type){
								/**
								 * Если определён тип интерфейса Tunnel
								 */
								#ifdef IFT_TUNNEL
									// Если это виртуальный интерфейс (Tunnel)
									case IFT_TUNNEL:
								#endif
								/**
								 * Если определён тип интерфейса STF
								 */
								#ifdef IFT_STF
									// Если это виртуальный интерфейс (STF)
									case IFT_STF:
								#endif
								// Если это виртуальный интерфейс (PPP)
								case IFT_PPP:
								// Если это виртуальный интерфейс (GIF)
								case IFT_GIF: {
									// Устанавливаем результат окончательно
									result = true;
									// Прерываем цикл, так как точно нашли
									goto End;
								}
							}
						}
					}
				}
				/**
				 * Завершаем поиск туннельного интерфейса
				 */
				End:
				// Освобождаем память списка сетевых интерфейсов
				::freeifaddrs(ptr);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки туннельного сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     результат проверки туннельного сетевого интерфейса
 */
bool awh::eth::Interface::isTunnel(const unique_ptr <net::addr_t> & addr) const noexcept {
	// Возвращаем результат проверки имени интерфейса
	return this->isTunnel(this->name(addr));
}
/**
 * @brief Метод проверки виртуального сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     результат проверки виртуального сетевого интерфейса
 */
bool awh::eth::Interface::isVirtual(const string & name) const noexcept {
	// Результат работы функции
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
				// Перебираем все сетевые интерфейсы
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Если имя интерфейса совпадает
					if(this->_fmk->compare(ifa->ifa_name, name)){
						// Проверяем флаги: если интерфейс имеет флаги POINTOPOINT или LOOPBACK
						result = ((ifa->ifa_flags & IFF_POINTOPOINT) || (ifa->ifa_flags & IFF_LOOPBACK));
						// Дополнительная точная проверка через AF_LINK
						if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)){
							// Получаем структуру адреса канального уровня
							struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
							/**
							 * Определяем тип интерфейса
							 */
							switch(sdl->sdl_type){
								/**
								 * Если определён тип интерфейса Bridge
								 */
								#ifdef IFT_BRIDGE
									// Если это виртуальный интерфейс (Bridge)
									case IFT_BRIDGE:
								#endif
								/**
								 * Если определён тип интерфейса VLAN
								 */
								#ifdef IFT_L2VLAN
									// Если это виртуальный интерфейс (VLAN)
									case IFT_L2VLAN:
								#endif
								/**
								 * Если определён тип интерфейса Tunnel
								 */
								#ifdef IFT_TUNNEL
									// Если это виртуальный интерфейс (Tunnel)
									case IFT_TUNNEL:
								#endif
								/**
								 * Если определён тип интерфейса STF
								 */
								#ifdef IFT_STF
									// Если это виртуальный интерфейс (STF)
									case IFT_STF:
								#endif
								// Если это виртуальный интерфейс (Loopback)
								case IFT_LOOP:
								// Если это виртуальный интерфейс (PPP)
								case IFT_PPP:
								// Если это виртуальный интерфейс (GIF)
								case IFT_GIF: {
									// Устанавливаем результат
									result = true;
									// Прерываем цикл
									goto End;
								}
							}
						}
					}
				}
				/**
				 * Завершаем поиск виртуального интерфейса
				 */
				End:
				// Освобождаем память списка сетевых интерфейсов
				::freeifaddrs(ptr);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки виртуального сетевого интерфейса по адресу
 *
 * @param addr адрес сетевого подключения
 * @return     результат проверки виртуального сетевого интерфейса
 */
bool awh::eth::Interface::isVirtual(const unique_ptr <net::addr_t> & addr) const noexcept {
	// Возвращаем результат проверки имени интерфейса
	return this->isVirtual(this->name(addr));
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
 * @brief Метод создания сетевого интерфейса
 *
 * @param type тип сетевого интерфейса
 * @param name имя сетевого интерфейса
 * @return     дескриптор созданного сетевого интерфейса
 */
awh::net::socket_t awh::eth::Interface::create(const event::eth_t type, string & name) const noexcept {
	// Результат работы функции
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
						// Перебираем интерфейсы
						for(struct ifaddrs * ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем интерфейсы без адреса AF_LINK
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_LINK))
								// Переходим к следующему интерфейсу
								continue;
							// Получаем структуру адреса канального уровня
							struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
							// Проверяем тип интерфейса (должен быть Ethernet)
							if(sdl->sdl_type == IFT_ETHER){
								// Проверяем флаги: не петля, и (активный или можно активировать)
								if(!(ifa->ifa_flags & IFF_LOOPBACK)){
									// Сохраняем имя
									name = ifa->ifa_name;
									// Прерываем поиск
									break;
								}
							}
						}
						// Освобождаем список
						::freeifaddrs(ifap);
					}
				}
				// Если имя интерфейса определено
				if(!name.empty()){
					// Создаём сокет для управления для поднятия интерфейса
					result = ::socket(AF_INET, SOCK_DGRAM, 0);
					// Если сокет управления создан
					if(result != net::invalid_socket_t){
						// Структура запроса
						struct ifreq ifr{0};
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
					// Теперь открываем BPF устройство для работы с пакетами
					char buffer[32];
					// Перебираем устройства BPF
					for(uint16_t i = 0; i < 256; ++i){
						// Формируем путь
						::snprintf(buffer, sizeof(buffer), "/dev/bpf%u", i);
						// Пытаемся открыть устройство
						if((result = ::open(buffer, O_RDWR)) != net::invalid_socket_t){
							// Структура для привязки
							struct ifreq ifr{0};
							// Копируем имя интерфейса
							::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
							// Устанавливаем завершающий ноль
							ifr.ifr_name[IFNAMSIZ - 1] = '\0';
							// Пытаемся привязать BPF к интерфейсу
							if(::ioctl(result, BIOCSETIF, &ifr) == 0){
								// Включаем немедленный режим (возврат при получении пакета сразу)
								unsigned int on = 1;
								// Активируем немедленный режим
								if(::ioctl(result, BIOCIMMEDIATE, &on) == 0)
									// Успешно открыли и привязали
									goto Success;
							}
							// Если привязка не удалась, закрываем и пробуем следующий BPF
							::close(result);
							// Сбрасываем результат
							result = net::invalid_socket_t;
						}
					}
					// Метка успешного завершения
					Success:;
				}
			} break;
			// Если создаётся передача сырых IP-пакетов между точками
			case static_cast <uint8_t> (event::eth_t::TUN): {
				/**
				 * Для операционной системы MacOS X
				 */
				#if __APPLE__ || __MACH__
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD
				 */
				#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Создаём сокет для управления UTUN интерфейсом
					result = ::open("/dev/tun", O_RDWR);
					// Если сокет не создан
					if(result == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
					name.resize(IFNAMSIZ, '\0');
					// Получаем имя созданного интерфейса
					if(::fdevname_r(result, &name[0], IFNAMSIZ) == nullptr){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
				#endif
			} break;
			// Если создаётся передача кадров Ethernet (с MAC-адресами)
			case static_cast <uint8_t> (event::eth_t::TAP): {
				/**
				 * Для операционной системы MacOS X
				 */
				#if __APPLE__ || __MACH__
					/**
					 * В штатной поставке MacOS X нет /dev/tap.
					 * Для поддержки tap требуются сторонние расширения (например, tuntaposx).
					 * Пытаемся найти доступные устройства, если драйвер установлен.
					 */
					for(uint8_t i = 0; i < 16; ++i){
						// Пытаемся открыть устройство
						if((result = ::open(this->_fmk->format("/dev/tap%u", i).c_str(), O_RDWR)) != net::invalid_socket_t){
							// Формируем имя интерфейса
							name = this->_fmk->format("tap%u", i);
							// Прерываем поиск
							break;
						}
					}
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD
				 */
				#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Открываем клонирующее устройство
					if((result = ::open("/dev/tap", O_RDWR)) != net::invalid_socket_t){
						// Выделяем буфер для имени интерфейса
						name.resize(IFNAMSIZ, '\0');
						// Получаем имя созданного интерфейса
						if(::fdevname_r(result, &name[0], IFNAMSIZ) == nullptr){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
					}
				#endif
			} break;
			// Если создаётся общий туннельный интерфейс (IPv6-in-IPv4, IPv4-in-IPv6, IPv6-in-IPv6)
			case static_cast <uint8_t> (event::eth_t::GIF):
				// Создаём клонирующий интерфейс GIF
				result = ::iface::clonable("gif", name, this->_log);
			break;
			// Если создаётся GRE-туннель (включая с ключом)
			case static_cast <uint8_t> (event::eth_t::GRE):
				// Создаём клонирующий интерфейс GRE
				result = ::iface::clonable("gre", name, this->_log);
			break;
			// Если создаётся беспроводной интерфейс
			case static_cast <uint8_t> (event::eth_t::WLAN):
				// Создаём клонирующий интерфейс WLAN
				result = ::iface::clonable("wlan", name, this->_log);
			break;
			// Если создаётся интерфейс логической сегментации на основе 802.1Q
			case static_cast <uint8_t> (event::eth_t::VLAN):
				// Создаём клонирующий интерфейс VLAN
				result = ::iface::clonable("vlan", name, this->_log);
			break;
			// Если создаётся интерфейс агрегации каналов
			case static_cast <uint8_t> (event::eth_t::BOND):
				// Создаём клонирующий интерфейс LAGG
				result = ::iface::clonable("lagg", name, this->_log);
			break;
			// Если создаётся интерфейс объединения интерфейсов на уровне L2
			case static_cast <uint8_t> (event::eth_t::BRIDGE):
				// Создаём клонирующий интерфейс Bridge
				result = ::iface::clonable("bridge", name, this->_log);
			break;
			// Если создаётся неизвестный тип интерфейса
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unsupported network interface type", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     MTU сетевого интерфейса
 */
uint16_t awh::eth::Interface::mtu(const string & name) const noexcept {
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
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Выводим результат по умолчанию
				return false;
			}
			// Настраиваем интерфейс
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
			// Устанавливаем завершающий ноль
			ifr.ifr_name[IFNAMSIZ - 1] = '\0';
			// Извлекаем MTU из интерфейса
			if(::ioctl(sock, SIOCGIFMTU, &ifr) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
				#endif
				// Закрываем сокет
				::close(sock);
				// Выводим результат по умолчанию
				return false;
			}
			// Закрываем сокет
			::close(sock);
			// Выводим результат
			return static_cast <uint16_t> (ifr.ifr_mtu);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @param mtu  размер MTU интерфейса
 * @return     результат установки MTU сетевого интерфейса
 */
bool awh::eth::Interface::mtu(const string & name, const uint16_t mtu) const noexcept {
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
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
			// Устанавливаем завершающий ноль
			ifr.ifr_name[IFNAMSIZ - 1] = '\0';
			// Если не удалось получить флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCGIFFLAGS, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			// Устанавливаем MTU интерфейса
			ifr.ifr_mtu = static_cast <int32_t> (mtu);
			// Применяем новый MTU интерфейса
			if(!(result = (::ioctl(sock, SIOCSIFMTU, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, mtu), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, mtu), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения установленных флагов сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @return     флаги сетевого интерфейса
 */
unordered_set <awh::event::eth_flag_t> awh::eth::Interface::flags(const string & name) const noexcept {
	// Результат работы функции
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
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
			// Устанавливаем завершающий ноль
			ifr.ifr_name[IFNAMSIZ - 1] = '\0';
			// Если не удалось получить флаги интерфейса
			if(::ioctl(sock, SIOCGIFFLAGS, &ifr) != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(errno));
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки флага сетевого интерфейса
 *
 * @param name имя сетевого интерфейса
 * @param flag флаг сетевого интерфейса
 * @param mode режим включения/выключения флага
 * @return     результат установки флага сетевого интерфейса
 */
bool awh::eth::Interface::flag(const string & name, const event::eth_flag_t flag, const event::mode_t mode) const noexcept {
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
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			struct ifreq ifr{0};
			// Копируем имя интерфейса
			::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
			// Устанавливаем завершающий ноль
			ifr.ifr_name[IFNAMSIZ - 1] = '\0';
			// Если не удалось получить флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCGIFFLAGS, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
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
						// Выводим сообщение об ошибке
						this->_log->debug("Passed network interface flag cannot be modified", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Passed network interface flag cannot be modified", log_t::flag_t::WARNING);
					#endif
					// Закрываем сокет
					::close(sock);
					// Выводим результат
					return result;
				}
			}
			// Применяем новые флаги интерфейса
			if(!(result = (::ioctl(sock, SIOCSIFFLAGS, &ifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (flag), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
 * @param name   имя сетевого интерфейса
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       IP-адрес сетевого интерфейса
 */
unique_ptr <awh::net::addr_t> awh::eth::Interface::getAddress(const string & name, const event::family_t family) const noexcept {
	// Результат работы функции
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
					// Выводим сообщение об ошибке
					this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
				#endif
				// Выходим из функции
				return result;
			}
			// Перебираем все сетевые интерфейсы
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
							// Выводим результат
							return result;
						}
						// Если интерфейс является IPv6
						case AF_INET6: {
							// Создаём объект для хранения IPv6-адреса
							result = make_unique <net::addr_net_ipv6_t> ();
							// Копируем IP-адрес в результат
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (result.get())->address[0], &reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr, sizeof(in6_addr));
						} break;
						// В остальных случаях пропускаем интерфейс
						default: continue;
					}
				}
			}
			// Освобождаем память от списка сетевых интерфейсов
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки IP-адреса на сетевой интерфейс
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для установки
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат установки IP-адреса
 */
bool awh::eth::Interface::setAddress(const string & name, const unique_ptr <net::addr_t> & ip, const uint8_t prefix) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если название сетевого интерфейса и адрес для установки переданы
	if(!name.empty() && (ip != nullptr)){
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
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4: {
					// Объект запроса псевдонима интерфейса (для атомарной установки адреса и маски)
					struct in_aliasreq ifra = {0};
					// Копируем имя сетевого интерфейса
					::strncpy(ifra.ifra_name, name.c_str(), IFNAMSIZ - 1);
					// Устанавливаем семейство адресов IPv4
					ifra.ifra_addr.sin_family = AF_INET;
					// Устанавливаем длину структуры
					ifra.ifra_addr.sin_len = sizeof(struct sockaddr_in);
					// Устанавливаем IP-адрес интерфейса
					ifra.ifra_addr.sin_addr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (ip.get())->address;
					// Устанавливаем семейство маски
					ifra.ifra_mask.sin_family = AF_INET;
					// Устанавливаем длину структуры маски
					ifra.ifra_mask.sin_len = sizeof(struct sockaddr_in);
					// Если префикс подсети больше 32 или равен 0
					if((prefix > 32) || (prefix == 0))
						// Устанавливаем маску подсети интерфейса как /32
						ifra.ifra_mask.sin_addr.s_addr = 0xFFFFFFFF;
					// Устанавливаем маску подсети интерфейса
					else ifra.ifra_mask.sin_addr.s_addr = ::iface::prefix2mask(prefix);
					// Устанавливаем семейство широковещательного адреса
					ifra.ifra_broadaddr.sin_family = AF_INET;
					// Устанавливаем длину структуры широковещательного адреса
					ifra.ifra_broadaddr.sin_len = sizeof(struct sockaddr_in);
					// Вычисляем широковещательный адрес
					ifra.ifra_broadaddr.sin_addr.s_addr = ((ifra.ifra_addr.sin_addr.s_addr & ifra.ifra_mask.sin_addr.s_addr) | ~ifra.ifra_mask.sin_addr.s_addr);
					// Применяем новый IP-адрес и маску интерфейса
					if(!(result = (::ioctl(sock, SIOCAIFADDR, &ifra) == 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				} break;
				// Если адрес является IPv6
				case 16: {
					// Объект запроса псевдонима интерфейса (для атомарной установки адреса и маски)
					struct in6_aliasreq ifra6 = {0};
					// Копируем имя сетевого интерфейса
					::strncpy(ifra6.ifra_name, name.c_str(), IFNAMSIZ - 1);
					// Устанавливаем семейство адресов IPv6
					ifra6.ifra_addr.sin6_family = AF_INET6;
					// Устанавливаем длину структуры
					ifra6.ifra_addr.sin6_len = sizeof(struct sockaddr_in6);
					// Устанавливаем IP-адрес интерфейса
					::memcpy(&ifra6.ifra_addr.sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (ip.get())->address[0], 16);
					// Устанавливаем семейство маски
					ifra6.ifra_prefixmask.sin6_family = AF_INET6;
					// Устанавливаем длину структуры маски
					ifra6.ifra_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
					// Если префикс задан
					if((prefix > 0) && (prefix <= 128)){
						// Текущее значение маски подсети
						uint32_t mask = static_cast <uint32_t> (prefix);
						// Проходим по байтам
						for(uint8_t i = 0; i < 16; ++i){
							// Если префикс больше либо равен 8
							if(mask >= 8){
								// Устанавливаем байт маски подсети
								ifra6.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
								// Уменьшаем префикс на 8
								mask -= 8;
							// Если префикс меньше 8, но больше нуля
							} else if(mask > 0) {
								// Устанавливаем байт маски подсети
								ifra6.ifra_prefixmask.sin6_addr.s6_addr[i] = static_cast <uint8_t> (0xff << (8 - mask));
								// Обнуляем префикс
								mask = 0;
							// Зануляем байт маски подсети
							} else ifra6.ifra_prefixmask.sin6_addr.s6_addr[i] = 0;
						}
					// Устанавливаем маску соответствующую префиксу /128
					} else ::memset(&ifra6.ifra_prefixmask.sin6_addr, 0xff, 16);
					/**
					 * Устанавливаем бесконечное время жизни адреса
					 */
					ifra6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
					ifra6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
					// Применяем новый IP-адрес и маску интерфейса
					if(!(result = (::ioctl(sock, SIOCAIFADDR_IN6, &ifra6) == 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод изменения параметров сетевого интерфейса точка-точка
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для получения
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат изменения параметров сетевого интерфейса точка-точка
 */
bool awh::eth::Interface::getBinding(const string & name, unique_ptr <net::addr_t> & ip, unique_ptr <net::addr_t> & peer, uint8_t & prefix) const noexcept {
	// Результат работы функции
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
				// Перебираем все сетевые интерфейсы
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
									prefix = ::iface::mask2prefix(msk->sin_addr);
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
										// Вычисляем префикс
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
				End:
				// Освобождаем память списка сетевых интерфейсов
				::freeifaddrs(ptr);
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки параметров сетевого интерфейса точка-точка
 *
 * @param name   имя сетевого интерфейса
 * @param ip     адрес сетевого интерфейса для установки
 * @param peer   адрес удалённого пира (для точка-точка)
 * @param prefix префикс подсети
 * @return       результат установки параметров сетевого интерфейса точка-точка
 */
bool awh::eth::Interface::setBinding(const string & name, const unique_ptr <net::addr_t> & ip, const unique_ptr <net::addr_t> & peer, const uint8_t prefix) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если название сетевого интерфейса и адреса для установки переданы
	if(!name.empty() && (ip != nullptr) && (peer != nullptr) && (ip->size == peer->size)){
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
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
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
			/**
			 * Определяем тип адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4: {
					// Объект запроса псевдонима интерфейса
					struct in_aliasreq ifra = {0};
					// Копируем имя сетевого интерфейса
					::strncpy(ifra.ifra_name, name.c_str(), IFNAMSIZ - 1);
					// Устанавливаем семейство адресов IPv4
					ifra.ifra_addr.sin_family = AF_INET;
					// Устанавливаем длину структуры
					ifra.ifra_addr.sin_len = sizeof(struct sockaddr_in);
					// Устанавливаем IP-адрес интерфейса
					ifra.ifra_addr.sin_addr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (ip.get())->address;
					// Устанавливаем семейство маски
					ifra.ifra_mask.sin_family = AF_INET;
					// Устанавливаем длину структуры маски
					ifra.ifra_mask.sin_len = sizeof(struct sockaddr_in);
					// Если префикс подсети больше 32 или равен 0
					if((prefix > 32) || (prefix == 0))
						// Устанавливаем маску подсети интерфейса как /32
						ifra.ifra_mask.sin_addr.s_addr = 0xFFFFFFFF;
					// Устанавливаем маску подсети интерфейса
					else ifra.ifra_mask.sin_addr.s_addr = ::iface::prefix2mask(prefix);
					// Устанавливаем семейство адреса удаленого пира
					ifra.ifra_broadaddr.sin_family = AF_INET;
					// Устанавливаем длину структуры адреса удаленого пира
					ifra.ifra_broadaddr.sin_len = sizeof(struct sockaddr_in);
					// Устанавливаем IP-адрес удалённого пира
					ifra.ifra_broadaddr.sin_addr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (peer.get())->address;
					// Применяем новый IP-адрес, маску и адрес пира интерфейса
					if(!(result = (::ioctl(sock, SIOCAIFADDR, &ifra) == 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				} break;
				// Если адрес является IPv6
				case 16: {
					// Объект запроса псевдонима интерфейса
					struct in6_aliasreq ifra6 = {0};
					// Копируем имя сетевого интерфейса
					::strncpy(ifra6.ifra_name, name.c_str(), IFNAMSIZ - 1);
					// Устанавливаем семейство адресов IPv6
					ifra6.ifra_addr.sin6_family = AF_INET6;
					// Устанавливаем длину структуры
					ifra6.ifra_addr.sin6_len = sizeof(struct sockaddr_in6);
					// Устанавливаем IP-адрес интерфейса
					::memcpy(&ifra6.ifra_addr.sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (ip.get())->address[0], 16);
					// Устанавливаем семейство адреса пира
					ifra6.ifra_dstaddr.sin6_family = AF_INET6;
					// Устанавливаем длину структуры адреса пира
					ifra6.ifra_dstaddr.sin6_len = sizeof(struct sockaddr_in6);
					// Устанавливаем IP-адрес удалённого пира
					::memcpy(&ifra6.ifra_dstaddr.sin6_addr, &awh_cast <const net::addr_net_ipv6_t *> (peer.get())->address[0], 16);
					// Устанавливаем семейство маски
					ifra6.ifra_prefixmask.sin6_family = AF_INET6;
					// Устанавливаем длину структуры маски
					ifra6.ifra_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
					// Если префикс задан
					if((prefix > 0) && (prefix <= 128)){
						// Текущее значение маски подсети
						uint32_t mask = static_cast <uint32_t> (prefix);
						// Проходим по байтам
						for(uint8_t i = 0; i < 16; ++i){
							// Если префикс больше либо равен 8
							if(mask >= 8){
								// Устанавливаем байт маски подсети
								ifra6.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
								// Уменьшаем префикс на 8
								mask -= 8;
							// Если префикс меньше 8, но больше нуля
							} else if(mask > 0) {
								// Устанавливаем байт маски подсети
								ifra6.ifra_prefixmask.sin6_addr.s6_addr[i] = static_cast <uint8_t> (0xff << (8 - mask));
								// Обнуляем префикс
								mask = 0;
							// Зануляем байт маски подсети
							} else ifra6.ifra_prefixmask.sin6_addr.s6_addr[i] = 0;
						}
					// Устанавливаем маску соответствующую префиксу /128
					} else ::memset(&ifra6.ifra_prefixmask.sin6_addr, 0xff, 16);
					/**
					 * Устанавливаем бесконечное время жизни адреса
					 */
					ifra6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
					ifra6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
					// Применяем новый IP-адрес и маску интерфейса
					if(!(result = (::ioctl(sock, SIOCAIFADDR_IN6, &ifra6) == 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, ::strerror(errno));
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, error.what());
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
awh::eth::Interface::Interface(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::eth::Interface::~Interface() noexcept {}
