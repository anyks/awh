/**
 * @file: gateway.cpp
 * @date: 2026-01-28
 * @license: LicenseRef-AWH-1.0
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
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(int32_t) - 1))) : sizeof(int32_t))

/**
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <memory>
#include <vector>
#include <cstring>
#include <cstdlib>

/**
 * Системные заголовочные файлы
 */
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/sysctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/gateway.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статичные функции в пространство имён
 *
 */
namespace gw {
	/**
	 * @brief Структура распарсенных адресов сообщения маршрута
	 *
	 */
	typedef struct Addrs {
		// Адрес назначения маршрута
		struct sockaddr * dst;
		// Адрес шлюза маршрута
		struct sockaddr * gw;
		// Маска подсети маршрута
		struct sockaddr * mask;
		// Сетевой интерфейс маршрута (канальный уровень)
		struct sockaddr_dl * ifp;
		/**
		 * @brief Конструктор
		 *
		 */
		Addrs() noexcept :
		 dst(nullptr), gw(nullptr),
		 mask(nullptr), ifp(nullptr) {}
	} addrs_t;

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
		// Возвращаем маску подсети
		return htonl((0xFFFFFFFFU) << (32 - static_cast <uint32_t> (prefix)));
	}
	/**
	 * @brief Функция получения следующего адреса маршрута
	 *
	 * @param addr объект текущего адреса маршрута
	 * @return     объект следующего адреса маршрута
	 */
	static struct sockaddr * advance(struct sockaddr * addr) noexcept {
		// Получаем длину структуры адреса
		const uint32_t length = static_cast <uint32_t> (addr->sa_len ? addr->sa_len : sizeof(long));
		// Извлекаем объект следующего адреса маршрута
		return reinterpret_cast <struct sockaddr *> (reinterpret_cast <uint8_t *> (addr) + ROUNDUP(length));
	}
	/**
	 * @brief Функция разбора адресов сообщения маршрута
	 *
	 * @param rtm объект сообщения маршрута
	 * @return    структура распарсенных адресов маршрута
	 */
	static addrs_t parse(const struct rt_msghdr * rtm) noexcept {
		// Результат разбора адресов маршрута
		addrs_t result;
		// Объект текущего адреса маршрута
		struct sockaddr * sa = reinterpret_cast <struct sockaddr *> (const_cast <struct rt_msghdr *> (rtm) + 1);
		// Если присутствует адрес назначения в маршруте
		if(rtm->rtm_addrs & RTA_DST){
			// Извлекаем адрес назначения маршрута
			result.dst = sa;
			// Переходим к следующему адресу маршрута
			sa = advance(sa);
		}
		// Если присутствует шлюз в маршруте
		if(rtm->rtm_addrs & RTA_GATEWAY){
			// Если адрес шлюза является ссылочным
			if(sa->sa_family == AF_LINK)
				// Извлекаем сетевой интерфейс маршрута
				result.ifp = reinterpret_cast <struct sockaddr_dl *> (sa);
			// Иначе извлекаем адрес шлюза маршрута
			else result.gw = sa;
			// Переходим к следующему адресу маршрута
			sa = advance(sa);
		}
		// Если присутствует маска подсети в маршруте
		if(rtm->rtm_addrs & RTA_NETMASK){
			// Извлекаем маску подсети маршрута
			result.mask = sa;
			// Переходим к следующему адресу маршрута
			sa = advance(sa);
		}
		// Если присутствует маска клонирования в маршруте
		if(rtm->rtm_addrs & RTA_GENMASK)
			// Переходим к следующему адресу маршрута
			sa = advance(sa);
		// Если присутствует сетевой интерфейс в маршруте
		if(rtm->rtm_addrs & RTA_IFP){
			// Если сетевой интерфейс ещё не был извлечён и адрес является ссылочным
			if((result.ifp == nullptr) && (sa->sa_family == AF_LINK))
				// Извлекаем сетевой интерфейс маршрута
				result.ifp = reinterpret_cast <struct sockaddr_dl *> (sa);
			// Переходим к следующему адресу маршрута
			sa = advance(sa);
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция запроса маршрута для адреса назначения через RTM_GET
	 *
	 * @param dst    адрес назначения для запроса маршрута
	 * @param buffer буфер для приёма ответа маршрута
	 * @param size   размер буфера для приёма ответа маршрута
	 * @param rtm    объект полученного маршрута (выходной параметр)
	 * @return       результат выполнения запроса маршрута
	 */
	static bool query(const struct sockaddr * dst, uint8_t * buffer, const size_t size, struct rt_msghdr ** rtm) noexcept {
		// Создаём сокет маршрутизации
		const awh::net::socket_t sock = ::socket(PF_ROUTE, SOCK_RAW, 0);
		// Если сокет не создан
		if(sock == awh::net::invalid_socket_t)
			// Выводим результат
			return false;
		// Буфер запроса маршрута
		uint8_t request[512];
		// Зануляем буфер запроса маршрута
		::memset(request, 0, sizeof(request));
		// Получаем объект заголовка запроса маршрута
		struct rt_msghdr * rtq = reinterpret_cast <struct rt_msghdr *> (request);
		// Получаем буфер полезной нагрузки запроса маршрута
		uint8_t * cp = (request + sizeof(struct rt_msghdr));
		// Устанавливаем тип сообщения на получение маршрута
		rtq->rtm_type = RTM_GET;
		// Устанавливаем флаги маршрута
		rtq->rtm_flags = RTF_UP;
		// Устанавливаем используемые поля адресов
		rtq->rtm_addrs = RTA_DST;
		// Устанавливаем версию маршрута
		rtq->rtm_version = RTM_VERSION;
		// Получаем идентификатор процесса
		const pid_t pid = ::getpid();
		// Устанавливаем идентификатор процесса
		rtq->rtm_pid = pid;
		// Получаем уникальный порядковый номер запроса
		static int32_t seq = 0;
		// Вычисляем текущий порядковый номер запроса
		const int32_t current = ++seq;
		// Устанавливаем порядковый номер запроса
		rtq->rtm_seq = current;
		// Копируем адрес назначения в запрос маршрута
		::memcpy(cp, dst, dst->sa_len);
		// Смещаем указатель полезной нагрузки
		cp += ROUNDUP(dst->sa_len);
		// Устанавливаем полный размер сообщения запроса
		rtq->rtm_msglen = static_cast <uint16_t> (cp - request);
		// Отправляем запрос маршрута
		if(::write(sock, request, rtq->rtm_msglen) <= 0){
			// Закрываем сокет маршрутизации
			::close(sock);
			// Выводим результат
			return false;
		}
		// Размер прочитанных данных
		ssize_t bytes = 0;
		/**
		 * Читаем ответы, пока не получим свой по порядковому номеру и идентификатору процесса
		 */
		do {
			// Читаем очередной ответ маршрута
			bytes = ::read(sock, buffer, size);
		/**
		 * Проверяем, что данные из сокета прочитаны удачно, а также совпадает порядковый номер и идентификатор процесса
		 */
		} while((bytes > 0) && ((reinterpret_cast <struct rt_msghdr *> (buffer)->rtm_seq != current) || (reinterpret_cast <struct rt_msghdr *> (buffer)->rtm_pid != pid)));
		// Закрываем сокет маршрутизации
		::close(sock);
		// Если ответ не получен
		if(bytes <= 0)
			// Выводим результат
			return false;
		// Устанавливаем указатель на полученный маршрут
		(* rtm) = reinterpret_cast <struct rt_msghdr *> (buffer);
		// Выводим результат
		return true;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::eth::Gateway::Route::Route() noexcept :
 ifname{""}, prefix(0),
 destination(nullptr), gateway(nullptr) {}

/**
 * @brief Метод получения маршрута для указанного адреса
 *
 * @param route объект для извлечения маршрута
 * @return      результат получения маршрута
 */
bool awh::eth::Gateway::get(route_t & route) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес шлюза инициализирован
		if(route.gateway != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(route.gateway->size){
				// Если адрес является IPv4
				case 4: {
					// Индекс сетевого интерфейса для поиска
					uint32_t searchIfIndex = 0;
					// Если задано имя интерфейса
					if(!route.ifname.empty())
						// Получаем индекс интерфейса по имени
						searchIfIndex = ::if_nametoindex(route.ifname.c_str());
					// Если адрес назначения не инициализирован
					if(route.destination == nullptr)
						// Инициализируем объект адреса назначения в маршруте
						route.destination = make_unique <net::addr_net_ipv4_t> ();
					// Флаг поиска маршрута по умолчанию (если ничего не задано)
					const bool lookForDefault = (
						(awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address == 0) &&
						(awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address == 0) && (searchIfIndex == 0)
					);
					// Индекс сетевого интерфейса найденного маршрута
					uint32_t foundIfIndex = 0;
					/**
					 * @brief Функция сопоставления и заполнения маршрута из сообщения
					 *
					 * @param rtm объект сообщения маршрута
					 * @return    результат совпадения и заполнения маршрута
					 */
					auto process = [&](struct rt_msghdr * rtm) -> bool {
						// Разбираем адреса сообщения маршрута
						const ::gw::addrs_t addrs = ::gw::parse(rtm);
						// Объект адреса шлюза маршрута
						struct sockaddr_in * gw = (((addrs.gw != nullptr) && (addrs.gw->sa_family == AF_INET)) ? reinterpret_cast <struct sockaddr_in *> (addrs.gw) : nullptr);
						// Объект адреса назначения маршрута
						struct sockaddr_in * dst = (((addrs.dst != nullptr) && (addrs.dst->sa_family == AF_INET)) ? reinterpret_cast <struct sockaddr_in *> (addrs.dst) : nullptr);
						// Объект маски подсети маршрута
						struct sockaddr_in * mask = (((addrs.mask != nullptr) && (addrs.mask->sa_family == AF_INET)) ? reinterpret_cast <struct sockaddr_in *> (addrs.mask) : nullptr);
						// Объект сетевого интерфейса маршрута
						struct sockaddr_dl * ifp = addrs.ifp;
						// Предполагаем совпадение и проверяем условия
						bool match = true;
						// Если ищем маршрут по умолчанию
						if(lookForDefault)
							// Если адрес назначения 0 (и маска подразумевается 0 или отсутствует)
							match = ((dst != nullptr ? dst->sin_addr.s_addr : 0) == 0);
						// Если ищем конкретный маршрут
						else {
							// Если задан gateway, он должен совпадать
							if(awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address != 0)
								// Накапливаем результат совпадения по адресу шлюза
								match = match && ((gw != nullptr ? gw->sin_addr.s_addr : 0) == awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address);
							// Если задан destination, он должен совпадать
							if(awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address != 0)
								// Накапливаем результат совпадения по адресу назначения
								match = match && ((dst != nullptr ? dst->sin_addr.s_addr : 0) == awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address);
							// Если задан интерфейс
							if(searchIfIndex != 0)
								// Накапливаем результат совпадения по индексу интерфейса
								match = match && ((ifp != nullptr) && (ifp->sdl_index == searchIfIndex));
						}
						// Если маршрут не найден
						if(!match)
							// Выводим результат
							return false;
						// Запоминаем индекс сетевого интерфейса найденного маршрута
						foundIfIndex = (ifp != nullptr ? ifp->sdl_index : rtm->rtm_index);
						// Если задан адрес шлюза
						if(gw != nullptr)
							// Устанавливаем адрес шлюза
							awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = gw->sin_addr.s_addr;
						// Иначе зануляем адрес шлюза
						else awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = 0;
						// Если задан адрес назначения
						if(dst != nullptr)
							// Устанавливаем адрес назначения
							awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address = dst->sin_addr.s_addr;
						// Иначе зануляем адрес назначения
						else awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address = 0;
						// Вычисляем префикс
						route.prefix = 0;
						// Если задана маска подсети
						if(mask != nullptr){
							// Преобразуем маску в префикс
							uint32_t addr = ntohl(mask->sin_addr.s_addr);
							/**
							 * Подсчитываем количество единичных бит в маске
							 */
							while(addr & 0x80000000){
								// Увеличиваем префикс
								route.prefix++;
								// Сдвигаем маску влево
								addr <<= 1;
							}
						// Если маска не задана
						} else if(rtm->rtm_flags & RTF_HOST)
							// Если это хостовый маршрут без маски, то /32
							route.prefix = 32;
						// Выводим результат
						return true;
					};
					/**
					 * Быстрый путь: при незаданном адресе назначения запрашиваем маршрут напрямую через RTM_GET.
					 * Для конкретного адреса назначения используется полный дамп (точное совпадение).
					 */
					if(awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address == 0){
						// Структура адреса назначения для запроса (0 — маршрут по умолчанию)
						struct sockaddr_in qdst{0};
						// Устанавливаем длину структуры
						qdst.sin_len = sizeof(struct sockaddr_in);
						// Устанавливаем семейство адресов
						qdst.sin_family = AF_INET;
						// Буфер для приёма ответа маршрута
						uint8_t reply[2048];
						// Объект полученного маршрута
						struct rt_msghdr * rtm = nullptr;
						// Выполняем запрос маршрута и обрабатываем ответ
						if(::gw::query(reinterpret_cast <struct sockaddr *> (&qdst), reply, sizeof(reply), &rtm) && process(rtm))
							// Устанавливаем флаг успешного поиска маршрута
							result = true;
					}
					/**
					 * Медленный путь: перебираем всю таблицу маршрутизации
					 */
					if(!result){
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
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
						// Буфер данных для получения маршрутов
						vector <uint8_t> buffer(length, 0);
						// Извлекаем маршруты в буфер
						if(::sysctl(mib, 6, &buffer[0], &length, nullptr, 0) < 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
						// Получаем итератор следующего маршрута
						uint8_t * begin = &buffer[0];
						// Получаем конец всех маршрутов
						uint8_t * end = (begin + length);
						/**
						 * Перебираем все маршруты
						 */
						while(begin < end){
							// Если оставшихся данных недостаточно для заголовка маршрута
							if(static_cast <size_t> (end - begin) < sizeof(struct rt_msghdr))
								// Выходим из цикла
								break;
							// Приводим к структуре маршрута
							struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
							// Если версия маршрута не совпадает
							if(rtm->rtm_version != RTM_VERSION)
								// Выходим из цикла
								break;
							// Если длина сообщения некорректна или выходит за границы буфера
							if((rtm->rtm_msglen == 0) || (static_cast <size_t> (end - begin) < rtm->rtm_msglen))
								// Выходим из цикла
								break;
							// Обрабатываем сообщение маршрута
							if(process(rtm)){
								// Устанавливаем флаг успешного поиска маршрута
								result = true;
								// Выходим из цикла (первый найденный)
								break;
							}
							// Переходим к следующему маршруту
							begin += rtm->rtm_msglen;
						}
					}
					// Если найден маршрут и известен индекс сетевого интерфейса
					if(result && (foundIfIndex > 0)){
						// Буфер для имени сетевого интерфейса
						char ifname[IF_NAMESIZE];
						// Зануляем буфер имени сетевого интерфейса
						::memset(ifname, 0, sizeof(ifname));
						// Получаем имя сетевого интерфейса по его индексу
						if(::if_indextoname(foundIfIndex, ifname) != nullptr)
							// Устанавливаем имя сетевого интерфейса
							route.ifname = ifname;
					}
				} break;
				// Если адрес является IPv6
				case 16: {
					// Если адрес назначения не инициализирован
					if(route.destination == nullptr)
						// Инициализируем объект адреса назначения в маршруте
						route.destination = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем нулевой адрес для сравнения
					const uint8_t zeroAddr[16] = {0};
					// Получаем адрес шлюза для поиска
					const uint8_t * searchGw = &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0];
					// Получаем адрес назначения для поиска
					const uint8_t * searchDest = &awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0];
					// Флаг задания адреса шлюза
					const bool isGw = (::memcmp(searchGw, zeroAddr, 16) != 0);
					// Флаг задания адреса назначения
					const bool isDest = (::memcmp(searchDest, zeroAddr, 16) != 0);
					// Индекс сетевого интерфейса для поиска
					uint32_t searchIfIndex = 0;
					// Если задано имя интерфейса
					if(!route.ifname.empty())
						// Получаем индекс интерфейса по имени
						searchIfIndex = ::if_nametoindex(route.ifname.c_str());
					// Флаг поиска маршрута по умолчанию
					const bool lookForDefault = (!isDest && !isGw && (searchIfIndex == 0));
					// Индекс сетевого интерфейса найденного маршрута
					uint32_t foundIfIndex = 0;
					/**
					 * @brief Функция сопоставления и заполнения маршрута из сообщения
					 *
					 * @param rtm объект сообщения маршрута
					 * @return    результат совпадения и заполнения маршрута
					 */
					auto process = [&](struct rt_msghdr * rtm) -> bool {
						// Разбираем адреса сообщения маршрута
						const ::gw::addrs_t addrs = ::gw::parse(rtm);
						// Объект адреса шлюза маршрута
						struct sockaddr_in6 * gw = (((addrs.gw != nullptr) && (addrs.gw->sa_family == AF_INET6)) ? reinterpret_cast <struct sockaddr_in6 *> (addrs.gw) : nullptr);
						// Объект адреса назначения маршрута
						struct sockaddr_in6 * dst = (((addrs.dst != nullptr) && (addrs.dst->sa_family == AF_INET6)) ? reinterpret_cast <struct sockaddr_in6 *> (addrs.dst) : nullptr);
						// Объект маски подсети маршрута
						struct sockaddr_in6 * mask = (((addrs.mask != nullptr) && (addrs.mask->sa_family == AF_INET6)) ? reinterpret_cast <struct sockaddr_in6 *> (addrs.mask) : nullptr);
						// Объект сетевого интерфейса маршрута
						struct sockaddr_dl * ifp = addrs.ifp;
						// Предполагаем совпадение и проверяем условия
						bool match = true;
						// Если ищем маршрут по умолчанию
						if(lookForDefault)
							// Если адрес назначения задан и не равен 0
							match = ((dst == nullptr) || IN6_IS_ADDR_UNSPECIFIED(&dst->sin6_addr));
						// Если ищем конкретный маршрут
						else {
							// Проверка шлюза
							if(isGw)
								// Накапливаем результат совпадения по адресу шлюза
								match = match && ((gw != nullptr) && (::memcmp(&gw->sin6_addr, searchGw, 16) == 0));
							// Проверка назначения
							if(isDest)
								// Накапливаем результат совпадения по адресу назначения
								match = match && ((dst != nullptr) && (::memcmp(&dst->sin6_addr, searchDest, 16) == 0));
							// Проверка интерфейса
							if(searchIfIndex != 0)
								// Накапливаем результат совпадения по индексу интерфейса
								match = match && ((ifp != nullptr) && (ifp->sdl_index == searchIfIndex));
						}
						// Если маршрут не найден
						if(!match)
							// Выводим результат
							return false;
						// Запоминаем индекс сетевого интерфейса найденного маршрута
						foundIfIndex = (ifp != nullptr ? ifp->sdl_index : rtm->rtm_index);
						// Если задан адрес шлюза
						if(gw != nullptr)
							// Устанавливаем адрес шлюза
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], &gw->sin6_addr, 16);
						// Иначе зануляем адрес шлюза
						else ::memset(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 0, 16);
						// Если задан адрес назначения
						if(dst != nullptr)
							// Устанавливаем адрес назначения
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], &dst->sin6_addr, 16);
						// Иначе зануляем адрес назначения
						else ::memset(&awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], 0, 16);
						// Вычисляем префикс
						route.prefix = 0;
						// Если задана маска подсети
						if(mask != nullptr){
							/**
							 * Преобразуем маску в префикс
							 */
							for(uint8_t i = 0; i < 16; ++i){
								// Получаем байт маски
								uint8_t byte = mask->sin6_addr.s6_addr[i];
								/**
								 * Подсчитываем количество единичных бит в маске
								 */
								while(byte & 0x80){
									// Увеличиваем префикс
									route.prefix++;
									// Сдвигаем байт влево
									byte <<= 1;
								}
							}
						// Если маска не задана
						} else if(rtm->rtm_flags & RTF_HOST)
							// Если это хостовый маршрут без маски, то /128
							route.prefix = 128;
						// Выводим результат
						return true;
					};
					/**
					 * Быстрый путь: при незаданном адресе назначения запрашиваем маршрут напрямую через RTM_GET.
					 * Для конкретного адреса назначения используется полный дамп (точное совпадение).
					 */
					if(!isDest){
						// Структура адреса назначения для запроса (0 — маршрут по умолчанию)
						struct sockaddr_in6 qdst{0};
						// Устанавливаем длину структуры
						qdst.sin6_len = sizeof(struct sockaddr_in6);
						// Устанавливаем семейство адресов
						qdst.sin6_family = AF_INET6;
						// Буфер для приёма ответа маршрута
						uint8_t reply[2048];
						// Объект полученного маршрута
						struct rt_msghdr * rtm = nullptr;
						// Выполняем запрос маршрута и обрабатываем ответ
						if(::gw::query(reinterpret_cast <struct sockaddr *> (&qdst), reply, sizeof(reply), &rtm) && process(rtm))
							// Устанавливаем флаг успешного поиска маршрута
							result = true;
					}
					/**
					 * Медленный путь: перебираем всю таблицу маршрутизации
					 */
					if(!result){
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
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
						// Буфер данных для получения маршрутов
						vector <uint8_t> buffer(length, 0);
						// Извлекаем маршруты в буфер
						if(::sysctl(mib, 6, &buffer[0], &length, nullptr, 0) < 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
						// Получаем итератор следующего маршрута
						uint8_t * begin = &buffer[0];
						// Получаем конец всех маршрутов
						uint8_t * end = (begin + length);
						/**
						 * Перебираем все маршруты
						 */
						while(begin < end){
							// Если оставшихся данных недостаточно для заголовка маршрута
							if(static_cast <size_t> (end - begin) < sizeof(struct rt_msghdr))
								// Выходим из цикла
								break;
							// Приводим к структуре маршрута
							struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
							// Если версия маршрута не совпадает
							if(rtm->rtm_version != RTM_VERSION)
								// Выходим из цикла
								break;
							// Если длина сообщения некорректна или выходит за границы буфера
							if((rtm->rtm_msglen == 0) || (static_cast <size_t> (end - begin) < rtm->rtm_msglen))
								// Выходим из цикла
								break;
							// Обрабатываем сообщение маршрута
							if(process(rtm)){
								// Устанавливаем флаг успешного поиска маршрута
								result = true;
								// Выходим из цикла (первый найденный)
								break;
							}
							// Переходим к следующему маршруту
							begin += rtm->rtm_msglen;
						}
					}
					// Если найден маршрут и известен индекс сетевого интерфейса
					if(result && (foundIfIndex > 0)){
						// Буфер для имени сетевого интерфейса
						char ifname[IF_NAMESIZE];
						// Зануляем буфер имени сетевого интерфейса
						::memset(ifname, 0, sizeof(ifname));
						// Получаем имя сетевого интерфейса по его индексу
						if(::if_indextoname(foundIfIndex, ifname) != nullptr)
							// Устанавливаем имя сетевого интерфейса
							route.ifname = ifname;
					}
				} break;
				// Во всех остальных случаях
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unsupported address family", __PRETTY_FUNCTION__, make_tuple(route.gateway->size), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unsupported address family", log_t::flag_t::CRITICAL);
					#endif
				} break;
			}
		// Если адрес шлюза не инициализирован
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Gateway address is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Gateway address is not initialized", log_t::flag_t::CRITICAL);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод добавления маршрута
 *
 * @param route объект маршрута для добавления
 * @return      результат добавления маршрута
 */
