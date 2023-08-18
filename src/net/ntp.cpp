/**
 * @file: ntp.cpp
 * @date: 2023-08-17
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <net/ntp.hpp>

/**
 * host Метод извлечения хоста компьютера
 * @return хост компьютера с которого производится запрос
 */
string awh::NTP::Worker::host() const noexcept {
	// Результат работы функции
	string result = "";
	// Если список сетей установлен
	if(!this->_network.empty()){
		// Если количество элементов больше 1
		if(this->_network.size() > 1){
			// Подключаем устройство генератора
			mt19937 generator(const_cast <ntp_t *> (this->_self)->_randev());
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, this->_network.size() - 1);
			// Получаем ip адрес
			result = this->_network.at(dist6(generator));
		}
		// Выводим только первый элемент
		result = this->_network.front();
	}
	// Если IP-адрес не установлен
	if(result.empty()){
		// Определяем тип подключения
		switch(this->_family){
			// Для протокола IPv6
			case AF_INET6: return "::";
			// Для протокола IPv4
			case AF_INET: return "0.0.0.0";
		}
	}
	// Выводим результат
	return result;
}
/**
 * close Метод закрытия подключения
 */
void awh::NTP::Worker::close() noexcept {
	// Если файловый дескриптор не закрыт
	if(this->_fd != INVALID_SOCKET){
		/**
		 * Если операционной системой является Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем закрытие сокета
			closesocket(this->_fd);
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#else
			// Выполняем закрытие сокета
			::close(this->_fd);
		#endif
		// Выполняем сброс файлового дескриптора
		this->_fd = INVALID_SOCKET;
	}
}
/**
 * cancel Метод отмены выполнения запроса
 */
void awh::NTP::Worker::cancel() noexcept {
	// Если работа NTP-клиента запущена
	if(this->_mode){
		// Выполняем остановку работы резолвера
		this->_mode = !this->_mode;
		// Выполняем закрытие подключения
		this->close();
	}
}
/**
 * request Метод выполнения запроса
 * @return полученный UnixTimeStamp
 */
time_t awh::NTP::Worker::request() noexcept {
	// Результат работы функции
	time_t result = 0;
	// Получаем хост текущего компьютера
	const string & host = this->host();
	// Выполняем пересортировку серверов NTP
	const_cast <ntp_t *> (this->_self)->shuffle(this->_family);
	// Определяем тип подключения
	switch(this->_family){
		// Для протокола IPv4
		case AF_INET: {
			// Если список серверов существует
			if((this->_mode = !this->_self->_serversIPv4.empty())){
				// Создаём объект клиента
				struct sockaddr_in client;
				// Создаём объект сервера
				struct sockaddr_in server;
				// Запоминаем размер структуры
				this->_peer.size = sizeof(client);
				// Переходим по всему списку NTP-серверов
				for(auto & addr : this->_self->_serversIPv4){
					// Очищаем всю структуру для клиента
					memset(&client, 0, sizeof(client));
					// Очищаем всю структуру для сервера
					memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					client.sin_family = this->_family;
					// Устанавливаем протокол интернета
					server.sin_family = this->_family;
					// Устанавливаем произвольный порт для локального подключения
					client.sin_port = htons(0);
					// Устанавливаем порт для локального подключения
					server.sin_port = htons(addr.port);
					// Устанавливаем адрес для подключения
					memcpy(&server.sin_addr.s_addr, addr.ip, sizeof(addr.ip));
					// Устанавливаем адрес для локальго подключения
					inet_pton(this->_family, host.c_str(), &client.sin_addr.s_addr);
					// Выполняем копирование объекта подключения клиента
					memcpy(&this->_peer.client, &client, this->_peer.size);
					// Выполняем копирование объекта подключения сервера
					memcpy(&this->_peer.server, &server, this->_peer.size);
					// Обнуляем серверную структуру
					memset(&((struct sockaddr_in *) (&this->_peer.server))->sin_zero, 0, sizeof(server.sin_zero));
					{
						// Временный буфер данных для преобразования IP-адреса
						char buffer[INET_ADDRSTRLEN];
						// Выполняем запрос на удалённый NTP-сервер
						result = this->send(host, inet_ntop(this->_family, &addr.ip, buffer, sizeof(buffer)));
						// Если результат получен или получение данных закрыто, тогда выходим из цикла
						if((result > 0) || !this->_mode)
							// Выходим из цикла
							break;
					}
				}
			}
		} break;
		// Для протокола IPv6
		case AF_INET6: {
			// Если список серверов существует
			if((this->_mode = !this->_self->_serversIPv6.empty())){
				// Создаём объект клиента
				struct sockaddr_in6 client;
				// Создаём объект сервера
				struct sockaddr_in6 server;
				// Запоминаем размер структуры
				this->_peer.size = sizeof(client);
				// Переходим по всему списку NTP-серверов
				for(auto & addr : this->_self->_serversIPv6){
					// Очищаем всю структуру для клиента
					memset(&client, 0, sizeof(client));
					// Очищаем всю структуру для сервера
					memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					client.sin6_family = this->_family;
					// Устанавливаем протокол интернета
					server.sin6_family = this->_family;
					// Устанавливаем произвольный порт для локального подключения
					client.sin6_port = htons(0);
					// Устанавливаем порт для локального подключения
					server.sin6_port = htons(addr.port);
					// Устанавливаем адрес для подключения
					memcpy(&server.sin6_addr, addr.ip, sizeof(addr.ip));
					// Устанавливаем адрес для локальго подключения
					inet_pton(this->_family, host.c_str(), &client.sin6_addr);
					// Выполняем копирование объекта подключения клиента
					memcpy(&this->_peer.client, &client, this->_peer.size);
					// Выполняем копирование объекта подключения сервера
					memcpy(&this->_peer.server, &server, this->_peer.size);
					{
						// Временный буфер данных для преобразования IP-адреса
						char buffer[INET6_ADDRSTRLEN];
						// Выполняем запрос на удалённый NTP-сервер
						result = this->send(host, inet_ntop(this->_family, &addr.ip, buffer, sizeof(buffer)));
						// Если результат получен или получение данных закрыто, тогда выходим из цикла
						if((result > 0) || !this->_mode)
							// Выходим из цикла
							break;
					}
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * Метод отправки запроса на удалённый сервер NTP
 * @param from адрес компьютера с которого выполняется запрос
 * @param to   адрес NTP-сервера на который выполняется запрос
 * @return     полученный UnixTimeStamp
 */
time_t awh::NTP::Worker::send(const string & from, const string & to) noexcept {
	// Результат работы функции
	time_t result = 0;
	// Если доменное имя установлено
	if(this->_mode && !from.empty() && !to.empty()){
		// Создаём объект пакета запроса
		packet_t packet;
		// Устанавливаем типа пакета
		packet.mode = 0x1B;
		// Создаём сокет подключения
		this->_fd = ::socket(this->_family, SOCK_DGRAM, IPPROTO_UDP);
		// Если сокет не создан создан и работа резолвера не остановлена
		if(this->_mode && (this->_fd == INVALID_SOCKET)){
			// Выводим в лог сообщение
			this->_self->_log->print("file descriptor needed for the NTP request could not be allocated", log_t::flag_t::WARNING);
			// Выходим из приложения
			return result;
		// Если сокет создан удачно и работа резолвера не остановлена
		} else if(this->_mode) {
			// Количество отправленных или полученных байт
			int64_t bytes = 0;
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->_fd);
			// Устанавливаем разрешение на закрытие сокета при неиспользовании
			this->_socket.closeOnExec(this->_fd);
			// Устанавливаем размер буфера передачи данных на чтение
			// this->_socket.bufferSize(this->_fd, sizeof(packet) * 2, 1, socket_t::mode_t::READ);
			// Устанавливаем размер буфера передачи данных на запись
			// this->_socket.bufferSize(this->_fd, sizeof(packet) * 2, 1, socket_t::mode_t::WRITE);
			// Устанавливаем таймаут на получение данных из сокета
			this->_socket.timeout(this->_fd, this->_self->_timeout * 1000, socket_t::mode_t::READ);
			// Устанавливаем таймаут на запись данных в сокет
			this->_socket.timeout(this->_fd, this->_self->_timeout * 1000, socket_t::mode_t::WRITE);
			// Выполняем бинд на сокет
			if(::bind(this->_fd, (struct sockaddr *) (&this->_peer.client), this->_peer.size) < 0){
				// Выводим в лог сообщение
				this->_self->_log->print("bind local network [%s]", log_t::flag_t::CRITICAL, from.c_str());
				// Выходим из функции
				return result;
			}
			// Если запрос на NTP-сервер успешно отправлен
			if((bytes = ::sendto(this->_fd, (const char *) &packet, sizeof(packet), 0, (struct sockaddr *) &this->_peer.server, this->_peer.size)) > 0){
				// Получаем объект NTP-клиента
				ntp_t * self = const_cast <ntp_t *> (this->_self);
				// Выполняем чтение ответа сервера
				const int64_t bytes = ::recvfrom(this->_fd, (char *) &packet, sizeof(packet), 0, (struct sockaddr *) &this->_peer.server, &this->_peer.size);
				// Если данные прочитать не удалось
				if(bytes <= 0){
					// Если сокет находится в блокирующем режиме
					if(bytes < 0){
						// Определяем тип ошибки
						switch(errno){
							// Если ошибка не обнаружена, выходим
							case 0: break;
							// Если произведена неудачная запись в PIPE
							case EPIPE:
								// Выводим в лог сообщение
								self->_log->print("EPIPE [server = %s]", log_t::flag_t::WARNING, to.c_str());
							break;
							// Если произведён сброс подключения
							case ECONNRESET:
								// Выводим в лог сообщение
								self->_log->print("ECONNRESET [server = %s]", log_t::flag_t::WARNING, to.c_str());
							break;
							// Для остальных ошибок
							default:
								// Выводим в лог сообщение
								self->_log->print("%s [server = %s]", log_t::flag_t::WARNING, strerror(errno), to.c_str());
						}
					}
					// Если работа резолвера ещё не остановлена
					if(this->_mode){
						// Выполняем закрытие подключения
						this->close();
						// Замораживаем поток на период времени в 10ms
						this_thread::sleep_for(10ms);
					}
					// Выполняем попытку получить IP-адрес с другого сервера
					return result;
				// Если данные получены удачно
				} else {
					/**
					 * Эти два поля содержат метку времени в секундах, когда пакет покинул сервер NTP.
					 * Количество секунд соответствует секундам, прошедшим с 1900 года.
					 * ntohl() преобразует порядок битов/байтов из сетевого в "порядок байтов" хоста.
					 */
					// Получаем штамп времени в секундах
					packet.transmitedTimeStampSec = ntohl(packet.transmitedTimeStampSec);
					// Получаем штамп времени в долях секунды
					packet.transmitedTimeStampSecFrac = ntohl(packet.transmitedTimeStampSecFrac);
					/**
					 * Извлекаем 32 бита, которые представляют собой метку времени в секундах (начиная с эпохи NTP) с момента, когда пакет покинул сервер.
					 * Вычитаем 70 лет секунд из секунд с 1900 года. Это оставит секунды с эпохи UNIX 1970 года.
					 * (1900)---------(1970)**********(Время когд пакет покинул сервер)
					 */
					// Получаем количество секунд с UNIX-эпохи
					return (static_cast <time_t> (packet.transmitedTimeStampSec - NTP_TIMESTAMP_DELTA) * 1000l);
				}
				// Если мы получили результат
				if(result > 0)
					// Выводим результат
					return result;
			// Если сообщение отправить не удалось
			} else if(bytes <= 0) {
				// Если сокет находится в блокирующем режиме
				if(bytes < 0){
					// Определяем тип ошибки
					switch(errno){
						// Если ошибка не обнаружена, выходим
						case 0: break;
						// Если произведена неудачная запись в PIPE
						case EPIPE:
							// Выводим в лог сообщение
							this->_self->_log->print("EPIPE [server = %s]", log_t::flag_t::WARNING, to.c_str());
						break;
						// Если произведён сброс подключения
						case ECONNRESET:
							// Выводим в лог сообщение
							this->_self->_log->print("ECONNRESET [server = %s]", log_t::flag_t::WARNING, to.c_str());
						break;
						// Для остальных ошибок
						default:
							// Выводим в лог сообщение
							this->_self->_log->print("%s [server = %s]", log_t::flag_t::WARNING, strerror(errno), to.c_str());
					}
				}
			}
			// Выполняем закрытие подключения
			this->close();
		}
	}
	// Выводим результат
	return result;
}
/**
 * ~Worker Деструктор
 */
awh::NTP::Worker::~Worker() noexcept {
	// Выполняем закрытие файлового дерскриптора (сокета)
	this->close();
}
/**
 * clear Метод очистки данных NTP-клиента
 * @return результат работы функции
 */
bool awh::NTP::clear() noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы NTP-клиента соответствует
	if((result = hold.access({}, status_t::CLEAR))){
		// Выполняем отмену выполненных запросов IPv4
		this->cancel(AF_INET);
		// Выполняем отмену выполненных запросов IPv6
		this->cancel(AF_INET6);
		// Выполняем сброс списока серверов IPv4
		this->replace(AF_INET);
		// Выполняем сброс списока серверов IPv6
		this->replace(AF_INET6);
	}
	// Выводим результат
	return result;
}
/**
 * cancel Метод отмены выполнения запроса
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::NTP::cancel(const int family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Определяем тип протокола подключения
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int> (AF_INET):
			// Выполняем отмену резолвинга домена
			this->_workerIPv4->cancel();
		break;
		// Если тип протокола подключения IPv6
		case static_cast <int> (AF_INET6):
			// Выполняем отмену резолвинга домена
			this->_workerIPv6->cancel();
		break;
	}
}
/**
 * shuffle Метод пересортировки серверов NTP
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::NTP::shuffle(const int family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выбираем стаднарт рандомайзера
	mt19937 generator(this->_randev());
	// Определяем тип протокола подключения
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int> (AF_INET):
			// Выполняем рандомную сортировку списка NTP-серверов
			::shuffle(this->_serversIPv4.begin(), this->_serversIPv4.end(), generator);
		break;
		// Если тип протокола подключения IPv6
		case static_cast <int> (AF_INET6):
			// Выполняем рандомную сортировку списка NTP-серверов
			::shuffle(this->_serversIPv6.begin(), this->_serversIPv6.end(), generator);
		break;
	}
}
/**
 * timeout Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::NTP::timeout(const uint8_t sec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем установку таймаута ожидания выполнения запроса
	this->_timeout = sec;
}
/**
 * ns Метод добавления DNS-серверов
 * @param servers адреса DNS-серверов
 */
void awh::NTP::ns(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если DNS-сервера получены
	if(!servers.empty())
		// Выполняем установку списка DNS-серверов
		this->_dns.servers(servers);
}
/**
 * server Метод получения данных NTP-сервера
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       запрошенный NTP-сервер
 */
string awh::NTP::server(const int family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Результат работы функции
	string result = "";
	// Подключаем устройство генератора
	mt19937 generator(this->_randev());
	// Определяем тип протокола подключения
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int> (AF_INET): {
			// Временный буфер данных для преобразования IP-адреса
			char buffer[INET_ADDRSTRLEN];
			// Если список серверов пустой
			if(this->_serversIPv4.empty())
				// Устанавливаем новый список имён
				this->replace(family);
			// Получаем первое значение итератора
			auto it = this->_serversIPv4.begin();
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, this->_serversIPv4.size() - 1);
			// Выполняем выбор нужного сервера в списке, в произвольном виде
			advance(it, dist6(generator));
			// Выполняем получение данных IP-адреса
			result = inet_ntop(family, &it->ip, buffer, sizeof(buffer));
		} break;
		// Если тип протокола подключения IPv6
		case static_cast <int> (AF_INET6): {
			// Временный буфер данных для преобразования IP-адреса
			char buffer[INET6_ADDRSTRLEN];
			// Если список серверов пустой
			if(this->_serversIPv6.empty())
				// Устанавливаем новый список имён
				this->replace(family);
			// Получаем первое значение итератора
			auto it = this->_serversIPv6.begin();
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, this->_serversIPv6.size() - 1);
			// Выполняем выбор нужного сервера в списке, в произвольном виде
			advance(it, dist6(generator));
			// Выполняем получение данных IP-адреса
			result = inet_ntop(family, &it->ip, buffer, sizeof(buffer));
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * server Метод добавления NTP-сервера
 * @param server адрес NTP-сервера
 */
void awh::NTP::server(const string & server) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если адрес сервера передан
	if(!server.empty()){
		// Определяем тип передаваемого IP-адреса
		switch(static_cast <uint8_t> (this->_net.host(server))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем добавление IPv4 адреса в список серверов
				this->server(AF_INET, server);
			break;
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем добавление IPv6 адреса в список серверов
				this->server(AF_INET6, server);
			break;
		}
	}
}
/**
 * server Метод добавления NTP-сервера
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param server адрес NTP-сервера
 */
void awh::NTP::server(const int family, const string & server) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы NTP-клиента соответствует
	if(hold.access({status_t::NTSS_SET}, status_t::NTS_SET)){
		// Если адрес сервера передан
		if(!server.empty()){
			// Порт переданного сервера
			u_int port = 123;
			// Хост переданного сервера
			string host = "";
			// Определяем тип передаваемого сервера
			switch(static_cast <uint8_t> (this->_net.host(server))){
				// Если домен является аппаратным адресом сетевого интерфейса
				case static_cast <uint8_t> (net_t::type_t::MAC):
				// Если домен является адресом/Маски сети
				case static_cast <uint8_t> (net_t::type_t::NETW):
				// Если домен является адресом в файловой системе
				case static_cast <uint8_t> (net_t::type_t::ADDR):
				// Если домен является HTTP адресом
				case static_cast <uint8_t> (net_t::type_t::HTTP): break;
				// Если хост является IPv4 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4): {
					// Выполняем поиск разделителя порта
					const size_t pos = server.rfind(":");
					// Если позиция разделителя найдена
					if(pos != string::npos){
						// Извлекаем хост сервера имён
						host = server.substr(0, pos);
						// Извлекаем порт сервера имён
						port = static_cast <u_int> (::stoi(server.substr(pos + 1)));
					// Извлекаем хост сервера имён
					} else host = server;
				} break;
				// Если хост является IPv6 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6): {
					// Если первый символ является разделителем
					if(server.front() == '['){
						// Выполняем поиск разделителя порта
						const size_t pos = server.rfind("]:");
						// Если позиция разделителя найдена
						if(pos != string::npos){
							// Извлекаем хост сервера имён
							host = server.substr(1, pos - 1);
							// Запоминаем полученный порт
							port = static_cast <u_int> (::stoi(server.substr(pos + 2)));
						// Заполняем полученный сервер
						} else if(server.back() == ']')
							// Извлекаем хост сервера имён
							host = server.substr(1, server.size() - 2);
					// Заполняем полученный сервер
					} else if(server.back() != ']')
						// Извлекаем хост сервера имён
						host = server;
				} break;
				// Если хост является доменным именем
				case static_cast <uint8_t> (net_t::type_t::DOMN): {
					// Выполняем поиск разделителя порта
					const size_t pos = server.rfind(":");
					// Если позиция разделителя найдена
					if(pos != string::npos){
						// Извлекаем хост сервера имён
						host = server.substr(0, pos);
						// Извлекаем порт сервера имён
						port = static_cast <u_int> (::stoi(server.substr(pos + 1)));
					// Извлекаем хост сервера имён
					} else host = server;
					// Выполняем получение IP адрес хоста доменного имени
					string ip = this->_dns.host(family, host);
					// Если IP-адрес мы не получили, выполняем запрос на сервер
					if(ip.empty())
						// Выполняем запрос IP-адреса с удалённого сервера
						host = this->_dns.resolve(family, host);
					// Выполняем установку IP-адреса
					else host = std::move(ip);
				} break;
				// Значит скорее всего, садрес является доменным именем
				default: {
					// Выполняем поиск разделителя порта
					const size_t pos = server.rfind(":");
					// Если позиция разделителя найдена
					if(pos != string::npos){
						// Извлекаем хост сервера имён
						host = this->_dns.host(family, server.substr(0, pos));
						// Извлекаем порт сервера имён
						port = static_cast <u_int> (::stoi(server.substr(pos + 1)));
					// Извлекаем хост сервера имён
					} else host = this->_dns.host(family, server);
				}
			}
			// Если NTP-сервер имён получен
			if(!host.empty()){
				// Определяем тип протокола подключения
				switch(family){
					// Если тип протокола подключения IPv4
					case static_cast <int> (AF_INET): {
						// Создаём объект сервера NTP
						server_t <1> server;
						// Запоминаем полученный порт
						server.port = port;
						// Запоминаем полученный сервер
						inet_pton(family, host.c_str(), &server.ip);
						// Если добавляемый хост сервера ещё не существует в списке серверов
						if(std::find_if(this->_serversIPv4.begin(), this->_serversIPv4.end(), [&server](const server_t <1> & item) noexcept -> bool {
							// Выполняем сравнение двух IP-адресов
							return (::memcmp(&server.ip, &item.ip, sizeof(item.ip)) == 0);
						}) == this->_serversIPv4.end()){
							// Выполняем добавление полученный сервер в список NTP-серверов
							this->_serversIPv4.push_back(std::move(server));
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим заголовок запроса
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD NTP SERVER ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры запроса
								cout << host << ":" << port << endl;
							#endif
						}
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <int> (AF_INET6): {
						// Создаём объект сервера NTP
						server_t <4> server;
						// Запоминаем полученный порт
						server.port = port;
						// Запоминаем полученный сервер
						inet_pton(family, host.c_str(), &server.ip);
						// Если добавляемый хост сервера ещё не существует в списке серверов
						if(std::find_if(this->_serversIPv6.begin(), this->_serversIPv6.end(), [&server](const server_t <4> & item) noexcept -> bool {
							// Выполняем сравнение двух IP-адресов
							return (::memcmp(&server.ip, &item.ip, sizeof(item.ip)) == 0);
						}) == this->_serversIPv6.end()){
							// Выполняем добавление полученный сервер в список NTP-серверов
							this->_serversIPv6.push_back(std::move(server));
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим заголовок запроса
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD NTP SERVER ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры запроса
								cout << "[" << host << "]:" << port << endl;
							#endif
						}
					} break;
				}
			// Если имя сервера не получено, выводим в лог сообщение
			} else this->_log->print("NTP IPv%u server %s does not add", log_t::flag_t::WARNING, (family == AF_INET6 ? 6 : 4), server.c_str());
		}
	}
}
/**
 * servers Метод добавления NTP-серверов
 * @param servers адреса NTP-серверов
 */
void awh::NTP::servers(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если список серверов передан
	if(!servers.empty()){
		// Создаём объект холдирования
		hold_t <status_t> hold(this->_status);
		// Если статус работы NTP-клиента соответствует
		if(hold.access({status_t::NTSS_REP}, status_t::NTSS_SET)){
			// Переходим по всем нейм серверам и добавляем их
			for(auto & server : servers){
				// Определяем тип передаваемого IP-адреса
				switch(static_cast <uint8_t> (this->_net.host(server))){
					// Если домен является аппаратным адресом сетевого интерфейса
					case static_cast <uint8_t> (net_t::type_t::MAC):
					// Если домен является адресом/Маски сети
					case static_cast <uint8_t> (net_t::type_t::NETW):
					// Если домен является адресом в файловой системе
					case static_cast <uint8_t> (net_t::type_t::ADDR):
					// Если домен является HTTP адресом
					case static_cast <uint8_t> (net_t::type_t::HTTP): break;
					// Если IP-адрес является IPv4 адресом
					case static_cast <uint8_t> (net_t::type_t::IPV4):
						// Выполняем добавление IPv4 адреса в список серверов
						this->server(AF_INET, server);
					break;
					// Если IP-адрес является IPv6 адресом
					case static_cast <uint8_t> (net_t::type_t::IPV6):
						// Выполняем добавление IPv6 адреса в список серверов
						this->server(AF_INET6, server);
					break;
					// Для всех остальных адресов
					default: {
						// Выполняем добавление IPv4 адреса в список серверов
						this->server(AF_INET, server);
						// Выполняем добавление IPv6 адреса в список серверов
						this->server(AF_INET6, server);
					}
				}
			}
		}
	}
}
/**
 * servers Метод добавления NTP-серверов
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers адреса NTP-серверов
 */
void awh::NTP::servers(const int family, const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если список серверов передан
	if(!servers.empty()){	
		// Создаём объект холдирования
		hold_t <status_t> hold(this->_status);
		// Если статус работы NTP-резолвера соответствует
		if(hold.access({status_t::NTSS_REP}, status_t::NTSS_SET)){
			// Переходим по всем нейм серверам и добавляем их
			for(auto & server : servers)
				// Выполняем добавление нового сервера
				this->server(family, server);
		}
	}
}
/**
 * replace Метод замены существующих NTP-серверов
 * @param servers адреса NTP-серверов
 */
void awh::NTP::replace(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы NTP-клиента соответствует
	if(hold.access({status_t::REQUEST}, status_t::NTSS_REP)){
		// Список серверов IPv4
		vector <string> ipv4, ipv6;
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : servers){
			// Определяем тип передаваемого IP-адреса
			switch(static_cast <uint8_t> (this->_net.host(server))){
				// Если домен является аппаратным адресом сетевого интерфейса
				case static_cast <uint8_t> (net_t::type_t::MAC):
				// Если домен является адресом/Маски сети
				case static_cast <uint8_t> (net_t::type_t::NETW):
				// Если домен является адресом в файловой системе
				case static_cast <uint8_t> (net_t::type_t::ADDR):
				// Если домен является HTTP адресом
				case static_cast <uint8_t> (net_t::type_t::HTTP): break;
				// Если IP-адрес является IPv4 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4):
					// Выполняем добавление IPv4 адреса в список серверов
					ipv4.push_back(server);
				break;
				// Если IP-адрес является IPv6 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6):
					// Выполняем добавление IPv6 адреса в список серверов
					ipv6.push_back(server);
				break;
				// Для всех остальных адресов
				default: {
					// Выполняем добавление IPv4 адреса в список серверов
					ipv4.push_back(server);
					// Выполняем добавление IPv6 адреса в список серверов
					ipv6.push_back(server);
				}
			}
		}
		// Выполняем замену списка серверов
		this->replace(AF_INET, ipv4);
		// Выполняем замену списка серверов
		this->replace(AF_INET6, ipv6);
	}
}
/**
 * replace Метод замены существующих NTP-серверов
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers адреса NTP-серверов
 */
void awh::NTP::replace(const int family, const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы NTP-клиента соответствует
	if(hold.access({status_t::REQUEST}, status_t::NTSS_REP)){
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case static_cast <int> (AF_INET):
				// Выполняем очистку списка NTP-серверов
				this->_serversIPv4.clear();
			break;
			// Для протокола IPv6
			case static_cast <int> (AF_INET6):
				// Выполняем очистку списка NTP-серверов
				this->_serversIPv6.clear();
			break;
		}
		// Если нейм сервера переданы, удаляем все настроенные NTP=серверы и приостанавливаем все ожидающие решения
		if(!servers.empty())
			// Устанавливаем новый список серверов
			this->servers(family, servers);
		// Если список серверов не передан
		else {
			// Список серверов
			vector <string> servers;
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case static_cast <int> (AF_INET):
					// Устанавливаем список серверов
					servers = IPV4_NTP;
				break;
				// Для протокола IPv6
				case static_cast <int> (AF_INET6):
					// Устанавливаем список серверов
					servers = IPV6_NTP;
				break;
			}
			// Устанавливаем новый список серверов
			this->servers(family, std::move(servers));
		}
	}
}
/**
 * network Метод установки адреса сетевых плат, с которых нужно выполнять запросы
 * @param network IP-адреса сетевых плат
 */
