/**
 * @file: sys.hpp
 * @date: 2025-10-30
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

#ifndef __AWH_NETWORK__
#define __AWH_NETWORK__

/**
 * Стандартные модули
 */
#include <string>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Системные модули
	 */
	#include <Ws2def.h>
	#include <winsock2.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Системные модули
	 */
	#include <sys/socket.h>
#endif

/**
 * Наши модули
 */
#include "event.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс для работы с системными ресурсами
	 */
	typedef class AWH_SHARED_EXPORT System {
		public:
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				/**
				 * @brief Тип сокета
				 *
				 */
				using socket_t = SOCKET;
				/**
				 * @brief Некорректный сокет
				 *
				 */
				static constexpr socket_t invalid_socket_t = INVALID_SOCKET;
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				/**
				 * @brief Тип сокета
				 *
				 */
				using socket_t = int32_t;
				/**
				 * @brief Некорректный сокет
				 *
				 */
				static constexpr socket_t invalid_socket_t = -1;
			#endif
		public:
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
				 * @param size размер адреса
				 */
				explicit Address(const uint16_t size = 0) noexcept : size(size) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Address() noexcept = default;
			} address_t;
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
				explicit AddressIPv4() noexcept : address_t(4), address(0) {}
			} address_ipv4_t;
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
				explicit AddressIPv6() noexcept : address_t(16), address{0} {}
			} address_ipv6_t;
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
				explicit AddressMAC() noexcept : address_t(6), address{0} {}
			} address_mac_t;
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
				 * @param prefix префикс сети
				 * @param size   размер адреса
				 */
				explicit AddressNetwork(const uint8_t prefix, const uint16_t size) noexcept :
				 address_t(size), prefix(prefix) {}
			} address_network_t;
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
				explicit AddressNetworkIPv4() noexcept : address_network_t(32, 4), address(0) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~AddressNetworkIPv4() noexcept = default;
			} address_network_ipv4_t;
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
				explicit AddressNetworkIPv6() noexcept : address_network_t(128, 16), address{0} {}
			} address_network_ipv6_t;
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
		public:
			/**
			 * @brief Структура сетевых адресов
			 *
			 */
			typedef struct Addresses {
				// Название сетвого интерфейса
				string iface;
				// MAC-адрес сети
				address_t mac;
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
				explicit Addresses(
					address_t host      = address_t{},
					address_t broadcast = address_t{},
					address_t multicast = address_t{}
				) noexcept :
				 iface{""},
				 mac(address_mac_t{}), host(host),
				 broadcast(broadcast), multicast(multicast) {}
			} addresses_t;
		public:
			/**
			 * @brief Структура хоста
			 *
			 */
			typedef struct Host {
				// Сокет хоста
				socket_t fd;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Host() noexcept : fd(invalid_socket_t) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Host() noexcept = default;
			} host_t;
			/**
			 * @brief Структура IP-хоста
			 *
			 */
			typedef struct HostIP : public host_t {
				// Порт хоста
				uint16_t port;
				// IP-адрес хоста
				address_t ip;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit HostIP() noexcept : port(0) {}
			} host_ip_t;
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
		public:
			/**
			 * @brief Структура состояния события
			 *
			 */
			typedef struct State {
				bool onlyIPv6;              // Флаг активации только IPv6
				event::mode_t mode;         // Флаг режима события
				event::node_t node;         // Флаг узла события
				event::type_t type;         // Флаг типа события
				event::family_t family;     // Флаг семейства события
				event::status_t status;     // Флаг статуса события
				event::address_t address;   // Флаг адреса события
				event::protocol_t protocol; // Флаг протокола события
				/**
				 * @brief Конструктор
				 *
				 */
				explicit State() noexcept :
				 onlyIPv6(false),
				 mode(event::mode_t::NONE),
				 node(event::node_t::NONE),
				 type(event::type_t::NONE),
				 family(event::family_t::NONE),
				 status(event::status_t::NONE),
				 address(event::address_t::NONE),
				 protocol(event::protocol_t::NONE) {}
			} state_t;
		public:
			/**
			 * @brief Структура обратных вызовов события
			 *
			 */
			typedef struct Callbacks {
				// Обратный вызов при ошибке события
				event::callback::error_t error;
				// Обратный вызов при изменении статуса события
				event::callback::status_t status;
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
				event::callback::accept_t accept;
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
				event::callback::read_t read;
				// Обратный вызов при записи события
				event::callback::write_t write;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit CallbacksClient() noexcept : read(nullptr), write(nullptr) {}
			} callbacks_client_t;
		public:
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
		public:
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
				uint16_t delay;
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
				uint16_t backlog;
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
				unordered_map <awh::event::action_t, uint16_t> timeouts;
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
				unordered_map <awh::event::action_t, uint16_t> timeouts;
				// Активные действия события
				unordered_map <awh::event::action_t, awh::event::notify_t> actions;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Peer() noexcept {}
			} peer_t;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод применения сетевой оптимизации операционной системы
			 *
			 */
			void boostingNetwork() const noexcept;
		public:
			/**
			 * @brief Метод поиска сетевых адресов соседа
			 *
			 * @param addresses структура сетевых адресов
			 */
			void peerAddresses(addresses_t & addresses) const noexcept;
			/**
			 * @brief Метод поиска сетевых адресов узла
			 *
			 * @param addresses структура сетевых адресов
			 */
			void nodeAddresses(addresses_t & addresses) const noexcept;
		public:
			/**
			 * @brief Метод установки значений адресов по умолчанию
			 *
			 * @param addresses структура сетевых адресов
			 */
			void defaultAddress(addresses_t & addresses) const noexcept;
		public:
			/**
			 * @brief Метод получения имени сетевого интерфейса по IP-адресу
			 *
			 * @param ip IP-адрес
			 * @return   имя сетевого интерфейса
			 */
			string interfaceName(const address_t & ip) const noexcept;
			/**
			 * @brief Метод получения имени сетевого интерфейса по MAC-адресу
			 *
			 * @param mac MAC-адрес
			 * @return    имя сетевого интерфейса
			 */
			string interfaceName(const address_mac_t & mac) const noexcept;
		public:
			/**
			 * @brief Метод поиска MAC-адреса по имени сетевого интерфейса
			 *
			 * @param iface     имя сетевого интерфейса
			 * @param addresses структура сетевых адресов
			 */
			void macAddress(const char * iface, addresses_t & addresses) const noexcept;
		public:
			/**
			 * @brief Метод поиска сетевых адресов по заданной сети
			 *
			 * @param net       сетевой адрес подсети в хостовом порядке
			 * @param addresses структура сетевых адресов
			 */
			void addresses(const address_t & net, addresses_t & addresses) const noexcept;
		public:
			/**
			 * @brief Метод проверки принадлежности IP-адреса подсети
			 *
			 * @param ip     проверяемый IP-адрес в хостовом порядке
			 * @param net    сетевой адрес подсети в хостовом порядке
			 * @param prefix префикс подсети
			 * @return       результат проверки
			 */
			bool isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод сравнения двух IPv6-адресов по префиксу (в битах)
			 *
			 * @param a      Первый IPv6-адрес
			 * @param b      Второй IPv6-адрес
			 * @param length Длина префикса в битах
			 * @return       Результат сравнения
			 */
			bool ipv6PrefixEqual(const uint8_t * a, const uint8_t * b, const uint8_t length) const noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit System(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~System() noexcept;
	} sys_t;
};

#endif // __AWH_NETWORK__
