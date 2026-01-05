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
			/**
			 * @brief Конструктор
			 *
			 */
			explicit ServerCallbacks() noexcept :
			 write(nullptr), event(nullptr), accept(nullptr) {}
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
		/**
		 * @brief Пространство имён для работы с SCTP
		 *
		 */
		namespace sctp {
			/**
			 * Идентификатор полезной нагрузки SCTP
			 */
			enum class ppid_t : uint8_t {
				DTLS       = 0x32, // (RFC 6083) DTLS поверх SCTP
				WEBRTC_STR = 0x33, // Строковые данные канала WebRTC
				WEBRTC_BIN = 0x35  // Бинарные данные канала WebRTC
			};
			/**
			 * Типы индикаторов события аутентификации SCTP
			 */
			enum class auth_indics_t : uint8_t {
				NONE     = 0x00, // Тип аутентификации отсутствует
				NEW_KEY  = 0x01, // Событие нового ключа
				NO_AUTH  = 0x02, // Событие отсутствия аутентификации
				FREE_KEY = 0x03  // Событие освобождения ключа
			};
			/**
			 * Флаги отправки сообщения SCTP
			 */
			enum class send_failed_t : uint8_t {
				NONE   = 0x00, // Флаг отсутствует
				SENT   = 0x01, // Сообщение отправлено
				UNSENT = 0x02  // Сообщение не отправлено
			};
			/**
			 * Индикаторы доставки SCTP
			 */
			enum class pdapi_indics_t : uint8_t {
				NONE                     = 0x00, // Индикатор отсутствует
				PARTIAL_DELIVERY_ABORTED = 0x01  // Частичная доставка прервана
			};
			/**
			 * Типы сброса потоков SCTP
			 */
			enum class stream_reset_t : uint8_t {
				NONE         = 0x00, // Тип сброса отсутствует
				DENIED       = 0x01, // Сброс отклонён
				FAILED       = 0x02, // Сброс не выполнен
				OUTGOING_SSN = 0x03, // Сброс исходящих потоков
				INCOMING_SSN = 0x04  // Сброс входящих потоков
			};
			/**
			 * Типы изменения потоков SCTP
			 */
			enum class stream_change_t : uint8_t {
				NONE   = 0x00, // Тип изменения отсутствует
				FAILED = 0x01, // Изменение не выполнено
				DENIED = 0x02  // Изменение отклонено
			};
			/**
			 * Статусы состояния сокета SCTP
			 */
			enum class state_status_t : uint8_t {
				NONE              = 0x00, // Статус отсутствует
				BOUND             = 0x01, // Вызван bind(), но ассоциация не установлена
				CLOSED            = 0x02, // Ассоциация не существует (сокет создан, но не привязан/не использован)
				LISTEN            = 0x03, // Сервер вызвал listen() и ждёт входящих INIT (только для STREAM)
				ESTABLISHED       = 0x04, // Ассоциация установлена, можно передавать данные
				COOKIE_WAIT       = 0x05, // Клиент отправил INIT, ждёт INIT-ACK
				COOKIE_ECHOED     = 0x06, // Клиент получил INIT-ACK, отправил COOKIE-ECHO, ждёт COOKIE-ACK
				SHUTDOWN_SENT     = 0x07, // Отправлен SHUTDOWN, ждём SHUTDOWN-ACK
				SHUTDOWN_PENDING  = 0x08, // Приложение вызвало shutdown(), но есть неподтверждённые данные
				SHUTDOWN_RECEIVED = 0x09, // Получен SHUTDOWN от пираа, ждём
				SHUTDOWN_ACK_SENT = 0x0A  // Отправлен SHUTDOWN-ACK, ждём SHUTDOWN-COMPLETE
			};
			/**
			 * Типы событий SCTP
			 */
			enum class event_type_t : uint8_t {
				NONE                   = 0x00, // Тип события отсутствует
				DATA_IO                = 0x01, // Присылать уведомление о каждом входящем DATA-пакете
				SEND_FAILED            = 0x02, // Ошибка отправки сообщения
				REMOTE_ERROR           = 0x03, // Ошибка удалённого узла
				ASSOC_CHANGE           = 0x04, // Изменение ассоциации
				SHUTDOWN_EVENT         = 0x05, // Событие завершения работы
				SENDER_DRY_EVENT       = 0x06, // Событие "отправитель сухой"
				PEER_ADDR_CHANGE       = 0x07, // Изменение адреса однорангового узла
				ASSOC_RESET_EVENT      = 0x08, // Сброс ассоциации
				SEND_FAILED_EVENT      = 0x09, // Событие ошибки отправки
				STREAM_RESET_EVENT     = 0x0A, // Сброс потока
				STREAM_CHANGE_EVENT    = 0x0B, // Изменение потоков
				AUTHENTICATION_EVENT   = 0x0C, // Событие аутентификации
				ADAPTATION_INDICATION  = 0x0D, // Адаптационное указание
				PARTIAL_DELIVERY_EVENT = 0x0E  // Частичная доставка
			};
			/**
			 * Типы сброса ассоциации SCTP
			 */
			enum class assoc_reset_t : uint8_t {
				NONE   = 0x00, // Тип сброса отсутствует
				FAILED = 0x01, // Сброс не выполнен
				DENIED = 0x02  // Сброс отклонён
			};
			/**
			 * Информация об ассоциации SCTP
			 */
			enum class assoc_info_t : uint8_t {
				NONE                = 0x00, // Информация об ассоциации отсутствует
				SUPPORTS_PR         = 0x01, // Поддерживается частичное надёжное сообщение
				SUPPORTS_MAX        = 0x02, // Поддерживается максимальное количество сообщений
				SUPPORTS_AUTH       = 0x03, // Поддерживается аутентификация сообщений
				SUPPORTS_ASCONF     = 0x04, // Поддерживается динамическая конфигурация адресов
				SUPPORTS_MULTIBUF   = 0x05, // Поддерживается мультибуферизация сообщений
				SUPPORTS_RE_CONFIG  = 0x06, // Поддерживается повторная конфигурация ассоциации
				SUPPORTS_INTERLEAVE = 0x07  // Поддерживается перемежение сообщений
			};
			/**
			 * Состояния ассоциации SCTP
			 */
			enum class assoc_state_t : uint8_t {
				NONE 	      = 0x00, // Состояние ассоциации отсутствует
				COMM_UP       = 0x01, // Связь установлена
				COMM_LOST     = 0x02, // Связь потеряна
				RESTARTED     = 0x03, // Связь перезапущена
				SHUTDOWN_COMP = 0x04, // Завершение работы выполнено
				CANT_START    = 0x05  // Не удалось запустить связь
			};
			/**
			 * Состояния адреса однорангового узла SCTP
			 */
			enum class paddr_state_t : uint8_t {
				NONE        = 0x00, // Состояние адреса отсутствует
				ADDED       = 0x01, // Адрес был добавлен в ассоциацию
				REMOVED     = 0x02, // Адрес был удалён из ассоциации
				MADE_PRIM   = 0x03, // Адрес был установлен как основной
				CONFIRMED   = 0x04, // Адрес подтверждён одноранговым узлом
				AVAILABLE   = 0x05, // Адрес стал доступен
				UNREACHABLE = 0x06  // Адрес стал недоступен
			};
			/**
			 * Флаги информации о сообщении SCTP
			 */
			enum class info_t : uint8_t {
				NONE               = 0x00, // Флаг отсутствует
				PR_TTL             = 0x01, // Сообщение имеет ограничение по времени жизни
				PR_RTX             = 0x02, // Сообщение имеет ограничение по количеству повторных попыток
				PR_PRIO            = 0x03, // Сообщение имеет приоритет
				SEND_ALL           = 0x04, // Отправка сообщения всем ассоциациям
				ADDR_OVER          = 0x05, // Использовать адрес из to, даже если сокет подключён
				STATUS_EOF 	       = 0x06, // Сообщение содержит признак грациозного завершения
				STATUS_ABORT       = 0x07, // Сообщение содержит признак аварийного завершения
				SACK_IMMEDIATELY   = 0x08, // Установка бита последнего фрагмента DATA, для мгновенной отправки
				DELIVERY_UNORDERED = 0x09  // Сообщение доставляется без учёта порядка в потоке
			};
			/**
			 * @brief Множество типов событий SCTP
			 *
			 */
			using event_types_t = unordered_set <event_type_t>;
			/**
			 * @brief Структура метаданных сообщения SCTP
			 *
			 */
			typedef struct MessageInfo {
				ppid_t ppid;                  // Идентификатор полезной нагрузки
				uint16_t num;                 // Номер потока
				uint32_t ttl;                 // Время жизни (в миллисекундах)
				uint32_t ctx;                 // Контекст для уведомлений об ошибках
				unordered_set <info_t> flags; // Флаги сообщения
				/**
				 * @brief Конструктор
				 *
				 */
				explicit MessageInfo() noexcept :
				 ppid(ppid_t::DTLS),
				 num(0), ttl(0), ctx(0) {}
			} __attribute__((packed)) minfo_t;
			/**
			 * @brief Структура параметров рукопожатия SCTP
			 *
			 */
			typedef struct InitMessage {
				// Максимальное время инициализации SCTP
				uint16_t timeout;
				// Максимальное количество попыток подключения
				uint16_t attempts;
				// Максимальное количество исходящих потоков
				uint16_t ostreams;
				// Максимальное количество входящих потоков
				uint16_t istreams;
				
				/**
				 * @brief Конструктор
				 *
				 */
				explicit InitMessage() noexcept :
				 timeout(0), attempts(4),
				 ostreams(5), istreams(5) {}
			} __attribute__((packed)) initmsg_t;
			/**
			 * @brief Структура статуса SCTP подключения
			 *
			 */
			typedef struct Status {
				uint32_t id;          // ID ассоциации
				uint32_t ratewind;    // Размер окна скорости передачи
				uint16_t penddata;    // Количество ожидающих данных
				uint16_t ostreams;    // Количество исходящих потоков
				uint16_t istreams;    // Количество входящих потоков
				uint16_t unackdata;   // Количество неподтверждённых DATA чанков
				uint32_t fragpoint;   // Точка фрагментации в байтах
				state_status_t state; // Текущее состояние ассоциации
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Status() noexcept :
				 id(0),
				 ratewind(0), penddata(0),
				 ostreams(0), istreams(0),
				 unackdata(0), fragpoint(0),
				 state(state_status_t::NONE) {}
			} __attribute__((packed)) status_t;
			/**
			 * @brief Структура ошибки события SCTP
			 *
			 */
			typedef struct Error {
				int32_t code;   // Код ошибки события
				string message; // Сообщение ошибки события
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Error() noexcept :
				 code(0), message{""} {}
			} error_t;
			/**
			 * @brief Структура события SCTP
			 *
			 */
			typedef struct Event {
				// Идентификатор события
				uint32_t id;
				// Тип события
				event_type_t type;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Event() noexcept :
				 id(0), type(event_type_t::NONE) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Event() = default;
			} event_t;
			/**
			 * @brief Структура адаптационного указания SCTP
			 *
			 */
			typedef struct EventAdaptation : public event_t {
				// Адаптационное указание
				uint32_t indication;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventAdaptation() noexcept : indication(0) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventAdaptation() = default;
			} event_adaptation_t;
			/**
			 * @brief Структура изменения ассоциации события SCTP
			 *
			 */
			typedef struct EventAssocChange : public event_t {
				error_t error;              // Ошибка события
				uint16_t ostreams;          // Максимальное количество исходящих потоков
				uint16_t istreams;          // Максимальное количество входящих потоков
				assoc_state_t state;        // Состояние события
				vector <assoc_info_t> info; // Дополнительная информация события
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventAssocChange() noexcept :
				 ostreams(0), istreams(0),
				 state(assoc_state_t::NONE) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventAssocChange() = default;
			} event_assoc_change_t;
			/**
			 * @brief Структура сброса ассоциации SCTP
			 *
			 */
			typedef struct EventAssocReset : public event_t {
				// Последний TSN (Transmission Sequence Number), подтверждённый вами (вы получили его от пира)
				uint32_t localTSN;
				// Последний TSN (Transmission Sequence Number), подтверждённый пиром (он получил его от вас)
				uint32_t remoteTSN;
				// Флаги сброса ассоциации
				unordered_set <assoc_reset_t> flags;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventAssocReset() noexcept :
				 localTSN(0), remoteTSN(0) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventAssocReset() = default;
			} event_assoc_reset_t;
			/**
			 * @brief Структура ошибки удалённого узла SCTP
			 *
			 */
			typedef struct EventRemoteError : public event_t {
				error_t error;         // Ошибка события
				vector <uint8_t> data; // Дополнительная информация события
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventRemoteError() noexcept = default;
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventRemoteError() = default;
			} event_remote_error_t;
			/**
			 * @brief Структура изменения адреса однорангового узла SCTP
			 *
			 */
			typedef struct EventAddrChange : public event_t {
				error_t error;            // Ошибка события
				paddr_state_t state;      // Состояние события
				unique_ptr <addr_t> addr; // Адрес однорангового узла
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventAddrChange() noexcept :
				 state(paddr_state_t::NONE), addr(nullptr) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventAddrChange() = default;
			} event_addr_change_t;
			/**
			 * @brief Структура частичной доставки SCTP
			 *
			 */
			typedef struct PartialDeliveryEvent : public event_t {
				uint16_t stream;           // Номер потока
				uint16_t sequence;         // Последовательный номер сообщения
				pdapi_indics_t indication; // Индикатор частичной доставки
				/**
				 * @brief Конструктор
				 *
				 */
				explicit PartialDeliveryEvent() noexcept :
				 stream(0), sequence(0),
				 indication(pdapi_indics_t::NONE) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~PartialDeliveryEvent() = default;
			} event_pdapi_t;
			/**
			 * @brief Структура аутентификации SCTP
			 *
			 */
			typedef struct EventAuth : public event_t {
				uint16_t key;             // Номер ключа
				auth_indics_t indication; // Индикатор аутентификации
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventAuth() noexcept :
				 key(0), indication(auth_indics_t::NONE) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventAuth() = default;
			} event_auth_t;
			/**
			 * @brief Структура ошибки отправки SCTP
			 *
			 */
			typedef struct EventSendFailed : public event_t {
				error_t error;         // Ошибка события
				send_failed_t status;  // Статус отправки сообщения
				vector <uint8_t> data; // Дополнительная информация события
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventSendFailed() noexcept : status(send_failed_t::NONE) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventSendFailed() = default;
			} event_send_failed_t;
			/**
			 * @brief Структура сброса потоков SCTP
			 *
			 */
			typedef struct EventStreamReset : public event_t {
				vector <uint16_t> streams;            // Номера сброшенных потоков
				unordered_set <stream_reset_t> flags; // Типы сброса потоков
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventStreamReset() noexcept {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventStreamReset() = default;
			} event_stream_reset_t;
			/**
			 * @brief Структура изменения потоков SCTP
			 *
			 */
			typedef struct EventStreamChange : public event_t {
				// Максимальное количество исходящих потоков
				uint16_t ostreams;
				// Максимальное количество входящих потоков
				uint16_t istreams;
				// Флаги сброса ассоциации
				unordered_set <stream_change_t> flags;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit EventStreamChange() noexcept :
				 ostreams(0), istreams(0) {}
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~EventStreamChange() = default;
			} event_stream_change_t;
			/**
			 * @brief пространство имён работы с обратными вызовами
			 *
			 */
			namespace callback {
				/**
				 * Функция обратного вызова срабатывающая при получении информационных сообщений SCTP
				 */
				using info_t = std::function <void (const event::id_t, const minfo_t &)>;
				/**
				 * Функция обратного вызова срабатывающая при получении событий SCTP
				 */
				using events_t = std::function <void (const event::id_t, unique_ptr <event_t>)>;
			};
		};
		/**
		 * @brief Создаём тип данных SCTP события
		 *
		 */
		using sctp_event_t = unique_ptr <sctp::event_t>;
	};
};

#endif // __AWH_NETWORK__
