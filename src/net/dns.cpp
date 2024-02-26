/**
 * @file: dns.cpp
 * @date: 2023-08-05
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
#include <net/dns.hpp>

/**
 * host Метод извлечения хоста компьютера
 * @return хост компьютера с которого производится запрос
 */
string awh::DNS::Worker::host() const noexcept {
	// Результат работы функции
	string result = "";
	// Если список сетей установлен
	if(!this->_network.empty()){
		// Если количество элементов больше 1
		if(this->_network.size() > 1){
			// Подключаем устройство генератора
			mt19937 generator(const_cast <dns_t *> (this->_self)->_randev());
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
 * join Метод восстановления доменного имени
 * @param domain доменное имя для восстановления
 * @return       восстановленное доменное имя
 */
string awh::DNS::Worker::join(const vector <u_char> & domain) const noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя передано
	if(!domain.empty()){
		// Количество символов в слове
		uint16_t length = 0, offset = 0;
		// Переходим по всему доменному имени
		for(uint16_t i = 0; i < static_cast <uint16_t> (domain.size()); ++i){
			// Получаем количество символов
			length = static_cast <uint16_t> (domain[i]);
			// Выполняем перебор всех символов
			for(uint16_t j = 0; j < length; ++j){
				// Добавляем поддомен в строку результата
				result.append(1, domain[j + 1 + offset]);
				// Выполняем смещение в строке
				++i;
			}
			// Запоминаем значение смещения
			offset = (i + 1);
			// Если получили нулевой символ, выходим
			if(domain[i + 1] == 0) break;
			// Добавляем разделительную точку
			result.append(1, '.');
		}
	}
	// Выводим результат
	return result;
}
/**
 * split Метод разбивки доменного имени
 * @param domain доменное имя для разбивки
 * @return       разбитое доменное имя
 */
vector <u_char> awh::DNS::Worker::split(const string & domain) const noexcept {
	// Результат работы функции
	vector <u_char> result;
	// Если доменное имя передано
	if(!domain.empty()){
		// Секции доменного имени
		vector <string> sections;
		// Выполняем сплит доменного имени
		this->_self->_fmk->split(domain, ".", sections);
		// Если секции доменного имени получены
		if(!sections.empty()){
			// Переходим по всему списку секций
			for(auto & section : sections){
				// Добавляем в буфер данных размер записи
				result.push_back(static_cast <u_char> (section.size()));
				// Добавляем в буфер данные секции
				result.insert(result.end(), section.begin(), section.end());
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * extract Метод извлечения записи из ответа DNS
 * @param data буфер данных из которого нужно извлечь запись
 * @param pos  позиция в буфере данных
 * @return     запись в текстовом виде из ответа DNS
 */
vector <u_char> awh::DNS::Worker::extract(u_char * data, const size_t pos) const noexcept {
	// Результат работы функции
	vector <u_char> result;
	// Если данные переданы
	if(data != nullptr){
		// Устанавливаем итератор перебора
		int j = 0;
		// Перераспределяем результирующий буфер
		result.resize(254, 0);
		// Получаем временное значение буфера данных
		u_char * temp = (u_char *) &data[pos];
		// Выполняем перебор полученного буфера данных
		while((* temp) != 0){
			// Если найдено значение
			if((* temp) == 0xC0){
				// Увеличивам значение в буфере
				++temp;
				// Получаем новое значение буфера
				temp = (u_char *) &data[* temp];
			// Если значение не найдено
			} else {
				// Устанавливаем новое значение буфера
				result[j] = (* temp);
				// Увеличивам значение итератора
				++j;
				// Увеличивам значение в буфере
				++temp;
			}
		}
		// Устанавливаем конец строки
		result[j] = '\0';
	}
	// Выводим результат
	return result;
}
/**
 * close Метод закрытия подключения
 */
void awh::DNS::Worker::close() noexcept {
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
void awh::DNS::Worker::cancel() noexcept {
	// Если работа DNS-резолвера запущена
	if(this->_mode){
		// Выполняем остановку работы резолвера
		this->_mode = !this->_mode;
		// Выполняем закрытие подключения
		this->close();
	}
}
/**
 * request Метод выполнения запроса
 * @param domain название искомого домена
 * @return       полученный IP-адрес
 */
string awh::DNS::Worker::request(const string & domain) noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя передано
	if(!domain.empty()){
		// Получаем хост текущего компьютера
		const string & host = this->host();
		// Выполняем пересортировку серверов DNS
		const_cast <dns_t *> (this->_self)->shuffle(this->_family);
		// Определяем тип подключения
		switch(this->_family){
			// Для протокола IPv4
			case AF_INET: {
				// Если список серверов существует
				if((this->_mode = !this->_self->_serversIPv4.empty())){
					// Запоминаем запрашиваемое доменное имя
					this->_domain = domain;
					// Создаём объект клиента
					struct sockaddr_in client;
					// Создаём объект сервера
					struct sockaddr_in server;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Переходим по всему списку DNS-серверов
					for(auto & addr : this->_self->_serversIPv4){
						// Очищаем всю структуру для клиента
						::memset(&client, 0, sizeof(client));
						// Очищаем всю структуру для сервера
						::memset(&server, 0, sizeof(server));
						// Устанавливаем протокол интернета
						client.sin_family = this->_family;
						// Устанавливаем протокол интернета
						server.sin_family = this->_family;
						// Устанавливаем произвольный порт для локального подключения
						client.sin_port = htons(0);
						// Устанавливаем порт для локального подключения
						server.sin_port = htons(addr.port);
						// Устанавливаем адрес для подключения
						::memcpy(&server.sin_addr.s_addr, addr.ip, sizeof(addr.ip));
						// Устанавливаем адрес для локальго подключения
						inet_pton(this->_family, host.c_str(), &client.sin_addr.s_addr);
						// Выполняем копирование объекта подключения клиента
						::memcpy(&this->_peer.client, &client, this->_peer.size);
						// Выполняем копирование объекта подключения сервера
						::memcpy(&this->_peer.server, &server, this->_peer.size);
						// Обнуляем серверную структуру
						::memset(&((struct sockaddr_in *) (&this->_peer.server))->sin_zero, 0, sizeof(server.sin_zero));
						{
							// Временный буфер данных для преобразования IP-адреса
							char buffer[INET_ADDRSTRLEN];
							// Выполняем запрос на удалённый DNS-сервер
							result = this->send(host, inet_ntop(this->_family, &addr.ip, buffer, sizeof(buffer)));
							// Если результат получен или получение данных закрыто, тогда выходим из цикла
							if(!result.empty() || !this->_mode)
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
					// Запоминаем запрашиваемое доменное имя
					this->_domain = domain;
					// Создаём объект клиента
					struct sockaddr_in6 client;
					// Создаём объект сервера
					struct sockaddr_in6 server;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Переходим по всему списку DNS-серверов
					for(auto & addr : this->_self->_serversIPv6){
						// Очищаем всю структуру для клиента
						::memset(&client, 0, sizeof(client));
						// Очищаем всю структуру для сервера
						::memset(&server, 0, sizeof(server));
						// Устанавливаем протокол интернета
						client.sin6_family = this->_family;
						// Устанавливаем протокол интернета
						server.sin6_family = this->_family;
						// Устанавливаем произвольный порт для локального подключения
						client.sin6_port = htons(0);
						// Устанавливаем порт для локального подключения
						server.sin6_port = htons(addr.port);
						// Устанавливаем адрес для подключения
						::memcpy(&server.sin6_addr, addr.ip, sizeof(addr.ip));
						// Устанавливаем адрес для локальго подключения
						inet_pton(this->_family, host.c_str(), &client.sin6_addr);
						// Выполняем копирование объекта подключения клиента
						::memcpy(&this->_peer.client, &client, this->_peer.size);
						// Выполняем копирование объекта подключения сервера
						::memcpy(&this->_peer.server, &server, this->_peer.size);
						{
							// Временный буфер данных для преобразования IP-адреса
							char buffer[INET6_ADDRSTRLEN];
							// Выполняем запрос на удалённый DNS-сервер
							result = this->send(host, inet_ntop(this->_family, &addr.ip, buffer, sizeof(buffer)));
							// Если результат получен или получение данных закрыто, тогда выходим из цикла
							if(!result.empty() || !this->_mode)
								// Выходим из цикла
								break;
						}
					}
				}
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * Метод отправки запроса на удалённый сервер DNS
 * @param from адрес компьютера с которого выполняется запрос
 * @param to   адрес DNS-сервера на который выполняется запрос
 * @return     полученный IP-адрес
 */
string awh::DNS::Worker::send(const string & from, const string & to) noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя установлено
	if(this->_mode && !this->_domain.empty() && !from.empty() && !to.empty()){
		// Буфер пакета данных
		u_char buffer[65536];
		// Выполняем зануление буфера данных
		::memset(buffer, 0, sizeof(buffer));
		// Получаем объект заголовка
		head_t * header = reinterpret_cast <head_t *> (&buffer);
		// Устанавливаем идентификатор заголовка
		header->id = static_cast <u_short> (htons(getpid()));
		// Заполняем оставшуюся структуру пакетов
		header->z = 0;
		header->qr = 0;
		header->aa = 0;
		header->tc = 0;
		header->rd = 1;
		header->ra = 0;
		header->rcode = 0;
		header->opcode = 0;
		header->ancount = 0x0000;
		header->nscount = 0x0000;
		header->arcount = 0x0000;
		header->qdcount = htons(static_cast <u_short> (1));
		// Получаем размер запроса
		size_t size = sizeof(head_t);
		// Получаем доменное имя в нужном формате
		const auto & domain = this->split(this->_domain);
		// Выполняем копирование домена
		::memcpy(&buffer[size], domain.data(), domain.size());
		// Увеличиваем размер запроса
		size += (domain.size() + 1);
		// Создаём части флагов вопроса пакета запроса
		qflags_t * qflags = reinterpret_cast <qflags_t *> (&buffer[size]);
		// Определяем тип подключения
		switch(this->_family){
			// Для протокола IPv4
			case AF_INET:
				// Устанавливаем тип флага запроса
				qflags->qtype = htons(0x0001);
			break;
			// Для протокола IPv6
			case AF_INET6:
				// Устанавливаем тип флага запроса
				qflags->qtype = htons(0x1C);
			break;
		}
		// Устанавливаем класс флага запроса
		qflags->qclass = htons(0x0001);
		// Увеличиваем размер запроса
		size += sizeof(qflags_t);
		// Создаём сокет подключения
		this->_fd = ::socket(this->_family, SOCK_DGRAM, IPPROTO_UDP);
		// Если сокет не создан создан и работа резолвера не остановлена
		if(this->_mode && (this->_fd == INVALID_SOCKET)){
			// Выводим в лог сообщение
			this->_self->_log->print("File descriptor needed for the DNS request could not be allocated", log_t::flag_t::WARNING);
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
			// this->_socket.bufferSize(this->_fd, sizeof(buffer), 1, socket_t::mode_t::READ);
			// Устанавливаем размер буфера передачи данных на запись
			// this->_socket.bufferSize(this->_fd, sizeof(buffer), 1, socket_t::mode_t::WRITE);
			// Устанавливаем таймаут на получение данных из сокета
			this->_socket.timeout(this->_fd, this->_self->_timeout * 1000, socket_t::mode_t::READ);
			// Устанавливаем таймаут на запись данных в сокет
			this->_socket.timeout(this->_fd, this->_self->_timeout * 1000, socket_t::mode_t::WRITE);
			// Выполняем бинд на сокет
			if(::bind(this->_fd, (struct sockaddr *) (&this->_peer.client), this->_peer.size) < 0){
				// Выводим в лог сообщение
				this->_self->_log->print("Bind local network [%s]", log_t::flag_t::CRITICAL, from.c_str());
				// Выходим из функции
				return result;
			}
			// Если запрос на сервер DNS успешно отправлен
			if((bytes = ::sendto(this->_fd, (const char *) buffer, size, 0, (struct sockaddr *) &this->_peer.server, this->_peer.size)) > 0){
				// Буфер пакета данных
				u_char buffer[65536];
				// Получаем объект DNS-сервера
				dns_t * self = const_cast <dns_t *> (this->_self);
				// Выполняем зануление буфера данных
				::memset(buffer, 0, sizeof(buffer));
				// Выполняем чтение ответа сервера
				const int64_t bytes = ::recvfrom(this->_fd, (char *) buffer, sizeof(buffer), 0, (struct sockaddr *) &this->_peer.server, &this->_peer.size);
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
								self->_log->print("EPIPE [server = %s, domain = %s]", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
							break;
							// Если произведён сброс подключения
							case ECONNRESET:
								// Выводим в лог сообщение
								self->_log->print("ECONNRESET [server = %s, domain = %s]", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
							break;
							// Для остальных ошибок
							default:
								// Выводим в лог сообщение
								self->_log->print("%s [server = %s, domain = %s]", log_t::flag_t::WARNING, strerror(errno), to.c_str(), this->_domain.c_str());
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
					// Получаем объект заголовка
					head_t * header = reinterpret_cast <head_t *> (&buffer);
					// Определяем код выполнения операции
					switch(header->rcode){
						// Если операция выполнена удачно
						case 0: {
							// Получаем размер ответа
							size_t size = sizeof(head_t);
							// Получаем название доменного имени
							const string & qname = this->join(vector <u_char> (buffer + size, buffer + (size + 255)));
							// Увеличиваем размер буфера полученных данных
							size += (qname.size() + 2);
							// Создаём части флагов вопроса пакета ответа
							rr_flags_t * rrflags = nullptr;
							// Создаём части флагов вопроса пакета ответа
							qflags_t * qflags = reinterpret_cast <qflags_t *> (&buffer[size]);
							// Увеличиваем размер ответа
							size += sizeof(qflags_t);
							// Получаем количество записей
							const u_short count = ntohs(header->ancount);
							// Создаём список полученных типов записей
							vector <u_int> type(count, 0);
							// Получаем список названий записей
							vector <string> name(count, "");
							// Получаем список значений записей
							vector <string> rdata(count, "");
							// Выполняем перебор всех полученных записей
							for(u_short i = 0; i < count; ++i){
								// Выполняем извлечение названия записи
								name[i] = this->join(this->extract(buffer, size));
								// Увеличиваем размер полученных данных
								size += 2;
								// Создаём части флагов вопроса пакета ответа
								rrflags = reinterpret_cast <rr_flags_t *> (&buffer[size]);
								// Увеличиваем размер ответа
								size = ((size + sizeof(rr_flags_t)) - 2);
								// Определяем тип записи
								switch(ntohs(rrflags->rtype)){
									// Если запись является интернет-протоколом IPv4
									case 1:
									// Если запись является интернет-протоколом IPv6
									case 28: {
										// Изменяем размер извлекаемых данных
										rdata[i].resize(ntohs(rrflags->rdlength), 0);
										// Выполняем парсинг IP-адреса
										for(int j = 0; j < ntohs(rrflags->rdlength); ++j)
											// Выполняем парсинг IP-адреса
											rdata[i][j] = static_cast <u_char> (buffer[size + j]);
										// Устанавливаем тип полученных данных
										type[i] = ntohs(rrflags->rtype);
									} break;
									// Если запись является каноническим именем
									case 5: {
										// Выполняем извлечение значение записи
										rdata[i] = this->join(this->extract(buffer, size));
										// Устанавливаем тип полученных данных
										type[i] = ntohs(rrflags->rtype);
									} break;
								}
								// Увеличиваем размер полученных данных
								size += ntohs(rrflags->rdlength);
							}
							// Список IP-адресов
							vector <string> ips;
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим начальный разделитель
								printf ("------------------------------------------------------------\n\n");
								// Выводим заголовок
								printf("DNS server response:\n");
								// Выводим название доменного имени
								printf("QNAME: %s\n", qname.c_str());
								// Переходим по всему списку записей
								for(int i = 0; i < count; ++i){
									// Выводим название записи
									printf("\nNAME: %s\n", name[i].c_str());
									// Определяем тип записи
									switch(type[i]){
										// Если тип полученной записи CNAME
										case 5: printf("CNAME: %s\n", rdata[i].c_str()); break;
										// Если тип полученной записи IPv4
										case 1:
										// Если тип полученной записи IPv6
										case 28: {
											// Создаём буфер данных
											char buffer[INET6_ADDRSTRLEN];
											// Зануляем буфер данных
											::memset(buffer, 0, sizeof(buffer));
											// Получаем IP-адрес принадлежащий доменному имени
											const string ip = inet_ntop(this->_family, rdata[i].c_str(), buffer, sizeof(buffer));
											// Если IP-адрес получен
											if(!ip.empty()){
												// Если чёрный список IP-адресов получен
												if((count == 1) || !self->isInBlackList(this->_family, this->_domain, ip)){
													// Добавляем IP-адрес в список адресов
													ips.push_back(ip);
													// Записываем данные в кэш
													self->setToCache(this->_family, this->_domain, ip);
												}
												// Выводим информацию об IP-адресе
												printf("IPv4: %s\n", ip.c_str());
											}
										} break;
									}
								}
								// Выводим конечный разделитель
								printf ("\n------------------------------------------------------------\n\n");
							/**
							 * Если режим отладки отключён
							 */
							#else
								// Создаём буфер данных
								char buffer[INET6_ADDRSTRLEN];
								// Переходим по всему списку записей
								for(int i = 0; i < count; ++i){
									// Если тип полученной записи IPv4 или IPv6
									if((type[i] == 1) || (type[i] == 28)){
										// Зануляем буфер данных
										::memset(buffer, 0, sizeof(buffer));
										// Получаем IP-адрес принадлежащий доменному имени
										const string ip = inet_ntop(this->_family, rdata[i].c_str(), buffer, sizeof(buffer));
										// Если IP-адрес получен
										if(!ip.empty()){
											// Если чёрный список IP-адресов получен
											if((count == 1) || !self->isInBlackList(this->_family, this->_domain, ip)){
												// Добавляем IP-адрес в список адресов
												ips.push_back(ip);
												// Записываем данные в кэш
												self->setToCache(this->_family, this->_domain, ip);
											}
										}
									}
								}
							#endif
							// Если список IP-адресов получен
							if(!ips.empty()){
								// Если количество IP-адресов в списке больше 1-го
								if(ips.size() > 1){
									// Переходим по всему списку полученных адресов
									for(auto & addr : ips){
										// Если IP-адрес не найден в списке
										if(self->_using.count(addr) < 1){
											// Выполняем установку IP-адреса
											result = std::move(addr);
											// Выходим из цикла
											break;
										}
									}
								}
								// Если IP-адрес не установлен
								if(result.empty()){
									// Выполняем установку IP-адреса
									result = std::move(ips.front());
									// Если количество IP-адресов в списке больше 1-го
									if(ips.size() > 1){
										// Получаем текущее значение адреса
										auto it = ips.begin();
										// Выполняем смещение итератора
										std::advance(it, 1);
										// Переходим по всему списку полученных адресов
										for(; it != ips.end(); ++it)
											// Очищаем список используемых IP-адресов
											self->_using.erase(* it);
									}
								// Если IP-адрес получен, то запоминаем полученный адрес
								} else self->_using.emplace(result);
							}
						} break;
						// Если сервер DNS не смог интерпретировать запрос
						case 1:
							// Выводим в лог сообщение
							self->_log->print("DNS query format error to nameserver %s for domain %s", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
						break;
						// Если проблемы возникли на DNS-сервера
						case 2:
							// Выводим в лог сообщение
							self->_log->print("DNS server failure %s for domain %s", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
						break;
						// Если доменное имя указанное в запросе не существует
						case 3:
							// Выводим в лог сообщение
							self->_log->print("Domain name %s referenced in the query for nameserver %s does not exist", log_t::flag_t::WARNING, this->_domain.c_str(), to.c_str());
						break;
						// Если DNS-сервер не поддерживает подобный тип запросов
						case 4:
							// Выводим в лог сообщение
							self->_log->print("DNS server is not implemented at %s for domain %s", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
						break;
						// Если DNS-сервер отказался выполнять наш запрос (например по политическим причинам)
						case 5:
							// Выводим в лог сообщение
							self->_log->print("DNS request is refused to nameserver %s for domain %s", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
						break;
					}
				}
				// Если мы получили результат
				if(!result.empty())
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
							this->_self->_log->print("EPIPE [server = %s, domain = %s]", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
						break;
						// Если произведён сброс подключения
						case ECONNRESET:
							// Выводим в лог сообщение
							this->_self->_log->print("ECONNRESET [server = %s, domain = %s]", log_t::flag_t::WARNING, to.c_str(), this->_domain.c_str());
						break;
						// Для остальных ошибок
						default:
							// Выводим в лог сообщение
							this->_self->_log->print("%s [server = %s, domain = %s]", log_t::flag_t::WARNING, strerror(errno), to.c_str(), this->_domain.c_str());
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
awh::DNS::Worker::~Worker() noexcept {
	// Выполняем закрытие файлового дерскриптора (сокета)
	this->close();
}
/**
 * encode Метод кодирования интернационального доменного имени
 * @param domain доменное имя для кодирования
 * @return       результат работы кодирования
 */
string awh::DNS::encode(const string & domain) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Если используется модуль IDN
	 */
	#if defined(AWH_IDN)
		// Если доменное имя передано
		if(!domain.empty()){
			/**
			 * Если операционной системой является Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Результирующий буфер данных
				wchar_t buffer[255];
				// Выполняем кодирования доменного имени
				if(IdnToAscii(0, this->_fmk->convert(domain).c_str(), -1, buffer, sizeof(buffer)) == 0)
					// Выводим в лог сообщение
					this->_log->print("IDN encode failed (%d)", log_t::flag_t::CRITICAL, GetLastError());
				// Получаем результат кодирования
				else result = this->_fmk->convert(wstring(buffer));
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#else
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем кодирования доменного имени
				const int rc = idn2_to_ascii_8z(domain.c_str(), &buffer, IDN2_NONTRANSITIONAL);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS)
					// Выводим в лог сообщение
					this->_log->print("IDN encode failed (%d): %s", log_t::flag_t::CRITICAL, rc, idn2_strerror(rc));
				// Получаем результат кодирования
				else result = buffer;
				// Очищаем буфер данных
				free(buffer);
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * decode Метод декодирования интернационального доменного имени
 * @param domain доменное имя для декодирования
 * @return       результат работы декодирования
 */
string awh::DNS::decode(const string & domain) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Если используется модуль IDN
	 */
	#if defined(AWH_IDN)
		// Если доменное имя передано
		if(!domain.empty()){
			/**
			 * Если операционной системой является Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Результирующий буфер данных
				wchar_t buffer[255];
				// Выполняем кодирования доменного имени
				if(IdnToUnicode(0, this->_fmk->convert(domain).c_str(), -1, buffer, sizeof(buffer)) == 0)
					// Выводим в лог сообщение
					this->_log->print("IDN decode failed (%d)", log_t::flag_t::CRITICAL, GetLastError());
				// Получаем результат кодирования
				else result = this->_fmk->convert(wstring(buffer));
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#else
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем декодирования доменного имени
				const int rc = idn2_to_unicode_8z8z(domain.c_str(), &buffer, 0);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS)
					// Выводим в лог сообщение
					this->_log->print("IDN decode failed (%d): %s", log_t::flag_t::CRITICAL, rc, idn2_strerror(rc));
				// Получаем результат декодирования
				else result = buffer;
				// Очищаем буфер данных
				free(buffer);
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * clear Метод очистки данных DNS-резолвера
 * @return результат работы функции
 */
bool awh::DNS::clear() noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if((result = hold.access({}, status_t::CLEAR))){
		// Выполняем сброс кэша DNS-резолвера
		this->flush();
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
 * flush Метод сброса кэша DNS-резолвера
 * @return результат работы функции
 */
bool awh::DNS::flush() noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if((result = hold.access({status_t::CLEAR}, status_t::FLUSH))){
		// Выполняем сброс кэша полученных IPv4-адресов
		this->_cacheIPv4.clear();
		// Выполняем сброс кэша полученных IPv6-адресов
		this->_cacheIPv6.clear();
	}
	// Выводим результат
	return result;
}
/**
 * cancel Метод отмены выполнения запроса
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::DNS::cancel(const int family) noexcept {
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
 * shuffle Метод пересортировки серверов DNS
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::DNS::shuffle(const int family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выбираем стаднарт рандомайзера
	mt19937 generator(this->_randev());
	// Определяем тип протокола подключения
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int> (AF_INET):
			// Выполняем рандомную сортировку списка DNS-серверов
			::shuffle(this->_serversIPv4.begin(), this->_serversIPv4.end(), generator);
		break;
		// Если тип протокола подключения IPv6
		case static_cast <int> (AF_INET6):
			// Выполняем рандомную сортировку списка DNS-серверов
			::shuffle(this->_serversIPv6.begin(), this->_serversIPv6.end(), generator);
		break;
	}
}
/**
 * timeout Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::DNS::timeout(const uint8_t sec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем установку таймаута ожидания выполнения запроса
	this->_timeout = sec;
}
/**
 * timeToLive Метод установки времени жизни DNS-кэша
 * @param ttl время жизни DNS-кэша в миллисекундах
 */
void awh::DNS::timeToLive(const time_t ttl) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем установку DNS-кэша
	this->_ttl = ttl;
}
/**
 * cache Метод получения IP-адреса из кэша
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @return       IP-адрес находящийся в кэше
 */
string awh::DNS::cache(const int family, const string & domain) noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя передано
	if(!domain.empty()){
		// Список полученных IP-адресов
		vector <string> ips;
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Если кэш доменных имён проинициализирован
				if(!this->_cacheIPv4.empty()){
					// Временный буфер данных для преобразования IP-адреса
					char buffer[INET_ADDRSTRLEN];
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv4.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto it = ret.first; it != ret.second;){
						// Если IP-адрес не находится в чёрном списке
						if(!it->second.forbidden && !it->second.localhost){
							// Если время жизни кэша ещё не вышло
							if((it->second.create == 0) || ((this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - it->second.create) <= this->_ttl)){
								// Выполняем формирование списка полученных IP-адресов
								ips.push_back(inet_ntop(family, &it->second.ip, buffer, sizeof(buffer)));
								// Выполняем смещение итератора
								++it;
							// Если время жизни кэша уже вышло
							} else {
								// Выполняем блокировку потока
								const lock_guard <recursive_mutex> lock(this->_mtx);
								// Выполняем удаление записи из кэша
								it = this->_cacheIPv4.erase(it);
							}
						// Выполняем смещение итератора
						} else ++it;
					}
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Если кэш доменных имён проинициализирован
				if(!this->_cacheIPv6.empty()){
					// Временный буфер данных для преобразования IP-адреса
					char buffer[INET6_ADDRSTRLEN];
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv6.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto it = ret.first; it != ret.second;){
						// Если IP-адрес не находится в чёрном списке
						if(!it->second.forbidden && !it->second.localhost){
							// Если время жизни кэша ещё не вышло
							if((it->second.create == 0) || ((this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - it->second.create) <= this->_ttl)){
								// Выполняем формирование списка полученных IP-адресов
								ips.push_back(inet_ntop(family, &it->second.ip, buffer, sizeof(buffer)));
								// Выполняем смещение итератора
								++it;
							// Если время жизни кэша уже вышло
							} else {
								// Выполняем блокировку потока
								const lock_guard <recursive_mutex> lock(this->_mtx);
								// Выполняем удаление записи из кэша
								it = this->_cacheIPv6.erase(it);
							}
						// Выполняем смещение итератора
						} else ++it;
					}
				}
			} break;
		}
		// Если список IP-адресов получен
		if(!ips.empty()){
			// Подключаем устройство генератора
			mt19937 generator(this->_randev());
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, ips.size() - 1);
			// Выполняем получение результата
			result = std::forward <string> (ips.at(dist6(generator)));
		}
	}
	// Выводим результат
	return result;
}
/**
 * clearCache Метод очистки кэша для указанного доменного имени
 * @param domain доменное имя для которого выполняется очистка кэша
 */
void awh::DNS::clearCache(const string & domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		// Выполняем очистку кэша доменного имени для IPv4
		this->clearCache(AF_INET, domain);
		// Выполняем очистку кэша доменного имени для IPv6
		this->clearCache(AF_INET6, domain);
	}
}
/**
 * clearCache Метод очистки кэша для указанного доменного имени
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя для которого выполняется очистка кэша
 */
void awh::DNS::clearCache(const int family, const string & domain) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя передано
	if(!domain.empty()){
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second;){
					// Если мы IP-адрес не запрещён
					if(!it->second.forbidden)
						// Выполняем удаление IP-адреса
						it = this->_cacheIPv4.erase(it);
					// Иначе продолжаем перебор дальше
					else ++it;
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second;){
					// Если мы IP-адрес не запрещён
					if(!it->second.forbidden)
						// Выполняем удаление IP-адреса
						it = this->_cacheIPv6.erase(it);
					// Иначе продолжаем перебор дальше
					else ++it;
				}
			} break;
		}
	}
}
/**
 * clearCache Метод очистки кэша
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::clearCache(const bool localhost) noexcept {
	// Выполняем очистку кэша доменного имени для IPv4
	this->clearCache(AF_INET, localhost);
	// Выполняем очистку кэша доменного имени для IPv6
	this->clearCache(AF_INET6, localhost);
}
/**
 * clearCache Метод очистки кэша
 * @param family    тип интернет-протокола AF_INET, AF_INET6
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::clearCache(const int family, const bool localhost) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Определяем тип протокола подключения
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int> (AF_INET): {
			// Переходим по всему списку IP-адресов
			for(auto it = this->_cacheIPv4.begin(); it != this->_cacheIPv4.end();){
				// Если мы нашли нужный тип IP-адреса
				if(!it->second.forbidden && (it->second.localhost == localhost))
					// Выполняем удаление IP-адреса
					it = this->_cacheIPv4.erase(it);
				// Иначе продолжаем перебор дальше
				else ++it;
			}
		} break;
		// Если тип протокола подключения IPv6
		case static_cast <int> (AF_INET6): {
			// Переходим по всему списку IP-адресов
			for(auto it = this->_cacheIPv6.begin(); it != this->_cacheIPv6.end();){
				// Если мы нашли нужный тип IP-адреса
				if(!it->second.forbidden && (it->second.localhost == localhost))
					// Выполняем удаление IP-адреса
					it = this->_cacheIPv6.erase(it);
				// Иначе продолжаем перебор дальше
				else ++it;
			}
		} break;
	}
}
/**
 * setToCache Метод добавления IP-адреса в кэш
 * @param domain    доменное имя соответствующее IP-адресу
 * @param ip        адрес для добавления к кэш
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::setToCache(const string & domain, const string & ip, const bool localhost) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Определяем тип передаваемого IP-адреса
		switch(static_cast <uint8_t> (this->_net.host(ip))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем добавление IP-адреса в кэш
				this->setToCache(AF_INET, domain, ip, localhost);
			break;
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем добавление IP-адреса в кэш
				this->setToCache(AF_INET6, domain, ip, localhost);
			break;
		}
	}
}
/**
 * setToCache Метод добавления IP-адреса в кэш
 * @param family    тип интернет-протокола AF_INET, AF_INET6
 * @param domain    доменное имя соответствующее IP-адресу
 * @param ip        адрес для добавления к кэш
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::setToCache(const int family, const string & domain, const string & ip, const bool localhost) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Результат проверки наличия IP-адреса в кэше
		bool result = false;
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[1];
					// Выполняем копирование полученных данных в переданный буфер
					inet_pton(family, ip.c_str(), &buffer);
					// Выполняем проверку соответствует ли IP-адрес в кэше добавляемому сейчас
					result = (memcmp(it->second.ip, buffer, sizeof(buffer)) == 0);
					// Если IP-адрес соответствует переданному адресу, выходим
					if(result) break;
				}
				// Если IP-адрес в кэше не найден
				if(!result){
					// Создаём объект кэша
					cache_t <1> cache;
					// Устанавливаем флаг локального хоста
					cache.localhost = localhost;
					// Устанавливаем время создания кэша
					cache.create = (this->_ttl > 0 ? this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) : this->_ttl);
					// Выполняем копирование полученных данных в переданный буфер
					inet_pton(family, ip.c_str(), &cache.ip);
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv4.emplace(domain, std::move(cache));
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[4];
					// Выполняем копирование полученных данных в переданный буфер
					inet_pton(family, ip.c_str(), &buffer);
					// Выполняем проверку соответствует ли IP-адрес в кэше добавляемому сейчас
					result = (memcmp(it->second.ip, buffer, sizeof(buffer)) == 0);
					// Если IP-адрес соответствует переданному адресу, выходим
					if(result) break;
				}
				// Если IP-адрес в кэше не найден
				if(!result){
					// Создаём объект кэша
					cache_t <4> cache;
					// Устанавливаем флаг локального хоста
					cache.localhost = localhost;
					// Устанавливаем время жизни кэша
					cache.create = (this->_ttl > 0 ? this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) : this->_ttl);
					// Выполняем копирование полученных данных в переданный буфер
					inet_pton(family, ip.c_str(), &cache.ip);
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv6.emplace(domain, std::move(cache));
				}
			} break;
		}
	}
}
/**
 * clearBlackList Метод очистки чёрного списка
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::DNS::clearBlackList(const string & domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		// Выполняем очистку чёрного списка домена, для IPv4 адреса
		this->clearBlackList(AF_INET, domain);
		// Выполняем очистку чёрного списка домена, для IPv6 адреса
		this->clearBlackList(AF_INET6, domain);
	}
}
/**
 * clearBlackList Метод очистки чёрного списка
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::DNS::clearBlackList(const int family, const string & domain) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя передано
	if(!domain.empty()){
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Если мы нашли запрещённую запись
					if(it->second.forbidden)
						// Снимаем флаг запрещённого IP-адреса
						it->second.forbidden = !it->second.forbidden;
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Если мы нашли запрещённую запись
					if(it->second.forbidden)
						// Снимаем флаг запрещённого IP-адреса
						it->second.forbidden = !it->second.forbidden;
				}
			} break;
		}
	}
}
/**
 * delInBlackList Метод удаления IP-адреса из чёрного списока
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::DNS::delInBlackList(const string & domain, const string & ip) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Определяем тип передаваемого IP-адреса
		switch(static_cast <uint8_t> (this->_net.host(ip))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем удаление IPv4 адреса из чёрного списка
				this->delInBlackList(AF_INET, domain, ip);
			break;
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем удаление IPv6 адреса из чёрного списка
				this->delInBlackList(AF_INET6, domain, ip);
			break;
		}
	}
}
/**
 * delInBlackList Метод удаления IP-адреса из чёрного списока
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::DNS::delInBlackList(const int family, const string & domain, const string & ip) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){	
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Если мы нашли запрещённую запись
					if(it->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[1];
						// Выполняем копирование полученных данных в переданный буфер
						inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if(memcmp(it->second.ip, buffer, sizeof(buffer)) == 0)
							// Снимаем флаг запрещённого IP-адреса
							it->second.forbidden = !it->second.forbidden;
					}
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Если мы нашли запрещённую запись
					if(it->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[4];
						// Выполняем копирование полученных данных в переданный буфер
						inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if(memcmp(it->second.ip, buffer, sizeof(buffer)) == 0)
							// Снимаем флаг запрещённого IP-адреса
							it->second.forbidden = !it->second.forbidden;
					}
				}
			} break;
		}
	}
}
/**
 * setToBlackList Метод добавления IP-адреса в чёрный список
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::DNS::setToBlackList(const string & domain, const string & ip) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){	
		// Определяем тип передаваемого IP-адреса
		switch(static_cast <uint8_t> (this->_net.host(ip))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем добавление IPv4 адреса в чёрный список
				this->setToBlackList(AF_INET, domain, ip);
			break;
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем добавление IPv6 адреса в чёрный список
				this->setToBlackList(AF_INET6, domain, ip);
			break;
		}
	}
}
/**
 * setToBlackList Метод добавления IP-адреса в чёрный список
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::DNS::setToBlackList(const int family, const string & domain, const string & ip) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Создаём объект кэша
				cache_t <1> cache;
				// Устанавливаем флаг запрещённого домена
				cache.forbidden = true;
				// Выполняем копирование полученных данных в переданный буфер
				inet_pton(family, ip.c_str(), &cache.ip);
				// Если список кэша является пустым
				if(this->_cacheIPv4.empty())
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv4.emplace(domain, std::move(cache));
				// Если данные в кэше уже есть
				else {
					// Результат поиска IP-адреса
					bool result = false;
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv4.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto it = ret.first; it != ret.second; ++it){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[1];
						// Выполняем копирование полученных данных в переданный буфер
						inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (memcmp(it->second.ip, buffer, sizeof(buffer)) == 0))){
							// Выполняем блокировку IP-адреса
							it->second.forbidden = result;
							// Выходим из цикла
							break;
						}
					}
					// Если адрес не найден
					if(!result)
						// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
						this->_cacheIPv4.emplace(domain, std::move(cache));
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Создаём объект кэша
				cache_t <4> cache;
				// Устанавливаем флаг запрещённого домена
				cache.forbidden = true;
				// Выполняем копирование полученных данных в переданный буфер
				inet_pton(family, ip.c_str(), &cache.ip);
				// Если список кэша является пустым
				if(this->_cacheIPv6.empty())
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv6.emplace(domain, std::move(cache));
				// Если данные в кэше уже есть
				else {
					// Результат поиска IP-адреса
					bool result = false;
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv6.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto it = ret.first; it != ret.second; ++it){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[4];
						// Выполняем копирование полученных данных в переданный буфер
						inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (memcmp(it->second.ip, buffer, sizeof(buffer)) == 0))){
							// Выполняем блокировку IP-адреса
							it->second.forbidden = result;
							// Выходим из цикла
							break;
						}
					}
					// Если адрес не найден
					if(!result)
						// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
						this->_cacheIPv6.emplace(domain, std::move(cache));
				}
			} break;
		}
	}
}
/**
 * isInBlackList Метод проверки наличия IP-адреса в чёрном списке
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::DNS::isInBlackList(const string & domain, const string & ip) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Определяем тип передаваемого IP-адреса
		switch(static_cast <uint8_t> (this->_net.host(ip))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем проверку наличия IPv4 адреса в чёрном списоке
				return this->isInBlackList(AF_INET, domain, ip);
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем проверку наличия IPv6 адреса в чёрном списоке
				return this->isInBlackList(AF_INET6, domain, ip);
		}
	}
	// Выводим результат
	return result;
}
/**
 * isInBlackList Метод проверки наличия IP-адреса в чёрном списке
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::DNS::isInBlackList(const int family, const string & domain, const string & ip) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Если мы нашли запрещённую запись
					if(it->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[1];
						// Выполняем копирование полученных данных в переданный буфер
						inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (memcmp(it->second.ip, buffer, sizeof(buffer)) == 0)))
							// Выводим результат проверки
							return result;
					}
				}
			}
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Если мы нашли запрещённую запись
					if(it->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[4];
						// Выполняем копирование полученных данных в переданный буфер
						inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (memcmp(it->second.ip, buffer, sizeof(buffer)) == 0)))
							// Выводим результат проверки
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
 * server Метод получения данных сервера имён
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       запрошенный сервер имён
 */
string awh::DNS::server(const int family) noexcept {
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
 * server Метод добавления сервера DNS
 * @param server адрес DNS-сервера
 */
void awh::DNS::server(const string & server) noexcept {
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
 * server Метод добавления сервера DNS
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param server адрес DNS-сервера
 */
void awh::DNS::server(const int family, const string & server) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::NSS_SET}, status_t::NS_SET)){
		// Если адрес сервера передан
		if(!server.empty()){
			// Порт переданного сервера
			u_int port = 53;
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
					string ip = this->host(family, host);
					// Если IP-адрес мы не получили, выполняем запрос на сервер
					if(ip.empty()){
						// Создаём объект DNS-резолвера
						dns_t dns(this->_fmk, this->_log);
						// Выполняем запрос IP-адреса с удалённого сервера
						host = dns.resolve(family, host);
					// Выполняем установку IP-адреса
					} else host = std::move(ip);
				} break;
				// Значит скорее всего, садрес является доменным именем
				default: {
					// Выполняем поиск разделителя порта
					const size_t pos = server.rfind(":");
					// Если позиция разделителя найдена
					if(pos != string::npos){
						// Извлекаем хост сервера имён
						host = this->host(family, server.substr(0, pos));
						// Извлекаем порт сервера имён
						port = static_cast <u_int> (::stoi(server.substr(pos + 1)));
					// Извлекаем хост сервера имён
					} else host = this->host(family, server);
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
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD DNS SERVER ^^^^^^^^^\x1B[0m" << endl;
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
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD DNS SERVER ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры запроса
								cout << "[" << host << "]:" << port << endl;
							#endif
						}
					} break;
				}
			// Если имя сервера не получено, выводим в лог сообщение
			} else this->_log->print("DNS IPv%u server %s does not add", log_t::flag_t::WARNING, (family == AF_INET6 ? 6 : 4), server.c_str());
		}
	}
}
/**
 * servers Метод добавления серверов DNS
 * @param servers адреса DNS-серверов
 */
void awh::DNS::servers(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если список серверов передан
	if(!servers.empty()){
		// Создаём объект холдирования
		hold_t <status_t> hold(this->_status);
		// Если статус работы DNS-резолвера соответствует
		if(hold.access({status_t::NSS_REP}, status_t::NSS_SET)){
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
 * servers Метод добавления серверов DNS
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers адреса DNS-серверов
 */
void awh::DNS::servers(const int family, const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если список серверов передан
	if(!servers.empty()){	
		// Создаём объект холдирования
		hold_t <status_t> hold(this->_status);
		// Если статус работы DNS-резолвера соответствует
		if(hold.access({status_t::NSS_REP}, status_t::NSS_SET)){
			// Переходим по всем нейм серверам и добавляем их
			for(auto & server : servers)
				// Выполняем добавление нового сервера
				this->server(family, server);
		}
	}
}
/**
 * replace Метод замены существующих серверов DNS
 * @param servers адреса DNS-серверов
 */
void awh::DNS::replace(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::RESOLVE}, status_t::NSS_REP)){
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
 * replace Метод замены существующих серверов DNS
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers адреса DNS-серверов
 */
void awh::DNS::replace(const int family, const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::RESOLVE}, status_t::NSS_REP)){
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case static_cast <int> (AF_INET):
				// Выполняем очистку списка DNS-серверов
				this->_serversIPv4.clear();
			break;
			// Для протокола IPv6
			case static_cast <int> (AF_INET6):
				// Выполняем очистку списка DNS-серверов
				this->_serversIPv6.clear();
			break;
		}
		// Если нейм сервера переданы, удаляем все настроенные серверы имён и приостанавливаем все ожидающие решения
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
					servers = IPV4_RESOLVER;
				break;
				// Для протокола IPv6
				case static_cast <int> (AF_INET6):
					// Устанавливаем список серверов
					servers = IPV6_RESOLVER;
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
void awh::DNS::network(const vector <string> & network) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы установки параметров сети соответствует
	if(hold.access({}, status_t::NET_SET)){
		// Если список адресов сетевых плат передан
		if(!network.empty()){
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
						string ip = this->host(AF_INET6, host);
						// Если результат получен, выполняем пинг
						if(!ip.empty())
							// Выполняем добавление полученного хоста в список
							this->_workerIPv6->_network.push_back(ip);
						// Если результат не получен, выполняем получение IPv4 адреса
						else {
							// Выполняем получение IP-адреса для IPv4
							ip = this->host(AF_INET, host);
							// Если IP-адрес успешно получен
							if(!ip.empty())
								// Выполняем добавление полученного хоста в список
								this->_workerIPv4->_network.push_back(ip);
							// Выводим сообщение об ошибке
							else this->_log->print("Passed %s address is not legitimate", log_t::flag_t::WARNING, host.c_str());
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
void awh::DNS::network(const int family, const vector <string> & network) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
 * prefix Метод установки префикса переменной окружения
 * @param prefix префикс переменной окружения для установки
 */
void awh::DNS::prefix(const string & prefix) noexcept {
	// Выполняем установку префикса переменной окружения
	this->_prefix = prefix;
}
/**
 * hosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::DNS::hosts(const string & filename) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если адрес файла получен
	if(!filename.empty()){
		// Создаём объект для работы с файловой системой
		fs_t fs(this->_fmk, this->_log);
		// Выполняем установку адреса файла hosts
		const string & hostsFilename = fs.realPath(filename, false);
		// Если файл существует в файловой системе
		if(fs.isFile(hostsFilename)){
			// Выполняем очистку списка локальных IPv4 адресов из кэша
			this->clearCache(AF_INET, true);
			// Выполняем очистку списка локальных IPv6 адресов из кэша
			this->clearCache(AF_INET6, true);
			// Выполняем чтение содержимого файла
			fs.readFile(hostsFilename, [this](const string & entry) noexcept -> void {
				// Если запись не является комментарием
				if(!entry.empty() && (entry.size() > 1) && (entry.front() != '#')){
					// Хост который будем собирать
					string host = "";
					// Список полученных хостов
					vector <string> hosts;
					// Выполняем перебор всех полученных символов
					for(size_t i = 0; i < entry.size(); i++){
						// Выполняем получение текущего символа
						const u_char c = entry.at(i);
						// Если символ является комментарием
						if(c == '#')
							// Выходим из цикла
							break;
						// Если символ является пробелом
						if(::isspace(c) || (c == 32) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') || (c == '\v')){
							// Если хост уже собран
							if(!host.empty()){
								// Выполняем добавление хоста в список хостов
								hosts.push_back(host);
								// Выполняем очистку хоста
								host.clear();
							}
						// Выполняем сборку хоста
						} else host.append(1, c);
					}
					// Если хост уже собран
					if(!host.empty())
						// Выполняем добавление хоста в список хостов
						hosts.push_back(std::move(host));
					// Если количество хостов в списке больше одного
					if(hosts.size() > 1){
						// Тип интернет-протокола AF_INET, AF_INET6
						int family = 0;
						// Определяем тип передаваемого сервера
						switch(static_cast <uint8_t> (this->_net.host(hosts.front()))){
							// Если хост является доменом или IPv4 адресом
							case static_cast <uint8_t> (net_t::type_t::IPV4): family = AF_INET; break;
							// Если хост является IPv6 адресом, переводим ip адрес в полную форму
							case static_cast <uint8_t> (net_t::type_t::IPV6): family = AF_INET6; break;
						}
						// Если мы удачно определили тип интернет-протокола
						if(family > 0){
							// Выполняем перебор всего списка хостов
							for(size_t i = 1; i < hosts.size(); i++)
								// Выполняем добавление в кэш новый IP-адрес
								this->setToCache(family, hosts.at(i), hosts.front(), true);
						// Сообщаем, что определить IP-адрес не удалось
						} else this->_log->print("Entry provided [%s] is not an IP-address", log_t::flag_t::WARNING, hosts.front().c_str());
					// Выводим сообщение, что текст передан неверный
					} else this->_log->print("Hosts in entry %s not found", log_t::flag_t::WARNING, entry.c_str());
				}
			});
		}
	// Если имя сервера не получено, выводим в лог сообщение
	} else this->_log->print("Hosts file address is not passed", log_t::flag_t::WARNING);
}
/**
 * host Метод определение локального IP-адреса по имени домена
 * @param name название сервера
 * @return     полученный IP-адрес
 */
string awh::DNS::host(const string & name) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем получение IP-адреса для IPv6
	const string & result = this->host(AF_INET6, name);
	// Если результат не получен, выполняем запрос для IPv4
	if(result.empty())
		// Выполняем получение IP-адреса для IPv4
		return this->host(AF_INET, name);
	// Выводим результат
	return result;
}
/**
 * host Метод определение локального IP-адреса по имени домена
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param name   название сервера
 * @return       полученный IP-адрес
 */
string awh::DNS::host(const int family, const string & name) noexcept {
	// Результат работы функции
	string result = "";
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({}, status_t::RESOLVE)){
		// Если домен передан
		if(!name.empty()){
			// Если доменное имя является локальным
			if(this->_fmk->is(name, fmk_t::check_t::LATIAN)){
				// Переводим доменное имя в нижний регистр
				this->_fmk->transform(name, fmk_t::transform_t::LOWER);
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем резолвинг доменного имени
					struct hostent * domain = ::gethostbyname(name.c_str());
					// Если хост мы не получили
					if(domain == nullptr){
						// Выполняем извлечение текста ошибки
						DWORD error = WSAGetLastError();
						// Если текст ошибки мы получили
						if(error != 0){
							// Если хост мы не нашли
							if(error == WSAHOST_NOT_FOUND)
								// Выводим сообщение об ошибке
								this->_log->print("Hosts %s not found", log_t::flag_t::WARNING, name.c_str());
							// Если в записи записи хоста в DNS-сервере обнаружено
							else if(error == WSANO_DATA)
								// Выводим сообщение об ошибке
								this->_log->print("No data record found for %s", log_t::flag_t::WARNING, name.c_str());
							// Выводим сообщение об ошибке
							else this->_log->print("An error occured while verifying %s", log_t::flag_t::CRITICAL, name.c_str());
						}
						// Выходим из функции
						return result;
					}
				/**
				 * Если операционной системой является Nix-подобная
				 */
				#else
					// Переменная получения ошибки
					h_errno = 0;
					// Выполняем резолвинг доменного имени
					struct hostent * domain = ::gethostbyname2(name.c_str(), family);
					// Если хост мы не получили
					if(domain == nullptr){
						// Определяем тип ошибки
						switch(h_errno){
							// Если DNS-сервер в данный момент не доступен
							case TRY_AGAIN:
								// Выводим сообщение об ошибке
								this->_log->print("Unable to obtain an answer from a DNS-server for %s", log_t::flag_t::WARNING, name.c_str());
							break;
							// Если адрес не найден
							case NO_ADDRESS:
								// Выводим сообщение об ошибке
								this->_log->print("%s is valid, but lacks a corresponding IP-address", log_t::flag_t::WARNING, name.c_str());
							break;
							// Если доменное имя не найдено
							case HOST_NOT_FOUND:
								// Выводим сообщение об ошибке
								this->_log->print("Hosts %s not found", log_t::flag_t::WARNING, name.c_str());
							break;
							// Выводим сообщение по умолчанию
							default: this->_log->print("An error occured while verifying %s", log_t::flag_t::CRITICAL, name.c_str());
						}
						// Выходим из функции
						return result;
					}
				#endif
				// Индекс полученного IP-адреса
				size_t index = 0;
				// Список полученных IP-адресов
				vector <string> ips;
				// Определяем тип протокола подключения
				switch(family){
					// Если тип протокола подключения IPv4
					case static_cast <int> (AF_INET): {
						// Создаём объект сервера
						struct sockaddr_in server;
						// Создаём буфер данных для получения результата
						char buffer[INET_ADDRSTRLEN];
						// Выполняем перебор всего списка полученных IP-адресов
						while(domain->h_addr_list[index] != 0){
							// Выполняем зануление буфера данных
							::memset(buffer, 0, sizeof(buffer));
							// Очищаем всю структуру для сервера
							::memset(&server, 0, sizeof(server));
							// Устанавливаем протокол интернета
							server.sin_family = family;
							// Выполняем копирование данных типа подключения
							::memcpy(&server.sin_addr.s_addr, domain->h_addr_list[index++], domain->h_length);
							// Копируем полученные данные IP-адреса
							ips.push_back(inet_ntop(family, &server.sin_addr, buffer, sizeof(buffer)));
						}
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <int> (AF_INET6): {
						// Создаём объект сервера
						struct sockaddr_in6 server;
						// Создаём буфер данных для получения результата
						char buffer[INET6_ADDRSTRLEN];
						// Выполняем перебор всего списка полученных IP-адресов
						while(domain->h_addr_list[index] != 0){
							// Выполняем зануление буфера данных
							::memset(buffer, 0, sizeof(buffer));
							// Очищаем всю структуру для сервера
							::memset(&server, 0, sizeof(server));
							// Устанавливаем протокол интернета
							server.sin6_family = family;
							// Выполняем копирование данных типа подключения
							::memcpy(&server.sin6_addr.s6_addr, domain->h_addr_list[index++], domain->h_length);
							// Копируем полученные данные IP-адреса
							ips.push_back(inet_ntop(family, &server.sin6_addr, buffer, sizeof(buffer)));
						}
					} break;
				}
				// Если список IP-адресов получен
				if(!ips.empty()){
					// Подключаем устройство генератора
					mt19937 generator(this->_randev());
					// Выполняем генерирование случайного числа
					uniform_int_distribution <mt19937::result_type> dist6(0, ips.size() - 1);
					// Выполняем получение результата
					result = std::forward <string> (ips.at(dist6(generator)));
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * resolve Метод ресолвинга домена
 * @param host хост сервера
 * @return     полученный IP-адрес
 */
string awh::DNS::resolve(const string & host) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем получение IP-адреса для IPv6
	const string & result = this->resolve(AF_INET6, host);
	// Если результат не получен, выполняем запрос для IPv4
	if(result.empty())
		// Выполняем получение IP-адреса для IPv4
		return this->resolve(AF_INET, host);
	// Выводим результат
	return result;
}
/**
 * resolve Метод ресолвинга домена
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param host   хост сервера
 * @return       полученный IP-адрес
 */
string awh::DNS::resolve(const int family, const string & host) noexcept {
	// Результат работы функции
	string result = "";
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({}, status_t::RESOLVE)){
		// Если домен передан
		if(!host.empty()){
			/**
			 * Если используется модуль IDN
			 */
			#if AWH_IDN
				// Получаем доменное имя в интернациональном виде
				const string & domain = this->encode(host);
			/**
			 * Если модуль IDN не используется
			 */
			#else
				// Получаем доменное имя как оно есть
				const string & domain = host;
			#endif
			// Определяем тип передаваемого сервера
			switch(static_cast <uint8_t> (this->_net.host(domain))){
				// Если домен является IPv4 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Если домен является IPv6 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6):
					// Выводим переданый хост обратно
					return host;
				break;
				// Если домен является аппаратным адресом сетевого интерфейса
				case static_cast <uint8_t> (net_t::type_t::MAC):
				// Если домен является адресом/Маски сети
				case static_cast <uint8_t> (net_t::type_t::NETW):
				// Если домен является адресом в файловой системе
				case static_cast <uint8_t> (net_t::type_t::ADDR):
				// Если домен является HTTP адресом
				case static_cast <uint8_t> (net_t::type_t::HTTP):
					// Сообщаем, что адрес нам не подходит
					return result;
				break;
				// Если домен является доменным именем
				case static_cast <uint8_t> (net_t::type_t::DOMN): {
					// Выполняем поиск IP-адреса в кэше DNS
					result = this->cache(family, domain);
					// Если IP-адрес получен
					if(!result.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ DOMAIN RESOLVE ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << domain << endl;
							// Определяем тип протокола подключения
							switch(family){
								// Если тип протокола подключения IPv4
								case static_cast <int> (AF_INET):
									// Выводим информацию об IP-адресе
									cout << "IPv4: " << result << endl;
								break;
								// Если тип протокола подключения IPv6
								case static_cast <int> (AF_INET6):
									// Выводим информацию об IP-адресе
									cout << "IPv6: " << result << endl;
								break;
							}
							// Выводим переход на новую строку
							cout << endl;
						#endif
						// Выводим полученный результат
						return result;
					}{
						// Если префикс переменной окружения установлен
						if(!this->_prefix.empty()){
							// Получаем название доменного имени
							string postfix = domain;
							// Выполняем замену точек в названии доменного имени
							this->_fmk->replace(postfix, ".", "_");
							// Переводим постфикс в верхний регистр
							this->_fmk->transform(postfix, fmk_t::transform_t::UPPER);
							// Определяем тип протокола подключения
							switch(family){
								// Если тип протокола подключения IPv4
								case static_cast <int> (AF_INET): {
									// Получаем значение переменной
									const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV4_%s", this->_prefix.c_str(), postfix.c_str()).c_str());
									// Если IP-адрес из переменной окружения получен
									if(env != nullptr)
										// Выводим полученный результат
										return env;
								} break;
								// Если тип протокола подключения IPv6
								case static_cast <int> (AF_INET6): {
									// Получаем значение переменной
									const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV6_%s", this->_prefix.c_str(), postfix.c_str()).c_str());
									// Если IP-адрес из переменной окружения получен
									if(env != nullptr)
										// Выводим полученный результат
										return env;
								} break;
							}
						}
						// Переводим доменное имя в нижний регистр
						this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
						// Определяем тип протокола подключения
						switch(family){
							// Если тип протокола подключения IPv4
							case static_cast <int> (AF_INET): {
								// Если список DNS-серверов пустой
								if(this->_serversIPv4.empty())
									// Устанавливаем список серверов IPv4
									this->replace(AF_INET);
								// Выполняем получение IP-адреса
								result = this->_workerIPv4->request(domain);
							} break;
							// Если тип протокола подключения IPv6
							case static_cast <int> (AF_INET6): {
								// Если список DNS-серверов пустой
								if(this->_serversIPv6.empty())
									// Устанавливаем список серверов IPv6
									this->replace(AF_INET6);
								// Выполняем получение IP-адреса
								result = this->_workerIPv6->request(domain);
							} break;
						}
						// Если IP-адрес получен
						if(!result.empty())
							// Выводим полученный результат
							return result;
						// Выполняем запрос адреса на локальном резолвере операционной системы
						else {
							// Создаём объект DNS-резолвера
							dns_t dns(this->_fmk, this->_log);
							// Выполняем получение IP адрес хоста доменного имени
							return dns.host(family, host);
						}
					}
				} break;
				// Значит скорее всего, садрес является доменным именем
				default: {
					// Создаём объект DNS-резолвера
					dns_t dns(this->_fmk, this->_log);
					// Выполняем получение IP адрес хоста доменного имени
					return dns.host(family, host);
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * search Метод поиска доменного имени соответствующего IP-адресу
 * @param ip адрес для поиска доменного имени
 * @return   список найденных доменных имён
 */
vector <string> awh::DNS::search(const string & ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		// Определяем тип передаваемого IP-адреса
		switch(static_cast <uint8_t> (this->_net.host(ip))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем поиск доменных имён по IP-адресу
				return this->search(AF_INET, ip);
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем поиск доменных имён по IP-адресу
				return this->search(AF_INET6, ip);
		}
	}
	// Выводим результат
	return vector <string> ();
}
/**
 * search Метод поиска доменного имени соответствующего IP-адресу
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для поиска доменного имени
 * @return       список найденных доменных имён
 */
vector <string> awh::DNS::search(const int family, const string & ip) noexcept {
	// Результат работы функции
	vector <string> result;
	// Если IP-адрес передан
	if(!ip.empty()){
		// Определяем тип протокола подключения
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int> (AF_INET): {
				// Переходим по всему списку IP-адресов
				for(auto it = this->_cacheIPv4.begin(); it != this->_cacheIPv4.end(); ++it){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[1];
					// Выполняем копирование полученных данных в переданный буфер
					inet_pton(family, ip.c_str(), &buffer);
					// Если IP-адрес соответствует переданному адресу
					if(memcmp(it->second.ip, buffer, sizeof(buffer)) == 0)
						// Выполняем добавление доменное имя в список
						result.push_back(it->first);
				}
			}
			// Если тип протокола подключения IPv6
			case static_cast <int> (AF_INET6): {
				// Переходим по всему списку IP-адресов
				for(auto it = this->_cacheIPv6.begin(); it != this->_cacheIPv6.end(); ++it){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[4];
					// Выполняем копирование полученных данных в переданный буфер
					inet_pton(family, ip.c_str(), &buffer);
					// Если IP-адрес соответствует переданному адресу
					if(memcmp(it->second.ip, buffer, sizeof(buffer)) == 0)
						// Выполняем добавление доменное имя в список
						result.push_back(it->first);
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * DNS Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::DNS::DNS(const fmk_t * fmk, const log_t * log) noexcept :
 _ttl(0), _timeout(5), _prefix(AWH_SHORT_NAME),
 _workerIPv4(nullptr), _workerIPv6(nullptr), _fmk(fmk), _log(log) {
	// Выполняем создание воркера для IPv4
	this->_workerIPv4 = unique_ptr <worker_t> (new worker_t(AF_INET, this));
	// Выполняем создание воркера для IPv6
	this->_workerIPv6 = unique_ptr <worker_t> (new worker_t(AF_INET6, this));
}
/**
 * ~DNS Деструктор
 */
awh::DNS::~DNS() noexcept {
	// Выполняем очистку модуля DNS-резолвера
	this->clear();
}
