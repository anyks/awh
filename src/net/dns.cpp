/**
 * @file: dns.cpp
 * @date: 2022-08-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
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
 * shuffle Метод пересортировки серверов DNS
 */
void awh::DNS::Worker::shuffle() noexcept {
	// Выполняем поиск интернет-протокола для получения DNS сервера
	auto it = const_cast <dns_t *> (this->_dns)->_servers.find(this->_family);
	// Если интернет протокол сервера получен
	if(it != this->_dns->_servers.end()){
		// Создаём объект рандомайзера
		random_device rd;
		// Выбираем стаднарт рандомайзера
		mt19937 generator(rd());
		// Выполняем рандомную сортировку списка DNS серверов
		::shuffle(it->second.begin(), it->second.end(), generator);
	}
}
/**
 * response Событие срабатывающееся при получении данных с DNS сервера
 * @param io      объект события чтения
 * @param revents идентификатор события
 */
void awh::DNS::Worker::response(ev::io & io, int revents) noexcept {
	// Выводимый IP адрес
	string ip = "";
	// Выполняем остановку чтения сокета
	this->_io.stop();
	// Буфер пакета данных
	u_char buffer[65536];
	// Выполняем зануление буфера данных
	memset(buffer, 0, sizeof(buffer));
	// Получаем объект DNS сервера
	dns_t * dns = const_cast <dns_t *> (this->_dns);
	// Выполняем чтение ответа от сервера
	if(::recvfrom(io.fd, (char *) buffer, sizeof(buffer), 0, (struct sockaddr *) &this->_addr, &this->_socklen) > 0){
		// Получаем объект заголовка
		head_t * header = reinterpret_cast <head_t *> (&buffer);
		// Получаем размер ответа
		size_t size = sizeof(head_t);
		// Получаем название доменного имени
		const string & qname = this->join(vector <u_char> (buffer + size, buffer + (size + 255)));
		// Увеличиваем размер буфера полученных данных
		size += (qname.size() + 2);
		// Создаём части флагов вопроса пакета ответа
		rrflags_t * rrflags = nullptr;
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

		cout << " ^^^^^^^^^^^^^^^^^^1 " << ntohs(header->ancount) << " ==== " << ntohs(header->arcount) << " ==== " << (u_short) header->rcode << endl;

		// Выполняем перебор всех полученных записей
		for(u_short i = 0; i < count; ++i){
			// Выполняем извлечение названия записи
			name[i] = this->join(this->extract(buffer, size));
			// Увеличиваем размер полученных данных
			size += 2;
			// Создаём части флагов вопроса пакета ответа
			rrflags = reinterpret_cast <rrflags_t *> (&buffer[size]);
			// Увеличиваем размер ответа
			size = ((size + sizeof(rrflags_t)) - 2);

			

			cout << " ^^^^^^^^^^^^^^^^^^2 " << (u_short) ntohs(rrflags->rtype) << " === " << (u_short) ntohs(rrflags->rdlength) << " ||| " << count << endl;

			// Определяем тип записи
			switch(ntohs(rrflags->rtype)){
				// Если запись является интернет-протоколом IPv4
				case 1: {
					cout << " =================== IPV4 " << endl;

					// Создаём буфер данных
					u_char data[254];
					// Выполняем зануление буфера данных
					memset(data, 0, sizeof(data));
					// Выполняем парсинг IPv4 адреса
					for(int j = 0; j < ntohs(rrflags->rdlength); ++j)
						// Выполняем парсинг IP адреса
						data[j] = (u_char) buffer[size + j];
					// Добавляем запись в список записей
					rdata[i] = move((const char *) &data);
					// Устанавливаем тип полученных данных
					type[i] = ntohs(rrflags->rtype);
				} break;
				// Если запись является интернет-протоколом IPv6
				case 28: {
					cout << " ===================1 IPV6 " << ntohs(rrflags->rdlength) << endl;


					// Создаём буфер данных
					u_char data[254];
					// Выполняем зануление буфера данных
					memset(data, 0, sizeof(data));
					// Выполняем парсинг IPv6 адреса
					for(int j = 0; j < ntohs(rrflags->rdlength); ++j)
						// Выполняем парсинг IP адреса
						data[j] = (u_char) buffer[size + j];
					// Добавляем запись в список записей
					rdata[i] = move((const char *) &data);

					cout << " ===================2 IPV6 " << rdata[i].size() << endl;

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
		// Список IP адресов
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

						cout << " -------------------!!!!!!!!! " << rdata[i].size() << endl;

						// Получаем IP адрес принадлежащий доменному имени
						const string ip = inet_ntop(this->_family, rdata[i].c_str(), buffer, sizeof(buffer));
						// Если IP адрес получен
						if(!ip.empty()){
							// Если чёрный список IP адресов получен
							if(!dns->isInBlackList(this->_family, ip)){
								// Добавляем IP адрес в список адресов
								ips.push_back(ip);
								// Записываем данные в кэш
								dns->setToCache(this->_family, this->_domain, ip);
							}
							// Выводим информацию об IP адресе
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
					// Получаем IP адрес принадлежащий доменному имени
					const string ip = inet_ntop(this->_family, rdata[i].c_str(), buffer, sizeof(buffer));
					// Если IP адрес получен
					if(!ip.empty()){
						// Если чёрный список IP адресов получен
						if(!dns->isInBlackList(this->_family, ip)){
							// Добавляем IP адрес в список адресов
							ips.push_back(ip);
							// Записываем данные в кэш
							dns->setToCache(this->_family, this->_domain, ip);
						}
					}
				}
			}
		#endif
		// Если список IP адресов получен
		if(!ips.empty()){
			// Выполняем рандомизацию генератора случайных чисел
			srand(dns->_fmk->nanoTimestamp());
			// Выполняем установку IP адреса
			ip = ips.at(rand() % ips.size());
		// Если чёрный список доменных имён не пустой
		} else if(!dns->emptyBlackList(this->_family))
			// Выполняем очистку чёрного списка
			dns->clearBlackList(this->_family);
	// Если ответ получен не был, но время ещё есть
	} else if(this->_mode) {
		// Выполняем закрытие подключения
		this->close();
		// Выполняем пересортировку серверов DNS
		this->shuffle();
		// Замораживаем поток на период времени в 100ms
		this_thread::sleep_for(10ms);
		// Выполняем запрос снова
		this->request(this->_domain);
		// Выходим из функции
		return;
	}
	// Выполняем закрытие подключения
	this->close();
	// Получаем идентификатор DNS запроса
	const size_t did = this->_did;
	// Получаем тип протокола интернета
	const int family = this->_family;
	// Выполняем блокировку потока
	dns->_mtx.worker.lock();
	// Удаляем домен из списка доменов
	dns->_workers.erase(did);
	// Выполняем разблокировку потока
	dns->_mtx.worker.unlock();
	// Если функция обратного вызова установлена
	if(dns->_fn != nullptr)
		// Выводим полученный IP адрес
		dns->_fn(ip, family, did);
}
/**
 * timeout Функция выполняемая по таймеру для чистки мусора
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::DNS::Worker::timeout(ev::timer & timer, int revents) noexcept {
	// Выполняем остановку работы таймера
	timer.stop();
	// Выполняем остановку чтения сокета
	this->_io.stop();
	// Выполняем закрытие подключения
	this->close();
	// Выполняем остановку работы
	this->_mode = false;
	// Если возникла ошибка, выводим в лог сообщение
	this->_dns->_log->print("%s request failed", log_t::flag_t::CRITICAL, this->_domain.c_str());
	// Выполняем отмену DNS запроса
	const_cast <dns_t *> (this->_dns)->cancel(this->_did);
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
		for(uint16_t i = 0; i < (uint16_t) domain.size(); ++i){
			// Получаем количество символов
			length = (uint16_t) domain[i];
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
		vector <wstring> sections;
		// Выполняем сплит доменного имени
		this->_dns->_fmk->split(domain, ".", sections);
		// Если секции доменного имени получены
		if(!sections.empty()){
			// Переходим по всему списку секций
			for(auto & section : sections){
				// Добавляем в буфер данных размер записи
				result.push_back((u_char) section.size());
				// Получаем строку для добавления в буфер данных
				const string & data = this->_dns->_fmk->convert(section);
				// Добавляем в буфер данные секции
				result.insert(result.end(), data.begin(), data.end());
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
	if(this->_fd > -1){
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
		this->_fd = -1;
	}
}
/**
 * request Метод выполнения запроса
 * @param domain название искомого домена
 * @return       результат выполнения запроса
 */
bool awh::DNS::Worker::request(const string & domain) noexcept {
	// Результат работы функции
	bool result = false;
	// Если доменное имя передано
	if(!domain.empty()){
		// Запоминаем запрашиваемое доменное имя
		this->_domain = domain;
		// Выполняем поиск интернет-протокола для получения DNS сервера
		auto it = const_cast <dns_t *> (this->_dns)->_servers.find(this->_family);
		// Если интернет протокол сервера получен
		if((result = (it != this->_dns->_servers.end()))){
			// Если режим работы не запущен
			if(!this->_mode){
				// Запоминаем, что работа началась
				this->_mode = !this->_mode;
				// Устанавливаем базу событий
				this->_timer.set(this->_base);
				// Устанавливаем функцию обратного вызова
				this->_timer.set <worker_t, &worker_t::timeout> (this);
				// Запускаем работу таймера
				this->_timer.start((double) this->_dns->_timeout);
			}
			// Переходим по всему списку DNS серверов
			for(auto & server : it->second){
				// Очищаем всю структуру клиента
				memset(&this->_addr, 0, sizeof(this->_addr));
				// Определяем тип подключения
				switch(this->_family){
					// Для протокола IPv4
					case AF_INET: {
						// Создаём объект клиента
						struct sockaddr_in client;
						// Очищаем всю структуру для клиента
						memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sin_family = this->_family;
						// Устанавливаем произвольный порт
						client.sin_port = htons(server.port);
						// Устанавливаем адрес для локальго подключения
						client.sin_addr.s_addr = inet_addr(server.host.c_str());
						// Запоминаем размер структуры
						this->_socklen = sizeof(client);
						// Выполняем копирование объекта подключения клиента
						memcpy(&this->_addr, &client, this->_socklen);
					} break;
					// Для протокола IPv6
					case AF_INET6: {
						// Создаём объект клиента
						struct sockaddr_in6 client;
						// Очищаем всю структуру для клиента
						memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sin6_family = this->_family;
						// Устанавливаем произвольный порт для
						client.sin6_port = htons(server.port);

						cout << " ^^^^^^^^^^^^^^^^^^^^^^^ " << server.host << endl;

						// Указываем адрес IPv6 для клиента
						inet_pton(this->_family, server.host.c_str(), &client.sin6_addr);
						// Запоминаем размер структуры
						this->_socklen = sizeof(client);
						// Выполняем копирование объекта подключения клиента
						memcpy(&this->_addr, &client, this->_socklen);
					} break;
				}{
					// Буфер пакета данных
					u_char buffer[65536];
					// Выполняем зануление буфера данных
					memset(buffer, 0, sizeof(buffer));
					// Получаем объект заголовка
					head_t * header = reinterpret_cast <head_t *> (&buffer);
					// Устанавливаем идентификатор заголовка
					header->id = (u_short) htons(getpid());
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
					header->qdcount = htons((u_short) 1);
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
					// Устанавливаем тип флага запроса
					qflags->qtype = htons(28);// htons(0x0001);
					// Устанавливаем класс флага запроса
					qflags->qclass = htons(0x0001);
					// Увеличиваем размер запроса
					size += sizeof(qflags_t);
					// Создаём сокет подключения
					this->_fd = ::socket(this->_family, SOCK_DGRAM, IPPROTO_UDP);
					// Если сокет не создан создан
					if(this->_fd < 0){
						// Выводим в лог сообщение
						this->_dns->_log->print("file descriptor needed for the DNS request could not be allocated", log_t::flag_t::WARNING);
						// Выходим из приложения
						continue;
					// Если сокет создан удачно
					} else {
						// Устанавливаем разрешение на повторное использование сокета
						this->_socket.reuseable(this->_fd);
						// Устанавливаем разрешение на закрытие сокета при неиспользовании
						this->_socket.closeonexec(this->_fd);
						// Устанавливаем размер буфера передачи данных
						this->_socket.bufferSize(this->_fd, sizeof(buffer), sizeof(buffer), 1);
						// Если запрос на сервер DNS успешно отправлен
						if((result = (::sendto(this->_fd, (const char *) buffer, size, 0, (struct sockaddr *) &this->_addr, this->_socklen) > 0))){
							// Устанавливаем сокет для чтения
							this->_io.set(this->_fd, ev::READ);
							// Устанавливаем базу событий
							this->_io.set(this->_base);
							// Устанавливаем событие на чтение данных подключения
							this->_io.set <worker_t, &worker_t::response> (this);
							// Запускаем чтение данных с клиента
							this->_io.start();
							// Выходим из функции
							return result;
						}
						// Выполняем закрытие подключения
						this->close();
					}
				}
			}
			// Если запрос отправлен не был, но время ещё есть
			if(this->_mode){
				// Выполняем пересортировку серверов DNS
				this->shuffle();
				// Замораживаем поток на период времени в 100ms
				this_thread::sleep_for(1s);
				// Выполняем запрос снова
				return this->request(domain);
			}
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
 * clearZombie Метод очистки зомби-запросов
 */
void awh::DNS::clearZombie() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::CLEAR, status_t::RESOLVE}, status_t::ZOMBIE)){
		// Если список воркеров не пустой
		if(!this->_workers.empty()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.worker);
			// Список зомби-воркеров
			vector <size_t> zombie;
			// Получаем текущее значение даты
			const time_t date = this->_fmk->unixTimestamp();
			// Переходим по всем воркерам
			for(auto & worker : this->_workers){
				// Если DNS запрос устарел на 3-и минуты, убиваем его
				if((date - worker.first) >= 180000){
					// Останавливаем события ожидания появления в сокете данных
					worker.second->_io.stop();
					// Останавливаем таймер ожидания выполнения запроса
					worker.second->_timer.stop();
					// Добавляем идентификатор воркеров в список зомби
					zombie.push_back(worker.first);
				}
			}
			// Если список зомби-воркеров существует
			if(!zombie.empty()){
				// Переходим по всему списку зомби-воркеров
				for(auto it = zombie.begin(); it != zombie.end();){
					// Выполняем отмену DNS запроса
					this->cancel(* it);
					// Удаляем зомби-воркер из списка зомби
					it = zombie.erase(it);
				}
			}
		}
	}
}
/**
 * resolving Метод получения IP адреса доменного имени
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param did    идентификатор DNS запроса
 */
