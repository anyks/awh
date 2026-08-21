/**
 * @file iface.cpp
 * @date 2026-01-28
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
#include <cctype>
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
/**
 * Заголовок запросов управления сокетами
 *
 * @note У Sun Solaris и illumos запросы SIOC* объявлены ИМЕННО ЗДЕСЬ, а не в
 *       sys/ioctl.h, как у BSD. Без него компилятор не находит даже SIOCGIFFLAGS
 */
#include <sys/sockio.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
/**
 * Заголовки управления устройствами передачи данных
 *
 * @note У Sun Solaris и illumos устройства передачи данных заводятся и сносятся
 *       НЕ запросами управления сокетом, как у BSD, а отдельной системной
 *       библиотекой. Приёмов SIOCIFCREATE и SIOCIFDESTROY здесь нет вовсе
 *
 * @note Заголовки libdliptun.h, libdlvnic.h и libdlaggr.h системой НЕ поставляются,
 *       хотя вызовы из них библиотекой вывозятся. Опираться на них означало бы
 *       объявлять чужие структуры своими силами и держать их ABI вручную, поэтому
 *       здесь применяются единственно поставляемые заголовки
 *
 */
/**
 * Заголовки потокового интерфейса устройств передачи данных
 *
 * @note Ими открывается устройство туннеля: отдельных устройств вроде «/dev/tun»
 *       эти системы не имеют, зато устройство передачи данных открывается напрямую
 *
 */
#include <stropts.h>
#include <sys/dlpi.h>

#include <libdladm.h>
#include <libdllink.h>
#include <libdlvlan.h>
#include <libdlbridge.h>

/**
 * @brief Опознаватель модуля сетевых интерфейсов
 *
 * @note Заведён по образцу модуля MS Windows: сообщения о самом модуле метятся
 *       им, и по журналу видно, какая система отказала
 *
 */
static constexpr const char * __AWH_IFACE_BACKEND__ = "Sun Solaris interface backend";

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

/**
 * Для операционной системы macOS
 */
#if __APPLE__ || __MACH__
	/**
	 * Подключаем заголовочные файлы для работы с uTUN интерфейсами
	 */
	#include <net/if_utun.h>
	#include <sys/kern_control.h>
/**
 * Для операционной системы FreeBSD, NetBSD, OpenBSD
 */
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
	/**
	 * Подключаем заголовочный файл для работы с TUN интерфейсами
	 */
	#include <net/if_tun.h>
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статичные функции в пространство имён
 *
 */
