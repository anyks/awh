/**
 * @file: socks5.cpp
 * @date: 2026-05-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <client/socks5.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Инкапсулируем статические параметры в пространство имён
 */
namespace {
	/**
	 * @brief Функция комбинирования хеш-кодов
	 *
	 * @param seed  исходный хеш-код
	 * @param value добавочный хеш-код
	 */
	void combine(size_t & seed, const size_t value) noexcept {
		// Комбинируем хеш-коды
		seed ^= (value + 0x9E3779B9 + (seed << 6) + (seed >> 2));
	}
};

/**
 * @brief Фабричный метод создания идентификатора инициатора запроса
 *
 * @param addr     объект параметров подключения инициатора запроса
 * @param protocol протокол инициатора запроса
 * @return         идентификатор инициатора запроса
 */
awh::Socks5::Origin & awh::Socks5::Origin::from(const net::attr_t * addr, const event::protocol_t protocol) noexcept {
	// Если объект параметров подключения инициатора запроса передан корректно
	if(addr != nullptr){
		// Устанавливаем тип адреса инициатора запроса
		this->type = addr->type;
		// Устанавливаем протокол инициатора запроса
		this->protocol = protocol;
		/**
		 * Определяем тип адреса хоста для подключения
		 */
		switch(static_cast <uint8_t> (addr->type)){
			// Если тип адреса соответствует FQDN
			case static_cast <uint8_t> (net::type_t::FQDN): {
				// Извлекаем доменное имя хоста для подключения
				const string & fqdn = awh_cast <const net::attr_fqdn_t *> (addr)->domain;
				// Определяем длину доменного имени хоста для подключения, ограничивая её размером буфера для хранения доменного имени
				const size_t length = ::min(fqdn.length(), sizeof(this->fqdn.data) - 1);
				// Копируем доменное имя хоста для подключения
				::memcpy(this->fqdn.data, &fqdn[0], length);
				// Устанавливаем завершающий нулевой символ
				this->fqdn.data[length] = '\0';
				// Устанавливаем порт хоста для подключения
				this->fqdn.port = htons(awh_cast <const net::attr_fqdn_t *> (addr)->port);
			} break;
			// Если тип адреса соответствует IPv4
			case static_cast <uint8_t> (net::type_t::IPV4): {
				// Устанавливаем порт
				this->ip4.port = htons(awh_cast <const net::attr_net_t *> (addr)->port);
				// Устанавливаем адрес инициатора запроса
				this->ip4.address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (addr)->ip.get())->address;
			} break;
			// Если тип адреса соответствует IPv6
			case static_cast <uint8_t> (net::type_t::IPV6): {
				// Устанавливаем порт
				this->ip6.port = htons(awh_cast <const net::attr_net_t *> (addr)->port);
				// Устанавливаем адрес инициатора запроса
				::memcpy(&this->ip6.address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (addr)->ip.get())->address[0], 16);
			} break;
		}
	}
	// Возвращаем идентификатор инициатора запроса
	return (* this);
}
/**
 * @brief Оператор сравнения
 *
 * @param other другой объект для сравнения
 * @return      результат сравнения
 */
bool awh::Socks5::Origin::operator == (const Origin & other) const noexcept {
	// Сравниваем семейство адресов
	if((this->type != other.type) || (this->protocol != other.protocol))
		// Выводим отрицательный результат
		return false;
	/**
	 * Сравниваем данные в зависимости от семейства адресов
	 */
	switch(static_cast <uint8_t> (this->type)){
		// Если тип адреса соответствует FQDN
		case static_cast <uint8_t> (net::type_t::FQDN):
			// Выполняем сравнение FQDN адресов
			return (
				(this->fqdn.port == other.fqdn.port) &&
				(::strcmp(this->fqdn.data, other.fqdn.data) == 0)
			);
		// Если адрес установлен как IPv4
		case static_cast <uint8_t> (net::type_t::IPV4):
			// Выполняем сравнение IPv4 адресов
			return (
				(this->ip4.port == other.ip4.port) &&
				(this->ip4.address == other.ip4.address)
			);
		// Если адрес установлен как IPv6
		case static_cast <uint8_t> (net::type_t::IPV6):
			// Выполняем сравнение IPv6 адресов
			return (
				(this->ip6.port == other.ip6.port) &&
				(this->ip6.address == other.ip6.address)
			);
		// В остальных случаях выводим отрицательный результат
		default: return false;
	}
}
/**
 * @brief Конструктор
 *
 */