void awh::NTP::network(const vector <string> & network) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы установки параметров сети соответствует
	if(hold.access({}, status_t::NET_SET)){
		// Если список адресов сетевых плат передан
		if(!network.empty()){
			// Выполняем добавление полученных сетей в DNS-резолвер
			this->_dns.network(network);
			// Переходим по всему списку полученных адресов
			for(auto & host : network){
				// Определяем к какому адресу относится полученный хост
				switch(static_cast <uint8_t> (this->_net.host(host))){
					// Если домен является аппаратным адресом сетевого интерфейса
					case static_cast <uint8_t> (net_t::type_t::MAC):
					// Если домен является адресом/Маски сети
					case static_cast <uint8_t> (net_t::type_t::NETW):
					// Если домен является адресом в файловой системе
					case static_cast <uint8_t> (net_t::type_t::ADDR):
					// Если домен является HTTP адресом
					case static_cast <uint8_t> (net_t::type_t::HTTP): break;
					// Если IP-адрес является IPv4 адресом
					case static_cast <uint8_t> (net_t::type_t::IPV4):
						// Выполняем добавление полученного хоста в список
						this->_workerIPv4->_network.push_back(host);
					break;
					// Если IP-адрес является IPv6 адресом
					case static_cast <uint8_t> (net_t::type_t::IPV6):
						// Выполняем добавление полученного хоста в список
						this->_workerIPv6->_network.push_back(host);
					break;
					// Для всех остальных адресов
					default: {
						// Выполняем получение IP-адреса для IPv6
						string ip = this->_dns.host(AF_INET6, host);
						// Если результат получен, выполняем пинг
						if(!ip.empty())
							// Выполняем добавление полученного хоста в список
							this->_workerIPv6->_network.push_back(ip);
						// Если результат не получен, выполняем получение IPv4 адреса
						else {
							// Выполняем получение IP-адреса для IPv4
							ip = this->_dns.host(AF_INET, host);
							// Если IP-адрес успешно получен
							if(!ip.empty())
								// Выполняем добавление полученного хоста в список
								this->_workerIPv4->_network.push_back(ip);
							// Выводим сообщение об ошибке
							else this->_log->print("passed %s address is not legitimate", log_t::flag_t::WARNING, host.c_str());
						}
					}
				}
			}
		}
	}
}
/**
 * network Метод установки адреса сетевых плат, с которых нужно выполнять запросы
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param network IP-адреса сетевых плат
 */