bool awh::eth::Gateway::add(const route_t & route) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес шлюза инициализирован
		if(route.gateway != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(route.gateway->size){
				// Если адрес является IPv4
				case 4: {
					// Создаём сокет для добавления маршрутов
					net::socket_t sock = ::socket(PF_ROUTE, SOCK_RAW, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Буфер для добавления маршрута
					char buffer[1024];
					// Зануляем буфер для добавления маршрута
					::memset(buffer, 0, sizeof(buffer));
					// Структура сообщения для добавления маршрута
					struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (buffer);
					// Буфер полезной нагрузки
					char * cp = (buffer + sizeof(struct rt_msghdr));
					// Порядковый номер устанавливаемого маршрута
					rtm->rtm_seq = 1;
					// Идентификатор процесса
					rtm->rtm_pid = ::getpid();
					// Тип сообщения
					rtm->rtm_type = RTM_ADD;
					// Версия маршрута
					rtm->rtm_version = RTM_VERSION;
					// Флаги: UP и STATIC
					rtm->rtm_flags = (RTF_UP | RTF_STATIC);
					// Маска используемых полей
					rtm->rtm_addrs = (RTA_DST | RTA_NETMASK);
					// Адрес назначения (RTA_DST)
					struct sockaddr_in dst{0};
					// Устанавливаем длину структуры
					dst.sin_len = sizeof(struct sockaddr_in);
					// Устанавливаем семейство адресов
					dst.sin_family = AF_INET;
					// Если адрес назначения не инициализирован
					if(route.destination == nullptr)
						// Инициализируем объект адреса назначения в маршруте
						const_cast <route_t &> (route).destination = make_unique <net::addr_net_ipv4_t> ();
					// Получаем адрес назначения
					dst.sin_addr.s_addr = awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address;
					// Копируем в буфер адрес назначения
					::memcpy(cp, &dst, dst.sin_len);
					// Сдвигаем буфер полезной нагрузки
					cp += ROUNDUP(dst.sin_len);
					// Получаем флаг установленного сетевого интерфейса
					const bool isIfname = (!route.ifname.empty());
					// Получаем флаг установленного шлюза
					const bool isGateway = (awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address > 0);
					// Получаем флаг маршрута по умолчанию
					const bool isDefault = (awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address == 0);
					/**
					 * Случай 1: Specific Route + Gateway IP (Ifname не указан) [$ sudo route add -net 1.0.0.0/8 10.0.0.1]
					 * Случай 4: Default Route + Gateway IP (Ifname игнорируется или не важен) [$ sudo route add default 192.168.7.1]
					 */
					if(isGateway){
						// Структура IPv4 шлюза
						struct sockaddr_in gw{0};
						// Устанавливаем длину структуры
						gw.sin_len = sizeof(struct sockaddr_in);
						// Устанавливаем семейство адресов
						gw.sin_family = AF_INET;
						// Устанавливаем адрес шлюза
						gw.sin_addr.s_addr = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
						// Устанавливаем флаг шлюза (так как это IP)
						rtm->rtm_flags |= RTF_GATEWAY;
						// Устанавливаем флаг адреса шлюза
						rtm->rtm_addrs |= RTA_GATEWAY;
						// Устанавливаем в буфер адрес шлюза
						::memcpy(cp, &gw, gw.sin_len);
						// Сдвигаем буфер полезной нагрузки
						cp += ROUNDUP(gw.sin_len);
					/**
					 * Случай 3: Default Route + No Gateway IP + Ifname [$ sudo route add default 192.168.7.1]
					 * Нужно найти IP адрес интерфейса и использовать его как шлюз
					 */
					} else if(isDefault && !isGateway && isIfname) {
						// Флаг нахождения адреса
						bool found = false;
						// Список сетевых интерфейсов
						struct ifaddrs * ifptr = nullptr;
						// Получаем список сетевых интерфейсов
						if(::getifaddrs(&ifptr) == 0){
							/**
							 * Перебираем интерфейсы
							 */
							for(struct ifaddrs * ifa = ifptr; ifa != nullptr; ifa = ifa->ifa_next){
								// Если интерфейс совпадает и семейство адресов IPv4
								if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_INET) && (::strcmp(ifa->ifa_name, route.ifname.c_str()) == 0)){
									// Структура IPv4 шлюза
									struct sockaddr_in gw{0};
									// Устанавливаем длину структуры
									gw.sin_len = sizeof(struct sockaddr_in);
									// Устанавливаем семейство адресов
									gw.sin_family = AF_INET;
									// Устанавливаем адрес шлюза (IP интерфейса)
									gw.sin_addr = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr)->sin_addr;
									// Устанавливаем флаг шлюза (так как это IP)
									rtm->rtm_flags |= RTF_GATEWAY;
									// Устанавливаем флаг адреса шлюза
									rtm->rtm_addrs |= RTA_GATEWAY;
									// Устанавливаем в буфер адрес шлюза
									::memcpy(cp, &gw, gw.sin_len);
									// Сдвигаем буфер полезной нагрузки
									cp += ROUNDUP(gw.sin_len);
									// Устанавливаем флаг нахождения
									found = true;
									// Прерываем цикл
									break;
								}
							}
							// Освобождаем список интерфейсов
							::freeifaddrs(ifptr);
						}
						// Если IP-адрес сетевого интерфейса не найден
						if(!found){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to find IPv4 address for interface \"%s\"", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, route.ifname.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to find IPv4 address for interface \"%s\"", log_t::flag_t::WARNING, route.ifname.c_str());
							#endif
						}
					/**
					 * Случай 2: Specific Route + No Gateway IP + Ifname [$ sudo route add -net 10.0.0.0/8 -interface utun6]
					 * Используем интерфейс как link-layer gateway (-interface)
					 */
					} else if(!isDefault && !isGateway && isIfname) {
						// Получаем индекс интерфейса
						const uint32_t index = ::if_nametoindex(route.ifname.c_str());
						// Если найден интерфейс по имени
						if(index > 0) {
							// Структура канального уровня
							struct sockaddr_dl sdl{0};
							// Устанавливаем длину структуры
							sdl.sdl_len = sizeof(struct sockaddr_dl);
							// Устанавливаем семейство адресов
							sdl.sdl_family = AF_LINK;
							// Устанавливаем индекс интерфейса
							sdl.sdl_index = static_cast <uint16_t> (index);
							// Устанавливаем флаг адреса шлюза (RTA_GATEWAY), но НЕ RTF_GATEWAY
							rtm->rtm_addrs |= RTA_GATEWAY;
							// Устанавливаем в буфер адрес шлюза
							::memcpy(cp, &sdl, sdl.sdl_len);
							// Сдвигаем буфер полезной нагрузки
							cp += ROUNDUP(sdl.sdl_len);
						}
					}
					// Маска подсети (RTA_NETMASK)
					struct sockaddr_in mask{0};
					// Устанавливаем длину структуры
					mask.sin_len = sizeof(struct sockaddr_in);
					// Устанавливаем семейство адресов
					mask.sin_family = AF_INET;
					// Если адрес назначения является default route
					if(dst.sin_addr.s_addr == 0)
						// Default route - mask 0
						mask.sin_addr.s_addr = 0;
					// Если префикс задан
					else if(route.prefix > 0)
						// Prefix defined
						mask.sin_addr.s_addr = ::gw::prefix2mask(route.prefix);
					// Если префикс сети не установлен
					else {
						// Устанавливаем флаг хостового маршрута
						rtm->rtm_flags |= RTF_HOST;
						// Устанавливаем маску соответствующую префиксу /32
						mask.sin_addr.s_addr = 0xFFFFFFFF;
					}
					// Копируем маску подсети в буфер
					::memcpy(cp, &mask, mask.sin_len);
					// Сдвигаем буфер полезной нагрузки
					cp += ROUNDUP(mask.sin_len);
					// Полный размер сообщения
					rtm->rtm_msglen = (cp - buffer);
					// Записываем маршрут в систему
					if(!(result = (::write(sock, buffer, rtm->rtm_msglen) > 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
				} break;
				// Если адрес является IPv6
				case 16: {
					// Создаём сокет для добавления маршрутов
					net::socket_t sock = ::socket(PF_ROUTE, SOCK_RAW, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Буфер для добавления маршрута
					char buffer[1024];
					// Зануляем буфер для добавления маршрута
					::memset(buffer, 0, sizeof(buffer));
					// Структура сообщения для добавления маршрута
					struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (buffer);
					// Буфер полезной нагрузки
					char * cp = (buffer + sizeof(struct rt_msghdr));
					// Устанавливаем порядковый номер устанавливаемого маршрута
					rtm->rtm_seq = 1;
					// Устанавливаем идентификатор процесса
					rtm->rtm_pid = ::getpid();
					// Устанавливаем тип сообщения
					rtm->rtm_type = RTM_ADD;
					// Устанавливаем версию маршрута
					rtm->rtm_version = RTM_VERSION;
					// Устанавливаем флаги: UP и STATIC
					rtm->rtm_flags = (RTF_UP | RTF_STATIC);
					// Устанавливаем маску используемых полей
					rtm->rtm_addrs = (RTA_DST | RTA_NETMASK);
					// Адрес назначения (RTA_DST)
					struct sockaddr_in6 dst{0};
					// Устанавливаем длину структуры
					dst.sin6_len = sizeof(struct sockaddr_in6);
					// Устанавливаем семейство адресов
					dst.sin6_family = AF_INET6;
					// Если адрес назначения не инициализирован
					if(route.destination == nullptr)
						// Инициализируем объект адреса назначения в маршруте
						const_cast <route_t &> (route).destination = make_unique <net::addr_net_ipv6_t> ();
					// Получаем адрес назначения
					::memcpy(&dst.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], 16);
					// Копируем в буфер адрес назначения
					::memcpy(cp, &dst, dst.sin6_len);
					// Сдвигаем буфер полезной нагрузки
					cp += ROUNDUP(dst.sin6_len);
					// Нулевой адрес для проверки
					const uint8_t zeroAddr[16] = {0};
					// Получаем флаг установленного сетевого интерфейса
					const bool isIfname = (!route.ifname.empty());
					// Получаем флаг маршрута по умолчанию
					const bool isDefault = IN6_IS_ADDR_UNSPECIFIED(&dst.sin6_addr);
					// Получаем флаг установленного шлюза
					const bool isGateway = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], zeroAddr, 16) != 0);
					/**
					 * Случай 1: Specific Route + Gateway IP (Ifname не указан) [$ sudo route add -net 1.0.0.0/8 10.0.0.1]
					 * Случай 4: Default Route + Gateway IP (Ifname игнорируется или не важен) [$ sudo route add default 192.168.7.1]
					 */
					if(isGateway){
						// Структура IPv6 шлюза
						struct sockaddr_in6 gw{0};
						// Устанавливаем длину структуры
						gw.sin6_len = sizeof(struct sockaddr_in6);
						// Устанавливаем семейство адресов
						gw.sin6_family = AF_INET6;
						// Устанавливаем адрес шлюза
						::memcpy(&gw.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 16);
						// Устанавливаем флаг шлюза
						rtm->rtm_flags |= RTF_GATEWAY;
						// Устанавливаем флаг адреса шлюза
						rtm->rtm_addrs |= RTA_GATEWAY;
						// Копируем в буфер адрес шлюза
						::memcpy(cp, &gw, gw.sin6_len);
						// Сдвигаем буфер полезной нагрузки
						cp += ROUNDUP(gw.sin6_len);
					/**
					 * Случай 3: Default Route + No Gateway IP + Ifname [$ sudo route add default 192.168.7.1]
					 * Нужно найти IP адрес интерфейса и использовать его как шлюз
					 */
					} else if(isDefault && !isGateway && isIfname) {
						// Флаг нахождения адреса
						bool found = false;
						// Список сетевых интерфейсов
						struct ifaddrs * ifptr = nullptr;
						// Получаем список сетевых интерфейсов
						if(::getifaddrs(&ifptr) == 0){
							/**
							 * Перебираем интерфейсы
							 */
							for(struct ifaddrs * ifa = ifptr; ifa != nullptr; ifa = ifa->ifa_next){
								// Если интерфейс совпадает и семейство адресов IPv6
								if((ifa->ifa_addr != nullptr) && (ifa->ifa_addr->sa_family == AF_INET6) && (::strcmp(ifa->ifa_name, route.ifname.c_str()) == 0)){
									// Структура IPv6 шлюза
									struct sockaddr_in6 gw{0};
									// Устанавливаем длину структуры
									gw.sin6_len = sizeof(struct sockaddr_in6);
									// Устанавливаем семейство адресов
									gw.sin6_family = AF_INET6;
									// Устанавливаем адрес шлюза
									gw.sin6_addr = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr)->sin6_addr;
									// Устанавливаем флаг шлюза
									rtm->rtm_flags |= RTF_GATEWAY;
									// Устанавливаем флаг адреса шлюза
									rtm->rtm_addrs |= RTA_GATEWAY;
									// Копируем в буфер адрес шлюза
									::memcpy(cp, &gw, gw.sin6_len);
									// Сдвигаем буфер полезной нагрузки
									cp += ROUNDUP(gw.sin6_len);
									// Устанавливаем флаг нахождения
									found = true;
									// Прерываем цикл
									break;
								}
							}
							// Освобождаем список интерфейсов
							::freeifaddrs(ifptr);
						}
						// Если IP-адрес сетевого интерфейса не найден
						if(!found){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to find IPv6 address for interface \"%s\"", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, route.ifname.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to find IPv6 address for interface \"%s\"", log_t::flag_t::WARNING, route.ifname.c_str());
							#endif
						}
					/**
					 * Случай 2: Specific Route + No Gateway IP + Ifname [$ sudo route add -net 10.0.0.0/8 -interface utun6]
					 * Используем интерфейс как link-layer gateway (-interface)
					 */
					} else if(!isDefault && !isGateway && isIfname) {
						// Получаем индекс интерфейса
						const uint32_t index = ::if_nametoindex(route.ifname.c_str());
						// Если найден интерфейс по имени
						if(index > 0) {
							// Структура канального уровня
							struct sockaddr_dl sdl{0};
							// Устанавливаем длину структуры
							sdl.sdl_len = sizeof(struct sockaddr_dl);
							// Устанавливаем семейство адресов
							sdl.sdl_family = AF_LINK;
							// Устанавливаем индекс интерфейса
							sdl.sdl_index = static_cast<uint16_t>(index);
							// Устанавливаем флаг адреса шлюза (RTA_GATEWAY), но НЕ RTF_GATEWAY
							rtm->rtm_addrs |= RTA_GATEWAY;
							// Копируем в буфер адрес шлюза
							::memcpy(cp, &sdl, sdl.sdl_len);
							// Сдвигаем буфер полезной нагрузки
							cp += ROUNDUP(sdl.sdl_len);
						}
					}
					// Маска подсети (RTA_NETMASK)
					struct sockaddr_in6 mask{0};
					// Устанавливаем длину структуры
					mask.sin6_len = sizeof(struct sockaddr_in6);
					// Устанавливаем семейство адресов
					mask.sin6_family = AF_INET6;
					// Если адрес назначения является default route
					if(IN6_IS_ADDR_UNSPECIFIED(&dst.sin6_addr)){
						/**
						 * Default route - mask 0 (already zeroed)
						 */
					// Если префикс задан
					} else if(route.prefix > 0) {
						// Текущий префикс
						uint32_t prefix = static_cast <uint32_t> (route.prefix);
						/**
						 * Проходим по байтам
						 */
						for(uint8_t i = 0; i < 16; ++i){
							// Если префикс больше либо равен 8
							if(prefix >= 8){
								// Устанавливаем байт маски подсети
								mask.sin6_addr.s6_addr[i] = 0xFF;
								// Уменьшаем префикс на 8
								prefix -= 8;
							// Если префикс меньше 8, но больше нуля
							} else if(prefix > 0) {
								// Устанавливаем байт маски подсети
								mask.sin6_addr.s6_addr[i] = static_cast <uint8_t> (0xFF << (8 - prefix));
								// Обнуляем префикс
								prefix = 0;
							// Зануляем байт маски подсети
							} else mask.sin6_addr.s6_addr[i] = 0;
						}
					// Если префикс сети не установлен
					} else {
						// Устанавливаем маску соответствующую префиксу /128
						::memset(&mask.sin6_addr, 0xFF, 16);
						// Устанавливаем флаг хостового маршрута
						rtm->rtm_flags |= RTF_HOST;
					}
					// Копируем маску подсети в буфер
					::memcpy(cp, &mask, mask.sin6_len);
					// Сдвигаем буфер полезной нагрузки
					cp += ROUNDUP(mask.sin6_len);
					// Полный размер сообщения
					rtm->rtm_msglen = (cp - buffer);
					// Записываем маршрут в систему
					if(!(result = (::write(sock, buffer, rtm->rtm_msglen) > 0))){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
					// Закрываем
					::close(sock);
				} break;
				// Во всех остальных случаях
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Unsupported address family", __PRETTY_FUNCTION__, make_tuple(route.gateway->size), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unsupported address family", log_t::flag_t::CRITICAL);
					#endif
				} break;
			}
		// Если адрес шлюза не инициализирован
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Gateway address is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Gateway address is not initialized", log_t::flag_t::CRITICAL);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод удаления маршрута
 *
 * @param route объект маршрута для удаления
 * @return      результат удаления маршрута
 */