void awh::DNS::resolving(const string & ip, const int family, const size_t did) noexcept {
	// Выполняем поиск интернет-протокол
	auto it = this->_servers.find(family);
	// Если интернет-протокол найден
	if(it != this->_servers.end()){
		// Выполняем извлечение локального DNS резолвера
		const string & domain = (this->_dns.count(did) > 0 ? this->_dns.at(did)->_domain : "");
		// Если доменное имя локального DNS резолвера получено
		if(!domain.empty()){
			// Выполняем переход по всем серверам
			for(auto jt = it->second.begin(); jt != it->second.end(); ++jt){
				// Если доменное имя совпадает с сервером имён
				if(domain.compare(jt->host) == 0){
					// Если IP адрес получен
					if(!ip.empty())
						// Заменяем доменное имя на полученный IP адрес
						jt->host = ip;
					// Если IP адрес не получен, удаляем доменное имя из списка и выходим
					else it->second.erase(jt);
					// Выходим из цикла
					break;
				}
			}
		}
	}
	// Удаляем объект DNS запроса
	this->_dns.erase(did);
}
/**
 * clear Метод сброса кэша резолвера
 * @return результат работы функции
 */
bool awh::DNS::clear() noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if((result = hold.access({}, status_t::CLEAR))){
		// Выполняем сброс кэша DNS резолвера
		this->flush();
		// Выполняем отмену выполненных запросов
		this->cancel();
		// Убиваем зомби-процессы если такие имеются
		this->clearZombie();
		// Выполняем сброс списока серверов IPv4
		this->replace(AF_INET);
		// Выполняем сброс списока серверов IPv6
		this->replace(AF_INET6);
		// Очищаем доменное имя локального DNS резолвера
		this->_domain.clear();
	}
	// Выводим результат
	return result;
}
/**
 * flush Метод сброса кэша DNS резолвера
 * @return результат работы функции
 */
