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
 * access Метод проверки на разрешение выполнения операции
 * @param comp  статус сравнения
 * @param hold  статус установки
 * @param equal флаг эквивалентности
 * @return      результат проверки
 */
bool awh::DNS::Holder::access(const set <status_t> & comp, const status_t hold, const bool equal) noexcept {
	// Определяем есть ли фиксированные статусы
	this->_flag = this->_status->empty();
	// Если результат не получен
	if(!this->_flag && !comp.empty())
		// Получаем результат сравнения
		this->_flag = (equal ? (comp.count(this->_status->top()) > 0) : (comp.count(this->_status->top()) < 1));
	// Если результат получен, выполняем холд
	if(this->_flag) this->_status->push(hold);
	// Выводим результат
	return this->_flag;
}
/**
 * ~Holder Деструктор
 */
awh::DNS::Holder::~Holder() noexcept {
	// Если холдирование выполнено
	if(this->_flag) this->_status->pop();
}
/**
 * timeout Метод таймаута ожидания получения данных
 */
void awh::DNS::Worker::timeout() noexcept {
	// Выполняем отмену ожидания полученя данных
	this->cancel();
	// Если возникла ошибка, выводим в лог сообщение
	this->_self->_log->print("%s request failed", log_t::flag_t::WARNING, this->_domain.c_str());
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
	// Выполняем остановку работы резолвера
	this->_mode = !this->_mode;
	// Выполняем закрытие подключения
	this->close();
}
/**
 * Метод отправки запроса на удалённый сервер DNS
 * @param server адрес DNS-сервера
 * @return       полученный IP-адрес
 */
