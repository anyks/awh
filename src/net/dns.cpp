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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <net/dns.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Метод получения бинарных данных
 *
 * @param type тип бинарного буфера данных
 * @return     бинарные данные буфера
 */
uint8_t * awh::DNS::Buffer::get(const type_t type) noexcept {
	/**
	 * Определяем тип бинарного буфера
	 */
	switch(static_cast <uint8_t> (type)){
		// Если нужно выполнить получение буфера для обмена данными с DNS-сервером
		case static_cast <uint8_t> (type_t::DATA):
			// Выводим данные бинарного буфера
			return this->_data;
		// Если нужно выполнить получение буфера для извлечения IP-адреса
		case static_cast <uint8_t> (type_t::ADDR):
			// Выводим данные бинарного буфера
			return this->_addr;
	}
	// Выводим пустой значение
	return nullptr;
}
/**
 * @brief Метод очистки бинарного буфера данных
 *
 * @param type   тип бинарного буфера данных
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::DNS::Buffer::clear(const type_t type, const int32_t family) noexcept {
	/**
	 * Определяем тип бинарного буфера
	 */
	switch(static_cast <uint8_t> (type)){
		// Если нужно выполнить очистку буфера для обмена данными с DNS-сервером
		case static_cast <uint8_t> (type_t::DATA):
			// Заполняем нулями бинарный буфер
			::memset(this->_data, 0, this->size(type, family));
		break;
		// Если нужно выполнить очистку буфера для извлечения IP-адреса
		case static_cast <uint8_t> (type_t::ADDR):
			// Заполняем нулями бинарный буфер
			::memset(this->_addr, 0, this->size(type, family));
		break;
	}
}
/**
 * @brief Метод получения размера буфера
 *
 * @param type   тип бинарного буфера данных
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       размер бинарного буфера данных
 */
size_t awh::DNS::Buffer::size(const type_t type, const int32_t family) const noexcept {
	/**
	 * Определяем тип бинарного буфера
	 */
	switch(static_cast <uint8_t> (type)){
		// Если нужно выполнить получение размера для обмена данными с DNS-сервером
		case static_cast <uint8_t> (type_t::DATA):
			// Выводим размер обменного буфера
			return AWH_DATA_SIZE;
		// Если нужно выполнить получение размера буфера для извлечения IP-адреса
		case static_cast <uint8_t> (type_t::ADDR): {
			/**
			 * Определяем тип подключения
			 */
			switch(family){
				// Для протокола IPv4
				case AF_INET:
					// Выводим размер для IP-адреса
					return INET_ADDRSTRLEN;
				// Для протокола IPv6
				case AF_INET6:
					// Выводим размер для IP-адреса
					return INET6_ADDRSTRLEN;
			}
		} break;
	}
	// Выводим пустой размер
	return 0;
}
/**
 * @brief Метод извлечения хоста компьютера
 *
 * @return хост компьютера с которого производится запрос
 */
string awh::DNS::Worker::host() const noexcept {
	// Результат работы функции
	string result = "";
	// Если список сетей установлен
	if(!this->_network.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если количество элементов больше 1
			if(this->_network.size() > 1){
				// Подключаем устройство генератора
				mt19937 generator(const_cast <dns_t *> (this->_self)->_randev());
				// Выполняем генерирование случайного числа
				uniform_int_distribution <mt19937::result_type> dist6(0, this->_network.size() - 1);
				// Получаем ip адрес
				result = this->_network.at(dist6(generator));
			// Выводим только первый элемент
			} else result = this->_network.front();
		/**
		 * Если возникает ошибка
		 */
		} catch(const runtime_error & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_self->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_self->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
			// Выводим только первый элемент
			result = this->_network.front();
		}
	}
	// Если IP-адрес не установлен
	if(result.empty()){
		/**
		 * Определяем тип подключения
		 */
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
 * @brief Максимальное время жизни записи в кэше DNS (в секундах)
 *
 * Ответ DNS-сервера может содержать произвольно большое значение TTL, подделанная запись
 * с таким значением осталась бы в кэше навсегда, поэтому время жизни ограничиваем одними сутками
 */
static constexpr uint32_t DNS_MAX_TTL = 86400;
/**
 * @brief Максимальное количество переходов по указателям сжатия в доменном имени
 *
 */
static constexpr uint8_t DNS_MAX_HOPS = 128;
/**
 * @brief Функция формирования ключа сравнения доменного имени
 *
 * Ключ состоит из частей доменного имени в нижнем регистре, каждой из которых предшествует её длина,
 * так две разные последовательности частей доменного имени никогда не дают одинаковый ключ
 *
 * @param labels составные части доменного имени
 * @return       ключ сравнения доменного имени
 */
static string dnsKey(const vector <string> & labels) noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Переходим по всем частям доменного имени
		for(auto & label : labels){
			// Добавляем длину части доменного имени
			result.append(1, static_cast <char> (label.size()));
			// Переходим по всем символам части доменного имени
			for(auto & c : label)
				// Добавляем символ в нижнем регистре
				result.append(1, ((c >= 'A') && (c <= 'Z')) ? static_cast <char> (c + 32) : c);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Выполняем очистку результата
		result.clear();
	}
	// Выводим результат
	return result;
}
/**
 * @brief Функция проверки адреса отправителя ответа DNS-сервера
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param from   адрес с которого получен ответ
 * @param to     адрес DNS-сервера на который отправлен запрос
 * @return       результат проверки
 */
static bool dnsPeer(const int32_t family, const struct sockaddr_storage & from, const struct sockaddr_storage & to) noexcept {
	/**
	 * Определяем тип подключения
	 */
	switch(family){
		// Для протокола IPv4
		case AF_INET: {
			// Получаем адрес отправителя
			const struct sockaddr_in * a = reinterpret_cast <const struct sockaddr_in *> (&from);
			// Получаем адрес DNS-сервера
			const struct sockaddr_in * b = reinterpret_cast <const struct sockaddr_in *> (&to);
			// Выводим результат сравнения адреса и порта
			return (
				(a->sin_family == AF_INET) &&
				(a->sin_port == b->sin_port) &&
				(a->sin_addr.s_addr == b->sin_addr.s_addr)
			);
		}
		// Для протокола IPv6
		case AF_INET6: {
			// Получаем адрес отправителя
			const struct sockaddr_in6 * a = reinterpret_cast <const struct sockaddr_in6 *> (&from);
			// Получаем адрес DNS-сервера
			const struct sockaddr_in6 * b = reinterpret_cast <const struct sockaddr_in6 *> (&to);
			// Выводим результат сравнения адреса и порта
			return (
				(a->sin6_family == AF_INET6) &&
				(a->sin6_port == b->sin6_port) &&
				(::memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(a->sin6_addr)) == 0)
			);
		}
	}
	// Выводим результат
	return false;
}
/**
 * @brief Метод разбивки доменного имени
 *
 * @param domain доменное имя для разбивки
 * @return       разбитое доменное имя
 */