bool awh::DNS::flush() noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if((result = hold.access({status_t::CLEAR}, status_t::FLUSH))){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.cache);
		// Выполняем сброс кэша полученных IP адресов
		this->_cache.clear();
	}
	// Выводим результат
	return result;
}
/**
 * base Метод установки базы событий
 * @param base объект базы событий
 */
void awh::DNS::base(struct ev_loop * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Если база передана
	if(base != nullptr)
		// Создаем базу событий
		this->_base = base;
}
/**
 * cancel Метод отмены выполнения запроса
 * @param did идентификатор DNS запроса
 * @return    результат работы функции
 */
bool awh::DNS::cancel(const size_t did) noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if((result = hold.access({status_t::RESOLVE}, status_t::CANCEL, false))){
		// Если список воркеров не пустой
		if((result = !this->_workers.empty())){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.worker);
			// Если идентификатор DNS запроса передан
			if(did > 0){
				// Выполняем поиск воркера
				auto it = this->_workers.find(did);
				// Если воркер найден
				if(it != this->_workers.end()){
					// Если функция обратного вызова установлена
					if(this->_fn != nullptr)
						// Выводим полученный IP адрес
						this->_fn("", it->second->_family, it->second->_did);
					// Удаляем объект воркера
					this->_workers.erase(it);
				}
			// Если нужно остановить работу всех DNS запросов
			} else {
				// Переходим по всем воркерам
				for(auto it = this->_workers.begin(); it != this->_workers.end();){
					// Если функция обратного вызова установлена
					if(this->_fn != nullptr)
						// Выводим полученный IP адрес
						this->_fn("", it->second->_family, it->second->_did);
					// Удаляем объект воркера
					it = this->_workers.erase(it);
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * timeout Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::DNS::timeout(const uint8_t sec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Выполняем установку таймаута ожидания выполнения запроса
	this->_timeout = sec;
}
/**
 * cache Метод получения IP адреса из кэша
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP адресу
 * @return       IP адрес находящийся в кэше
 */
string awh::DNS::cache(const int family, const string & domain) noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя передано и кэш доменных имён проинициализирован
	if(!domain.empty() && !this->_cache.empty()){
		// Выполняем получение семейства протоколов
		auto it = this->_cache.find(family);
		// Если семейство протоколов получено
		if(it != this->_cache.end()){
			// Получаем количество IP адресов в кэше
			const size_t count = it->second.count(domain);
			// Если количество адресов всего 1
			if(count == 1){
				// Выполняем проверку запрашиваемого хоста в кэше
				auto jt = it->second.find(domain);
				// Если хост найден, выводим его
				if(jt != it->second.end()){
					// Если чёрный список IP адресов получен
					if(!this->isInBlackList(family, jt->second))
						// Выполняем получение результата
						result = jt->second;
				}
			// Если доменных имён в кэше больше 1-го
			} else if(count > 1) {
				// Список полученных IP адресов
				vector <string> ips;
				// Получаем диапазон IP адресов в кэше
				auto ret = it->second.equal_range(domain);
				// Переходим по всему списку IP адресов
				for(auto it = ret.first; it != ret.second; ++it){
					// Если чёрный список IP адресов получен
					if(!this->isInBlackList(family, it->second))
						// Добавляем IP адрес в список адресов пригодных для работы
						ips.push_back(it->second);
				}
				// Если список IP адресов получен
				if(!ips.empty()){
					// Выполняем рандомизацию генератора случайных чисел
					srand(this->_fmk->nanoTimestamp());
					// Выполняем получение результата
					result = move(ips.at(rand() % ips.size()));
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * clearCache Метод очистки кэша
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::DNS::clearCache(const int family) noexcept {
	// Если кэш не пустой
	if(!this->_cache.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.cache);
		// Выполняем поиск интернет-протокола
		auto it = this->_cache.find(family);
		// Если интернет-протокол найден
		if(it != this->_cache.end())
			// Выполняем очистку кэша
			it->second.clear();
	}
}
/**
 * setToCache Метод добавления IP адреса в кэш
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP адресу
 * @param ip     адрес для добавления к кэш
 */
void awh::DNS::setToCache(const int family, const string & domain, const string & ip) noexcept {
	// Если доменное имя и IP адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.cache);
		// Выполняем поиск интернет-протокола
		auto it = this->_cache.find(family);
		// Если интернет-протокол найден
		if(it != this->_cache.end())
			// Записываем данные в кэш
			it->second.emplace(domain, ip);
		// Если интернет-протокол не найден
		else {
			// Выполняем формирование интернет-протокола
			auto ret = this->_cache.emplace(family, unordered_multimap <string, string> ());
			// Устанавливаем запрещённый IP адрес
			ret.first->second.emplace(domain, ip);
		}
	}
}
/**
 * clearBlackList Метод очистки чёрного списка
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::DNS::clearBlackList(const int family) noexcept {
	// Если чёрный список не пустой
	if(!this->_blacklist.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.black);
		// Выполняем поиск интернет-протокола
		auto it = this->_blacklist.find(family);
		// Если интернет-протокол найден
		if(it != this->_blacklist.end())
			// Выполняем очистку чёрного списка
			it->second.clear();
	}
}
/**
 * delInBlackList Метод удаления IP адреса из чёрного списока
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::DNS::delInBlackList(const int family, const string & ip) noexcept {
	// Если IP адрес передан
	if(!ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.black);
		// Выполняем поиск интернет-протокола
		auto it = this->_blacklist.find(family);
		// Если интернет-протокол найден
		if(it != this->_blacklist.end()){
			// Выполняем поиск IP адреса в чёрном списке
			auto jt = it->second.find(ip);
			// Если IP адрес найден в чёрном списке, удаляем его
			if(jt != it->second.end()) it->second.erase(jt);
		}
	}
}
/**
 * setToBlackList Метод добавления IP адреса в чёрный список
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для добавления в чёрный список
 */
void awh::DNS::setToBlackList(const int family, const string & ip) noexcept {
	// Если IP адрес передан, добавляем IP адрес в чёрный список
	if(!ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.black);
		// Выполняем поиск интернет-протокола
		auto it = this->_blacklist.find(family);
		// Если интернет-протокол найден
		if(it != this->_blacklist.end())
			// Добавляем IP адрес в список запрещённых адресов
			it->second.emplace(ip);
		// Если интернет-протокол не найден
		else {
			// Формируем интернет-протокол, чёрного списка
			auto ret = this->_blacklist.emplace(family, set <string> ());
			// Устанавливаем запрещённый IP адрес
			ret.first->second.emplace(ip);
		}
	}
}
/**
 * emptyBlackList Метод проверки заполненности чёрного списка
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       результат проверки чёрного списка
 */
bool awh::DNS::emptyBlackList(const int family) const noexcept {
	// Выполняем поиск семейства протоколов чёрного списка
	auto it = this->_blacklist.find(family);
	// Выводим результат проверки
	return (it == this->_blacklist.end());
}
/**
 * isInBlackList Метод проверки наличия IP адреса в чёрном списке
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP адреса в чёрном списке
 */
bool awh::DNS::isInBlackList(const int family, const string & ip) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP адрес для проверки передан
	if(!ip.empty()){
		// Выполняем поиск семейства протоколов чёрного списка
		auto it = this->_blacklist.find(family);
		// Выполняем проверку наличия IP адреса в чёрном списке
		result = ((it != this->_blacklist.end()) && !it->second.empty() && (it->second.count(ip) > 0));
	}
	// Выводим результат
	return result;
}
/**
 * server Метод получения данных сервера имён
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       запрошенный сервер имён
 */
const awh::DNS::serv_t & awh::DNS::server(const int family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Если список серверов пустой
	if(this->_servers.empty())
		// Устанавливаем новый список имён
		this->replace(family);	
	// Выполняем поиск семейство протоколов
	const auto & servers = this->_servers.at(family);
	// Получаем первое значение итератора
	auto it = servers.begin();
	// Выполняем рандомизацию генератора случайных чисел
	srand(this->_fmk->nanoTimestamp());
	// Выполняем выбор нужного сервера в списке, в произвольном виде
	advance(it, (rand() % servers.size()));
	// Выводим результат
	return (* it);
}
/**
 * server Метод добавления сервера DNS
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param server параметры DNS сервера
 */
void awh::DNS::server(const int family, const serv_t & server) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::NSS_SET}, status_t::NS_SET)){
		// Результат работы
		serv_t result;
		// Устанавливаем порт сервера
		result.port = server.port;
		// Определяем тип передаваемого сервера
		switch((uint8_t) this->_nwk->parseHost(server.host)){
			// Если хост является доменом или IPv4 адресом
			case (uint8_t) network_t::type_t::IPV4: result.host = server.host; break;
			// Если хост является IPv6 адресом, переводим ip адрес в полную форму
			case (uint8_t) network_t::type_t::IPV6: result.host = this->_nwk->setLowIp6(server.host); break;
			// Если хост является доменным именем
			case (uint8_t) network_t::type_t::DOMNAME: {
				// Создаём объект DNS
				unique_ptr <dns_t> dns(new dns_t(this->_fmk, this->_log, this->_nwk, this->_base));
				// Устанавливаем доменное имя локального DNS резолвера
				dns->_domain = server.host;
				// Устанавливаем хост сервера
				result.host = dns->_domain;
				// Устанавливаем событие на получение данных с DNS сервера
				dns->on(std::bind(&dns_t::resolving, this, _1, _2, _3));
				// Выполняем резолвинг домена
				const size_t did = dns->resolve(server.host, family);
				// Выполняем добавление DNS резолвера в список внутренних DNS резолверов
				this->_dns.emplace(did, move(dns));
			} break;
		}
		// Если сервер имён получен
		if(!result.host.empty()){
			// Выполняем инициализацию списка серверов
			auto ret = this->_servers.emplace(family, vector <serv_t> ());
			// Устанавливаем сервера имён
			ret.first->second.push_back(move(result));
		// Если имя сервера не получено, выводим в лог сообщение
		} else this->_log->print("name server [%s:%u] does not add", log_t::flag_t::CRITICAL, server.host.c_str(), server.port);
	}
}
/**
 * servers Метод добавления серверов DNS
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers параметры DNS серверов
 */