namespace iface {
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
	static void copyName(char (& buffer)[LIFNAMSIZ], const string_view name) noexcept {
		// Определяем количество копируемых байт с учётом завершающего нуля
		const size_t length = (name.size() < static_cast <size_t> (LIFNAMSIZ - 1)) ? name.size() : static_cast <size_t> (LIFNAMSIZ - 1);
		// Копируем имя интерфейса ровно на длину переданного представления
		::memcpy(buffer, name.data(), length);
		// Устанавливаем завершающий ноль
		buffer[length] = '\0';
	}
	/**
	 * @brief Функция копирования названия сетевого интерфейса
	 *
	 * @note Разновидность заведена под обычный запрос: имя ЛОГИЧЕСКОГО интерфейса
	 *       вдвое длиннее обычного (LIFNAMSIZ против IFNAMSIZ), и одним посредником
	 *       оба не обслужить - размер массива входит в его вид
	 *
	 * @param buffer буфер, в который копируется название
	 * @param name   название сетевого интерфейса
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
	 * @brief Функция получения MTU средствами канального уровня
	 *
	 * @details Запасной путь для связей, у которых уровня IP нет вовсе: `etherstub`
	 *          и прочие связи без поднятого интерфейса IP перечисляются `getifaddrs`
	 *          записью канального уровня, но `SIOCGIFMTU` по ним отвечает `ENXIO` -
	 *          сокет IP такой связи не знает. Размер же у связи есть, и канальный
	 *          уровень его отдаёт
	 *
	 * @param name имя связи канального уровня
	 * @return     размер MTU либо ноль, если связь канальному уровню неизвестна
	 *
	 */
	static uint32_t datalinkMtu(const string_view name) noexcept {
		// Объект управления связями канального уровня
		dladm_handle_t handle = nullptr;
		// Выполняем открытие управления связями канального уровня
		if(::dladm_open(&handle) != DLADM_STATUS_OK)
			// Выводим значение по умолчанию
			return 0;
		// Гарантируем закрытие управления связями при любом выходе
		const unique_ptr <dladm_handle_t, void (*)(dladm_handle_t *)> guard(&handle, [](dladm_handle_t * handle) noexcept -> void {
			// Выполняем закрытие управления связями канального уровня
			::dladm_close(* handle);
		});
		// Название связи канального уровня
		const string label(name);
		// Опознаватель связи канального уровня
		datalink_id_t link = 0;
		// Признаки и разновидности связи канального уровня
		uint32_t flags = 0, media = 0;
		// Класс связи канального уровня
		datalink_class_t kind = DATALINK_CLASS_PHYS;
		// Выполняем поиск связи канального уровня по названию
		if(::dladm_name2info(handle, label.c_str(), &link, &flags, &kind, &media) != DLADM_STATUS_OK)
			// Выводим значение по умолчанию
			return 0;
		// Свойства связи канального уровня
		dladm_attr_t attr{};
		// Выполняем извлечение свойств связи канального уровня
		if(::dladm_info(handle, link, &attr) != DLADM_STATUS_OK)
			// Выводим значение по умолчанию
			return 0;
		// Выводим размер MTU связи канального уровня
		return static_cast <uint32_t> (attr.da_max_sdu);
	}
	/**
	 * @brief Функция проверки типа канального уровня на принадлежность туннелю
	 *
	 * @param type тип интерфейса канального уровня (sdl_type)
	 * @return     результат проверки
	 *
	 */
	static bool isTunnelLinkType(const uint8_t type) noexcept {
		/**
		 * Определяем тип интерфейса
		 */
		switch(type){
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
			/**
			 * Разновидности GIF у этих систем нет
			 *
			 * @note Туннель общего вида Sun Solaris и illumos заводят отдельным
			 *       средством, а разновидности канального уровня IFT_GIF у них не
			 *       существует вовсе - оттого проверка на неё и снята
			 */
				// Сообщаем, что это туннельный интерфейс
				return true;
		}
		// Сообщаем, что это не туннельный интерфейс
		return false;
	}
	/**
	 * @brief Функция проверки типа канального уровня на принадлежность виртуальному интерфейсу
	 *
	 * @param type тип интерфейса канального уровня (sdl_type)
	 * @return     результат проверки
	 *
	 */
	static bool isVirtualLinkType(const uint8_t type) noexcept {
		/**
		 * Определяем тип интерфейса
		 */
		switch(type){
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
			/**
			 * Разновидности GIF у этих систем нет
			 *
			 * @note Туннель общего вида Sun Solaris и illumos заводят отдельным
			 *       средством, а разновидности канального уровня IFT_GIF у них не
			 *       существует вовсе - оттого проверка на неё и снята
			 */
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
					// Ищем MAC-адрес интерфейса
					if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)){
						// Получаем текущее значение аппаратного сетевого адреса
						struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
						// Проверяем длину MAC-адреса
						if(sdl->sdl_alen == 6){
							// Получаем указатель на MAC-адрес
							const uint8_t * mac = reinterpret_cast <const uint8_t *> (LLADDR(sdl));
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
			// Дополнительная точная проверка через AF_LINK
			if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_LINK)){
				// Получаем структуру адреса канального уровня
				struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
				// Если тип интерфейса точно определён, возвращаем результат окончательно
				if(virt ? ::iface::isVirtualLinkType(sdl->sdl_type) : ::iface::isTunnelLinkType(sdl->sdl_type))
					// Сообщаем, что интерфейс точно классифицирован
					return true;
			}
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция заведения логического интерфейса под адрес IPv6
	 *
	 * @details Адрес IPv6 у Sun Solaris и illumos на ОСНОВНОЙ логический интерфейс не
	 *          ложится: там живёт адрес канальной связи (`fe80::/10`), заведённый
	 *          системой, и подменить его нельзя - запрос отвечает отказом
	 *          «запрошенный адрес назначить невозможно». Под всякий свой адрес IPv6
	 *          система велит заводить ОТДЕЛЬНЫЙ логический интерфейс, и заводит его
	 *          сама, выдавая имя вида "awhtun0:1"
	 *
	 * @note Установлено щупом на стенде Solaris 11.4 21.08.2026 - весь путь целиком:
	 *       на основной отказ, SIOCLIFADDIF заводит "awhtun0:1", адрес на него
	 *       ложится, подъём проходит, SIOCLIFREMOVEIF снимает. Тем же путём идёт и
	 *       `ipadm`, заводя под статический адрес IPv6 новый объект адреса
	 *
	 * @warning Адреса IPv4 это НЕ касается: они ложатся на основной интерфейс прямо,
	 *          и заводить им логический незачем
	 *
	 * @param sock управляющий сокет
	 * @param name имя сетевого устройства
	 * @param log  объект работы с логами
	 * @return     имя заведённого логического интерфейса либо пустая строка
	 *
	 */
	static string logical(const awh::net::socket_t sock, const string_view name, const awh::log_t * log) noexcept {
		// Результат работы функции
		string result;
		// Объект запроса настройки логического интерфейса
		struct lifreq lifr;
		// Заполняем объект запроса нулями
		::memset(&lifr, 0, sizeof(lifr));
		// Копируем имя сетевого устройства
		::iface::copyName(lifr.lifr_name, name);
		// Заводим логический интерфейс, имя ему система выдаёт сама
		if(::ioctl(sock, SIOCLIFADDIF, &lifr) != 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("%s: logical interface could not be created on \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(sock, name), awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("%s: logical interface could not be created on \"%s\": %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
			#endif
			// Выводим пустой результат
			return result;
		}
		// Запоминаем имя заведённого логического интерфейса
		result.assign(lifr.lifr_name, ::strnlen(lifr.lifr_name, sizeof(lifr.lifr_name)));
		// Выводим результат
		return result;
	};
	/**
	 * @brief Функция поиска логического интерфейса, несущего заданный адрес
	 *
	 * @details Снятие адреса IPv6 ведётся сносом того логического интерфейса, каким он
	 *          был заведён, а имя его движок не хранит: узлу принадлежит связь, а не
	 *          логический интерфейс. Оттого имя отыскивается по самому адресу
	 *
	 * @param sock управляющий сокет
	 * @param name имя сетевого устройства
	 * @param ip   искомый адрес
	 * @return     имя логического интерфейса либо пустая строка
	 *
	 */
	static string lookup(const awh::net::socket_t sock, const string_view name, const awh::net::addr_t * ip) noexcept {
		// Результат работы функции
		string result;
		// Запрос числа логических интерфейсов машины
		struct lifnum count;
		// Заполняем запрос числа нулями
		::memset(&count, 0, sizeof(count));
		// Спрашиваем интерфейсы семейства искомого адреса
		count.lifn_family = AF_INET6;
		// Если число логических интерфейсов машины получить не удалось
		if((::ioctl(sock, SIOCGLIFNUM, &count) != 0) || (count.lifn_count <= 0))
			// Выводим пустой результат
			return result;
		// Состав логических интерфейсов машины
		vector <struct lifreq> items(static_cast <size_t> (count.lifn_count));
		// Запрос состава логических интерфейсов машины
		struct lifconf conf;
		// Заполняем запрос состава нулями
		::memset(&conf, 0, sizeof(conf));
		// Спрашиваем интерфейсы семейства искомого адреса
		conf.lifc_family = AF_INET6;
		// Устанавливаем буфер под состав
		conf.lifc_buf = reinterpret_cast <caddr_t> (items.data());
		// Устанавливаем размер буфера под состав
		conf.lifc_len = static_cast <int32_t> (items.size() * sizeof(struct lifreq));
		// Если состав логических интерфейсов машины получить не удалось
		if(::ioctl(sock, SIOCGLIFCONF, &conf) != 0)
			// Выводим пустой результат
			return result;
		// Получаем число полученных записей состава
		const size_t size = (static_cast <size_t> (conf.lifc_len) / sizeof(struct lifreq));
		/**
		 * Переходим по всему составу логических интерфейсов машины
		 */
		for(size_t i = 0; i < size; i++){
			// Получаем имя очередного логического интерфейса
			const string label(items[i].lifr_name, ::strnlen(items[i].lifr_name, sizeof(items[i].lifr_name)));
			// Если логический интерфейс принадлежит другому устройству
			if((label.size() < name.size()) || (label.compare(0, name.size(), name) != 0))
				// Переходим к следующему логическому интерфейсу
				continue;
			// Объект запроса настройки логического интерфейса
			struct lifreq lifr;
			// Заполняем объект запроса нулями
			::memset(&lifr, 0, sizeof(lifr));
			// Копируем имя логического интерфейса
			::iface::copyName(lifr.lifr_name, label);
			// Если адрес логического интерфейса получить не удалось
			if(::ioctl(sock, SIOCGLIFADDR, &lifr) != 0)
				// Переходим к следующему логическому интерфейсу
				continue;
			// Получаем адрес логического интерфейса нужного вида
			const struct sockaddr_in6 * addr = reinterpret_cast <const struct sockaddr_in6 *> (&lifr.lifr_addr);
			// Если адрес логического интерфейса совпал с искомым
			if(::memcmp(&addr->sin6_addr, &awh_cast <const awh::net::addr_net_ipv6_t *> (ip)->address[0], 16) == 0){
				// Запоминаем имя найденного логического интерфейса
				result = label;
				// Выходим из цикла
				break;
			}
		}
		// Выводим результат
		return result;
	};
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
				 * Адрес, маска и широковещательный адрес ставятся ТРЕМЯ запросами
				 *
				 * @details Единого запроса вида SIOCAIFADDR, каким BSD ставит всё разом,
				 *          у Sun Solaris и illumos нет: настройка интерфейсов там идёт
				 *          через "логический интерфейс" структурой `lifreq`, и всякая
				 *          её часть задаётся своим запросом
				 *
				 * @warning Порядок НЕ атомарен, в отличие от пути BSD: между запросами
				 *          интерфейс успевает побыть с новым адресом и старой маской.
				 *          Обойти это нечем - иного пути система не предлагает, - но
				 *          знать об этом обязан всякий, кто правит здесь дальше
				 *
				 * @note Поля длины (`sin_len`) у этих систем нет вовсе: `sockaddr_in`
				 *       там устроен по SVR4, а не по BSD
				 */
				// Объект запроса настройки логического интерфейса
				struct lifreq lifr;
				// Заполняем объект запроса нулями
				::memset(&lifr, 0, sizeof(lifr));
				// Копируем имя сетевого интерфейса
				::iface::copyName(lifr.lifr_name, name);
				// Получаем адрес интерфейса нужного вида
				struct sockaddr_in * addr = reinterpret_cast <struct sockaddr_in *> (&lifr.lifr_addr);
				// Устанавливаем семейство адресов IPv4
				addr->sin_family = AF_INET;
				// Устанавливаем IP-адрес интерфейса
				addr->sin_addr.s_addr = awh_cast <const awh::net::addr_net_ipv4_t *> (ip)->address;
				// Применяем новый IP-адрес интерфейса
				if(!(result = (::ioctl(sock, SIOCSLIFADDR, &lifr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s: address could not be assigned to interface \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s: address could not be assigned to interface \"%s\": %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				// Значение маски подсети интерфейса
				const uint32_t netmask = (((prefix > 32) || (prefix == 0)) ? 0xFFFFFFFF : ::iface::prefix2mask(prefix));
				// Получаем маску подсети нужного вида
				struct sockaddr_in * mask = reinterpret_cast <struct sockaddr_in *> (&lifr.lifr_addr);
				// Заполняем маску подсети нулями
				::memset(mask, 0, sizeof(struct sockaddr_in));
				// Устанавливаем семейство маски
				mask->sin_family = AF_INET;
				// Устанавливаем маску подсети интерфейса
				mask->sin_addr.s_addr = netmask;
				// Применяем маску подсети интерфейса
				if(!(result = (::ioctl(sock, SIOCSLIFNETMASK, &lifr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s: netmask could not be assigned to interface \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s: netmask could not be assigned to interface \"%s\": %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				// Получаем адрес назначения нужного вида
				struct sockaddr_in * dest = reinterpret_cast <struct sockaddr_in *> (&lifr.lifr_addr);
				// Заполняем адрес назначения нулями
				::memset(dest, 0, sizeof(struct sockaddr_in));
				// Устанавливаем семейство адреса назначения
				dest->sin_family = AF_INET;
				// Если задан адрес удалённого пира (точка-точка)
				if(peer != nullptr){
					// Устанавливаем IP-адрес удалённого пира
					dest->sin_addr.s_addr = awh_cast <const awh::net::addr_net_ipv4_t *> (peer)->address;
					// Применяем адрес удалённого пира
					result = (::ioctl(sock, SIOCSLIFDSTADDR, &lifr) == 0);
				// Если адрес удалённого пира не задан
				} else {
					// Вычисляем широковещательный адрес
					dest->sin_addr.s_addr = ((awh_cast <const awh::net::addr_net_ipv4_t *> (ip)->address & netmask) | ~netmask);
					// Применяем широковещательный адрес
					result = (::ioctl(sock, SIOCSLIFBRDADDR, &lifr) == 0);
				}
				// Если применить адрес назначения не удалось
				if(!result){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s: broadcast address could not be assigned to interface \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s: broadcast address could not be assigned to interface \"%s\": %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					#endif
				}
			} break;
			// Если адрес является IPv6
			case 16: {
				/**
				 * Устройство то же, что и у IPv4: три запроса через `lifreq`
				 *
				 * @note Времени жизни адреса здесь не задаётся: запроса, равного
				 *       `SIOCAIFADDR_IN6` с полем `ifra_lifetime`, у этих систем нет,
				 *       а поставленный таким путём адрес и без того бессрочен
				 */
				// Объект запроса настройки логического интерфейса
				struct lifreq lifr;
				// Заполняем объект запроса нулями
				::memset(&lifr, 0, sizeof(lifr));
				// Копируем имя сетевого интерфейса
				::iface::copyName(lifr.lifr_name, name);
				// Получаем адрес интерфейса нужного вида
				struct sockaddr_in6 * addr = reinterpret_cast <struct sockaddr_in6 *> (&lifr.lifr_addr);
				// Устанавливаем семейство адресов IPv6
				addr->sin6_family = AF_INET6;
				// Устанавливаем IP-адрес интерфейса
				::memcpy(&addr->sin6_addr, &awh_cast <const awh::net::addr_net_ipv6_t *> (ip)->address[0], 16);
				// Применяем новый IP-адрес интерфейса
				if(!(result = (::ioctl(sock, SIOCSLIFADDR, &lifr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s: address could not be assigned to interface \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s: address could not be assigned to interface \"%s\": %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				// Получаем маску подсети нужного вида
				struct sockaddr_in6 * mask = reinterpret_cast <struct sockaddr_in6 *> (&lifr.lifr_addr);
				// Заполняем маску подсети нулями
				::memset(mask, 0, sizeof(struct sockaddr_in6));
				// Устанавливаем семейство маски
				mask->sin6_family = AF_INET6;
				// Если префикс задан
				if((prefix > 0) && (prefix <= 128)){
					// Текущее значение маски подсети
					uint32_t bits = static_cast <uint32_t> (prefix);
					/**
					 * Проходим по байтам
					 */
					for(uint8_t i = 0; i < 16; ++i){
						// Если префикс больше либо равен 8
						if(bits >= 8){
							// Устанавливаем байт маски подсети
							mask->sin6_addr.s6_addr[i] = 0xFF;
							// Уменьшаем префикс на 8
							bits -= 8;
						// Если префикс меньше 8, но больше нуля
						} else if(bits > 0) {
							// Устанавливаем байт маски подсети
							mask->sin6_addr.s6_addr[i] = static_cast <uint8_t> (0xFF << (8 - bits));
							// Обнуляем префикс
							bits = 0;
						// Зануляем байт маски подсети
						} else mask->sin6_addr.s6_addr[i] = 0;
					}
				// Устанавливаем маску соответствующую префиксу /128
				} else ::memset(&mask->sin6_addr, 0xFF, 16);
				// Применяем маску подсети интерфейса
				if(!(result = (::ioctl(sock, SIOCSLIFNETMASK, &lifr) == 0))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s: netmask could not be assigned to interface \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s: netmask could not be assigned to interface \"%s\": %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
					#endif
					// Выходим из функции
					return result;
				}
				// Если задан адрес удалённого пира (точка-точка)
				if(peer != nullptr){
					// Получаем адрес пира нужного вида
					struct sockaddr_in6 * dest = reinterpret_cast <struct sockaddr_in6 *> (&lifr.lifr_addr);
					// Заполняем адрес пира нулями
					::memset(dest, 0, sizeof(struct sockaddr_in6));
					// Устанавливаем семейство адреса пира
					dest->sin6_family = AF_INET6;
					// Устанавливаем IP-адрес удалённого пира
					::memcpy(&dest->sin6_addr, &awh_cast <const awh::net::addr_net_ipv6_t *> (peer)->address[0], 16);
					// Применяем адрес удалённого пира
					if(!(result = (::ioctl(sock, SIOCSLIFDSTADDR, &lifr) == 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							log->debug("%s: peer address could not be assigned to interface \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(sock, name, static_cast <uint16_t> (prefix)), awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log->print("%s: peer address could not be assigned to interface \"%s\": %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
						#endif
					}
				}
			} break;
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция отказа в заведении устройства передачи данных
	 *
	 * @details У BSD устройство заводится пустым запросом SIOCIFCREATE и настраивается
	 *          после, отчего подписи с одним лишь именем хватает. Sun Solaris и illumos
	 *          устроены иначе: приёмов SIOCIFCREATE и SIOCIFDESTROY у них нет вовсе, а
	 *          устройство передачи данных заводится СРАЗУ СО ВСЕЙ СВОЕЙ НАСТРОЙКОЙ -
	 *          логическому сегменту нужны несущее устройство и метка, агрегации - список
	 *          участников, туннелю - оба конца. Пустых устройств, настраиваемых после
	 *          заведения, эти системы не знают
	 *
	 * @warning Отказ здесь - НЕ упрощение и не пропуск: сведений для заведения не несёт
	 *          сама подпись create(type, name), и никакая библиотека этого не исправит.
	 *          Закрывается это расширением договора, а не средствами внутри модуля
	 *
	 * @param kind название разновидности устройства передачи данных
	 * @param log  объект работы с логами
	 * @return     дескриптор созданного сетевого интерфейса
	 *
	 */
	/**
	 * @brief Функция подачи запроса управления устройством передачи кадров
	 *
	 * @param fd      дескриптор открытого устройства
	 * @param request буфер запроса
	 * @param size    размер буфера запроса
	 * @param expect  ожидаемый вид подтверждения
	 * @param log     объект работы с логами
	 * @return        результат подачи запроса
	 *
	 */
	static bool request(const int32_t fd, void * request, const size_t size, const uint32_t expect, const awh::log_t * log) noexcept {
		// Буфер под подтверждение запроса
		char buffer[1024];
		// Описание передаваемого сообщения
		struct strbuf message;
		// Устанавливаем буфер передаваемого сообщения
		message.buf = reinterpret_cast <char *> (request);
		// Устанавливаем размер передаваемого сообщения
		message.len = static_cast <int32_t> (size);
		// Устанавливаем предел размера передаваемого сообщения
		message.maxlen = static_cast <int32_t> (size);
		// Подаём запрос управления устройством
		if(::putmsg(fd, &message, nullptr, 0) < 0){
			// Записываем ошибку в лог
			log->print("%s: data link request could not be submitted: %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, ::strerror(errno));
			// Выходим из функции
			return false;
		}
		// Признаки принимаемого сообщения
		int32_t flags = 0;
		// Устанавливаем буфер принимаемого сообщения
		message.buf = buffer;
		// Сбрасываем размер принимаемого сообщения
		message.len = 0;
		// Устанавливаем предел размера принимаемого сообщения
		message.maxlen = static_cast <int32_t> (sizeof(buffer));
		// Принимаем подтверждение запроса
		if(::getmsg(fd, &message, nullptr, &flags) < 0){
			// Записываем ошибку в лог
			log->print("%s: data link request was not acknowledged: %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, ::strerror(errno));
			// Выходим из функции
			return false;
		}
		// Получаем вид полученного подтверждения
		const uint32_t primitive = reinterpret_cast <union DL_primitives *> (buffer)->dl_primitive;
		// Если полученное подтверждение не соответствует ожидаемому
		if(primitive != expect){
			// Записываем ошибку в лог
			log->print("%s: data link request was refused by the kernel, acknowledgement is 0x%X instead of 0x%X", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, primitive, expect);
			// Выходим из функции
			return false;
		}
		// Выводим результат
		return true;
	};
	/**
	 * @brief Функция открытия устройства переноса данных туннеля
	 *
	 * @details У Sun Solaris и illumos отдельных устройств туннеля вроде «/dev/tun»
	 *          нет вовсе, но устройство передачи данных открывается напрямую как
	 *          «/dev/net/<имя>», и после перевода в сырой режим обмен идёт
	 *          обыкновенными read и write ГОЛЫМ ДЕСКРИПТОРОМ - ровно тем, что
	 *          договор и возвращает. Оттого библиотека libdlpi здесь не нужна:
	 *          она вернула бы свой описатель, который поверх дескриптора пришлось
	 *          бы где-то держать и освобождать
	 *
	 * @warning Устройство должно быть заведено ЗАРАНЕЕ распорядителем машины:
	 *          заведение устройств передачи данных - надзорный шаг, и точки входа
	 *          для него система поставляет без объявлений. Условие это записано
	 *          предварительным в README, как это сделано для libsctp у Linux
	 *
	 * @warning Устройство переносит КАДРЫ канального уровня в обоих случаях: разделения
	 *          на перенос пакетов и перенос кадров, как у BSD, эти системы не знают.
	 *          Приписывание и снятие заголовка кадра, а равно ответы на разрешение
	 *          адресов, ложатся на движок - так же, как это делается поверх драйвера
	 *          tap-windows6 у MS Windows
	 *
	 * @param name имя заведённого устройства передачи данных
	 * @param log  объект работы с логами
	 * @return     дескриптор открытого устройства
	 *
	 */
	static awh::net::socket_t tunnel(const string & name, const awh::log_t * log) noexcept {
		// Если имя устройства передачи данных не передано
		if(name.empty()){
			// Записываем ошибку в лог
			log->print("%s: tunnel device name is required: the system creates data links administratively, and the name of a prepared device must be given", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__);
			// Выводим результат
			return awh::net::invalid_socket_t;
		}
		// Формируем путь к устройству передачи данных
		const string path = ("/dev/net/" + name);
		// Открываем устройство передачи данных
		const awh::net::socket_t result = ::open(path.c_str(), O_RDWR);
		// Если устройство передачи данных не открыто
		if(result == awh::net::invalid_socket_t){
			// Записываем ошибку в лог
			log->print("%s: tunnel device \"%s\" could not be opened: %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, path.c_str(), ::strerror(errno));
			// Выводим результат
			return awh::net::invalid_socket_t;
		}
		// Запрос привязки к устройству передачи данных
		dl_bind_req_t bind;
		// Заполняем запрос привязки нулями
		::memset(&bind, 0, sizeof(bind));
		// Устанавливаем вид запроса
		bind.dl_primitive = DL_BIND_REQ;
		// Устанавливаем опознаватель протокола
		bind.dl_sap = 0;
		// Устанавливаем режим обслуживания без установления соединения
		bind.dl_service_mode = DL_CLDLS;
		// Выполняем привязку к устройству передачи данных
		if(!::iface::request(result, &bind, sizeof(bind), DL_BIND_ACK, log)){
			// Закрываем устройство передачи данных
			::close(result);
			// Выводим результат
			return awh::net::invalid_socket_t;
		}
		/**
		 * Переводим устройство в сырой режим
		 *
		 * @note Без этого обмен идёт сообщениями потокового интерфейса, а не
		 *       готовыми кадрами, и обыкновенные read и write непригодны
		 */
		struct strioctl control;
		// Заполняем запрос управления нулями
		::memset(&control, 0, sizeof(control));
		// Устанавливаем запрос перевода в сырой режим
		control.ic_cmd = DLIOCRAW;
		// Устанавливаем бессрочное ожидание ответа
		control.ic_timout = -1;
		// Переводим устройство передачи данных в сырой режим
		if(::ioctl(result, I_STR, &control) < 0){
			// Записываем ошибку в лог
			log->print("%s: tunnel device \"%s\" could not be switched to raw mode: %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, path.c_str(), ::strerror(errno));
			// Закрываем устройство передачи данных
			::close(result);
			// Выводим результат
			return awh::net::invalid_socket_t;
		}
		/**
		 * Требуем сохранения границ сообщений при чтении
		 *
		 * @details Поток отдаёт обыкновенному чтению столько, сколько накопилось: два
		 *          кадра склеиваются в одно чтение, и границы между ними теряются.
		 *          Режим RMSGD велит отдавать РОВНО ОДНО сообщение за чтение, а остаток
		 *          придержать до следующего - кадр к кадру, как их и клало устройство
		 *
		 * @warning Без этого разбор принимает склейку за один кадр: заголовок снимается
		 *          у первого, а второй уходит в мусор вместе с ним. Проверено разбором
		 *          вызовов - чтение отдавало 196 октетов там, где кадр 98
		 */
		if(::ioctl(result, I_SRDOPT, RMSGD) < 0){
			// Записываем ошибку в лог
			log->print("%s: tunnel device \"%s\" could not be switched to message-discard read mode: %s", awh::log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, path.c_str(), ::strerror(errno));
			// Закрываем устройство передачи данных
			::close(result);
			// Выводим результат
			return awh::net::invalid_socket_t;
		}
		/**
		 * Включаем всеприёмные режимы
		 *
		 * @note Отбор по опознавателю протокола и по аппаратному адресу режет
		 *       кадры туннеля: проверено опытом - без всеприёма по опознавателю
		 *       отправленный кадр не приходит вовсе
		 */
		for(const uint32_t level : {static_cast <uint32_t> (DL_PROMISC_SAP), static_cast <uint32_t> (DL_PROMISC_PHYS)}){
			// Запрос включения всеприёмного режима
			dl_promiscon_req_t promisc;
			// Заполняем запрос включения нулями
			::memset(&promisc, 0, sizeof(promisc));
			// Устанавливаем вид запроса
			promisc.dl_primitive = DL_PROMISCON_REQ;
			// Устанавливаем уровень всеприёмного режима
			promisc.dl_level = level;
			// Включаем всеприёмный режим
			if(!::iface::request(result, &promisc, sizeof(promisc), DL_OK_ACK, log)){
				// Закрываем устройство передачи данных
				::close(result);
				// Выводим результат
				return awh::net::invalid_socket_t;
			}
		}
		// Выводим результат
		return result;
	};
	/**
	 * @brief Функция отказа в заведении устройства передачи данных
	 */
	static awh::net::socket_t unsupported(string_view kind, const awh::log_t * log) noexcept {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			log->debug("%s: network interface of type \"%s\" cannot be created: the system creates a data link together with its entire configuration, which the interface name alone does not carry", __PRETTY_FUNCTION__, make_tuple(kind), awh::log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, string(kind).c_str());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log->print("%s: network interface of type \"%s\" cannot be created: the system creates a data link together with its entire configuration, which the interface name alone does not carry", awh::log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, string(kind).c_str());
		#endif
		// Возвращаем значение по умолчанию
		return awh::net::invalid_socket_t;
	};
};

/**
 * @brief Метод удаления сетевого интерфейса
 *
 * @details Приёма SIOCIFDESTROY у Sun Solaris и illumos нет: устройства передачи
 *          данных сносит отдельная системная библиотека, причём разными вызовами
 *          для разной разновидности устройства. Разновидность узнаётся по имени
 *          разрешением его в описатель устройства
 *
 * @warning Поставляемыми заголовками закрыты логический сегмент и мост. Туннель,
 *          мнимое устройство и агрегация сносятся вызовами, объявления которых
 *          система НЕ поставляет, и по ним выдаётся явный отказ с названием
 *          разновидности - молча такое устройство не пропускается
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
			// Описатель работы с устройствами передачи данных
			dladm_handle_t handle = nullptr;
			// Буфер под описание ошибки библиотеки
			char buffer[DLADM_STRSIZE];
			// Заводим описатель работы с устройствами передачи данных
			dladm_status_t status = ::dladm_open(&handle);
			// Если описатель работы с устройствами передачи данных не заведён
			if(!(result = (status == DLADM_STATUS_OK))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s: data link manager could not be opened: %s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, ::dladm_status2str(status, buffer));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s: data link manager could not be opened: %s", log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, ::dladm_status2str(status, buffer));
				#endif
				// Возвращаем результат
				return result;
			}
			// Гарантируем закрытие описателя при любом выходе
			const unique_ptr <dladm_handle_t, void (*)(dladm_handle_t *)> guard(&handle, [](dladm_handle_t * handle) noexcept -> void {
				// Закрываем описатель работы с устройствами передачи данных
				::dladm_close(* handle);
			});
			// Описатель устройства передачи данных
			datalink_id_t link = DATALINK_INVALID_LINKID;
			// Признаки устройства и его среда передачи
			uint32_t flags = 0, media = 0;
			// Разновидность устройства передачи данных
			datalink_class_t kind = static_cast <datalink_class_t> (0);
			// Получаем имя интерфейса в виде строки
			const string label(name);
			// Разрешаем имя интерфейса в описатель устройства передачи данных
			status = ::dladm_name2info(handle, label.c_str(), &link, &flags, &kind, &media);
			// Если имя интерфейса не разрешено
			if(!(result = (status == DLADM_STATUS_OK))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s: interface \"%s\" was not found: %s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, label.c_str(), ::dladm_status2str(status, buffer));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s: interface \"%s\" was not found: %s", log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, label.c_str(), ::dladm_status2str(status, buffer));
				#endif
				// Возвращаем результат
				return result;
			}
			/**
			 * Определяем разновидность устройства передачи данных
			 */
			switch(static_cast <uint32_t> (kind)){
				// Если сносится логический сегмент на основе 802.1Q
				case static_cast <uint32_t> (DATALINK_CLASS_VLAN):
					// Сносим логический сегмент
					status = ::dladm_vlan_delete(handle, link, DLADM_OPT_ACTIVE | DLADM_OPT_PERSIST);
				break;
				// Если сносится мост уровня L2
				case static_cast <uint32_t> (DATALINK_CLASS_BRIDGE): {
					/**
					 * Отбрасываем порядковый номер имени устройства
					 *
					 * @note Мост носит имя без номера, а устройство, которым он виден
					 *       в системе, - с номером: мосту "foo" отвечает устройство
					 *       "foo0". Снос принимает имя МОСТА, потому номер отбрасывается
					 */
					string bridge(label);
					// Отбрасываем завершающие цифры имени устройства
					while(!bridge.empty() && (::isdigit(static_cast <uint8_t> (bridge.back())) != 0))
						// Отбрасываем последний символ имени
						bridge.pop_back();
					// Сносим мост уровня L2
					status = ::dladm_bridge_delete(handle, bridge.c_str(), DLADM_OPT_ACTIVE | DLADM_OPT_PERSIST);
				} break;
				// Если сносится устройство иной разновидности
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s: interface \"%s\" of data link class 0x%X cannot be removed: the system does not ship the declarations required for it", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, label.c_str(), static_cast <uint32_t> (kind));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("%s: interface \"%s\" of data link class 0x%X cannot be removed: the system does not ship the declarations required for it", log_t::flag_t::WARNING, ::__AWH_IFACE_BACKEND__, label.c_str(), static_cast <uint32_t> (kind));
					#endif
					// Возвращаем результат
					return false;
				}
			}
			// Если снос устройства передачи данных не удался
			if(!(result = (status == DLADM_STATUS_OK))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s: interface \"%s\" could not be removed: %s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, label.c_str(), ::dladm_status2str(status, buffer));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s: interface \"%s\" could not be removed: %s", log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, label.c_str(), ::dladm_status2str(status, buffer));
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
				result = ::iface::classify(ptr, name, false, this->_fmk);
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
				const string name = ::iface::findNameByAddr(ptr, addr);
				// Если имя интерфейса найдено, выполняем классификацию по тому же списку
				if(!name.empty())
					// Выполняем классификацию интерфейса как туннельного
					result = ::iface::classify(ptr, name, false, this->_fmk);
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
				result = ::iface::classify(ptr, name, true, this->_fmk);
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
				const string name = ::iface::findNameByAddr(ptr, addr);
				// Если имя интерфейса найдено, выполняем классификацию по тому же списку
				if(!name.empty())
					// Выполняем классификацию интерфейса как виртуального
					result = ::iface::classify(ptr, name, true, this->_fmk);
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
		result = ::iface::findNameByAddr(ptr, addr);
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
					/**
					 * Перебираем устройства BPF
					 */
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
				 * Для операционной системы macOS
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
					// Получаем информацию о контроллере UTUN
					if(::ioctl(result, CTLIOCGINFO, &ctlInfo) == net::invalid_socket_t){
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
						// Закрываем сокет
						::close(result);
						// Возвращаем результат
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
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Закрываем сокет
						::close(result);
						// Возвращаем результат
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
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), name), log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
						// Закрываем сокет
						::close(result);
						// Возвращаем результат
						return net::invalid_socket_t;
					}
					// Обрезаем имя интерфейса по нулевому символу
					name.resize(::strlen(name.c_str()));
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD
				 */
				#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
					/**
					 * Заводим устройство туннеля
					 *
					 * @details У FreeBSD заведено клонирующее устройство: открытие «/dev/tun»
					 *          выдаёт свободный номер само, а какой именно достался - узнаётся
					 *          после по описателю. У NetBSD и OpenBSD клонирующего устройства
					 *          нет вовсе, и свободный номер отыскивается перебором - открытием
					 *          «/dev/tun0», «/dev/tun1» и далее, пока одно из них не поддастся
					 *
					 * @note Отыскав номер перебором, мы знаем имя устройства из самого пути, и
					 *       спрашивать его у системы не нужно. Это кстати: разрешения имени по
					 *       описателю у этих систем тоже нет - у NetBSD оно зовётся иначе, а у
					 *       OpenBSD отсутствует
					 *
					 */
					#if __FreeBSD__
						// Создаём сокет для управления UTUN интерфейсом
						result = ::open("/dev/tun", O_RDWR);
					#else
						/**
						 * Выполняем перебор устройств туннеля в поисках свободного
						 */
						for(uint16_t unit = 0; unit < 0x100; unit++){
							// Формируем путь к очередному устройству туннеля
							const string path = ("/dev/tun" + to_string(unit));
							// Если устройство туннеля удалось открыть
							if((result = ::open(path.c_str(), O_RDWR)) != net::invalid_socket_t){
								// Запоминаем имя интерфейса, взятое из пути открытого устройства
								name = path.substr(5);
								// Выходим из цикла перебора, так-как свободное устройство найдено
								break;
							}
						}
					#endif
					// Если сокет не создан
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
					/**
					 * Узнаём имя заведённого интерфейса
					 *
					 * @note Разрешение имени по описателю нужно лишь клонирующему устройству:
					 *       перебор имя уже знает из пути, которым устройство открывал
					 */
					#if __FreeBSD__
					// Выделяем буфер для имени интерфейса
					name.resize(IFNAMSIZ, '\0');
					// Получаем имя созданного интерфейса
					if(::fdevname_r(result, &name[0], IFNAMSIZ) == nullptr){
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
						// Закрываем сокет
						::close(result);
						// Возвращаем результат
						return net::invalid_socket_t;
					}
					// Обрезаем имя интерфейса по нулевому символу
					name.resize(::strlen(name.c_str()));
					#endif
					/**
					 * Включаем заголовок семейства адресов в передаваемых пакетах
					 *
					 * @details Пакет туннеля несёт впереди четыре октета с указанием семейства
					 *          адресов, и разбор в движке на них рассчитан
					 *
					 * @note У OpenBSD настройки этой нет: заголовок там задан выбором самого
					 *       устройства и присутствует у «/dev/tunN» всегда. Задача решается
					 *       выбором устройства, а не настройкой открытого - потому просьба и
					 *       опускается, а не отвечает отказом
					 *
					 */
					#if !__OpenBSD__
					// Флаг активации заголовка
					int32_t flag = 1;
					// Включаем информацию о пакете TUN (заголовок)
					if(::ioctl(result, TUNSIFHEAD, &flag) != 0){
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
						// Закрываем сокет
						::close(result);
						// Возвращаем результат
						return net::invalid_socket_t;
					}
					#endif
				/**
				 * Для операционных систем Sun Solaris и illumos
				 */
				#elif defined(__sun)
					/**
					 * Открываем устройство переноса данных туннеля
					 *
					 * @note Имя устройства обязательно: заводится оно надзорно, и модуль
					 *       лишь открывает уже заведённое - подробности у ::iface::tunnel
					 */
					result = ::iface::tunnel(name, this->_log);
				#endif
			} break;
			// Если создаётся передача кадров Ethernet (с MAC-адресами)
			case static_cast <uint8_t> (event::eth_t::TAP): {
				/**
				 * Для операционной системы macOS
				 */
				#if __APPLE__ || __MACH__
					/**
					 * В штатной поставке macOS нет /dev/tap.
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
					/**
					 * Заводим устройство передачи кадров
					 *
					 * @details У FreeBSD заведено клонирующее устройство, выдающее свободный
					 *          номер само. У OpenBSD его нет, и свободный номер отыскивается
					 *          перебором. NetBSD клонирующее устройство имеет, но разрешения
					 *          имени по описателю у него нет, оттого перебор годится обеим:
					 *          имя тогда известно из самого пути
					 *
					 */
					#if __FreeBSD__
						// Открываем клонирующее устройство
						result = ::open("/dev/tap", O_RDWR);
					#else
						// Сбрасываем описатель устройства перед перебором
						result = net::invalid_socket_t;
						/**
						 * Выполняем перебор устройств передачи кадров в поисках свободного
						 */
						for(uint16_t unit = 0; unit < 0x100; unit++){
							// Формируем путь к очередному устройству передачи кадров
							const string path = ("/dev/tap" + to_string(unit));
							// Если устройство передачи кадров удалось открыть
							if((result = ::open(path.c_str(), O_RDWR)) != net::invalid_socket_t){
								// Запоминаем имя интерфейса, взятое из пути открытого устройства
								name = path.substr(5);
								// Выходим из цикла перебора, так-как свободное устройство найдено
								break;
							}
						}
					#endif
					// Если устройство передачи кадров заведено
					if(result != net::invalid_socket_t){
						/**
						 * Узнаём имя заведённого интерфейса
						 *
						 * @note Разрешение имени по описателю нужно лишь клонирующему устройству:
						 *       перебор имя уже знает из пути, которым устройство открывал
						 */
						#if __FreeBSD__
						// Выделяем буфер для имени интерфейса
						name.resize(IFNAMSIZ, '\0');
						// Получаем имя созданного интерфейса
						if(::fdevname_r(result, &name[0], IFNAMSIZ) == nullptr){
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
							// Закрываем сокет
							::close(result);
							// Возвращаем результат
							return net::invalid_socket_t;
						}
						// Обрезаем имя интерфейса по нулевому символу
						name.resize(::strlen(name.c_str()));
						#endif
					}
				/**
				 * Для операционных систем Sun Solaris и illumos
				 */
				#elif defined(__sun)
					/**
					 * Открываем устройство переноса данных туннеля
					 *
					 * @note Имя устройства обязательно: заводится оно надзорно, и модуль
					 *       лишь открывает уже заведённое - подробности у ::iface::tunnel
					 */
					result = ::iface::tunnel(name, this->_log);
				#endif
			} break;
			// Если создаётся общий туннельный интерфейс (IPv6-in-IPv4, IPv4-in-IPv6, IPv6-in-IPv6)
			case static_cast <uint8_t> (event::eth_t::GIF):
				// Отказываем в заведении интерфейса GIF
				result = ::iface::unsupported("gif", this->_log);
			break;
			// Если создаётся GRE-туннель (включая с ключом)
			case static_cast <uint8_t> (event::eth_t::GRE):
				// Отказываем в заведении интерфейса GRE
				result = ::iface::unsupported("gre", this->_log);
			break;
			// Если создаётся беспроводной интерфейс
			case static_cast <uint8_t> (event::eth_t::WLAN):
				// Отказываем в заведении интерфейса WLAN
				result = ::iface::unsupported("wlan", this->_log);
			break;
			// Если создаётся интерфейс логической сегментации на основе 802.1Q
			case static_cast <uint8_t> (event::eth_t::VLAN):
				// Отказываем в заведении интерфейса VLAN
				result = ::iface::unsupported("vlan", this->_log);
			break;
			// Если создаётся интерфейс агрегации каналов
			case static_cast <uint8_t> (event::eth_t::BOND):
				// Отказываем в заведении интерфейса LAGG
				result = ::iface::unsupported("lagg", this->_log);
			break;
			// Если создаётся интерфейс объединения интерфейсов на уровне L2
			case static_cast <uint8_t> (event::eth_t::BRIDGE):
				// Отказываем в заведении интерфейса Bridge
				result = ::iface::unsupported("bridge", this->_log);
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
			::iface::copyName(ifr.ifr_name, name);
			// Извлекаем MTU из интерфейса
			if(::ioctl(sock, SIOCGIFMTU, &ifr) != 0){
				// Запоминаем причину отказа: закрытие сокета её затрёт
				const int32_t reason = errno;
				// Закрываем сокет
				::close(sock);
				/**
				 * Спрашиваем размер у канального уровня
				 *
				 * @details Список интерфейсов отдаёт `getifaddrs`, и он перечисляет в том
				 *          числе связи канального уровня без поднятого интерфейса IP -
				 *          `etherstub` тому пример. Сокет IP о такой связи не знает и
				 *          отвечает `ENXIO`, но размер у неё есть, и канальный уровень
				 *          его отдаёт
				 *
				 * @warning Прежде отказ здесь был окончательным, и перечисление расходилось
				 *          с опросом: имя, названное `available()`, отвечало нулём у `mtu()`.
				 *          Расхождение это ловилось лишь там, где такая связь заведена
				 */
				const uint32_t mtu = ::iface::datalinkMtu(name);
				// Если канальный уровень размер назвал
				if(mtu > 0)
					// Выводим размер MTU связи канального уровня
					return mtu;
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::strerror(reason));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(reason));
				#endif
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
			::iface::copyName(ifr.ifr_name, name);
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
			::iface::copyName(ifr.ifr_name, name);
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
			::iface::copyName(ifr.ifr_name, name);
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
 * @brief Метод снятия адреса сетевого устройства и пути к тому концу связи
 *
 * @details Снятие ведётся ОБНУЛЕНИЕМ адреса тем же запросом, каким он ставился:
 *          отдельного приёма снятия у Sun Solaris и illumos нет. Установлено щупом
 *          на стенде Solaris 11.4 21.08.2026: после обнуления опрос отдаёт 0.0.0.0,
 *          а состав адресов машины связи уже не показывает
 *
 * @warning Связь канального уровня движку НЕ ПРИНАДЛЕЖИТ - заводится она
 *          административно (`dladm`) и сноситься им не может. Оттого убирать за
 *          собою приходится ровно то, что он ставил: адрес и путь к тому концу
 *
 * @param name название сетевого устройства
 * @param ip   снимаемый адрес
 * @param peer адрес того конца связи, путь к которому снимается
 * @return     результат выполнения снятия
 *
 */
bool awh::eth::Interface::delAddress(string_view name, const net::addr_t * ip, const net::addr_t * peer) const noexcept {
	// Переменная результата
	bool result = false;
	// Если название сетевого устройства и снимаемый адрес переданы
	if(!name.empty() && (ip != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Семейство управляющего сокета обязано совпадать с семейством адреса
			 *
			 * @details Запросы SIOCSLIF* берут семейство у САМОГО сокета, а не у
			 *          передаваемой структуры: сокет AF_INET отвечает на всякий запрос
			 *          с адресом IPv6 отказом «семейство адресов не поддержано
			 *          семейством протокола»
			 */
			// Создаём управляющий сокет семейства снимаемого адреса
			net::socket_t sock = ::socket(((ip->size == 16) ? AF_INET6 : AF_INET), SOCK_DGRAM, 0);
			// Если создание управляющего сокета прошло неудачно
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
			// Объект запроса настройки логического интерфейса
			struct lifreq lifr;
			/**
			 * Снимаем путь к тому концу связи прежде самого адреса
			 *
			 * @details Путь опирается на адрес устройства, и снятие в обратном порядке
			 *          система отвергает: адреса, которым путь проложен, уже нет
			 */
			if(peer != nullptr){
				// Заполняем объект запроса нулями
				::memset(&lifr, 0, sizeof(lifr));
				// Копируем имя сетевого устройства
				::iface::copyName(lifr.lifr_name, name);
				/**
				 * Определяем тип снимаемого адреса
				 */
				switch(peer->size){
					// Если адрес является IPv4
					case 4: {
						// Получаем адрес того конца связи нужного вида
						struct sockaddr_in * addr = reinterpret_cast <struct sockaddr_in *> (&lifr.lifr_dstaddr);
						// Устанавливаем семейство адресов IPv4
						addr->sin_family = AF_INET;
						// Обнуляем адрес того конца связи
						addr->sin_addr.s_addr = INADDR_ANY;
					} break;
					// Если адрес является IPv6
					case 16: {
						// Получаем адрес того конца связи нужного вида
						struct sockaddr_in6 * addr = reinterpret_cast <struct sockaddr_in6 *> (&lifr.lifr_dstaddr);
						// Устанавливаем семейство адресов IPv6
						addr->sin6_family = AF_INET6;
						// Обнуляем адрес того конца связи
						addr->sin6_addr = in6addr_any;
					} break;
				}
				// Снимаем путь к тому концу связи, отказ здесь снятию адреса не помеха
				::ioctl(sock, SIOCSLIFDSTADDR, &lifr);
			}
			/**
			 * Адрес IPv6 снимается СНОСОМ логического интерфейса
			 *
			 * @details Заводился он на своём логическом интерфейсе - обнулять там нечего,
			 *          сносится весь интерфейс целиком. Имени его движок не хранит: узлу
			 *          принадлежит связь, а не логический интерфейс, - оттого имя
			 *          отыскивается по самому адресу
			 *
			 * @warning Снести ОСНОВНОЙ интерфейс нельзя: на нём живёт адрес канальной связи,
			 *          заведённый системой. Поиск потому и ведётся по адресу - основной под
			 *          условие не подходит
			 */
			if(ip->size == 16){
				// Отыскиваем логический интерфейс, несущий снимаемый адрес
				const string target = ::iface::lookup(sock, name, ip);
				// Если логический интерфейс со снимаемым адресом найден
				if(!target.empty()){
					// Заполняем объект запроса нулями
					::memset(&lifr, 0, sizeof(lifr));
					// Копируем имя логического интерфейса
					::iface::copyName(lifr.lifr_name, target);
					// Сносим логический интерфейс вместе с его адресом
					result = (::ioctl(sock, SIOCLIFREMOVEIF, &lifr) == 0);
				}
				// Закрываем управляющий сокет
				::close(sock);
				// Возвращаем результат
				return result;
			}
			// Заполняем объект запроса нулями
			::memset(&lifr, 0, sizeof(lifr));
			// Копируем имя сетевого устройства
			::iface::copyName(lifr.lifr_name, name);
			/**
			 * Определяем тип снимаемого адреса
			 */
			switch(ip->size){
				// Если адрес является IPv4
				case 4: {
					// Получаем адрес устройства нужного вида
					struct sockaddr_in * addr = reinterpret_cast <struct sockaddr_in *> (&lifr.lifr_addr);
					// Устанавливаем семейство адресов IPv4
					addr->sin_family = AF_INET;
					// Обнуляем адрес устройства
					addr->sin_addr.s_addr = INADDR_ANY;
				} break;
				// Если адрес является IPv6
				case 16: {
					// Получаем адрес устройства нужного вида
					struct sockaddr_in6 * addr = reinterpret_cast <struct sockaddr_in6 *> (&lifr.lifr_addr);
					// Устанавливаем семейство адресов IPv6
					addr->sin6_family = AF_INET6;
					// Обнуляем адрес устройства
					addr->sin6_addr = in6addr_any;
				} break;
			}
			// Снимаем адрес сетевого устройства обнулением
			if(!(result = (::ioctl(sock, SIOCSLIFADDR, &lifr) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s: address could not be removed from interface \"%s\": %s", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s: address could not be removed from interface \"%s\": %s", log_t::flag_t::CRITICAL, ::__AWH_IFACE_BACKEND__, string(name).c_str(), ::strerror(errno));
				#endif
			}
			// Закрываем управляющий сокет
			::close(sock);
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
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
			/**
			 * Семейство управляющего сокета обязано совпадать с семейством адреса
			 *
			 * @details Настройка интерфейса у Sun Solaris и illumos идёт запросами
			 *          SIOCSLIF* по управляющему сокету, и семейство берётся у НЕГО,
			 *          а не у передаваемой структуры. Сокет AF_INET отвечает на всякий
			 *          запрос с адресом IPv6 отказом «семейство адресов не поддержано
			 *          семейством протокола», и адрес до интерфейса не доходит вовсе
			 *
			 * @note Установлено на стенде Solaris 11.4 21.08.2026: обмен по IPv6 через
			 *       устройство туннеля не шёл, а причиной значилось негодное окружение
			 */
			// Создаём сокет для управления интерфейсом
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
			/**
			 * Адресу IPv6 нужен СВОЙ логический интерфейс
			 *
			 * @details На основном логическом интерфейсе живёт адрес канальной связи,
			 *          заведённый системой, и подменить его нельзя. Под свой адрес IPv6
			 *          система велит заводить отдельный логический интерфейс
			 *
			 * @warning Адреса IPv4 ложатся на основной интерфейс прямо, и заводить им
			 *          логический незачем: лишний интерфейс пришлось бы ещё и сносить
			 */
			const string target = ((ip->size == 16) ? ::iface::logical(sock, name, this->_log) : string(name));
			// Если логический интерфейс под адрес IPv6 завести не удалось
			if(target.empty()){
				// Закрываем управляющий сокет
				::close(sock);
				// Возвращаем результат
				return result;
			}
			result = ::iface::applyAddress(sock, target, ip, nullptr, prefix, this->_log);
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
			/**
			 * Семейство управляющего сокета обязано совпадать с семейством адреса
			 *
			 * @details Настройка интерфейса у Sun Solaris и illumos идёт запросами
			 *          SIOCSLIF* по управляющему сокету, и семейство берётся у НЕГО,
			 *          а не у передаваемой структуры. Сокет AF_INET отвечает на всякий
			 *          запрос с адресом IPv6 отказом «семейство адресов не поддержано
			 *          семейством протокола», и адрес до интерфейса не доходит вовсе
			 *
			 * @note Установлено на стенде Solaris 11.4 21.08.2026: обмен по IPv6 через
			 *       устройство туннеля не шёл, а причиной значилось негодное окружение
			 */
			// Создаём сокет для управления интерфейсом
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
			/**
			 * Адресу IPv6 нужен СВОЙ логический интерфейс
			 *
			 * @details На основном логическом интерфейсе живёт адрес канальной связи,
			 *          заведённый системой, и подменить его нельзя. Под свой адрес IPv6
			 *          система велит заводить отдельный логический интерфейс
			 *
			 * @warning Адреса IPv4 ложатся на основной интерфейс прямо, и заводить им
			 *          логический незачем: лишний интерфейс пришлось бы ещё и сносить
			 */
			const string target = ((ip->size == 16) ? ::iface::logical(sock, name, this->_log) : string(name));
			// Если логический интерфейс под адрес IPv6 завести не удалось
			if(target.empty()){
				// Закрываем управляющий сокет
				::close(sock);
				// Возвращаем результат
				return result;
			}
			result = ::iface::applyAddress(sock, target, ip, peer, prefix, this->_log);
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
			/**
			 * Семейство управляющего сокета обязано совпадать с семейством адреса
			 *
			 * @details Настройка интерфейса у Sun Solaris и illumos идёт запросами
			 *          SIOCSLIF* по управляющему сокету, и семейство берётся у НЕГО,
			 *          а не у передаваемой структуры. Сокет AF_INET отвечает на всякий
			 *          запрос с адресом IPv6 отказом «семейство адресов не поддержано
			 *          семейством протокола», и адрес до интерфейса не доходит вовсе
			 *
			 * @note Установлено на стенде Solaris 11.4 21.08.2026: обмен по IPv6 через
			 *       устройство туннеля не шёл, а причиной значилось негодное окружение
			 */
			// Создаём единственный управляющий сокет для всех операций
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
			/**
			 * Адресу IPv6 нужен СВОЙ логический интерфейс
			 *
			 * @details На основном логическом интерфейсе живёт адрес канальной связи,
			 *          заведённый системой, и подменить его нельзя. Под свой адрес IPv6
			 *          система велит заводить отдельный логический интерфейс
			 *
			 * @warning Адреса IPv4 ложатся на основной интерфейс прямо, и заводить им
			 *          логический незачем: лишний интерфейс пришлось бы ещё и сносить
			 */
			const string target = ((ip->size == 16) ? ::iface::logical(sock, name, this->_log) : string(name));
			// Если логический интерфейс под адрес IPv6 завести не удалось
			if(target.empty()){
				// Закрываем управляющий сокет
				::close(sock);
				// Возвращаем результат
				return result;
			}
			result = ::iface::applyAddress(sock, target, ip, peer, prefix, this->_log);
			/**
			 * Шаг 2. Устанавливаем MTU интерфейса (если задан)
			 */
			if(result && (mtu > 0)){
				// Настраиваем интерфейс
				struct ifreq ifr{0};
				// Копируем имя интерфейса
				::iface::copyName(ifr.ifr_name, name);
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
				::iface::copyName(ifr.ifr_name, name);
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
