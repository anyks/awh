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

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NETWORK__
#define __AWH_NETWORK__

/**
 * Стандартные модули
 */
#include <array>
#include <string>
#include <cstdint>
#include <unordered_set>

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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
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
		 * @brief Режимы установки типа сокета
		 *
		 */
		enum class socket_mode_t : uint8_t {
			ENABLED  = 0x01, // Включено
			DISABLED = 0x02  // Выключено
		};

		/**
		 * @brief События сокета
		 *
		 */
		enum class socket_event_t : uint8_t {
			READ  = 0x01, // Чтение
			WRITE = 0x02  // Запись
		};

		/**
		 * @brief Типы IP-адресов
		 *
		 */
		enum class ip_type_t : uint8_t {
			NONE   = 0x00, // Тип не определён
			LOCAL  = 0x01, // Локальный адрес
			GLOBAL = 0x02, // Глобальный адрес
			MASK   = 0x03  // Маска подсети
		};

		/**
		 * @brief Идентификаторы разновидностей адресов
		 *
		 */
		enum class type_t : uint8_t {
			NONE  = 0x00, // Не определено
			FS    = 0x01, // Адрес в файловой системе
			MAC   = 0x02, // Аппаратный адрес сетевого интерфейса
			IPV4  = 0x04, // Адрес подключения IPv4
			IPV6  = 0x05, // Адрес подключения IPv6
			FQDN  = 0x06, // Доменная зона
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
		typedef struct Address_MAC : public addr_t {
			// Буфер MAC-адреса
			array <uint8_t, 6> address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Address_MAC() noexcept :
			 addr_t(6), address{0} {}
		} addr_mac_t;

		/**
		 * @brief Структура сетевого адреса
		 *
		 */
		typedef struct Address_Network : public addr_t {
			// Префикс сети
			uint8_t prefix;
			/**
			 * @brief Конструктор
			 *
			 * @param prefix префикс сети
			 * @param size   размер адреса
			 */
			explicit Address_Network(const uint8_t prefix, const uint16_t size) noexcept :
			 addr_t(size), prefix(prefix) {}
		} addr_net_t;

		/**
		 * @brief Структура IPv4 сетевого адреса
		 *
		 */
		typedef struct Address_Network_IPv4 : public addr_net_t {
			// IP-адрес сети
			uint32_t address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Address_Network_IPv4() noexcept :
			 addr_net_t(32, 4), address(0) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Address_Network_IPv4() noexcept = default;
		} addr_net_ipv4_t;

		/**
		 * @brief Структура IPv6 сетевого адреса
		 *
		 */
		typedef struct Address_Network_IPv6 : public addr_net_t {
			// Буфер IP-адрес сети
			array <uint8_t, 16> address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Address_Network_IPv6() noexcept :
			 addr_net_t(128, 16), address{0} {}
		} addr_net_ipv6_t;

		/**
		 * @brief Структура адреса файловой системы
		 *
		 */
		typedef struct Address_Filesystem : public addr_t {
			// Путь к файлу, каталогу или сокету
			string address;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Address_Filesystem() noexcept : address{""} {}
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
			 iface{""}, ip(std::move(ip)),
			 mac(make_unique <addr_mac_t> ()) {}
		} src_t;

		/**
		 * @brief Структура атрибутов подключения
		 *
		 */
		typedef struct Attributes {
			// Тип адреса подключения
			type_t type;
			/**
			 * @brief Конструктор
			 *
			 * @param type тип адреса подключения
			 */
			explicit Attributes(const type_t type) noexcept : type(type) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Attributes() noexcept = default;
		} attr_t;

		/**
		 * @brief Структура FQDN-адреса подключения
		 *
		 */
		typedef struct Attributes_FQDN : public attr_t {
			// Порт хоста
			uint16_t port;
			// Доменное имя хоста
			string domain;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Attributes_FQDN() noexcept :
			 attr_t(type_t::FQDN), port(0), domain{""} {}
		} attr_fqdn_t;

		/**
		 * @brief Структура IP-адреса подключения
		 *
		 */
		typedef struct Attributes_Network : public attr_t {
			// Порт хоста
			uint16_t port;
			// IP-адрес хоста
			unique_ptr <addr_t> ip;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Attributes_Network() noexcept :
			 attr_t(type_t::IPV4), port(0),
			 ip(make_unique <addr_net_ipv4_t> ()) {}
		} attr_net_t;

		/**
		 * @brief Структура UDS-адреса подключения
		 *
		 */
		typedef struct Attributes_Unix_Domain_Socket : public attr_t {
			// Путь к сокету
			unique_ptr <addr_t> path;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Attributes_Unix_Domain_Socket() noexcept :
			 attr_t(type_t::FS), path(make_unique <addr_fs_t> ()) {}
		} attr_uds_t;

		/**
		 * @brief Структура метаданных последнего принятого дейтаграммного пакета
		 *
		 */
		typedef struct Datagram_Info {
			// Сырое значение TTL/Hop Limit последнего принятого пакета (RFC, 0..255)
			uint8_t hops;
			// Индекс входного интерфейса
			uint32_t ifaceIndex;
			// Семейство принятого пакета
			event::family_t family;
			// Протокол принятого пакета
			event::protocol_t protocol;
			// Класс трафика (TOS/Traffic Class)
			event::dscp_t trafficClass;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Datagram_Info() noexcept :
			 hops(0), ifaceIndex(0),
			 family(event::family_t::NONE),
			 protocol(event::protocol_t::NONE),
			 trafficClass(event::dscp_t::CS0) {}
		} dgram_info_t;

		/**
		 * @brief Структура информации о пакетах в тоннеле
		 *
		 */
		typedef struct Tunnel_Info {
			// Количество хопов
			event::hops_t hops;
			// Семейство адресов
			event::family_t family;
			// Протокол подключения
			event::protocol_t protocol;
			// Адрес назначения подключения
			unique_ptr <attr_t> target;
			// Адрес источника подключения
			unique_ptr <attr_t> source;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Tunnel_Info() noexcept :
			 hops(event::hops_t::WORLD),
			 family(event::family_t::NONE),
			 protocol(event::protocol_t::NONE),
			 target(nullptr), source(nullptr) {}
		} tun_info_t;

		/**
		 * @brief Структура сетевого интерфейса
		 *
		 */
		typedef struct Interface {
			string name;                             // Название интерфейса
			uint16_t mtu;                            // MTU интерфейса
			unordered_set <event::eth_flag_t> flags; // Флаги интерфейса
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Interface() noexcept : name{""}, mtu(0), flags{} {}
		} iface_t;

		/**
		 * Для операционной системы Linux или FreeBSD
		 */
		#if __linux__ || __FreeBSD__
			/**
			 * @brief Пространство имён для работы с SCTP
			 *
			 */
			namespace sctp {
				/**
				 * @brief Идентификатор полезной нагрузки SCTP
				 *
				 */
				enum class ppid_t : uint8_t {
					DTLS       = 0x32, // (RFC 6083) DTLS поверх SCTP
					WEBRTC_STR = 0x33, // Строковые данные канала WebRTC
					WEBRTC_BIN = 0x35  // Бинарные данные канала WebRTC
				};

				/**
				 * @brief Статусы таймаутов SCTP
				 *
				 */
				enum class timeout_t : uint8_t {
					NONE        = 0x00, // Таймаут отсутствует
					INIT        = 0x01, // Таймаут INIT
					DATA        = 0x02, // Таймаут DATA
					SACK        = 0x03, // Таймаут SACK
					SHUTDOWN    = 0x04, // Таймаут SHUTDOWN
					HEARTBEAT   = 0x05, // Таймаут HEARTBEAT
					COOKIE      = 0x06, // Таймаут COOKIE-ECHO
					SHUTDOWNACK = 0x07  // Таймаут SHUTDOWN-ACK
				};

				/**
				 * @brief Типы аутентификации события SCTP
				 *
				 */
				enum class auth_type_t : uint8_t {
					HMAC_RSVD    = 0x00, // ЗаSCTPрезервировано
					HMAC_SHA1    = 0x01, // HMAC-SHA1 аутентификация
					HMAC_SHA256  = 0x02  // HMAC-SHA256 аутентификация
				};

				/**
				 * @brief Типы чанков попадающие под аутентификацию SCTP
				 *
				 */
				enum class auth_chunk_t : uint8_t {
					DATA              = 0x00, // Чанк DATA подлежит аутентификации
					INIT              = 0x01, // Чанк INIT подлежит аутентификации
					INIT_ACK          = 0x02, // Чанк INIT-ACK подлежит аутентификации
					SACK              = 0x03, // Чанк SACK подлежит аутентификации
					HEARTBEAT         = 0x04, // Чанк HEARTBEAT подлежит аутентификации
					HEARTBEAT_ACK     = 0x05, // Чанк HEARTBEAT-ACK подлежит аутентификации
					ABORT             = 0x06, // Чанк ABORT подлежит аутентификации
					SHUTDOWN          = 0x07, // Чанк SHUTDOWN подлежит аутентификации
					SHUTDOWN_ACK      = 0x08, // Чанк SHUTDOWN-ACK подлежит аутентификации
					ERROR             = 0x09, // Чанк ERROR подлежит аутентификации
					COOKIE_ECHO       = 0x0A, // Чанк COOKIE-ECHO подлежит аутентификации
					COOKIE_ACK        = 0x0B, // Чанк COOKIE-ACK подлежит аутентификации
					ECNE              = 0x0C, // Чанк ECNE подлежит аутентификации
					CWR               = 0x0D, // Чанк CWR подлежит аутентификации
					SHUTDOWN_COMPLETE = 0x0E, // Чанк SHUTDOWN-COMPLETE подлежит аутентификации
					AUTH              = 0x0F, // Чанк AUTH подлежит аутентификации
					FORWARD_TSN       = 0x10, // Чанк FORWARD-TSN подлежит аутентификации
					RE_CONFIG         = 0x11  // Чанк RE-CONFIG подлежит аутентификации
				};

				/**
				 * @brief Типы индикаторов события аутентификации
				 *
				 */
				enum class auth_indics_t : uint8_t {
					NONE     = 0x00, // Тип аутентификации отсутствует
					NEW_KEY  = 0x01, // Событие нового ключа
					NO_AUTH  = 0x02, // Событие отсутствия аутентификации
					FREE_KEY = 0x03  // Событие освобождения ключа
				};

				/**
				 * @brief Флаги отправки сообщения SCTP
				 *
				 */
				enum class send_failed_t : uint8_t {
					NONE   = 0x00, // Флаг отсутствует
					SENT   = 0x01, // Сообщение отправлено
					UNSENT = 0x02  // Сообщение не отправлено
				};

				/**
				 * @brief Индикаторы доставки SCTP
				 *
				 */
				enum class pdapi_indics_t : uint8_t {
					NONE                     = 0x00, // Индикатор отсутствует
					PARTIAL_DELIVERY_ABORTED = 0x01  // Частичная доставка прервана
				};

				/**
				 * @brief Типы сброса потоков SCTP
				 *
				 */
				enum class stream_reset_t : uint8_t {
					NONE         = 0x00, // Тип сброса отсутствует
					DENIED       = 0x01, // Сброс отклонён
					FAILED       = 0x02, // Сброс не выполнен
					OUTGOING_SSN = 0x03, // Сброс исходящих потоков
					INCOMING_SSN = 0x04  // Сброс входящих потоков
				};

				/**
				 * @brief Типы изменения потоков SCTP
				 *
				 */
				enum class stream_change_t : uint8_t {
					NONE   = 0x00, // Тип изменения отсутствует
					FAILED = 0x01, // Изменение не выполнено
					DENIED = 0x02  // Изменение отклонено
				};

				/**
				 * @brief Статусы состояния сокета SCTP
				 *
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
				 * @brief Типы событий SCTP
				 *
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
				 * @brief Типы сброса ассоциации SCTP
				 *
				 */
				enum class assoc_reset_t : uint8_t {
					NONE   = 0x00, // Тип сброса отсутствует
					FAILED = 0x01, // Сброс не выполнен
					DENIED = 0x02  // Сброс отклонён
				};

				/**
				 * @brief Информация об ассоциации SCTP
				 *
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
				 * @brief Состояния ассоциации SCTP
				 *
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
				 * @brief Состояния адреса однорангового узла SCTP
				 *
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
				 * @brief Флаги информации о сообщении SCTP
				 *
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
				typedef struct Message_Info {
					ppid_t ppid;                  // Идентификатор полезной нагрузки
					uint16_t num;                 // Номер потока
					uint32_t ttl;                 // Время жизни (в миллисекундах)
					uint32_t ctx;                 // Контекст для уведомлений об ошибках
					unordered_set <info_t> flags; // Флаги сообщения
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Message_Info() noexcept :
					 ppid(ppid_t::DTLS),
					 num(0), ttl(0), ctx(0) {}
				} __attribute__((packed)) minfo_t;

				/**
				 * @brief Структура инициализации рукопожатия SCTP
				 *
				 */
				typedef struct Initialization_Message {
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
					explicit Initialization_Message() noexcept :
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
				typedef struct Event_Adaptation : public event_t {
					// Адаптационное указание
					uint32_t indication;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Event_Adaptation() noexcept : indication(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Adaptation() = default;
				} event_adaptation_t;

				/**
				 * @brief Структура изменения ассоциации события SCTP
				 *
				 */
				typedef struct Event_Association_Change : public event_t {
					error_t error;              // Ошибка события
					uint16_t ostreams;          // Максимальное количество исходящих потоков
					uint16_t istreams;          // Максимальное количество входящих потоков
					assoc_state_t state;        // Состояние события
					vector <assoc_info_t> info; // Дополнительная информация события
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Event_Association_Change() noexcept :
					 ostreams(0), istreams(0),
					 state(assoc_state_t::NONE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Association_Change() = default;
				} event_assoc_change_t;

				/**
				 * @brief Структура сброса ассоциации SCTP
				 *
				 */
				typedef struct Event_Association_Reset : public event_t {
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
					explicit Event_Association_Reset() noexcept :
					 localTSN(0), remoteTSN(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Association_Reset() = default;
				} event_assoc_reset_t;

				/**
				 * @brief Структура ошибки удалённого узла SCTP
				 *
				 */
				typedef struct Event_Remote_Error : public event_t {
					error_t error;         // Ошибка события
					vector <uint8_t> data; // Дополнительная информация события
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Event_Remote_Error() noexcept = default;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Remote_Error() = default;
				} event_remote_error_t;

				/**
				 * @brief Структура изменения адреса однорангового узла SCTP
				 *
				 */
				typedef struct Event_Address_Change : public event_t {
					error_t error;            // Ошибка события
					paddr_state_t state;      // Состояние события
					unique_ptr <addr_t> addr; // Адрес однорангового узла
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Event_Address_Change() noexcept :
					 state(paddr_state_t::NONE), addr(nullptr) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Address_Change() = default;
				} event_addr_change_t;

				/**
				 * @brief Структура частичной доставки SCTP
				 *
				 */
				typedef struct Partial_Delivery_Event : public event_t {
					uint16_t stream;           // Номер потока
					uint16_t sequence;         // Последовательный номер сообщения
					pdapi_indics_t indication; // Индикатор частичной доставки
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Partial_Delivery_Event() noexcept :
					 stream(0), sequence(0),
					 indication(pdapi_indics_t::NONE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Partial_Delivery_Event() = default;
				} event_pdapi_t;

				/**
				 * @brief Структура аутентификации SCTP
				 *
				 */
				typedef struct Event_Authentication : public event_t {
					uint16_t key;             // Номер ключа
					auth_indics_t indication; // Индикатор аутентификации
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Event_Authentication() noexcept :
					 key(0), indication(auth_indics_t::NONE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Authentication() = default;
				} event_auth_t;

				/**
				 * @brief Структура ошибки отправки SCTP
				 *
				 */
				typedef struct Event_Send_Failed : public event_t {
					error_t error;         // Ошибка события
					send_failed_t status;  // Статус отправки сообщения
					vector <uint8_t> data; // Дополнительная информация события
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Event_Send_Failed() noexcept : status(send_failed_t::NONE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Send_Failed() = default;
				} event_send_failed_t;

				/**
				 * @brief Структура сброса потоков SCTP
				 *
				 */
				typedef struct Event_Stream_Reset : public event_t {
					vector <uint16_t> streams;            // Номера сброшенных потоков
					unordered_set <stream_reset_t> flags; // Типы сброса потоков
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Event_Stream_Reset() noexcept {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Stream_Reset() = default;
				} event_stream_reset_t;

				/**
				 * @brief Структура изменения потоков SCTP
				 *
				 */
				typedef struct Event_Stream_Change : public event_t {
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
					explicit Event_Stream_Change() noexcept :
					 ostreams(0), istreams(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Event_Stream_Change() = default;
				} event_stream_change_t;
			};

			/**
			 * @brief Создаём тип данных SCTP события
			 *
			 */
			using sctp_event_t = unique_ptr <sctp::event_t>;
		#endif
	};
};

#endif // __AWH_NETWORK__