string awh::DNS::Worker::send(const string & server) noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя установлено
	if(!this->_domain.empty()){
		// Буфер пакета данных
		u_char buffer[65536];
		// Выполняем зануление буфера данных
		memset(buffer, 0, sizeof(buffer));
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
		memcpy(&buffer[size], domain.data(), domain.size());
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
			this->_self->_log->print("file descriptor needed for the DNS request could not be allocated", log_t::flag_t::WARNING);
			// Выходим из приложения
			return result;
		// Если сокет создан удачно и работа резолвера не остановлена
		} else if(this->_mode) {
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->_fd);
			// Устанавливаем разрешение на закрытие сокета при неиспользовании
			this->_socket.closeonexec(this->_fd);
			// Устанавливаем размер буфера передачи данных
			this->_socket.bufferSize(this->_fd, sizeof(buffer), sizeof(buffer), 1);
			// Если запрос на сервер DNS успешно отправлен
			if(::sendto(this->_fd, (const char *) buffer, size, 0, (struct sockaddr *) &this->_addr, this->_socklen) > 0){
				// Буфер пакета данных
				u_char buffer[65536];
				// Получаем объект DNS-сервера
				dns_t * self = const_cast <dns_t *> (this->_self);
				// Выполняем зануление буфера данных
				memset(buffer, 0, sizeof(buffer));
				// Выполняем чтение ответа сервера
				const int64_t bytes = ::recvfrom(this->_fd, (char *) buffer, sizeof(buffer), 0, (struct sockaddr *) &this->_addr, &this->_socklen);
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
								self->_log->print("EPIPE", log_t::flag_t::WARNING);
							break;
							// Если произведён сброс подключения
							case ECONNRESET:
								// Выводим в лог сообщение
								self->_log->print("ECONNRESET", log_t::flag_t::WARNING);
							break;
							// Для остальных ошибок
							default:
								// Выводим в лог сообщение
								self->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
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
											memset(buffer, 0, sizeof(buffer));
											// Получаем IP-адрес принадлежащий доменному имени
											const string ip = inet_ntop(this->_family, rdata[i].c_str(), buffer, sizeof(buffer));
											// Если IP-адрес получен
											if(!ip.empty()){
												// Если чёрный список IP-адресов получен
												if(!self->isInBlackList(this->_family, this->_domain, ip)){
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
										memset(buffer, 0, sizeof(buffer));
										// Получаем IP-адрес принадлежащий доменному имени
										const string ip = inet_ntop(this->_family, rdata[i].c_str(), buffer, sizeof(buffer));
										// Если IP-адрес получен
										if(!ip.empty()){
											// Если чёрный список IP-адресов получен
											if(!self->isInBlackList(this->_family, this->_domain, ip)){
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
								// Выполняем остановку работы таймера ожидания полученя данных
								this->_timer.stop(this->_tid);
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
							self->_log->print("DNS query format error [%s] for name server [%s]", log_t::flag_t::WARNING, this->_domain.c_str(), server.c_str());
						break;
						// Если проблемы возникли на DNS-сервера
						case 2:
							// Выводим в лог сообщение
							self->_log->print("DNS server failure [%s] for name server [%s]", log_t::flag_t::WARNING, this->_domain.c_str(), server.c_str());
						break;
						// Если доменное имя указанное в запросе не существует
						case 3:
							// Выводим в лог сообщение
							self->_log->print("the domain name referenced in the query for name server [%s] does not exist [%s]", log_t::flag_t::WARNING, server.c_str(), this->_domain.c_str());
						break;
						// Если DNS-сервер не поддерживает подобный тип запросов
						case 4:
							// Выводим в лог сообщение
							self->_log->print("DNS server is not implemented [%s] for name server [%s]", log_t::flag_t::WARNING, this->_domain.c_str(), server.c_str());
						break;
						// Если DNS-сервер отказался выполнять наш запрос (например по политическим причинам)
						case 5:
							// Выводим в лог сообщение
							self->_log->print("DNS request is refused [%s] for name server [%s]", log_t::flag_t::WARNING, this->_domain.c_str(), server.c_str());
						break;
					}
				}
				// Если мы получили результат
				if(!result.empty())
					// Выводим результат
					return result;
			// Выводим сообщение, что у нас проблема с подключением к сети
			} else this->_self->_log->print("no network connection", log_t::flag_t::WARNING);
			// Выполняем закрытие подключения
			this->close();
		}
	}
	// Выводим результат
	return result;
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
					// Выполняем установку таймера
					this->_tid = this->_timer.setTimeout(std::bind(&worker_t::timeout, this), this->_self->_timeout * 1000);
					// Переходим по всему списку DNS-серверов
					for(auto & server : this->_self->_serversIPv4){
						// Создаём объект клиента
						struct sockaddr_in client;
						// Очищаем всю структуру для клиента
						memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sin_family = this->_family;
						// Устанавливаем произвольный порт
						client.sin_port = htons(server.port);
						// Устанавливаем адрес для подключения
						memcpy(&client.sin_addr.s_addr, server.ip, sizeof(server.ip));
						// Запоминаем размер структуры
						this->_socklen = sizeof(client);
						// Очищаем всю структуру клиента
						memset(&this->_addr, 0, sizeof(this->_addr));
						// Выполняем копирование объекта подключения клиента
						memcpy(&this->_addr, &client, this->_socklen);
						{
							// Временный буфер данных для преобразования IP-адреса
							char buffer[INET_ADDRSTRLEN];
							// Выполняем запрос на удалённый DNS-сервер
							result = this->send(inet_ntop(this->_family, &server.ip, buffer, sizeof(buffer)));
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
					// Выполняем установку таймера
					this->_tid = this->_timer.setTimeout(std::bind(&worker_t::timeout, this), this->_self->_timeout * 1000);
					// Переходим по всему списку DNS-серверов
					for(auto & server : this->_self->_serversIPv6){
						// Создаём объект клиента
						struct sockaddr_in6 client;
						// Очищаем всю структуру для клиента
						memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sin6_family = this->_family;
						// Устанавливаем произвольный порт для
						client.sin6_port = htons(server.port);
						// Устанавливаем адрес для подключения
						memcpy(&client.sin6_addr, server.ip, sizeof(server.ip));
						// Запоминаем размер структуры
						this->_socklen = sizeof(client);
						// Очищаем всю структуру клиента
						memset(&this->_addr, 0, sizeof(this->_addr));
						// Выполняем копирование объекта подключения клиента
						memcpy(&this->_addr, &client, this->_socklen);
						{
							// Временный буфер данных для преобразования IP-адреса
							char buffer[INET6_ADDRSTRLEN];
							// Выполняем запрос на удалённый DNS-сервер
							result = this->send(inet_ntop(this->_family, &server.ip, buffer, sizeof(buffer)));
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
 * ~Worker Деструктор
 */
awh::DNS::Worker::~Worker() noexcept {
	// Выполняем остановку таймера
	this->_timer.stop(this->_tid);
	// Выполняем закрытие файлового дерскриптора (сокета)
	this->close();
}
/**
 * Если используется модуль IDN
 */
#if defined(AWH_IDN)
	/**
	 * idnEncode Метод кодирования интернационального доменного имени
	 * @param domain доменное имя для кодирования
	 * @return       результат работы кодирования
	 */
	string awh::DNS::idnEncode(const string & domain) const noexcept {
		// Результат работы функции
		string result = "";
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
					this->_log->print("idn encode failed (%d)", log_t::flag_t::CRITICAL, GetLastError());
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
					this->_log->print("idn encode failed (%d): %s", log_t::flag_t::CRITICAL, rc, idn2_strerror(rc));
				// Получаем результат кодирования
				else result = buffer;
				// Очищаем буфер данных
				delete [] buffer;
			#endif
		}
		// Выводим результат
		return result;
	}
	/**
	 * idnDecode Метод декодирования интернационального доменного имени
	 * @param domain доменное имя для декодирования
	 * @return       результат работы декодирования
	 */
	string awh::DNS::idnDecode(const string & domain) const noexcept {
		// Результат работы функции
		string result = "";
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
					this->_log->print("idn decode failed (%d)", log_t::flag_t::CRITICAL, GetLastError());
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
					this->_log->print("idn decode failed (%d): %s", log_t::flag_t::CRITICAL, rc, idn2_strerror(rc));
				// Получаем результат декодирования
				else result = buffer;
				// Очищаем буфер данных
				delete [] buffer;
			#endif
		}
		// Выводим результат
		return result;
	}
#endif
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
	hold_t hold(&this->_status);
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
	hold_t hold(&this->_status);
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
 * ttl Метод установки времени жизни DNS-кэша
 * @param ttl время жизни DNS-кэша в миллисекундах
 */
void awh::DNS::ttl(const time_t ttl) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем установку DNS-кэша
	this->_ttl = ttl;
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
	// Создаём объект рандомайзера
	random_device random;
	// Выбираем стаднарт рандомайзера
	mt19937 generator(random());
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
			// Выполняем инициализацию генератора
			random_device dev;
			// Подключаем устройство генератора
			mt19937 rng(dev());
			// Выполняем генерирование случайного числа
			uniform_int_distribution <mt19937::result_type> dist6(0, ips.size() - 1);
			// Выполняем получение результата
			result = std::forward <string> (ips.at(dist6(rng)));
		}
	}
	// Выводим результат
	return result;
}
/**
 * clearCache Метод очистки кэша
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 */
void awh::DNS::clearCache(const int family, const string & domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
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
 * @param family    тип интернет-протокола AF_INET, AF_INET6
 * @param domain    доменное имя соответствующее IP-адресу
 * @param ip        адрес для добавления к кэш
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::setToCache(const int family, const string & domain, const string & ip, const bool localhost) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Результат проверки наличия IP-адреса в кэше
		bool result = false;
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
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 */
void awh::DNS::clearBlackList(const int family, const string & domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
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
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::DNS::delInBlackList(const int family, const string & domain, const string & ip) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
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
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::DNS::setToBlackList(const int family, const string & domain, const string & ip) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
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
	// Выполняем инициализацию генератора
	random_device dev;
	// Подключаем устройство генератора
	mt19937 rng(dev());
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
			advance(it, dist6(rng));
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
			advance(it, dist6(rng));
			// Выполняем получение данных IP-адреса
			result = inet_ntop(family, &it->ip, buffer, sizeof(buffer));
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * server Метод добавления сервера DNS
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param server параметры DNS-сервера
 */
void awh::DNS::server(const int family, const string & server) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::NSS_SET}, status_t::NS_SET)){
		// Если адрес сервера передан
		if(!server.empty()){
			// Порт переданного сервера
			u_int port = 53;
			// Хост переданного сервера
			string host = "";
			// Определяем тип протокола подключения
			switch(family){
				// Если тип протокола подключения IPv4
				case static_cast <int> (AF_INET): {
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
				// Если тип протокола подключения IPv6
				case static_cast <int> (AF_INET6): {
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
			}
			// Определяем тип передаваемого сервера
			switch(static_cast <uint8_t> (this->_net.host(host))){
				// Если домен является аппаратным адресом сетевого интерфейса
				case static_cast <uint8_t> (net_t::type_t::MAC):
				// Если домен является адресом/Маски сети
				case static_cast <uint8_t> (net_t::type_t::NETW):
				// Если домен является адресом в файловой системе
				case static_cast <uint8_t> (net_t::type_t::ADDR):
				// Если домен является HTTP адресом
				case static_cast <uint8_t> (net_t::type_t::HTTP):
					// Выполняем удаление хоста
					host.clear();
				break;
				// Если хост является доменом или IPv4 адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Если хост является IPv6 адресом, переводим ip адрес в полную форму
				case static_cast <uint8_t> (net_t::type_t::IPV6): break;
				// Если хост является доменным именем
				case static_cast <uint8_t> (net_t::type_t::DOMN): {
					// Создаём объект DNS-резолвера
					dns_t dns(this->_fmk, this->_log);
					// Выполняем получение IP адрес хоста доменного имени
					string ip = dns.host(family, host);
					// Если IP-адрес мы не получили, выполняем запрос на сервер
					if(ip.empty())
						// Выполняем запрос IP-адреса с удалённого сервера
						host = dns.resolve(family, host);
					// Выполняем установку IP-адреса
					else host = std::move(ip);
				} break;
				// Значит скорее всего, садрес является доменным именем
				default: {
					// Создаём объект DNS-резолвера
					dns_t dns(this->_fmk, this->_log);
					// Выполняем получение IP адрес хоста доменного имени
					host = dns.host(family, host);
				}
			}
			// Если DNS-сервер имён получен
			if(!host.empty()){
				// Определяем тип протокола подключения
				switch(family){
					// Если тип протокола подключения IPv4
					case static_cast <int> (AF_INET): {
						// Создаём объект сервера имени DNS
						server_t <1> server;
						// Запоминаем полученный порт
						server.port = port;
						// Запоминаем полученный сервер
						inet_pton(family, host.c_str(), &server.ip);
						// Выполняем добавление полученный сервер в список DNS-серверов имён
						this->_serversIPv4.push_back(std::move(server));
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD NAME SERVER ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << host << ":" << port << endl;
						#endif
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <int> (AF_INET6): {
						// Создаём объект сервера имени DNS
						server_t <4> server;
						// Запоминаем полученный порт
						server.port = port;
						// Запоминаем полученный сервер
						inet_pton(family, host.c_str(), &server.ip);
						// Выполняем добавление полученный сервер в список DNS-серверов имён
						this->_serversIPv6.push_back(std::move(server));
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD NAME SERVER ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << "[" << host << "]:" << port << endl;
						#endif
					} break;
				}
			// Если имя сервера не получено, выводим в лог сообщение
			} else this->_log->print("name server [%s:%u] does not add", log_t::flag_t::WARNING, server.c_str(), port);
		}
	}
}
/**
 * servers Метод добавления серверов DNS
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers параметры DNS-серверов
 */
void awh::DNS::servers(const int family, const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::NSS_REP}, status_t::NSS_SET)){
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : servers)
			// Выполняем добавление нового сервера
			this->server(family, server);
	}
}
/**
 * replace Метод замены существующих серверов DNS
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers параметры DNS-серверов
 */