void awh::NTP::network(const int family, const vector <string> & network) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы установки параметров сети соответствует
	if(hold.access({}, status_t::NET_SET)){
		// Если список адресов сетевых плат передан
		if(!network.empty()){
			// Выполняем добавление полученных сетей в DNS-резолвер
			this->_dns.network(family, network);
			// Переходим по всему списку полученных адресов
			for(auto & host : network){
				// Определяем тип передаваемого IP-адреса
				switch(family){
					// Если IP-адрес является IPv4 адресом
					case static_cast <int> (AF_INET):
						// Выполняем добавление полученного хоста в список
						this->_workerIPv4->_network.push_back(host);
					break;
					// Если IP-адрес является IPv6 адресом
					case static_cast <int> (AF_INET6):
						// Выполняем добавление полученного хоста в список
						this->_workerIPv6->_network.push_back(host);
					break;
				}
			}
		}
	}
}
/**
 * request Метод выполнение получения времени с NTP-сервера
 * @return полученный UnixTimeStamp
 */
time_t awh::NTP::request() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем получение UnixTimeStamp для IPv6
	const time_t result = this->request(AF_INET6);
	// Если результат не получен, выполняем запрос для IPv4
	if(result == 0)
		// Выполняем получение UnixTimeStamp для IPv4
		return this->request(AF_INET);
	// Выводим результат
	return result;
}
/**
 * request Метод выполнение получения времени с NTP-сервера
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       полученный UnixTimeStamp
 */
