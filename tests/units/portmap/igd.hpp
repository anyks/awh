/**
 * @file: igd.hpp
 * @date: 2026-08-05
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл поддельного устройства доступа в сеть с поддержкой UPnP —
 *        ответ на рассылку обнаружения, выдача описания устройства и приём вызова действия службы
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_FAKE_IGD__
#define __AWH_FAKE_IGD__
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdio>
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют: отвечающие им
 *       объявления приходят там из winsock2.h, подключаемого через единую точку
 *       sys/win32.hpp, а недостающее восполняет tests/posix.hpp
 *
 */
#if _WIN32 || _WIN64
	#include <sys/win32.hpp>
/**
 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
 */
#else
	#include <poll.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <net/if.h>
#endif

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"
// Поддельное устройство доступа в сеть с поддержкой UPnP
class FakeIGD {
	public:
		// Как отвечать на вызов действия службы
		enum class Mode : uint8_t { OK, FAULT, TRUNCATED, GARBAGE, DROP, SLOW, HTTP_ERROR, NO_SERVICE, BAD_XML, STALL };
	private:
		int _udp;                       // Гнездо обнаружения устройств
		int _tcp;                       // Гнездо управления устройством
		uint16_t _port;                 // Порт гнезда управления устройством
		std::string _address;           // Адрес машины в местной сети
		int _udp6;                      // Гнездо обнаружения устройств сетью IPv6
		std::string _address6;          // Адрес машины в местной сети IPv6
		/**
		 * @brief Второе гнездо управления устройством, отведённое сети IPv4
		 *
		 * @details Держится оно лишь там, где одно гнездо обеим сетям служить не может.
		 *          Совмещение задаётся настройкой IPV6_V6ONLY со значением лжи, и OpenBSD
		 *          отвечает на неё отказом «недопустимый довод» - проверено опытом, - а
		 *          macOS, FreeBSD и NetBSD принимают. Там, где совмещение доступно, гнездо
		 *          это не заводится вовсе
		 *
		 * @note Оба гнезда встают на один и тот же порт: он объявляется в ответе рассылки
		 *       один, и разводить их по разным значило бы объявлять неверный. Встать так
		 *       им ничто не мешает - семейства адресов у них разные, а совмещения нет
		 *
		 */
		int _tcp4;
		std::thread _udp6Thread;
		std::thread _udpThread;
		std::thread _tcpThread;
		std::thread _tcp4Thread;
		std::atomic <bool> _working;
		std::atomic <int> _calls;       // Количество принятых вызовов действий
		std::atomic <int> _fetches;     // Количество выданных описаний устройства
		std::atomic <int> _searches;    // Количество принятых рассылок обнаружения
		std::vector <int> _stalled;     // Подключения, оставленные без ответа
	public:
		Mode mode = Mode::OK;           // Как отвечать на вызов действия
		uint32_t fault = 718;           // Код отказа службы, выдаваемый при Mode::FAULT
		uint32_t entries = 0;           // Количество перенаправлений в выдаваемом перечне
		bool firewall = true;           // Признак того, что заслон IPv6 включён
		bool pinholes = true;           // Признак того, что пробои заслона IPv6 дозволены
		uint32_t unique = 4242;         // Опознаватель, выдаваемый проделанному пробою
		bool answerSearch = true;       // Отвечать ли на рассылку обнаружения
		std::string iface = "127.0.0.1";
		std::string device = "lo0";
	private:
		/**
		 * @brief Метод получения адреса машины в местной сети
		 *
		 * @details Описание устройства объявляется этим адресом, а не адресом петли:
		 *          модуль не читает описание по адресу вне местной сети - тем он оберегает
		 *          себя от подставного ответчика, уводящего на чужой узел, - и петля под
		 *          этот заслон попадает. Обмен при этом с машины не уходит: соединение
		 *          идёт на её же адрес
		 *
		 * @return адрес машины в местной сети, пустой при его отсутствии
		 *
		 */
		static std::string address() noexcept {
			// Перечень сетевых устройств машины
			struct ifaddrs * list = nullptr;
			// Если перечень сетевых устройств получить не удалось, выводим пустой адрес
			if(::getifaddrs(&list) != 0) return std::string();
			// Собираемый адрес машины в местной сети
			std::string result;
			/**
			 * Выполняем перебор всех сетевых устройств машины
			 */
			for(struct ifaddrs * item = list; item != nullptr; item = item->ifa_next){
				// Если устройство не поднято либо адреса сети IPv4 не имеет, пропускаем его
				if((item->ifa_addr == nullptr) || (item->ifa_addr->sa_family != AF_INET)) continue;
				// Если устройство является петлёй, пропускаем его
				if((item->ifa_flags & IFF_LOOPBACK) || !(item->ifa_flags & IFF_UP)) continue;
				// Получаем адрес очередного сетевого устройства
				const uint32_t value = ntohl(reinterpret_cast <struct sockaddr_in *> (item->ifa_addr)->sin_addr.s_addr);
				// Если адрес принадлежит сети связи, пропускаем его
				if((value >> 16) == 0xA9FE) continue;
				// Если адрес местной сетью не является, пропускаем его
				if(!(((value >> 24) == 10) || (((value >> 20) & 0xFFF) == 0xAC1) || ((value >> 16) == 0xC0A8))) continue;
				// Запоминаем адрес машины в местной сети
				result.assign(::inet_ntoa(reinterpret_cast <struct sockaddr_in *> (item->ifa_addr)->sin_addr));
				// Выходим из перебора сетевых устройств
				break;
			}
			// Выполняем освобождение перечня сетевых устройств
			::freeifaddrs(list);
			// Выводим собранный адрес машины в местной сети
			return result;
		}
		/**
		 * @brief Метод получения адреса машины в местной сети IPv6
		 *
		 * @details Годится лишь адрес, отведённый договором местным сетям: заслон модуля
		 *          не пускает чтение описания по адресу вне местной сети, а петля и
		 *          адрес связи под него не подходят - первая объявлена зарезервированной,
		 *          второй без указания устройства неоднозначен
		 *
		 * @return адрес машины в местной сети IPv6, пустой при его отсутствии
		 *
		 */
		static std::string address6() noexcept {
			// Перечень сетевых устройств машины
			struct ifaddrs * list = nullptr;
			// Если перечень сетевых устройств получить не удалось, выводим пустой адрес
			if(::getifaddrs(&list) != 0) return std::string();
			// Собираемый адрес машины в местной сети IPv6
			std::string result;
			/**
			 * Выполняем перебор всех сетевых устройств машины
			 */
			for(struct ifaddrs * item = list; item != nullptr; item = item->ifa_next){
				// Если устройство не поднято либо адреса сети IPv6 не имеет, пропускаем его
				if((item->ifa_addr == nullptr) || (item->ifa_addr->sa_family != AF_INET6)) continue;
				// Если устройство является петлёй, пропускаем его
				if((item->ifa_flags & IFF_LOOPBACK) || !(item->ifa_flags & IFF_UP)) continue;
				// Получаем адрес очередного сетевого устройства
				const struct in6_addr & value = reinterpret_cast <struct sockaddr_in6 *> (item->ifa_addr)->sin6_addr;
				// Если адрес местным сетям договором не отведён, пропускаем его
				if((value.s6_addr[0] & 0xFE) != 0xFC) continue;
				// Место под запись адреса машины
				char buffer[INET6_ADDRSTRLEN] = {0};
				// Если запись адреса собрать не удалось, пропускаем его
				if(::inet_ntop(AF_INET6, &value, buffer, sizeof(buffer)) == nullptr) continue;
				// Запоминаем адрес машины в местной сети IPv6
				result.assign(buffer);
				// Выходим из перебора сетевых устройств
				break;
			}
			// Выполняем освобождение перечня сетевых устройств
			::freeifaddrs(list);
			// Выводим собранный адрес машины в местной сети IPv6
			return result;
		}
	public:
		bool ready() const noexcept { return ((this->_udp >= 0) && (this->_tcp >= 0) && !this->_address.empty()); }
		// Признак готовности поддельного шлюза к обмену сетью IPv6
		bool ready6() const noexcept { return (this->ready() && (this->_udp6 >= 0) && !this->_address6.empty()); }
		int calls() const noexcept { return this->_calls.load(); }
		int fetches() const noexcept { return this->_fetches.load(); }
		int searches() const noexcept { return this->_searches.load(); }
		uint16_t port() const noexcept { return this->_port; }
	private:
		// Описание устройства
		std::string description() const noexcept {
			if(this->mode == Mode::BAD_XML) return "<root><device><serviceList>";
			std::string services =
				"<service><serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>"
				"<serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>"
				"<controlURL>/ctl/IPConn</controlURL><eventSubURL>/evt/IPConn</eventSubURL>"
				"<SCPDURL>/IPConn.xml</SCPDURL></service>";
			/**
			 * Дописываем службу заслона IPv6
			 *
			 * @note Настоящее устройство держит обе службы разом: перенаправление портов
			 *       сети IPv4 и пробои заслона сети IPv6, - и объявляет их в одном описании
			 */
			services += "<service><serviceType>urn:schemas-upnp-org:service:WANIPv6FirewallControl:1</serviceType>"
			            "<serviceId>urn:upnp-org:serviceId:WANIPv6FC1</serviceId>"
			            "<controlURL>/ctl/IPv6FC</controlURL><eventSubURL>/evt/IPv6FC</eventSubURL>"
			            "<SCPDURL>/IPv6FC.xml</SCPDURL></service>";
			if(this->mode == Mode::NO_SERVICE) services = "";
			return std::string(
				"<?xml version=\"1.0\"?>"
				"<root xmlns=\"urn:schemas-upnp-org:device-1-0\"><specVersion><major>1</major><minor>0</minor></specVersion>"
				"<device><deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>"
				"<friendlyName>Fake IGD</friendlyName><manufacturer>AWH</manufacturer><modelName>Test</modelName>"
				"<UDN>uuid:fake-igd-0001</UDN>"
				"<deviceList><device><deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>"
				"<UDN>uuid:fake-igd-0002</UDN>"
				"<deviceList><device><deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>"
				"<UDN>uuid:fake-igd-0003</UDN><serviceList>") + services + "</serviceList></device></deviceList>"
				"</device></deviceList></device></root>";
		}
		// Ответ на вызов действия службы
		std::string soap(const std::string & request) const noexcept {
			if(request.find("AddPortMapping") != std::string::npos)
				return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
				       "<s:Body><u:AddPortMappingResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/>"
				       "</s:Body></s:Envelope>";
			/**
			 * Если спрашивается состояние заслона IPv6
			 *
			 * @note Спрос этот предшествует проделыванию пробоя: заслон бывает отключён
			 *       либо пробои им запрещены, и просить о пробое тогда бесполезно
			 */
			if(request.find("GetFirewallStatus") != std::string::npos)
				return std::string(
					"<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
					"<u:GetFirewallStatusResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\">"
					"<FirewallEnabled>") + (this->firewall ? "1" : "0") + "</FirewallEnabled>"
					"<InboundPinholeAllowed>" + (this->pinholes ? "1" : "0") + "</InboundPinholeAllowed>"
					"</u:GetFirewallStatusResponse></s:Body></s:Envelope>";
			// Если проделывается пробой заслона IPv6, выводим выданный ему опознаватель
			if(request.find("AddPinhole") != std::string::npos)
				return std::string(
					"<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
					"<u:AddPinholeResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\">"
					"<UniqueID>") + std::to_string(this->unique) + "</UniqueID></u:AddPinholeResponse></s:Body></s:Envelope>";
			// Если продлевается срок пробоя заслона IPv6, выводим пустой ответ
			if(request.find("UpdatePinhole") != std::string::npos)
				return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
				       "<s:Body><u:UpdatePinholeResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\"/>"
				       "</s:Body></s:Envelope>";
			// Если заделывается пробой заслона IPv6, выводим пустой ответ
			if(request.find("DeletePinhole") != std::string::npos)
				return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
				       "<s:Body><u:DeletePinholeResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\"/>"
				       "</s:Body></s:Envelope>";
			if(request.find("GetExternalIPAddress") != std::string::npos)
				return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
				       "<s:Body><u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
				       "<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress></u:GetExternalIPAddressResponse></s:Body></s:Envelope>";
			/**
			 * Если читается очередная запись перечня заведённых перенаправлений
			 *
			 * @note Перечень читается по порядковому номеру, пока служба не ответит отказом
			 *       «номер вне перечня»: иного признака конца договор не даёт
			 */
			if(request.find("GetGenericPortMappingEntry") != std::string::npos){
				// Получаем порядковый номер читаемой записи перечня
				const size_t at = request.find("<NewPortMappingIndex>");
				// Порядковый номер читаемой записи перечня
				const uint32_t index = ((at == std::string::npos) ? 0 : static_cast <uint32_t> (::atol(request.c_str() + at + 21)));
				/**
				 * Если порядковый номер вышел за пределы перечня
				 */
				if(index >= this->entries)
					// Выводим отказ службы с кодом «номер вне перечня»
					return std::string(
						"<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><s:Fault>"
						"<faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail>"
						"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\"><errorCode>713</errorCode>"
						"<errorDescription>SpecifiedArrayIndexInvalid</errorDescription></UPnPError></detail></s:Fault></s:Body></s:Envelope>");
				// Выводим очередную запись перечня заведённых перенаправлений
				return std::string(
					"<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
					"<u:GetGenericPortMappingEntryResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
					"<NewRemoteHost></NewRemoteHost><NewExternalPort>") + std::to_string(40000 + index) + "</NewExternalPort>"
					"<NewProtocol>TCP</NewProtocol><NewInternalPort>" + std::to_string(8000 + index) + "</NewInternalPort>"
					"<NewInternalClient>" + this->_address + "</NewInternalClient><NewEnabled>1</NewEnabled>"
					"<NewPortMappingDescription>запись " + std::to_string(index) + "</NewPortMappingDescription>"
					"<NewLeaseDuration>3600</NewLeaseDuration></u:GetGenericPortMappingEntryResponse></s:Body></s:Envelope>";
			}
			if(request.find("DeletePortMapping") != std::string::npos)
				return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
				       "<s:Body><u:DeletePortMappingResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/>"
				       "</s:Body></s:Envelope>";
			return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
			       "<s:Body><u:GenericResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/></s:Body></s:Envelope>";
		}
		// Отказ службы, записанный по правилам SOAP
		std::string refusal() const noexcept {
			return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><s:Fault>"
			       "<faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail>"
			       "<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\"><errorCode>" + std::to_string(this->fault) + "</errorCode>"
			       "<errorDescription>Refused</errorDescription></UPnPError></detail></s:Fault></s:Body></s:Envelope>";
		}
	public:
		void start() noexcept {
			this->_working.store(true);
			// Поток обнаружения устройств
			this->_udpThread = std::thread([this]() noexcept -> void {
				char buffer[2048];
				while(this->_working.load()){
					struct sockaddr_in peer; socklen_t length = sizeof(peer);
					::memset(&peer, 0, sizeof(peer));
					const ssize_t size = ::recvfrom(this->_udp, reinterpret_cast <char *> (buffer), sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&peer), &length);
					if(size <= 0) continue;
					buffer[size] = 0;
					if(::strncmp(buffer, "M-SEARCH", 8) != 0) continue;
					this->_searches.fetch_add(1);
					if(!this->answerSearch) continue;
					char answer[1024];
					const int count = ::snprintf(answer, sizeof(answer),
						"HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=1800\r\nEXT:\r\n"
						"LOCATION: http://%s:%u/rootDesc.xml\r\n"
						"SERVER: Test/1.0 UPnP/1.0 FakeIGD/1.0\r\n"
						"ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
						"USN: uuid:fake-igd-0001::urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n\r\n",
						this->_address.c_str(), static_cast <unsigned> (this->_port));
					::sendto(this->_udp, reinterpret_cast <const char *> (answer), static_cast <size_t> (count), 0, reinterpret_cast <struct sockaddr *> (&peer), length);
				}
			});
			/**
			 * Поток обнаружения устройств сетью IPv6
			 *
			 * @note Ответ выдаётся адресом машины в местной сети IPv6, а не адресом петли:
			 *       петля объявлена договором зарезервированной, и заслон модуля её не пустит
			 */
			this->_udp6Thread = std::thread([this]() noexcept -> void {
				char buffer[2048];
				while(this->_working.load()){
					struct sockaddr_in6 peer; socklen_t length = sizeof(peer);
					::memset(&peer, 0, sizeof(peer));
					const ssize_t size = ::recvfrom(this->_udp6, reinterpret_cast <char *> (buffer), sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&peer), &length);
					if(size <= 0) continue;
					buffer[size] = 0;
					if(::strncmp(buffer, "M-SEARCH", 8) != 0) continue;
					this->_searches.fetch_add(1);
					if(!this->answerSearch) continue;
					char answer[1024];
					const int count = ::snprintf(answer, sizeof(answer),
						"HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=1800\r\nEXT:\r\n"
						"LOCATION: http://[%s]:%u/rootDesc.xml\r\n"
						"SERVER: Test/1.0 UPnP/1.0 FakeIGD/1.0\r\n"
						"ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
						"USN: uuid:fake-igd-0001::urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n\r\n",
						this->_address6.c_str(), static_cast <unsigned> (this->_port));
					::sendto(this->_udp6, reinterpret_cast <const char *> (answer), static_cast <size_t> (count), 0, reinterpret_cast <struct sockaddr *> (&peer), length);
				}
			});
			// Поток управления устройством
			this->_tcpThread = std::thread(&FakeIGD::control, this, this->_tcp);
			/**
			 * Если заведено отдельное гнездо управления сети IPv4, заводим ему свою нить
			 */
			if(this->_tcp4 >= 0)
				// Выполняем запуск нити обслуживания подключений управления сетью IPv4
				this->_tcp4Thread = std::thread(&FakeIGD::control, this, this->_tcp4);
		}
		/**
		 * @brief Метод обслуживания подключений управления устройством
		 *
		 * @details Ведётся он одинаково для обеих сетей, а гнездо передаётся доводом:
		 *          там, где одно гнездо служит обеим сетям, нить заводится одна, а где
		 *          совмещение недоступно - по нити на семейство адресов
		 *
		 * @param listener гнездо, принимающее подключения
		 *
		 */
		void control(const int listener) noexcept {
				while(this->_working.load()){
					/**
					 * @brief Ожидание подключения ведётся опросом, а не самим приёмом
					 *
					 * @details Приём подключения безвыходен: срок SO_RCVTIMEO на него не
					 *          распространяется ни в одной системе - проверено опытом на
					 *          macOS и OpenBSD, - и снять с него нить можно лишь закрытием
					 *          гнезда. Но закрытие будит вызов не везде: macOS отвечает
					 *          ECONNABORTED за миг, а OpenBSD чужую нить не снимает вовсе,
					 *          и ожидание её завершения стояло там навсегда
					 *
					 * @note Опрос со сроком возвращает нить в круг, где сверяется признак
					 *       работы, и остановка опирается на свой же признак, а не на
					 *       поведение системы - одинаково на всякой из них
					 *
					 */
					struct pollfd event;
					/**
					 * Устанавливаем гнездо, за которым ведётся наблюдение
					 *
					 * @warning Наблюдение ведётся за гнездом, переданным доводом, а не за
					 *          гнездом сети IPv6: там, где заведено второе гнездо, нить
					 *          сети IPv4 опрашивала чужое гнездо и подключения своего не
					 *          дожидалась вовсе
					 */
					event.fd = listener;
					// Устанавливаем ожидаемое событие готовности к приёму
					event.events = POLLIN;
					// Сбрасываем перечень наступивших событий
					event.revents = 0;
					// Если подключения за отведённый срок не поступило, идём на новый круг
					if(::poll(&event, 1, 100) <= 0) continue;
					const int peer = ::accept(listener, nullptr, nullptr);
					if(peer < 0) continue;
					std::string request;
					// Читаем запрос целиком
					for(;;){
						char buffer[4096];
						const ssize_t size = ::recv(peer, reinterpret_cast <char *> (buffer), sizeof(buffer), 0);
						if(size <= 0) break;
						request.append(buffer, static_cast <size_t> (size));
						const size_t head = request.find("\r\n\r\n");
						if(head == std::string::npos) continue;
						// Дочитываем тело по объявленной длине
						size_t length = 0;
						const size_t at = request.find("Content-Length:");
						if(at != std::string::npos) length = static_cast <size_t> (::atol(request.c_str() + at + 15));
						if(request.size() >= (head + 4 + length)) break;
					}
					if(request.empty()){ ::closesocket(peer); continue; }
					const bool control = (request.compare(0, 4, "POST") == 0);
					if(control) this->_calls.fetch_add(1);
					else this->_fetches.fetch_add(1);
					std::string body, answer;
					const Mode mode = this->mode;
					if(control && (mode == Mode::DROP)){ ::closesocket(peer); continue; }
					/**
					 * Если вызов действия оставляется без ответа
					 *
					 * @note Подключение при этом держится открытым: само по себе оно не
					 *       оборвётся, и обмен остаётся ждать ответа до истечения срока -
					 *       именно тогда модуль и повторяет шаг
					 */
					if(control && (mode == Mode::STALL)){ this->_stalled.push_back(peer); continue; }
					if(control && (mode == Mode::SLOW)){ std::this_thread::sleep_for(std::chrono::milliseconds(900)); }
					if(control && (mode == Mode::GARBAGE)){
						const char * junk = "!!! это не разметка вовсе !!!";
						answer = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: ")
						       + std::to_string(::strlen(junk)) + "\r\nConnection: close\r\n\r\n" + junk;
						::send(peer, reinterpret_cast <char *> (answer.data()), answer.size(), 0); ::closesocket(peer); continue;
					}
					if(control && (mode == Mode::HTTP_ERROR)){
						answer = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
						::send(peer, reinterpret_cast <char *> (answer.data()), answer.size(), 0); ::closesocket(peer); continue;
					}
					if(control) body = ((mode == Mode::FAULT) ? this->refusal() : this->soap(request));
					else body = this->description();
					const bool error = (control && (mode == Mode::FAULT));
					answer = std::string("HTTP/1.1 ") + (error ? "500 Internal Server Error" : "200 OK")
					       + "\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nContent-Length: "
					       + std::to_string(control && (mode == Mode::TRUNCATED) ? body.size() + 64 : body.size())
					       + "\r\nConnection: close\r\n\r\n" + body;
					::send(peer, reinterpret_cast <char *> (answer.data()), answer.size(), 0);
					::closesocket(peer);
				}
		}

		void stop() noexcept {
			this->_working.store(false);
			/**
			 * Гнёзда закрываются уже после завершения нитей
			 *
			 * @note Закрыть их раньше значило бы оставить нитям обмен по сброшенным
			 *       дескрипторам, а то и по чужим, если система успеет выдать те же
			 *       номера новым гнёздам. Снимают нити с ожидания выставленные им
			 *       сроки, а не закрытие, потому спешить с ним нужды нет
			 *
			 */
			if(this->_udpThread.joinable()) this->_udpThread.join();
			if(this->_udp6Thread.joinable()) this->_udp6Thread.join();
			if(this->_tcpThread.joinable()) this->_tcpThread.join();
			if(this->_tcp4Thread.joinable()) this->_tcp4Thread.join();
			if(this->_udp >= 0){ ::closesocket(this->_udp); this->_udp = -1; }
			if(this->_udp6 >= 0){ ::closesocket(this->_udp6); this->_udp6 = -1; }
			if(this->_tcp >= 0){ ::closesocket(this->_tcp); this->_tcp = -1; }
			if(this->_tcp4 >= 0){ ::closesocket(this->_tcp4); this->_tcp4 = -1; }
			// Выполняем закрытие подключений, оставленных без ответа
			for(const int peer : this->_stalled) ::closesocket(peer);
			// Выполняем очистку перечня подключений, оставленных без ответа
			this->_stalled.clear();
		}
	public:
		FakeIGD() noexcept : _udp(-1), _tcp(-1), _port(0), _address(address()), _udp6(-1), _address6(address6()), _tcp4(-1), _working(false), _calls(0), _fetches(0), _searches(0) {
			// Гнездо обнаружения устройств
			this->_udp = ::socket(AF_INET, SOCK_DGRAM, 0);
			if(this->_udp >= 0){
				int yes = 1;
				::setsockopt(this->_udp, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&yes), sizeof(yes));
				/**
				 * Настройки SO_REUSEPORT у MS Windows нет вовсе: разделение порта между
				 * гнёздами выражается там одним лишь SO_REUSEADDR, поставленным выше
				 */
				#ifdef SO_REUSEPORT
					::setsockopt(this->_udp, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast <const char *> (&yes), sizeof(yes));
				#endif
								::setReceiveTimeout(this->_udp, 100);
				struct sockaddr_in address; ::memset(&address, 0, sizeof(address));
				address.sin_family = AF_INET; address.sin_port = htons(1900);
				address.sin_addr.s_addr = htonl(INADDR_ANY);
				if(::bind(this->_udp, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0){
					::closesocket(this->_udp); this->_udp = -1;
				} else {
					// Вступаем в группу обнаружения устройств на устройстве петли
					struct ip_mreq group; ::memset(&group, 0, sizeof(group));
					group.imr_multiaddr.s_addr = ::inet_addr("239.255.255.250");
					group.imr_interface.s_addr = ::inet_addr(this->iface.c_str());
					if(::setsockopt(this->_udp, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast <const char *> (&group), sizeof(group)) != 0)
						::fprintf(stderr, "вступление в группу не удалось: %s\n", ::strerror(errno));
				}
			}
			/**
			 * Гнездо обнаружения устройств сетью IPv6
			 *
			 * @note Вступление в группу ведётся устройством петли: рассылка модуля уходит
			 *       им же, и принимать её следует на нём
			 */
			this->_udp6 = ::socket(AF_INET6, SOCK_DGRAM, 0);
			if(this->_udp6 >= 0){
				int yes = 1;
				::setsockopt(this->_udp6, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&yes), sizeof(yes));
				/**
				 * Настройки SO_REUSEPORT у MS Windows нет вовсе: разделение порта между
				 * гнёздами выражается там одним лишь SO_REUSEADDR, поставленным выше
				 */
				#ifdef SO_REUSEPORT
					::setsockopt(this->_udp6, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast <const char *> (&yes), sizeof(yes));
				#endif
				::setsockopt(this->_udp6, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast <const char *> (&yes), sizeof(yes));
								::setReceiveTimeout(this->_udp6, 100);
				struct sockaddr_in6 address; ::memset(&address, 0, sizeof(address));
				address.sin6_family = AF_INET6; address.sin6_port = htons(1900); address.sin6_addr = in6addr_any;
				if(::bind(this->_udp6, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0){
					::closesocket(this->_udp6); this->_udp6 = -1;
				} else {
					// Вступаем в группу обнаружения устройств на устройстве петли
					struct ipv6_mreq group; ::memset(&group, 0, sizeof(group));
					::inet_pton(AF_INET6, "FF02::C", &group.ipv6mr_multiaddr);
					group.ipv6mr_interface = ::if_nametoindex(this->device.c_str());
					if(::setsockopt(this->_udp6, IPPROTO_IPV6, IPV6_JOIN_GROUP, reinterpret_cast <const char *> (&group), sizeof(group)) != 0){
						::closesocket(this->_udp6); this->_udp6 = -1;
					}
				}
			}
			// Гнездо управления устройством
			this->_tcp = ::socket(AF_INET6, SOCK_STREAM, 0);
			/**
			 * Если гнездо управления устройством заведено
			 *
			 * @note Гнездо заводится разновидностью IPv6 без запрета на IPv4: обмен ведётся
			 *       то одной сетью, то другой, а порт у описания устройства один - держать
			 *       под каждую сеть своё гнездо значило бы объявлять разные порты
			 */
			if(this->_tcp >= 0){
				int yes = 1, no = 0;
				::setsockopt(this->_tcp, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&yes), sizeof(yes));
				// Признак того, что одно гнездо служит обеим сетям
				const bool merged = (::setsockopt(this->_tcp, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast <const char *> (&no), sizeof(no)) == 0);
								::setReceiveTimeout(this->_tcp, 100);
				struct sockaddr_in6 address; ::memset(&address, 0, sizeof(address));
				address.sin6_family = AF_INET6; address.sin6_port = 0; address.sin6_addr = in6addr_any;
				if((::bind(this->_tcp, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0) || (::listen(this->_tcp, 8) != 0)){
					::closesocket(this->_tcp); this->_tcp = -1;
				} else {
					socklen_t length = sizeof(address);
					::getsockname(this->_tcp, reinterpret_cast <struct sockaddr *> (&address), &length);
					this->_port = ntohs(address.sin6_port);
					/**
					 * Если одно гнездо обеим сетям служить не может, заводим второе - сети IPv4
					 *
					 * @details Совмещение сетей на одном гнезде задаётся настройкой IPV6_V6ONLY
					 *          со значением лжи. OpenBSD отвечает на неё отказом «недопустимый
					 *          довод» - отображения адресов IPv4 в IPv6 там нет вовсе, - и
					 *          гнездо служит одной лишь сети IPv6. Обмен UPnP по IPv4 при этом
					 *          описания устройства забрать не мог, и проверки отказывали
					 *
					 * @note Второе гнездо встаёт на тот же порт, что и первое: в ответе рассылки
					 *       он объявляется один. Помехи в том нет - семейства адресов разные, а
					 *       совмещения, которое их бы столкнуло, система как раз и не даёт
					 *
					 */
					if(!merged){
						// Выполняем заведение гнезда управления устройством сети IPv4
						this->_tcp4 = ::socket(AF_INET, SOCK_STREAM, 0);
						// Если гнездо управления устройством сети IPv4 заведено
						if(this->_tcp4 >= 0){
							// Разрешаем повторное использование адреса гнезда
							::setsockopt(this->_tcp4, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&yes), sizeof(yes));
							// Устанавливаем срок ожидания обмена гнезда
							::setReceiveTimeout(this->_tcp4, 100);
							// Адрес, на котором гнездо принимает подключения
							struct sockaddr_in target; ::memset(&target, 0, sizeof(target));
							// Устанавливаем семейство адреса гнезда
							target.sin_family = AF_INET;
							// Устанавливаем порт гнезда, объявленный в ответе рассылки
							target.sin_port = htons(this->_port);
							// Устанавливаем приём подключений на всех устройствах машины
							target.sin_addr.s_addr = htonl(INADDR_ANY);
							// Если привязать гнездо к адресу либо открыть приём подключений не удалось
							if((::bind(this->_tcp4, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)) != 0) || (::listen(this->_tcp4, 8) != 0)){
								// Выполняем закрытие гнезда управления устройством сети IPv4
								::closesocket(this->_tcp4);
								// Сбрасываем дескриптор гнезда управления устройством сети IPv4
								this->_tcp4 = -1;
							}
						}
					}
				}
			}
		}
		~FakeIGD() noexcept { this->stop(); }
};
#endif
