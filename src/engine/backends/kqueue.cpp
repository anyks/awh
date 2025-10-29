/**
 * @file: kqueue.cpp
 * @date: 2025-10-27
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
 * Если максимальное количество опрашиваемых событий за одну итерацию (64, 128, 256, 512, 1024)
 */
#ifndef AWH_MAX_POLL_EVENTS_COUNT
	/**
	 * Устанавливаем максимальное количество опрашиваемых событий за одну итерацию (64)
	 */
	#define AWH_MAX_POLL_EVENTS_COUNT 0x40
#endif

/**
 * Стандартные модули
 */
#include <cerrno>
#include <atomic>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

/**
 * Подключаем системные заголовки
 */
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * Подключаем наши заголовочные файлы
 */
#include <sys/os.hpp>

/**
 * Подключаем заголовочный файл асинхронного движка ввода-вывода
 */
#include <engine/io.hpp>
#include <engine/fds.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Структура адреса
 *
 */
typedef struct Address {
	// Размер адреса
	uint16_t size;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Address() noexcept : size(0) {}
	/**
	 * @brief Деструктор
	 *
	 */
	virtual ~Address() noexcept = default;
} __attribute__((packed)) address_t;

/**
 * @brief Структура IPv4-адреса
 *
 */
typedef struct AddressIPv4 : public address_t {
	// IP-адрес
	uint32_t address;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit AddressIPv4() noexcept : address(0) {}
} __attribute__((packed)) address_ipv4_t;

/**
 * @brief Структура IPv6-адреса
 *
 */
typedef struct AddressIPv6 : public address_t {
	// IP-адрес
	uint8_t address[16];
	/**
	 * @brief Конструктор
	 *
	 */
	explicit AddressIPv6() noexcept : address{0} {}
} __attribute__((packed)) address_ipv6_t;

/**
 * @brief Структура MAC-адреса
 *
 */
typedef struct AddressMAC : public address_t {
	// MAC-адрес
	uint8_t address[6];
	/**
	 * @brief Конструктор
	 *
	 */
	explicit AddressMAC() noexcept : address{0} {}
} __attribute__((packed)) address_mac_t;

/**
 * @brief Структура сетевого адреса
 *
 */
typedef struct AddressNetwork : public address_t {
	// Префикс сети
	uint8_t prefix;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit AddressNetwork() noexcept : prefix(0) {}
} __attribute__((packed)) address_network_t;

/**
 * @brief Структура IPv4 сетевого адреса
 *
 */
typedef struct AddressNetworkIPv4 : public address_network_t {
	// IP-адрес сети
	uint32_t address;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit AddressNetworkIPv4() noexcept : address(0) {}
	/**
	 * @brief Деструктор
	 *
	 */
	virtual ~AddressNetworkIPv4() noexcept = default;
} __attribute__((packed)) address_network_ipv4_t;

/**
 * @brief Структура IPv6 сетевого адреса
 *
 */
typedef struct AddressNetworkIPv6 : public address_network_t {
	// IP-адрес сети
	uint8_t address[16];
	/**
	 * @brief Конструктор
	 *
	 */
	explicit AddressNetworkIPv6() noexcept : address{0} {}
} __attribute__((packed)) address_network_ipv6_t;

/**
 * @brief Структура адреса файловой системы
 *
 */
typedef struct AddressFilesystem : public address_t {
	// Путь к файлу, каталогу или сокету
	string address;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit AddressFilesystem() noexcept : address{""} {}
} address_fs_t;

/**
 * @brief Структура хоста
 *
 */
typedef struct Host {
	// Сокет хоста
	int32_t fd;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Host() noexcept : fd(-1) {}
	/**
	 * @brief Деструктор
	 *
	 */
	virtual ~Host() noexcept = default;
} __attribute__((packed)) host_t;

/**
 * @brief Структура IP-хоста
 *
 */
typedef struct HostIP : public host_t {
	// IP-адрес хоста
	address_t ip;
	// Порт хоста
	uint32_t port;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit HostIP() noexcept : port(0) {}
} __attribute__((packed)) host_ip_t;

/**
 * @brief Структура UNIX-хоста
 *
 */
typedef struct HostUDC : public host_t {
	// Путь к сокету
	address_fs_t path;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit HostUDC() noexcept {}
} host_udc_t;

/**
 * @brief Структура состояния события
 *
 */
typedef struct State {
	bool onlyIPv6;                   // Флаг активации только IPv6
	awh::event::mode_t mode;         // Флаг режима события
	awh::event::node_t node;         // Флаг узла события
	awh::event::type_t type;         // Флаг типа события
	awh::event::family_t family;     // Флаг семейства события
	awh::event::status_t status;     // Флаг статуса события
	awh::event::address_t address;   // Флаг адреса события
	awh::event::protocol_t protocol; // Флаг протокола события
	/**
	 * @brief Конструктор
	 *
	 */
	explicit State() noexcept :
	 onlyIPv6(false),
	 mode(awh::event::mode_t::NONE),
	 node(awh::event::node_t::NONE),
	 type(awh::event::type_t::NONE),
	 family(awh::event::family_t::NONE),
	 status(awh::event::status_t::NONE),
	 address(awh::event::address_t::NONE),
	 protocol(awh::event::protocol_t::NONE) {}
} __attribute__((packed)) state_t;

/**
 * @brief Структура обратных вызовов события
 *
 */
typedef struct Callbacks {
	// Обратный вызов при ошибке события
	awh::engine_t::errorCallback error;
	// Обратный вызов при изменении статуса события
	awh::engine_t::statusCallback status;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Callbacks() noexcept : error(nullptr), status(nullptr) {}
	/**
	 * @brief Конструктор
	 *
	 */
	virtual ~Callbacks() = default;
} callbacks_t;

/**
 * @brief Структура обратных вызовов сервера
 *
 */
typedef struct CallbacksServer : public callbacks_t {
	// Обратный вызов при принятии события
	awh::engine_t::acceptCallback accept;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit CallbacksServer() noexcept : accept(nullptr) {}
} callbacks_server_t;

/**
 * @brief Структура обратных вызовов клиента
 *
 */
typedef struct CallbacksClient : public callbacks_t {
	// Обратный вызов при чтении события
	awh::engine_t::readCallback read;
	// Обратный вызов при записи события
	awh::engine_t::writeCallback write;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit CallbacksClient() noexcept : read(nullptr), write(nullptr) {}
} callbacks_client_t;

/**
 * @brief Структура конечного подключения
 *
 */
typedef struct Endpoint {
	// Размер объекта подключения
	socklen_t size;
	// Параметры подключения клиента
	struct sockaddr_storage client;
	// Параметры подключения сервера
	struct sockaddr_storage server;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Endpoint() noexcept :
	 size(0), client{0}, server{0} {}
} endpoint_t;

/**
 * @brief Структура узла события
 *
 */
typedef struct Node {
	// Состояние события
	state_t state;
	/**
	 * @brief Конструктор
	 *
	 */
	virtual ~Node() = default;
} node_t;

/**
 * @brief Структура таймера
 *
 */
typedef struct Timer : public node_t {
	// Задержка времени таймера в миллисекундах
	uint32_t delay;
	// Обратные вызовы события
	callbacks_t callbacks;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Timer() noexcept : delay(0) {}
} timer_t;

/**
 * @brief Структура файловой системы
 *
 */
typedef struct Filesystem : public node_t {
	// Файловый дескриптор
	int32_t fd;
	// Путь к файлу, каталогу или сокету
	address_fs_t path;
	// Обратные вызовы события
	callbacks_client_t callbacks;
	// Чёрный список адресов которым запрещён доступ
	unordered_set <unique_ptr <address_t>> blacklist;
	// Белый список адресов которым разрешён доступ
	unordered_set <unique_ptr <address_t>> whitelist;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Filesystem() noexcept : fd(-1) {}
} fs_t;

/**
 * @brief Структура сервера
 *
 */
typedef struct Server : public node_t {
	// Хост события
	host_t host;
	// Размер очереди ожидания подключения
	uint32_t backlog;
	// Объект параметров конечной точки
	endpoint_t endpoint;
	// MAC-адрес сетевого интерфейса
	address_mac_t macAddress;
	// Обратные вызовы события
	callbacks_server_t callbacks;
	// Сетевые интерфейсы события
	unordered_set <string> interfaces;
	// Опции активных событий
	unordered_set <awh::event::option_t> options;
	// Чёрный список пиров которым запрещён доступ
	unordered_set <unique_ptr <address_t>> blacklist;
	// Белый список пиров которым разрешён доступ
	unordered_set <unique_ptr <address_t>> whitelist;
	// Сетевые адреса для выхода в интернет
	unordered_set <unique_ptr <address_network_t>> networks;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Server() noexcept : backlog{SOMAXCONN} {}
} server_t;

/**
 * @brief Структура клиента
 *
 */
typedef struct Client : public node_t {
	// Хост события
	host_t host;
	// Объект параметров конечной точки
	endpoint_t endpoint;
	// Обратные вызовы события
	callbacks_client_t callbacks;
	// Сетевые интерфейсы события
	unordered_set <string> interfaces;
	// Опции активных событий
	unordered_set <awh::event::option_t> options;
	// Чёрный список серверов к которым запрещёно подключение
	unordered_set <unique_ptr <address_t>> blacklist;
	// Белый список серверов к которым разрешено подключение
	unordered_set <unique_ptr <address_t>> whitelist;
	// Сетевые адреса для выхода в интернет
	unordered_set <unique_ptr <address_network_t>> networks;
	// Размеры активных буферов события
	unordered_map <awh::event::action_t, size_t> bufferSize;
	// Активные таймауты события
	unordered_map <awh::event::action_t, uint32_t> timeouts;
	// Активные действия события
	unordered_map <awh::event::action_t, awh::event::notify_t> actions;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Client() noexcept {}
} client_t;

/**
 * @brief Структура подключённого клиента
 *
 */
typedef struct Peer : public node_t {
	// Хост события
	host_t host;
	// MAC-адрес сетевого интерфейса
	address_mac_t macAddress;
	// Обратные вызовы события
	callbacks_client_t callbacks;
	// Опции активных событий
	unordered_set <awh::event::option_t> options;
	// Размеры активных буферов события
	unordered_map <awh::event::action_t, size_t> bufferSize;
	// Активные таймауты события
	unordered_map <awh::event::action_t, uint32_t> timeouts;
	// Активные действия события
	unordered_map <awh::event::action_t, awh::event::notify_t> actions;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Peer() noexcept {}
} peer_t;

/**
 * Глобальная переменная списка узлов событий
 */
static unordered_map <awh::event::id_t, unique_ptr <node_t>> __awh_nodes__;

/**
 * @brief Функция генерации уникального идентификатора
 *
 * @return уникальный идентификатор
 */
static uint32_t identifier() noexcept {
	// Начинаем с 1 (0 можно оставить как "invalid")
	static atomic_uint32_t id{1};
	// Выводим новое значение идентификатора
	return id.fetch_add(1, memory_order_relaxed);
}

/**
 * @brief Структура сетевых адресов
 *
 */
typedef struct Networks {
	// Название сетвого интерфейса
	string iface;
	// Хост сети в хостовом порядке
    address_t host;
	// Бродкаст сети в хостовом порядке
    address_t broadcast;
	// Мультикаст сети в хостовом порядке
    address_t multicast;
	/**
	 * @brief Конструктор
	 * 
	 */
	explicit Networks() noexcept : iface{""} {}
} __attribute__((packed)) networks_t;

/**
 * @brief Вспомогательная функция проверки принадлежности IP-адреса подсети
 *
 * @param ip     проверяемый IP-адрес в хостовом порядке
 * @param net    сетевой адрес подсети в хостовом порядке
 * @param prefix префикс подсети
 * @return       результат проверки
 */
static bool isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) noexcept {
	// Если префикс равен нулю, то любой IP-адрес принадлежит подсети
	if(prefix == 0)
		// Выводим результат проверки
		return true;
	// Вычисляем маску подсети
	uint32_t mask = (~((1U << (32 - prefix)) - 1));
	// Проверяем принадлежность IP-адреса подсети
	return ((ip & mask) == (net & mask));
}

/**
 * @brief Сравнение двух IPv6-адресов по префиксу (в битах)
 *
 * @param a      Первый IPv6-адрес
 * @param b      Второй IPv6-адрес
 * @param length Длина префикса в битах
 * @return       Результат сравнения
 */
