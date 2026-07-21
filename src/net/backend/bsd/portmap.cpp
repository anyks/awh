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
 * Стандартные заголовочные файлы
 */
#include <array>
#include <random>
#include <cerrno>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <shared_mutex>

/**
 * Системные заголовочные файлы
 */
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

/**
 * Заголовочные файлы работы с модулем MiniUPnP
 */
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnperrors.h>
#include <miniupnpc/upnpcommands.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/locker.hpp>
#include <net/eth/portmap.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Время жизни кеша обнаруженного IGD в миллисекундах
	 *
	 * @note Шлюз может перезагрузиться или сменить IP-адрес, поэтому кеш периодически обновляется
	 */
	constexpr uint64_t AWH_IGD_CACHE_TTL = 0xEA60;

	/**
	 * @brief Время жизни кеша шлюза по умолчанию в миллисекундах
	 *
	 * @note Маршрут по умолчанию может смениться при переключении сети, поэтому кеш периодически обновляется
	 */
	constexpr uint64_t AWH_GATEWAY_CACHE_TTL = 0xEA60;

	/**
	 * @brief Время жизни кеша публичного IP-адреса NAT-PMP в миллисекундах
	 *
	 * @note Внешний (публичный) IP-адрес может смениться у провайдера, поэтому кеш периодически обновляется
	 */
	constexpr uint64_t AWH_NATPMP_PUBLIC_CACHE_TTL = 0xEA60;

	/**
	 * @brief Флаг одноразовой инициализации мьютексов для кешей IGD и шлюза
	 *
	 */
	once_flag __awh_init_once__;

	/**
	 * @brief Режим безопасности работы потоков
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;

	/**
	 * Блокировка доступа к глобальному кешу IGD
	 */
	static lock_state_t <std::shared_mutex> __awh_igd_cache_mutex__;

	/**
	 * Блокировка доступа к глобальному кешу шлюза
	 */
	static lock_state_t <std::shared_mutex> __awh_gateway_cache_mutex__;

	/**
	 * Блокировка доступа к глобальному кешу публичного IP-адреса NAT-PMP
	 */
	static lock_state_t <std::shared_mutex> __awh_natpmp_public_cache_mutex__;
};

