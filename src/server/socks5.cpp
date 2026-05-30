/**
 * @file: socks5.cpp
 * @date: 2026-05-30
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
#include <server/socks5.hpp>

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
awh::server::Socks5::Origin & awh::server::Socks5::Origin::from(const net::attr_t * addr, const event::protocol_t protocol) noexcept {
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
bool awh::server::Socks5::Origin::operator == (const Origin & other) const noexcept {
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
awh::server::Socks5::Origin::Origin() noexcept :
 type(net::type_t::NONE),
 protocol(event::protocol_t::NONE) {};

/**
 * @brief Оператор вычисления хеш-кода
 *
 * @param id объект для вычисления хеш-кода
 * @return   хеш-код объекта
 */
size_t awh::server::Socks5::Origin_Hash::operator()(const origin_t & id) const noexcept {
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
 * @brief Метод обработки события разрешения подключения
 *
 * @param eid идентификатор сервера
 * @param cid идентификатор клиента
 */
void awh::server::Socks5::accept(const event::id_t eid, const event::id_t cid) noexcept {

}
/**
 * @brief Метод обработки событий получения данных сервером
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных сервера
 * @param size   размер данных сервера
 */
void awh::server::Socks5::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {

}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::server::Socks5::stop() noexcept {

}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::server::Socks5::start() noexcept {

}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::server::Socks5::threadSafety(const bool mode) noexcept {

}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::server::Socks5::callback(const callback_t & callback) noexcept {

}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
 *
 * @return результат выполнения установки
 */
bool awh::server::Socks5::membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для сервера SOCKS5)
 *
 * @return результат выполнения установки
 */
bool awh::server::Socks5::membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения приостановки работы
 */