awh::Socks5::Origin::Origin() noexcept :
 type(net::type_t::NONE),
 protocol(event::protocol_t::NONE) {};

/**
 * @brief Оператор вычисления хеш-кода
 *
 * @param id объект для вычисления хеш-кода
 * @return   хеш-код объекта
 */
size_t awh::Socks5::Origin_Hash::operator()(const origin_t & id) const noexcept {
	// Вычисляем начальный хеш-код по семейству адресов
	size_t result = hash <uint8_t> {}(static_cast <uint8_t> (id.type));
	// Комбинируем хеш-код протокола
	::combine(result, hash <uint16_t> {}(static_cast <uint8_t> (id.protocol)));
	/**
	 * Сравниваем данные в зависимости от семейства адресов
	 */
	switch(static_cast <uint8_t> (id.type)){
		// Если тип адреса соответствует FQDN
		case static_cast <uint8_t> (net::type_t::FQDN): {
			// Безопасное чтение 108 байт пути сокета как массива uint64_t
			const uint64_t * words = reinterpret_cast <const uint64_t *> (id.fqdn.data);
			// Хэш по всем 255 байтам, включая нули
			for(uint8_t i = 0; i < static_cast <uint8_t> (sizeof(id.fqdn.data) / sizeof(uint64_t)); ++i)
				// Комбинируем хеш-код части пути сокета
				::combine(result, hash <uint64_t> {}(words[i]));
		} break;
		// Если адрес установлен как IPv4
		case static_cast <uint8_t> (net::type_t::IPV4): {
			// Комбинируем хеш-код порта
			::combine(result, hash <uint16_t> {}(id.ip4.port));
			// Комбинируем хеш-код IPv4 адреса
			::combine(result, hash <uint32_t> {}(id.ip4.address));
		} break;
		// Если адрес установлен как IPv6
		case static_cast <uint8_t> (net::type_t::IPV6): {
			// Безопасное чтение 128-битного адреса как двух uint64_t
			const uint64_t hi = (* reinterpret_cast <const uint64_t *> (&id.ip6.address[0]));
			const uint64_t lo = (* reinterpret_cast <const uint64_t *> (&id.ip6.address[0] + 8));
			// Комбинируем хеш-коды IPv6 адреса
			::combine(result, hash <uint64_t> {}(hi));
			::combine(result, hash <uint64_t> {}(lo));
			// Комбинируем хеш-код порта
			::combine(result, hash <uint16_t> {}(id.ip6.port));
		} break;
	}
	// Возвращаем хеш-код
	return result;
}

/**
 * @brief Метод очистки активных сессий клиентов, работающих через прокси
 *
 */
void awh::Socks5::clearSessions() noexcept {
	// Выполняем блокировку потока для работы с TLS
	const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Очищаем активные сессии клиентов, работающих через прокси
	this->_sessions.clear();
}
/**
 * @brief Метод получения направления работы socks5 прокси
 *
 * @return направление работы socks5 прокси
 */