static bool ipv6PrefixEqual(const uint8_t * a, const uint8_t * b, const uint8_t length) noexcept {
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
}

/**
 * @brief Функция поиска сетевого интерфейса по заданной сети
 *
 * @param network сетевой адрес подсети в хостовом порядке
 * @param log     объект для работы с логами
 * @return        структура сетевых адресов
 */
static networks_t findInterfaceInNetwork(const address_network_ipv4_t & network, const awh::log_t * log) noexcept {
	// Результат работы функции
	networks_t result;
	// Проверяем корректность префикса сети
	if(network.prefix > 32){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Invalid prefix");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::WARNING, "Invalid prefix");
		#endif
		// Выводим пустой результат
		return result;
	}
	// Проверка выравнивания сетевого адреса по маске
	const uint32_t mask = ((network.prefix == 0) ? 0 : (~((1U << (32 - network.prefix)) - 1)));
	// Если сетевой адрес не выровнен по маске
	if((network.address & mask) != network.address){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Network address is not aligned to prefix");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::WARNING, "Network address is not aligned to prefix");
		#endif
		// Выводим пустой результат
		return result;
	}
	// Получаем список сетевых интерфейсов
	struct ifaddrs * ifaddrs_ptr = nullptr;
	// Выполняем получение списка сетевых интерфейсов
	if(::getifaddrs(&ifaddrs_ptr) != 0){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "getifaddrs failed");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::WARNING, "getifaddrs failed");
		#endif
		// Выводим пустой результат
		return result;
	}
	// Инициализируем поля результата
	result.host = address_network_ipv4_t{};
	// Инициализируем поля результата
	result.broadcast = address_network_ipv4_t{};
	// Инициализируем поля результата
	result.multicast = address_network_ipv4_t{};
	// Устанавливаем размер хостового адреса
	result.host.size = 4;
	// Устанавливаем размер широковещательного адреса
	result.broadcast.size = 4;
	// Устанавливаем размер мультикастового адреса
	result.multicast.size = 4;
	// Устанавливаем префикс мультикастового адреса
	static_cast <address_network_ipv4_t &> (result.multicast).prefix = 32;
	// Устанавливаем префикс хостового адреса
	static_cast <address_network_ipv4_t &> (result.host).prefix = network.prefix;
	// Устанавливаем префикс широковещательного адреса
	static_cast <address_network_ipv4_t &> (result.broadcast).prefix = network.prefix;
	// Перебираем все сетевые интерфейсы
	for(struct ifaddrs * ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next){
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
		struct sockaddr_in * addr_in = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr);
		// Преобразуем IP-адрес в хостовый порядок
		const uint32_t ip = ntohl(addr_in->sin_addr.s_addr);
		// Проверяем принадлежность IP-адреса подсети
		if(::isInSubnet(ip, network.address, network.prefix)){
			// Устанавливаем название сетевого интерфейса
			result.iface = ifa->ifa_name;
			// Устанавливаем хост сети
			static_cast <address_network_ipv4_t &> (result.host).address = ip;
			// Получаем broadcast из системы или вычисляем
			if(ifa->ifa_broadaddr){
				// Получаем broadcast из системы
				struct sockaddr_in * bcast_in = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_broadaddr);
				// Устанавливаем бродкаст сети
				static_cast <address_network_ipv4_t &> (result.broadcast).address = ntohl(bcast_in->sin_addr.s_addr);
			// Если broadcast не задан системой
			} else {
				// Вычисляем бродкаст сети
				uint32_t net = (ip & mask);
				// Устанавливаем бродкаст сети
				static_cast <address_network_ipv4_t &> (result.broadcast).address = (net | (~mask));
			}
			// Multicast: 239.255.X.Y
			static_cast <address_network_ipv4_t &> (result.multicast).address = (0xEFAF0000U) | (ip & 0x0000FFFFU);
			// Прерываем цикл поиска
			break;
		}
	}
	// Освобождаем память от списка сетевых интерфейсов
	::freeifaddrs(ifaddrs_ptr);
	// Выводим результат работы функции
	return result;
}

/**
 * @brief Функция поиска сетевого интерфейса по заданной IPv6-сети
 *
 * @param network сетевой адрес подсети
 * @param log     объект для работы с логами
 * @return        структура сетевых адресов
 */
static networks_t findInterfaceInIPv6Network(const address_network_ipv6_t & network, const awh::log_t * log) noexcept {
	// Результат работы функции
	networks_t result;
	// Проверяем корректность префикса сети
	if(network.prefix > 128){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Invalid IPv6 prefix");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::WARNING, "Invalid IPv6 prefix");
		#endif
		// Выводим пустой результат
		return result;
	}
	// Получаем список сетевых интерфейсов
	struct ifaddrs * ifaddrs_ptr = nullptr;
	// Выполняем получение списка сетевых интерфейсов
	if(::getifaddrs(&ifaddrs_ptr) != 0){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "getifaddrs failed");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::WARNING, "getifaddrs failed");
		#endif
		// Выводим пустой результат
		return result;
	}
	// Инициализируем поля результата
	result.host = address_network_ipv6_t{};
	// Инициализируем поля результата
	result.multicast = address_network_ipv6_t{};
	// Устанавливаем размер хостового адреса
	result.host.size = 16;
	// Устанавливаем размер мультикастового адреса
	result.multicast.size = 16;
	// Устанавливаем префикс мультикастового адреса
	static_cast <address_network_ipv6_t &> (result.multicast).prefix = 128;
	// Устанавливаем префикс хостового адреса
	static_cast <address_network_ipv6_t &> (result.host).prefix = network.prefix;
	// Временный IPv6-адрес
	struct in6_addr addr;
	// Перебираем все сетевые интерфейсы
	for(struct ifaddrs * ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next){
		// Пропускаем не IPv6-интерфейсы
		if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
			// Пропускаем интерфейсы, которые не являются IPv6
			continue;
		// Пропускаем выключенные интерфейсы
		if(!(ifa->ifa_flags & IFF_UP))
			// Пропускаем неактивные интерфейсы
			continue;
		// Получаем указатель на IPv6-адрес
		struct sockaddr_in6 * addr_in6 = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
		// Получаем ссылку на IP-адрес
		const in6_addr & ip = addr_in6->sin6_addr;
		// Пропускаем link-local, если не ищем их (опционально)
		// if(IN6_IS_ADDR_LINKLOCAL(&ip)) continue;
		// Проверяем принадлежность IP-адреса подсети
		if(::ipv6PrefixEqual(ip.s6_addr, network.address, network.prefix)){
			// Устанавливаем название сетевого интерфейса
			result.iface = ifa->ifa_name;
			// Хост: просто копируем найденный адрес
			::memcpy(static_cast <address_network_ipv6_t &> (result.host).address, ip.s6_addr, sizeof(ip.s6_addr));
			// Multicast: используем ff02::1 (все хосты в локальном сегменте)
			// Или можно сделать производный адрес, но обычно используют стандартные
			::inet_pton(AF_INET6, "ff02::1", &addr);
			// Получаем ссылку на Multicast-адрес
			::memcpy(static_cast <address_network_ipv6_t &> (result.multicast).address, addr.s6_addr, sizeof(addr.s6_addr));
			// Прерываем цикл поиска
			break;
		}
	}
	// Освобождаем память списка сетевых интерфейсов
	::freeifaddrs(ifaddrs_ptr);
	// Выводим результат работы функции
	return result;
}

/**
 * @brief Функция применения сетевой оптимизации операционной системы
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
static void boostingNetwork([[maybe_unused]] const awh::fmk_t * fmk, const awh::log_t * log) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем инициализацию объекта работы с операционноы системы
		awh::os_t os(log);
		// Выполняем инициализацию объекта работы с файловыми дескрипторами
		awh::fds_t fds(log);
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
					log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, ::strerror(errno));
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					log->print("%s", awh::log_t::flag_t::WARNING, ::strerror(errno));
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
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
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
						if(fmk->exists("cubic", algorithm))
							// Активируем выбранный нами алгоритм
							os.sysctl("net.inet.tcp.cc.algorithm", "cubic");
						// Если же найден алгоритм htcp
						else if(fmk->exists("htcp", algorithm))
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
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::WARNING, "Root privileges are required to apply network optimizations");
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
			log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			log->print("%s", awh::log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}

/**
 * @brief Метод опроса событий
 *
 * @param timeout таймаут опроса в миллисекундах
 * @return        результат выполнения опроса
 */
bool awh::IO::poll(const int32_t timeout) noexcept {

	return false;
}
/**
 * @brief Метод получения порта события
 *
 * @param id идентификатор события
 * @return   порт события
 */
uint32_t awh::IO::port(const event::id_t id) const noexcept {
	// Выполняем поиск идентификатора события
	auto i = ::__awh_nodes__.find(id);
	// Если идентификатор события найден
	if(i != ::__awh_nodes__.end()){
		/**
		 * Определяем семейство сокета
		 */
		switch(static_cast <uint8_t> (i->second->state.family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6):
			// Для семейства UDPv4
			case static_cast <uint8_t> (event::family_t::UDPV4):
			// Для семейства UDPv6
			case static_cast <uint8_t> (event::family_t::UDPV6): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER):
						// Возвращаем результат работы функции
						return static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).port;
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT):
						// Возвращаем результат работы функции
						return static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).port;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER):
						// Возвращаем результат работы функции
						return static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).port;
				}
			} break;
			// Для остальных семейств сокетов
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Port cannot be retrieved for events that are not network related", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Port cannot be retrieved for events that are not network related", log_t::flag_t::WARNING);
				#endif
			}
		}
	}
	// Возвращаем результат работы функции
	return 0;
}
/**
 * @brief Метод установки порта события
 *
 * @param id   идентификатор события
 * @param port порт события
 * @return     результат выполнения установки
 */
bool awh::IO::port(const event::id_t id, const uint32_t port) noexcept {
	// Выполняем поиск идентификатора события
	auto i = ::__awh_nodes__.find(id);
	// Если идентификатор события найден
	if(i != ::__awh_nodes__.end()){
		/**
		 * Определяем семейство сокета
		 */
		switch(static_cast <uint8_t> (i->second->state.family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6):
			// Для семейства UDPv4
			case static_cast <uint8_t> (event::family_t::UDPV4):
			// Для семейства UDPv6
			case static_cast <uint8_t> (event::family_t::UDPV6): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Устанавливаем порт события
						static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).port = port;
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Устанавливаем порт события
						static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).port = port;
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Устанавливаем порт события
						static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).port = port;
						// Возвращаем результат работы функции
						return true;
					}
				}
			} break;
			// Для остальных семейств сокетов
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Port cannot be set for events that are not network related",	 __PRETTY_FUNCTION__, std::make_tuple(id, port), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Port cannot be set for events that are not network related", log_t::flag_t::WARNING);
				#endif
			}
		}
	}
	// Возвращаем результат работы функции
	return false;
}
/**
 * @brief Метод получения хоста события
 *
 * @param id идентификатор события
 * @return   хост события
 */
