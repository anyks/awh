/**
 * @file: net.cpp
 * @date: 2025-11-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация базовых сетевых структур — конструирование, сравнение и преобразование адресов подключения (IPv4,
 *        IPv6, MAC, UDS), атрибутов и источников соединения, информации о датаграммах и туннелях
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <cstring>
#include <net/net.hpp>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>

	/**
	 * Закрепляем совпадение типа сокета AWH с системным
	 *
	 * @details Тип awh::net::socket_t выписан в открытом заголовке своими словами
	 *          (uintptr_t), чтобы тот не тянул за собой заголовки MS Windows. Проверка
	 *          эта следит, чтобы написание не разошлось с системным SOCKET: разойдись
	 *          оно, вызовы сокетного API усекали бы дескриптор, и обнаружилось бы это
	 *          лишь под нагрузкой, когда номера дескрипторов выйдут за пределы
	 *          младшего слова
	 *
	 * @note Проверка стоит здесь, а не в net/fds.cpp, поскольку объявление типа
	 *       принадлежит net/net.hpp, и правка написания должна вскрываться в том же
	 *       месте, где живёт само объявление
	 *
	 */
	static_assert(sizeof(awh::net::socket_t) == sizeof(SOCKET), "awh::net::socket_t size differs from system SOCKET");
	static_assert(std::is_same <awh::net::socket_t, SOCKET>::value, "awh::net::socket_t differs from system SOCKET");
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 * @param size размер адреса
 *
 */
awh::net::Address::Address(const uint16_t size) noexcept : size(size) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Address_MAC::Address_MAC() noexcept :
 addr_t(6), address{0} {}

/**
 * @brief Конструктор
 *
 * @param prefix префикс сети
 * @param size   размер адреса
 *
 */
awh::net::Address_Network::Address_Network(const uint8_t prefix, const uint16_t size) noexcept :
 addr_t(size), prefix(prefix) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Address_Network_IPv4::Address_Network_IPv4() noexcept :
 addr_net_t(32, 4), address(0) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Address_Network_IPv6::Address_Network_IPv6() noexcept :
 addr_net_t(128, 16), zone(0), address{0} {}

/**
 * @brief Конструктор
 *
 */
awh::net::Address_Filesystem::Address_Filesystem() noexcept : address{""} {}

/**
 * @brief Конструктор
 *
 * @param ip адрес сетевого подключения
 *
 */
awh::net::Source::Source(unique_ptr <addr_t> ip) noexcept :
 iface{""}, ip(::move(ip)),
 mac(make_unique <addr_mac_t> ()) {}

/**
 * @brief Конструктор
 *
 * @param type тип адреса подключения
 *
 */
awh::net::Attributes::Attributes(const type_t type) noexcept : type(type) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Attributes_FQDN::Attributes_FQDN() noexcept :
 attr_t(type_t::FQDN), port(0), domain{""} {}

/**
 * @brief Конструктор
 *
 */
awh::net::Attributes_Network::Attributes_Network() noexcept :
 attr_t(type_t::NONE), port(0), ip(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Attributes_Unix_Domain_Socket::Attributes_Unix_Domain_Socket() noexcept :
 attr_t(type_t::FS), path(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Origin_Key::Origin_Key() noexcept : size(0), data{0} {}
/**
 * @brief Конструктор
 *
 * @param data данные ключа сессии
 * @param size размер ключа сессии
 *
 */
awh::net::Origin_Key::Origin_Key(const uint8_t * data, const uint8_t size) noexcept : size(0), data{0} {
	// Если данные ключа сессии переданы и укладываются в размер ключа
	if((data != nullptr) && (size > 0) && (size <= MAX_ORIGIN_KEY_SIZE)){
		// Устанавливаем размер ключа сессии
		this->size = size;
		// Копируем данные ключа сессии
		::memcpy(this->data, data, size);
	}
}

/**
 * @brief Конструктор
 *
 */
awh::net::Datagram_Info::Datagram_Info() noexcept :
 hops(0), ifaceIndex(0),
 family(event::family_t::NONE),
 congestion(event::ecn_t::NOT_ECT),
 protocol(event::protocol_t::NONE),
 trafficClass(event::dscp_t::CS0) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Tunnel_Info::Tunnel_Info() noexcept :
 hops(event::hops_t::WORLD),
 family(event::family_t::NONE),
 protocol(event::protocol_t::NONE),
 target(nullptr), source(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::net::Interface::Interface() noexcept : name{""}, mtu(0), flags{} {}

/**
 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
 */
#if __linux__ || __FreeBSD__ || __sun
	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Message_Info::Message_Info() noexcept :
	 ppid(ppid_t::DTLS),
	 num(0), ttl(0), ctx(0) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Initialization_Message::Initialization_Message() noexcept :
	 timeout(0), attempts(4),
	 ostreams(5), istreams(5) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Status::Status() noexcept :
	 id(0),
	 ratewind(0), penddata(0),
	 ostreams(0), istreams(0),
	 unackdata(0), fragpoint(0),
	 state(state_status_t::NONE) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Error::Error() noexcept :
	 code(0), message{""} {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event::Event() noexcept :
	 id(0), type(event_type_t::NONE) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event_Adaptation::Event_Adaptation() noexcept : indication(0) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event_Association_Change::Event_Association_Change() noexcept :
	 ostreams(0), istreams(0),
	 state(assoc_state_t::NONE) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event_Association_Reset::Event_Association_Reset() noexcept :
	 localTSN(0), remoteTSN(0) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event_Address_Change::Event_Address_Change() noexcept :
	 state(paddr_state_t::NONE), addr(nullptr) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Partial_Delivery_Event::Partial_Delivery_Event() noexcept :
	 stream(0), sequence(0),
	 indication(pdapi_indics_t::NONE) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event_Authentication::Event_Authentication() noexcept :
	 key(0), indication(auth_indics_t::NONE) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event_Send_Failed::Event_Send_Failed() noexcept : status(send_failed_t::NONE) {}

	/**
	 * @brief Конструктор
	 *
	 */
	awh::net::sctp::Event_Stream_Change::Event_Stream_Change() noexcept :
	 ostreams(0), istreams(0) {}
#endif
