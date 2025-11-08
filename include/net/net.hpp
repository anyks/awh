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
#include "../sys/queue.hpp"

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
			// Размер очереди ожидания подключения
			uint16_t depth;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Backlog() noexcept : adaptive(false), depth(SOMAXCONN) {}
		} __attribute__((packed)) backlog_t;
		/**
		 * @brief Структура состояния события
		 *
		 */
		typedef struct State {
			uint16_t options;                     // Флаги опций события
			event::node_t node;                   // Флаг узла события
			event::type_t type;                   // Флаг типа события
			event::family_t family;               // Флаг семейства события
			event::address_t address;             // Флаг адреса события
			event::protocol_t protocol;           // Флаг протокола события
			std::atomic <event::status_t> status; // Флаг статуса события
			/**
			 * @brief Оператор присваивания
			 *
			 * @param state объект состояния события
			 * @return      ссылка на объект состояния события
			 */
			State & operator = (const State & state) noexcept {
				// Проверяем на самоприсваивание
				if(this != &state){
					/**
					 * Выполняем копирование полей состояния события
					 */
					this->options  = state.options;
					this->node     = state.node;
					this->type     = state.type;
					this->family   = state.family;
					this->address  = state.address;
					this->protocol = state.protocol;
					this->status.store(state.status.load());
				}
				// Возвращаем ссылку на объект состояния события
				return (* this);
			}
			/**
			 * @brief Конструктор
			 *
			 */
			explicit State() noexcept :
			 options(event::options::NONE),
			 node(event::node_t::NONE),
			 type(event::type_t::NONE),
			 family(event::family_t::NONE),
			 address(event::address_t::NONE),
			 protocol(event::protocol_t::NONE),
			 status(event::status_t::NONE) {}
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
			 * @brief Конструктор
			 *
			 */
			virtual ~Callbacks() = default;
		} callbacks_t;
		/**
		 * @brief Структура обратных вызовов сервера
		 *
		 */
		typedef struct ServerCallbacks : public callbacks_t {
			// Обратный вызов при принятии события
			event::callback::accept_t accept;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit ServerCallbacks() noexcept : accept(nullptr) {}
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
			// Обратный вызов при подключении события
			event::callback::connect_t connect;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit ClientCallbacks() noexcept :
			 read(nullptr), write(nullptr), connect(nullptr) {}
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
			/**
			 * @brief Конструктор
			 *
			 */
			explicit PeerCallbacks() noexcept : read(nullptr), write(nullptr) {}
		} peer_callbacks_t;
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
			explicit Endpoint() noexcept : size(0), client{0}, server{0} {}
		} endpoint_t;
		/**
		 * @brief Структура буфера данных
		 *
		 */
		typedef struct Buffer {
			// Размер буфера данных
			size_t size;
			// Указатель на буфер данных
			unique_ptr <uint8_t []> data;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Buffer() noexcept : size(0), data(nullptr) {}
		} buffer_t;
		/**
		 * @brief Структура передачи данных
		 *
		 */
		typedef struct Transfer {
			size_t offset;  // Смещение передачи данных
			buffer_t input; // Буфер получения данных
			queue_t output; // Очередь отправки данных
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Transfer(const fmk_t * fmk, const log_t * log) noexcept :
			 offset(0), output(fmk, log) {}
		} transfer_t;
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
		 * @brief Структура  пользовательского события
		 *
		 */
		typedef struct User : public node_t {
			// Обратный вызов при принятии события
			event::callback::user_t callback;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit User() noexcept : callback(nullptr) {}
		} user_t;
		/**
		 * @brief Структура файловой системы
		 *
		 */
		typedef struct Filesystem : public node_t {
			// Файловый дескриптор
			socket_t fd;
			// Объект передачи данных
			transfer_t transfer;
			// Путь к файлу, каталогу или сокету
			unique_ptr <addr_t> path;
			// Обратные вызовы события
			peer_callbacks_t callbacks;
			// Чёрный список адресов которым запрещён доступ
			unordered_map <string, event::address_t> blacklist;
			// Белый список адресов которым разрешён доступ
			unordered_map <string, event::address_t> whitelist;
			// Активные таймауты события
			unordered_map <awh::event::action_t, uint16_t> timeouts;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Filesystem(const fmk_t * fmk, const log_t * log) noexcept :
			 fd(invalid_socket_t), transfer(fmk, log), path(nullptr) {}
		} fs_t;
		/**
		 * @brief Структура подключённого клиента
		 *
		 */
		typedef struct Peer : public node_t {
			// Файловый дескриптор сервиса
			socket_t socket;
			// Объект передачи данных
			transfer_t transfer;
			// MAC-адрес сетевого интерфейса
			unique_ptr <addr_t> mac;
			// Хост подключения события
			unique_ptr <attr_t> remote;
			// Обратные вызовы события
			peer_callbacks_t callbacks;
			// Активные таймауты события
			unordered_map <awh::event::action_t, uint16_t> timeouts;
			// Активные действия события
			unordered_map <awh::event::action_t, awh::event::notify_t> actions;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Peer(const fmk_t * fmk, const log_t * log) noexcept :
			 socket(invalid_socket_t), transfer(fmk, log), mac(nullptr), remote(nullptr) {}
		} peer_t;
		/**
		 * @brief Структура клиента
		 *
		 */
		typedef struct Client : public node_t {
			// Файловый дескриптор сервиса
			socket_t socket;
			// Объект параметров конечной точки
			endpoint_t endpoint;
			// Объект передачи данных
			transfer_t transfer;
			// Источник сетевых адресов
			unique_ptr <addr_t> source;
			// Целевые параметры подключения
			unique_ptr <attr_t> target;
			// Обратные вызовы события
			client_callbacks_t callbacks;
			// Активные таймауты события
			unordered_map <awh::event::action_t, uint16_t> timeouts;
			// Активные действия события
			unordered_map <awh::event::action_t, awh::event::notify_t> actions;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Client(const fmk_t * fmk, const log_t * log) noexcept :
			 socket(invalid_socket_t), transfer(fmk, log), source(nullptr), target(nullptr) {}
		} client_t;
		/**
		 * @brief Структура сервера
		 *
		 */
		typedef struct Server : public node_t {
			// Файловый дескриптор сервиса
			socket_t socket;
			// Размер очереди ожидания подключения
			backlog_t backlog;
			// Объект параметров конечной точки
			endpoint_t endpoint;
			// Объект передачи данных
			transfer_t transfer;
			// Параметры хоста сервера
			unique_ptr <attr_t> host;
			// Обратные вызовы события
			server_callbacks_t callbacks;
			// Чёрный список пиров которым запрещён доступ
			unordered_map <string, event::address_t> blacklist;
			// Белый список пиров которым разрешён доступ
			unordered_map <string, event::address_t> whitelist;
			// Активные таймауты события
			unordered_map <awh::event::action_t, uint16_t> timeouts;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit Server(const fmk_t * fmk, const log_t * log) noexcept :
			 socket(invalid_socket_t), transfer(fmk, log) {}
		} server_t;
	};
};

#endif // __AWH_NETWORK__