awh::Socks5::route_t awh::Socks5::getRoute() const noexcept {
	// Выполняем блокировку потока для работы с TLS
	const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
	// Возвращаем направление работы socks5 прокси
	return this->_route;
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::Socks5::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для объекта блокировки
	this->_mtx.enabled = mode;
	// Устанавливаем режим безопасности работы потоков для объекта клиента
	client_t::threadSafety(mode);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::Socks5::callback(const callback_t & callback) noexcept {

}
/**
 * @brief Метод отправки данных серверу
 *
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных серверу
 */
size_t awh::Socks5::send(const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Идентификатор события клиента для отправки данных
		event::id_t eid = 0;
		{
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Если есть активные сессии клиентов, работающих через прокси
			if(!this->_sessions.empty())
				// Получаем идентификатор события клиента для первой активной сессии
				eid = this->_sessions.begin()->second;
		}
		// Если идентификатор события клиента для отправки данных получен
		if(eid > 0){
			// Если идентификатор TLS и объект TLS установлены
			if((this->_tid > 0) && (this->_coder != nullptr)){
				// Если шифрование данных TLS выполнено успешно
				if(this->_coder->encrypt(this->_tid, buffer, size))
					// Возвращаем размер отправленных данных
					return size;
				// Выводим результат по умолчанию
				return 0;
			}
			// Выполняем отправку данных серверу
			return this->_client->send(eid, buffer, size);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод получения данных от сервера
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 */
bool awh::Socks5::recv(const event::id_t eid) noexcept {
	// Если идентификатор клиента передан корректно
	if(eid > 0)
		// Получаем данные от сервера
		return this->_client->recv(eid);
	// Если идентификатор клиента не передан корректно
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных клиенту
 *
 * @param eid    идентификатор события клиента
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных клиенту
 */
size_t awh::Socks5::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если идентификатор TLS и объект TLS установлены
	if((this->_tid > 0) && (this->_coder != nullptr)){
		// Если шифрование данных TLS выполнено успешно
		if(this->_coder->encrypt(this->_tid, buffer, size))
			// Возвращаем размер отправленных данных
			return size;
		// Выводим результат по умолчанию
		return 0;
	}
	// Выполняем отправку данных серверу
	return this->_client->send(eid, buffer, size);
}
/**
 * @brief Метод установки параметров авторизации
 *
 * @param username имя пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Socks5::setUser(const string & username, const string & password) noexcept {
	// Выполняем блокировку потока для работы с TLS
	const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Устанавливаем параметры авторизации для объекта клиента
	this->_socks5.setUser(username, password);
}
/**
 * @brief Метод добавления идентификатора события клиента для конечной точки
 *
 * @param eid идентификатор события для добавления
 * @return    результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::addEventIdEndpoint(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём объект адреса назначения подключения для идентификатора события клиента
		unique_ptr <net::addr_t> target = nullptr;
		// Получаем адрес хоста целевой машины для идентификатора события клиента
		if(this->_client->getTarget(eid, target)){
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
			// Устанавливаем полученный IP-адрес
			awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(target);
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = this->_client->getPort(eid);
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = Origin().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Добавляем идентификатор события клиента для конечной точки
			return this->_sessions.emplace(endpoint, eid).second;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления идентификатора события клиента для конечной точки
 *
 * @param eid  идентификатор события для добавления
 * @param addr адрес хоста для добавления
 * @param port порт хоста для добавления
 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::addEventIdEndpoint(const event::id_t eid, string_view addr, const uint16_t port) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события клиента для конечной точки получен и адрес хоста для добавления не пустой
		if((eid > 0) && !addr.empty()){
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = nullptr;
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_addr.host(addr))){
				// Для типа FQDN
				case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_fqdn_t> ();
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (attr.get())->domain = addr;
				} break;
				// Для типа IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
				// Для типа IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если объект параметров подключения для идентификатора события клиента создан
			if(attr != nullptr){
				// Устанавливаем полученный порт
				awh_cast <net::attr_net_t *> (attr.get())->port = port;
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
				// Добавляем идентификатор события клиента для конечной точки
				return this->_sessions.emplace(endpoint, eid).second;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод удаления идентификатора события клиента для конечной точки
 *
 * @param eid идентификатор события для удаления
 * @return    результат выполнения удаления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::delEventIdEndpoint(const event::id_t eid) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём объект адреса назначения подключения для идентификатора события клиента
		unique_ptr <net::addr_t> target = nullptr;
		// Получаем адрес хоста целевой машины для идентификатора события клиента
		if(this->_client->getTarget(eid, target)){
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
			// Устанавливаем полученный IP-адрес
			awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(target);
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = this->_client->getPort(eid);
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = Origin().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск идентификатора события клиента для конечной точки
			auto i = this->_sessions.find(endpoint);
			// Если идентификатор события клиента для конечной точки найден
			if((result = (i != this->_sessions.end())))
				// Удаляем идентификатор события клиента для конечной точки
				this->_sessions.erase(i);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод удаления идентификатора события клиента для конечной точки
 *
 * @param eid  идентификатор события для удаления
 * @param addr адрес хоста для удаления
 * @param port порт хоста для удаления
 * @return     результат выполнения удаления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::delEventIdEndpoint(const event::id_t eid, string_view addr, const uint16_t port) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события клиента для конечной точки получен и адрес хоста для добавления не пустой
		if((eid > 0) && !addr.empty()){
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = nullptr;
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_addr.host(addr))){
				// Для типа FQDN
				case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_fqdn_t> ();
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (attr.get())->domain = addr;
				} break;
				// Для типа IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
				// Для типа IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если объект параметров подключения для идентификатора события клиента создан
			if(attr != nullptr){
				// Устанавливаем полученный порт
				awh_cast <net::attr_net_t *> (attr.get())->port = port;
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
				// Выполняем поиск идентификатора события клиента для конечной точки
				auto i = this->_sessions.find(endpoint);
				// Если идентификатор события клиента для конечной точки найден
				if((result = (i != this->_sessions.end())))
					// Удаляем идентификатор события клиента для конечной точки
					this->_sessions.erase(i);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки наличия идентификатора события клиента для конечной точки
 *
 * @param eid идентификатор события для проверки
 * @return    результат проверки наличия идентификатора события клиента для конечной точки
 */
bool awh::Socks5::isEventIdEndpoint(const event::id_t eid) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём объект адреса назначения подключения для идентификатора события клиента
		unique_ptr <net::addr_t> target = nullptr;
		// Получаем адрес хоста целевой машины для идентификатора события клиента
		if(this->_client->getTarget(eid, target)){
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
			// Устанавливаем полученный IP-адрес
			awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(target);
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = this->_client->getPort(eid);
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = Origin().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск идентификатора события клиента для конечной точки
			auto i = this->_sessions.find(endpoint);
			// Если идентификатор события клиента для конечной точки найден, устанавливаем результат
			result = (i != this->_sessions.end());
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки наличия идентификатора события клиента для конечной точки
 *
 * @param eid  идентификатор события для проверки
 * @param addr адрес хоста для проверки
 * @param port порт хоста для проверки
 * @return     результат проверки наличия идентификатора события клиента для конечной точки
 */
bool awh::Socks5::isEventIdEndpoint(const event::id_t eid, string_view addr, const uint16_t port) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события клиента для конечной точки получен и адрес хоста для добавления не пустой
		if((eid > 0) && !addr.empty()){
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = nullptr;
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_addr.host(addr))){
				// Для типа FQDN
				case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_fqdn_t> ();
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (attr.get())->domain = addr;
				} break;
				// Для типа IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
				// Для типа IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IP-адреса
					const_cast <socks5_t *> (this)->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если объект параметров подключения для идентификатора события клиента создан
			if(attr != nullptr){
				// Устанавливаем полученный порт
				awh_cast <net::attr_net_t *> (attr.get())->port = port;
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
				// Выполняем поиск идентификатора события клиента для конечной точки
				auto i = this->_sessions.find(endpoint);
				// Если идентификатор события клиента для конечной точки найден, устанавливаем результат
				result = (i != this->_sessions.end());
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param route  направление работы socks5 прокси
 * @param client объект юнита клиента
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Socks5::Socks5(const route_t route, unit::client_t * client, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, fmk, log), _route(route), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param route  направление работы socks5 прокси
 * @param client объект юнита клиента
 * @param coder  объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Socks5::Socks5(const route_t route, unit::client_t * client, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, coder, fmk, log), _route(route), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param route  направление работы socks5 прокси
 * @param client объект юнита клиента
 * @param dns    объект DNS-резолвера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Socks5::Socks5(const route_t route, unit::client_t * client, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept : 
 client_t(client, dns, fmk, log), _route(route), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param route  направление работы socks5 прокси
 * @param client объект юнита клиента
 * @param dns    объект DNS-резолвера
 * @param coder  объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Socks5::Socks5(const route_t route, unit::client_t * client, unit::dns_t * dns, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, dns, coder, fmk, log), _route(route), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Деструктор
 *
 */
awh::Socks5::~Socks5() noexcept {}