void awh::DNS::servers(const int family, const vector <serv_t> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::NSS_REP}, status_t::NSS_SET)){
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : servers) this->server(family, server);
	}
}
/**
 * replace Метод замены существующих серверов DNS
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers параметры DNS серверов
 */
void awh::DNS::replace(const int family, const vector <serv_t> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({}, status_t::NSS_REP)){
		// Выполняем поиск интернет-протокола
		auto it = this->_servers.find(family);
		// Если интернет-протокол найден
		if(it != this->_servers.end())
			// Выполняем очистку серверов имён
			it->second.clear();
		// Если нейм сервера переданы, удаляем все настроенные серверы имён и приостанавливаем все ожидающие решения
		if(!servers.empty())
			// Устанавливаем новый список серверов
			this->servers(family, servers);
		// Если список серверов не передан
		else {
			// Результат работы
			vector <serv_t> result;
			// Список серверов
			vector <string> servers;
			// Определяем тип подключения
			switch(family){
				// Для протокола IPv4
				case AF_INET:
					// Устанавливаем список серверов
					servers = IPV4_RESOLVER;
				break;
				// Для протокола IPv6
				case AF_INET6:
					// Устанавливаем список серверов
					servers = IPV6_RESOLVER;
				break;
			}
			// Изменяем размер данных
			result.resize(servers.size());
			// Переходим по всему списку серверов
			for(size_t i = 0; i < servers.size(); i++){
				// Устанавливаем хост сервера
				result.at(i).host = servers.at(i);
			}
			// Устанавливаем новый список серверов
			this->servers(family, result);
		}
	}
}
/**
 * on Метод установки функции обратного вызова для получения данных
 * @param callback функция обратного вызова срабатывающая при получении данных
 */