bool awh::server::Socks5::pause(const event::id_t eid) noexcept {
	// И для пиров и для их клиентов
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 */
bool awh::server::Socks5::resume(const event::id_t eid) noexcept {
	// И для пиров и для их клиентов
}
/**
 * @brief Метод уничтожения события клиента
 *
 * @param eid идентификатор события клиента для уничтожения
 */
void awh::server::Socks5::destroy(const event::id_t eid) noexcept {
	// Уничтожаем событие клиента
}
/**
 * @brief Метод получения данных от клиента (заглушка для сервера SOCKS5)
 *
 * @return результат получения данных
 */
bool awh::server::Socks5::recv(const event::id_t) noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных клиенту (заглушка для сервера SOCKS5)
 *
 * @return количество байт данных, отправленных клиенту
 */
size_t awh::server::Socks5::send(const event::id_t, const void *, const size_t) noexcept {
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод перемещения данных между сервером и другим событием (заглушка для сервера SOCKS5)
 *
 * @return результат выполнения перемещения
 */
bool awh::server::Socks5::splice(const event::id_t, const event::id_t) noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения опций клиента
 *
 * @param eid идентификатор события клиента
 * @return    опции клиента
 */
uint16_t awh::server::Socks5::getOptions(const event::id_t eid) const noexcept {
	// Только для удалённых клиентов принадлежащих пиру
}
/**
 * @brief Метод установки опций клиента
 *
 * @param eid     идентификатор события клиента
 * @param options опции клиента для установки
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Только для удалённых клиентов принадлежащих пиру
}
/**
 * @brief Метод установки опции клиента
 *
 * @param eid    идентификатор события клиента
 * @param option опция клиента для установки
 * @param mode   режим установки опции клиента
 * @return       результат выполнения установки
 */
bool awh::server::Socks5::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Только для удалённых клиентов принадлежащих пиру
}
/**
 * @brief Метод получения сетевого интерфейса для подключения к сети клиентов
 *
 * @return сетевой интерфейс сервера
 */
string awh::server::Socks5::getIface() const noexcept {
	// Только для клиентов принадлежащих пиру
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @param eid идентификатор события сервера
 * @return    сетевой интерфейс сервера
 */
string awh::server::Socks5::getIface(const event::id_t eid) const noexcept {
	// Только для серверов и удаленных клиентов принадлежащих пиру
}
/**
 * @brief Метод установки сетевого интерфейса для подключения к сети клиентов
 *
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setIface(string_view name) noexcept {
	// Только для клиентов принадлежащих пиру
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param eid  идентификатор события сервера
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setIface(const event::id_t eid, string_view name) noexcept {
	// Только для серверов и удаленных клиентов принадлежащих пиру
}
/**
 * @brief Метод получения порта удаленного клиента или текущего сервера
 *
 * @param eid идентификатор события клиента или сервера
 * @return    порт удаленного клиента или текущего сервера
 */
uint16_t awh::server::Socks5::getPort(const event::id_t eid) const noexcept {
	// Для серверов и удаленных клиентов принадлежащих пиру
}
/**
 * @brief Метод установки порта сервера
 *
 * @param eid  идентификатор события сервера
 * @param port порт сервера для установки
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setPort(const event::id_t eid, const uint16_t port) noexcept {
	// Только для серверов
}
/**
 * @brief Метод получения внутреннего порта события
 *
 * @param eid идентификатор события клиента
 * @return    внутренний порт события
 */
uint16_t awh::server::Socks5::getInternalPort(const event::id_t eid) const noexcept {
	// Порт пира с которым он подключился
}
/**
 * @brief Метод получения адреса хоста текущей машины
 *
 * @param eid идентификатор события сервера
 * @return    адрес хоста текущей машины
 */
string awh::server::Socks5::getHost(const event::id_t eid) const noexcept {
	// Только для адреса серверов
}
/**
 * @brief Метод установки адреса хоста текущей машины
 *
 * @param eid  идентификатор события сервера
 * @param host адрес хоста текущей машины
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setHost(const event::id_t eid, string_view host) noexcept {
	// Только для адреса серверов
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события клиента
 * @return    адрес хоста целевой машины
 */
string awh::server::Socks5::getTarget(const event::id_t eid) const noexcept {
	// Только для адреса клиентов принадлежащих пиру
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 */
bool awh::server::Socks5::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	// Только для адреса клиентов принадлежащих пиру
}
/**
 * @brief Метод установки адреса для подключения к сети клиентов
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::address_t address, string_view value) noexcept {
	// Только для клиентов принадлежащих пиру
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Только для адреса серверов и удаленных клиентов принадлежащих пиру
}
/**
 * @brief Метод установки адреса для подключения к сети клиентов
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::address_t address, const net::addr_t * value) noexcept {
	// Только для клиентов принадлежащих пиру
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Только для адреса серверов и удаленных клиентов принадлежащих пиру
}
/**
 * @brief Метод получения адреса для подключения к сети клиентов
 *
 * @param address тип адреса клиента или сервера
 * @return        значение адреса клиента или сервера
 */
string awh::server::Socks5::getAddress(const event::address_t address) const noexcept {
	// Только для клиентов принадлежащих пиру
}
/**
 * @brief Метод получения адреса клиента или текущего сервера
 *
 * @param eid     идентификатор события клиента или сервера
 * @param address тип адреса клиента или сервера
 * @return        значение адреса клиента или сервера
 */
string awh::server::Socks5::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Только для адреса самого пира (не удалённого клиента) а также адресов серверов
}
/**
 * @brief Метод получения адреса для подключения к сети клиентов
 *
 * @param address тип адреса клиента или сервера
 * @param value   объект для извлечения адреса клиента или сервера
 * @return        результат выполнения извлечения адреса клиента или сервера
 */
bool awh::server::Socks5::getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Только для клиентов принадлежащих пиру
}
/**
 * @brief Метод получения адреса клиента или текущего сервера
 *
 * @param eid     идентификатор события клиента или сервера
 * @param address тип адреса клиента или сервера
 * @param value   объект для извлечения адреса клиента или сервера
 * @return        результат выполнения извлечения адреса клиента или сервера
 */
