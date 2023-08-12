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

// Подключаем заголовочный файл
#include <net/ping.hpp>

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
 * close Метод закрытия подключения
 */
void awh::Ping::close() noexcept {
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
 * ping Метод запуска пинга IP-адреса в асинхронном режиме
 * @param host хост для выполнения пинга
 */
void awh::Ping::ping(const string & host) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если хост передан и пинг ещё не активирован
	if(!host.empty() && !this->_mode){
		// Если разрешено выводить информацию в лог
		if(!this->_noInfo)
			// Формируем сообщение для вывода в лог
			this->_log->print("PING %s: %u data bytes", log_t::flag_t::INFO, host.c_str(), sizeof(IcmpHeader));
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем пинг указанного адреса
				std::thread(&ping_t::work, this, AF_INET, host).detach();
			break;
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем пинг указанного адреса
				std::thread(&ping_t::work, this, AF_INET6, host).detach();
			break;
			// Для всех остальных адресов
			default: {
				// Выполняем получение IP-адреса для IPv6
				string ip = this->_dns.resolve(AF_INET6, host);
				// Если результат получен, выполняем пинг
				if(!ip.empty())
					// Выполняем пинг указанного адреса
					std::thread(&ping_t::work, this, AF_INET6, ip).detach();
				// Если результат не получен, выполняем получение IPv4 адреса
				else {
					// Выполняем получение IP-адреса для IPv4
					ip = this->_dns.resolve(AF_INET, host);
					// Если IP-адрес успешно получен
					if(!ip.empty())
						// Выполняем пинг указанного адреса
						std::thread(&ping_t::work, this, AF_INET, ip).detach();
					// Если IP-адрес не получен
					else {
						// Если разрешено выводить информацию в лог
						if(!this->_noInfo)
							// Выводим сообщение об ошибке
							this->_log->print("passed %s address is not legitimate", log_t::flag_t::CRITICAL, host.c_str());
						// Выходим из функции
						return;
					}
				}
			}
		}
	}
}
/**
 * work Метод запуска пинга IP-адреса в асинхронном режиме
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для выполнения пинга
 */