bool awh::eth::Gateway::remove(const route_t & route) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес шлюза инициализирован
		if(route.gateway != nullptr){
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
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Буфер данных для получения маршрутов
					vector <uint8_t> buffer(length, 0);
					// Извлекаем маршруты в буфер
					if(::sysctl(mib, 6, &buffer[0], &length, nullptr, 0) < 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Создаём сокет для удаления маршрутов
					net::socket_t sock = ::socket(PF_ROUTE, SOCK_RAW, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Получаем итератор следующего маршрута
					uint8_t * begin = &buffer[0];
					// Получаем конец всех маршрутов
					uint8_t * end = (begin + length);
					// Получаем маску подсети из переданного префикса
					const in_addr_t netmsk = ::gw::prefix2mask(route.prefix);
					// Индекс сетевого интерфейса для поиска (вычисляем один раз вне цикла)
					const uint32_t searchIfIndex = (!route.ifname.empty() ? ::if_nametoindex(route.ifname.c_str()) : 0);
					// Индекс текущего маршрута
					int32_t index = 0;
					/**
					 * Перебираем все маршруты
					 */
					while(begin < end){
						// Если оставшихся данных недостаточно для заголовка маршрута
						if(static_cast <size_t> (end - begin) < sizeof(struct rt_msghdr))
							// Выходим из цикла
							break;
						// Приводим к структуре маршрута
						struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
						// Если версия маршрута не совпадает
						if(rtm->rtm_version != RTM_VERSION)
							// Выходим из цикла
							break;
						// Если длина сообщения некорректна или выходит за границы буфера
						if((rtm->rtm_msglen == 0) || (static_cast <size_t> (end - begin) < rtm->rtm_msglen))
							// Выходим из цикла
							break;
						// Разбираем адреса сообщения маршрута
						const ::gw::addrs_t addrs = ::gw::parse(rtm);
						// Объект адреса шлюза маршрута
						struct sockaddr_in * gw = (((addrs.gw != nullptr) && (addrs.gw->sa_family == AF_INET)) ? reinterpret_cast <struct sockaddr_in *> (addrs.gw) : nullptr);
						// Объект адреса назначения маршрута
						struct sockaddr_in * dst = (((addrs.dst != nullptr) && (addrs.dst->sa_family == AF_INET)) ? reinterpret_cast <struct sockaddr_in *> (addrs.dst) : nullptr);
						// Объект маски подсети маршрута
						struct sockaddr_in * mask = (((addrs.mask != nullptr) && (addrs.mask->sa_family == AF_INET)) ? reinterpret_cast <struct sockaddr_in *> (addrs.mask) : nullptr);
						// Объект сетевого интерфейса маршрута
						struct sockaddr_dl * ifp = addrs.ifp;
						// Флаг совпадения маршрута
						bool match = true;
						/**
						 * РЕЖИМ 1:
						 * Удалить ЛЮБОЙ default route (dst_str == "0.0.0.0" или nullptr + dst=0.0.0.0)
						 */
						// Проверяем, является ли адрес назначения маршрута default route
						const bool isDefault = ((dst != nullptr) && (dst->sin_addr.s_addr == INADDR_ANY));
						// Если требуется удалить ЛЮБОЙ default route
						if(isDefault && (netmsk == 0) &&
						  (route.gateway != nullptr ? (awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address == 0) : true) &&
						  (route.destination != nullptr ? (awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address == 0) : true))
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
						// Если адрес назначения инициализирован
						if(route.destination != nullptr){
							// Если адрес назначения маршрута задан
							if(awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address > 0)
								// Устанавливаем флаг совпадения по адресу назначения маршрута
								match = ((dst != nullptr) && (dst->sin_addr.s_addr == awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address));
						// Инициализируем объект адреса назначения в маршруте
						} else const_cast <route_t &> (route).destination = make_unique <net::addr_net_ipv4_t> ();
						// Если адрес шлюза маршрута задан
						if(awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address > 0)
							// Устанавливаем флаг совпадения по адресу шлюза маршрута
							match = ((gw != nullptr) && (gw->sin_addr.s_addr == awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address));
						// Если имя сетевого интерфейса задано (и шлюз НЕ задан)
						else if(!route.ifname.empty())
							// Устанавливаем флаг совпадения по имени сетевого интерфейса маршрута
							match = ((ifp != nullptr) && (ifp->sdl_index == searchIfIndex));
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
							// Если исходное сообщение маршрута не помещается в буфер
							if(rtm->rtm_msglen > sizeof(buffer)){
								// Переходим к следующему маршруту
								begin += rtm->rtm_msglen;
								// Продолжаем перебор дальше
								continue;
							}
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
							// Устанавливаем адреса маршрута (только поддерживаемые)
							rtd->rtm_addrs = (rtm->rtm_addrs & (RTA_DST | RTA_GATEWAY | RTA_NETMASK | RTA_IFP));
							// Объект для чтения адресов из исходного маршрута
							struct sockaddr * src = reinterpret_cast <struct sockaddr *> (rtm + 1);
							// Если присутствуют адреса в маршруте
							if(rtm->rtm_addrs & RTA_DST){
								// Копируем адрес назначения маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							}
							// Если присутствует шлюз в маршруте
							if(rtm->rtm_addrs & RTA_GATEWAY){
								// Если адрес шлюза НЕ является ссылочным (AF_LINK)
								if(src->sa_family != AF_LINK){
									// Копируем адрес шлюза маршрута
									::memcpy(payload, src, src->sa_len);
									// Смещаем указатель полезной нагрузки
									payload += ROUNDUP(src->sa_len);
								// Иначе снимаем флаг шлюза
								} else rtd->rtm_addrs &= ~RTA_GATEWAY;
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							}
							// Если присутствует маска подсети в маршруте
							if(rtm->rtm_addrs & RTA_NETMASK){
								// Копируем маску подсети маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							}
							// Если присутствует маска клонирования в маршруте
							if(rtm->rtm_addrs & RTA_GENMASK)
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							// Если присутствует сетевой интерфейс в маршруте
							if(rtm->rtm_addrs & RTA_IFP){
								// Копируем сетевой интерфейс маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
							}
							// Устанавливаем длину сообщения маршрута
							rtd->rtm_msglen = (payload - buffer);
							// Записываем сообщение в сокет
							if(!(result = (::write(sock, buffer, rtd->rtm_msglen) > 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
							// Если результат успешный
							if(result)
								// Выходим из цикла
								break;
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
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Буфер данных для получения маршрутов
					vector <uint8_t> buffer(length, 0);
					// Извлекаем маршруты в буфер
					if(::sysctl(mib, 6, &buffer[0], &length, nullptr, 0) < 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Создаём сокет для удаления маршрутов
					net::socket_t sock = ::socket(PF_ROUTE, SOCK_RAW, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
					// Получаем итератор следующего маршрута
					uint8_t * begin = &buffer[0];
					// Получаем конец всех маршрутов
					uint8_t * end = (begin + length);
					// Получаем маску подсети из переданного префикса
					struct in6_addr netmsk{0};
					// Индекс сетевого интерфейса для поиска (вычисляем один раз вне цикла)
					const uint32_t searchIfIndex = (!route.ifname.empty() ? ::if_nametoindex(route.ifname.c_str()) : 0);
					// Если префикс задан
					if(route.prefix > 0){
						// Текущий префикс
						uint32_t prefix = static_cast <uint32_t> (route.prefix);
						/**
						 * Проходим по байтам
						 */
						for(uint8_t i = 0; i < 16; ++i){
							// Если префикс больше либо равен 8
							if(prefix >= 8){
								// Устанавливаем байт маски подсети
								netmsk.s6_addr[i] = 0xFF;
								// Уменьшаем префикс на 8
								prefix -= 8;
							// Если префикс меньше 8, но больше нуля
							} else if(prefix > 0) {
								// Устанавливаем байт маски подсети
								netmsk.s6_addr[i] = static_cast <uint8_t> (0xFF << (8 - prefix));
								// Обнуляем префикс
								prefix = 0;
							// Зануляем байт маски подсети
							} else netmsk.s6_addr[i] = 0;
						}
					}
					// Индекс текущего маршрута
					int32_t index = 0;
					/**
					 * Перебираем все маршруты
					 */
					while(begin < end){
						// Если оставшихся данных недостаточно для заголовка маршрута
						if(static_cast <size_t> (end - begin) < sizeof(struct rt_msghdr))
							// Выходим из цикла
							break;
						// Приводим к структуре маршрута
						struct rt_msghdr * rtm = reinterpret_cast <struct rt_msghdr *> (begin);
						// Если версия маршрута не совпадает
						if(rtm->rtm_version != RTM_VERSION)
							// Выходим из цикла
							break;
						// Если длина сообщения некорректна или выходит за границы буфера
						if((rtm->rtm_msglen == 0) || (static_cast <size_t> (end - begin) < rtm->rtm_msglen))
							// Выходим из цикла
							break;
						// Разбираем адреса сообщения маршрута
						const ::gw::addrs_t addrs = ::gw::parse(rtm);
						// Объект адреса шлюза маршрута
						struct sockaddr_in6 * gw = (((addrs.gw != nullptr) && (addrs.gw->sa_family == AF_INET6)) ? reinterpret_cast <struct sockaddr_in6 *> (addrs.gw) : nullptr);
						// Объект адреса назначения маршрута
						struct sockaddr_in6 * dst = (((addrs.dst != nullptr) && (addrs.dst->sa_family == AF_INET6)) ? reinterpret_cast <struct sockaddr_in6 *> (addrs.dst) : nullptr);
						// Объект маски подсети маршрута
						struct sockaddr_in6 * mask = (((addrs.mask != nullptr) && (addrs.mask->sa_family == AF_INET6)) ? reinterpret_cast <struct sockaddr_in6 *> (addrs.mask) : nullptr);
						// Объект сетевого интерфейса маршрута
						struct sockaddr_dl * ifp = addrs.ifp;
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
						if(isDefault && (route.prefix == 0) &&
						  (route.gateway != nullptr ? (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], zeroAddr, 16) == 0) : true) &&
						  (route.destination != nullptr ? (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], zeroAddr, 16) == 0) : true))
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
						// Если адрес назначения инициализирован
						if(route.destination != nullptr){
							// Если адрес назначения маршрута задан
							if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], zeroAddr, 16) != 0)
								// Устанавливаем флаг совпадения по адресу назначения маршрута
								match = match && ((dst != nullptr) && (::memcmp(&dst->sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.destination.get())->address[0], 16) == 0));
						// Инициализируем объект адреса назначения в маршруте
						} else const_cast <route_t &> (route).destination = make_unique <net::addr_net_ipv6_t> ();
						// Если адрес шлюза маршрута задан
						if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], zeroAddr, 16) != 0)
							// Устанавливаем флаг совпадения по адресу шлюза маршрута
							match = match && ((gw != nullptr) && (::memcmp(&gw->sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 16) == 0));
						// Если имя сетевого интерфейса задано
						else if(!route.ifname.empty())
							// Устанавливаем флаг совпадения по имени сетевого интерфейса маршрута
							match = match && ((ifp != nullptr) && (ifp->sdl_index == searchIfIndex));
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
							// Если исходное сообщение маршрута не помещается в буфер
							if(rtm->rtm_msglen > sizeof(buffer)){
								// Переходим к следующему маршруту
								begin += rtm->rtm_msglen;
								// Продолжаем перебор дальше
								continue;
							}
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
							// Устанавливаем адреса маршрута (только поддерживаемые)
							rtd->rtm_addrs = (rtm->rtm_addrs & (RTA_DST | RTA_GATEWAY | RTA_NETMASK | RTA_IFP));
							// Объект для чтения адресов из исходного маршрута
							struct sockaddr * src = reinterpret_cast <struct sockaddr *> (rtm + 1);
							// Если присутствуют адреса в маршруте
							if(rtm->rtm_addrs & RTA_DST){
								// Копируем адрес назначения маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							}
							// Если присутствует шлюз в маршруте
							if(rtm->rtm_addrs & RTA_GATEWAY){
								// Если адрес шлюза НЕ является ссылочным (AF_LINK)
								if(src->sa_family != AF_LINK){
									// Копируем адрес шлюза маршрута
									::memcpy(payload, src, src->sa_len);
									// Смещаем указатель полезной нагрузки
									payload += ROUNDUP(src->sa_len);
								// Иначе снимаем флаг шлюза
								} else rtd->rtm_addrs &= ~RTA_GATEWAY;
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							}
							// Если присутствует маска подсети в маршруте
							if(rtm->rtm_addrs & RTA_NETMASK){
								// Копируем маску подсети маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							}
							// Если присутствует маска клонирования в маршруте
							if(rtm->rtm_addrs & RTA_GENMASK)
								// Устанавливаем текущий адрес маршрута
								src = ::gw::advance(src);
							// Если присутствует сетевой интерфейс в маршруте
							if(rtm->rtm_addrs & RTA_IFP){
								// Копируем сетевой интерфейс маршрута
								::memcpy(payload, src, src->sa_len);
								// Смещаем указатель полезной нагрузки
								payload += ROUNDUP(src->sa_len);
							}
							// Устанавливаем длину сообщения маршрута
							rtd->rtm_msglen = (payload - buffer);
							// Записываем сообщение в сокет
							if(!(result = (::write(sock, buffer, rtd->rtm_msglen) > 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
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
							// Если результат успешный
							if(result)
								// Выходим из цикла
								break;
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
						// Записываем ошибку в лог
						this->_log->debug("Unsupported address family", __PRETTY_FUNCTION__, make_tuple(route.gateway->size), log_t::flag_t::CRITICAL);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unsupported address family", log_t::flag_t::CRITICAL);
					#endif
				} break;
			}
		// Если адрес шлюза не инициализирован
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Gateway address is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Gateway address is not initialized", log_t::flag_t::CRITICAL);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::eth::Gateway::Gateway(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::eth::Gateway::~Gateway() noexcept {}