string awh::IO::host(const event::id_t id) const noexcept {
	// Выполняем поиск идентификатора события
	auto i = ::__awh_nodes__.find(id);
	// Если идентификатор события найден
	if(i != ::__awh_nodes__.end()){
		/**
		 * Определяем семейство сокета
		 */
		switch(static_cast <uint8_t> (i->second->state.family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства UDPv4
			case static_cast <uint8_t> (event::family_t::UDPV4): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Устанавливаем полученный IP-адрес
						this->_net.v4(static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip).address, net_t::endian_t::BIG);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Устанавливаем полученный IP-адрес
						this->_net.v4(static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip).address, net_t::endian_t::BIG);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Устанавливаем полученный IP-адрес
						this->_net.v4(static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip).address, net_t::endian_t::BIG);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					}
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6):
			// Для семейства UDPv6
			case static_cast <uint8_t> (event::family_t::UDPV6): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Формируем буфер для хранения IP-адреса
						array <uint64_t, 2> address;
						// Выполняем копирование установленного IP-адреса
						::memcpy(&address[0], static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip).address, sizeof(address));
						// Устанавливаем полученный IP-адрес
						this->_net.v6(address, net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return static_cast <string> (this->_net);
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Формируем буфер для хранения IP-адреса
						array <uint64_t, 2> address;
						// Выполняем копирование установленного IP-адреса
						::memcpy(&address[0], static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip).address, sizeof(address));
						// Устанавливаем полученный IP-адрес
						this->_net.v6(address, net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return static_cast <string> (this->_net);
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Формируем буфер для хранения IP-адреса
						array <uint64_t, 2> address;
						// Выполняем копирование установленного IP-адреса
						::memcpy(&address[0], static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip).address, sizeof(address));
						// Устанавливаем полученный IP-адрес
						this->_net.v6(address, net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return static_cast <string> (this->_net);
					}
				}
			} break;
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER):
						// Возвращаем адрес сокета в UNIX-домене
						return static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <peer_t *> (i->second.get())->host).path).address;
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT):
						// Возвращаем адрес сокета в UNIX-домене
						return static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <client_t *> (i->second.get())->host).path).address;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER):
						// Возвращаем адрес сокета в UNIX-домене
						return static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <server_t *> (i->second.get())->host).path).address;
				}
			} break;
			// Для семейства директорий
			case static_cast <uint8_t> (event::family_t::DIR):
			// Для семейства файловой системы
			case static_cast <uint8_t> (event::family_t::FILE):
				// Возвращаем адрес файловой системы события
				return static_cast <address_fs_t &> (static_cast <fs_t *> (i->second.get())->path).address;
			// Для остальных семейств сокетов
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Host cannot be retrieved for events that are not network or filesystem related", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Host cannot be retrieved for events that are not network or filesystem related", log_t::flag_t::WARNING);
				#endif
			}
		}
	}
	// Возвращаем результат работы функции
	return "";
}
/**
 * @brief Метод установки хоста события
 *
 * @param id   идентификатор события
 * @param host хост события
 * @return     результат выполнения установки
 */
bool awh::IO::host(const event::id_t id, const string & host) noexcept {
	// Выполняем поиск идентификатора события
	auto i = ::__awh_nodes__.find(id);
	// Если идентификатор события найден
	if(i != ::__awh_nodes__.end()){
		/**
		 * Определяем семейство сокета
		 */
		switch(static_cast <uint8_t> (i->second->state.family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства UDPv4
			case static_cast <uint8_t> (event::family_t::UDPV4): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Устанавливаем полученный IP-адрес
						this->_net.parse(host, net_t::type_t::IPV4);
						// Получаем объект адреса для установки данных
						auto & address = static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip);
						// Устанавливаем размер буфера для хранения IP-адреса
						address.size = 4;
						// Устанавливаем префикс хостового адреса
						address.prefix = 32;
						// Копируем полученный IP-адрес в объект события
						address.address = this->_net.v4(net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Устанавливаем полученный IP-адрес
						this->_net.parse(host, net_t::type_t::IPV4);
						// Получаем объект адреса для установки данных
						auto & address = static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip);
						// Устанавливаем размер буфера для хранения IP-адреса
						address.size = 4;
						// Устанавливаем префикс хостового адреса
						address.prefix = 32;
						// Копируем полученный IP-адрес в объект события
						address.address = this->_net.v4(net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Устанавливаем полученный IP-адрес
						this->_net.parse(host, net_t::type_t::IPV4);
						// Получаем объект адреса для установки данных
						auto & address = static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip);
						// Устанавливаем размер буфера для хранения IP-адреса
						address.size = 4;
						// Устанавливаем префикс хостового адреса
						address.prefix = 32;
						// Копируем полученный IP-адрес в объект события
						address.address = this->_net.v4(net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return true;
					}
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6):
			// Для семейства UDPv6
			case static_cast <uint8_t> (event::family_t::UDPV6): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Устанавливаем полученный IP-адрес
						this->_net.parse(host, net_t::type_t::IPV6);
						// Получаем объект адреса для установки данных
						auto & address = static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip);
						// Устанавливаем размер буфера для хранения IP-адреса
						address.size = 16;
						// Устанавливаем префикс хостового адреса
						address.prefix = 128;
						// Копируем полученный IP-адрес в объект события
						::memcpy(address.address, &this->_net.v6(net_t::endian_t::BIG)[0], address.size);
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Устанавливаем полученный IP-адрес
						this->_net.parse(host, net_t::type_t::IPV6);
						// Получаем объект адреса для установки данных
						auto & address = static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip);
						// Устанавливаем размер буфера для хранения IP-адреса
						address.size = 16;
						// Устанавливаем префикс хостового адреса
						address.prefix = 128;
						// Копируем полученный IP-адрес в объект события
						::memcpy(address.address, &this->_net.v6(net_t::endian_t::BIG)[0], address.size);
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Устанавливаем полученный IP-адрес
						this->_net.parse(host, net_t::type_t::IPV6);
						// Получаем объект адреса для установки данных
						auto & address = static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip);
						// Устанавливаем размер буфера для хранения IP-адреса
						address.size = 16;
						// Устанавливаем префикс хостового адреса
						address.prefix = 128;
						// Копируем полученный IP-адрес в объект события
						::memcpy(address.address, &this->_net.v6(net_t::endian_t::BIG)[0], address.size);
						// Возвращаем результат работы функции
						return true;
					}
				}
			} break;
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Устанавливаем адрес сокета в UNIX-домене
						static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <peer_t *> (i->second.get())->host).path).address = host;
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Устанавливаем адрес сокета в UNIX-домене
						static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <client_t *> (i->second.get())->host).path).address = host;
						// Возвращаем результат работы функции
						return true;
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Устанавливаем адрес сокета в UNIX-домене
						static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <server_t *> (i->second.get())->host).path).address = host;
						// Возвращаем результат работы функции
						return true;
					}
				}
			} break;
			// Для семейства директорий
			case static_cast <uint8_t> (event::family_t::DIR):
			// Для семейства файловой системы
			case static_cast <uint8_t> (event::family_t::FILE): {
				// Устанавливаем адрес файловой системы события
				static_cast <address_fs_t &> (static_cast <fs_t *> (i->second.get())->path).address = host;
				// Возвращаем результат работы функции
				return true;
			}
			// Для остальных семейств сокетов
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Host cannot be set for events that are not network or filesystem related", __PRETTY_FUNCTION__, std::make_tuple(id, host), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Host cannot be set for events that are not network or filesystem related", log_t::flag_t::WARNING);
				#endif
			}
		}
	}
	// Возвращаем результат работы функции
	return false;
}
/**
 * @brief Метод получения адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @return        значение адреса события
 */
string awh::IO::address(const event::id_t id, const event::address_t address) const noexcept {
	// Выполняем поиск идентификатора события
	auto i = ::__awh_nodes__.find(id);
	// Если идентификатор события найден
	if(i != ::__awh_nodes__.end()){
		/**
		 * Определяем тип адреса события
		 */
		switch(static_cast <uint8_t> (address)){
			// Если тип адреса принадлежит к MAC-адресам
			case static_cast <uint8_t> (event::address_t::MAC): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Контейнер MAC-адреса
						uint64_t address;
						// Копируем MAC-адрес из объекта события
						::memcpy(&address, static_cast <peer_t *> (i->second.get())->macAddress.address, sizeof(6));
						// Устанавливаем полученный MAC-адрес
						this->_net.mac(address);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					} break;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Контейнер MAC-адреса
						uint64_t address;
						// Копируем MAC-адрес из объекта события
						::memcpy(&address, static_cast <server_t *> (i->second.get())->macAddress.address, sizeof(6));
						// Устанавливаем полученный MAC-адрес
						this->_net.mac(address);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					} break;
				}
			} break;
			// Если тип адреса принадлежит к Unix Domain Socket
			case static_cast <uint8_t> (event::address_t::UDS): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER):
						// Возвращаем адрес сокета в UNIX-домене
						return static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <peer_t *> (i->second.get())->host).path).address;
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT):
						// Возвращаем адрес сокета в UNIX-домене
						return static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <client_t *> (i->second.get())->host).path).address;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER):
						// Возвращаем адрес сокета в UNIX-домене
						return static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <server_t *> (i->second.get())->host).path).address;
				}
			} break;
			// Если тип адреса принадлежит к дирректориям файловой системы
			case static_cast <uint8_t> (event::address_t::DIR):
			// Если тип адреса принадлежит к файлам файловой системы
			case static_cast <uint8_t> (event::address_t::FILE):
				// Возвращаем адрес файловой системы события
				return static_cast <address_fs_t &> (static_cast <fs_t *> (i->second.get())->path).address;
			// Если тип адреса принадлежит к IPv4-адресам
			case static_cast <uint8_t> (event::address_t::IPV4): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Устанавливаем полученный IP-адрес
						this->_net.v4(static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip).address, net_t::endian_t::BIG);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Устанавливаем полученный IP-адрес
						this->_net.v4(static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip).address, net_t::endian_t::BIG);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Устанавливаем полученный IP-адрес
						this->_net.v4(static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip).address, net_t::endian_t::BIG);
						// Возвращаем хост события
						return static_cast <string> (this->_net);
					}
				}
			} break;
			// Если тип адреса принадлежит к IPv6-адресам
			case static_cast <uint8_t> (event::address_t::IPV6): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является соседом
					case static_cast <uint8_t> (event::node_t::PEER): {
						// Формируем буфер для хранения IP-адреса
						array <uint64_t, 2> address;
						// Выполняем копирование установленного IP-адреса
						::memcpy(&address[0], static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip).address, sizeof(address));
						// Устанавливаем полученный IP-адрес
						this->_net.v6(address, net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return static_cast <string> (this->_net);
					}
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Формируем буфер для хранения IP-адреса
						array <uint64_t, 2> address;
						// Выполняем копирование установленного IP-адреса
						::memcpy(&address[0], static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip).address, sizeof(address));
						// Устанавливаем полученный IP-адрес
						this->_net.v6(address, net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return static_cast <string> (this->_net);
					}
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Формируем буфер для хранения IP-адреса
						array <uint64_t, 2> address;
						// Выполняем копирование установленного IP-адреса
						::memcpy(&address[0], static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip).address, sizeof(address));
						// Устанавливаем полученный IP-адрес
						this->_net.v6(address, net_t::endian_t::BIG);
						// Возвращаем результат работы функции
						return static_cast <string> (this->_net);
					}
				}
			} break;
			// Если тип адреса принадлежит к сетям
			case static_cast <uint8_t> (event::address_t::NETWORK): {
				/**
				 * Определяем чем является текущая нода
				 */
				switch(static_cast <uint8_t> (i->second->state.node)){
					// Если нода является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Получаем объект клиента
						auto client = static_cast <client_t *> (i->second.get());
						// Если список сетей у клиента не пустой
						if(!client->networks.empty()){
							// Получаем первую сеть из списка
							const auto & i = client->networks.begin();
							// Получаем параметры сети
							const address_network_t & network = (* i->get());
							/**
							 * Определяем тип адреса сети
							 */
							switch(static_cast <uint8_t> (network.size)){
								// Если тип адреса сети является IPv4
								case 4: {
									// Устанавливаем полученный сетевой адрес
									this->_net.v4(static_cast <const address_network_ipv4_t &> (network).address, net_t::endian_t::BIG);
									// Возвращаем хост события
									return (static_cast <string> (this->_net) + "/" + std::to_string(static_cast <const address_network_ipv4_t &> (network).prefix));
								} break;
								// Если тип адреса сети является IPv6
								case 16: {
									// Формируем буфер для хранения IP-адреса
									array <uint64_t, 2> address;
									// Выполняем копирование установленного IP-адреса
									::memcpy(&address[0], static_cast <const address_network_ipv6_t &> (network).address, sizeof(address));
									// Устанавливаем полученный сетевой адрес
									this->_net.v6(address, net_t::endian_t::BIG);
									// Возвращаем результат работы функции
									return (static_cast <string> (this->_net) + "/" + std::to_string(static_cast <const address_network_ipv6_t &> (network).prefix));
								} break;
							}
						}
					} break;
					// Если нода является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Получаем объект клиента
						auto server = static_cast <server_t *> (i->second.get());
						// Если список сетей у клиента не пустой
						if(!server->networks.empty()){
							// Получаем первую сеть из списка
							const auto & i = server->networks.begin();
							// Получаем параметры сети
							const address_network_t & network = (* i->get());
							/**
							 * Определяем тип адреса сети
							 */
							switch(static_cast <uint8_t> (network.size)){
								// Если тип адреса сети является IPv4
								case 4: {
									// Устанавливаем полученный сетевой адрес
									this->_net.v4(static_cast <const address_network_ipv4_t &> (network).address, net_t::endian_t::BIG);
									// Возвращаем хост события
									return (static_cast <string> (this->_net) + "/" + std::to_string(static_cast <const address_network_ipv4_t &> (network).prefix));
								} break;
								// Если тип адреса сети является IPv6
								case 16: {
									// Формируем буфер для хранения IP-адреса
									array <uint64_t, 2> address;
									// Выполняем копирование установленного IP-адреса
									::memcpy(&address[0], static_cast <const address_network_ipv6_t &> (network).address, sizeof(address));
									// Устанавливаем полученный сетевой адрес
									this->_net.v6(address, net_t::endian_t::BIG);
									// Возвращаем результат работы функции
									return (static_cast <string> (this->_net) + "/" + std::to_string(static_cast <const address_network_ipv6_t &> (network).prefix));
								} break;
							}
						}
					} break;
				}
			} break;
		}
	}
	// Возвращаем результат работы функции
	return "";
}
/**
 * @brief Метод установки адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @param value   значение адреса события
 * @return        результат выполнения установки
 */