bool awh::server::Socks5::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Только для адреса самого пира (не удалённого клиента) а также адресов серверов
}
/**
 * @brief Метод получения размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       размер буфера клиента
 */
size_t awh::server::Socks5::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем размер буфера для клиента принадлежащего этому пиру
			return this->_server->getBufferSize(i->second.eid, action);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @param size   размер буфера клиента
 * @return       результат выполнения установки
 */
bool awh::server::Socks5::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_server->setBufferSize(eid, action, size))
			// Устанавливаем размер буфера для клиента принадлежащего этому пиру
			return this->_server->setBufferSize(i->second.eid, action, size);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения режима использования таймаута на чтение события клиента
 *
 * @param eid идентификатор события клиента
 * @return    режим использования таймаута на чтение события клиента
 */
awh::event::usage_t awh::server::Socks5::getUsageReadTimeout(const event::id_t eid) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем режим использования таймаута на чтение для клиента принадлежащего этому пиру
			return this->_server->getUsageReadTimeout(i->second.eid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события клиента
 *
 * @param eid   идентификатор события клиента
 * @param usage режим использования таймаута на чтение события клиента (reusable или disposable)
 */
void awh::server::Socks5::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем режим использования таймаута на чтение для клиента принадлежащего этому пиру
			return this->_server->setUsageReadTimeout(i->second.eid, usage);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (usage)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения таймаута клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       значение таймаута в миллисекундах
 */
uint32_t awh::server::Socks5::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем таймаут для клиента принадлежащего этому пиру
			return this->_server->getTimeout(i->second.eid, action);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки таймаута клиента
 *
 * @param eid     идентификатор события клиента
 * @param action  тип действия клиента
 * @param timeout значение таймаута в миллисекундах
 */
void awh::server::Socks5::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем таймаут для клиента принадлежащего этому пиру
			return this->_server->setTimeout(i->second.eid, action, timeout);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), timeout), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param eid       идентификатор события клиента
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 */
bool awh::server::Socks5::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_server->bandwidth(eid, limiting, bandwidth))
			// Устанавливаем пропускную способность для клиента принадлежащего этому пиру
			return this->_server->bandwidth(i->second.eid, limiting, bandwidth);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки параметров keep-alive для клиента
 *
 * @param eid   идентификатор события клиента
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::server::Socks5::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_server->keepAlive(eid, cnt, idle, intvl))
			// Устанавливаем параметры жизни подключения для клиента принадлежащего этому пиру
			return this->_server->keepAlive(i->second.eid, cnt, idle, intvl);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, cnt, idle, intvl), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки идентификатора события сервера
 *
 * @param eid идентификатор события сервера для установки
 */
void awh::server::Socks5::setEventId(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если DNS-резолвер или сервер находятся в нерабочем состоянии
		if(this->_dns != nullptr ? !this->_dns->working() : !this->_server->working()){
			/**
			 * Определяем протокол протокол клиента
			 */
			switch(static_cast <uint8_t> (this->_server->protocol(eid))){
				// Если протокол соответствует UDP
				case static_cast <uint8_t> (event::protocol_t::UDP):
					// Добавляем идентификатор события для сервера в список активных UDP серверов
					this->_servers.emplace(eid);
				break;
				// Если протокол соответствует TCP
				case static_cast <uint8_t> (event::protocol_t::TCP):
					// Устанавливаем идентификатор события для сервера
					this->_eid = eid;
				break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Конструктор
 *
 * @param server объект юнита сервера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::server::Socks5::Socks5(unit::server_t * server, const fmk_t * fmk, const log_t * log) noexcept :
 server_t(server, fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param server объект юнита сервера
 * @param dns    объект DNS-резолвера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::server::Socks5::Socks5(unit::server_t * server, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 server_t(server, dns, fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
* @brief Деструктор
*
*/
awh::server::Socks5::~Socks5() noexcept {}