void awh::Ping::work(const int family, const string & ip) noexcept {
	// Если IP-адрес передан и пинг ещё не активирован
	if(!ip.empty() && !this->_mode){
		// Выполняем активирование режима работы
		this->_mode = !this->_mode;
		// Очищаем всю структуру клиента
		memset(&this->_addr, 0, sizeof(this->_addr));
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case AF_INET: {
				// Создаём объект клиента
				struct sockaddr_in client;
				// Запоминаем размер структуры
				this->_socklen = sizeof(client);
				// Устанавливаем протокол интернета
				client.sin_family = family;
				// Устанавливаем произвольный порт
				client.sin_port = htons(0);
				// Устанавливаем IP-адрес для подключения
				inet_pton(family, ip.c_str(), &client.sin_addr.s_addr);
				// Выполняем копирование объекта подключения клиента
				memcpy(&this->_addr, &client, this->_socklen);
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
					if(getuid())
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
				// Запоминаем размер структуры
				this->_socklen = sizeof(client);
				// Устанавливаем протокол интернета
				client.sin6_family = family;
				// Устанавливаем произвольный порт для
				client.sin6_port = htons(0);
				// Устанавливаем IP-адрес для подключения
				inet_pton(family, ip.c_str(), &client.sin6_addr);
				// Выполняем копирование объекта подключения клиента
				memcpy(&this->_addr, &client, this->_socklen);
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
					if(getuid())
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
			if(!this->_noInfo)
				// Выводим в лог сообщение
				this->_log->print("file descriptor needed for the ICMP request could not be allocated", log_t::flag_t::WARNING);
			// Выходим из приложения
			return;
		// Если сокет создан удачно и работа резолвера не остановлена
		} else if(this->_mode) {
			// Выполняем инициализацию генератора
			random_device dev;
			// Подключаем устройство генератора
			mt19937 rng(dev());
			// Количество передаваемых байтов
			int64_t bytes = 0;
			// Индекс текущей итерации
			uint64_t index = 0;
			// Начальное значение времени
			time_t mseconds = 0;
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->_fd);
			// Устанавливаем разрешение на закрытие сокета при неиспользовании
			this->_socket.closeOnExec(this->_fd);
			// Устанавливаем размер буфера передачи данных на чтение
			this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::READ);
			// Устанавливаем размер буфера передачи данных на запись
			this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::WRITE);
			// Устанавливаем таймаут на получение данных из сокета
			this->_socket.timeout(this->_fd, this->_timeoutRead, socket_t::mode_t::READ);
			// Устанавливаем таймаут на запись данных в сокет
			this->_socket.timeout(this->_fd, this->_timeoutWrite, socket_t::mode_t::WRITE);
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, std::numeric_limits <uint32_t>::max() - 1);
			// Выполняем отправку отправку запросов до тех пор пока не остановят
			while(this->_mode){
				// Метка отправки данных
				Send:
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
				icmp.meta.echo.identifier = getpid();
				// Устанавливаем данные полезной нагрузки
				icmp.meta.echo.payload = static_cast <uint64_t> (dist6(rng));
				// Выполняем подсчёт контрольной суммы
				icmp.checksum = this->checksum(&icmp, sizeof(icmp));
				// Запоминаем текущее значение времени в миллисекундах
				mseconds = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
				// Если запрос на сервер DNS успешно отправлен
				if((bytes = ::sendto(this->_fd, reinterpret_cast <char *> (&icmp), sizeof(icmp), 0, (struct sockaddr *) &this->_addr, this->_socklen)) > 0){
					// Метка повторного получения данных
					Read:
					// Буфер для получения данных
					char buffer[1024];
					// Результат полученных данных
					auto * icmpResponseHeader = (struct IcmpHeader *) buffer;
					// Выполняем чтение ответа сервера
					bytes = ::recvfrom(this->_fd, reinterpret_cast <char *> (icmpResponseHeader), sizeof(buffer), 0, (struct sockaddr *) &this->_addr, &this->_socklen);
					// Если данные прочитать не удалось
					if(bytes <= 0){
						// Если сокет находится в блокирующем режиме
						if(bytes < 0){
							// Определяем тип ошибки
							switch(errno){
								// Если ошибка не обнаружена, выходим
								case 0: break;
								// Если нужно повторить получение данных
								case EAGAIN:
									// Снова пробуем получить данные
									goto Read;
								break;
								// Если произведена неудачная запись в PIPE
								case EPIPE:
									// Выводим в лог сообщение
									this->_log->print("EPIPE", log_t::flag_t::WARNING);
								break;
								// Если произведён сброс подключения
								case ECONNRESET:
									// Выводим в лог сообщение
									this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
								break;
								// Для остальных ошибок
								default:
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							}
							// Выполняем закрытие подключения
							this->close();
							// Выводим результат
							return;
						}
					// Если данные получены удачно
					} else {
						// Выполняем подсчёт количество прошедшего времени
						const time_t timeShifting = (this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - mseconds);
						// Если разрешено выводить информацию в лог
						if(!this->_noInfo)
							// Формируем сообщение для вывода в лог
							// this->_log->print("%zu bytes from %s icmp_seq=%zu ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), index, index + ((this->_shifting / 1000) * 2), this->_fmk->time2abbr(timeShifting).c_str());
							this->_log->print("%zu bytes from %s icmp_seq=%zu ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), index, (this->_timeoutRead + this->_timeoutWrite) / 1000, this->_fmk->time2abbr(timeShifting).c_str());
						// Если функция обратного вызова установлена
						if(this->_callback != nullptr){
							// Выполняем блокировку потока
							const lock_guard <recursive_mutex> lock(this->_mtx);
							// Выполняем функцию обратного вызова
							this->_callback(timeShifting, ip, this);
						}
					}
				// Если сообщение отправить не удалось
				} else if(bytes <= 0) {
					// Если сокет находится в блокирующем режиме
					if(bytes < 0){
						// Определяем тип ошибки
						switch(errno){
							// Если ошибка не обнаружена, выходим
							case 0: break;
							// Если нужно повторить получение данных
							case EAGAIN:
								// Снова пробуем отправить данные
								goto Send;
							break;
							// Если произведена неудачная запись в PIPE
							case EPIPE:
								// Выводим в лог сообщение
								this->_log->print("EPIPE", log_t::flag_t::WARNING);
							break;
							// Если произведён сброс подключения
							case ECONNRESET:
								// Выводим в лог сообщение
								this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
							break;
							// Для остальных ошибок
							default:
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
						}
						// Выполняем закрытие подключения
						this->close();
						// Выводим результат
						return;
					}
				}
				// Если работа резолвера ещё не остановлена
				if(this->_mode){
					// Устанавливаем время жизни сокета
					// this->_socket.timeToLive(family, this->_fd, i + ((this->_shifting / 1000) * 2));
					// Замораживаем поток на период времени в ${_shifting}
					this_thread::sleep_for(chrono::milliseconds(this->_shifting));
					// Выполняем смещение индекса последовательности
					index++;
				}
			}
			// Выполняем закрытие подключения
			this->close();
		}
	}
}
/**
 * ping Метод запуска пинга IP-адреса в синхронном режиме
 * @param host  хост для выполнения пинга
 * @param count количество итераций
 */
