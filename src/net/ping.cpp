/**
 * @file: ping.cpp
 * @date: 2023-08-11
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

/**
 * Чтобы пинг работал от непривилигированного пользователя
 * Linux:
 * # (где 0 и 0 - это UID и GID в операционной системе)
 * $ sysctl -w net.ipv4.ping_group_range="0 0"
 * 
 * FreeBSD:
 * # Нужно сменить владельца приложения на root и разрешить запуск остальным пользователям, где ./app/ping - ваше приложение
 * $ sudo chown root:wheel ./app/ping
 * $ sudo chmod 710 ./app/ping
 * $ sudo chmod ug+s ./app/ping
 */

// Подключаем заголовочный файл
#include <net/ping.hpp>

/**
 * host Метод извлечения хоста компьютера
 * @param family семейство сокета (AF_INET / AF_INET6)
 * @return       хост компьютера с которого производится запрос
 */
string awh::Ping::host(const int32_t family) const noexcept {
	// Результат работы функции
	string result = "";
	// Список адресов сетевых карт
	const vector <string> * network = nullptr;
	// Определяем тип подключения
	switch(family){
		// Для протокола IPv4
		case AF_INET:
			// Получаем список адресов для IPv4
			network = &this->_networkIPv4;
		break;
		// Для протокола IPv6
		case AF_INET6:
			// Получаем список адресов для IPv6
			network = &this->_networkIPv6;
		break;
	}
	// Если список сетей установлен
	if((network != nullptr) && !network->empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если количество элементов больше 1
			if(network->size() > 1){
				// Подключаем устройство генератора
				std::mt19937 generator(const_cast <ping_t *> (this)->_randev());
				// Выполняем генерирование случайного числа
				std::uniform_int_distribution <std::mt19937::result_type> dist6(0, network->size() - 1);
				// Получаем ip адрес
				result = network->at(dist6(generator));
			// Выводим только первый элемент
			} else result = network->front();
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(family), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
			// Выводим только первый элемент
			result = network->front();
		}
	}
	// Если IP-адрес не установлен
	if(result.empty()){
		// Определяем тип подключения
		switch(family){
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
 * checksum Метод подсчёта контрольной суммы
 * @param buffer буфер данных для подсчёта
 * @param size   размер данных для подсчёта
 * @return       подсчитанная контрольная сумма
 */
uint16_t awh::Ping::checksum(const void * buffer, const size_t size) noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если данные переданы верные
	if((buffer != nullptr) && (size > 0)){
		// Контрольная сумма расчёта
		uint32_t sum = 0;
		// Устанавливаем длину контрольной суммы
		size_t length = size;
		// Выполняем приведение буфера в нужную нам форму
		auto data = reinterpret_cast <const uint16_t *> (buffer);
		// Если длина буфера всего один байт
		if(length & 1)
			// Выполняем расчёт контрольной суммы
			sum = reinterpret_cast <const uint8_t *> (data)[length - 1];
		// Делим длину байт пополам
		length /= 2;
		// Выполняем перебор буфера байт
		while(length--){
			// Выполняем расчёт контрольной суммы
			sum += * data++;
			// Если контрольная сумма достигла предела
			if(sum & 0xffff0000)
				// Выполняем смещение на оставшиеся 16 байт
				sum = ((sum >> 16) + (sum & 0xffff));
		}
		// Выполняем получение результата контрольной суммы
		result = static_cast <uint16_t> (~sum);
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки запроса на сервер
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param index  индекс последовательности
 * @return       количество прочитанных байт
 */
int64_t awh::Ping::send(const int32_t family, const size_t index) noexcept {
	// Результат работы функции
	int64_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Подключаем устройство генератора
		std::mt19937 generator(this->_randev());
		// Выполняем генерирование случайного числа
		std::uniform_int_distribution <std::mt19937::result_type> dist6(0, std::numeric_limits <uint32_t>::max() - 1);
		// Создаём объект заголовков
		struct IcmpHeader icmp{};
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case AF_INET:
				// Выполняем установку типа запроса
				icmp.type = 8;
			break;
			// Для протокола IPv6
			case AF_INET6:
				// Выполняем установку типа запроса
				icmp.type = 128;
			break;
		}
		// Устанавливаем код запроса
		icmp.code = 0;
		// Устанавливаем контрольную сумму
		icmp.checksum = 0;
		// Устанавливаем номер последовательности
		icmp.meta.echo.sequence = index;
		// Устанавливаем идентификатор запроса
		icmp.meta.echo.identifier = ::getpid();
		// Устанавливаем данные полезной нагрузки
		icmp.meta.echo.payload = static_cast <uint64_t> (dist6(generator));
		// Выполняем подсчёт контрольной суммы
		icmp.checksum = this->checksum(&icmp, sizeof(icmp));
		// Если запрос на сервер успешно отправлен
		if((result = static_cast <int64_t> (::sendto(this->_fd, reinterpret_cast <char *> (&icmp), sizeof(icmp), 0, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size))) > 0){
			// Буфер для получения данных
			std::array <char, 1024> buffer;
			// Результат полученных данных
			auto * icmpResponseHeader = reinterpret_cast <struct IcmpHeader *> (buffer.data());
			// Выполняем чтение ответа сервера
			result = static_cast <int64_t> (::recvfrom(this->_fd, reinterpret_cast <char *> (icmpResponseHeader), sizeof(buffer), 0, reinterpret_cast <struct sockaddr *> (&this->_peer.server), &this->_peer.size));
			// Если данные прочитать не удалось
			if(result <= 0){
				// Если сокет находится в блокирующем режиме
				if(result < 0){
					// Определяем тип ошибки
					switch(AWH_ERROR()){
						// Если ошибка не обнаружена, выходим
						case 0: break;
						/**
						 * Если мы работаем не в MS Windows
						 */
						#if !defined(_WIN32) && !defined(_WIN64)
							// Если произведена неудачная запись в PIPE
							case EPIPE: {
								// Если разрешено выводить информацию в лог
								if(this->_verb)
									// Выводим в лог сообщение
									this->_log->print("EPIPE", log_t::flag_t::WARNING);
							} break;
							// Если произведён сброс подключения
							case ECONNRESET: {
								// Если разрешено выводить информацию в лог
								if(this->_verb)
									// Выводим в лог сообщение
									this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
							} break;
						/**
						 * Методы только для OS Windows
						 */
						#else
							// Если произведён сброс подключения
							case WSAECONNRESET: {
								// Если разрешено выводить информацию в лог
								if(this->_verb)
									// Выводим в лог сообщение
									this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
							} break;
						#endif
						// Для остальных ошибок
						default: {
							// Если разрешено выводить информацию в лог
							if(this->_verb)
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::WARNING, this->_socket.message(AWH_ERROR()).c_str());
						}
					}
					// Выводим результат
					return result;
				}
			}
		// Если сообщение отправить не удалось
		} else if(result <= 0) {
			// Если сокет находится в блокирующем режиме
			if(result < 0){
				// Определяем тип ошибки
				switch(AWH_ERROR()){
					// Если ошибка не обнаружена, выходим
					case 0: break;
					/**
					 * Если мы работаем не в MS Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						// Если произведена неудачная запись в PIPE
						case EPIPE: {
							// Если разрешено выводить информацию в лог
							if(this->_verb)
								// Выводим в лог сообщение
								this->_log->print("EPIPE", log_t::flag_t::WARNING);
						} break;
						// Если произведён сброс подключения
						case ECONNRESET: {
							// Если разрешено выводить информацию в лог
							if(this->_verb)
								// Выводим в лог сообщение
								this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
						} break;
					/**
					 * Методы только для OS Windows
					 */
					#else
						// Если произведён сброс подключения
						case WSAECONNRESET: {
							// Если разрешено выводить информацию в лог
							if(this->_verb)
								// Выводим в лог сообщение
								this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
						} break;
					#endif
					// Для остальных ошибок
					default: {
						// Если разрешено выводить информацию в лог
						if(this->_verb)
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::WARNING, this->_socket.message(AWH_ERROR()).c_str());
					}
				}
				// Выводим результат
				return result;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(family, index), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * close Метод закрытия подключения
 */
void awh::Ping::close() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
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
 * cancel Метод остановки запущенной работы
 */
void awh::Ping::cancel() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если режим работы пинга активирован
	if(this->_mode){
		// Выполняем остановку работы резолвера
		this->_mode = !this->_mode;
		// Выполняем закрытие подключения
		this->close();
	}
}
/**
 * working Метод проверки запуска работы модуля
 * @return результат работы
 */
bool awh::Ping::working() const noexcept {
	// Выполняем проверку запущен ли модуль
	return this->_mode;
}
/**
 * ping Метод запуска пинга хоста в асинхронном режиме
 * @param host хост для выполнения пинга
 */
void awh::Ping::ping(const string & host) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если хост передан и пинг ещё не активирован
	if(!host.empty() && !this->_mode){
		// Если разрешено выводить информацию в лог
		if(this->_verb)
			// Формируем сообщение для вывода в лог
			this->_log->print("PING %s: %u data bytes", log_t::flag_t::INFO, host.c_str(), sizeof(IcmpHeader));
		/**
		 * Выполняем определение соответствие хоста
		 */
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если домен является адресом в файловой системе
			case static_cast <uint8_t> (net_t::type_t::FS):
			// Если домен является аппаратным адресом сетевого интерфейса
			case static_cast <uint8_t> (net_t::type_t::MAC):
			// Если домен является URL-адресом
			case static_cast <uint8_t> (net_t::type_t::URL):
			// Если домен является адресом/Маски сети
			case static_cast <uint8_t> (net_t::type_t::NETWORK): break;
			// Если IP-адрес является IPv4-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем пинг указанного адреса
				std::thread(&ping_t::_work, this, AF_INET, host).detach();
			break;
			// Если IP-адрес является IPv6-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем пинг указанного адреса
				std::thread(&ping_t::_work, this, AF_INET6, host).detach();
			break;
			// Для всех остальных адресов
			default: {
				// Выполняем получение IP-адреса для IPv6
				string ip = this->_dns.resolve(AF_INET6, host);
				// Если результат получен, выполняем пинг
				if(!ip.empty())
					// Выполняем пинг указанного адреса
					std::thread(&ping_t::_work, this, AF_INET6, ip).detach();
				// Если результат не получен, выполняем получение IPv4-адреса
				else {
					// Выполняем получение IP-адреса для IPv4
					ip = this->_dns.resolve(AF_INET, host);
					// Если IP-адрес успешно получен
					if(!ip.empty())
						// Выполняем пинг указанного адреса
						std::thread(&ping_t::_work, this, AF_INET, ip).detach();
					// Если IP-адрес не получен
					else {
						// Если разрешено выводить информацию в лог
						if(this->_verb)
							// Выводим сообщение об ошибке
							this->_log->print("Passed %s address is not legitimate", log_t::flag_t::CRITICAL, host.c_str());
						// Выходим из функции
						return;
					}
				}
			}
		}
	}
}
/**
 * ping Метод запуска пинга хоста в синхронном режиме
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param host   хост для выполнения пинга
 */