time_t awh::NTP::request(const int family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы NTP-клиента соответствует
	if(hold.access({}, status_t::REQUEST)){
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Если список NTP-серверов пустой
				if(this->_serversIPv4.empty())
					// Устанавливаем список серверов IPv4
					this->replace(AF_INET);
				// Выполняем получение UnixTimeStamp
				return this->_workerIPv4->request();
			}
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Если список NTP-серверов пустой
				if(this->_serversIPv6.empty())
					// Устанавливаем список серверов IPv6
					this->replace(AF_INET6);
				// Выполняем получение UnixTimeStamp
				return this->_workerIPv6->request();
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * NTP Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::NTP::NTP(const fmk_t * fmk, const log_t * log) noexcept :
 _dns(fmk, log), _ttl(0), _timeout(5),
 _workerIPv4(nullptr), _workerIPv6(nullptr), _fmk(fmk), _log(log) {
	// Выполняем создание воркера для IPv4
	this->_workerIPv4 = unique_ptr <worker_t> (new worker_t(AF_INET, this));
	// Выполняем создание воркера для IPv6
	this->_workerIPv6 = unique_ptr <worker_t> (new worker_t(AF_INET6, this));
}
/**
 * ~NTP Деструктор
 */
awh::NTP::~NTP() noexcept {
	// Выполняем очистку модуля NTP-клиента
	this->clear();
}
