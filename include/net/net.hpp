/**
 * @file: net.hpp
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

#ifndef __AWH_NETWORK__
#define __AWH_NETWORK__

/**
 * Стандартные модули
 */
#include <array>
#include <atomic>
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
	 * @brief Пространство имён для работы с сетью
	 *
	 */
	namespace net {
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
		/**
		 * Режимы установки типа сокета
		 */
		enum class socket_mode_t : uint8_t {
			ENABLED  = 0x01, // Включено
			DISABLED = 0x02  // Выключено
		};
		/**
		 * События сокета
		 */
		enum class socket_event_t : uint8_t {
			READ  = 0x01, // Чтение
			WRITE = 0x02  // Запись
		};
		/**
		 * @brief Структура метаданных сообщения SCTP
		 *
		 */
		typedef struct SctpMessageInfo {
			uint16_t num;             // Номер потока
			uint32_t ttl;             // Время жизни (в миллисекундах)
			uint32_t ctx;             // Контекст для уведомлений об ошибках
			uint32_t flags;           // Флаги сообщения
			event::sctp::ppid_t ppid; // Идентификатор полезной нагрузки
			/**
			 * @brief Конструктор
			 *
			 */
			explicit SctpMessageInfo() noexcept :
			 num(0), ttl(0), ctx(0), flags(0),
			 ppid(event::sctp::ppid_t::DTLS) {}
		} sctp_minfo_t;
		/**
		 * @brief Структура параметров рукопожатия SCTP
		 *
		 */
		typedef struct SctpHandshake {
			// Максимальное количество исходящих потоков
			uint16_t ostreams;
			// Максимальное количество входящих потоков
			uint16_t instreams;
			// Максимальное количество попыток подключения
			uint16_t attempts;
			// Максимальное время инициализации SCTP
			uint16_t initTimeout;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit SctpHandshake() noexcept :
				ostreams(5), instreams(5),
				attempts(4), initTimeout(0) {}
		} sctp_handshake_t;
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
		} addr_t;
		/**
		 * @brief Структура MAC-адреса
		 *
		 */
		typedef struct AddressMAC : public addr_t {
			// Буфер MAC-адреса
			array <uint8_t, 6> address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit AddressMAC() noexcept : addr_t(6), address{0} {}
		} addr_mac_t;
		/**
		 * @brief Структура сетевого адреса
		 *
		 */
		typedef struct AddressNetwork : public addr_t {
			// Префикс сети
			uint8_t prefix;
			/**
			 * @brief Конструктор
			 *
			 * @param prefix префикс сети
			 * @param size   размер адреса
			 */
			explicit AddressNetwork(const uint8_t prefix, const uint16_t size) noexcept :
			 addr_t(size), prefix(prefix) {}
		} addr_net_t;
		/**
		 * @brief Структура IPv4 сетевого адреса
		 *
		 */
		typedef struct AddressNetworkIPv4 : public addr_net_t {
			// IP-адрес сети
			uint32_t address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit AddressNetworkIPv4() noexcept : addr_net_t(32, 4), address(0) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~AddressNetworkIPv4() noexcept = default;
		} addr_net_ipv4_t;
		/**
		 * @brief Структура IPv6 сетевого адреса
		 *
		 */
		typedef struct AddressNetworkIPv6 : public addr_net_t {
			// Буфер IP-адрес сети
			array <uint8_t, 16> address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit AddressNetworkIPv6() noexcept : addr_net_t(128, 16), address{0} {}
		} addr_net_ipv6_t;
		/**
		 * @brief Структура адреса файловой системы
		 *
		 */
		typedef struct AddressFilesystem : public addr_t {
			// Путь к файлу, каталогу или сокету
			string address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit AddressFilesystem() noexcept : address{""} {}
		} addr_fs_t;
		/**
		 * @brief Структура сетевых адресов текущей машины
		 *
		 */
		typedef struct Source {
			// Название сетвого интерфейса
			string iface;
			// IP-адрес сети
			unique_ptr <addr_t> ip;
			// MAC-адрес сети
			unique_ptr <addr_t> mac;
			/**
			 * @brief Конструктор
			 *
			 * @param ip адрес сетевого подключения
			 */
			explicit Source(unique_ptr <addr_t> ip) noexcept :
			 iface{""}, ip(std::move(ip)), mac(make_unique <addr_mac_t> ()) {}
		} src_t;
		/**
		 * @brief Структура атрибутов подключения
		 *
		 */
		typedef struct Attributes {
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Attributes() noexcept = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Attributes() noexcept = default;
		} attr_t;
		/**
		 * @brief Структура IP-адреса подключения
		 *
		 */
		typedef struct AttributesNet : public attr_t {
			// Порт хоста
			uint16_t port;
			// IP-адрес хоста
			unique_ptr <addr_t> ip;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit AttributesNet() noexcept :
			 port(0), ip(make_unique <addr_net_ipv4_t> ()) {}
		} attr_net_t;
		/**
		 * @brief Структура UDS-адреса подключения
		 *
		 */
		typedef struct AttributesUDS : public attr_t {
			// Путь к сокету
			unique_ptr <addr_t> path;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit AttributesUDS() noexcept :
			 path(make_unique <addr_fs_t> ()) {}
		} attr_uds_t;
		/**
		 * @brief Структура очереди ожидания подключения
		 *
		 */
		typedef struct Backlog {
			// Адаптивный режим очереди ожидания подключения
			bool adaptive;
			// Максимальное количество подключений
			uint16_t max;
			// Количество уже подключённых клиентов
			uint16_t count;
			// Размер очереди ожидания подключения
			uint16_t depth;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Backlog() noexcept :
			 adaptive(false), max(100), count(0), depth(SOMAXCONN) {}
		} __attribute__((packed)) backlog_t;
		/**
		 * @brief Структура состояния события
		 *
		 */
		typedef struct State {
			uint16_t options;                // Флаги опций события
			event::node_t node;              // Флаг узла события
			event::hops_t hops;              // Флаг хопов события
			event::type_t type;              // Флаг типа события
			event::family_t family;          // Флаг семейства события
			event::address_t address;        // Флаг адреса события
			event::protocol_t protocol;      // Флаг протокола события
			event::delivery_mode_t delivery; // Флаг типа режима доставки события
			atomic <event::status_t> status; // Флаг статуса события
			atomic <event::status_t> oldset; // Флаг старого статуса события
			/**
			 * @brief Конструктор
			 *
			 */
			explicit State() noexcept :
			 options(event::options::NONE),
			 node(event::node_t::NONE),
			 hops(event::hops_t::WORLD),
			 type(event::type_t::NONE),
			 family(event::family_t::NONE),
			 address(event::address_t::NONE),
			 protocol(event::protocol_t::NONE),
			 delivery(event::delivery_mode_t::UNICAST),
			 status(event::status_t::NONE),
			 oldset(event::status_t::NONE) {}
		} __attribute__((packed)) state_t;
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
			 * @brief Деструктор
			 *
			 */
			virtual ~Callbacks() = default;
		} callbacks_t;
		/**
		 * @brief Структура обратных вызовов файловой системы
		 *
		 */
		typedef struct FileSystemCallbacks : public callbacks_t {
			// Обратный вызов при чтении события
			event::callback::read_t read;
			// Обратный вызов при записи события
			event::callback::write_t write;
			// Обратный вызов при получении общего события
			event::callback::event_t event;
			// Обратный вызов при изменении события
			event::callback::change_t change;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit FileSystemCallbacks() noexcept :
			 read(nullptr), write(nullptr),
			 event(nullptr), change(nullptr) {}
		} fs_callbacks_t;
		/**
		 * @brief Структура обратных вызовов сервера
		 *
		 */
		typedef struct ServerCallbacks : public callbacks_t {
			// Обратный вызов при записи события
			event::callback::write_t write;
			// Обратный вызов при получении общего события
			event::callback::event_t event;
			// Обратный вызов при принятии события
			event::callback::accept_t accept;
			// Обратный вызов при принятии первых событий однорангового узла-источника
			event::callback::origin_t origin;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit ServerCallbacks() noexcept :
			 write(nullptr), event(nullptr),
			 accept(nullptr), origin(nullptr) {}
		} server_callbacks_t;
		/**
		 * @brief Структура обратных вызовов клиента
		 *
		 */
		typedef struct ClientCallbacks : public callbacks_t {
			// Обратный вызов при чтении события
			event::callback::read_t read;
			// Обратный вызов при записи события
			event::callback::write_t write;
			// Обратный вызов при получении общего события
			event::callback::event_t event;
			// Обратный вызов при подключении события
			event::callback::connect_t connect;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit ClientCallbacks() noexcept :
			 read(nullptr), write(nullptr),
			 event(nullptr), connect(nullptr) {}
		} client_callbacks_t;
		/**
		 * @brief Структура обратных вызовов подключённого клиента
		 *
		 */
		typedef struct PeerCallbacks : public callbacks_t {
			// Обратный вызов при чтении события
			event::callback::read_t read;
			// Обратный вызов при записи события
			event::callback::write_t write;
			// Обратный вызов при получении общего события
			event::callback::event_t event;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit PeerCallbacks() noexcept :
			 read(nullptr), write(nullptr), event(nullptr) {}
		} peer_callbacks_t;
		/**
		 * @brief Структура узла события
		 *
		 */
		typedef struct Node {
			// Идентификатор события
			event::id_t id;
			// Состояние события
			state_t state;
			// Счётчик ссылок на событие
			atomic_uint16_t refs;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Node() noexcept : id(0), refs(0) {}
			/**
			 * @brief Деструктор
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
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Timer() = default;
		} timer_t;
		/**
		 * @brief Структура  пользовательского события
		 *
		 */
		typedef struct User : public node_t {
			// Обратные вызовы события
			peer_callbacks_t callbacks;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit User() noexcept = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~User() = default;
		} user_t;
		/**
		 * @brief Структура файловой системы
		 *
		 */
		typedef struct FileSystem : public node_t {
			// Путь к файлу, каталогу или сокету
			unique_ptr <addr_t> path;
			// Обратные вызовы события
			fs_callbacks_t callbacks;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit FileSystem() noexcept : path(nullptr) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~FileSystem() = default;
		} fs_t;
		/**
		 * @brief Структура межпроцессного взаимодействия
		 *
		 */
		typedef struct InterProcessCommunication : public node_t {
			// Обратные вызовы события
			peer_callbacks_t callbacks;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit InterProcessCommunication() = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~InterProcessCommunication() = default;
		} ipc_t;
		/**
		 * @brief Структура подключённого клиента
		 *
		 */
		typedef struct Peer : public node_t {
			// MAC-адрес сетевого интерфейса
			unique_ptr <addr_t> mac;
			// Хост подключения события
			unique_ptr <attr_t> remote;
			// Обратные вызовы события
			peer_callbacks_t callbacks;
			// Активные таймауты события
			unordered_map <event::action_t, uint32_t> timeouts;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Peer() noexcept : mac(nullptr), remote(nullptr) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Peer() = default;
		} peer_t;
		/**
		 * @brief Структура клиента
		 *
		 */
		typedef struct Client : public node_t {
			// Источник сетевых адресов
			unique_ptr <addr_t> source;
			// Целевые параметры подключения
			unique_ptr <attr_t> target;
			// Обратные вызовы события
			client_callbacks_t callbacks;
			// Активные таймауты события
			unordered_map <event::action_t, uint32_t> timeouts;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Client() noexcept : source(nullptr), target(nullptr) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Client() = default;
		} client_t;
		/**
		 * @brief Структура сервера
		 *
		 */
		typedef struct Server : public node_t {
			// Размер очереди ожидания подключения
			backlog_t backlog;
			// Параметры хоста сервера
			unique_ptr <attr_t> host;
			// Обратные вызовы события
			server_callbacks_t callbacks;
			// Чёрный список пиров которым запрещён доступ
			unordered_map <string, event::address_t> blacklist;
			// Белый список пиров которым разрешён доступ
			unordered_map <string, event::address_t> whitelist;
			// Активные таймауты события
			unordered_map <event::action_t, uint32_t> timeouts;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Server() = default;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Server() = default;
		} server_t;
	};
};

#endif // __AWH_NETWORK__