void awh::Ping::ping(const int32_t family, const string & host) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если хост передан и пинг ещё не активирован
	if(!host.empty() && !this->_mode){
		// Если разрешено выводить информацию в лог
		if(this->_verb)
			// Формируем сообщение для вывода в лог
			this->_log->print("PING %s: %u data bytes", log_t::flag_t::INFO, host.c_str(), sizeof(IcmpHeader));
		/**
		 * Выполняем определение соответствие хоста
		 */
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если домен является адресом в файловой системе
			case static_cast <uint8_t> (net_t::type_t::FS):
			// Если домен является аппаратным адресом сетевого интерфейса
			case static_cast <uint8_t> (net_t::type_t::MAC):
			// Если домен является URL-адресом
			case static_cast <uint8_t> (net_t::type_t::URL):
			// Если домен является адресом/Маски сети
			case static_cast <uint8_t> (net_t::type_t::NETWORK): break;
			// Если IP-адрес является IPv4-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем пинг указанного адреса
				std::thread(&ping_t::_work, this, AF_INET, host).detach();
			break;
			// Если IP-адрес является IPv6-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем пинг указанного адреса
				std::thread(&ping_t::_work, this, AF_INET6, host).detach();
			break;
			// Для всех остальных адресов
			default: {
				// Выполняем получение IP-адреса
				const string & ip = this->_dns.resolve(family, host);
				// Если результат получен, выполняем пинг
				if(!ip.empty())
					// Выполняем пинг указанного адреса
					std::thread(&ping_t::_work, this, family, ip).detach();
				// Если результат не получен и разрешено выводить информацию в лог
				else if(this->_verb)
					// Выводим сообщение об ошибке
					this->_log->print("Passed %s address is not legitimate", log_t::flag_t::CRITICAL, host.c_str());
			}
		}
	}
}
/**
 * _work Метод запуска пинга IP-адреса в асинхронном режиме
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для выполнения пинга
 */