bool awh::IO::address(const event::id_t id, const event::address_t address, const string & value) noexcept {
	// Если значение адреса для установки передано не пустым
	if(!value.empty()){
		// Выполняем поиск идентификатора события
		auto i = ::__awh_nodes__.find(id);
		// Если идентификатор события найден
		if(i != ::__awh_nodes__.end()){
			/**
			 * Определяем тип адреса события
			 */
			switch(static_cast <uint8_t> (address)){
				// Если тип адреса не определён
				case static_cast <uint8_t> (event::address_t::NONE): {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Address type NONE cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Address type NONE cannot be set", log_t::flag_t::WARNING);
					#endif
				} break;
				// Если тип адреса принадлежит к MAC-адресам
				case static_cast <uint8_t> (event::address_t::MAC): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный MAC-адрес
							this->_net.parse(value, net_t::type_t::MAC);
							// Получаем текущее значение MAC-адреса
							const uint64_t mac = this->_net.mac();
							// Получаем объект адреса для установки данных
							auto & macAddress = static_cast <peer_t *> (i->second.get())->macAddress;
							// Устанавливаем размер буфера для хранения MAC-адреса
							macAddress.size = 6;
							// Копируем полученный MAC-адрес в объект события
							::memcpy(macAddress.address, &mac, macAddress.size);
							// Возвращаем результат работы функции
							return true;
						} break;
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный MAC-адрес
							this->_net.parse(value, net_t::type_t::MAC);
							// Получаем текущее значение MAC-адреса
							const uint64_t mac = this->_net.mac();
							// Получаем объект адреса для установки данных
							auto & macAddress = static_cast <server_t *> (i->second.get())->macAddress;
							// Устанавливаем размер буфера для хранения MAC-адреса
							macAddress.size = 6;
							// Копируем полученный MAC-адрес в объект события
							::memcpy(macAddress.address, &mac, macAddress.size);
							// Возвращаем результат работы функции
							return true;
						} break;
						// Если нода имеет неподдерживаемый тип
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("MAC address can only be set for PEER or SERVER nodes", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("MAC address can only be set for PEER or SERVER nodes", log_t::flag_t::WARNING);
							#endif
						}
					}
				} break;
				// Если тип адреса принадлежит к Unix Domain Socket
				case static_cast <uint8_t> (event::address_t::UDS): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем адрес сокета в UNIX-домене
							static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <peer_t *> (i->second.get())->host).path).address = value;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем адрес сокета в UNIX-домене
							static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <client_t *> (i->second.get())->host).path).address = value;
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем адрес сокета в UNIX-домене
							static_cast <address_fs_t &> (static_cast <host_udc_t &> (static_cast <server_t *> (i->second.get())->host).path).address = value;
							// Возвращаем результат работы функции
							return true;
						}
					}
				} break;
				// Если тип адреса принадлежит к дирректориям файловой системы
				case static_cast <uint8_t> (event::address_t::DIR):
				// Если тип адреса принадлежит к файлам файловой системы
				case static_cast <uint8_t> (event::address_t::FILE): {
					// Устанавливаем адрес файловой системы события
					static_cast <address_fs_t &> (static_cast <fs_t *> (i->second.get())->path).address = value;
					// Возвращаем результат работы функции
					return true;
				} break;
				// Если тип адреса принадлежит к IPv4-адресам
				case static_cast <uint8_t> (event::address_t::IPV4): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(value, net_t::type_t::IPV4);
							// Получаем объект адреса для установки данных
							auto & address = static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip);
							// Устанавливаем размер буфера для хранения IP-адреса
							address.size = 4;
							// Устанавливаем префикс хостового адреса
							address.prefix = 32;
							// Копируем полученный IP-адрес в объект события
							address.address = this->_net.v4(net_t::endian_t::BIG);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(value, net_t::type_t::IPV4);
							// Получаем объект адреса для установки данных
							auto & address = static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip);
							// Устанавливаем размер буфера для хранения IP-адреса
							address.size = 4;
							// Устанавливаем префикс хостового адреса
							address.prefix = 32;
							// Копируем полученный IP-адрес в объект события
							address.address = this->_net.v4(net_t::endian_t::BIG);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(value, net_t::type_t::IPV4);
							// Получаем объект адреса для установки данных
							auto & address = static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip);
							// Устанавливаем размер буфера для хранения IP-адреса
							address.size = 4;
							// Устанавливаем префикс хостового адреса
							address.prefix = 32;
							// Копируем полученный IP-адрес в объект события
							address.address = this->_net.v4(net_t::endian_t::BIG);
							// Возвращаем результат работы функции
							return true;
						}
					}
				} break;
				// Если тип адреса принадлежит к IPv6-адресам
				case static_cast <uint8_t> (event::address_t::IPV6): {
					/**
					 * Определяем чем является текущая нода
					 */
					switch(static_cast <uint8_t> (i->second->state.node)){
						// Если нода является соседом
						case static_cast <uint8_t> (event::node_t::PEER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(value, net_t::type_t::IPV6);
							// Получаем объект адреса для установки данных
							auto & address = static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip);
							// Устанавливаем размер буфера для хранения IP-адреса
							address.size = 16;
							// Устанавливаем префикс хостового адреса
							address.prefix = 128;
							// Копируем полученный IP-адрес в объект события
							::memcpy(address.address, &this->_net.v6(net_t::endian_t::BIG)[0], address.size);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является клиентом
						case static_cast <uint8_t> (event::node_t::CLIENT): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(value, net_t::type_t::IPV6);
							// Получаем объект адреса для установки данных
							auto & address = static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <client_t *> (i->second.get())->host).ip);
							// Устанавливаем размер буфера для хранения IP-адреса
							address.size = 16;
							// Устанавливаем префикс хостового адреса
							address.prefix = 128;
							// Копируем полученный IP-адрес в объект события
							::memcpy(address.address, &this->_net.v6(net_t::endian_t::BIG)[0], address.size);
							// Возвращаем результат работы функции
							return true;
						}
						// Если нода является сервером
						case static_cast <uint8_t> (event::node_t::SERVER): {
							// Устанавливаем полученный IP-адрес
							this->_net.parse(value, net_t::type_t::IPV6);
							// Получаем объект адреса для установки данных
							auto & address = static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <server_t *> (i->second.get())->host).ip);
							// Устанавливаем размер буфера для хранения IP-адреса
							address.size = 16;
							// Устанавливаем префикс хостового адреса
							address.prefix = 128;
							// Копируем полученный IP-адрес в объект события
							::memcpy(address.address, &this->_net.v6(net_t::endian_t::BIG)[0], address.size);
							// Возвращаем результат работы функции
							return true;
						}
					}
				} break;
				// Если тип адреса принадлежит к сетям
				case static_cast <uint8_t> (event::address_t::NETWORK): {
					// IP-адрес сети
					string ip = "";
					// Маска сети
					string mask = "";
					// Тип сети
					net_t::type_t type = net_t::type_t::NONE;
					// Выполняем поиск разделителя сети
					auto pos = value.find('/');
					// Если разделитель найден
					if(pos != string::npos){
						// Получаем значение IP-адреса сети
						ip = value.substr(0, pos);
						// Получаем значение маски сети
						mask = value.substr(pos + 1);
					}
					/**
					 * Определяем тип полученного IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_net.host(ip))){
						// Для типа IPv4
						case static_cast <uint8_t> (net_t::type_t::IPV4): {
							// Если маска сети не указана
							if(mask.empty())
								// Устанавливаем маску по умолчанию для IPv4
								mask = "32";
							// Устанавливаем тип сети
							type = net_t::type_t::IPV4;
						} break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_t::type_t::IPV6): {
							// Если маска сети не указана
							if(mask.empty())
								// Устанавливаем маску по умолчанию для IPv6
								mask = "128";
							// Устанавливаем тип сети
							type = net_t::type_t::IPV6;
						} break;
					}
					/**
					 * Определяем какой тип сети необходимо установить
					 */
					switch(static_cast <uint8_t> (type)){
						// Для типа IPv4
						case static_cast <uint8_t> (net_t::type_t::IPV4): {
							// Параметры сетей интерфейсов
							networks_t networks;
							// IP-адрес сети
							address_network_ipv4_t network;
							// Выполняем парсинг IP-адреса сети
							this->_net.parse(ip, net_t::type_t::IPV4);
							// Получаем значение IP-адреса сети
							network.address = this->_net.v4(net_t::endian_t::BIG);
							// Если маска является префиксом сети
							if(this->_fmk->is(mask, fmk_t::check_t::NUMBER))
								// Устанавливаем префикс сети
								network.prefix = this->_fmk->atoi <uint8_t> (mask);
							// Если маска является стандартной маской сети
							else
								// Устанавливаем префикс сети
								network.prefix = this->_net.mask2Prefix(mask, type);
							// Выполняем поиск интерфейса в указанной сети по префиксу
							networks = ::move(::findInterfaceInNetwork(network, this->_log));
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является соседом
								case static_cast <uint8_t> (event::node_t::PEER): {
									// Копируем полученный IP-адрес в объект события
									static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip) = ::move(static_cast <address_network_ipv4_t &> (networks.host));
									// Возвращаем результат работы функции
									return true;
								}
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем объект клиента
									auto client = static_cast <client_t *> (i->second.get());
									// Устанавливаем название сетевого интерфейса
									client->interfaces.emplace(::move(networks.iface));
									// Добавляем адрес сети для выхода в интернет
									client->networks.insert(make_unique <address_network_ipv4_t> (::move(network)));
									// Копируем полученный IP-адрес в объект события
									static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (client->host).ip) = ::move(static_cast <address_network_ipv4_t &> (networks.host));
									// Возвращаем результат работы функции
									return true;
								}
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем объект сервера
									auto server = static_cast <server_t *> (i->second.get());
									// Устанавливаем название сетевого интерфейса
									server->interfaces.emplace(::move(networks.iface));
									// Добавляем адрес сети для выхода в интернет
									server->networks.insert(make_unique <address_network_ipv4_t> (::move(network)));
									// Копируем полученный IP-адрес в объект события
									static_cast <address_network_ipv4_t &> (static_cast <host_ip_t &> (server->host).ip) = ::move(static_cast <address_network_ipv4_t &> (networks.host));
									// Возвращаем результат работы функции
									return true;
								}
							}
						} break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_t::type_t::IPV6): {
							// Выполняем парсинг IP-адреса сети
							this->_net.parse(ip, net_t::type_t::IPV6);
							// Параметры сетей интерфейсов
							networks_t networks;
							// IP-адрес сети
							address_network_ipv6_t network;
							// Копируем значение IP-адреса сети
							::memcpy(&network.address[0], &this->_net.v6(net_t::endian_t::BIG)[0], sizeof(network.address));
							// Если маска является префиксом сети
							if(this->_fmk->is(mask, fmk_t::check_t::NUMBER))
								// Устанавливаем префикс сети
								network.prefix = this->_fmk->atoi <uint8_t> (mask);
							// Если маска является стандартной маской сети
							else
								// Устанавливаем префикс сети
								network.prefix = this->_net.mask2Prefix(mask, type);
							// Выполняем поиск интерфейса в указанной сети по префиксу
							networks = ::move(::findInterfaceInIPv6Network(network, this->_log));
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является соседом
								case static_cast <uint8_t> (event::node_t::PEER): {
									// Копируем полученный IP-адрес в объект события
									static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (static_cast <peer_t *> (i->second.get())->host).ip) = ::move(static_cast <address_network_ipv6_t &> (networks.host));
									// Возвращаем результат работы функции
									return true;
								}
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем объект клиента
									auto client = static_cast <client_t *> (i->second.get());
									// Добавляем название сетевого интерфейса
									client->interfaces.emplace(::move(networks.iface));
									// Добавляем адрес сети для выхода в интернет
									client->networks.insert(make_unique <address_network_ipv6_t> (::move(network)));
									// Копируем полученный IP-адрес в объект события
									static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (client->host).ip) = ::move(static_cast <address_network_ipv6_t &> (networks.host));
									// Возвращаем результат работы функции
									return true;
								}
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем объект сервера
									auto server = static_cast <server_t *> (i->second.get());
									// Устанавливаем название сетевого интерфейса
									server->interfaces.emplace(::move(networks.iface));
									// Добавляем адрес сети для выхода в интернет
									server->networks.insert(make_unique <address_network_ipv6_t> (::move(network)));
									// Копируем полученный IP-адрес в объект события
									static_cast <address_network_ipv6_t &> (static_cast <host_ip_t &> (server->host).ip) = ::move(static_cast <address_network_ipv6_t &> (networks.host));
									// Возвращаем результат работы функции
									return true;
								}
							}
						} break;
					}
				} break;
				// Для остальных типов адресов
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Unsupported address type cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, value), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Unsupported address type cannot be set", log_t::flag_t::WARNING);
					#endif
				}
			}
		}
	}
	// Возвращаем результат работы функции
	return false;
}
/**
 * @brief Метод настройки события
 *
 * @param id    идентификатор события
 * @param delay задержка таймера события в миллисекундах
 * @return      результат выполнения настройки
 */