void awh::DNS::on(function <void (const string &, const int, const size_t)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Устанавливаем функцию обратного вызова
	this->_fn = callback;
}
/**
 * resolve Метод ресолвинга домена
 * @param host   хост сервера
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       идентификатор DNS запроса
 */
size_t awh::DNS::resolve(const string & host, const int family) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->_status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({}, status_t::RESOLVE)){
		// Убиваем зомби-процессы если такие имеются
		this->clearZombie();
		// Если домен передан
		if(!host.empty()){
			// Определяем тип передаваемого сервера
			switch((uint8_t) this->_nwk->parseHost(host)){
				// Если домен является IPv4 адресом
				case (uint8_t) network_t::type_t::IPV4:
				// Если домен является IPv6 адресом
				case (uint8_t) network_t::type_t::IPV6: {
					// Если функция обратного вызова установлена
					if(this->_fn != nullptr)
						// Выводим полученный IP адрес
						this->_fn(host, family, result);
				} break;
				// Если домен является аппаратным адресом сетевого интерфейса
				case (uint8_t) network_t::type_t::MAC:
				// Если домен является адресом/Маски сети
				case (uint8_t) network_t::type_t::NETWORK:
				// Если домен является адресом в файловой системе
				case (uint8_t) network_t::type_t::ADDRESS:
				// Если домен является HTTP методом
				case (uint8_t) network_t::type_t::HTTPMETHOD:
				// Если домен является HTTP адресом
				case (uint8_t) network_t::type_t::HTTPADDRESS: {
					// Если функция обратного вызова установлена
					if(this->_fn != nullptr)
						// Выводим полученный IP адрес
						this->_fn("", family, result);
				} break;
				// Если домен является доменным именем
				case (uint8_t) network_t::type_t::DOMNAME: {
					// Выполняем поиск IP адреса в кэше DNS
					const string & ip = this->cache(family, host);
					// Если IP адрес получен
					if(!ip.empty()){
						// Если функция обратного вызова установлена
						if(this->_fn != nullptr)
							// Выводим полученный IP адрес
							this->_fn(ip, family, result);
						// Выходим из функции
						return result;
					}
					// Если база событий установлена
					if(this->_base != nullptr){
						// Выполняем блокировку потока
						this->_mtx.worker.lock();
						// Получаем идентификатор воркера
						result = this->_fmk->unixTimestamp();
						// Добавляем воркер резолвинга в список воркеров
						auto ret = this->_workers.emplace(result, unique_ptr <worker_t> (new worker_t(result, family, this->_base, this)));
						// Выполняем разблокировку потока
						this->_mtx.worker.unlock();
						// Если запрос на сервер не выполнен
						if(!ret.first->second->request(host)){
							// Выполняем блокировку потока
							this->_mtx.worker.lock();
							// Удаляем объект воркера
							this->_workers.erase(result);
							// Выполняем разблокировку потока
							this->_mtx.worker.unlock();
							// Выводим в лог сообщение
							this->_log->print("request for %s returned immediately", log_t::flag_t::CRITICAL, host.c_str());
							// Выполняем обнуление результата
							result = 0;
							// Если функция обратного вызова установлена
							if(this->_fn != nullptr)
								// Выводим полученный IP адрес
								this->_fn("", family, result);
						}
					}
				} break;
				// Значит скорее всего, садрес является доменным именем
				default: {
					// Если доменное имя является локальным
					if(this->_fmk->isLatian(this->_fmk->convert(host))){
						// Определяем тип протокола подключения
						switch(family){
							// Если тип протокола подключения IPv4
							case AF_INET: {
								/**
								 * Методы только для OS Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Выполняем резолвинг доменного имени
									struct hostent * domain = gethostbyname(host.c_str());
								/**
								 * Если операционной системой является Nix-подобная
								 */
								#else
									// Выполняем резолвинг доменного имени
									struct hostent * domain = gethostbyname2(host.c_str(), AF_INET);
								#endif
								// Создаём объект сервера
								struct sockaddr_in server;
								// Очищаем всю структуру для сервера
								memset(&server, 0, sizeof(server));
								// Создаем буфер для получения ip адреса
								char buffer[INET_ADDRSTRLEN];
								// Заполняем структуру нулями
								memset(buffer, 0, sizeof(buffer));
								// Устанавливаем протокол интернета
								server.sin_family = AF_INET;
								// Выполняем копирование данных типа подключения
								memcpy(&server.sin_addr.s_addr, domain->h_addr, domain->h_length);
								// Копируем полученные данные
								inet_ntop(AF_INET, &server.sin_addr, buffer, sizeof(buffer));
								// Если функция обратного вызова установлена
								if(this->_fn != nullptr)
									// Выводим полученный IP адрес
									this->_fn(buffer, family, result);
							} break;
							// Если тип протокола подключения IPv6
							case AF_INET6: {
								/**
								 * Методы только для OS Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Выполняем резолвинг доменного имени
									struct hostent * domain = gethostbyname(host.c_str());
								/**
								 * Если операционной системой является Nix-подобная
								 */
								#else
									// Выполняем резолвинг доменного имени
									struct hostent * domain = gethostbyname2(host.c_str(), AF_INET6);
								#endif
								// Создаём объект сервера
								struct sockaddr_in6 server;
								// Очищаем всю структуру для сервера
								memset(&server, 0, sizeof(server));
								// Создаем буфер для получения ip адреса
								char buffer[INET6_ADDRSTRLEN];
								// Заполняем структуру нулями
								memset(buffer, 0, sizeof(buffer));
								// Устанавливаем протокол интернета
								server.sin6_family = AF_INET6;
								// Выполняем копирование данных типа подключения
								memcpy(&server.sin6_addr.s6_addr, domain->h_addr, domain->h_length);
								// Копируем полученные данные
								inet_ntop(AF_INET6, &server.sin6_addr, buffer, sizeof(buffer));
								// Если функция обратного вызова установлена
								if(this->_fn != nullptr)
									// Выводим полученный IP адрес
									this->_fn(buffer, family, result);
							} break;
						}
					// Если в качестве хоста, прислали какую-то чушь и функция обратного вызова установлена
					} else if(this->_fn != nullptr)
						// Выводим полученный IP адрес
						this->_fn("", family, result);
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
 * @param nwk объект методов для работы с сетью
 */
awh::DNS::DNS(const fmk_t * fmk, const log_t * log, const network_t * nwk) noexcept : _domain(""), _timeout(30), _fn(nullptr), _fmk(fmk), _log(log), _nwk(nwk), _base(nullptr) {
	// Устанавливаем список серверов IPv4
	this->replace(AF_INET);
	// Устанавливаем список серверов IPv6
	this->replace(AF_INET6);
}
/**
 * DNS Конструктор
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 * @param nwk  объект методов для работы с сетью
 * @param base база событий
 */
awh::DNS::DNS(const fmk_t * fmk, const log_t * log, const network_t * nwk, struct ev_loop * base) noexcept : _domain(""), _timeout(30), _fn(nullptr), _fmk(fmk), _log(log), _nwk(nwk), _base(base) {
	// Устанавливаем список серверов IPv4
	this->replace(AF_INET);
	// Устанавливаем список серверов IPv6
	this->replace(AF_INET6);
}
/**
 * ~DNS Деструктор
 */
awh::DNS::~DNS() noexcept {
	// Выполняем очистку модуля DNS резолвера
	this->clear();
}