void awh::Ping::_work(const int32_t family, const string & ip) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы PING-клиента соответствует
	if(hold.access({}, status_t::PING)){
		// Если IP-адрес передан и пинг ещё не активирован
		if((this->_mode = !ip.empty())){
			// Получаем хост текущего компьютера
			const string & host = this->host(family);
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					// Создаём объект клиента
					struct sockaddr_in client;
					// Создаём объект сервера
					struct sockaddr_in server;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Очищаем всю структуру для клиента
					::memset(&client, 0, sizeof(client));
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					client.sin_family = family;
					// Устанавливаем протокол интернета
					server.sin_family = family;
					// Устанавливаем произвольный порт для локального подключения
					client.sin_port = htons(0);
					// Устанавливаем порт для локального подключения
					server.sin_port = htons(0);
					// Устанавливаем IP-адрес для подключения
					::inet_pton(family, ip.c_str(), &server.sin_addr.s_addr);
					// Устанавливаем адрес для локальго подключения
					::inet_pton(family, host.c_str(), &client.sin_addr.s_addr);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&this->_peer.server, &server, this->_peer.size);
					// Обнуляем серверную структуру
					::memset(&(reinterpret_cast <struct sockaddr_in *> (&this->_peer.server))->sin_zero, 0, sizeof(server.sin_zero));
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Создаём сокет подключения
						this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMP);
					/**
					 * Методы только для *Nix-подобных операционных систем
					 */
					#else
						// Если пользователь является привилигированным
						if(::getuid())
							// Создаём сокет подключения
							this->_fd = ::socket(family, SOCK_DGRAM, IPPROTO_ICMP);
						// Создаём сокет подключения
						else this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMP);
					#endif
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Создаём объект клиента
					struct sockaddr_in6 client;
					// Создаём объект сервера
					struct sockaddr_in6 server;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Очищаем всю структуру для клиента
					::memset(&client, 0, sizeof(client));
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					client.sin6_family = family;
					// Устанавливаем протокол интернета
					server.sin6_family = family;
					// Устанавливаем произвольный порт для локального подключения
					client.sin6_port = htons(0);
					// Устанавливаем порт для локального подключения
					server.sin6_port = htons(0);
					// Устанавливаем IP-адрес для подключения
					::inet_pton(family, ip.c_str(), &server.sin6_addr);
					// Устанавливаем адрес для локальго подключения
					::inet_pton(family, host.c_str(), &client.sin6_addr);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&this->_peer.server, &server, this->_peer.size);
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Создаём сокет подключения  (IPPROTO_ICMP6)
						this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMPV6);
					/**
					 * Методы только для *Nix-подобных операционных систем
					 */
					#else
						// Если пользователь является привилигированным
						if(::getuid())
							// Создаём сокет подключения
							this->_fd = ::socket(family, SOCK_DGRAM, IPPROTO_ICMPV6);
						// Создаём сокет подключения
						else this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMPV6);
					#endif
				} break;
			}
			// Если сокет не создан создан и работа резолвера не остановлена
			if(this->_mode && (this->_fd == INVALID_SOCKET)){
				// Выполняем закрытие подключения
				this->close();
				// Если разрешено выводить информацию в лог
				if(this->_verb)
					// Выводим в лог сообщение
					this->_log->print("File descriptor needed for the ICMP request could not be allocated", log_t::flag_t::WARNING);
				// Выходим из приложения
				return;
			// Если сокет создан удачно и работа резолвера не остановлена
			} else if(this->_mode) {
				// Индекс текущей итерации
				uint64_t index = 0;
				// Устанавливаем разрешение на повторное использование сокета
				this->_socket.reuseable(this->_fd);
				// Устанавливаем разрешение на закрытие сокета при неиспользовании
				this->_socket.closeOnExec(this->_fd);
				// Устанавливаем размер буфера передачи данных на чтение
				// this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::READ);
				// Устанавливаем размер буфера передачи данных на запись
				// this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::WRITE);
				// Устанавливаем таймаут на получение данных из сокета
				this->_socket.timeout(this->_fd, this->_timeoutRead, socket_t::mode_t::READ);
				// Устанавливаем таймаут на запись данных в сокет
				this->_socket.timeout(this->_fd, this->_timeoutWrite, socket_t::mode_t::WRITE);
				// Выполняем бинд на сокет
				if(::bind(this->_fd, reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.size) < 0){
					// Выполняем закрытие подключения
					this->close();
					// Выводим в лог сообщение
					this->_log->print("Bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
				}
				// Выполняем отправку отправку запросов до тех пор пока не остановят
				while(this->_mode){
					// Запоминаем текущее значение времени в миллисекундах
					const time_t mseconds = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Выполняем запрос на сервер
					const int64_t bytes = this->send(family, index);
					// Если данные прочитать не удалось
					if(bytes <= 0){
						// Выполняем закрытие подключения
						this->close();
						// Выводим результат
						return;
					}
					// Выполняем подсчёт количество прошедшего времени
					const time_t timeShifting = (this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - mseconds);
					// Если разрешено выводить информацию в лог
					if(this->_verb)
						// Формируем сообщение для вывода в лог
						// this->_log->print("%zu bytes from %s icmp_seq=%u ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), index, index + ((this->_shifting / 1000) * 2), this->_fmk->time2abbr(timeShifting).c_str());
						this->_log->print("%zu bytes from %s icmp_seq=%u ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), index, (this->_timeoutRead + this->_timeoutWrite) / 1000, this->_fmk->time2abbr(timeShifting).c_str());
					{
						// Выполняем блокировку потока
						const lock_guard <std::recursive_mutex> lock(this->_mtx);
						// Если функция обратного вызова установлена
						if(this->_callback != nullptr)
							// Выполняем функцию обратного вызова
							this->_callback(timeShifting, ip, this);
					}
					// Если работа резолвера ещё не остановлена
					if(this->_mode){
						// Устанавливаем время жизни сокета
						// this->_socket.timeToLive(family, this->_fd, i + ((this->_shifting / 1000) * 2));
						// Замораживаем поток на период времени в ${_shifting}
						std::this_thread::sleep_for(chrono::milliseconds(this->_shifting));
						// Выполняем смещение индекса последовательности
						index++;
					}
				}
				// Выполняем закрытие подключения
				this->close();
			}
		}
	}
}
/**
 * ping Метод запуска пинга хоста в синхронном режиме
 * @param host  хост для выполнения пинга
 * @param count количество итераций
 * @return      количество миллисекунд ответа хоста
 */