bool awh::IO::setup(const event::id_t id, const uint32_t delay) noexcept {
	
	return false;
}
/**
 * @brief Метод настройки события
 *
 * @param id   идентификатор события
 * @param node тип узла события
 * @return     результат выполнения настройки
 */
bool awh::IO::setup(const event::id_t id, const event::node_t node) noexcept {
	return false;
}
/**
 * @brief Метод удаления события
 *
 * @param id идентификатор события
 * @return   результат выполнения удаления
 */
bool awh::IO::destroy(const event::id_t id) noexcept {
	
	return false;
}
/**
 * @brief Метод создания нового события на основе существующего
 *
 * @param id       идентификатор существующего события
 * @param protocol протокол сокета
 * @param mode     режим сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::id_t id, const event::protocol_t protocol, const event::mode_t mode) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	// Выполняем поиск идентификатора события
	auto i = ::__awh_nodes__.find(id);
	// Если идентификатор события найден
	if(i != ::__awh_nodes__.end()){
		// Если событие уже инициализированно
		if(i->second->state.status == event::status_t::INITIAL){
			/**
			 * Определяем семейство сокета
			 */
			switch(static_cast <uint8_t> (i->second->state.family)){
				// Для семейства UDPv4
				case static_cast <uint8_t> (event::family_t::UDPV4):
				// Для семейства UDPv6
				case static_cast <uint8_t> (event::family_t::UDPV6): {
					// Флаг удачного выполнения объединение событий
					bool ok = true;
					/**
					 * Определяем тип сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.type)){
						// Для типа сокета RAW
						case static_cast <uint8_t> (event::type_t::RAW): {
							// Выполняем создание события
							auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
							// Устанавливаем флаг режима сокета
							ret.first->second->state.mode = mode;
							// Устанавливаем флаг протокола сокета
							ret.first->second->state.protocol = protocol;
							// Устанавливаем флаг типа сокета
							ret.first->second->state.type = i->second->state.type;
							// Устанавливаем флаг семейства сокета
							ret.first->second->state.family = i->second->state.family;
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем текущее значение объекта клиента
									client_t * first = static_cast <client_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства UDPv4
										case static_cast <uint8_t> (event::family_t::UDPV4): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как RAW
												case static_cast <uint8_t> (event::protocol_t::RAW):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
												break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
												break;
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
										// Для семейства UDPv6
										case static_cast <uint8_t> (event::family_t::UDPV6): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как RAW
											case static_cast <uint8_t> (event::protocol_t::RAW):
											// Если протокол определён как ICMP
											case static_cast <uint8_t> (event::protocol_t::ICMP):
											// Если протокол определён как IGMP
											case static_cast <uint8_t> (event::protocol_t::IGMP): break;
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_RAW, 0);
											break;
											// Если протокол определён как UDP
											case static_cast <uint8_t> (event::protocol_t::UDP):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_RAW, IPPROTO_UDP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"RAW socket type only supports UDP protocol or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
													), log_t::flag_t::WARNING
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									}
								} break;
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем текущее значение объекта сервера
									server_t * first = static_cast <server_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства UDPv4
										case static_cast <uint8_t> (event::family_t::UDPV4): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как RAW
												case static_cast <uint8_t> (event::protocol_t::RAW):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
												break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
												break;
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
										// Для семейства UDPv6
										case static_cast <uint8_t> (event::family_t::UDPV6): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как RAW
											case static_cast <uint8_t> (event::protocol_t::RAW):
											// Если протокол определён как ICMP
											case static_cast <uint8_t> (event::protocol_t::ICMP):
											// Если протокол определён как IGMP
											case static_cast <uint8_t> (event::protocol_t::IGMP): break;
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_RAW, 0);
											break;
											// Если протокол определён как UDP
											case static_cast <uint8_t> (event::protocol_t::UDP):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_RAW, IPPROTO_UDP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"RAW socket type only supports UDP protocol or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
													), log_t::flag_t::WARNING
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									}
								} break;
							}
							// Если всё прошло успешно
							if(ok)
								// Возвращаем идентификатор созданного события
								result = ret.first->first;
							// Если всё прошло не успешно
							else
								// Удаляем созданное событие
								::__awh_nodes__.erase(ret.first);
						} break;
						// Для типа сокета DATAGRAM
						case static_cast <uint8_t> (event::type_t::DATAGRAM): {
							// Выполняем создание события
							auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
							// Устанавливаем флаг режима сокета
							ret.first->second->state.mode = mode;
							// Устанавливаем флаг протокола сокета
							ret.first->second->state.protocol = protocol;
							// Устанавливаем флаг типа сокета
							ret.first->second->state.type = i->second->state.type;
							// Устанавливаем флаг семейства сокета
							ret.first->second->state.family = i->second->state.family;
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем текущее значение объекта клиента
									client_t * first = static_cast <client_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства UDPv4
										case static_cast <uint8_t> (event::family_t::UDPV4): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
												break;
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
										// Для семейства UDPv6
										case static_cast <uint8_t> (event::family_t::UDPV6): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как ICMP
											case static_cast <uint8_t> (event::protocol_t::ICMP):
											// Если протокол определён как IGMP
											case static_cast <uint8_t> (event::protocol_t::IGMP): break;
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, 0);
											break;
											// Если протокол определён как UDP
											case static_cast <uint8_t> (event::protocol_t::UDP):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_DGRAM, IPPROTO_UDP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
													), log_t::flag_t::WARNING
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									}
								} break;
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем текущее значение объекта сервера
									server_t * first = static_cast <server_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства UDPv4
										case static_cast <uint8_t> (event::family_t::UDPV4): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
												break;
												// Если протокол определён как IGMP
												case static_cast <uint8_t> (event::protocol_t::IGMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
										// Для семейства UDPv6
										case static_cast <uint8_t> (event::family_t::UDPV6): {
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
											/**
											 * Определяем протокол
											 */
											switch(static_cast <uint8_t> (protocol)){
												// Если протокол не определён
												case static_cast <uint8_t> (event::protocol_t::NONE):
												// Если протокол определён как UDP
												case static_cast <uint8_t> (event::protocol_t::UDP): break;
												// Если протокол определён как ICMP
												case static_cast <uint8_t> (event::protocol_t::ICMP):
													// Создаем сокет подключения
													second->host.fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
												break;
												// Если установлен другой протокол
												default: ok = false;
											}
										} break;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										/**
										 * Определяем протокол
										 */
										switch(static_cast <uint8_t> (protocol)){
											// Если протокол определён как ICMP
											case static_cast <uint8_t> (event::protocol_t::ICMP):
											// Если протокол определён как IGMP
											case static_cast <uint8_t> (event::protocol_t::IGMP): break;
											// Если протокол не определён
											case static_cast <uint8_t> (event::protocol_t::NONE):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, 0);
											break;
											// Если протокол определён как UDP
											case static_cast <uint8_t> (event::protocol_t::UDP):
												// Создаем сокет подключения
												second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_DGRAM, IPPROTO_UDP);
											break;
											// Если установлен другой протокол
											default: ok = false;
										}
										// Если всё хорошо, продолжаем работу
										if(ok){
											// Выполняем копирование объекта подключения клиента
											::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
											// Выполняем копирование объекта подключения сервера
											::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
										// Если протокол не определён
										} else {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug(
													"DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol",
													__PRETTY_FUNCTION__, std::make_tuple(
														id, static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
													), log_t::flag_t::WARNING
												);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
											#endif
										}
									}
								} break;
							}
							// Если всё прошло успешно
							if(ok)
								// Возвращаем идентификатор созданного события
								result = ret.first->first;
							// Если всё прошло не успешно
							else
								// Удаляем созданное событие
								::__awh_nodes__.erase(ret.first);
						} break;
						// Для неизвестного типа сокета
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"An event for a UDP socket cannot be created because it has an invalid initialization type",
									__PRETTY_FUNCTION__, std::make_tuple(
										id, static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("An event for a UDP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
							#endif
						}
					}
				} break;
				// Для семейства UNIX-доменных сокетов
				case static_cast <uint8_t> (event::family_t::UDS): {
					/**
					 * Определяем тип сокета
					 */
					switch(static_cast <uint8_t> (i->second->state.type)){
						// Для типа сокета STREAM
						case static_cast <uint8_t> (event::type_t::STREAM): {
							// Выполняем создание события
							auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
							// Устанавливаем флаг режима сокета
							ret.first->second->state.mode = mode;
							// Устанавливаем флаг протокола сокета
							ret.first->second->state.protocol = protocol;
							// Устанавливаем флаг типа сокета
							ret.first->second->state.type = i->second->state.type;
							// Устанавливаем флаг семейства сокета
							ret.first->second->state.family = i->second->state.family;
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем текущее значение объекта клиента
									client_t * first = static_cast <client_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									// Запоминаем размер структуры
									second->endpoint.size = sizeof(struct sockaddr_un);
									// Создаем сокет подключения
									second->host.fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
									// Выполняем копирование объекта подключения клиента
									::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
									// Выполняем копирование объекта подключения сервера
									::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
								} break;
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем текущее значение объекта сервера
									server_t * first = static_cast <server_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									// Запоминаем размер структуры
									second->endpoint.size = sizeof(struct sockaddr_un);
									// Создаем сокет подключения
									second->host.fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
									// Выполняем копирование объекта подключения клиента
									::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
									// Выполняем копирование объекта подключения сервера
									::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
								} break;
							}
							// Возвращаем идентификатор созданного события
							result = ret.first->first;
						} break;
						// Для типа сокета SEQPACKET
						case static_cast <uint8_t> (event::type_t::SEQPACKET): {
							// Выполняем создание события
							auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
							// Устанавливаем флаг режима сокета
							ret.first->second->state.mode = mode;
							// Устанавливаем флаг протокола сокета
							ret.first->second->state.protocol = protocol;
							// Устанавливаем флаг типа сокета
							ret.first->second->state.type = i->second->state.type;
							// Устанавливаем флаг семейства сокета
							ret.first->second->state.family = i->second->state.family;
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем текущее значение объекта клиента
									client_t * first = static_cast <client_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									// Запоминаем размер структуры
									second->endpoint.size = sizeof(struct sockaddr_un);
									// Создаем сокет подключения
									second->host.fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
									// Выполняем копирование объекта подключения клиента
									::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
									// Выполняем копирование объекта подключения сервера
									::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
								} break;
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем текущее значение объекта сервера
									server_t * first = static_cast <server_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									// Запоминаем размер структуры
									second->endpoint.size = sizeof(struct sockaddr_un);
									// Создаем сокет подключения
									second->host.fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
									// Выполняем копирование объекта подключения клиента
									::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
									// Выполняем копирование объекта подключения сервера
									::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
								} break;
							}
							// Возвращаем идентификатор созданного события
							result = ret.first->first;
						} break;
						// Для типа сокета DATAGRAM
						case static_cast <uint8_t> (event::type_t::DATAGRAM): {
							// Выполняем создание события
							auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
							// Устанавливаем флаг режима сокета
							ret.first->second->state.mode = mode;
							// Устанавливаем флаг протокола сокета
							ret.first->second->state.protocol = protocol;
							// Устанавливаем флаг типа сокета
							ret.first->second->state.type = i->second->state.type;
							// Устанавливаем флаг семейства сокета
							ret.first->second->state.family = i->second->state.family;
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем текущее значение объекта клиента
									client_t * first = static_cast <client_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									// Запоминаем размер структуры
									second->endpoint.size = sizeof(struct sockaddr_un);
									// Создаем сокет подключения
									second->host.fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
									// Выполняем копирование объекта подключения клиента
									::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
									// Выполняем копирование объекта подключения сервера
									::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
								} break;
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем текущее значение объекта сервера
									server_t * first = static_cast <server_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									// Запоминаем размер структуры
									second->endpoint.size = sizeof(struct sockaddr_un);
									// Создаем сокет подключения
									second->host.fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
									// Выполняем копирование объекта подключения клиента
									::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
									// Выполняем копирование объекта подключения сервера
									::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
								} break;
							}
							// Возвращаем идентификатор созданного события
							result = ret.first->first;
						} break;
						// Для неизвестного типа сокета
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"An event for a Unix socket cannot be created because it has an invalid initialization type",
									__PRETTY_FUNCTION__, std::make_tuple(
										id, static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("An event for a Unix socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
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
					switch(static_cast <uint8_t> (i->second->state.type)){
						// Для типа сокета STREAM
						case static_cast <uint8_t> (event::type_t::STREAM): {
							// Выполняем создание события
							auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
							// Устанавливаем флаг режима сокета
							ret.first->second->state.mode = mode;
							// Устанавливаем флаг протокола сокета
							ret.first->second->state.protocol = protocol;
							// Устанавливаем флаг типа сокета
							ret.first->second->state.type = i->second->state.type;
							// Устанавливаем флаг семейства сокета
							ret.first->second->state.family = i->second->state.family;
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем текущее значение объекта клиента
									client_t * first = static_cast <client_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства IPv4
										case static_cast <uint8_t> (event::family_t::IPV4):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
										break;
										// Для семейства IPv6
										case static_cast <uint8_t> (event::family_t::IPV6):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
										break;
									}
									/**
									 * Определяем протокол
									 */
									switch(static_cast <uint8_t> (protocol)){
										// Если протокол не определён
										case static_cast <uint8_t> (event::protocol_t::NONE):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, 0);
										break;
										// Если протокол определён как TCP
										case static_cast <uint8_t> (event::protocol_t::TCP):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_TCP);
										break;
										// Если протокол определён как SCTP
										case static_cast <uint8_t> (event::protocol_t::SCTP):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_STREAM, IPPROTO_SCTP);
										break;
										// Если установлен другой протокол
										default: ok = false;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									// Если протокол не определён
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													id, static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем текущее значение объекта сервера
									server_t * first = static_cast <server_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства IPv4
										case static_cast <uint8_t> (event::family_t::IPV4):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
										break;
										// Для семейства IPv6
										case static_cast <uint8_t> (event::family_t::IPV6):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
										break;
									}
									/**
									 * Определяем протокол
									 */
									switch(static_cast <uint8_t> (protocol)){
										// Если протокол не определён
										case static_cast <uint8_t> (event::protocol_t::NONE):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, 0);
										break;
										// Если протокол определён как TCP
										case static_cast <uint8_t> (event::protocol_t::TCP):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_TCP);
										break;
										// Если протокол определён как SCTP
										case static_cast <uint8_t> (event::protocol_t::SCTP):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_STREAM, IPPROTO_SCTP);
										break;
										// Если установлен другой протокол
										default: ok = false;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									// Если протокол не определён
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													id, static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
							}
							// Если всё прошло успешно
							if(ok)
								// Возвращаем идентификатор созданного события
								result = ret.first->first;
							// Если всё прошло не успешно
							else
								// Удаляем созданное событие
								::__awh_nodes__.erase(ret.first);
						} break;
						// Для типа сокета SEQPACKET
						case static_cast <uint8_t> (event::type_t::SEQPACKET): {
							// Выполняем создание события
							auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
							// Устанавливаем флаг режима сокета
							ret.first->second->state.mode = mode;
							// Устанавливаем флаг протокола сокета
							ret.first->second->state.protocol = protocol;
							// Устанавливаем флаг типа сокета
							ret.first->second->state.type = i->second->state.type;
							// Устанавливаем флаг семейства сокета
							ret.first->second->state.family = i->second->state.family;
							/**
							 * Определяем чем является текущая нода
							 */
							switch(static_cast <uint8_t> (i->second->state.node)){
								// Если нода является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Получаем текущее значение объекта клиента
									client_t * first = static_cast <client_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства IPv4
										case static_cast <uint8_t> (event::family_t::IPV4):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
										break;
										// Для семейства IPv6
										case static_cast <uint8_t> (event::family_t::IPV6):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
										break;
									}
									/**
									 * Определяем протокол
									 */
									switch(static_cast <uint8_t> (protocol)){
										// Если протокол определён как SCTP
										case static_cast <uint8_t> (event::protocol_t::SCTP):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.client.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
										break;
										// Если установлен другой протокол
										default: ok = false;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									// Если протокол не определён
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													id, static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
								// Если нода является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Получаем текущее значение объекта сервера
									server_t * first = static_cast <server_t *> (i->second.get());
									// Получаем текущее значение пира получающего параметры
									client_t * second = static_cast <client_t *> (ret.first->second.get());
									/**
									 * Определяем тип подключения
									 */
									switch(static_cast <uint8_t> (i->second->state.family)){
										// Для семейства IPv4
										case static_cast <uint8_t> (event::family_t::IPV4):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in);
										break;
										// Для семейства IPv6
										case static_cast <uint8_t> (event::family_t::IPV6):
											// Запоминаем размер структуры
											second->endpoint.size = sizeof(struct sockaddr_in6);
										break;
									}
									/**
									 * Определяем протокол
									 */
									switch(static_cast <uint8_t> (protocol)){
										// Если протокол определён как SCTP
										case static_cast <uint8_t> (event::protocol_t::SCTP):
											// Создаем сокет подключения
											second->host.fd = ::socket(first->endpoint.server.ss_family, SOCK_SEQPACKET, IPPROTO_SCTP);
										break;
										// Если установлен другой протокол
										default: ok = false;
									}
									// Если всё хорошо, продолжаем работу
									if(ok){
										// Выполняем копирование объекта подключения клиента
										::memcpy(&second->endpoint.client, &first->endpoint.client, sizeof(struct sockaddr_storage));
										// Выполняем копирование объекта подключения сервера
										::memcpy(&second->endpoint.server, &first->endpoint.server, sizeof(struct sockaddr_storage));
									// Если протокол не определён
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													id, static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
							}
							// Если всё прошло успешно
							if(ok)
								// Возвращаем идентификатор созданного события
								result = ret.first->first;
							// Если всё прошло не успешно
							else
								// Удаляем созданное событие
								::__awh_nodes__.erase(ret.first);
						} break;
						// Для неизвестного типа сокета
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug(
									"An event for a IP socket cannot be created because it has an invalid initialization type",
									__PRETTY_FUNCTION__, std::make_tuple(
										id, static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
									), log_t::flag_t::WARNING
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("An event for a IP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
							#endif
						}
					}
				} break;
				// Для семейства директорий
				case static_cast <uint8_t> (event::family_t::DIR):
				// Для семейства файловой системы
				case static_cast <uint8_t> (event::family_t::FILE): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <fs_t> ());
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = i->second->state.type;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = i->second->state.family;
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для семейства таймеров
				case static_cast <uint8_t> (event::family_t::TIMER):
				// Для семейства интервалов
				case static_cast <uint8_t> (event::family_t::INTERVAL): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <timer_t> ());
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = i->second->state.type;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = i->second->state.family;
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для неизвестного семейства
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Event cannot be created because the family it belongs to is not defined",
							__PRETTY_FUNCTION__, std::make_tuple(
								id, static_cast <uint16_t> (protocol),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Event cannot be created because the family it belongs to is not defined", log_t::flag_t::WARNING);
					#endif
				}
			}
		// Событие ещё не инициализированно
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"Event ID=%u has not yet been initialized",
					__PRETTY_FUNCTION__, std::make_tuple(
						id, static_cast <uint16_t> (protocol),
						static_cast <uint16_t> (mode)
					), log_t::flag_t::WARNING, id
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Event ID=%u has not yet been initialized", log_t::flag_t::WARNING, id);
			#endif
		}
	// Если событие не найдено
	} else {
		/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"Event ID=%u is not exist",
					__PRETTY_FUNCTION__, std::make_tuple(
						id, static_cast <uint16_t> (protocol),
						static_cast <uint16_t> (mode)
					), log_t::flag_t::WARNING, id
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Event ID=%u is not exist", log_t::flag_t::WARNING, id);
			#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод создания нового события
 *
 * @param family   семейство сокета
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @param mode     режим сокета
 * @return         идентификатор созданного события
 */
awh::event::id_t awh::IO::event(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Определяем семейство сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства UDPv4
		case static_cast <uint8_t> (event::family_t::UDPV4):
		// Для семейства UDPv6
		case static_cast <uint8_t> (event::family_t::UDPV6): {
			// Флаг удачного выполнения объединение событий
			bool ok = true;
			/**
			 * Определяем тип сокета
			 */
			switch(static_cast <uint8_t> (type)){
				// Для типа сокета RAW
				case static_cast <uint8_t> (event::type_t::RAW): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					/**
					 * Определяем тип подключения
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол не определён
								case static_cast <uint8_t> (event::protocol_t::NONE):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_RAW, 0);
								break;
								// Если протокол не определён
								case static_cast <uint8_t> (event::protocol_t::RAW):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
								break;
								// Если протокол определён как UDP
								case static_cast <uint8_t> (event::protocol_t::UDP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
								break;
								// Если протокол определён как IGMP
								case static_cast <uint8_t> (event::protocol_t::IGMP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
								break;
								// Если протокол определён как ICMP
								case static_cast <uint8_t> (event::protocol_t::ICMP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол не определён
								case static_cast <uint8_t> (event::protocol_t::NONE):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_RAW, 0);
								break;
								// Если протокол определён как UDP
								case static_cast <uint8_t> (event::protocol_t::UDP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_UDP);
								break;
								// Если протокол определён как ICMP
								case static_cast <uint8_t> (event::protocol_t::ICMP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
					}
					// Если всё прошло успешно
					if(ok)
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					// Если всё прошло не успешно
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"RAW socket type only supports UDP protocol or Unix family socket with empty protocol",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("RAW socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
						#endif
						// Удаляем созданное событие
						::__awh_nodes__.erase(ret.first);
					}
				} break;
				// Для типа сокета DATAGRAM
				case static_cast <uint8_t> (event::type_t::DATAGRAM): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					/**
					 * Определяем тип подключения
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства UDPv4
						case static_cast <uint8_t> (event::family_t::UDPV4): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол не определён
								case static_cast <uint8_t> (event::protocol_t::NONE):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_DGRAM, 0);
								break;
								// Если протокол определён как UDP
								case static_cast <uint8_t> (event::protocol_t::UDP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
								break;
								// Если протокол определён как IGMP
								case static_cast <uint8_t> (event::protocol_t::IGMP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IGMP);
								break;
								// Если протокол определён как ICMP
								case static_cast <uint8_t> (event::protocol_t::ICMP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
						// Для семейства UDPv6
						case static_cast <uint8_t> (event::family_t::UDPV6): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол не определён
								case static_cast <uint8_t> (event::protocol_t::NONE):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_DGRAM, 0);
								break;
								// Если протокол определён как UDP
								case static_cast <uint8_t> (event::protocol_t::UDP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
								break;
								// Если протокол определён как ICMP
								case static_cast <uint8_t> (event::protocol_t::ICMP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
					}
					// Если всё прошло успешно
					if(ok)
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					// Если всё прошло не успешно
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("DGRAM socket type only supports UDP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
						#endif
						// Удаляем созданное событие
						::__awh_nodes__.erase(ret.first);
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
							"An event for a UDP socket cannot be created because it has an invalid initialization type",
							__PRETTY_FUNCTION__, std::make_tuple(
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (type),
								static_cast <uint16_t> (protocol),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("An event for a UDP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
					#endif
				}
			}
		} break;
		// Для семейства UNIX-доменных сокетов
		case static_cast <uint8_t> (event::family_t::UDS): {
			/**
			 * Определяем тип сокета
			 */
			switch(static_cast <uint8_t> (type)){
				// Для типа сокета STREAM
				case static_cast <uint8_t> (event::type_t::STREAM): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Создаем сокет подключения
					static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для типа сокета SEQPACKET
				case static_cast <uint8_t> (event::type_t::SEQPACKET): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Создаем сокет подключения
					static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для типа сокета DATAGRAM
				case static_cast <uint8_t> (event::type_t::DATAGRAM): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					// Создаем сокет подключения
					static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
					// Возвращаем идентификатор созданного события
					result = ret.first->first;
				} break;
				// Для неизвестного типа сокета
				default: {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"An event for a Unix socket cannot be created because it has an invalid initialization type",
							__PRETTY_FUNCTION__, std::make_tuple(
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (type),
								static_cast <uint16_t> (protocol),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("An event for a Unix socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
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
				// Для типа сокета STREAM
				case static_cast <uint8_t> (event::type_t::STREAM): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					/**
					 * Определяем тип подключения
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол не определён
								case static_cast <uint8_t> (event::protocol_t::NONE):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_STREAM, 0);
								break;
								// Если протокол определён как TCP
								case static_cast <uint8_t> (event::protocol_t::TCP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
								break;
								// Если протокол определён как SCTP
								case static_cast <uint8_t> (event::protocol_t::SCTP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол не определён
								case static_cast <uint8_t> (event::protocol_t::NONE):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_STREAM, 0);
								break;
								// Если протокол определён как TCP
								case static_cast <uint8_t> (event::protocol_t::TCP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
								break;
								// Если протокол определён как SCTP
								case static_cast <uint8_t> (event::protocol_t::SCTP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
					}
					// Если всё прошло успешно
					if(ok)
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					// Если всё прошло не успешно
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("STREAM socket type only supports TCP or SCTP protocols or Unix family socket with empty protocol", log_t::flag_t::WARNING);
						#endif
						// Удаляем созданное событие
						::__awh_nodes__.erase(ret.first);
					}
				} break;
				// Для типа сокета SEQPACKET
				case static_cast <uint8_t> (event::type_t::SEQPACKET): {
					// Выполняем создание события
					auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
					// Устанавливаем флаг типа сокета
					ret.first->second->state.type = type;
					// Устанавливаем флаг режима сокета
					ret.first->second->state.mode = mode;
					// Устанавливаем флаг семейства сокета
					ret.first->second->state.family = family;
					// Устанавливаем флаг протокола сокета
					ret.first->second->state.protocol = protocol;
					/**
					 * Определяем тип подключения
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол определён как SCTP
								case static_cast <uint8_t> (event::protocol_t::SCTP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							/**
							 * Определяем протокол
							 */
							switch(static_cast <uint8_t> (protocol)){
								// Если протокол определён как SCTP
								case static_cast <uint8_t> (event::protocol_t::SCTP):
									// Создаем сокет подключения
									static_cast <client_t *> (ret.first->second.get())->host.fd = ::socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
								break;
								// Если установлен другой протокол
								default: ok = false;
							}
						} break;
					}
					// Если всё прошло успешно
					if(ok)
						// Возвращаем идентификатор созданного события
						result = ret.first->first;
					// Если всё прошло не успешно
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol", log_t::flag_t::WARNING);
						#endif
						// Удаляем созданное событие
						::__awh_nodes__.erase(ret.first);
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
							"An event for a IP socket cannot be created because it has an invalid initialization type",
							__PRETTY_FUNCTION__, std::make_tuple(
								static_cast <uint16_t> (family),
								static_cast <uint16_t> (type),
								static_cast <uint16_t> (protocol),
								static_cast <uint16_t> (mode)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("An event for a IP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
					#endif
				}
			}
		} break;
		// Для семейства директорий
		case static_cast <uint8_t> (event::family_t::DIR):
		// Для семейства файловой системы
		case static_cast <uint8_t> (event::family_t::FILE): {
			// Выполняем создание события
			auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <fs_t> ());
			// Устанавливаем флаг типа сокета
			ret.first->second->state.type = type;
			// Устанавливаем флаг режима сокета
			ret.first->second->state.mode = mode;
			// Устанавливаем флаг семейства сокета
			ret.first->second->state.family = family;
			// Устанавливаем флаг протокола сокета
			ret.first->second->state.protocol = protocol;
			// Возвращаем идентификатор созданного события
			result = ret.first->first;
		} break;
		// Для семейства таймеров
		case static_cast <uint8_t> (event::family_t::TIMER):
		// Для семейства интервалов
		case static_cast <uint8_t> (event::family_t::INTERVAL): {
			// Выполняем создание события
			auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <timer_t> ());
			// Устанавливаем флаг типа сокета
			ret.first->second->state.type = type;
			// Устанавливаем флаг режима сокета
			ret.first->second->state.mode = mode;
			// Устанавливаем флаг семейства сокета
			ret.first->second->state.family = family;
			// Устанавливаем флаг протокола сокета
			ret.first->second->state.protocol = protocol;
			// Возвращаем идентификатор созданного события
			result = ret.first->first;
		} break;
		// Для неизвестного семейства
		default: {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"Event cannot be created because the family it belongs to is not defined",
					__PRETTY_FUNCTION__, std::make_tuple(
						static_cast <uint16_t> (family),
						static_cast <uint16_t> (type),
						static_cast <uint16_t> (protocol),
						static_cast <uint16_t> (mode)
					), log_t::flag_t::WARNING
				);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Event cannot be created because the family it belongs to is not defined", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод получения пары событий для сокета
 *
 * @param family   семейство сокета
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @param mode     режим сокета
 * @return         пара идентификаторов созданных событий
 */
std::array <awh::event::id_t, 2> awh::IO::events(const event::family_t family, const event::type_t type, const event::protocol_t protocol, const event::mode_t mode) noexcept {
	// Результат работы функции
	std::array <awh::event::id_t, 2> result = {0,0};
	{
		// Список сокетов для инициализации
		int32_t fds[2] = {-1,-1};
		/**
		 * Определяем семейство сокета
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPC
			case static_cast <uint8_t> (event::family_t::IPC): {
				// Выполняем инициализацию файловых дескрипторов
				if(::pipe(fds) != 0){
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
								static_cast <uint16_t> (protocol),
								static_cast <uint16_t> (mode)
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
			// Для семейства UDPv4
			case static_cast <uint8_t> (event::family_t::UDPV4):
			// Для семейства UDPv6
			case static_cast <uint8_t> (event::family_t::UDPV6): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета RAW
					case static_cast <uint8_t> (event::type_t::RAW): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства UDPv4
							case static_cast <uint8_t> (event::family_t::UDPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, 0, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как RAW
									case static_cast <uint8_t> (event::protocol_t::RAW): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_RAW, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_UDP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_IGMP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_RAW, IPPROTO_ICMP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
								}
							} break;
							// Для семейства UDPv6
							case static_cast <uint8_t> (event::family_t::UDPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_RAW, 0, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_RAW, IPPROTO_UDP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
								}
							} break;
						}
					} break;
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						/**
						 * Определяем тип подключения
						 */
						switch(static_cast <uint8_t> (family)){
							// Для семейства UDPv4
							case static_cast <uint8_t> (event::family_t::UDPV4): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, 0, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, IPPROTO_UDP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как IGMP
									case static_cast <uint8_t> (event::protocol_t::IGMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, IPPROTO_IGMP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_DGRAM, IPPROTO_ICMP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
								}
							} break;
							// Для семейства UDPv6
							case static_cast <uint8_t> (event::family_t::UDPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_DGRAM, 0, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как ICMP
									case static_cast <uint8_t> (event::protocol_t::ICMP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
								}
							} break;
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
								"An event for a UDP socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a UDP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства UNIX-доменных сокетов
			case static_cast <uint8_t> (event::family_t::UDS): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета STREAM
					case static_cast <uint8_t> (event::type_t::STREAM): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0){
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
					// Для типа сокета SEQPACKET
					case static_cast <uint8_t> (event::type_t::SEQPACKET): {
						// Выполняем инициализацию файловых дескрипторов
						if(::socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) != 0){
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
										static_cast <uint16_t> (protocol),
										static_cast <uint16_t> (mode)
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
					// Для типа сокета DATAGRAM
					case static_cast <uint8_t> (event::type_t::DATAGRAM): {
						/**
						 * Определяем протокол
						 */
						switch(static_cast <uint8_t> (protocol)){
							// Если протокол не определён
							case static_cast <uint8_t> (event::protocol_t::NONE): {
								// Выполняем инициализацию файловых дескрипторов
								if(::socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) != 0){
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
												static_cast <uint16_t> (protocol),
												static_cast <uint16_t> (mode)
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
								"An event for a Unix socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a Unix socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				/**
				 * Определяем тип сокета
				 */
				switch(static_cast <uint8_t> (type)){
					// Для типа сокета STREAM
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
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_STREAM, 0, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_STREAM, IPPROTO_TCP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_STREAM, IPPROTO_SCTP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол не определён
									case static_cast <uint8_t> (event::protocol_t::NONE): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_STREAM, 0, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как TCP
									case static_cast <uint8_t> (event::protocol_t::TCP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_STREAM, IPPROTO_TCP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_STREAM, IPPROTO_SCTP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
								}
							} break;
						}
					} break;
					// Для типа сокета SEQPACKET
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
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если установлен другой протокол
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													static_cast <uint16_t> (family),
													static_cast <uint16_t> (type),
													static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
								}
							} break;
							// Для семейства IPv6
							case static_cast <uint8_t> (event::family_t::IPV6): {
								/**
								 * Определяем протокол
								 */
								switch(static_cast <uint8_t> (protocol)){
									// Если протокол определён как SCTP
									case static_cast <uint8_t> (event::protocol_t::SCTP): {
										// Выполняем инициализацию файловых дескрипторов
										if(::socketpair(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, fds) != 0){
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
														static_cast <uint16_t> (protocol),
														static_cast <uint16_t> (mode)
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
									// Если установлен другой протокол
									default: {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug(
												"SEQPACKET socket type only supports SCTP protocol or Unix family socket with empty protocol",
												__PRETTY_FUNCTION__, std::make_tuple(
													static_cast <uint16_t> (family),
													static_cast <uint16_t> (type),
													static_cast <uint16_t> (protocol),
													static_cast <uint16_t> (mode)
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
								}
							} break;
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
								"An event for a IP socket cannot be created because it has an invalid initialization type",
								__PRETTY_FUNCTION__, std::make_tuple(
									static_cast <uint16_t> (family),
									static_cast <uint16_t> (type),
									static_cast <uint16_t> (protocol),
									static_cast <uint16_t> (mode)
								), log_t::flag_t::WARNING
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("An event for a IP socket cannot be created because it has an invalid initialization type", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
		}
		// Если пара сокетов создана удачно
		if((fds[0] != -1) && (fds[1] != -1)){
			{
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг режима сокета
				ret.first->second->state.mode = mode;
				// Устанавливаем флаг семейства сокета
				ret.first->second->state.family = family;
				// Устанавливаем флаг протокола сокета
				ret.first->second->state.protocol = protocol;
				// Создаем сокет подключения
				static_cast <client_t *> (ret.first->second.get())->host.fd = fds[0];
				// Возвращаем идентификатор созданного события
				result[0] = ret.first->first;
			}{
				// Выполняем создание события
				auto ret = ::__awh_nodes__.emplace(::identifier(), make_unique <client_t> ());
				// Устанавливаем флаг типа сокета
				ret.first->second->state.type = type;
				// Устанавливаем флаг режима сокета
				ret.first->second->state.mode = mode;
				// Устанавливаем флаг семейства сокета
				ret.first->second->state.family = family;
				// Устанавливаем флаг протокола сокета
				ret.first->second->state.protocol = protocol;
				// Создаем сокет подключения
				static_cast <client_t *> (ret.first->second.get())->host.fd = fds[1];
				// Возвращаем идентификатор созданного события
				result[1] = ret.first->first;
			}
		}
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Метод получения режима действия события
 *
 * @param id     идентификатор события
 * @param action действие события
 * @return       режим действия события
 */
awh::event::notify_t awh::IO::action(const event::id_t id, const event::action_t action) noexcept {

	return event::notify_t::DISABLED;
}
/**
 * @brief Метод установки режима действия события
 *
 * @param id     идентификатор события
 * @param action действие события
 * @param notify уведомления события
 * @return       результат выполнения установки
 */
bool awh::IO::action(const event::id_t id, const event::action_t action, const event::notify_t notify) noexcept {

	return false;
}
/**
 * @brief Метод установки флага только IPv6 для события
 *
 * @param id     идентификатор события
 * @param enable флаг только IPv6
 * @return       результат выполнения установки
 */
bool awh::IO::onlyIPv6(const event::id_t id, const bool enable) noexcept {

	return false;
}
/**
 * @brief Метод установки опции события
 *
 * @param id     идентификатор события
 * @param option опция события
 * @param value  значение опции события
 * @return       результат выполнения установки
 */
bool awh::IO::option(const event::id_t id, const event::option_t option, const int32_t value) noexcept {
	
	return false;
}
/**
 * @brief Метод отключения события
 *
 * @param id идентификатор события
 * @return   результат выполнения отключения
 */
bool awh::IO::disconnect(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод подключения события к удалённому хосту
 *
 * @param id    идентификатор события
 * @param async флаг асинхронного подключения
 * @return      результат выполнения подключения
 */
bool awh::IO::connect(const event::id_t id, const bool async) noexcept {

	return false;
}
/**
 * @brief Метод принятия входящего соединения события
 *
 * @param id    идентификатор события
 * @param max   максимальное количество входящих соединений
 * @param async флаг асинхронного принятия соединения
 * @return      результат выполнения принятия соединения
 */
bool awh::IO::accept(const event::id_t id, const uint32_t max, const bool async) noexcept {

	return false;
}
/**
 * @brief Метод отправки события
 *
 * @param value значение события для отправки
 * @return      результат выполнения отправки
 */
bool awh::IO::post(const uint32_t value) noexcept {
	
	return false;
}
/**
 * @brief Метод отправки данных события
 *
 * @param id   идентификатор события
 * @param data указатель на данные для отправки
 * @param size размер данных для отправки
 * @return     результат выполнения отправки
 */
bool awh::IO::send(const event::id_t id, const char * data, const size_t size) noexcept {

	return false;
}
/**
 * @brief Метод очистки всех адресов сетей для выхода в интернет
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearNetworks(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод получения списка адресов сетей для выхода в интернет
 *
 * @param id идентификатор события
 * @return   список адресов сетей события
 */
std::unordered_set <string> awh::IO::networks(const event::id_t id) const noexcept {
	
	return {};
}
/**
 * @brief Метод добавления адреса сети для выхода в интернет
 *
 * @param id      идентификатор события
 * @param network адрес сети для добавления
 * @return        результат выполнения добавления
 */
bool awh::IO::addNetwork(const event::id_t id, const string & network) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления адреса сети для выхода в интернет
 *
 * @param id      идентификатор события
 * @param network адрес сети для удаления
 * @return        результат выполнения удаления
 */
bool awh::IO::removeNetwork(const event::id_t id, const string & network) noexcept {
	
	return false;
}
/**
 * @brief Метод добавления списка адресов сетей для выхода в интернет
 *
 * @param id       идентификатор события
 * @param networks список адресов сетей для добавления
 * @return         результат выполнения добавления
 */
bool awh::IO::addNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления списка адресов сетей для выхода в интернет
 *
 * @param id       идентификатор события
 * @param networks список адресов сетей для удаления
 * @return         результат выполнения удаления
 */
bool awh::IO::removeNetworks(const event::id_t id, const std::unordered_set <string> & networks) noexcept {

	return false;
}
/**
 * @brief Метод очистки всех сетевых интерфейсов события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearNetworkInterfaces(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод получения списка сетевых интерфейсов события
 *
 * @param id идентификатор события
 * @return   список сетевых интерфейсов события
 */
std::unordered_set <string> awh::IO::networkInterfaces(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод добавления сетевого интерфейса для события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для добавления
 * @return     результат выполнения добавления
 */
bool awh::IO::addNetworkInterface(const event::id_t id, const string & name) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления сетевого интерфейса для события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для удаления
 * @return     результат выполнения удаления
 */
bool awh::IO::removeNetworkInterface(const event::id_t id, const string & name) noexcept {
	
	return false;
}
/**
 * @brief Метод добавления списка сетевых интерфейсов для события
 *
 * @param id    идентификатор события
 * @param names список сетевых интерфейсов для добавления
 * @return      результат выполнения добавления
 */
bool awh::IO::addNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept {
	
	return false;
}
/**
 * @brief Метод удаления списка сетевых интерфейсов для события
 *
 * @param id    идентификатор события
 * @param names список сетевых интерфейсов для удаления
 * @return      результат выполнения удаления
 */
bool awh::IO::removeNetworkInterfaces(const event::id_t id, const std::unordered_set <string> & names) noexcept {
	
	return false;
}
/**
 * @brief Метод присоединения события к мультикаст группе
 *
 * @param id               идентификатор события
 * @param multicastAddress адрес мультикаст группы для присоединения
 * @return                 результат выполнения присоединения
 */
bool awh::IO::multicastJoin(const event::id_t id, const string & multicastAddress) noexcept {

	return false;
}
/**
 * @brief Метод выхода события из мультикаст группы
 *
 * @param id               идентификатор события
 * @param multicastAddress адрес мультикаст группы для выхода
 * @return                 результат выполнения выхода
 */
bool awh::IO::multicastLeave(const event::id_t id, const string & multicastAddress) noexcept {

	return false;
}
/**
 * @brief Метод очистки чёрного списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearBlacklist(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод добавления адреса в чёрный список события
 *
 * @param id      идентификатор события
 * @param address адрес для добавления в чёрный список
 * @return        результат выполнения добавления
 */
bool awh::IO::addToBlacklist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод удаления адреса из чёрного списка события
 *
 * @param id      идентификатор события
 * @param address адрес для удаления из чёрного списка
 * @return        результат выполнения удаления
 */
bool awh::IO::removeFromBlacklist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод получения чёрного списка события
 *
 * @param id идентификатор события
 * @return   чёрный список события
 */
std::unordered_map <awh::event::address_t, string> awh::IO::blacklist(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод очистки белого списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 */
bool awh::IO::clearWhitelist(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод добавления адреса в белый список события
 * @param id      идентификатор события
 * @param address адрес для добавления в белый список
 * @return        результат выполнения добавления
 */
bool awh::IO::addToWhitelist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод удаления адреса из белого списка события
 *
 * @param id      идентификатор события
 * @param address адрес для удаления из белого списка
 * @return        результат выполнения удаления
 */
bool awh::IO::removeFromWhitelist(const event::id_t id, const string & address) noexcept {

	return false;
}
/**
 * @brief Метод получения белого списка события
 *
 * @param id идентификатор события
 * @return   белый список события
 */
std::unordered_map <awh::event::address_t, string> awh::IO::whitelist(const event::id_t id) const noexcept {

	return {};
}
/**
 * @brief Метод установки таймаута на чтение события
 *
 * @param id      идентификатор события
 * @param timeout значение таймаута в миллисекундах
 */
void awh::IO::readTimeout(const event::id_t id, const uint32_t timeout) noexcept {

	
}
/**
 * @brief Метод установки таймаута на запись события
 *
 * @param id      идентификатор события
 * @param timeout значение таймаута в миллисекундах
 */
void awh::IO::writeTimeout(const event::id_t id, const uint32_t timeout) noexcept {

}
/**
 * @brief Метод установки глубины очереди принятия входящих соединений события
 *
 * @param id       идентификатор события
 * @param depth    глубина очереди принятия входящих соединений
 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
 */
void awh::IO::backlog(const event::id_t id, const uint32_t depth, const bool adaptive) noexcept {

}
/**
 * @brief Метод получения размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия с буфером
 * @return       размер буфера события
 */
size_t awh::IO::bufferSize(const event::id_t id, const event::action_t action) noexcept {

	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия с буфером
 * @param size   размер буфера события
 * @return       результат выполнения установки
 */
bool awh::IO::bufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept {

	return false;
}
/**
 * @brief Метод установки параметров keep-alive для события
 *
 * @param id    идентификатор события
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::IO::keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {

	return false;
}
/**
 * @brief Метод приостановки события
 *
 * @param id идентификатор события
 * @return   результат выполнения приостановки
 */
bool awh::IO::pause(const event::id_t id) noexcept {
	
	return false;
}
/**
 * @brief Метод возобновления события
 *
 * @param id идентификатор события
 * @return   результат выполнения возобновления
 */
bool awh::IO::resume(const event::id_t id) noexcept {

	return false;
}
/**
 * @brief Метод проверки состояния события
 *
 * @param id идентификатор события
 * @return   состояние события
 */
bool awh::IO::isAlive(const event::id_t id) const noexcept {

	return false;
}
/**
 * @brief Метод инициализации основного движка фреймворка
 *
 * @return результат выполнения инициализации
 */
bool awh::IO::initialize() noexcept {
	/**
	 * Выполняем настройку сетевых параметров
	 */
	::boostingNetwork(this->_fmk, this->_log);


	return false;
}
/**
 * @brief Метод деинициализации основного движка фреймворка
 *
 * @return результат выполнения деинициализации
 */
bool awh::IO::deinitialize() noexcept {

	return false;
}
/**
 * @brief Метод проверки состояния инициализации основного движка фреймворка
 *
 * @return состояние инициализации
 */
bool awh::IO::isInitialized() const noexcept {

	return false;
}
/**
 * @brief Метод получения режима события
 *
 * @param id идентификатор события
 * @return   режим события
 */
awh::event::mode_t awh::IO::mode(const event::id_t id) noexcept {
	
	return event::mode_t::NONE;
}
/**
 * @brief Метод получения узла события
 *
 * @param id идентификатор события
 * @return   узел события
 */
awh::event::node_t awh::IO::node(const event::id_t id) noexcept {

	return event::node_t::NONE;
}
/**
 * @brief Метод получения типа события
 *
 * @param id идентификатор события
 * @return   тип события
 */
awh::event::type_t awh::IO::type(const event::id_t id) noexcept {

	return event::type_t::NONE;
}
/**
 * @brief Метод получения семейства события
 *
 * @param id идентификатор события
 * @return   семейство события
 */
awh::event::family_t awh::IO::family(const event::id_t id) noexcept {

	return event::family_t::NONE;
}
/**
 * @brief Метод получения статуса события
 *
 * @param id идентификатор события
 * @return   статус события
 */
awh::event::status_t awh::IO::status(const event::id_t id) noexcept {

	return event::status_t::NONE;
}
/**
 * @brief Методы установки функции обратного вызова на чтение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const readCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на запись события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const writeCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на ошибку события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const errorCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на изменение статуса события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const statusCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на принятие события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const acceptCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на подключение события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const connectCallback & cb) noexcept {

}
/**
 * @brief Методы установки функции обратного вызова на получение пользовательского события
 *
 * @param id идентификатор события
 * @param cb объект обратного вызова события
 */
void awh::IO::on(const event::id_t id, const userEventCallback & cb) noexcept {

}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 */
awh::IO::IO(const fmk_t * fmk, const log_t * log) noexcept : engine_t(fmk, log) {

}
/**
 * @brief Деструктор
 *
 */
awh::IO::~IO() noexcept {

}