void awh::DNS::replace(const int family, const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
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
 * readHosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::DNS::readHosts(const string & filename) noexcept {
	// Если адрес файла получен
	if(!filename.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Создаём объект для работы с файловой системой
		fs_t fs(this->_fmk, this->_log);
		// Выполняем установку адреса файла hosts
		const string & hostsFilename = fs.realPath(filename);
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
						// Если символ является пробелом
						if(isspace(c) || (c == 32) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') || (c == '\v')){
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
						} else this->_log->print("the entry provided [%s] is not an IP address", log_t::flag_t::WARNING, hosts.front().c_str());
					// Выводим сообщение, что текст передан неверный
					} else this->_log->print("hosts in entry %s not found", log_t::flag_t::WARNING, entry.c_str());
				}
			});
		}
	// Если имя сервера не получено, выводим в лог сообщение
	} else this->_log->print("hosts file address is not passed", log_t::flag_t::WARNING);
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
	hold_t hold(&this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({}, status_t::RESOLVE)){
		// Если домен передан
		if(!name.empty()){
			// Если доменное имя является локальным
			if(this->_fmk->is(name, fmk_t::check_t::LATIAN)){
				// Определяем тип протокола подключения
				switch(family){
					// Если тип протокола подключения IPv4
					case static_cast <int> (AF_INET): {
						/**
						 * Методы только для OS Windows
						 */
						#if defined(_WIN32) || defined(_WIN64)
							// Выполняем резолвинг доменного имени
							struct hostent * domain = ::gethostbyname(name.c_str());
						/**
						 * Если операционной системой является Nix-подобная
						 */
						#else
							// Выполняем резолвинг доменного имени
							struct hostent * domain = ::gethostbyname2(name.c_str(), AF_INET);
						#endif
						// Создаём объект сервера
						struct sockaddr_in server;
						// Очищаем всю структуру для сервера
						memset(&server, 0, sizeof(server));
						// Создаем буфер для получения ip адреса
						result.resize(INET_ADDRSTRLEN, 0);
						// Устанавливаем протокол интернета
						server.sin_family = AF_INET;
						// Выполняем копирование данных типа подключения
						memcpy(&server.sin_addr.s_addr, domain->h_addr, domain->h_length);
						// Копируем полученные данные
						inet_ntop(AF_INET, &server.sin_addr, result.data(), result.size());
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <int> (AF_INET6): {
						/**
						 * Методы только для OS Windows
						 */
						#if defined(_WIN32) || defined(_WIN64)
							// Выполняем резолвинг доменного имени
							struct hostent * domain = ::gethostbyname(name.c_str());
						/**
						 * Если операционной системой является Nix-подобная
						 */
						#else
							// Выполняем резолвинг доменного имени
							struct hostent * domain = ::gethostbyname2(name.c_str(), AF_INET6);
						#endif
						// Создаём объект сервера
						struct sockaddr_in6 server;
						// Очищаем всю структуру для сервера
						memset(&server, 0, sizeof(server));
						// Создаем буфер для получения ip адреса
						result.resize(INET6_ADDRSTRLEN, 0);
						// Устанавливаем протокол интернета
						server.sin6_family = AF_INET6;
						// Выполняем копирование данных типа подключения
						memcpy(&server.sin6_addr.s6_addr, domain->h_addr, domain->h_length);
						// Копируем полученные данные
						inet_ntop(AF_INET6, &server.sin6_addr, result.data(), result.size());
					} break;
				}
			}
		}
	}
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
	hold_t hold(&this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({}, status_t::RESOLVE)){
		// Если домен передан
		if(!host.empty()){
			/**
			 * Если используется модуль IDN
			 */
			#if AWH_IDN
				// Получаем доменное имя в интернациональном виде
				const string & domain = this->idnEncode(host);
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
						#endif
						// Выводим полученный результат
						return result;
					}{
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
 _net(fmk, log), _ttl(0), _timeout(30),
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