/**
 * @brief Внутренние служебные объекты модуля проброса портов
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Структура кеша обнаруженного IGD-шлюза UPnP
	 *
	 * @note Храним только владеющие строки (std::string), а не UPNPUrls,
	 *       чтобы не тащить ручное освобождение через FreeUPNPUrls в статический кеш
	 */
	struct IgdCache {
		// Абсолютное время истечения кеша в миллисекундах (0 - кеш пустой)
		uint64_t expire;
		// URL управления IGD
		string controlURL;
		// Тип сервиса IGD
		string serviceType;
		// Внутренний IP-адрес клиента в сторону IGD
		string internalAddress;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit IgdCache() noexcept :
		 expire(0), controlURL{""},
		 serviceType{""}, internalAddress{""} {}
	} __awh_igd_cache__;

	/**
	 * @brief Функция получения параметров IGD-шлюза UPnP (из кеша или через обнаружение)
	 *
	 * @param out    объект для записи параметров найденного IGD-шлюза
	 * @param status результат выполнения операции UPnP (1 - успех, 0 - шлюз не обнаружен, иначе код ошибки UPnP)
	 * @param now    текущая метка времени в миллисекундах
	 * @return       результат получения параметров IGD-шлюза
	 */
	static bool resolveIGD(IgdCache & out, int32_t & status, const uint64_t now) noexcept {
		/**
		 * Быстрый путь: читаем валидный кеш под разделяемой блокировкой
		 */
		{
			// Блокируем доступ к глобальному кешу IGD на чтение
			const locker_t <std::shared_mutex> lock(::__awh_igd_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Если кеш ещё актуален
			if((::__awh_igd_cache__.expire > now) && !::__awh_igd_cache__.controlURL.empty()){
				// Копируем параметры IGD из кеша (копируются только строки - это дёшево)
				out = ::__awh_igd_cache__;
				// Устанавливаем признак успеха
				status = 1;
				// Выходим
				return true;
			}
		}
		/**
		 * Медленный путь: выполняем обнаружение БЕЗ удержания блокировки (это занимает несколько секунд)
		 */
		// Ищем устройства UPnP в локальной сети (3 секунды таймаут)
		UPNPDev * devlist = ::upnpDiscover(3000, nullptr, nullptr, 0, 0, 2, nullptr);
		// Если устройства не найдены
		if(devlist == nullptr){
			// Устанавливаем признак того, что шлюз не обнаружен
			status = 0;
			// Выходим
			return false;
		}
		// Действующий шлюз IGD
		UPNPUrls urls = {0};
		// Структура данных IGD
		IGDdatas data = {0};
		// Буфер для хранения внутреннего IP-адреса
		vector <char> internal(64, 0);
		// Получаем действующий шлюз IGD
		status = ::UPNP_GetValidIGD(devlist, &urls, &data, &internal[0], internal.size(), nullptr, 0);
		// Освобождаем память списка устройств UPnP
		::freeUPNPDevlist(devlist);
		// Если не удалось получить действующий шлюз IGD
		if(status != 1){
			// Освобождаем память URL-ов UPnP
			::FreeUPNPUrls(&urls);
			// Выходим
			return false;
		}
		// Снимаем только нужные строки из UPNPUrls/IGDdatas
		out.controlURL      = (urls.controlURL != nullptr ? urls.controlURL : "");
		out.serviceType     = data.first.servicetype;
		out.internalAddress = &internal[0];
		// Устанавливаем время истечения кеша
		out.expire          = (now + AWH_IGD_CACHE_TTL);
		// Освобождаем память URL-ов UPnP сразу - в кеше остаются только std::string
		::FreeUPNPUrls(&urls);
		/**
		 * Публикуем результат в кеш под эксклюзивной блокировкой
		 */
		{
			// Блокируем доступ к глобальному кешу IGD на запись
			const locker_t <std::shared_mutex> lock(::__awh_igd_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Сохраняем параметры IGD в кеш
			::__awh_igd_cache__ = out;
		}
		// Все удачно
		return true;
	}

	/**
	 * @brief Структура кеша адреса шлюза по умолчанию
	 *
	 */
	struct GatewayCache {
		// Флаг наличия закешированного IPv4-шлюза
		bool has4;
		// Флаг наличия закешированного IPv6-шлюза
		bool has6;
		// Абсолютное время истечения кеша IPv4-шлюза в миллисекундах
		uint64_t expire4;
		// Абсолютное время истечения кеша IPv6-шлюза в миллисекундах
		uint64_t expire6;
		// IPv4-адрес шлюза по умолчанию
		uint32_t address4;
		// IPv6-адрес шлюза по умолчанию
		array <uint8_t, 16> address6;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit GatewayCache() noexcept :
		 has4(false), has6(false),
		 expire4(0), expire6(0),
		 address4(0), address6{0} {}
	} __awh_gateway_cache__;

	/**
	 * @brief Функция получения адреса шлюза по умолчанию (из кеша или через таблицу маршрутизации)
	 *
	 * @param gateway объект работы с маршрутами
	 * @param route   объект маршрута (адрес шлюза должен быть предварительно инициализирован под нужное семейство)
	 * @param family  семейство IP-адресов (AF_INET или AF_INET6)
	 * @param now     текущая метка времени в миллисекундах
	 * @return        результат получения адреса шлюза
	 */
	static bool resolveGateway(const eth::gateway_t & gateway, eth::gateway_t::route_t & route, const int32_t family, const uint64_t now) noexcept {
		/**
		 * Быстрый путь: читаем валидный кеш под разделяемой блокировкой
		 */
		{
			// Блокируем доступ к глобальному кешу шлюза на чтение
			const locker_t <std::shared_mutex> lock(::__awh_gateway_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Если закеширован актуальный IPv4-шлюз
			if((family == AF_INET) && ::__awh_gateway_cache__.has4 && (::__awh_gateway_cache__.expire4 > now)){
				// Устанавливаем закешированный IPv4-адрес шлюза в маршрут
				awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address = ::__awh_gateway_cache__.address4;
				// Выходим
				return true;
			// Если закеширован актуальный IPv6-шлюз
			} else if((family == AF_INET6) && ::__awh_gateway_cache__.has6 && (::__awh_gateway_cache__.expire6 > now)) {
				// Устанавливаем закешированный IPv6-адрес шлюза в маршрут
				awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address = ::__awh_gateway_cache__.address6;
				// Выходим
				return true;
			}
		}
		/**
		 * Медленный путь: запрашиваем таблицу маршрутизации БЕЗ удержания блокировки
		 */
		if(!gateway.get(route))
			// Если маршрут не получен, выходим
			return false;
		/**
		 * Публикуем результат в кеш под эксклюзивной блокировкой
		 */
		{
			// Блокируем доступ к глобальному кешу шлюза на запись
			const locker_t <std::shared_mutex> lock(::__awh_gateway_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если адрес является IPv4
			if(family == AF_INET){
				// Сохраняем IPv4-адрес шлюза в кеш
				::__awh_gateway_cache__.address4 = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
				// Устанавливаем флаг наличия закешированного IPv4-шлюза
				::__awh_gateway_cache__.has4 = true;
				// Устанавливаем время истечения кеша IPv4-шлюза
				::__awh_gateway_cache__.expire4 = (now + AWH_GATEWAY_CACHE_TTL);
			// Если адрес является IPv6
			} else if(family == AF_INET6) {
				// Сохраняем IPv6-адрес шлюза в кеш
				::__awh_gateway_cache__.address6 = awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address;
				// Устанавливаем флаг наличия закешированного IPv6-шлюза
				::__awh_gateway_cache__.has6 = true;
				// Устанавливаем время истечения кеша IPv6-шлюза
				::__awh_gateway_cache__.expire6 = (now + AWH_GATEWAY_CACHE_TTL);
			}
		}
		// Все удачно
		return true;
	}

	/**
	 * @brief Структура кеша публичного IPv4-адреса NAT-PMP
	 *
	 * @note NAT-PMP определяет только IPv4-адрес публичной точки доступа (RFC 6886)
	 */
	struct NatPmpPublicCache {
		// Флаг наличия закешированного публичного IPv4-адреса
		bool has;
		// Абсолютное время истечения кеша в миллисекундах
		uint64_t expire;
		// Публичный IPv4-адрес (сетевой порядок байт)
		uint32_t address;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit NatPmpPublicCache() noexcept : has(false), expire(0), address(0) {}
	} __awh_natpmp_public_cache__;
};

/**
 * @brief Инкапсулируем статические прототипы функций в пространство имён работы с сокетами
 *
 */
namespace options {
	/**
	 * @brief Функция разрешения повторного использования адреса сокета
	 *
	 * @param sock сетевой сокет
	 * @param log  объект работы с логами
	 * @return     результат установки опции
	 */
	static bool reuseAddress(const awh::net::socket_t sock, const awh::log_t * log) noexcept {
		// Флаги установки опции
		int32_t flags = 1;
		// Переменная результата
		bool result = false;
		// Разрешаем повторно использовать тот же сокет после отключения
		if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags))))){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock), awh::log_t::flag_t::WARNING, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
			#endif
		}
		// Возвращаем результат
		return result;
	}
	/**
	 * @brief Функция установки таймаута сокета
	 *
	 * @param sock  сетевой сокет
	 * @param event событие сокета
	 * @param msec  время таймаута в миллисекундах
	 * @return      результат установки таймаута
	 */
	static bool timeout(const awh::net::socket_t sock, const awh::net::socket_event_t event, const uint32_t msec, const awh::log_t * log) noexcept {
		// Переменная результата
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
			case static_cast <uint8_t> (awh::net::socket_event_t::READ): {
				// Выполняем установку таймаута на чтение данных из сокета
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, static_cast <uint16_t> (event), msec), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log->print("%s", awh::log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			} break;
			// Если необходимо установить таймаут на запись
			case static_cast <uint8_t> (awh::net::socket_event_t::WRITE): {
				// Выполняем установку таймаута на запись данных в сокет
				if(!(result = !static_cast <bool> (::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sock, static_cast <uint16_t> (event), msec), awh::log_t::flag_t::CRITICAL, ::strerror(errno));
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
		// Все удачно
		return result;
	}
};

/**
 * @brief Внутренние служебные функции, использующие пространство имён работы с сокетами
 *
 */
namespace {
	/**
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Функция получения публичного IPv4-адреса NAT-PMP (из кеша или запросом к шлюзу)
	 *
	 * @param out    переменная для записи публичного IPv4-адреса (сетевой порядок байт)
	 * @param server параметры подключения к шлюзу
	 * @param family семейство IP-адресов (AF_INET или AF_INET6)
	 * @param now    текущая метка времени в миллисекундах
	 * @param log    объект работы с логами
	 * @return       результат получения публичного IPv4-адреса
	 */
	static bool resolveNatPmpPublicIP(uint32_t & out, const struct sockaddr_storage & server, const int32_t family, const uint64_t now, const log_t * log) noexcept {
		/**
		 * Быстрый путь: читаем валидный кеш под разделяемой блокировкой
		 */
		{
			// Блокируем доступ к глобальному кешу публичного IP на чтение
			const locker_t <std::shared_mutex> lock(::__awh_natpmp_public_cache_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Если кеш ещё актуален
			if(::__awh_natpmp_public_cache__.has && (::__awh_natpmp_public_cache__.expire > now)){
				// Возвращаем закешированный публичный IPv4-адрес
				out = ::__awh_natpmp_public_cache__.address;
				// Выходим
				return true;
			}
		}
		/**
		 * Медленный путь: запрашиваем публичный адрес у шлюза БЕЗ удержания блокировки
		 */
		// Выполняем создание UDP сокета
		net::socket_t sock = ::socket(family, SOCK_DGRAM, 0);
		// Если сокет не создан
		if(sock == net::invalid_socket_t)
			// Выходим
			return false;
		// Разрешаем повторное использование адреса сокета
		if(!::options::reuseAddress(sock, log)){
			// Закрываем сокет
			::close(sock);
			// Выходим
			return false;
		}
		// Размер структуры адреса шлюза
		const socklen_t serverLen = ((family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in));
		// Устанавливаем таймаут на запись сокета (5 секунд)
		if(!::options::timeout(sock, awh::net::socket_event_t::WRITE, 5000, log)){
			// Закрываем сокет
			::close(sock);
			// Выходим
			return false;
		}
		// Запрос публичного адреса NAT-PMP (версия=0, опкод=0)
		uint8_t request[2] = {0, 0};
		// Отправляем запрос публичного адреса
		if(::sendto(sock, reinterpret_cast <char *> (request), 2, 0, reinterpret_cast <const struct sockaddr *> (&server), serverLen) != 2){
			// Закрываем сокет
			::close(sock);
			// Выходим
			return false;
		}
		// Устанавливаем таймаут на чтение из сокета (5 секунд)
		if(!::options::timeout(sock, awh::net::socket_event_t::READ, 5000, log)){
			// Закрываем сокет
			::close(sock);
			// Выходим
			return false;
		}
		// Параметры получения ответа от шлюза
		struct sockaddr_storage from{};
		// Размер структуры адреса отправителя
		socklen_t fromLen = sizeof(from);
		// Буфер для приёма ответа
		char buffer[32];
		// Получаем ответ от шлюза
		const ssize_t bytes = ::recvfrom(sock, buffer, sizeof(buffer), 0, reinterpret_cast <struct sockaddr *> (&from), &fromLen);
		// Закрываем сокет
		::close(sock);
		// Если получен некорректный ответ (опкод 128 = ответ на запрос публичного адреса, код результата = 0)
		if((bytes < 12) || (static_cast <uint8_t> (buffer[1]) != 128) || (ntohs(* reinterpret_cast <const uint16_t *> (buffer + 2)) != 0))
			// Выходим
			return false;
		// Извлекаем публичный IPv4-адрес (offset 8, сетевой порядок байт)
		::memcpy(&out, buffer + 8, 4);
		/**
		 * Публикуем результат в кеш под эксклюзивной блокировкой
		 */
		{
			// Блокируем доступ к глобальному кешу публичного IP на запись
			const locker_t <std::shared_mutex> lock(::__awh_natpmp_public_cache_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Сохраняем публичный IPv4-адрес в кеш
			::__awh_natpmp_public_cache__.address = out;
			// Устанавливаем флаг наличия закешированного публичного IPv4-адреса
			::__awh_natpmp_public_cache__.has = true;
			// Устанавливаем время истечения кеша
			::__awh_natpmp_public_cache__.expire = (now + AWH_NATPMP_PUBLIC_CACHE_TTL);
		}
		// Все удачно
		return true;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::eth::Port_Mapping::Forwarding::Forwarding() noexcept :
 type(type_t::NONE), proto(proto_t::NONE), lifeTime(0),
 internalPort(0), externalPort(0), description{0},
 internalAddress{nullptr}, externalAddress{nullptr} {}

/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::eth::Port_Mapping::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Активируем работу мьютекса блокировки потока при работе с глобальным кешем IGD
	::__awh_igd_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с глобальным кешем шлюза
	::__awh_gateway_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с глобальным кешем публичного IP NAT-PMP
	::__awh_natpmp_public_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод получения списка проброшенных портов на маршрутизаторе
 *
 * @return список параметров проброшенных портов на маршрутизаторе
 */
vector <awh::eth::Port_Mapping::fwd_t> awh::eth::Port_Mapping::mappings() const noexcept {
	// Переменная результата
	vector <fwd_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Параметры обнаруженного IGD-шлюза
		IgdCache igd{};
		// Результат выполнения операции UPnP
		int32_t status = 0;
		// Получаем параметры IGD-шлюза (из кеша или через обнаружение)
		if(!::resolveIGD(igd, status, this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS))){
			// Если шлюз обнаружен, но является недействительным
			if(status != 0){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, ::strupnperror(status));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::WARNING, ::strupnperror(status));
				#endif
			}
			// Возвращаем пустой результат
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
				igd.controlURL.c_str(), igd.serviceType.c_str(), std::to_string(index++).c_str(),
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
			result.back().lifeTime = this->_fmk->atoi <uint32_t> (duration, ::strlen(duration));
			// Устанавливаем внутренний порт в результирующую запись
			result.back().internalPort = this->_fmk->atoi <uint16_t> (internalPort, ::strlen(internalPort));
			// Устанавливаем внешний порт в результирующую запись
			result.back().externalPort = this->_fmk->atoi <uint16_t> (externalPort, ::strlen(externalPort));
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
 * @brief Метод установки/удаления проброса портов на маршрутизаторе
 *
 * @param fwd  объект параметров проброса порта (при успехе обновляется назначенным внешним портом)
 * @param mode режим включения/выключения проброса порта
 * @return     результат выполнения установки
 */
bool awh::eth::Port_Mapping::mapping(fwd_t & fwd, const event::mode_t mode) const noexcept {
	// Переменная результата
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
				// Семейство IP-адресов
				int32_t family = AF_UNSPEC;
				// Если указан внутренний IP-адрес
				if(fwd.internalAddress != nullptr){
					/**
					 * Определяем тип адреса
					 */
					switch(fwd.internalAddress->size){
						// Если адрес является IPv4
						case 4: family = AF_INET; break;
						// Если адрес является IPv6
						case 16: family = AF_INET6; break;
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC){
					// Если указан внешний IP-адрес
					if(fwd.externalAddress != nullptr){
						/**
						 * Определяем тип адреса
						 */
						switch(fwd.externalAddress->size){
							// Если адрес является IPv4
							case 4: family = AF_INET; break;
							// Если адрес является IPv6
							case 16: family = AF_INET6; break;
						}
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC)
					// Устанавливаем семейство адресов IPv4 по умолчанию
					family = AF_INET;
				// Структура маршрута
				gateway_t::route_t route{};
				/**
				 * Определяем семейство IP-адресов
				 */
				switch(family){
					// Если адрес является IPv4
					case AF_INET:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv4_t> ();
					break;
					// Если адрес является IPv6
					case AF_INET6:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv6_t> ();
					break;
				}
				// Если получаем маршрут для указанного адреса (из кеша или через таблицу маршрутизации)
				if(::resolveGateway(this->_gateway, route, family, this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS))){
					// Выполняем создание UDP сокета
					net::socket_t sock = ::socket(family, SOCK_DGRAM, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
									static_cast <uint16_t> (mode)
								),
								log_t::flag_t::CRITICAL, ::strerror(errno)
							);
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
					// Разрешаем повторное использование адреса сокета
					if(!::options::reuseAddress(sock, this->_log)){
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return result;
					}
					// PCP MAP request (RFC 6887)
					uint8_t request[60] = {0};
					// Устанавливаем значение версии
					request[0] = 2;
					// Устанавливаем опкод MAP (Client Request)
					request[1] = 1;
					// Размер объекта подключения
					socklen_t size = 0;
					// Параметры подключения к шлюзу
					struct sockaddr_storage addr{0};
					/**
					 * Определяем семейство IP-адресов
					 */
					switch(family){
						// Если адрес является IPv4
						case AF_INET: {
							/**
							 * Client IP Address (Header offset 8) -> ::ffff:192.168.x.x
							 * IPv4-mapped IPv6 address
							 */
							request[18] = 0xFF;
							request[19] = 0xFF;
							// Объект адреса шлюза
							struct sockaddr_in gw = {0};
							// Целевой адрес для "подключения" (чтобы узнать свой IP)
							struct sockaddr_in dst = {0};
							// Устанавливаем семейство адресов IPv4
							gw.sin_family = family;
							// Устанавливаем порт MDNS
							gw.sin_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							gw.sin_addr.s_addr = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
							// Выполняем "подключение" сокета к шлюзу
							if(::connect(sock, reinterpret_cast <struct sockaddr *> (&gw), sizeof(gw)) == 0){
								// Получаем локальный адрес сокета
								socklen_t length = sizeof(dst);
								// Извлекаем локальный адрес сокета
								::getsockname(sock, reinterpret_cast <struct sockaddr *> (&dst), &length);
								// Добавляем локальный IP-адрес клиента
								::memcpy(request + 20, &dst.sin_addr, 4);
							}
							// Закрываем сокет
							::close(sock);
							// Если не удалось определить локальный IP-адрес
							if(dst.sin_addr.s_addr == 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"Failed to determine local IP for PCP",
										__PRETTY_FUNCTION__,
										make_tuple(
											fwd.lifeTime,
											fwd.description,
											fwd.internalPort,
											fwd.externalPort,
											static_cast <uint16_t> (fwd.type),
											static_cast <uint16_t> (fwd.proto),
											static_cast <uint16_t> (mode)
										),
										log_t::flag_t::CRITICAL
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Failed to determine local IP for PCP", log_t::flag_t::CRITICAL);
								#endif
								// Возвращаем результат
								return result;
							}
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in);
							// Копируем адрес шлюза
							::memcpy(&addr, &gw, size);
						} break;
						// Если адрес является IPv6
						case AF_INET6: {
							// Объект адреса шлюза
							struct sockaddr_in6 gw = {0};
							// Целевой адрес для "подключения" (чтобы узнать свой IP)
							struct sockaddr_in6 dst = {0};
							// Устанавливаем семейство адресов IPv6
							gw.sin6_family = family;
							// Устанавливаем порт MDNS
							gw.sin6_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							::memcpy(&gw.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 16);
							// Выполняем "подключение" сокета к шлюзу
							if(::connect(sock, reinterpret_cast <struct sockaddr *> (&gw), sizeof(gw)) == 0){
								// Получаем локальный адрес сокета
								socklen_t length = sizeof(dst);
								// Извлекаем локальный адрес сокета
								::getsockname(sock, reinterpret_cast <struct sockaddr *> (&dst), &length);
								// Добавляем локальный IP-адрес клиента
								::memcpy(request + 8, &dst.sin6_addr, 16);
							}
							// Закрываем сокет
							::close(sock);
							// Если не удалось определить локальный IP-адрес
							if(IN6_IS_ADDR_UNSPECIFIED(&dst.sin6_addr)){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug(
										"Failed to determine local IP for PCP",
										__PRETTY_FUNCTION__,
										make_tuple(
											fwd.lifeTime,
											fwd.description,
											fwd.internalPort,
											fwd.externalPort,
											static_cast <uint16_t> (fwd.type),
											static_cast <uint16_t> (fwd.proto),
											static_cast <uint16_t> (mode)
										),
										log_t::flag_t::CRITICAL
									);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Failed to determine local IP for PCP", log_t::flag_t::CRITICAL);
								#endif
								// Возвращаем результат
								return result;
							}
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in6);
							// Копируем адрес шлюза
							::memcpy(&addr, &gw, size);
						} break;
					}
					// Выполняем создание UDP сокета
					sock = ::socket(family, SOCK_DGRAM, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
									static_cast <uint16_t> (mode)
								),
								log_t::flag_t::CRITICAL, ::strerror(errno)
							);
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
					// Разрешаем повторное использование адреса сокета
					if(!::options::reuseAddress(sock, this->_log)){
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return result;
					}
					/**
					 * Определяем режим работы проброса порта
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо пробросить порт
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем время жизни (Header offset 4)
							(* reinterpret_cast <uint32_t *> (request + 4)) = htonl(fwd.lifeTime);
						break;
						// Если необходимо убрать проброшенный порт
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Устанавливаем время жизни (Header offset 4)
							(* reinterpret_cast <uint32_t *> (request + 4)) = htonl(0);
						break;
					}
					// Локальный буфер PCP-nonce (Mapping Nonce, 12 байт) для защиты от подделки ответа
					uint8_t nonce[12] = {0};
					// Если описание записи не содержит достаточного количества байт для nonce
					if(::strlen(fwd.description) < 12){
						// Криптостойкий источник случайных чисел
						std::random_device randev;
						/**
						 * Перебираем байты nonce
						 */
						for(uint8_t i = 0; i < 12; ++i)
							// Заполняем nonce случайными байтами и копируем в поле запроса
							nonce[i] = request[24 + i] = static_cast <uint8_t> (randev());
					// Если описание записи содержит достаточно байт
					} else {
						/**
						 * Перебираем байты nonce
						 */
						for(uint8_t i = 0; i < 12; ++i)
							// Используем описание записи как nonce и копируем в поле запроса
							nonce[i] = request[24 + i] = static_cast <uint8_t> (fwd.description[i]);
					}
					/**
					 * Определяем протокол проброса порта
					 */
					switch(static_cast <uint8_t> (fwd.proto)){
						// Если протокол проброса порта является TCP
						case static_cast <uint8_t> (proto_t::TCP):
							// Устанавливаем протокол TCP в запросе
							request[36] = 6; // протокол: 17 = UDP, 6 = TCP
						break;
						// Если протокол проброса порта является UDP
						case static_cast <uint8_t> (proto_t::UDP):
						default:
							// Устанавливаем протокол UDP в запросе
							request[36] = 17; // протокол: 17 = UDP, 6 = TCP
						break;
					}
					// Устанавливаем внутренний порт (Offset 40)
					(* reinterpret_cast <uint16_t *> (request + 40)) = htons(fwd.internalPort);
					// Устанавливаем внешний порт (Offset 42) (0 = авто)
					(* reinterpret_cast <uint16_t *> (request + 42)) = htons(fwd.externalPort);
					// Устанавливаем таймаут на запись сокета (5 секунд)
					if(!::options::timeout(sock, awh::net::socket_event_t::WRITE, 5000, this->_log)){
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return result;
					}
					// Отправляем запрос на проброс порта
					ssize_t bytes = ::sendto(sock, reinterpret_cast <char *> (request), 60, 0, reinterpret_cast <struct sockaddr *> (&addr), size);
					// Если не удалось отправить запрос
					if(bytes != 60){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
									static_cast <uint16_t> (mode)
								),
								log_t::flag_t::CRITICAL, ::strerror(errno)
							);
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
					// Устанавливаем таймаут на чтение из сокета (5 секунд)
					if(!::options::timeout(sock, awh::net::socket_event_t::READ, 5000, this->_log)){
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return result;
					}
					// Буфер для приёма ответа
					char buffer[1024];
					// Получаем ответ от шлюза
					bytes = ::recvfrom(sock, buffer, sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&addr), &size);
					// Закрываем сокет
					::close(sock);
					// Если не удалось отправить запрос
					if(bytes <= 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
									static_cast <uint16_t> (mode)
								),
								log_t::flag_t::CRITICAL, ::strerror(errno)
							);
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
					// Устанавливаем терминальный нулевой символ в буфере
					buffer[bytes] = '\0';
					// Если получен ответ с корректным размером и опкодом (129 = MAP Opcode Response)
					if((bytes >= 60) && static_cast <uint8_t> (buffer[1]) == 129){
						// Получаем код результата
						const uint8_t code = buffer[3];
						// Если код результата не отрицательный
						if(code != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"PCP Error Code: %d",
									__PRETTY_FUNCTION__,
									make_tuple(
										fwd.lifeTime,
										fwd.description,
										fwd.internalPort,
										fwd.externalPort,
										static_cast <uint16_t> (fwd.type),
										static_cast <uint16_t> (fwd.proto),
										static_cast <uint16_t> (mode)
									),
									log_t::flag_t::CRITICAL, code
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("PCP Error Code: %d", log_t::flag_t::CRITICAL, code);
							#endif
							// Возвращаем результат
							return result;
						}
						/**
						 * Определяем режим работы проброса порта
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если необходимо пробросить порт
							case static_cast <uint8_t> (event::mode_t::ENABLED): {
								// Проверяем совпадение внутреннего порта (эхо в ответе, offset 40)
								if((result = (fwd.internalPort == ntohs(* reinterpret_cast <const uint16_t *> (buffer + 40))))){
									// Проверяем совпадение nonce для защиты от подделки ответа
									if(!(result = (::memcmp(buffer + 24, nonce, 12) == 0))){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug(
												"Response was forged by an attacker on PCP",
												__PRETTY_FUNCTION__,
												make_tuple(
													fwd.lifeTime,
													fwd.description,
													fwd.internalPort,
													fwd.externalPort,
													static_cast <uint16_t> (fwd.type),
													static_cast <uint16_t> (fwd.proto),
													static_cast <uint16_t> (mode)
												),
												log_t::flag_t::CRITICAL
											);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Response was forged by an attacker on PCP", log_t::flag_t::CRITICAL);
										#endif
									// Если ответ подлинный
									} else {
										// Сохраняем назначенный маршрутизатором внешний порт (Assigned External Port, offset 42)
										fwd.externalPort = ntohs(* reinterpret_cast <const uint16_t *> (buffer + 42));
										// Сохраняем назначенное маршрутизатором время жизни проброса порта (Lifetime, offset 4)
										fwd.lifeTime = ntohl(* reinterpret_cast <const uint32_t *> (buffer + 4));
										// Префикс IPv4-mapped IPv6 адреса (::ffff:0:0)
										static const uint8_t v4MappedPrefix[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};
										// Если назначенный внешний адрес (Assigned External IP, offset 44) является IPv4-mapped IPv6
										if(::memcmp(buffer + 44, v4MappedPrefix, 12) == 0){
											// Инициализируем объект внешнего IPv4-адреса
											fwd.externalAddress = make_unique <net::addr_net_ipv4_t> ();
											// Сохраняем назначенный внешний IPv4-адрес (последние 4 байта, сетевой порядок)
											::memcpy(&awh_cast <net::addr_net_ipv4_t *> (fwd.externalAddress.get())->address, buffer + 56, 4);
										// Если назначенный внешний адрес является IPv6
										} else {
											// Инициализируем объект внешнего IPv6-адреса
											fwd.externalAddress = make_unique <net::addr_net_ipv6_t> ();
											// Сохраняем назначенный внешний IPv6-адрес (16 байт, сетевой порядок)
											::memcpy(&awh_cast <net::addr_net_ipv6_t *> (fwd.externalAddress.get())->address[0], buffer + 44, 16);
										}
									}
								// Если установленный порт не соответствует
								} else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug(
											"Port PCP mapping failed",
											__PRETTY_FUNCTION__,
											make_tuple(
												fwd.lifeTime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Port PCP mapping failed", log_t::flag_t::CRITICAL);
									#endif
								}
							} break;
							// Если необходимо убрать проброшенный порт
							case static_cast <uint8_t> (event::mode_t::DISABLED): {
								// Проверяем совпадение внутреннего порта (эхо в ответе, offset 40)
								if((result = (fwd.internalPort == ntohs(* reinterpret_cast <const uint16_t *> (buffer + 40))))){
									// Проверяем совпадение nonce для защиты от подделки ответа
									if(!(result = (::memcmp(buffer + 24, nonce, 12) == 0))){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Записываем ошибку в лог
											this->_log->debug(
												"Response was forged by an attacker on PCP",
												__PRETTY_FUNCTION__,
												make_tuple(
													fwd.lifeTime,
													fwd.description,
													fwd.internalPort,
													fwd.externalPort,
													static_cast <uint16_t> (fwd.type),
													static_cast <uint16_t> (fwd.proto),
													static_cast <uint16_t> (mode)
												),
												log_t::flag_t::CRITICAL
											);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Записываем ошибку в лог
											this->_log->print("Response was forged by an attacker on PCP", log_t::flag_t::CRITICAL);
										#endif
									}
								// Если установленный порт не соответствует
								} else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug(
											"Port PCP unmapping failed",
											__PRETTY_FUNCTION__,
											make_tuple(
												fwd.lifeTime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Port PCP unmapping failed", log_t::flag_t::CRITICAL);
									#endif
								}
							} break;
						}
					}
				// Если маршрут не получен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"Gateway address could not be obtained",
							__PRETTY_FUNCTION__,
							make_tuple(
								fwd.lifeTime,
								fwd.description,
								fwd.internalPort,
								fwd.externalPort,
								static_cast <uint16_t> (fwd.type),
								static_cast <uint16_t> (fwd.proto),
								static_cast <uint16_t> (mode)
							),
							log_t::flag_t::CRITICAL
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Gateway address could not be obtained", log_t::flag_t::CRITICAL);
					#endif
				}
			} break;
			// Если тип проброса порта является UPNP
			case static_cast <uint8_t> (type_t::UPNP): {
				// Параметры обнаруженного IGD-шлюза
				IgdCache igd{};
				// Результат выполнения операции UPnP
				int32_t status = 0;
				// Получаем параметры IGD-шлюза (из кеша или через обнаружение)
				if(!::resolveIGD(igd, status, this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS))){
					// Если шлюз обнаружен, но является недействительным
					if(status != 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
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
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::WARNING, ::strupnperror(status));
						#endif
					}
					// Возвращаем пустой результат
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
							igd.controlURL.c_str(),
							// Устанавливаем тип сервиса
							igd.serviceType.c_str(),
							// Устанавливаем внешний порт
							externalPort,
							// Устанавливаем внутренний порт
							internalPort,
							// Устанавливаем внутренний IP-адрес
							igd.internalAddress.c_str(),
							// Устанавливаем описание проброса порта
							(::strlen(fwd.description) == 0 ? this->_fmk->format("%s (%s)", AWH_NAME, AWH_SHORT_NAME).c_str() : &fwd.description[0]),
							// Устанавливаем протокол порта
							(fwd.proto == proto_t::TCP ? "TCP" : "UDP"),
							// Устанавливаем внешний IP-адрес, если он указан
							&externalAddress[0],
							// Устанавливаем время жизни проброса порта
							(fwd.lifeTime == 0 ? "0" : std::to_string(fwd.lifeTime).c_str())
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
							igd.controlURL.c_str(),
							// Устанавливаем тип сервиса
							igd.serviceType.c_str(),
							// Устанавливаем внешний порт
							externalPort,
							// Устанавливаем протокол порта
							(fwd.proto == proto_t::TCP ? "TCP" : "UDP"),
							// Устанавливаем внешний IP-адрес, если он указан
							&externalAddress[0]
						);
					} break;
				}
				// Если возникла ошибка
				if(!(result = (status == UPNPCOMMAND_SUCCESS))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							make_tuple(
								fwd.lifeTime,
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
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strupnperror(status));
					#endif
				}
			} break;
			// Если тип проброса порта является NAT_PMP
			case static_cast <uint8_t> (type_t::NAT_PMP): {
				// Семейство IP-адресов
				int32_t family = AF_UNSPEC;
				// Если указан внутренний IP-адрес
				if(fwd.internalAddress != nullptr){
					/**
					 * Определяем тип адреса
					 */
					switch(fwd.internalAddress->size){
						// Если адрес является IPv4
						case 4: family = AF_INET; break;
						// Если адрес является IPv6
						case 16: family = AF_INET6; break;
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC){
					// Если указан внешний IP-адрес
					if(fwd.externalAddress != nullptr){
						/**
						 * Определяем тип адреса
						 */
						switch(fwd.externalAddress->size){
							// Если адрес является IPv4
							case 4: family = AF_INET; break;
							// Если адрес является IPv6
							case 16: family = AF_INET6; break;
						}
					}
				}
				// Если семейство адресов не определено
				if(family == AF_UNSPEC)
					// Устанавливаем семейство адресов IPv4 по умолчанию
					family = AF_INET;
				// Структура маршрута
				gateway_t::route_t route{};
				/**
				 * Определяем семейство IP-адресов
				 */
				switch(family){
					// Если адрес является IPv4
					case AF_INET:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv4_t> ();
					break;
					// Если адрес является IPv6
					case AF_INET6:
						// Инициализируем объект адреса шлюза в маршруте
						route.gateway = make_unique <net::addr_net_ipv6_t> ();
					break;
				}
				// Если получаем маршрут для указанного адреса (из кеша или через таблицу маршрутизации)
				if(::resolveGateway(this->_gateway, route, family, this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS))){
					// Выполняем создание UDP сокета
					net::socket_t sock = ::socket(family, SOCK_DGRAM, 0);
					// Если сокет не создан
					if(sock == net::invalid_socket_t){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
									static_cast <uint16_t> (mode)
								),
								log_t::flag_t::CRITICAL, ::strerror(errno)
							);
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
					// Разрешаем повторное использование адреса сокета
					if(!::options::reuseAddress(sock, this->_log)){
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return result;
					}
					// Размер объекта подключения
					socklen_t size = 0;
					// Параметры получения ответа от шлюза
					struct sockaddr_storage client{0};
					// Параметры подключения к шлюзу
					struct sockaddr_storage server{0};
					/**
					 * Определяем семейство IP-адресов
					 */
					switch(family){
						// Если адрес является IPv4
						case AF_INET: {
							// Объект адреса шлюза
							struct sockaddr_in gw = {0};
							// Устанавливаем семейство адресов IPv4
							gw.sin_family = family;
							// Устанавливаем порт MDNS
							gw.sin_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							gw.sin_addr.s_addr = awh_cast <net::addr_net_ipv4_t *> (route.gateway.get())->address;
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in);
							// Копируем адрес шлюза
							::memcpy(&server, &gw, size);
							// Копируем адрес клиента
							::memcpy(&client, &gw, size);
						} break;
						// Если адрес является IPv6
						case AF_INET6: {
							// Объект адреса шлюза
							struct sockaddr_in6 gw = {0};
							// Устанавливаем семейство адресов IPv6
							gw.sin6_family = family;
							// Устанавливаем порт MDNS
							gw.sin6_port = htons(5351);
							// Устанавливаем IP-адрес шлюза
							::memcpy(&gw.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (route.gateway.get())->address[0], 16);
							// Запоминаем размер структуры
							size = sizeof(struct sockaddr_in6);
							// Копируем адрес клиента
							::memcpy(&client, &gw, size);
							// Копируем адрес шлюза
							::memcpy(&server, &gw, size);
						} break;
					}
					// Буфер запроса NAT-PMP (RFC 6886)
					uint8_t request[12] = {0};
					// Буфер для приёма ответа
					char buffer[1024];
					// Количество переданных/принятых байт
					ssize_t bytes = 0;
					// Устанавливаем версию протокола NAT-PMP
					request[0] = 0;
					/**
					 * Определяем протокол проброса порта
					 */
					switch(static_cast <uint8_t> (fwd.proto)){
						// Если протокол проброса порта является TCP
						case static_cast <uint8_t> (proto_t::TCP):
							// Устанавливаем опкод = 2 (map TCP)
							request[1] = 2;
						break;
						// Если протокол проброса порта является UDP
						case static_cast <uint8_t> (proto_t::UDP):
						default:
							// Устанавливаем опкод = 1 (map UDP)
							request[1] = 1;
						break;
					}
					// Устанавливаем зарезервировано
					request[2] = 0;
					request[3] = 0;
					/**
					 * Определяем режим работы проброса порта
					 */
					switch(static_cast <uint8_t> (mode)){
						// Если необходимо пробросить порт
						case static_cast <uint8_t> (event::mode_t::ENABLED):
							// Устанавливаем время жизни (секунды)
							(* reinterpret_cast <uint32_t *> (request + 8)) = htonl(fwd.lifeTime);
						break;
						// Если необходимо убрать проброшенный порт
						case static_cast <uint8_t> (event::mode_t::DISABLED):
							// Устанавливаем время жизни (секунды)
							(* reinterpret_cast <uint32_t *> (request + 8)) = htonl(0);
						break;
					}
					// Устанавливаем внутренний порт
					(* reinterpret_cast <uint16_t *> (request + 4)) = htons(fwd.internalPort);
					// Устанавливаем внешний порт = 0 (авто)
					(* reinterpret_cast <uint16_t *> (request + 6)) = htons(fwd.externalPort);
					// Устанавливаем таймаут на запись сокета (5 секунд)
					if(!::options::timeout(sock, awh::net::socket_event_t::WRITE, 5000, this->_log)){
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return result;
					}
					// Отправляем запрос на проброс порта
					bytes = ::sendto(sock, reinterpret_cast <char *> (request), 12, 0, reinterpret_cast <struct sockaddr *> (&server), size);
					// Если не удалось отправить запрос
					if(bytes <= 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
									static_cast <uint16_t> (mode)
								),
								log_t::flag_t::CRITICAL, ::strerror(errno)
							);
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
					// Устанавливаем таймаут на чтение из сокета (5 секунд)
					if(!::options::timeout(sock, awh::net::socket_event_t::READ, 5000, this->_log)){
						// Закрываем сокет
						::close(sock);
						// Возвращаем результат
						return result;
					}
					// Получаем ответ от шлюза
					bytes = ::recvfrom(sock, buffer, sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&client), &size);
					// Закрываем сокет
					::close(sock);
					// Если не удалось отправить запрос
					if(bytes <= 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								make_tuple(
									fwd.lifeTime,
									fwd.description,
									fwd.internalPort,
									fwd.externalPort,
									static_cast <uint16_t> (fwd.type),
									static_cast <uint16_t> (fwd.proto),
									static_cast <uint16_t> (mode)
								),
								log_t::flag_t::CRITICAL, ::strerror(errno)
							);
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
					// Устанавливаем терминальный нулевой символ в буфере
					buffer[bytes] = '\0';
					// === 5. Парсим ответ ===
					if((bytes >= 16) && (static_cast <uint8_t> (buffer[1]) == (request[1] | 128))){
						// Успех (Result code = 0) проверяем в map_resp[2..3]
						const uint16_t code = ntohs(* reinterpret_cast <const uint16_t *> (buffer + 2));
						// Если возникла ошибка
						if(code != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"NAT-PMP Error Code: %d",
									__PRETTY_FUNCTION__,
									make_tuple(
										fwd.lifeTime,
										fwd.description,
										fwd.internalPort,
										fwd.externalPort,
										static_cast <uint16_t> (fwd.type),
										static_cast <uint16_t> (fwd.proto),
										static_cast <uint16_t> (mode)
									),
									log_t::flag_t::CRITICAL, code
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("NAT-PMP Error Code: %d", log_t::flag_t::CRITICAL, code);
							#endif
							// Возвращаем результат
							return result;
						}
						/**
						 * Определяем режим работы проброса порта
						 */
						switch(static_cast <uint8_t> (mode)){
							// Если необходимо пробросить порт
							case static_cast <uint8_t> (event::mode_t::ENABLED): {
								// Проверяем совпадение внутреннего порта (эхо в ответе, offset 8) - внешний порт может быть назначен автоматически
								if(!(result = (fwd.internalPort == ntohs(* reinterpret_cast <const uint16_t *> (buffer + 8))))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug(
											"Port NAT-PMP mapping failed",
											__PRETTY_FUNCTION__,
											make_tuple(
												fwd.lifeTime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Port NAT-PMP mapping failed", log_t::flag_t::CRITICAL);
									#endif
								// Если проброс порта выполнен успешно
								} else {
									// Сохраняем назначенный маршрутизатором внешний порт (Mapped External Port, offset 10)
									fwd.externalPort = ntohs(* reinterpret_cast <const uint16_t *> (buffer + 10));
									// Сохраняем назначенное маршрутизатором время жизни проброса порта (Lifetime, offset 12)
									fwd.lifeTime = ntohl(* reinterpret_cast <const uint32_t *> (buffer + 12));
									// Публичный IPv4-адрес NAT-PMP (сетевой порядок байт)
									uint32_t publicIp = 0;
									// Определяем публичный внешний IPv4-адрес (из кеша или запросом к шлюзу) и сохраняем его в результат
									if(::resolveNatPmpPublicIP(publicIp, server, family, this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS), this->_log) && (publicIp != 0)){
										// Инициализируем объект внешнего IPv4-адреса
										fwd.externalAddress = make_unique <net::addr_net_ipv4_t> ();
										// Сохраняем назначенный публичный внешний IPv4-адрес
										awh_cast <net::addr_net_ipv4_t *> (fwd.externalAddress.get())->address = publicIp;
									}
								}
							} break;
							// Если необходимо убрать проброшенный порт
							case static_cast <uint8_t> (event::mode_t::DISABLED): {
								// Проверяем совпадение внешнего порта
								if(!(result = (ntohs(* reinterpret_cast <const uint16_t *> (buffer + 10)) == 0))){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Записываем ошибку в лог
										this->_log->debug(
											"Port NAT-PMP unmapping failed",
											__PRETTY_FUNCTION__,
											make_tuple(
												fwd.lifeTime,
												fwd.description,
												fwd.internalPort,
												fwd.externalPort,
												static_cast <uint16_t> (fwd.type),
												static_cast <uint16_t> (fwd.proto),
												static_cast <uint16_t> (mode)
											),
											log_t::flag_t::CRITICAL
										);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Записываем ошибку в лог
										this->_log->print("Port NAT-PMP unmapping failed", log_t::flag_t::CRITICAL);
									#endif
								}
							} break;
						}
					}
				// Если маршрут не получен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(
							"Gateway address could not be obtained",
							__PRETTY_FUNCTION__,
							make_tuple(
								fwd.lifeTime,
								fwd.description,
								fwd.internalPort,
								fwd.externalPort,
								static_cast <uint16_t> (fwd.type),
								static_cast <uint16_t> (fwd.proto),
								static_cast <uint16_t> (mode)
							),
							log_t::flag_t::CRITICAL
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Gateway address could not be obtained", log_t::flag_t::CRITICAL);
					#endif
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
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(
					fwd.lifeTime,
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
awh::eth::Port_Mapping::Port_Mapping(const fmk_t * fmk, const log_t * log) noexcept :
 _gateway(fmk, log), _addr(fmk, log), _fmk(fmk), _log(log) {
	/**
	 * Выполняем одноразовую инициализацию мьютексов для кешей IGD и шлюза для всех экземпляров класса Port_Mapping
	 */
	std::call_once(::__awh_init_once__, [this]() noexcept {
		// Активируем работу мьютекса блокировки потока при работе с глобальным кэшем IGD
		::__awh_igd_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Активируем работу мьютекса блокировки потока при работе с глобальным кэшем шлюза
		::__awh_gateway_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
		// Активируем работу мьютекса блокировки потока при работе с глобальным кэшем публичного IP NAT-PMP
		::__awh_natpmp_public_cache_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	});
}
/**
 * @brief Деструктор
 *
 */
awh::eth::Port_Mapping::~Port_Mapping() noexcept {}
