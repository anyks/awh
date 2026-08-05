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
#include <vector>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
// Поддельное устройство доступа в сеть с поддержкой UPnP
class FakeIGD {
	public:
		// Как отвечать на вызов действия службы
		enum class Mode : uint8_t { OK, FAULT, TRUNCATED, GARBAGE, DROP, SLOW, HTTP_ERROR, NO_SERVICE, BAD_XML };
	private:
		int _udp;                       // Гнездо обнаружения устройств
		int _tcp;                       // Гнездо управления устройством
		uint16_t _port;                 // Порт гнезда управления устройством
		std::string _address;           // Адрес машины в местной сети
		std::thread _udpThread;
		std::thread _tcpThread;
		std::atomic <bool> _working;
		std::atomic <int> _calls;       // Количество принятых вызовов действий
		std::atomic <int> _fetches;     // Количество выданных описаний устройства
		std::atomic <int> _searches;    // Количество принятых рассылок обнаружения
	public:
		Mode mode = Mode::OK;           // Как отвечать на вызов действия
		bool answerSearch = true;       // Отвечать ли на рассылку обнаружения
		std::string iface = "127.0.0.1";
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
	public:
		bool ready() const noexcept { return ((this->_udp >= 0) && (this->_tcp >= 0) && !this->_address.empty()); }
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
			if(request.find("GetExternalIPAddress") != std::string::npos)
				return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
				       "<s:Body><u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
				       "<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress></u:GetExternalIPAddressResponse></s:Body></s:Envelope>";
			if(request.find("DeletePortMapping") != std::string::npos)
				return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
				       "<s:Body><u:DeletePortMappingResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/>"
				       "</s:Body></s:Envelope>";
			return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
			       "<s:Body><u:GenericResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/></s:Body></s:Envelope>";
		}
		// Отказ службы, записанный по правилам SOAP
		static std::string fault(){
			return "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><s:Fault>"
			       "<faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail>"
			       "<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\"><errorCode>718</errorCode>"
			       "<errorDescription>ConflictInMappingEntry</errorDescription></UPnPError></detail></s:Fault></s:Body></s:Envelope>";
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
					const ssize_t size = ::recvfrom(this->_udp, buffer, sizeof(buffer) - 1, 0, reinterpret_cast <struct sockaddr *> (&peer), &length);
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
					::sendto(this->_udp, answer, static_cast <size_t> (count), 0, reinterpret_cast <struct sockaddr *> (&peer), length);
				}
			});
			// Поток управления устройством
			this->_tcpThread = std::thread([this]() noexcept -> void {
				while(this->_working.load()){
					const int peer = ::accept(this->_tcp, nullptr, nullptr);
					if(peer < 0) continue;
					std::string request;
					// Читаем запрос целиком
					for(;;){
						char buffer[4096];
						const ssize_t size = ::recv(peer, buffer, sizeof(buffer), 0);
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
					if(request.empty()){ ::close(peer); continue; }
					const bool control = (request.compare(0, 4, "POST") == 0);
					if(control) this->_calls.fetch_add(1);
					else this->_fetches.fetch_add(1);
					std::string body, answer;
					const Mode mode = this->mode;
					if(control && (mode == Mode::DROP)){ ::close(peer); continue; }
					if(control && (mode == Mode::SLOW)){ ::usleep(900000); }
					if(control && (mode == Mode::GARBAGE)){
						const char * junk = "!!! это не разметка вовсе !!!";
						answer = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: ")
						       + std::to_string(::strlen(junk)) + "\r\nConnection: close\r\n\r\n" + junk;
						::send(peer, answer.data(), answer.size(), 0); ::close(peer); continue;
					}
					if(control && (mode == Mode::HTTP_ERROR)){
						answer = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
						::send(peer, answer.data(), answer.size(), 0); ::close(peer); continue;
					}
					if(control) body = ((mode == Mode::FAULT) ? fault() : this->soap(request));
					else body = this->description();
					const bool error = (control && (mode == Mode::FAULT));
					answer = std::string("HTTP/1.1 ") + (error ? "500 Internal Server Error" : "200 OK")
					       + "\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nContent-Length: "
					       + std::to_string(control && (mode == Mode::TRUNCATED) ? body.size() + 64 : body.size())
					       + "\r\nConnection: close\r\n\r\n" + body;
					::send(peer, answer.data(), answer.size(), 0);
					::close(peer);
				}
			});
		}
		void stop() noexcept {
			this->_working.store(false);
			if(this->_udp >= 0){ ::close(this->_udp); this->_udp = -1; }
			if(this->_tcp >= 0){ ::close(this->_tcp); this->_tcp = -1; }
			if(this->_udpThread.joinable()) this->_udpThread.join();
			if(this->_tcpThread.joinable()) this->_tcpThread.join();
		}
	public:
		FakeIGD() noexcept : _udp(-1), _tcp(-1), _port(0), _address(address()), _working(false), _calls(0), _fetches(0), _searches(0) {
			// Гнездо обнаружения устройств
			this->_udp = ::socket(AF_INET, SOCK_DGRAM, 0);
			if(this->_udp >= 0){
				int yes = 1;
				::setsockopt(this->_udp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
				::setsockopt(this->_udp, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
				struct timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 100000;
				::setsockopt(this->_udp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
				struct sockaddr_in address; ::memset(&address, 0, sizeof(address));
				address.sin_family = AF_INET; address.sin_port = htons(1900);
				address.sin_addr.s_addr = htonl(INADDR_ANY);
				if(::bind(this->_udp, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0){
					::close(this->_udp); this->_udp = -1;
				} else {
					// Вступаем в группу обнаружения устройств на устройстве петли
					struct ip_mreq group; ::memset(&group, 0, sizeof(group));
					group.imr_multiaddr.s_addr = ::inet_addr("239.255.255.250");
					group.imr_interface.s_addr = ::inet_addr(this->iface.c_str());
					if(::setsockopt(this->_udp, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) != 0)
						::fprintf(stderr, "вступление в группу не удалось: %s\n", ::strerror(errno));
				}
			}
			// Гнездо управления устройством
			this->_tcp = ::socket(AF_INET, SOCK_STREAM, 0);
			if(this->_tcp >= 0){
				int yes = 1;
				::setsockopt(this->_tcp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
				struct timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 100000;
				::setsockopt(this->_tcp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
				struct sockaddr_in address; ::memset(&address, 0, sizeof(address));
				address.sin_family = AF_INET; address.sin_port = 0;
				address.sin_addr.s_addr = htonl(INADDR_ANY);
				if((::bind(this->_tcp, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0) || (::listen(this->_tcp, 8) != 0)){
					::close(this->_tcp); this->_tcp = -1;
				} else {
					socklen_t length = sizeof(address);
					::getsockname(this->_tcp, reinterpret_cast <struct sockaddr *> (&address), &length);
					this->_port = ntohs(address.sin_port);
				}
			}
		}
		~FakeIGD() noexcept { this->stop(); }
};
#endif