vector <uint8_t> awh::DNS::Worker::split(const string & domain) const noexcept {
	// Результат работы функции
	vector <uint8_t> result;
	// Если доменное имя передано
	if(!domain.empty()){
		// Получаем доменное имя без завершающей точки
		const string name = ((domain.back() == '.') ? domain.substr(0, domain.size() - 1) : domain);
		/**
		 * Доменное имя длиннее 253 символов не помещается в 255 байт формата DNS (RFC 1035),
		 * такое имя отвергаем, чтобы запрос не выходил за пределы допустимого размера
		 */
		if(name.empty() || (name.size() > 253))
			// Выводим пустой результат
			return result;
		// Секции доменного имени
		vector <string> sections;
		// Выполняем сплит доменного имени
		this->_self->_fmk->split(name, ".", sections);
		// Если секции доменного имени получены
		if(!sections.empty()){
			// Переходим по всему списку секций
			for(auto & section : sections){
				// Если секция пустая или длиннее 63 символов (RFC 1035)
				if(section.empty() || (section.size() > 63)){
					// Выполняем очистку результата
					result.clear();
					// Выводим пустой результат
					return result;
				}
				// Добавляем в буфер данных размер записи
				result.push_back(static_cast <uint8_t> (section.size()));
				// Добавляем в буфер данные секции
				result.insert(result.end(), section.begin(), section.end());
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения доменного имени из ответа DNS
 *
 * Все чтения ограничены размером пакета, указатель сжатия читается целиком (14 бит)
 * и обязан указывать строго назад относительно предыдущей точки перехода,
 * поэтому зацикленные указатели невозможны, дополнительно ограничено количество переходов
 *
 * @param data   буфер данных из которого нужно извлечь запись
 * @param size   размер буфера данных
 * @param offset позиция в буфере данных (после выполнения указывает на конец доменного имени)
 * @param labels составные части извлечённого доменного имени
 * @return       результат извлечения доменного имени
 */
bool awh::DNS::Worker::extract(const uint8_t * data, const size_t size, size_t & offset, vector <string> & labels) const noexcept {
	// Выполняем очистку списка частей доменного имени
	labels.clear();
	// Если данные переданы
	if((data != nullptr) && (offset < size)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Количество переходов по указателям
			uint8_t hops = 0;
			// Флаг выполненного перехода по указателю
			bool jumped = false;
			// Общая длина доменного имени в формате DNS
			size_t length = 0;
			// Позиция чтения, конец доменного имени и граница для указателей
			size_t pos = offset, end = 0, limit = offset;
			/**
			 * Выполняем перебор полученного буфера данных
			 */
			for(;;){
				// Если позиция вышла за пределы пакета
				if(pos >= size)
					// Выходим с ошибкой
					return false;
				// Получаем текущий байт
				const uint8_t byte = data[pos];
				// Если найден конец доменного имени
				if(byte == 0){
					// Если переход по указателю не выполнялся
					if(!jumped)
						// Запоминаем конец доменного имени
						end = (pos + 1);
					// Выходим из цикла
					break;
				}
				/**
				 * Определяем тип записи по двум старшим битам
				 */
				switch(byte & 0xC0){
					// Если найдена обычная часть доменного имени
					case 0x00: {
						// Если часть доменного имени выходит за пределы пакета
						if((pos + 1 + byte) > size)
							// Выходим с ошибкой
							return false;
						// Увеличиваем общую длину доменного имени
						length += (static_cast <size_t> (byte) + 1);
						// Если общая длина доменного имени превышает 255 байт (RFC 1035)
						if(length > 255)
							// Выходим с ошибкой
							return false;
						// Добавляем часть доменного имени
						labels.emplace_back(reinterpret_cast <const char *> (data + pos + 1), static_cast <size_t> (byte));
						// Выполняем смещение позиции
						pos += (static_cast <size_t> (byte) + 1);
					} break;
					// Если найден указатель сжатия
					case 0xC0: {
						// Если указатель выходит за пределы пакета или переходов слишком много
						if(((pos + 1) >= size) || (++hops > DNS_MAX_HOPS))
							// Выходим с ошибкой
							return false;
						// Получаем полное 14-битное значение указателя
						const size_t target = ((static_cast <size_t> (byte & 0x3F) << 8) | static_cast <size_t> (data[pos + 1]));
						// Если указатель не указывает строго назад, он может образовать петлю
						if(target >= limit)
							// Выходим с ошибкой
							return false;
						// Если переход по указателю ещё не выполнялся
						if(!jumped)
							// Запоминаем конец доменного имени
							end = (pos + 2);
						// Запоминаем что переход выполнен
						jumped = true;
						// Устанавливаем новую границу и позицию
						limit = pos = target;
					} break;
					// Расширенные типы частей (0x40, 0x80) не поддерживаются
					default: return false;
				}
			}
			// Устанавливаем смещение на конец доменного имени
			offset = end;
			// Выводим результат
			return true;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку списка частей доменного имени
			labels.clear();
		}
	}
	// Выводим результат
	return false;
}
/**
 * @brief Метод восстановления доменного имени
 *
 * @param buffer буфер бинарных данных записи
 * @param size   размер буфера бинарных данных
 * @return       восстановленное доменное имя
 */
string awh::DNS::Worker::join(const uint8_t * buffer, const size_t size) const noexcept {
	// Выполняем извлечение частей доменного имени
	const auto & items = this->items(buffer, size);
	// Выводим результат
	return this->_self->_fmk->join(items, ".");
}
/**
 * @brief Метод извлечения частей доменного имени
 *
 * @param buffer буфер бинарных данных записи
 * @param size   размер буфера бинарных данных
 * @return       восстановленное доменное имя
 */
vector <string> awh::DNS::Worker::items(const uint8_t * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Если доменное имя передано
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Количество символов в слове
			size_t length = 0;
			// Переходим по всему доменному имени
			for(size_t i = 0; i < size;){
				// Получаем количество символов
				length = static_cast <size_t> (buffer[i]);
				// Если получили нулевой символ или указатель сжатия (два старших бита), выходим
				if((length == 0) || ((length & 0xC0) == 0xC0))
					// Выходим из цикла
					break;
				// Если часть длиннее 63 символов или выходит за пределы буфера
				if((length > 63) || ((i + 1 + length) > size)){
					// Выполняем очистку результата
					result.clear();
					// Выходим из цикла
					break;
				}
				// Добавляем часть доменного имени
				result.emplace_back(reinterpret_cast <const char *> (buffer + i + 1), length);
				// Выполняем смещение в строке
				i += (length + 1);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку результата
			result.clear();
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод закрытия подключения
 *
 */
void awh::DNS::Worker::close() noexcept {
	// Если сетевой сокет не закрыт
	if(this->_sock != INVALID_SOCKET){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем закрытие сокета
			::closesocket(this->_sock);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Выполняем закрытие сокета
			::close(this->_sock);
		#endif
		// Выполняем сброс сетевого сокета
		this->_sock = INVALID_SOCKET;
	}
}
/**
 * @brief Метод отмены выполнения запроса
 *
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
 * @brief Метод выполнения запроса
 *
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
		// Получаем объект DNS-сервера
		dns_t * self = const_cast <dns_t *> (this->_self);
		// Выполняем пересортировку серверов DNS
		self->shuffle(this->_family);
		// Выполняем очистку буфера данных
		self->_buffer.clear(buffer_t::type_t::ADDR, this->_family);
		// Получаем размер буфера данных
		const size_t size = self->_buffer.size(buffer_t::type_t::ADDR, this->_family);
		/**
		 * Определяем тип подключения
		 */
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
						::inet_pton(this->_family, host.c_str(), &client.sin_addr.s_addr);
						// Выполняем копирование объекта подключения клиента
						::memcpy(&this->_peer.client, &client, this->_peer.size);
						// Выполняем копирование объекта подключения сервера
						::memcpy(&this->_peer.server, &server, this->_peer.size);
						// Обнуляем серверную структуру
						::memset(&(reinterpret_cast <struct sockaddr_in *> (&this->_peer.server))->sin_zero, 0, sizeof(server.sin_zero));
						{
							// Выполняем запрос на удалённый DNS-сервер
							result = this->send(domain, host, ::inet_ntop(this->_family, &addr.ip, reinterpret_cast <char *> (self->_buffer.get(buffer_t::type_t::ADDR)), size));
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
						::inet_pton(this->_family, host.c_str(), &client.sin6_addr);
						// Выполняем копирование объекта подключения клиента
						::memcpy(&this->_peer.client, &client, this->_peer.size);
						// Выполняем копирование объекта подключения сервера
						::memcpy(&this->_peer.server, &server, this->_peer.size);
						{
							// Выполняем запрос на удалённый DNS-сервер
							result = this->send(domain, host, ::inet_ntop(this->_family, &addr.ip, reinterpret_cast <char *> (self->_buffer.get(buffer_t::type_t::ADDR)), size));
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
 * @param fqdn полное доменное имя для которого выполняется отправка запроса
 * @param from адрес компьютера с которого выполняется запрос
 * @param to   адрес DNS-сервера на который выполняется запрос
 * @return     полученный IP-адрес
 */
string awh::DNS::Worker::send(const string & fqdn, const string & from, const string & to) noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя установлено
	if(this->_mode && !fqdn.empty() && !from.empty() && !to.empty()){
		// Получаем объект DNS-сервера
		dns_t * self = const_cast <dns_t *> (this->_self);
		// Получаем доменное имя в нужном формате
		const auto & domain = this->split(fqdn);
		// Если доменное имя не прошло проверку (пустые части, часть длиннее 63 или имя длиннее 253 символов)
		if(domain.empty()){
			// Выводим в лог сообщение
			self->_log->print("Invalid domain name for the DNS request [DOMAIN=%s]", log_t::flag_t::WARNING, fqdn.c_str());
			// Выходим из функции
			return result;
		}
		// Выполняем очистку буфера данных
		self->_buffer.clear(buffer_t::type_t::DATA);
		// Получаем объект заголовка
		head_t * header = reinterpret_cast <head_t *> (self->_buffer.get(buffer_t::type_t::DATA));
		/**
		 * Идентификатор запроса выбирается случайно для каждого запроса,
		 * по нему ответ связывается с запросом и отсекаются подделанные ответы (RFC 5452)
		 */
		try {
			// Выполняем генерацию идентификатора запроса
			this->_id = static_cast <uint16_t> (self->_randev() & 0xFFFF);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем генерацию идентификатора запроса из текущего времени
			this->_id = static_cast <uint16_t> (std::chrono::steady_clock::now().time_since_epoch().count() & 0xFFFF);
		}
		// Устанавливаем идентификатор заголовка
		header->id = this->_id;
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
		header->qdcount = htons(static_cast <uint16_t> (1));
		// Получаем размер запроса
		size_t size = sizeof(head_t);
		// Выполняем копирование домена
		::memcpy(&self->_buffer.get(buffer_t::type_t::DATA)[size], domain.data(), domain.size());
		// Увеличиваем размер запроса
		size += (domain.size() + 1);
		// Тип записи DNS-запроса
		uint16_t qtype = 0;
		/**
		 * Определяем тип DNS-запроса
		 */
		switch(static_cast <uint8_t> (this->_qtype)){
			// Если тип DNS-запроса установлен как IP-адрес
			case static_cast <uint8_t> (q_type_t::IP): {
				/**
				 * Определяем тип подключения
				 */
				switch(this->_family){
					// Для протокола IPv4
					case AF_INET:
						// Устанавливаем тип флага запроса
						qtype = 0x0001;
					break;
					// Для протокола IPv6
					case AF_INET6:
						// Устанавливаем тип флага запроса
						qtype = 0x1C;
					break;
				}
			} break;
			// Если тип DNS-запроса установлен как PTR-запись
			case static_cast <uint8_t> (q_type_t::PTR):
				// Устанавливаем тип флага запроса
				qtype = 0xC;
			break;
		}
		// Создаём части флагов вопроса пакета запроса
		q_flags_t * qflags = reinterpret_cast <q_flags_t *> (&self->_buffer.get(buffer_t::type_t::DATA)[size]);
		// Устанавливаем тип флага запроса
		qflags->type = htons(qtype);
		// Устанавливаем класс флага запроса
		qflags->cls = htons(0x0001);
		// Увеличиваем размер запроса
		size += sizeof(q_flags_t);
		// Формируем ключ сравнения запрашиваемого доменного имени
		const string & qkey = dnsKey(this->items(domain.data(), domain.size()));
		// Создаём сокет подключения
		this->_sock = ::socket(this->_family, SOCK_DGRAM, IPPROTO_UDP);
		// Если сокет не создан создан и работа резолвера не остановлена
		if(this->_mode && (this->_sock == INVALID_SOCKET)){
			// Выводим в лог сообщение
			this->_self->_log->print("File descriptor needed for the DNS request could not be allocated", log_t::flag_t::WARNING);
			// Выполняем закрытие подключения
			this->close();
			// Выходим из приложения
			return result;
		// Если сокет создан удачно и работа резолвера не остановлена
		} else if(this->_mode) {
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->_sock);
			// Устанавливаем разрешение на закрытие сокета при неиспользовании
			this->_socket.closeOnExec(this->_sock);
			// Устанавливаем размер буфера передачи данных на чтение
			// this->_socket.bufferSize(this->_sock, AWH_BUFFER_SIZE_RCV, 1, socket_t::mode_t::READ);
			// Устанавливаем размер буфера передачи данных на запись
			// this->_socket.bufferSize(this->_sock, AWH_BUFFER_SIZE_SND, 1, socket_t::mode_t::WRITE);
			// Устанавливаем таймаут на получение данных из сокета
			this->_socket.timeout(this->_sock, this->_self->_timeout * 1000, socket_t::mode_t::READ);
			// Устанавливаем таймаут на запись данных в сокет
			this->_socket.timeout(this->_sock, this->_self->_timeout * 1000, socket_t::mode_t::WRITE);
			// Выполняем бинд на сокет
			if(::bind(this->_sock, reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.size) < 0){
				// Выводим в лог сообщение
				this->_self->_log->print("Bind local network [%s]", log_t::flag_t::CRITICAL, from.c_str());
				// Выполняем закрытие подключения
				this->close();
				// Выходим из функции
				return result;
			}
			// Количество отправленных или полученных байт
			int64_t bytes = 0;
			// Если запрос на сервер DNS успешно отправлен
			if((bytes = static_cast <int64_t> (::sendto(this->_sock, reinterpret_cast <const char *> (self->_buffer.get(buffer_t::type_t::DATA)), size, 0, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size))) > 0){
				// Смещение в бинарном буфере (после раздела запроса)
				size_t offset = 0;
				// Получаем бинарный буфер данных
				const uint8_t * data = self->_buffer.get(buffer_t::type_t::DATA);
				// Адрес с которого получен ответ
				struct sockaddr_storage peer;
				// Размер адреса с которого получен ответ
				socklen_t peerSize = 0;
				// Получаем время окончания ожидания ответа
				const auto deadline = (std::chrono::steady_clock::now() + std::chrono::seconds(this->_self->_timeout > 0 ? this->_self->_timeout : 1));
				/**
				 * Ожидаем ответ, связанный с нашим запросом. Ответы с чужого адреса, с другим идентификатором,
				 * без флага ответа или на другой вопрос отбрасываются, ожидание продолжается до истечения таймаута
				 */
				for(;;){
					// Выполняем очистку буфера данных
					self->_buffer.clear(buffer_t::type_t::DATA);
					// Очищаем адрес отправителя
					::memset(&peer, 0, sizeof(peer));
					// Устанавливаем размер адреса отправителя
					peerSize = sizeof(peer);
					// Выполняем чтение ответа сервера (адрес DNS-сервера в объекте подключения не перезаписываем)
					bytes = static_cast <int64_t> (::recvfrom(this->_sock, reinterpret_cast <char *> (self->_buffer.get(buffer_t::type_t::DATA)), self->_buffer.size(buffer_t::type_t::DATA), 0, reinterpret_cast <struct sockaddr *> (&peer), &peerSize));
					// Если данные прочитать не удалось
					if(bytes < 0)
						// Выходим из цикла
						break;
					// Если работа резолвера остановлена
					if(!this->_mode){
						// Сбрасываем количество полученных байт
						bytes = 0;
						// Выходим из цикла
						break;
					}
					// Если ответ пришёл с адреса и порта DNS-сервера, на который отправлен запрос
					if(dnsPeer(this->_family, peer, this->_peer.server) && (static_cast <size_t> (bytes) >= sizeof(head_t))){
						// Получаем количество записей в разделе запроса
						const uint16_t qdcount = static_cast <uint16_t> ((data[4] << 8) | data[5]);
						// Если идентификатор совпадает и пакет является ответом (QR = 1)
						if((::memcmp(data, &this->_id, sizeof(this->_id)) == 0) && ((data[2] & 0x80) != 0)){
							// Если в ответе ровно один вопрос
							if(qdcount == 1){
								// Составные части доменного имени вопроса
								vector <string> labels;
								// Устанавливаем смещение на раздел запроса
								offset = sizeof(head_t);
								// Если вопрос совпадает с отправленным: имя, тип и класс записи
								if(this->extract(data, static_cast <size_t> (bytes), offset, labels) &&
								   ((offset + sizeof(q_flags_t)) <= static_cast <size_t> (bytes)) &&
								   (static_cast <uint16_t> ((data[offset] << 8) | data[offset + 1]) == qtype) &&
								   (static_cast <uint16_t> ((data[offset + 2] << 8) | data[offset + 3]) == 0x0001) &&
								   (dnsKey(labels) == qkey)){
									// Увеличиваем смещение в буфере
									offset += sizeof(q_flags_t);
									// Выходим из цикла, ответ получен
									break;
								}
							/**
							 * Сервер может вернуть ошибку без раздела запроса (например FORMERR),
							 * такой ответ записей не содержит и в кэш ничего не попадает
							 */
							} else if((qdcount == 0) && ((data[3] & 0x0F) != 0)) {
								// Устанавливаем смещение на конец заголовка
								offset = sizeof(head_t);
								// Выходим из цикла, ответ получен
								break;
							}
						}
					}
					// Получаем текущее время
					const auto now = std::chrono::steady_clock::now();
					// Если время ожидания ответа истекло
					if(now >= deadline){
						// Сбрасываем количество полученных байт
						bytes = 0;
						// Выходим из цикла
						break;
					}
					// Устанавливаем оставшееся время ожидания ответа
					this->_socket.timeout(this->_sock, static_cast <uint32_t> (std::chrono::duration_cast <std::chrono::milliseconds> (deadline - now).count()) + 1, socket_t::mode_t::READ);
				}
				// Если данные прочитать не удалось
				if(bytes <= 0){
					// Выполняем закрытие подключения
					this->close();
					// Если сокет находится в блокирующем режиме
					if(bytes < 0){
						/**
						 * Определяем тип ошибки
						 */
						switch(AWH_ERROR()){
							// Если ошибка не обнаружена, выходим
							case 0: break;
							/**
							 * Для операционной системы не являющейся MS Windows
							 */
							#if !_WIN32 && !_WIN64
								// Если произведена неудачная запись в PIPE
								case EPIPE:
									// Выводим в лог сообщение
									self->_log->print("EPIPE [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
								break;
								// Если произведён сброс подключения
								case ECONNRESET:
									// Выводим в лог сообщение
									self->_log->print("ECONNRESET [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
								break;
							/**
							 * Для операционной системы MS Windows
							 */
							#else
								// Если произведён сброс подключения
								case WSAECONNRESET:
									// Выводим в лог сообщение
									self->_log->print("ECONNRESET [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
								break;
							#endif
							// Для остальных ошибок
							default:
								// Выводим в лог сообщение
								self->_log->print("%s [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, this->_socket.message(AWH_ERROR()).c_str(), to.c_str(), fqdn.c_str());
						}
					}
					// Если работа резолвера ещё не остановлена
					if(this->_mode)
						// Замораживаем поток на период времени в 10ms
						std::this_thread::sleep_for(10ms);
					// Выполняем попытку получить IP-адрес с другого сервера
					return result;
				// Если данные получены удачно
				} else {
					// Выполняем закрытие подключения
					this->close();
					/**
					 * Определяем код выполнения операции
					 */
					switch(data[3] & 0x0F){
						// Если операция выполнена удачно
						case 0: {
							// Список полученных записей
							vector <item_t> answer;
							// Список полученных серверов имён
							vector <item_t> authority;
							// Список полученных дополнительных записей
							vector <item_t> additional;
							// Размер полученного ответа
							const size_t length = static_cast <size_t> (bytes);
							// Получаем количество записей в разделах ответа, серверов имён и дополнительных записей
							const size_t ancount = static_cast <size_t> ((data[6] << 8) | data[7]);
							const size_t nscount = static_cast <size_t> ((data[8] << 8) | data[9]);
							const size_t arcount = static_cast <size_t> ((data[10] << 8) | data[11]);
							// Флаг корректности полученного ответа
							bool valid = true;
							/**
							 * Выполняем перебор всех записей всех разделов, каждое чтение ограничено размером пакета,
							 * значения количества записей и длины данных записей из пакета не принимаются на веру
							 */
							for(size_t i = 0; valid && (i < (ancount + nscount + arcount)); ++i){
								// Создаём новый объект полученной записи
								item_t item;
								// Если доменное имя записи не извлечено или заголовок записи выходит за пределы пакета
								if(!this->extract(data, length, offset, item.items) || ((offset + 10) > length)){
									// Помечаем ответ как некорректный
									valid = false;
									// Выходим из цикла
									break;
								}
								// Получаем тип записи
								const uint16_t type = static_cast <uint16_t> ((data[offset] << 8) | data[offset + 1]);
								// Получаем класс записи
								const uint16_t cls = static_cast <uint16_t> ((data[offset + 2] << 8) | data[offset + 3]);
								// Устанавливаем время жизни записи
								item.ttl = ((static_cast <uint32_t> (data[offset + 4]) << 24) | (static_cast <uint32_t> (data[offset + 5]) << 16) | (static_cast <uint32_t> (data[offset + 6]) << 8) | static_cast <uint32_t> (data[offset + 7]));
								// Получаем длину данных записи
								const size_t rdlength = static_cast <size_t> ((data[offset + 8] << 8) | data[offset + 9]);
								// Увеличиваем смещение в буфере
								offset += 10;
								// Если данные записи выходят за пределы пакета
								if((offset + rdlength) > length){
									// Помечаем ответ как некорректный
									valid = false;
									// Выходим из цикла
									break;
								}
								// Если запись принадлежит классу IN
								if(cls == 0x0001){
									/**
									 * Определяем тип полученной записи
									 */
									switch(type){
										// Если запись является интернет-протоколом IPv4
										case 1:
										// Если запись является интернет-протоколом IPv6
										case 28: {
											// Если длина данных записи не соответствует адресу (4 байта для IPv4, 16 для IPv6)
											if(rdlength != ((type == 1) ? 4 : 16)){
												// Помечаем ответ как некорректный
												valid = false;
												// Выходим из условия
												break;
											}
											// Устанавливаем данные записи
											item.record.assign(reinterpret_cast <const char *> (data + offset), rdlength);
											// Устанавливаем тип полученных данных
											item.type = type;
										} break;
										// Если мы получили сервер имён
										case 2:
										// Если запись является каноническим именем
										case 5:
										// Если запись является PTR
										case 12: {
											// Позиция доменного имени в данных записи
											size_t pos = offset;
											// Если доменное имя не извлечено или выходит за пределы данных записи
											if(!this->extract(data, length, pos, item.target) || (pos > (offset + rdlength))){
												// Помечаем ответ как некорректный
												valid = false;
												// Выходим из условия
												break;
											}
											// Выполняем извлечение значение записи
											item.record = self->_fmk->join(item.target, ".");
											// Устанавливаем тип полученных данных
											item.type = type;
										} break;
									}
								}
								// Увеличиваем смещение в буфере на размер данных записи
								offset += rdlength;
								// Если ответ корректный
								if(valid){
									// Если запись относится к разделу ответов
									if(i < ancount)
										// Добавляем запись в раздел ответов
										answer.push_back(::move(item));
									// Если запись относится к разделу серверов имён
									else if(i < (ancount + nscount))
										// Добавляем запись в раздел серверов имён
										authority.push_back(::move(item));
									// Добавляем запись в раздел дополнительных записей
									else additional.push_back(::move(item));
								}
							}
							// Если ответ некорректный
							if(!valid){
								// Выводим в лог сообщение
								self->_log->print("Malformed DNS response from nameserver %s for domain %s", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
								// Выполняем попытку получить IP-адрес с другого сервера
								return result;
							}
							// Список полученных записей
							vector <string> items;
							// Выполняем очистку списка полученных PTR-записей
							this->_ptr.clear();
							/**
							 * Принимаются только записи раздела ответов, владелец которых является запрашиваемым
							 * доменным именем или каноническим именем (CNAME), достижимым из него.
							 * Записи раздела дополнительных записей и записи с чужим владельцем в кэш не попадают.
							 */
							std::unordered_set <string> chain = {qkey};
							// Выполняем построение цепочки канонических имён (не более 16 переходов)
							for(uint8_t j = 0; j < 16; ++j){
								// Флаг добавления канонического имени
								bool added = false;
								// Выполняем перебор всего списка ответов
								for(auto & item : answer){
									// Если запись является каноническим именем и её владелец находится в цепочке
									if((item.type == 5) && (chain.count(dnsKey(item.items)) > 0))
										// Добавляем каноническое имя в цепочку
										added = (chain.emplace(dnsKey(item.target)).second || added);
								}
								// Если новых канонических имён не добавлено
								if(!added)
									// Выходим из цикла
									break;
							}
							// Выполняем перебор всего списка ответов
							for(auto & item : answer){
								// Если владелец записи не находится в цепочке запрашиваемого доменного имени
								if(chain.count(dnsKey(item.items)) == 0)
									// Пропускаем запись
									continue;
								/**
								 * Определяем тип записи
								 */
								switch(item.type){
									// Если тип получения записи PTR
									case 12: {
										/**
										 * Результат PTR-запроса в прямой кэш не записывается: иначе ответ на обратный запрос
										 * создавал бы запись «доменное имя -> IP-адрес» для любого имени, которое вернул сервер
										 */
										if(this->_qtype == q_type_t::PTR){
											// Выполняем извлечение PTR-записи
											items.push_back(item.record);
											// Добавляем PTR-запись в список полученных
											this->_ptr.push_back(item.record);
										}
									} break;
									// Если тип полученной записи IPv4
									case 1:
									// Если тип полученной записи IPv6
									case 28: {
										// Если выполнялся запрос IP-адреса
										if(this->_qtype == q_type_t::IP){
											// Тип интернет-протокола
											const int32_t family = ((item.type == 1) ? AF_INET : AF_INET6);
											// Выполняем очистку буфера данных
											self->_buffer.clear(buffer_t::type_t::ADDR, family);
											// Получаем размер буфера данных
											const size_t size = self->_buffer.size(buffer_t::type_t::ADDR, family);
											// Получаем IP-адрес принадлежащий доменному имени
											const char * addr = ::inet_ntop(family, item.record.data(), reinterpret_cast <char *> (self->_buffer.get(buffer_t::type_t::ADDR)), size);
											// Если IP-адрес получен
											if(addr != nullptr){
												// Получаем IP-адрес
												const string ip = addr;
												/**
												 * Копируем IP-адрес непосредственно в результат
												 * нам приходится это делать, так-как при разрыве подключения адрес добавляется в черный список
												 * если в выдаче IP-адрес только один, то он находится в чёрном списке, в этом случае результат всегда будет пустым.
												 */
												result = ip;
												// Если IP-адрес не находится в чёрном списке
												if(!self->isInBlackList(family, fqdn, ip)){
													// Добавляем IP-адрес в список адресов
													items.push_back(ip);
													/**
													 * Запись кэшируется под запрашиваемым доменным именем, а не под именем владельца:
													 * иначе сервер одного домена через CNAME мог бы подложить в кэш адрес для другого домена.
													 * Время жизни ограничивается сутками, чтобы подделанная запись не жила в кэше вечно.
													 */
													self->setToCache(family, fqdn, ip, (item.ttl > DNS_MAX_TTL ? DNS_MAX_TTL : item.ttl));
												}
											}
										}
									} break;
								}
							}
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим начальный разделитель
								std::cout << "------------------------------------------------------------" << std::endl << std::endl << std::flush;
								// Выводим заголовок
								std::cout << "DNS RESPONSE:" << std::endl << std::endl << std::flush;
								// Выводим название доменного имени
								printf("QNAME: %s\n", fqdn.c_str());
								// Выполняем перебор всех разделов ответа
								for(auto * section : {&answer, &authority, &additional}){
									// Выполняем перебор всего списка записей раздела
									for(auto & item : (* section)){
										// Выводим название записи
										printf("\nNAME: %s\n", self->_fmk->join(item.items, ".").c_str());
										/**
										 * Определяем тип записи
										 */
										switch(item.type){
											// Если тип полученной записи NS
											case 2: printf("NS: %s\n", item.record.c_str()); break;
											// Если тип полученной записи CNAME
											case 5: printf("CNAME: %s\n", item.record.c_str()); break;
											// Если тип получения записи PTR
											case 12: printf("PTR: %s\n", item.record.c_str()); break;
											// Если тип полученной записи IPv4
											case 1:
											// Если тип полученной записи IPv6
											case 28: {
												// Тип интернет-протокола
												const int32_t family = ((item.type == 1) ? AF_INET : AF_INET6);
												// Буфер для извлечения IP-адреса
												char buffer[INET6_ADDRSTRLEN];
												// Получаем IP-адрес принадлежащий доменному имени
												const char * ip = ::inet_ntop(family, item.record.data(), buffer, sizeof(buffer));
												// Выводим IP-адрес
												printf("%s: %s\n", (item.type == 1 ? "IPv4" : "IPv6"), (ip != nullptr ? ip : ""));
											} break;
										}
									}
								}
								// Выводим конечный разделитель
								std::cout << std::endl << "------------------------------------------------------------" << std::endl << std::endl << std::flush;
							#endif
							// Если список записей получен
							if(!items.empty()){
								// Если количество записей в списке больше 1-й
								if(items.size() > 1){
									// Переходим по всему списку полученных записей
									for(auto & addr : items){
										// Если запись не найдена в списке
										if(self->_using.find(addr) == self->_using.end()){
											// Выполняем установку записи
											result.assign(addr.begin(), addr.end());
											// Выходим из цикла
											break;
										}
									}
								}
								// Если запись не установлена
								if(result.empty()){
									// Выполняем установку первой записи в списке
									result = items.front();
									// Если количество записей в списке больше 1-й
									if(items.size() > 1){
										// Получаем текущее значение записи
										auto i = items.begin();
										// Выполняем смещение итератора
										advance(i, 1);
										// Переходим по всему списку полученных записей
										for(; i != items.end(); ++i)
											// Очищаем список используемых записей
											self->_using.erase(* i);
									}
								// Если запись получена, то запоминаем полученную запись
								} else self->_using.emplace(result);
							// Если IP-адрес получен всего один и он в чёрном списке
							} else if(!result.empty())
								// Выполняем удаления IP-адреса из чёрного списка
								self->delInBlackList(this->_family, fqdn, result);
						} break;
						// Если сервер DNS не смог интерпретировать запрос
						case 1:
							// Выводим в лог сообщение
							self->_log->print("DNS query format error to nameserver %s for domain %s", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
						break;
						// Если проблемы возникли на DNS-сервера
						case 2:
							// Выводим в лог сообщение
							self->_log->print("DNS server failure %s for domain %s", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
						break;
						// Если доменное имя указанное в запросе не существует
						case 3:
							// Выводим в лог сообщение
							self->_log->print("Domain name %s referenced in the query for nameserver %s does not exist", log_t::flag_t::WARNING, fqdn.c_str(), to.c_str());
						break;
						// Если DNS-сервер не поддерживает подобный тип запросов
						case 4:
							// Выводим в лог сообщение
							self->_log->print("DNS server is not implemented at %s for domain %s", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
						break;
						// Если DNS-сервер отказался выполнять наш запрос (например по политическим причинам)
						case 5:
							// Выводим в лог сообщение
							self->_log->print("DNS request is refused to nameserver %s for domain %s", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
						break;
					}
				}
				// Если мы получили результат
				if(!result.empty())
					// Выводим результат
					return result;
			// Если сообщение отправить не удалось
			} else if(bytes <= 0) {
				// Выполняем закрытие подключения
				this->close();
				// Если сокет находится в блокирующем режиме
				if(bytes < 0){
					/**
					 * Определяем тип ошибки
					 */
					switch(AWH_ERROR()){
						// Если ошибка не обнаружена, выходим
						case 0: break;
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#if !_WIN32 && !_WIN64
							// Если произведена неудачная запись в PIPE
							case EPIPE:
								// Выводим в лог сообщение
								this->_self->_log->print("EPIPE [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
							break;
							// Если произведён сброс подключения
							case ECONNRESET:
								// Выводим в лог сообщение
								this->_self->_log->print("ECONNRESET [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
							break;
						/**
						 * Для операционной системы MS Windows
						 */
						#else
							// Если произведён сброс подключения
							case WSAECONNRESET:
								// Выводим в лог сообщение
								this->_self->_log->print("ECONNRESET [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, to.c_str(), fqdn.c_str());
							break;
						#endif
						// Для остальных ошибок
						default:
							// Выводим в лог сообщение
							this->_self->_log->print("%s [SERVER=%s, DOMAIN=%s]", log_t::flag_t::WARNING, this->_socket.message(AWH_ERROR()).c_str(), to.c_str(), fqdn.c_str());
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Деструктор
 *
 */
awh::DNS::Worker::~Worker() noexcept {
	// Выполняем закрытие файлового дерскриптора (сокета)
	this->close();
}
/**
 * @brief Метод кодирования интернационального доменного имени
 *
 * @param domain доменное имя для кодирования
 * @return       результат работы кодирования
 */
string awh::DNS::encode(const string & domain) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Если используется модуль IDN
	 */
	#if AWH_IDN
		// Если доменное имя передано
		if(!domain.empty() && (domain.front() != '-') && (domain.back() != '-')){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Результирующий буфер данных
				wchar_t buffer[_MAX_PATH];
				// Выполняем кодирования доменного имени
				if(::IdnToAscii(0, this->_fmk->convert(domain).c_str(), -1, buffer, sizeof(buffer)) == 0)
					// Выводим в лог сообщение
					this->_log->print("IDN encode failed (%d): DOMAIN=\"%s\"", log_t::flag_t::CRITICAL, GetLastError(), domain.c_str());
				// Получаем результат кодирования
				else result = this->_fmk->convert(wstring{buffer});
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем кодирования доменного имени
				const int32_t rc = ::idn2_to_ascii_8z(domain.c_str(), &buffer, IDN2_NONTRANSITIONAL);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS)
					// Выводим в лог сообщение
					this->_log->print("IDN encode failed (%d): %s, DOMAIN=\"%s\"", log_t::flag_t::CRITICAL, rc, idn2_strerror(rc), domain.c_str());
				// Получаем результат кодирования
				else result = buffer;
				// Если память была выделенна
				if(buffer != nullptr)
					// Очищаем буфер данных
					::free(buffer);
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * @brief Метод декодирования интернационального доменного имени
 *
 * @param domain доменное имя для декодирования
 * @return       результат работы декодирования
 */
string awh::DNS::decode(const string & domain) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Если используется модуль IDN
	 */
	#if AWH_IDN
		// Если доменное имя передано
		if(!domain.empty() && (domain.front() != '-') && (domain.back() != '-')){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Результирующий буфер данных
				wchar_t buffer[_MAX_PATH];
				// Выполняем кодирования доменного имени
				if(::IdnToUnicode(0, this->_fmk->convert(domain).c_str(), -1, buffer, sizeof(buffer)) == 0)
					// Выводим в лог сообщение
					this->_log->print("IDN decode failed (%d): DOMAIN=\"%s\"", log_t::flag_t::CRITICAL, GetLastError(), domain.c_str());
				// Получаем результат кодирования
				else result = this->_fmk->convert(wstring{buffer});
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем декодирования доменного имени
				const int32_t rc = ::idn2_to_unicode_8z8z(domain.c_str(), &buffer, 0);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS)
					// Выводим в лог сообщение
					this->_log->print("IDN decode failed (%d): %s, DOMAIN=\"%s\"", log_t::flag_t::CRITICAL, rc, idn2_strerror(rc), domain.c_str());
				// Получаем результат декодирования
				else result = buffer;
				// Если память была выделенна
				if(buffer != nullptr)
					// Очищаем буфер данных
					::free(buffer);
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * @brief Метод очистки данных DNS-резолвера
 *
 * @return результат работы функции
 */
bool awh::DNS::clear() noexcept {
	// Результат работы функции
	bool result = false;
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
 * @brief Метод сброса кэша DNS-резолвера
 *
 * @return результат работы функции
 */
bool awh::DNS::flush() noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if((result = hold.access({status_t::CLEAR}, status_t::FLUSH))){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Выполняем сброс кэша полученных IPv4-адресов
		this->_cacheIPv4.clear();
		// Выполняем сброс кэша полученных IPv6-адресов
		this->_cacheIPv6.clear();
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод отмены выполнения запроса
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::DNS::cancel(const int32_t family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	/**
	 * Определяем тип протокола подключения
	 */
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int32_t> (AF_INET):
			// Выполняем отмену резолвинга домена
			this->_workerIPv4->cancel();
		break;
		// Если тип протокола подключения IPv6
		case static_cast <int32_t> (AF_INET6):
			// Выполняем отмену резолвинга домена
			this->_workerIPv6->cancel();
		break;
	}
}
/**
 * @brief Метод пересортировки серверов DNS
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::DNS::shuffle(const int32_t family) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Выбираем стаднарт рандомайзера
		mt19937 generator(this->_randev());
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET):
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(this->_serversIPv4.begin(), this->_serversIPv4.end(), generator);
			break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6):
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(this->_serversIPv6.begin(), this->_serversIPv6.end(), generator);
			break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const runtime_error & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(family), log_t::flag_t::WARNING, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, error.what());
		#endif
	}
}
/**
 * @brief Метод установки времени ожидания выполнения запроса
 *
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::DNS::timeout(const uint8_t sec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку таймаута ожидания выполнения запроса
	this->_timeout = sec;
}
/**
 * @brief Метод получения IP-адреса из кэша
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @return       IP-адрес находящийся в кэше
 */
string awh::DNS::cache(const int32_t family, const string & domain) noexcept {
	// Результат работы функции
	string result = "";
	// Если доменное имя передано
	if(!domain.empty()){
		/**
		 * Кэш и буфер адресов читаются под блокировкой: они изменяются из других потоков
		 * (запись в кэш резолвером, очистка кэша), чтение без блокировки приводит к гонке
		 */
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Список полученных IP-адресов
		vector <string> ips;
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Если кэш доменных имён проинициализирован
				if(!this->_cacheIPv4.empty()){
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv4.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto i = ret.first; i != ret.second;){
						// Если IP-адрес не находится в чёрном списке
						if(!i->second.forbidden){
							// Если время жизни кэша ещё не вышло
							if((i->second.create == 0) || ((this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS) - i->second.create) <= static_cast <uint64_t> (i->second.ttl))){
								// Выполняем очистку буфера данных
								this->_buffer.clear(buffer_t::type_t::ADDR, family);
								// Получаем размер буфера данных
								const size_t size = this->_buffer.size(buffer_t::type_t::ADDR, family);
								// Выполняем формирование списка полученных IP-адресов
								ips.push_back(::inet_ntop(family, &i->second.ip, reinterpret_cast <char *> (this->_buffer.get(buffer_t::type_t::ADDR)), size));
								// Выполняем смещение итератора
								++i;
							// Если время жизни кэша уже вышло
							} else {
								// Выполняем блокировку потока
								const lock_guard <std::recursive_mutex> lock(this->_mtx);
								// Выполняем удаление записи из кэша
								i = this->_cacheIPv4.erase(i);
							}
						// Выполняем смещение итератора
						} else ++i;
					}
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Если кэш доменных имён проинициализирован
				if(!this->_cacheIPv6.empty()){
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv6.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto i = ret.first; i != ret.second;){
						// Если IP-адрес не находится в чёрном списке
						if(!i->second.forbidden){
							// Если время жизни кэша ещё не вышло
							if((i->second.create == 0) || ((this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS) - i->second.create) <= static_cast <uint64_t> (i->second.ttl))){
								// Выполняем очистку буфера данных
								this->_buffer.clear(buffer_t::type_t::ADDR, family);
								// Получаем размер буфера данных
								const size_t size = this->_buffer.size(buffer_t::type_t::ADDR, family);
								// Выполняем формирование списка полученных IP-адресов
								ips.push_back(::inet_ntop(family, &i->second.ip, reinterpret_cast <char *> (this->_buffer.get(buffer_t::type_t::ADDR)), size));
								// Выполняем смещение итератора
								++i;
							// Если время жизни кэша уже вышло
							} else {
								// Выполняем блокировку потока
								const lock_guard <std::recursive_mutex> lock(this->_mtx);
								// Выполняем удаление записи из кэша
								i = this->_cacheIPv6.erase(i);
							}
						// Выполняем смещение итератора
						} else ++i;
					}
				}
			} break;
		}
		// Если список IP-адресов получен
		if(!ips.empty()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Подключаем устройство генератора
				mt19937 generator(this->_randev());
				// Выполняем генерирование случайного числа
				uniform_int_distribution <mt19937::result_type> dist6(0, ips.size() - 1);
				// Выполняем получение результата
				result = ::move(ips.at(::move(dist6(generator))));
			/**
			 * Если возникает ошибка
			 */
			} catch(const runtime_error & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(family, domain), log_t::flag_t::WARNING, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::WARNING, error.what());
				#endif
				// Выполняем извлечение первого адреса из списка
				result = ips.front();
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод очистки кэша для указанного доменного имени
 *
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
 * @brief Метод очистки кэша для указанного доменного имени
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя для которого выполняется очистка кэша
 */
void awh::DNS::clearCache(const int32_t family, const string & domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second;){
					// Если мы IP-адрес не запрещён
					if(!i->second.forbidden)
						// Выполняем удаление IP-адреса
						i = this->_cacheIPv4.erase(i);
					// Иначе продолжаем перебор дальше
					else ++i;
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second;){
					// Если мы IP-адрес не запрещён
					if(!i->second.forbidden)
						// Выполняем удаление IP-адреса
						i = this->_cacheIPv6.erase(i);
					// Иначе продолжаем перебор дальше
					else ++i;
				}
			} break;
		}
	}
}
/**
 * @brief Метод очистки кэша
 *
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::clearCache(const bool localhost) noexcept {
	// Выполняем очистку кэша доменного имени для IPv4
	this->clearCache(AF_INET, localhost);
	// Выполняем очистку кэша доменного имени для IPv6
	this->clearCache(AF_INET6, localhost);
}
/**
 * @brief Метод очистки кэша
 *
 * @param family    тип интернет-протокола AF_INET, AF_INET6
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::clearCache(const int32_t family, const bool localhost) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	/**
	 * Определяем тип протокола подключения
	 */
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int32_t> (AF_INET): {
			// Переходим по всему списку IP-адресов
			for(auto i = this->_cacheIPv4.begin(); i != this->_cacheIPv4.end();){
				// Если мы нашли нужный тип IP-адреса
				if(!i->second.forbidden && (i->second.localhost == localhost))
					// Выполняем удаление IP-адреса
					i = this->_cacheIPv4.erase(i);
				// Иначе продолжаем перебор дальше
				else ++i;
			}
		} break;
		// Если тип протокола подключения IPv6
		case static_cast <int32_t> (AF_INET6): {
			// Переходим по всему списку IP-адресов
			for(auto i = this->_cacheIPv6.begin(); i != this->_cacheIPv6.end();){
				// Если мы нашли нужный тип IP-адреса
				if(!i->second.forbidden && (i->second.localhost == localhost))
					// Выполняем удаление IP-адреса
					i = this->_cacheIPv6.erase(i);
				// Иначе продолжаем перебор дальше
				else ++i;
			}
		} break;
	}
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param domain    доменное имя соответствующее IP-адресу
 * @param ip        адрес для добавления к кэш
 * @param ttl       время жизни кэша доменного имени
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::setToCache(const string & domain, const string & ip, const uint32_t ttl, const bool localhost) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty() && (ttl > 0)){
		/**
		 * Определяем тип передаваемого IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_net.host(ip))){
			// Если IP-адрес является IPv4 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Выполняем добавление IP-адреса в кэш
				this->setToCache(AF_INET, domain, ip, ttl, localhost);
			break;
			// Если IP-адрес является IPv6 адресом
			case static_cast <uint8_t> (net_t::type_t::IPV6):
				// Выполняем добавление IP-адреса в кэш
				this->setToCache(AF_INET6, domain, ip, ttl, localhost);
			break;
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param family    тип интернет-протокола AF_INET, AF_INET6
 * @param domain    доменное имя соответствующее IP-адресу
 * @param ip        адрес для добавления к кэш
 * @param ttl       время жизни кэша доменного имени
 * @param localhost флаг обозначающий добавление локального адреса
 */
void awh::DNS::setToCache(const int32_t family, const string & domain, const string & ip, const uint32_t ttl, const bool localhost) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty() && (ttl > 0)){
		// Результат проверки наличия IP-адреса в кэше
		bool result = false;
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[1];
					// Выполняем копирование полученных данных в переданный буфер
					::inet_pton(family, ip.c_str(), &buffer);
					// Выполняем проверку соответствует ли IP-адрес в кэше добавляемому сейчас
					result = (::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0);
					// Если IP-адрес соответствует переданному адресу
					if(result)
						// Выходим из условия
						break;
				}
				// Если IP-адрес в кэше не найден
				if(!result){
					// Создаём объект кэша
					cache_t <1> cache;
					// Устанавливаем время жизни кэша
					cache.ttl = ttl;
					// Устанавливаем флаг локального хоста
					cache.localhost = localhost;
					// Устанавливаем время создания кэша
					cache.create = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
					// Выполняем копирование полученных данных в переданный буфер
					::inet_pton(family, ip.c_str(), &cache.ip);
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv4.emplace(domain, cache);
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[4];
					// Выполняем копирование полученных данных в переданный буфер
					::inet_pton(family, ip.c_str(), &buffer);
					// Выполняем проверку соответствует ли IP-адрес в кэше добавляемому сейчас
					result = (::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0);
					// Если IP-адрес соответствует переданному адресу
					if(result)
						// Выходим из условия
						break;
				}
				// Если IP-адрес в кэше не найден
				if(!result){
					// Создаём объект кэша
					cache_t <4> cache;
					// Устанавливаем время жизни кэша
					cache.ttl = ttl;
					// Устанавливаем флаг локального хоста
					cache.localhost = localhost;
					// Устанавливаем время создания кэша
					cache.create = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
					// Выполняем копирование полученных данных в переданный буфер
					::inet_pton(family, ip.c_str(), &cache.ip);
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv6.emplace(domain, cache);
				}
			} break;
		}
	}
}
/**
 * @brief Метод очистки чёрного списка
 *
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
 * @brief Метод очистки чёрного списка
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::DNS::clearBlackList(const int32_t family, const string & domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty()){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Если мы нашли запрещённую запись
					if(i->second.forbidden)
						// Снимаем флаг запрещённого IP-адреса
						i->second.forbidden = !i->second.forbidden;
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Если мы нашли запрещённую запись
					if(i->second.forbidden)
						// Снимаем флаг запрещённого IP-адреса
						i->second.forbidden = !i->second.forbidden;
				}
			} break;
		}
	}
}
/**
 * @brief Метод удаления IP-адреса из чёрного списока
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::DNS::delInBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		/**
		 * Определяем тип передаваемого IP-адреса
		 */
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
 * @brief Метод удаления IP-адреса из чёрного списока
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::DNS::delInBlackList(const int32_t family, const string & domain, const string & ip) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Если мы нашли запрещённую запись
					if(i->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[1];
						// Выполняем копирование полученных данных в переданный буфер
						::inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if(::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0)
							// Снимаем флаг запрещённого IP-адреса
							i->second.forbidden = !i->second.forbidden;
					}
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Если мы нашли запрещённую запись
					if(i->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[4];
						// Выполняем копирование полученных данных в переданный буфер
						::inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if(::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0)
							// Снимаем флаг запрещённого IP-адреса
							i->second.forbidden = !i->second.forbidden;
					}
				}
			} break;
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::DNS::setToBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		/**
		 * Определяем тип передаваемого IP-адреса
		 */
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
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::DNS::setToBlackList(const int32_t family, const string & domain, const string & ip) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Создаём объект кэша
				cache_t <1> cache;
				// Устанавливаем флаг запрещённого домена
				cache.forbidden = true;
				// Выполняем копирование полученных данных в переданный буфер
				::inet_pton(family, ip.c_str(), &cache.ip);
				// Если список кэша является пустым
				if(this->_cacheIPv4.empty())
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv4.emplace(domain, cache);
				// Если данные в кэше уже есть
				else {
					// Результат поиска IP-адреса
					bool result = false;
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv4.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto i = ret.first; i != ret.second; ++i){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[1];
						// Выполняем копирование полученных данных в переданный буфер
						::inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0))){
							// Выполняем блокировку IP-адреса
							i->second.forbidden = result;
							// Выходим из цикла
							break;
						}
					}
					// Если адрес не найден
					if(!result)
						// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
						this->_cacheIPv4.emplace(domain, cache);
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Создаём объект кэша
				cache_t <4> cache;
				// Устанавливаем флаг запрещённого домена
				cache.forbidden = true;
				// Выполняем копирование полученных данных в переданный буфер
				::inet_pton(family, ip.c_str(), &cache.ip);
				// Если список кэша является пустым
				if(this->_cacheIPv6.empty())
					// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
					this->_cacheIPv6.emplace(domain, cache);
				// Если данные в кэше уже есть
				else {
					// Результат поиска IP-адреса
					bool result = false;
					// Получаем диапазон IP-адресов в кэше
					auto ret = this->_cacheIPv6.equal_range(domain);
					// Переходим по всему списку IP-адресов
					for(auto i = ret.first; i != ret.second; ++i){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[4];
						// Выполняем копирование полученных данных в переданный буфер
						::inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0))){
							// Выполняем блокировку IP-адреса
							i->second.forbidden = result;
							// Выходим из цикла
							break;
						}
					}
					// Если адрес не найден
					if(!result)
						// Выполняем установку полученного IP-адреса в кэш DNS-резолвера
						this->_cacheIPv6.emplace(domain, cache);
				}
			} break;
		}
	}
}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::DNS::isInBlackList(const string & domain, const string & ip) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		/**
		 * Определяем тип передаваемого IP-адреса
		 */
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
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::DNS::isInBlackList(const int32_t family, const string & domain, const string & ip) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если доменное имя и IP-адрес переданы
	if(!domain.empty() && !ip.empty()){
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(domain, fmk_t::transform_t::LOWER);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv4.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Если мы нашли запрещённую запись
					if(i->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[1];
						// Выполняем копирование полученных данных в переданный буфер
						::inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0)))
							// Выводим результат проверки
							return result;
					}
				}
			}
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Получаем диапазон IP-адресов в кэше
				auto ret = this->_cacheIPv6.equal_range(domain);
				// Переходим по всему списку IP-адресов
				for(auto i = ret.first; i != ret.second; ++i){
					// Если мы нашли запрещённую запись
					if(i->second.forbidden){
						// Создаём буфер бинарных данных IP-адреса
						uint32_t buffer[4];
						// Выполняем копирование полученных данных в переданный буфер
						::inet_pton(family, ip.c_str(), &buffer);
						// Если IP-адрес соответствует переданному адресу
						if((result = (::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0)))
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
 * @brief Метод получения данных сервера имён
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @return       запрошенный сервер имён
 */
string awh::DNS::server(const int32_t family) noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Подключаем устройство генератора
		mt19937 generator(this->_randev());
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Если список серверов пустой
				if(this->_serversIPv4.empty())
					// Устанавливаем новый список имён
					this->replace(family);
				// Получаем первое значение итератора
				auto i = this->_serversIPv4.begin();
				// Выполняем генерирование случайного числа
				uniform_int_distribution <mt19937::result_type> dist6(0, this->_serversIPv4.size() - 1);
				// Выполняем выбор нужного сервера в списке, в произвольном виде
				advance(i, dist6(generator));
				// Выполняем очистку буфера данных
				this->_buffer.clear(buffer_t::type_t::ADDR, family);
				// Получаем размер буфера данных
				const size_t size = this->_buffer.size(buffer_t::type_t::ADDR, family);
				// Выполняем получение данных IP-адреса
				result = ::inet_ntop(family, &i->ip, reinterpret_cast <char *> (this->_buffer.get(buffer_t::type_t::ADDR)), size);
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Если список серверов пустой
				if(this->_serversIPv6.empty())
					// Устанавливаем новый список имён
					this->replace(family);
				// Получаем первое значение итератора
				auto i = this->_serversIPv6.begin();
				// Выполняем генерирование случайного числа
				uniform_int_distribution <mt19937::result_type> dist6(0, this->_serversIPv6.size() - 1);
				// Выполняем выбор нужного сервера в списке, в произвольном виде
				advance(i, dist6(generator));
				// Выполняем очистку буфера данных
				this->_buffer.clear(buffer_t::type_t::ADDR, family);
				// Получаем размер буфера данных
				const size_t size = this->_buffer.size(buffer_t::type_t::ADDR, family);
				// Выполняем получение данных IP-адреса
				result = ::inet_ntop(family, &i->ip, reinterpret_cast <char *> (this->_buffer.get(buffer_t::type_t::ADDR)), size);
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const runtime_error & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(family), log_t::flag_t::WARNING, error.what());
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
 * @brief Метод добавления сервера DNS
 *
 * @param server адрес DNS-сервера
 */
void awh::DNS::server(const string & server) noexcept {
	// Если адрес сервера передан
	if(!server.empty()){
		/**
		 * Определяем тип передаваемого IP-адреса
		 */
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
 * @brief Метод добавления сервера DNS
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param server адрес DNS-сервера
 */
void awh::DNS::server(const int32_t family, const string & server) noexcept {
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::NSS_SET}, status_t::NS_SET)){
		// Если адрес сервера передан
		if(!server.empty()){
			// Хост переданного сервера
			string host = "";
			// Порт переданного сервера
			uint32_t port = 53;
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			/**
			 * Определяем тип передаваемого сервера
			 */
			switch(static_cast <uint8_t> (this->_net.host(server))){
				// Если домен является адресом в файловой системе
				case static_cast <uint8_t> (net_t::type_t::FS):
				// Если домен является аппаратным адресом сетевого интерфейса
				case static_cast <uint8_t> (net_t::type_t::MAC):
				// Если домен является URL-адресом
				case static_cast <uint8_t> (net_t::type_t::URL):
				// Если домен является адресом/Маски сети
				case static_cast <uint8_t> (net_t::type_t::NETWORK): break;
				// Если хост является IPv4-адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4): {
					// Выполняем поиск разделителя порта
					const size_t pos = server.rfind(":");
					// Если позиция разделителя найдена
					if(pos != string::npos){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Извлекаем хост сервера имён
							host = server.substr(0, pos);
							// Извлекаем порт сервера имён
							port = static_cast <uint32_t> (::stoi(server.substr(pos + 1)));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Извлекаем порт сервера имён
							port = 0;
						}
					// Извлекаем хост сервера имён
					} else host = server;
				} break;
				// Если хост является IPv6-адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6): {
					// Если первый символ является разделителем
					if(server.front() == '['){
						// Выполняем поиск разделителя порта
						const size_t pos = server.rfind("]:");
						// Если позиция разделителя найдена
						if(pos != string::npos){
							/**
							 * Выполняем отлов ошибок
							 */
							try {
								// Извлекаем хост сервера имён
								host = server.substr(1, pos - 1);
								// Запоминаем полученный порт
								port = static_cast <uint32_t> (::stoi(server.substr(pos + 2)));
							/**
							 * Если возникает ошибка
							 */
							} catch(const exception &) {
								// Извлекаем порт сервера имён
								port = 0;
							}
						// Заполняем полученный сервер
						} else if(server.back() == ']')
							// Извлекаем хост сервера имён
							host = server.substr(1, server.size() - 2);
					// Заполняем полученный сервер
					} else if(server.back() != ']')
						// Извлекаем хост сервера имён
						host = server;
				} break;
				// Если хост является доменной зоной
				case static_cast <uint8_t> (net_t::type_t::FQDN): {
					// Выполняем поиск разделителя порта
					const size_t pos = server.rfind(":");
					// Если позиция разделителя найдена
					if(pos != string::npos){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Извлекаем хост сервера имён
							host = server.substr(0, pos);
							// Извлекаем порт сервера имён
							port = static_cast <uint32_t> (::stoi(server.substr(pos + 1)));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Извлекаем порт сервера имён
							port = 0;
						}
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
					} else host = ::move(ip);
				} break;
				// Значит скорее всего, садрес является доменным именем
				default: {
					// Выполняем поиск разделителя порта
					const size_t pos = server.rfind(":");
					// Если позиция разделителя найдена
					if(pos != string::npos){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Извлекаем хост сервера имён
							host = this->host(family, server.substr(0, pos));
							// Извлекаем порт сервера имён
							port = static_cast <uint32_t> (::stoi(server.substr(pos + 1)));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Извлекаем порт сервера имён
							port = 0;
						}
					// Извлекаем хост сервера имён
					} else host = this->host(family, server);
				}
			}
			// Если DNS-сервер имён получен
			if(!host.empty()){
				/**
				 * Определяем тип протокола подключения
				 */
				switch(family){
					// Если тип протокола подключения IPv4
					case static_cast <int32_t> (AF_INET): {
						// Создаём объект сервера DNS
						server_t <1> server;
						// Запоминаем полученный порт
						server.port = port;
						// Запоминаем полученный сервер
						::inet_pton(family, host.c_str(), &server.ip);
						// Если добавляемый хост сервера ещё не существует в списке серверов
						if(find_if(this->_serversIPv4.begin(), this->_serversIPv4.end(), [&server](const server_t <1> & item) noexcept -> bool {
							// Выполняем сравнение двух IP-адресов
							return (::memcmp(&server.ip, &item.ip, sizeof(item.ip)) == 0);
						}) == this->_serversIPv4.end()){
							// Выполняем добавление полученный сервер в список DNS-серверов
							this->_serversIPv4.push_back(server);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим заголовок запроса
								std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD DNS SERVER ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
								// Выводим параметры запроса
								std::cout << host << ":" << port << std::endl << std::flush;
							#endif
						}
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <int32_t> (AF_INET6): {
						// Создаём объект сервера DNS
						server_t <4> server;
						// Запоминаем полученный порт
						server.port = port;
						// Запоминаем полученный сервер
						::inet_pton(family, host.c_str(), &server.ip);
						// Если добавляемый хост сервера ещё не существует в списке серверов
						if(find_if(this->_serversIPv6.begin(), this->_serversIPv6.end(), [&server](const server_t <4> & item) noexcept -> bool {
							// Выполняем сравнение двух IP-адресов
							return (::memcmp(&server.ip, &item.ip, sizeof(item.ip)) == 0);
						}) == this->_serversIPv6.end()){
							// Выполняем добавление полученный сервер в список DNS-серверов
							this->_serversIPv6.push_back(server);
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим заголовок запроса
								std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ ADD DNS SERVER ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
								// Выводим параметры запроса
								std::cout << "[" << host << "]:" << port << std::endl << std::flush;
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
 * @brief Метод добавления серверов DNS
 *
 * @param servers адреса DNS-серверов
 */
void awh::DNS::servers(const vector <string> & servers) noexcept {
	// Если список серверов передан
	if(!servers.empty()){
		// Создаём объект холдирования
		hold_t <status_t> hold(this->_status);
		// Если статус работы DNS-резолвера соответствует
		if(hold.access({status_t::NSS_REP}, status_t::NSS_SET)){
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			// Переходим по всем нейм серверам и добавляем их
			for(auto & server : servers){
				/**
				 * Определяем тип передаваемого IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_net.host(server))){
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
						// Выполняем добавление IPv4-адреса в список серверов
						this->server(AF_INET, server);
					break;
					// Если IP-адрес является IPv6-адресом
					case static_cast <uint8_t> (net_t::type_t::IPV6):
						// Выполняем добавление IPv6-адреса в список серверов
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
 * @brief Метод добавления серверов DNS
 *
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers адреса DNS-серверов
 */
void awh::DNS::servers(const int32_t family, const vector <string> & servers) noexcept {
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
 * @brief Метод замены существующих серверов DNS
 *
 * @param servers адреса DNS-серверов
 */
void awh::DNS::replace(const vector <string> & servers) noexcept {
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::RESOLVE}, status_t::NSS_REP)){
		// Список серверов IPv4
		vector <string> ipv4, ipv6;
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : servers){
			/**
			 * Определяем тип передаваемого IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_net.host(server))){
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
					// Выполняем добавление IPv4-адреса в список серверов
					ipv4.push_back(server);
				break;
				// Если IP-адрес является IPv6-адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6):
					// Выполняем добавление IPv6-адреса в список серверов
					ipv6.push_back(server);
				break;
				// Для всех остальных адресов
				default: {
					// Выполняем добавление IPv4-адреса в список серверов
					ipv4.push_back(server);
					// Выполняем добавление IPv6-адреса в список серверов
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
 * @brief Метод замены существующих серверов DNS
 *
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param servers адреса DNS-серверов
 */
void awh::DNS::replace(const int32_t family, const vector <string> & servers) noexcept {
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({status_t::RESOLVE, status_t::NSS_REP}, status_t::NSS_REP)){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		/**
		 * Определяем тип подключения
		 */
		switch(family){
			// Для протокола IPv4
			case static_cast <int32_t> (AF_INET):
				// Выполняем очистку списка DNS-серверов
				this->_serversIPv4.clear();
			break;
			// Для протокола IPv6
			case static_cast <int32_t> (AF_INET6):
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
			/**
			 * Определяем тип подключения
			 */
			switch(family){
				// Для протокола IPv4
				case static_cast <int32_t> (AF_INET):
					// Устанавливаем список серверов
					servers = IPV4_RESOLVER;
				break;
				// Для протокола IPv6
				case static_cast <int32_t> (AF_INET6):
					// Устанавливаем список серверов
					servers = IPV6_RESOLVER;
				break;
			}
			// Устанавливаем новый список серверов
			this->servers(family, servers);
		}
	}
}
/**
 * @brief Метод установки адреса сетевых плат, с которых нужно выполнять запросы
 *
 * @param network IP-адреса сетевых плат
 */
void awh::DNS::network(const vector <string> & network) noexcept {
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы установки параметров сети соответствует
	if(hold.access({}, status_t::NET_SET)){
		// Если список адресов сетевых плат передан
		if(!network.empty()){
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			// Переходим по всему списку полученных адресов
			for(auto & host : network){
				/**
				 * Определяем к какому адресу относится полученный хост
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
						this->_workerIPv4->_network.push_back(host);
					break;
					// Если IP-адрес является IPv6-адресом
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
						// Если результат не получен, выполняем получение IPv4-адреса
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
 * @brief Метод установки адреса сетевых плат, с которых нужно выполнять запросы
 *
 * @param family  тип интернет-протокола AF_INET, AF_INET6
 * @param network IP-адреса сетевых плат
 */
void awh::DNS::network(const int32_t family, const vector <string> & network) noexcept {
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы установки параметров сети соответствует
	if(hold.access({}, status_t::NET_SET)){
		// Если список адресов сетевых плат передан
		if(!network.empty()){
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			// Переходим по всему списку полученных адресов
			for(auto & host : network){
				/**
				 * Определяем тип передаваемого IP-адреса
				 */
				switch(family){
					// Если IP-адрес является IPv4 адресом
					case static_cast <int32_t> (AF_INET):
						// Выполняем добавление полученного хоста в список
						this->_workerIPv4->_network.push_back(host);
					break;
					// Если IP-адрес является IPv6 адресом
					case static_cast <int32_t> (AF_INET6):
						// Выполняем добавление полученного хоста в список
						this->_workerIPv6->_network.push_back(host);
					break;
				}
			}
		}
	}
}
/**
 * @brief Метод установки префикса переменной окружения
 *
 * @param prefix префикс переменной окружения для установки
 */
void awh::DNS::prefix(const string & prefix) noexcept {
	// Выполняем установку префикса переменной окружения
	this->_prefix = prefix;
}
/**
 * @brief Метод загрузки файла со списком хостов
 *
 * @param filename адрес файла для загрузки
 */
void awh::DNS::hosts(const string & filename) noexcept {
	// Если адрес файла получен
	if(!filename.empty()){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Создаём объект для работы с файловой системой
		fs_t fs(this->_fmk, this->_log);
		// Выполняем установку адреса файла hosts
		const string & hosts = fs.realPath(filename, false);
		// Если файл существует в файловой системе
		if(fs.isFile(hosts)){
			// Выполняем очистку списка локальных IPv4 адресов из кэша
			this->clearCache(AF_INET, true);
			// Выполняем очистку списка локальных IPv6 адресов из кэша
			this->clearCache(AF_INET6, true);
			// Выполняем чтение содержимого файла
			fs.readFile(hosts, [this](const string & entry) noexcept -> void {
				// Если запись не является комментарием
				if(!entry.empty() && (entry.size() > 1) && (entry.front() != '#')){
					// Хост который будем собирать
					string host = "";
					// Список полученных хостов
					vector <string> hosts;
					// Выполняем перебор всех полученных символов
					for(size_t i = 0; i < entry.size(); i++){
						// Выполняем получение текущего символа
						const uint8_t c = entry.at(i);
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
						hosts.push_back(::move(host));
					// Если количество хостов в списке больше одного
					if(hosts.size() > 1){
						// Тип интернет-протокола AF_INET, AF_INET6
						int32_t family = 0;
						/**
						 * Определяем тип передаваемого сервера
						 */
						switch(static_cast <uint8_t> (this->_net.host(hosts.front()))){
							// Если хост является доменом или IPv4 адресом
							case static_cast <uint8_t> (net_t::type_t::IPV4):
								// Устанавливаем семейстов IP-адресов
								family = AF_INET;
							break;
							// Если хост является IPv6 адресом, переводим ip адрес в полную форму
							case static_cast <uint8_t> (net_t::type_t::IPV6):
								// Устанавливаем семейстов IP-адресов
								family = AF_INET6;
							break;
						}
						// Если мы удачно определили тип интернет-протокола
						if(family > 0){
							// Выполняем перебор всего списка хостов
							for(size_t i = 1; i < hosts.size(); i++)
								// Выполняем добавление в кэш новый IP-адрес
								this->setToCache(family, hosts.at(i), hosts.front(), 4294967295, true);
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
 * @brief Метод определение локального IP-адреса по имени домена
 *
 * @param name название сервера
 * @return     полученный IP-адрес
 */
string awh::DNS::host(const string & name) noexcept {
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
 * @brief Метод определение локального IP-адреса по имени домена
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param name   название сервера
 * @return       полученный IP-адрес
 */
string awh::DNS::host(const int32_t family, const string & name) noexcept {
	// Результат работы функции
	string result = "";
	// Создаём объект холдирования
	hold_t <status_t> hold(this->_status);
	// Если статус работы DNS-резолвера соответствует
	if(hold.access({}, status_t::RESOLVE)){
		// Если домен передан
		if(!name.empty()){
			// Если доменное имя является локальным
			if(this->_fmk->is(name, fmk_t::check_t::LATIAN)){
				// Выполняем блокировку потока
				const lock_guard <std::recursive_mutex> lock(this->_mtx);
				// Переводим доменное имя в нижний регистр
				this->_fmk->transform(name, fmk_t::transform_t::LOWER);
				{
					// Создаём структуру запроса
					struct addrinfo hints;
					// Выполняем зануление структуры запроса
					::memset(&hints, 0, sizeof(hints));
					// Устанавливаем семейстов IP-адресов
					hints.ai_family = family;
					// Устанавливаем тип сокета
					hints.ai_socktype = SOCK_STREAM;
					// Создаём объект результата
					struct addrinfo * response = nullptr;
					// Выполняем резолвинг доменного имени или хоста
					const int32_t status = ::getaddrinfo(name.c_str(), nullptr, &hints, &response);
					// Если запрос не выполнен
					if(status != 0){
						// Выводим сообщение об ошибке
						this->_log->print("%s for %s", log_t::flag_t::WARNING, ::gai_strerror(status), name.c_str());
						// Выходим из функции
						return result;
					// Если запрос выполнен удачно
					} else {
						// Список полученных IP-адресов
						vector <string> ips;
						// Создаём объект пира
						struct addrinfo * peer = nullptr;
						/**
						 * Определяем тип передаваемого IP-адреса
						 */
						switch(family){
							// Если IP-адрес является IPv4 адресом
							case static_cast <int32_t> (AF_INET): {
								// Выполняем перебор всех полученных IP-адресов
								for(peer = response; peer != nullptr; peer = peer->ai_next) {
									// Выполняем извлечение IP-адреса
									struct sockaddr_in * ip = reinterpret_cast <struct sockaddr_in *> (peer->ai_addr);
									// Извлекаем значение адреса
									void * addr = &(ip->sin_addr);
									// Выполняем очистку буфера данных
									this->_buffer.clear(buffer_t::type_t::ADDR, family);
									// Получаем размер буфера данных
									const size_t size = this->_buffer.size(buffer_t::type_t::ADDR, family);
									// Копируем полученные данные IP-адреса
									ips.push_back(::inet_ntop(family, addr, reinterpret_cast <char *> (this->_buffer.get(buffer_t::type_t::ADDR)), size));
								}
							} break;
							// Если IP-адрес является IPv6 адресом
							case static_cast <int32_t> (AF_INET6): {
								// Выполняем перебор всех полученных IP-адресов
								for(peer = response; peer != nullptr; peer = peer->ai_next) {
									// Выполняем извлечение IP-адреса
									struct sockaddr_in6 * ip = reinterpret_cast <struct sockaddr_in6 *> (peer->ai_addr);
									// Извлекаем значение адреса
									void * addr = &(ip->sin6_addr);
									// Выполняем очистку буфера данных
									this->_buffer.clear(buffer_t::type_t::ADDR, family);
									// Получаем размер буфера данных
									const size_t size = this->_buffer.size(buffer_t::type_t::ADDR, family);
									// Копируем полученные данные IP-адреса
									ips.push_back(::inet_ntop(family, addr, reinterpret_cast <char *> (this->_buffer.get(buffer_t::type_t::ADDR)), size));
								}
							} break;
						}
						// Очищаем объект запроса
						::freeaddrinfo(response);
						// Если список IP-адресов получен
						if(!ips.empty()){
							/**
							 * Выполняем отлов ошибок
							 */
							try {
								// Подключаем устройство генератора
								mt19937 generator(this->_randev());
								// Выполняем генерирование случайного числа
								uniform_int_distribution <mt19937::result_type> dist6(0, ips.size() - 1);
								// Выполняем получение результата
								result = ::move(ips.at(::move(dist6(generator))));
							/**
							 * Если возникает ошибка
							 */
							} catch(const runtime_error & error) {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(family, name), log_t::flag_t::WARNING, error.what());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::WARNING, error.what());
								#endif
								// Выполняем извлечение первого адреса из списка
								result = ips.front();
							}
						}
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод ресолвинга домена
 *
 * @param host хост сервера
 * @return     полученный IP-адрес
 */
string awh::DNS::resolve(const string & host) noexcept {
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
 * @brief Метод ресолвинга домена
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param host   хост сервера
 * @return       полученный IP-адрес
 */
string awh::DNS::resolve(const int32_t family, const string & host) noexcept {
	// Результат работы функции
	string result = "";
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
			/**
			 * Определяем тип передаваемого сервера
			 */
			switch(static_cast <uint8_t> (this->_net.host(domain))){
				// Если домен является IPv4-адресом
				case static_cast <uint8_t> (net_t::type_t::IPV4):
				// Если домен является IPv6-адресом
				case static_cast <uint8_t> (net_t::type_t::IPV6):
					// Выводим переданый хост обратно
					return host;
				// Если домен является адресом в файловой системе
				case static_cast <uint8_t> (net_t::type_t::FS):
				// Если домен является аппаратным адресом сетевого интерфейса
				case static_cast <uint8_t> (net_t::type_t::MAC):
				// Если домен является URL-адресом
				case static_cast <uint8_t> (net_t::type_t::URL):
				// Если домен является адресом/Маски сети
				case static_cast <uint8_t> (net_t::type_t::NETWORK):
					// Сообщаем, что адрес нам не подходит
					return result;
				// Если домен является доменной зоной
				case static_cast <uint8_t> (net_t::type_t::FQDN): {
					// Выполняем поиск IP-адреса в кэше DNS
					result = this->cache(family, domain);
					// Если IP-адрес получен
					if(!result.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим заголовок запроса
							std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ DOMAIN RESOLVE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
							// Выводим параметры запроса
							std::cout << domain << std::endl << std::flush;
							/**
							 * Определяем тип протокола подключения
							 */
							switch(family){
								// Если тип протокола подключения IPv4
								case static_cast <int32_t> (AF_INET):
									// Выводим информацию об IP-адресе
									std::cout << "IPv4: " << result << std::endl << std::flush;
								break;
								// Если тип протокола подключения IPv6
								case static_cast <int32_t> (AF_INET6):
									// Выводим информацию об IP-адресе
									std::cout << "IPv6: " << result << std::endl << std::flush;
								break;
							}
							// Выводим переход на новую строку
							std::cout << std::endl << std::flush;
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
							/**
							 * Определяем тип протокола подключения
							 */
							switch(family){
								// Если тип протокола подключения IPv4
								case static_cast <int32_t> (AF_INET): {
									// Получаем значение переменной
									const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV4_%s", this->_prefix.c_str(), postfix.c_str()).c_str());
									// Если IP-адрес из переменной окружения получен
									if(env != nullptr)
										// Выводим полученный результат
										return env;
								} break;
								// Если тип протокола подключения IPv6
								case static_cast <int32_t> (AF_INET6): {
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
						/**
						 * Определяем тип протокола подключения
						 */
						switch(family){
							// Если тип протокола подключения IPv4
							case static_cast <int32_t> (AF_INET): {
								// Если список DNS-серверов пустой
								if(this->_serversIPv4.empty())
									// Устанавливаем список серверов IPv4
									this->replace(AF_INET);
								// Выполняем получение IP-адреса
								result = this->_workerIPv4->request(domain);
							} break;
							// Если тип протокола подключения IPv6
							case static_cast <int32_t> (AF_INET6): {
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
					// Выполняем поиск IP-адреса в кэше DNS
					result = this->cache(family, domain);
					// Если IP-адрес получен
					if(!result.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим заголовок запроса
							std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ DOMAIN RESOLVE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
							// Выводим параметры запроса
							std::cout << domain << std::endl << std::flush;
							/**
							 * Определяем тип протокола подключения
							 */
							switch(family){
								// Если тип протокола подключения IPv4
								case static_cast <int32_t> (AF_INET):
									// Выводим информацию об IP-адресе
									std::cout << "IPv4: " << result << std::endl << std::flush;
								break;
								// Если тип протокола подключения IPv6
								case static_cast <int32_t> (AF_INET6):
									// Выводим информацию об IP-адресе
									std::cout << "IPv6: " << result << std::endl << std::flush;
								break;
							}
							// Выводим переход на новую строку
							std::cout << std::endl << std::flush;
						#endif
						// Выводим полученный результат
						return result;
					// Если в кэше доменного имени нету
					} else {
						// Создаём объект DNS-резолвера
						dns_t dns(this->_fmk, this->_log);
						// Выполняем получение IP адрес хоста доменного имени
						return dns.host(family, host);
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param ip адрес для поиска доменного имени
 * @return   список найденных доменных имён
 */
vector <string> awh::DNS::search(const string & ip) noexcept {
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Определяем тип передаваемого IP-адреса
		 */
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
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param ip     адрес для поиска доменного имени
 * @return       список найденных доменных имён
 */
vector <string> awh::DNS::search(const int32_t family, const string & ip) noexcept {
	// Результат работы функции
	vector <string> result;
	// Если IP-адрес передан
	if(!ip.empty()){
		/**
		 * Выполняем блокировку потока на время чтения кэша
		 */
		std::unique_lock <std::recursive_mutex> lock(this->_mtx);
		/**
		 * Определяем тип протокола подключения
		 */
		switch(family){
			// Если тип протокола подключения IPv4
			case static_cast <int32_t> (AF_INET): {
				// Переходим по всему списку IP-адресов
				for(auto i = this->_cacheIPv4.begin(); i != this->_cacheIPv4.end(); ++i){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[1];
					// Выполняем копирование полученных данных в переданный буфер
					::inet_pton(family, ip.c_str(), &buffer);
					// Если IP-адрес соответствует переданному адресу
					if(::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0)
						// Выполняем добавление доменное имя в список
						result.push_back(i->first);
				}
			} break;
			// Если тип протокола подключения IPv6
			case static_cast <int32_t> (AF_INET6): {
				// Переходим по всему списку IP-адресов
				for(auto i = this->_cacheIPv6.begin(); i != this->_cacheIPv6.end(); ++i){
					// Создаём буфер бинарных данных IP-адреса
					uint32_t buffer[4];
					// Выполняем копирование полученных данных в переданный буфер
					::inet_pton(family, ip.c_str(), &buffer);
					// Если IP-адрес соответствует переданному адресу
					if(::memcmp(i->second.ip, buffer, sizeof(buffer)) == 0)
						// Выполняем добавление доменное имя в список
						result.push_back(i->first);
				}
			} break;
		}
		// Снимаем блокировку потока, запрос к DNS-серверу выполняется без неё
		lock.unlock();
		// Если список IP-адресов пустой
		if(result.empty()){
			// Устанавливаем полученный IP-адрес
			this->_net = ip;
			// Получаем доменное имя в виде ARPA-записи
			const string & domain = this->_net.arpa();
			/**
			 * Определяем тип протокола подключения
			 */
			switch(family){
				// Если тип протокола подключения IPv4
				case static_cast <int32_t> (AF_INET): {
					// Если список DNS-серверов пустой
					if(this->_serversIPv4.empty())
						// Устанавливаем список серверов IPv4
						this->replace(AF_INET);
					// Устанавливаем тип DNS-запроса
					this->_workerIPv4->_qtype = worker_t::q_type_t::PTR;
					/**
					 * PTR-записи в прямой кэш не попадают, поэтому результат берём из списка
					 * полученных воркером PTR-записей, а не повторным поиском в кэше
					 */
					if(!this->_workerIPv4->request(domain).empty())
						// Выполняем получение списка PTR-записей
						result = this->_workerIPv4->_ptr;
					// Возвращаем тип DNS-запроса, иначе следующие запросы IP-адреса уйдут как PTR-запросы
					this->_workerIPv4->_qtype = worker_t::q_type_t::IP;
				} break;
				// Если тип протокола подключения IPv6
				case static_cast <int32_t> (AF_INET6): {
					// Если список DNS-серверов пустой
					if(this->_serversIPv6.empty())
						// Устанавливаем список серверов IPv6
						this->replace(AF_INET6);
					// Устанавливаем тип DNS-запроса
					this->_workerIPv6->_qtype = worker_t::q_type_t::PTR;
					/**
					 * PTR-записи в прямой кэш не попадают, поэтому результат берём из списка
					 * полученных воркером PTR-записей, а не повторным поиском в кэше
					 */
					if(!this->_workerIPv6->request(domain).empty())
						// Выполняем получение списка PTR-записей
						result = this->_workerIPv6->_ptr;
					// Возвращаем тип DNS-запроса, иначе следующие запросы IP-адреса уйдут как PTR-запросы
					this->_workerIPv6->_qtype = worker_t::q_type_t::IP;
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::DNS::DNS(const fmk_t * fmk, const log_t * log) noexcept :
 _net(log), _timeout(5), _prefix{AWH_SHORT_NAME},
 _workerIPv4(nullptr), _workerIPv6(nullptr), _fmk(fmk), _log(log) {
	// Выполняем создание воркера для IPv4
	this->_workerIPv4 = std::make_unique <worker_t> (AF_INET, this);
	// Выполняем создание воркера для IPv6
	this->_workerIPv6 = std::make_unique <worker_t> (AF_INET6, this);
}
/**
 * @brief Деструктор
 *
 */
awh::DNS::~DNS() noexcept {
	// Выполняем очистку модуля DNS-резолвера
	this->clear();
}