double awh::Ping::ping(const string & host, const uint16_t count) noexcept {
	// Результат работы функции
	double result = 0.0;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если хост передан и пинг ещё не активирован
	if(!host.empty() && !this->_mode){
		// Если разрешено выводить информацию в лог
		if(!this->_noInfo)
			// Формируем сообщение для вывода в лог
			this->_log->print("PING %s: %u data bytes", log_t::flag_t::INFO, host.c_str(), sizeof(IcmpHeader));
		// Определяем тип передаваемого IP-адреса
		switch(static_cast <uint8_t> (this->_net.host(host))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем пинг указанного адреса
				result = this->ping(AF_INET, host, count);
			break;
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем пинг указанного адреса
				result = this->ping(AF_INET6, host, count);
			break;
			// Для всех остальных адресов
			default: {
				// Выполняем получение IP-адреса для IPv6
				string ip = this->_dns.resolve(AF_INET6, host);
				// Если результат получен, выполняем пинг
				if(!ip.empty())
					// Выполняем пинг указанного адреса
					result = this->ping(AF_INET6, ip, count);
				// Если результат не получен, выполняем получение IPv4 адреса
				else {
					// Выполняем получение IP-адреса для IPv4
					ip = this->_dns.resolve(AF_INET, host);
					// Если IP-адрес успешно получен
					if(!ip.empty())
						// Выполняем пинг указанного адреса
						result = this->ping(AF_INET, ip, count);
					// Если IP-адрес не получен
					else {
						// Если разрешено выводить информацию в лог
						if(!this->_noInfo)
							// Выводим сообщение об ошибке
							this->_log->print("passed %s address is not legitimate", log_t::flag_t::CRITICAL, host.c_str());
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
 * ping Метод запуска пинга IP-адреса в синхронном режиме
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для выполнения пинга
 * @param count  количество итераций
 */
double awh::Ping::ping(const int family, const string & ip, const uint16_t count) noexcept {
	// Результат работы функции
	double result = 0.0;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если IP-адрес передан и пинг ещё не активирован
	if(!ip.empty() && !this->_mode){
		// Выполняем активирование режима работы
		this->_mode = !this->_mode;
		// Очищаем всю структуру клиента
		memset(&this->_addr, 0, sizeof(this->_addr));
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case AF_INET: {
				// Создаём объект клиента
				struct sockaddr_in client;
				// Запоминаем размер структуры
				this->_socklen = sizeof(client);
				// Устанавливаем протокол интернета
				client.sin_family = family;
				// Устанавливаем произвольный порт
				client.sin_port = htons(0);
				// Устанавливаем IP-адрес для подключения
				inet_pton(family, ip.c_str(), &client.sin_addr.s_addr);
				// Выполняем копирование объекта подключения клиента
				memcpy(&this->_addr, &client, this->_socklen);
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
					if(getuid())
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
				// Запоминаем размер структуры
				this->_socklen = sizeof(client);
				// Устанавливаем протокол интернета
				client.sin6_family = family;
				// Устанавливаем произвольный порт для
				client.sin6_port = htons(0);
				// Устанавливаем IP-адрес для подключения
				inet_pton(family, ip.c_str(), &client.sin6_addr);
				// Выполняем копирование объекта подключения клиента
				memcpy(&this->_addr, &client, this->_socklen);
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
					if(getuid())
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
			if(!this->_noInfo)
				// Выводим в лог сообщение
				this->_log->print("file descriptor needed for the ICMP request could not be allocated", log_t::flag_t::WARNING);
			// Выходим из приложения
			return result;
		// Если сокет создан удачно и работа резолвера не остановлена
		} else if(this->_mode) {
			// Выполняем инициализацию генератора
			random_device dev;
			// Подключаем устройство генератора
			mt19937 rng(dev());
			// Количество передаваемых байтов
			int64_t bytes = 0;
			// Начальное значение времени
			time_t mseconds = 0;
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->_fd);
			// Устанавливаем разрешение на закрытие сокета при неиспользовании
			this->_socket.closeOnExec(this->_fd);
			// Устанавливаем размер буфера передачи данных на чтение
			this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::READ);
			// Устанавливаем размер буфера передачи данных на запись
			this->_socket.bufferSize(this->_fd, 1024, 1, socket_t::mode_t::WRITE);
			// Устанавливаем таймаут на получение данных из сокета
			this->_socket.timeout(this->_fd, this->_timeoutRead, socket_t::mode_t::READ);
			// Устанавливаем таймаут на запись данных в сокет
			this->_socket.timeout(this->_fd, this->_timeoutWrite, socket_t::mode_t::WRITE);
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, std::numeric_limits <uint32_t>::max() - 1);
			// Выполняем отправку указанного количества запросов
			for(uint16_t i = 0; i < count; i++){
				// Метка отправки данных
				Send:
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
				icmp.meta.echo.sequence = i;
				// Устанавливаем идентификатор запроса
				icmp.meta.echo.identifier = getpid();
				// Устанавливаем данные полезной нагрузки
				icmp.meta.echo.payload = static_cast <uint64_t> (dist6(rng));
				// Выполняем подсчёт контрольной суммы
				icmp.checksum = this->checksum(&icmp, sizeof(icmp));
				// Запоминаем текущее значение времени в миллисекундах
				mseconds = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
				// Если запрос на сервер DNS успешно отправлен
				if((bytes = ::sendto(this->_fd, reinterpret_cast <char *> (&icmp), sizeof(icmp), 0, (struct sockaddr *) &this->_addr, this->_socklen)) > 0){
					// Метка повторного получения данных
					Read:
					// Буфер для получения данных
					char buffer[1024];
					// Результат полученных данных
					auto * icmpResponseHeader = (struct IcmpHeader *) buffer;
					// Выполняем чтение ответа сервера
					bytes = ::recvfrom(this->_fd, reinterpret_cast <char *> (icmpResponseHeader), sizeof(buffer), 0, (struct sockaddr *) &this->_addr, &this->_socklen);
					// Если данные прочитать не удалось
					if(bytes <= 0){
						// Если сокет находится в блокирующем режиме
						if(bytes < 0){
							// Определяем тип ошибки
							switch(errno){
								// Если ошибка не обнаружена, выходим
								case 0: break;
								// Если нужно повторить получение данных
								case EAGAIN:
									// Снова пробуем получить данные
									goto Read;
								break;
								// Если произведена неудачная запись в PIPE
								case EPIPE:
									// Выводим в лог сообщение
									this->_log->print("EPIPE", log_t::flag_t::WARNING);
								break;
								// Если произведён сброс подключения
								case ECONNRESET:
									// Выводим в лог сообщение
									this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
								break;
								// Для остальных ошибок
								default:
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							}
							// Выполняем закрытие подключения
							this->close();
							// Выводим результат
							return result;
						}
					// Если данные получены удачно
					} else {
						// Выполняем подсчёт количество прошедшего времени
						const time_t timeShifting = (this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - mseconds);
						// Если разрешено выводить информацию в лог
						if(!this->_noInfo)
							// Формируем сообщение для вывода в лог
							// this->_log->print("%zu bytes from %s icmp_seq=%u ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), i, i + ((this->_shifting / 1000) * 2), this->_fmk->time2abbr(timeShifting).c_str());
							this->_log->print("%zu bytes from %s icmp_seq=%u ttl=%u time=%s", log_t::flag_t::INFO, bytes, ip.c_str(), i, (this->_timeoutRead + this->_timeoutWrite) / 1000, this->_fmk->time2abbr(timeShifting).c_str());
						// Увеличиваем общее количество времени
						result += static_cast <double> (timeShifting);
					}
				// Если сообщение отправить не удалось
				} else if(bytes <= 0) {
					// Если сокет находится в блокирующем режиме
					if(bytes < 0){
						// Определяем тип ошибки
						switch(errno){
							// Если ошибка не обнаружена, выходим
							case 0: break;
							// Если нужно повторить получение данных
							case EAGAIN:
								// Снова пробуем отправить данные
								goto Send;
							break;
							// Если произведена неудачная запись в PIPE
							case EPIPE:
								// Выводим в лог сообщение
								this->_log->print("EPIPE", log_t::flag_t::WARNING);
							break;
							// Если произведён сброс подключения
							case ECONNRESET:
								// Выводим в лог сообщение
								this->_log->print("ECONNRESET", log_t::flag_t::WARNING);
							break;
							// Для остальных ошибок
							default:
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
						}
						// Выполняем закрытие подключения
						this->close();
						// Выводим результат
						return result;
					}
				}
				// Если работа резолвера ещё не остановлена
				if(this->_mode){
					// Устанавливаем время жизни сокета
					// this->_socket.timeToLive(family, this->_fd, i + ((this->_shifting / 1000) * 2));
					// Замораживаем поток на период времени в ${_shifting}
					this_thread::sleep_for(chrono::milliseconds(this->_shifting));
				}
			}
			// Выполняем закрытие подключения
			this->close();
			// Выполняем расчет среднего затраченного времени
			result /= static_cast <double> (count);
			// Выполняем приведение число до 3-х знаков после запятой
			result = this->_fmk->floor(result, 3);
			// Если разрешено выводить информацию в лог
			if(!this->_noInfo){
				// Если времени затрачено меньше 5-ти секунд
				if(result < 5.0)
					// Выводим сообщение результата в лог
					this->_log->print("Your connection is good. Avg ping %s", log_t::flag_t::INFO, this->_fmk->time2abbr(result).c_str());
				// Выводим сообщение как есть
				else this->_log->print("Bad connection. Avg ping %s", log_t::flag_t::WARNING, this->_fmk->time2abbr(result).c_str());
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * noInfo Метод запрещающий выводить информацию пинга в лог
 * @param mode флаг для установки
 */
void awh::Ping::noInfo(const bool mode) noexcept {
	// Выполняем установку флага запрещающего выводить информацию пинга в лог
	this->_noInfo = mode;
}
/**
 * shifting Метод установки сдвига по времени выполнения пинга в миллисекундах
 * @param msec сдвиг по времени в миллисекундах
 */
void awh::Ping::shifting(const time_t msec) noexcept {
	// Выполняем установку сдвига по времени выполнения пинга
	this->_shifting = msec;
}
/**
 * ns Метод добавления серверов DNS
 * @param servers параметры DNS-серверов
 */
void awh::Ping::ns(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если список серверов передан
	if(!servers.empty())
		// Выполняем установку полученных DNS-серверов
		this->_dns.servers(servers);
}
/**
 * timeout Метод установки таймаутов в миллисекундах
 * @param read  таймаут на чтение
 * @param write таймаут на запись
 */
void awh::Ping::timeout(const time_t read, const time_t write) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем функцию обратного вызова
	this->_callback = callback;
}