double awh::Ping::ping(const string & host, const uint16_t count) noexcept {
	// Результат работы функции
	double result = .0;
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если хост передан и пинг ещё не активирован
	if(!host.empty() && !this->_mode){
		// Если разрешено выводить информацию в лог
		if(this->_verb)
			// Формируем сообщение для вывода в лог
			this->_log->print("PING %s: %u data bytes", log_t::flag_t::INFO, host.c_str(), sizeof(IcmpHeader));
		/**
		 * Выполняем определение соответствие хоста
		 */
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если домен является адресом в файловой системе
			case static_cast <uint8_t> (net_t::type_t::FS):
			// Если домен является аппаратным адресом сетевого интерфейса
			case static_cast <uint8_t> (net_t::type_t::MAC):
			// Если домен является URL-адресом
			case static_cast <uint8_t> (net_t::type_t::URL):
			// Если домен является адресом/Маски сети
			case static_cast <uint8_t> (net_t::type_t::NETWORK): break;
			// Если IP-адрес является IPv4-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем пинг указанного адреса
				result = this->_ping(AF_INET, host, count);
			break;
			// Если IP-адрес является IPv6-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем пинг указанного адреса
				result = this->_ping(AF_INET6, host, count);
			break;
			// Для всех остальных адресов
			default: {
				// Выполняем получение IP-адреса для IPv6
				string ip = this->_dns.resolve(AF_INET6, host);
				// Если результат получен, выполняем пинг
				if(!ip.empty())
					// Выполняем пинг указанного адреса
					result = this->_ping(AF_INET6, ip, count);
				// Если результат не получен, выполняем получение IPv4-адреса
				else {
					// Выполняем получение IP-адреса для IPv4
					ip = this->_dns.resolve(AF_INET, host);
					// Если IP-адрес успешно получен
					if(!ip.empty())
						// Выполняем пинг указанного адреса
						result = this->_ping(AF_INET, ip, count);
					// Если IP-адрес не получен
					else {
						// Если разрешено выводить информацию в лог
						if(this->_verb)
							// Выводим сообщение об ошибке
							this->_log->print("Passed %s address is not legitimate", log_t::flag_t::CRITICAL, host.c_str());
						// Выходим из функции
						return result;
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * ping Метод запуска пинга хоста в синхронном режиме
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param host   хост для выполнения пинга
 * @param count  количество итераций
 * @return       количество миллисекунд ответа хоста
 */
double awh::Ping::ping(const int32_t family, const string & host, const uint16_t count) noexcept {
	// Результат работы функции
	double result = .0;
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если хост передан и пинг ещё не активирован
	if(!host.empty() && !this->_mode){
		// Если разрешено выводить информацию в лог
		if(this->_verb)
			// Формируем сообщение для вывода в лог
			this->_log->print("PING %s: %u data bytes", log_t::flag_t::INFO, host.c_str(), sizeof(IcmpHeader));
		/**
		 * Выполняем определение соответствие хоста
		 */
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если домен является адресом в файловой системе
			case static_cast <uint8_t> (net_t::type_t::FS):
			// Если домен является аппаратным адресом сетевого интерфейса
			case static_cast <uint8_t> (net_t::type_t::MAC):
			// Если домен является URL-адресом
			case static_cast <uint8_t> (net_t::type_t::URL):
			// Если домен является адресом/Маски сети
			case static_cast <uint8_t> (net_t::type_t::NETWORK): break;
			// Если IP-адрес является IPv4-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем пинг указанного адреса
				result = this->_ping(AF_INET, host, count);
			break;
			// Если IP-адрес является IPv6-адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем пинг указанного адреса
				result = this->_ping(AF_INET6, host, count);
			break;
			// Для всех остальных адресов
			default: {
				// Выполняем получение IP-адреса
				const string & ip = this->_dns.resolve(family, host);
				// Если результат получен, выполняем пинг
				if(!ip.empty())
					// Выполняем пинг указанного адреса
					result = this->_ping(family, ip, count);
				// Если результат не получен и разрешено выводить информацию в лог
				else if(this->_verb)
					// Выводим сообщение об ошибке
					this->_log->print("Passed %s address is not legitimate", log_t::flag_t::CRITICAL, host.c_str());
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * _ping Метод запуска пинга IP-адреса в синхронном режиме
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для выполнения пинга
 * @param count  количество итераций
 * @return       количество миллисекунд ответа хоста
 */
double awh::Ping::_ping(const int32_t family, const string & ip, const uint16_t count) noexcept {
	// Результат работы функции
	double result = .0;
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы PING-клиента соответствует
	if(hold.access({}, status_t::PING)){
		// Если IP-адрес передан и пинг ещё не активирован
		if((this->_mode = !ip.empty())){
			// Получаем хост текущего компьютера
			const string & host = this->host(family);
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					// Создаём объект клиента
					struct sockaddr_in client;
					// Создаём объект сервера
					struct sockaddr_in server;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Очищаем всю структуру для клиента
					::memset(&client, 0, sizeof(client));
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					client.sin_family = family;
					// Устанавливаем протокол интернета
					server.sin_family = family;
					// Устанавливаем произвольный порт для локального подключения
					client.sin_port = htons(0);
					// Устанавливаем порт для локального подключения
					server.sin_port = htons(0);
					// Устанавливаем IP-адрес для подключения
					::inet_pton(family, ip.c_str(), &server.sin_addr.s_addr);
					// Устанавливаем адрес для локальго подключения
					::inet_pton(family, host.c_str(), &client.sin_addr.s_addr);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&this->_peer.server, &server, this->_peer.size);
					// Обнуляем серверную структуру
					::memset(&(reinterpret_cast <struct sockaddr_in *> (&this->_peer.server))->sin_zero, 0, sizeof(server.sin_zero));
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Создаём сокет подключения
						this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMP);
					/**
					 * Методы только для *Nix-подобных операционных систем
					 */
					#else
						// Если пользователь является привилигированным
						if(::getuid())
							// Создаём сокет подключения
							this->_fd = ::socket(family, SOCK_DGRAM, IPPROTO_ICMP);
						// Создаём сокет подключения
						else this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMP);
					#endif
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Создаём объект клиента
					struct sockaddr_in6 client;
					// Создаём объект сервера
					struct sockaddr_in6 server;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Очищаем всю структуру для клиента
					::memset(&client, 0, sizeof(client));
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					client.sin6_family = family;
					// Устанавливаем протокол интернета
					server.sin6_family = family;
					// Устанавливаем произвольный порт для локального подключения
					client.sin6_port = htons(0);
					// Устанавливаем порт для локального подключения
					server.sin6_port = htons(0);
					// Устанавливаем IP-адрес для подключения
					::inet_pton(family, ip.c_str(), &server.sin6_addr);
					// Устанавливаем адрес для локальго подключения
					::inet_pton(family, host.c_str(), &client.sin6_addr);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&this->_peer.server, &server, this->_peer.size);
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Создаём сокет подключения (IPPROTO_ICMP6)
						this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMPV6);
					/**
					 * Методы только для *Nix-подобных операционных систем
					 */
					#else
						// Если пользователь является привилигированным
						if(::getuid())
							// Создаём сокет подключения
							this->_fd = ::socket(family, SOCK_DGRAM, IPPROTO_ICMPV6);
						// Создаём сокет подключения
						else this->_fd = ::socket(family, SOCK_RAW, IPPROTO_ICMPV6);
					#endif
				} break;
			}
			// Если сокет не создан создан и работа резолвера не остановлена
			if(this->_mode && (this->_fd == INVALID_SOCKET)){
				// Если разрешено выводить информацию в лог
				if(this->_verb)
					// Выводим в лог сообщение
					this->_log->print("File descriptor needed for the ICMP request could not be allocated", log_t::flag_t::WARNING);
				// Выходим из приложения
				return result;
			// Если сокет создан удачно и работа резолвера не остановлена
			} else if(this->_mode) {
				// Устанавливаем разрешение на повторное использование сокета
				this->_socket.reuseable(this->_fd);
				// Устанавливаем разрешение на закрытие сокета при неиспользовании
				this->_socket.closeOnExec(this->_fd);
				// Устанавливаем размер буфера передачи данных на чтение
				// this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::READ);
				// Устанавливаем размер буфера передачи данных на запись
				// this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::WRITE);
				// Устанавливаем таймаут на получение данных из сокета
				this->_socket.timeout(this->_fd, this->_timeoutRead, socket_t::mode_t::READ);
				// Устанавливаем таймаут на запись данных в сокет
				this->_socket.timeout(this->_fd, this->_timeoutWrite, socket_t::mode_t::WRITE);
				// Выполняем бинд на сокет
				if(::bind(this->_fd, reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.size) < 0)
					// Выводим в лог сообщение
					this->_log->print("Bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
				// Выполняем отправку указанного количества запросов
				for(uint16_t i = 0; i < count; i++){
					// Запоминаем текущее значение времени в миллисекундах
					const time_t mseconds = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Выполняем запрос на сервер
					const int64_t bytes = this->send(family, i);
					// Если данные прочитать не удалось
					if(bytes <= 0)
						// Выводим результат
						return result;
					// Выполняем подсчёт количество прошедшего времени
					const time_t timeShifting = (this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - mseconds);
					// Если разрешено выводить информацию в лог
					if(this->_verb)
						// Формируем сообщение для вывода в лог
						// this->_log->print("%zu bytes from %s icmp_seq=%u ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), i, i + ((this->_shifting / 1000) * 2), this->_fmk->time2abbr(timeShifting).c_str());
						this->_log->print("%zu bytes from %s icmp_seq=%u ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), i, (this->_timeoutRead + this->_timeoutWrite) / 1000, this->_fmk->time2abbr(timeShifting).c_str());
					// Увеличиваем общее количество времени
					result += static_cast <double> (timeShifting);
					// Если работа резолвера ещё не остановлена
					if(this->_mode){
						// Устанавливаем время жизни сокета
						// this->_socket.timeToLive(family, this->_fd, i + ((this->_shifting / 1000) * 2));
						// Замораживаем поток на период времени в ${_shifting}
						std::this_thread::sleep_for(chrono::milliseconds(this->_shifting));
					}
				}
				// Выполняем закрытие подключения
				this->close();
				// Выполняем расчет среднего затраченного времени
				result /= static_cast <double> (count);
				// Выполняем приведение число до 3-х знаков после запятой
				result = this->_fmk->floor(result, 3);
				// Если разрешено выводить информацию в лог
				if(this->_verb){
					// Если времени затрачено меньше 5-ти секунд
					if(result < 5.0)
						// Выводим сообщение результата в лог
						this->_log->print("Your connection is good. Avg ping %s", log_t::flag_t::INFO, this->_fmk->time2abbr(result).c_str());
					// Выводим сообщение как есть
					else this->_log->print("Bad connection. Avg ping %s", log_t::flag_t::WARNING, this->_fmk->time2abbr(result).c_str());
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * verbose Метод разрешающий/запрещающий выводить информационных сообщений
 * @param mode флаг для установки
 */
void awh::Ping::verbose(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку флага запрещающего выводить информацию пинга в лог
	this->_verb = mode;
}
/**
 * shifting Метод установки сдвига по времени выполнения пинга в миллисекундах
 * @param msec сдвиг по времени в миллисекундах
 */
void awh::Ping::shifting(const time_t msec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку сдвига по времени выполнения пинга
	this->_shifting = msec;
}
/**
 * ns Метод добавления серверов DNS
 * @param servers параметры DNS-серверов
 */
void awh::Ping::ns(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если список серверов передан
	if(!servers.empty())
		// Выполняем установку полученных DNS-серверов
		this->_dns.servers(servers);
}
/**
 * network Метод установки адреса сетевых плат, с которых нужно выполнять запросы
 * @param network IP-адреса сетевых плат
 */
void awh::Ping::network(const vector <string> & network) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы установки параметров сети соответствует
	if(hold.access({}, status_t::NET_SET)){
		// Если список адресов сетевых плат передан
		if(!network.empty()){
			// Переходим по всему списку полученных адресов
			for(auto & host : network){
				/**
				 * Выполняем определение соответствие хоста
				 */
				switch(static_cast <uint8_t> (this->_net.host(host))){
					// Если домен является адресом в файловой системе
					case static_cast <uint8_t> (net_t::type_t::FS):
					// Если домен является аппаратным адресом сетевого интерфейса
					case static_cast <uint8_t> (net_t::type_t::MAC):
					// Если домен является URL-адресом
					case static_cast <uint8_t> (net_t::type_t::URL):
					// Если домен является адресом/Маски сети
					case static_cast <uint8_t> (net_t::type_t::NETWORK): break;
					// Если IP-адрес является IPv4-адресом
					case static_cast <uint8_t> (net_t::type_t::IPV4):
						// Выполняем добавление полученного хоста в список
						this->_networkIPv4.push_back(host);
					break;
					// Если IP-адрес является IPv6-адресом
					case static_cast <uint8_t> (net_t::type_t::IPV6):
						// Выполняем добавление полученного хоста в список
						this->_networkIPv6.push_back(host);
					break;
					// Для всех остальных адресов
					default: {
						// Выполняем получение IP-адреса для IPv6
						string ip = this->_dns.host(AF_INET6, host);
						// Если результат получен, выполняем пинг
						if(!ip.empty())
							// Выполняем добавление полученного хоста в список
							this->_networkIPv6.push_back(ip);
						// Если результат не получен, выполняем получение IPv4-адреса
						else {
							// Выполняем получение IP-адреса для IPv4
							ip = this->_dns.host(AF_INET, host);
							// Если IP-адрес успешно получен
							if(!ip.empty())
								// Выполняем добавление полученного хоста в список
								this->_networkIPv4.push_back(ip);
							// Если IP-адрес не получен и разрешено выводить информацию в лог
							else if(this->_verb)
								// Выводим сообщение об ошибке
								this->_log->print("Passed %s address is not legitimate", log_t::flag_t::WARNING, host.c_str());
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
void awh::Ping::network(const int32_t family, const vector <string> & network) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы установки параметров сети соответствует
	if(hold.access({}, status_t::NET_SET)){
		// Если список адресов сетевых плат передан
		if(!network.empty()){
			// Переходим по всему списку полученных адресов
			for(auto & host : network){
				// Определяем тип передаваемого IP-адреса
				switch(family){
					// Если IP-адрес является IPv4 адресом
					case static_cast <int32_t> (AF_INET):
						// Выполняем добавление полученного хоста в список
						this->_networkIPv4.push_back(host);
					break;
					// Если IP-адрес является IPv6 адресом
					case static_cast <int32_t> (AF_INET6):
						// Выполняем добавление полученного хоста в список
						this->_networkIPv6.push_back(host);
					break;
				}
			}
		}
	}
}
/**
 * timeout Метод установки таймаутов в миллисекундах
 * @param read  таймаут на чтение
 * @param write таймаут на запись
 */
void awh::Ping::timeout(const time_t read, const time_t write) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку таймаута на чтение
	this->_timeoutRead = read;
	// Выполняем установку таймаута на запись
	this->_timeoutWrite = write;
}
/**
 * on Метод установки функции обратного вызова, для работы в асинхронном режиме
 * @param callback функция обратного вызова
 */
void awh::Ping::on(function <void (const time_t, const string &, Ping *)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем функцию обратного вызова
	this->_callback = callback;
}
