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
 * Если размер MTU для UDP сообщений в IPv4 не определён
 */
#ifndef AWH_MTU_UDP_IPV4_PAYLOAD_SIZE
	/**
	 * Устанавливаем размер MTU для UDP сообщений в 1500 байт
	 * Стандартный размер Ethernet MTU минус заголовки IP и UDP = 1472 - 72 = 1400 байт, с запасом на возможную инкапсуляцию
	 *
	 * 1500 - 20 (IP) - 8 (UDP) = 1472 максимум без фрагментации
	 * Запас 72 байта на возможную инкапсуляцию (туннели, провайдерские заголовки)
	 * Больше → фрагментация → потеря пакетов
	 */
	#define AWH_MTU_UDP_IPV4_PAYLOAD_SIZE 0x578
#endif

/**
 * Если размер MTU для UDP сообщений в IPv6 не определён
 */
#ifndef AWH_MTU_UDP_IPV6_PAYLOAD_SIZE
	/**
	 * Устанавливаем размер MTU для UDP сообщений в 1500 байт
	 * Стандартный размер Ethernet MTU минус заголовки IP и UDP = 1452 - 72 = 1380 байт, с запасом на возможную инкапсуляцию
	 *
	 * 1500 - 40 (IPv6) - 8 (UDP) = 1452 максимум без фрагментации
	 * Запас 72 байта на инкапсуляцию (туннели часто используют двойную инкапсуляцию)
	 * IPv6 не фрагментирует на маршрутизаторах → фрагментированные пакеты отбрасываются
	 */
	#define AWH_MTU_UDP_IPV6_PAYLOAD_SIZE 0x564
#endif

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

	/**
	 * @brief Размер данных в буфере
	 *
	 */
	thread_local size_t __awh_size__ = 0;

	/**
	 * @brief Буфер временного хранения данных UDP сообщений
	 *
	 */
	thread_local uint8_t __awh_buffer__[AWH_MTU_UDP_IPV4_PAYLOAD_SIZE] = {0};
};

/**
 * @brief Фабричный метод создания идентификатора инициатора запроса
 *
 * @param addr     объект параметров подключения инициатора запроса
 * @param protocol протокол инициатора запроса
 * @return         идентификатор инициатора запроса
 */
awh::client::Socks5::Origin & awh::client::Socks5::Origin::from(const net::attr_t * addr, const event::protocol_t protocol) noexcept {
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
bool awh::client::Socks5::Origin::operator == (const Origin & other) const noexcept {
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
awh::client::Socks5::Origin::Origin() noexcept :
 type(net::type_t::NONE),
 protocol(event::protocol_t::NONE) {};

/**
 * @brief Оператор вычисления хеш-кода
 *
 * @param id объект для вычисления хеш-кода
 * @return   хеш-код объекта
 */
size_t awh::client::Socks5::Origin_Hash::operator()(const origin_t & id) const noexcept {
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
 * @brief Метод изменения статуса клиента
 *
 * @param status новый статус клиента
 * @param state  новое временное состояние клиента
 */
void awh::client::Socks5::status(const event::status_t status, const state_t state) noexcept {
	/**
	 * Временное состояние клиента
	 */
	switch(static_cast <uint8_t> (state)){
		// Если мы получили статус события клиента
		case static_cast <uint8_t> (state_t::CLIENT): {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Выполняем подключение клиента к удалённому серверу
					if(!this->_client->connect(this->_eid)){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to connect to remote server", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to connect to remote server", log_t::flag_t::WARNING);
							#endif
						}
					// Если подключение выполнено
					} else {
						// Выполняем запуск работы клиента, если клиент не запущен
						if(!this->_client->launch(this->_eid)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::WARNING, this->_eid);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, this->_eid);
								#endif
							}
						// Если клиент запущен удачно
						} else {
							// Если функция обратного вызова установлена
							if(this->_callback.is("launch"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", this->_eid, this->_client->getTarget(this->_eid), this->_client->getPort(this->_eid));
						}
					}
				} break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED): {
					// Если функция обратного вызова установлена
					if(this->_callback.is("status"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("status", status);
				} break;
			}
		} break;
		// Если мы получили статус события DNS-резолвера
		case static_cast <uint8_t> (state_t::RESOLVER): {
			/**
			 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие DNS-резолвера запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Выполняем резолвинг доменного имени
					if(!this->_dns->resolve(this->_dns->issue(), awh_cast <unit::unit_t *> (this->_client)->family(this->_eid), this->_host, this->_timeoutDNS.load(std::memory_order_acquire))){
						// Создаём текст ошибки резолвинга доменного имени
						const string error = this->_fmk->format("It was not possible to obtain an IP address for the domain name \"%s\"", this->_host.c_str());
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->_eid), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						// Выполняем функцию обратного вызова
						} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", this->_eid, event::error_t::NOT_FOUND, error);
					}
				} break;
				// Если событие DNS-резолвера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Останавливаем клиента
					this->_client->stop();
				break;
			}
		} break;
	}
}
/**
 * @brief Метод обработки событий подключения клиента к удалённому серверу
 *
 * @param eid идентификатор клиента
 * @param ok  результат подключения
 */
void awh::client::Socks5::connect(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если идентификатор клиента соответствует идентификатору socks5 клиента
		if(eid == this->_eid){
			// Если подключение выполнено успешно
			if(ok){
				// Размер буфера данных
				size_t size = 0;
				// Буфер данных запроса
				uint8_t * buffer = nullptr;
				// Если извлечение буфера данных запроса выполнено успешно
				if(this->_socks5.buffer(&buffer, size, this->_ctx)){
					// Если отправка запроса на прокси-сервер не выполнена
					if(this->_client->send(eid, buffer, size) != size){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
							#endif
						}
					}
				}
			// Если подключение не выполнено
			} else {
				// Если функция обратного вызова установлена
				if(this->_callback.is("connect"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, false);
			}
		// Если функция обратного вызова установлена
		} else if(this->_callback.is("connect"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, ok);
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid  идентификатор клиента
 * @param size размер данных для записи
 */
void awh::client::Socks5::write(const event::id_t eid, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("write")){
			// Если текущее состояние соответствует завершённому состоянию
			if(this->_ctx.state == proto::client_socks5_t::state_t::COMPLETED){
				// Если сообщение дешифровано для socks5-клиента
				if(eid == this->_eid){
					// Идентификатор события клиента для конечной точки
					event::id_t eid = 0;
					{
						// Выполняем блокировку потока для работы с TLS
						const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
						// Если активные сессии клиентов, работающих через прокси, не пусты
						if(!this->_sessions.empty())
							// Извлекаем идентификатор события клиента для конечной точки
							eid = this->_sessions.begin()->second.first;
					}
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::id_t, const size_t)> ("write", this->_eid, eid, size);
				// Выполняем функцию обратного вызова
				} else this->_callback.call <void (const event::id_t, const event::id_t, const size_t)> ("write", this->_eid, eid, size);
			// Если текущее состояние не соответствует завершённому состоянию, выполняем функцию обратного вызова для socks5-клиента
			} else this->_callback.call <void (const event::id_t, const event::id_t, const size_t)> ("write", this->_eid, eid, size);
		}
	}
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param eid    идентификатор клиента
 * @param status новый статус клиента
 */
void awh::client::Socks5::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
		// Если статус клиента изменился на "уничтожен"
		if((eid == this->_eid) && (status == event::status_t::DESTROYED)){
			// Если объект DNS-резолвера установлен
			if(this->_dns != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns->stop();
			// Останавливаем событие клиента
			else this->_client->stop();
		}
	}
}
/**
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных клиента
 * @param size   размер данных клиента
 */
void awh::client::Socks5::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если текущее состояние соответствует завершённому состоянию
			if(this->_ctx.state == proto::client_socks5_t::state_t::COMPLETED){
				// Если идентификатор клиента соответствует идентификатору socks5 клиента
				if(eid == this->_eid){
					// Если объект транспортного уровня безопасности установлен
					if((this->_coder != nullptr) && (this->_tid > 0)){
						// Если данные не расшифрованы
						if(!this->_coder->decrypt(this->_tid, buffer, size)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error_tls")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("TLS decryption data is failed", log_t::flag_t::WARNING);
								#endif
							}
						}
					// Если объект транспортного уровня безопасности не установлен
					} else {
						// Если функция обратного вызова установлена
						if(this->_callback.is("read")){
							// Идентификатор события клиента для конечной точки
							event::id_t eid = 0;
							{
								// Выполняем блокировку потока для работы с TLS
								const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
								// Если активные сессии клиентов, работающих через прокси, не пусты
								if(!this->_sessions.empty())
									// Извлекаем идентификатор события клиента для конечной точки
									eid = this->_sessions.begin()->second.first;
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, eid, buffer, size);
						}
					}
				// Если идентификатор клиента не соответствует идентификатору socks5 клиента
				} else {
					// Инициализируем объект заголовка UDP пакета
					proto::client_socks5_t::udp_head_t udp{};
					// Если парсинг данных от прокси-сервера выполнен успешно
					if(this->_socks5.parse(buffer, size, udp)){
						// Если хост клиента которому адресован UDP пакет установлен
						if(udp.host != nullptr){
							// Идентификатор события клиента для конечной точки
							event::id_t eid = 0;
							// Идентификатор TLS для клиента
							tls::coder_t::id_t tid = 0;
							{
								// Создаём идентификатор конечной точки для идентификатора события клиента
								const origin_t endpoint = origin_t().from(udp.host.get(), event::protocol_t::UDP);
								// Выполняем блокировку потока для работы с TLS
								const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
								// Ищем идентификатор события клиента для конечной точки
								auto i = this->_sessions.find(endpoint);
								// Если идентификатор события клиента для конечной точки найден
								if(i != this->_sessions.end()){
									// Извлекаем идентификатор события клиента для конечной точки
									eid = i->second.first;
									// Извлекаем идентификатор TLS для клиента
									tid = i->second.second;
								}
							}
							// Если идентификатор события клиента для конечной точки получен успешно
							if(eid > 0){
								// Если объект транспортного уровня безопасности установлен
								if((this->_coder != nullptr) && (tid > 0)){
									// Если данные не расшифрованы
									if(!this->_coder->decrypt(tid, buffer + udp.size, size - udp.size)){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error_tls")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, make_tuple(this->_eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("TLS decryption data is failed", log_t::flag_t::WARNING);
											#endif
										}
									}
								// Если объект транспортного уровня безопасности не установлен
								} else {
									// Если функция обратного вызова установлена
									if(this->_callback.is("read"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, eid, buffer + udp.size, size - udp.size);
								}
								// Выходим из функции
								return;
							}
						}
					}
					// Создаём текст ошибки резолвинга доменного имени
					const string error = "Client for whom the UDP packet was received was not found";
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, error.c_str());
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
						#endif
					// Выполняем функцию обратного вызова
					} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::NOT_FOUND, error);
				}
			// Если текущее состояние находится ещё в процессе общения с socks5 прокси-сервером
			} else {
				// Если парсинг данных от прокси-сервера выполнен успешно
				if(this->_socks5.parse(buffer, size, this->_ctx)){
					/**
					 * Определяем состояние парсинга данных от прокси-сервера
					 */
					switch(static_cast <uint8_t> (this->_ctx.state)){
						// Если текущее состояние соответствует ошибке работе с прокси-сервером
						case static_cast <uint8_t> (proto::client_socks5_t::state_t::BROKEN): {
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status));
								#endif
							// Выполняем функцию обратного вызова
							} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, this->_socks5.statusMessage(this->_ctx.status));
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, false);
						} break;
						// Если текущее состояние соответствует ожиданию выполнения подключения
						case static_cast <uint8_t> (proto::client_socks5_t::state_t::CONNECT): {
							// Идентификатор события клиента
							event::id_t eid = 0;
							// Идентификатор инициатора запроса
							const origin_t * origin = nullptr;
							{
								// Выполняем блокировку потока для работы с TLS
								const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
								// Если активные сессии клиентов, работающих через прокси, не пусты
								if(!this->_sessions.empty()){
									// Извлекаем идентификатор события клиента, работающего через прокси
									origin = &this->_sessions.begin()->first;
									// Извлекаем идентификатор события клиента для конечной точки
									eid = this->_sessions.begin()->second.first;
								}
							}
							// Если идентификатор инициатора запроса получен успешно
							if(origin != nullptr){
								/**
								 * Определяем протокол сесии клиента, работающего через прокси
								 */
								switch(static_cast <uint8_t> (origin->protocol)){
									// Если протокол соответствует UDP
									case static_cast <uint8_t> (event::protocol_t::UDP): {
										// Устанавливаем команду для UDP протокола
										this->_ctx.command = proto::client_socks5_t::command_t::UDP;
										// Получаем порт клиента для подключения, работающего через прокси
										uint16_t port = this->_client->getInternalPort(eid);
										/**
										 * Определяем тип данных сесии клиента, работающего через прокси
										 */
										switch(static_cast <uint8_t> (origin->type)){
											// Если тип данных соответствует IPv4
											case static_cast <uint8_t> (net::type_t::IPV4): {
												// Выполняем инициализацию объекта хоста
												this->_ctx.host = make_unique <net::attr_net_t> ();
												// Устанавливаем тип адреса события
												this->_ctx.host->type = net::type_t::IPV4;
												// Устанавливаем внутренний IP-адрес клиента
												this->_client->getAddress(eid, event::address_t::IPV4, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip);
												// Если адрес клиента установлен а порт не установлен
												if((awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address > 0) && (port == 0)){
													// Получаем внутренний порт socks5-клиента
													port = this->_client->getInternalPort(this->_eid);
													// Устанавливаем внутренний порт клиента
													this->_client->setInternalPort(eid, port);
												}
											} break;
											// Если тип данных соответствует IPv6
											case static_cast <uint8_t> (net::type_t::IPV6): {
												// Выполняем инициализацию объекта хоста
												this->_ctx.host = make_unique <net::attr_net_t> ();
												// Устанавливаем тип адреса события
												this->_ctx.host->type = net::type_t::IPV6;
												// Устанавливаем внутренний IP-адрес клиента
												this->_client->getAddress(eid, event::address_t::IPV6, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip);
												// Если адрес клиента установлен а порт не установлен
												if((::memcmp(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address[0], (uint8_t[16]){0}, 16) != 0) && (port == 0)){
													// Получаем внутренний порт socks5-клиента
													port = this->_client->getInternalPort(this->_eid);
													// Устанавливаем внутренний порт клиента
													this->_client->setInternalPort(eid, port);
												}
											} break;
										}
										// Устанавливаем внутренний порт клиента
										awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = port;
									} break;
									// Если протокол соответствует TCP
									case static_cast <uint8_t> (event::protocol_t::TCP): {
										// Устанавливаем команду для TCP протокола
										this->_ctx.command = proto::client_socks5_t::command_t::CONNECT;
										/**
										 * Определяем тип данных сесии клиента, работающего через прокси
										 */
										switch(static_cast <uint8_t> (origin->type)){
											// Если тип данных соответствует FQDN
											case static_cast <uint8_t> (net::type_t::FQDN): {
												// Выполняем инициализацию объекта хоста
												this->_ctx.host = make_unique <net::attr_fqdn_t> ();
												// Устанавливаем тип адреса события
												this->_ctx.host->type = net::type_t::FQDN;
												// Устанавливаем порт хоста для подключения
												awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->port = ntohs(origin->fqdn.port);
												// Устанавливаем доменное имя хоста для подключения
												awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->domain = origin->fqdn.data;
											} break;
											// Если тип данных соответствует IPv4
											case static_cast <uint8_t> (net::type_t::IPV4): {
												// Выполняем инициализацию объекта хоста
												this->_ctx.host = make_unique <net::attr_net_t> ();
												// Устанавливаем тип адреса события
												this->_ctx.host->type = net::type_t::IPV4;
												// Устанавливаем порт хоста для подключения
												awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = ntohs(origin->ip4.port);
												// Устанавливаем IP-адрес хоста для подключения
												awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address = origin->ip4.address;
											} break;
											// Если тип данных соответствует IPv6
											case static_cast <uint8_t> (net::type_t::IPV6): {
												// Выполняем инициализацию объекта хоста
												this->_ctx.host = make_unique <net::attr_net_t> ();
												// Устанавливаем тип адреса события
												this->_ctx.host->type = net::type_t::IPV6;
												// Устанавливаем порт хоста для подключения
												awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = ntohs(origin->ip6.port);
												// Создаём новый объект адреса клиента IPv6
												awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
												// Устанавливаем IP-адрес хоста для подключения
												::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address[0], &origin->ip6.address[0], 16);
											} break;
										}
									} break;
								}
								// Размер буфера данных
								size_t size = 0;
								// Буфер данных запроса
								uint8_t * buffer = nullptr;
								// Если извлечение буфера данных запроса выполнено успешно
								if(this->_socks5.buffer(&buffer, size, this->_ctx)){
									// Если отправка запроса на прокси-сервер не выполнена
									if(this->_client->send(this->_eid, buffer, size) != size){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
											#endif
										}
									// Выходим из функции
									} else return;
								}
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, false);
						} break;
						// Если текущее состояние соответствует выполненному рукопожатию
						case static_cast <uint8_t> (proto::client_socks5_t::state_t::HANDSHAKE): {
							// Количество активных сессий клиентов, работающих через прокси
							size_t count = 0;
							{
								// Выполняем блокировку потока для работы с TLS
								const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
								// Получаем количество активных сессий клиентов, работающих через прокси
								count = this->_sessions.size();
							}
							// Если количество активных сессий клиентов, работающих через прокси, больше 1
							if(count > 1){
								// Список активных сессий клиентов, работающих через прокси, для идентификатора события клиента
								unordered_map <event::id_t, tls::coder_t::id_t> sessions;
								{
									// Выполняем блокировку потока для работы с TLS
									const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
									/**
									 * Перебираем все активные сессии клиентов, работающих через прокси
									 */
									for(auto & session : this->_sessions){
										// Устанавливаем порт хоста для подключения к удалённому серверу
										if(this->_client->setPort(session.second.first, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port)){
											// Устанавливаем порт хоста для подключения к удалённому серверу
											if(this->_client->setTarget(session.second.first, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())){
												// Выполняем фиксацию изменений для клиента, работающего через прокси
												if(this->_client->commit(session.second.first)){
													// Выполняем запуск работы клиента, работающего через прокси
													if(this->_client->launch(session.second.first))
														// Добавляем идентификатор события клиента, работающего через прокси, и идентификатор TLS для клиента в список активных сессий клиентов
														sessions.emplace(session.second.first, session.second.second);
													// Если запуск работы клиента, работающего через прокси, не выполнен
													else {
														// Если функция обратного вызова не установлена
														if(!this->_callback.is("error")){
															/**
															 * Если включён режим отладки
															 */
															#if DEBUG_MODE
																// Выводим сообщение об ошибке
																this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(session.second.first, buffer, size), log_t::flag_t::WARNING, session.second.first);
															/**
															 * Если режим отладки не включён
															 */
															#else
																// Выводим сообщение об ошибке
																this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, session.second.first);
															#endif
														}
													}
												}
											}
										}
									}
								}
								// Если список активных сессий клиентов не пустой
								if(!sessions.empty()){
									// Устанавливаем состояние клиента как "завершённый"
									this->_ctx.state = proto::client_socks5_t::state_t::COMPLETED;
									/**
									 * Перебираем все активные сессии клиентов, работающих через прокси
									 */
									for(auto & session : sessions){
										// Получаем адрес хоста для подключения к удалённому серверу
										const string & target = this->_client->getTarget(session.first);
										// Если функция обратного вызова установлена
										if(this->_callback.is("ready"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", session.first, this->_client->family(session.first), target, target);
										// Если функция обратного вызова установлена
										if(this->_callback.is("launch"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", session.first, target, this->_client->getPort(session.first));
										// Если объект транспортного уровня безопасности установлен
										if((this->_coder != nullptr) && (session.second > 0)){
											// Если рукопожатие TLS не выполнено
											if(!this->_coder->handshake(session.second)){
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error_tls")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(session.first, buffer, size), log_t::flag_t::WARNING);
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
													#endif
												}
											}
										// Если объект транспортного уровня безопасности не установлен
										} else {
											// Если функция обратного вызова установлена
											if(this->_callback.is("connect"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, session.first, true);
										}
									}
									// Выходим из функции
									return;
								}
							// Если количество активных сессий клиентов всего одна
							} else if(count == 1) {
								// Идентификатор события клиента, работающего через прокси
								event::id_t eid = 0;
								// Идентификатор TLS для клиента
								tls::coder_t::id_t tid = 0;
								// Идентификатор инициатора запроса
								const origin_t * origin = nullptr;
								{
									// Выполняем блокировку потока для работы с TLS
									const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
									// Если активные сессии клиентов, работающих через прокси, не пусты
									if(!this->_sessions.empty()){
										// Извлекаем идентификатор события клиента, работающего через прокси
										origin = &this->_sessions.begin()->first;
										// Извлекаем идентификатор события клиента, работающего через прокси
										eid = this->_sessions.begin()->second.first;
										// Извлекаем идентификатор TLS для клиента
										tid = this->_sessions.begin()->second.second;
									}
								}
								// Если параметры клиента найдены успешно
								if((eid > 0) && (origin != nullptr)){
									// Порт хоста для подключения к удалённому серверу
									uint16_t port = 0;
									// Адрес хоста для подключения к удалённому серверу
									string target = "";
									/**
									 * Определяем протокол сесии клиента, работающего через прокси
									 */
									switch(static_cast <uint8_t> (origin->protocol)){
										// Если протокол соответствует UDP
										case static_cast <uint8_t> (event::protocol_t::UDP): {
											// Устанавливаем порт хоста для подключения к удалённому серверу
											if(this->_client->setPort(eid, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port)){
												// Устанавливаем порт хоста для подключения к удалённому серверу
												if(this->_client->setTarget(eid, awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())){
													// Выполняем фиксацию изменений для клиента, работающего через прокси
													if(this->_client->commit(eid)){
														// Выполняем запуск работы клиента, работающего через прокси
														if(this->_client->launch(eid)){
															// Получаем порт хоста для подключения к удалённому серверу
															port = this->_client->getPort(eid);
															// Получаем адрес хоста для подключения к удалённому серверу
															target = this->_client->getTarget(eid);
														// Если запуск работы клиента, работающего через прокси, не выполнен
														} else {
															// Если функция обратного вызова не установлена
															if(!this->_callback.is("error")){
																/**
																 * Если включён режим отладки
																 */
																#if DEBUG_MODE
																	// Выводим сообщение об ошибке
																	this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
																/**
																 * Если режим отладки не включён
																 */
																#else
																	// Выводим сообщение об ошибке
																	this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, eid);
																#endif
															}
														}
													}
												}
											}
										} break;
										// Если протокол соответствует TCP
										case static_cast <uint8_t> (event::protocol_t::TCP): {
											/**
											 * Определяем тип данных сесии клиента, работающего через прокси
											 */
											switch(static_cast <uint8_t> (origin->type)){
												// Если тип данных соответствует FQDN
												case static_cast <uint8_t> (net::type_t::FQDN): {
													// Устанавливаем доменное имя хоста для дальнейшего использования
													target = origin->fqdn.data;
													// Устанавливаем порт хоста для дальнейшего использования
													port = ntohs(origin->fqdn.port);
												} break;
												// Если тип данных соответствует IPv4
												case static_cast <uint8_t> (net::type_t::IPV4): {
													// Устанавливаем IP-адрес хоста для дальнейшего использования
													this->_addr.v4(origin->ip4.address);
													// Устанавливаем IP-адрес хоста для дальнейшего использования
													target = ::move(static_cast <string> (this->_addr));
													// Устанавливаем порт хоста для дальнейшего использования
													port = ntohs(origin->ip4.port);
												} break;
												// Если тип данных соответствует IPv6
												case static_cast <uint8_t> (net::type_t::IPV6): {
													// Устанавливаем IP-адрес хоста для дальнейшего использования
													this->_addr.v6(origin->ip6.address);
													// Устанавливаем IP-адрес хоста для дальнейшего использования
													target = ::move(static_cast <string> (this->_addr));
													// Устанавливаем порт хоста для дальнейшего использования
													port = ntohs(origin->ip6.port);
												} break;
											}
										} break;
									}
									// Если адрес хоста для подключения к удалённому серверу получен успешно
									if(!target.empty()){
										// Устанавливаем состояние клиента как "завершённый"
										this->_ctx.state = proto::client_socks5_t::state_t::COMPLETED;
										// Если функция обратного вызова установлена
										if(this->_callback.is("ready"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", eid, this->_client->family(eid), target, target);
										// Если функция обратного вызова установлена
										if(this->_callback.is("launch"))
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const event::id_t, const string &, const uint16_t)> ("launch", eid, target, port);
										// Если объект транспортного уровня безопасности установлен
										if((this->_coder != nullptr) && (tid > 0)){
											// Если рукопожатие TLS не выполнено
											if(!this->_coder->handshake(tid)){
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error_tls")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
													#endif
												}
											// Выходим из функции
											} else return;
										// Если объект транспортного уровня безопасности не установлен
										} else {
											// Если функция обратного вызова установлена
											if(this->_callback.is("connect"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, true);
											// Выходим из функции
											return;
										}
									// Если адрес хоста для подключения к удалённому серверу не получен
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Client event ID not found", __PRETTY_FUNCTION__, make_tuple(this->_eid, buffer, size), log_t::flag_t::WARNING);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Client event ID not found", log_t::flag_t::WARNING);
										#endif
									}
								}
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, false);
						} break;
						// В остальных случаях, проходим процедуру общения с сервером в автоматическом режиме
						default: {
							// Размер буфера данных
							size_t size = 0;
							// Буфер данных запроса
							uint8_t * buffer = nullptr;
							// Если извлечение буфера данных запроса выполнено успешно
							if(this->_socks5.buffer(&buffer, size, this->_ctx)){
								// Если отправка запроса на прокси-сервер не выполнена
								if(this->_client->send(this->_eid, buffer, size) != size){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Failed to send data to remote server", log_t::flag_t::WARNING);
										#endif
									}
								// Выходим из функции
								} else return;
							}
							// Если функция обратного вызова установлена
							if(this->_callback.is("connect"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, false);
						}
					}
				// Если парсинг данных от прокси-сервера не выполнен
				} else {
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Failed to parse data from proxy server", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to parse data from proxy server", log_t::flag_t::WARNING);
						#endif
					}
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param id    идентификатор TLS
 * @param eid   идентификатор клиента
 * @param state состояние TLS
 */
void awh::client::Socks5::stateTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state_tls"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const tls::coder_t::id_t, const tls::coder_t::state_t)> ("state_tls", id, state);
		// Если состояние рукопожатия успешно завершено
		if(state == tls::coder_t::state_t::HANDSHAKED){
			// Если функция обратного вызова установлена
			if(this->_callback.is("connect")){
				// Если сообщение дешифровано для socks5-клиента
				if(eid == this->_eid){
					// Идентификатор события клиента для конечной точки
					event::id_t eid = 0;
					{
						// Выполняем блокировку потока для работы с TLS
						const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
						// Если активные сессии клиентов, работающих через прокси, не пусты
						if(!this->_sessions.empty())
							// Извлекаем идентификатор события клиента для конечной точки
							eid = this->_sessions.begin()->second.first;
					}
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, true);
				// Выполняем функцию обратного вызова
				} else this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, eid, true);
			}
		}
	}
}
/**
 * @brief Метод получения событий шифрования/дешифрования данных TLS
 *
 * @param id     идентификатор TLS
 * @param eid    идентификатор клиента
 * @param event  тип события TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 */
void awh::client::Socks5::processTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION): {
					// Если идентификатор TLS соответствует идентификатору TLS для socks5 прокси
					if(id == this->_tid){
						// Отправляем данные обратно клиенту, которые были зашифрованы TLS
						if(!this->_client->send((eid > 0 ? eid : this->_eid), reinterpret_cast <const char *> (buffer), size)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
								#endif
							}
						}
					// Если идентификатор TLS не соответствует UDP-клиенту, работающему через прокси
					} else {
						// Сбрасываем размер буфера полезной нагрузки
						::__awh_size__ = 0;
						// Тип данных для события клиента
						net::type_t type = net::type_t::NONE;
						// Инициализируем объект заголовка UDP пакета
						proto::client_socks5_t::udp_head_t udp{};
						{
							// Выполняем блокировку потока для работы с TLS
							const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
							// Ищем идентификатор события клиента для конечной точки в списке активных сессий клиентов
							auto i = this->_mapping.find(eid);
							// Если идентификатор события клиента для конечной точки найден в списке активных сессий клиентов
							if(i != this->_mapping.end()){
								// Извлекаем тип данных для события клиента
								type = i->second.first->type;
								/**
								 * Определяем тип данных сесии клиента, работающего через прокси
								 */
								switch(static_cast <uint8_t> (type)){
									// Если тип данных соответствует FQDN
									case static_cast <uint8_t> (net::type_t::FQDN): {
										// Выполняем инициализацию объекта хоста
										udp.host = make_unique <net::attr_fqdn_t> ();
										// Устанавливаем тип адреса события
										udp.host->type = net::type_t::FQDN;
										// Устанавливаем порт хоста для подключения
										awh_cast <net::attr_fqdn_t *> (udp.host.get())->port = ntohs(i->second.first->fqdn.port);
										// Устанавливаем доменное имя хоста для подключения
										awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain = i->second.first->fqdn.data;
									} break;
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (net::type_t::IPV4): {
										// Выполняем инициализацию объекта хоста
										udp.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										udp.host->type = net::type_t::IPV4;
										// Устанавливаем порт хоста для подключения
										awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(i->second.first->ip4.port);
										// Устанавливаем IP-адрес хоста для подключения
										awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address = i->second.first->ip4.address;
									} break;
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (net::type_t::IPV6): {
										// Выполняем инициализацию объекта хоста
										udp.host = make_unique <net::attr_net_t> ();
										// Устанавливаем тип адреса события
										udp.host->type = net::type_t::IPV6;
										// Устанавливаем порт хоста для подключения
										awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(i->second.first->ip6.port);
										// Создаём новый объект адреса клиента IPv6
										awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
										// Устанавливаем IP-адрес хоста для подключения
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address[0], &i->second.first->ip6.address[0], 16);
									} break;
								}
								// Размер буфера данных
								size_t length = 0;
								// Буфер данных запроса
								uint8_t * data = nullptr;
								// Если извлечение буфера данных запроса выполнено успешно
								if(this->_socks5.buffer(&data, length, udp)){
									/**
									 * Определяем тип данных сесии клиента, работающего через прокси
									 */
									switch(static_cast <uint8_t> (type)){
										// Если тип данных соответствует FQDN
										case static_cast <uint8_t> (net::type_t::FQDN):
										// Если тип данных соответствует IPv6
										case static_cast <uint8_t> (net::type_t::IPV6):
											// Устанавливаем размер буфера полезной нагрузки для отправки
											::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV6_PAYLOAD_SIZE));
										break;
										// Если тип данных соответствует IPv4
										case static_cast <uint8_t> (net::type_t::IPV4):
											// Устанавливаем размер буфера полезной нагрузки для отправки
											::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
										break;
									}
									// Если размер буфера полезной нагрузки достаточно для отправки всех данных
									if(::__awh_size__ == (size + length)){
										// Копируем данные запроса в буфер полезной нагрузки
										::memcpy(&::__awh_buffer__[0], data, length);
										// Добавляем к буферу данных для отправки полезную нагрузку
										::memcpy(&::__awh_buffer__[length], buffer, size);
									// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
									} else {
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Message sent by the UDP is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
										#endif
										// Выходим из функции
										return;
									}
								// Если извлечение буфера данных запроса не выполнено
								} else {
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Failed to generate buffer for UDP packet", log_t::flag_t::WARNING);
									#endif
								}
							}
						}
						// Если буфер полезной нагрузки для отправки не пустой
						if(::__awh_size__ > 0){
							// Если отправка запроса на прокси-сервер не выполнена
							if(this->_client->send(eid, ::__awh_buffer__, ::__awh_size__) != ::__awh_size__){
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
									#endif
								}
							}
						}
					}
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION): {
					// Если функция обратного вызова установлена
					if(this->_callback.is("read")){
						// Если сообщение дешифровано для socks5-клиента
						if(eid == this->_eid){
							// Идентификатор события клиента для конечной точки
							event::id_t eid = 0;
							{
								// Выполняем блокировку потока для работы с TLS
								const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
								// Если активные сессии клиентов, работающих через прокси, не пусты
								if(!this->_sessions.empty())
									// Извлекаем идентификатор события клиента для конечной точки
									eid = this->_sessions.begin()->second.first;
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, eid, buffer, size);
						// Выполняем функцию обратного вызова
						} else this->_callback.call <void (const event::id_t, const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, eid, buffer, size);
					}
				} break;
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @return результат выполнения приостановки работы
 */
bool awh::client::Socks5::pause() noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор клиента установлен
		if(this->_eid > 0){
			// Приостанавливаем событие клиента
			if((result = this->_client->pause(this->_eid))){
				// Если активных сессий клиентов нет
				if(this->_sessions.empty())
					// Выводим текущий результат постановки на паузу
					return result;
				// Если активные сессии клиентов присутствуют
				else {
					/**
					 * Перебираем все активные сессии клиентов, работающих через прокси
					 */
					for(auto & session : this->_sessions){
						// Если протокол сесии клиента, работающего через прокси, не соответствует UDP
						if(session.first.protocol != event::protocol_t::UDP)
							// Пропускаем постановку клиента на паузу так-как мы его уже установили
							continue;
						// Приостанавливаем событие клиента для конечной точки
						this->_client->pause(session.second.first);
					}
				}
			}
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод возобновления работы клиента
 *
 * @return результат выполнения возобновления работы
 */
bool awh::client::Socks5::resume() noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор клиента установлен
		if(this->_eid > 0){
			// Возобновляем работу события клиента
			if((result = this->_client->resume(this->_eid))){
				// Если активных сессий клиентов нет
				if(this->_sessions.empty())
					// Выводим текущий результат возобновления работы
					return result;
				// Если активные сессии клиентов присутствуют
				else {
					/**
					 * Перебираем все активные сессии клиентов, работающих через прокси
					 */
					for(auto & session : this->_sessions){
						// Если протокол сесии клиента, работающего через прокси, не соответствует UDP
						if(session.first.protocol != event::protocol_t::UDP)
							// Пропускаем возобновление работы клиента так-как мы его уже установили
							continue;
						// Возобновляем работу события клиента для конечной точки
						this->_client->resume(session.second.first);
					}
				}
			}
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод мультиподключения клиентов к удалённым хостам (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения подключения
 */
bool awh::client::Socks5::connect() noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера (заглушка для клиента SOCKS5)
 *
 * @return результат выполнения отключения
 */
bool awh::client::Socks5::disconnect() noexcept {
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод очистки активных сессий клиентов, работающих через прокси
 *
 */
void awh::client::Socks5::clearSessions() noexcept {
	// Выполняем блокировку потока для работы с TLS
	const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Очищаем активные сессии клиентов, работающих через прокси
	this->_sessions.clear();
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::client::Socks5::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для объекта блокировки
	this->_mtx.enabled = mode;
	// Устанавливаем режим безопасности работы потоков для объекта клиента
	client_t::threadSafety(mode);
}
/**
 * @brief Метод отправки данных серверу
 *
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных серверу
 */
size_t awh::client::Socks5::send(const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Идентификатор события клиента, работающего через прокси
		event::id_t eid = 0;
		// Идентификатор TLS для клиента
		tls::coder_t::id_t tid = 0;
		// Идентификатор инициатора запроса
		const origin_t * origin = nullptr;
		{
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Если активные сессии клиентов, работающих через прокси, не пусты
			if(!this->_sessions.empty()){
				// Извлекаем идентификатор события клиента, работающего через прокси
				origin = &this->_sessions.begin()->first;
				// Извлекаем идентификатор события клиента, работающего через прокси
				eid = this->_sessions.begin()->second.first;
				// Извлекаем идентификатор TLS для клиента
				tid = this->_sessions.begin()->second.second;
			}
		}
		// Если параметры клиента найдены успешно
		if(origin != nullptr){
			/**
			 * Определяем протокол сесии клиента, работающего через прокси
			 */
			switch(static_cast <uint8_t> (origin->protocol)){
				// Если протокол соответствует UDP
				case static_cast <uint8_t> (event::protocol_t::UDP): {
					// Если идентификатор TLS и объект TLS установлены
					if((tid > 0) && (this->_coder != nullptr)){
						// Если шифрование данных TLS выполнено успешно
						if(this->_coder->encrypt(tid, buffer, size))
							// Возвращаем размер отправленных данных
							return size;
						// Выводим результат по умолчанию
						return 0;
					}
					// Инициализируем объект заголовка UDP пакета
					proto::client_socks5_t::udp_head_t udp{};
					/**
					 * Определяем тип данных сесии клиента, работающего через прокси
					 */
					switch(static_cast <uint8_t> (origin->type)){
						// Если тип данных соответствует FQDN
						case static_cast <uint8_t> (net::type_t::FQDN): {
							// Выполняем инициализацию объекта хоста
							udp.host = make_unique <net::attr_fqdn_t> ();
							// Устанавливаем тип адреса события
							udp.host->type = net::type_t::FQDN;
							// Устанавливаем порт хоста для подключения
							awh_cast <net::attr_fqdn_t *> (udp.host.get())->port = ntohs(origin->fqdn.port);
							// Устанавливаем доменное имя хоста для подключения
							awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain = origin->fqdn.data;
						} break;
						// Если тип данных соответствует IPv4
						case static_cast <uint8_t> (net::type_t::IPV4): {
							// Выполняем инициализацию объекта хоста
							udp.host = make_unique <net::attr_net_t> ();
							// Устанавливаем тип адреса события
							udp.host->type = net::type_t::IPV4;
							// Устанавливаем порт хоста для подключения
							awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(origin->ip4.port);
							// Устанавливаем IP-адрес хоста для подключения
							awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address = origin->ip4.address;
						} break;
						// Если тип данных соответствует IPv6
						case static_cast <uint8_t> (net::type_t::IPV6): {
							// Выполняем инициализацию объекта хоста
							udp.host = make_unique <net::attr_net_t> ();
							// Устанавливаем тип адреса события
							udp.host->type = net::type_t::IPV6;
							// Устанавливаем порт хоста для подключения
							awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(origin->ip6.port);
							// Создаём новый объект адреса клиента IPv6
							awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							// Устанавливаем IP-адрес хоста для подключения
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address[0], &origin->ip6.address[0], 16);
						} break;
					}
					// Размер буфера данных
					size_t length = 0;
					// Буфер данных запроса
					uint8_t * data = nullptr;
					// Если извлечение буфера данных запроса выполнено успешно
					if(this->_socks5.buffer(&data, length, udp)){
						/**
						 * Определяем тип данных сесии клиента, работающего через прокси
						 */
						switch(static_cast <uint8_t> (origin->type)){
							// Если тип данных соответствует FQDN
							case static_cast <uint8_t> (net::type_t::FQDN):
							// Если тип данных соответствует IPv6
							case static_cast <uint8_t> (net::type_t::IPV6):
								// Устанавливаем размер буфера полезной нагрузки для отправки
								::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV6_PAYLOAD_SIZE));
							break;
							// Если тип данных соответствует IPv4
							case static_cast <uint8_t> (net::type_t::IPV4):
								// Устанавливаем размер буфера полезной нагрузки для отправки
								::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
							break;
						}
						// Если размер буфера полезной нагрузки достаточно для отправки всех данных
						if(::__awh_size__ == (size + length)){
							// Копируем данные запроса в буфер полезной нагрузки
							::memcpy(&::__awh_buffer__[0], data, length);
							// Добавляем к буферу данных для отправки полезную нагрузку
							::memcpy(&::__awh_buffer__[length], buffer, size);
							// Выполняем отправку данных серверу
							return this->_client->send(eid, ::__awh_buffer__, ::__awh_size__);
						// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Message sent by the UDP is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
							#endif
						}
					// Если извлечение буфера данных запроса не выполнено
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to generate buffer for UDP packet", log_t::flag_t::WARNING);
						#endif
					}
				} break;
				// Если протокол соответствует TCP
				case static_cast <uint8_t> (event::protocol_t::TCP): {
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
					return this->_client->send(this->_eid, buffer, size);
				}
			}
		// Если инициатор запроса не найден
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
			#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
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
bool awh::client::Socks5::recv(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор клиента передан корректно
		if(eid == this->_eid)
			// Получаем данные от сервера
			return this->_client->recv(eid);
		// Если идентификатор клиента не передан корректно
		else {
			// Идентификатор инициатора запроса
			const origin_t * origin = nullptr;
			{
				// Выполняем блокировку потока для работы с TLS
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
				// Ищем идентификатор события клиента для конечной точки в списке активных сессий клиентов
				auto i = this->_mapping.find(eid);
				// Если идентификатор события клиента для конечной точки найден в списке активных сессий клиентов
				if(i != this->_mapping.end())
					// Извлекаем идентификатор события клиента, работающего через прокси
					origin = i->second.first;
			}
			// Если параметры клиента найдены успешно
			if(origin != nullptr){
				/**
				 * Определяем протокол сесии клиента, работающего через прокси
				 */
				switch(static_cast <uint8_t> (origin->protocol)){
					// Если протокол соответствует UDP
					case static_cast <uint8_t> (event::protocol_t::UDP):
						// Получаем данные от сервера
						return this->_client->recv(eid);
					// Если протокол соответствует TCP
					case static_cast <uint8_t> (event::protocol_t::TCP):
						// Получаем данные от сервера
						return this->_client->recv(this->_eid);
				}
			// Если идентификатор события клиента для конечной точки не найден в списке активных сессий клиентов
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
				#endif
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
size_t awh::client::Socks5::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор клиента передан корректно
		if(eid == this->_eid){
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
			return this->_client->send(this->_eid, buffer, size);
		// Если идентификатор клиента не передан корректно
		} else {
			// Идентификатор TLS для клиента
			tls::coder_t::id_t tid = 0;
			// Идентификатор инициатора запроса
			const origin_t * origin = nullptr;
			{
				// Выполняем блокировку потока для работы с TLS
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
				// Ищем идентификатор события клиента для конечной точки в списке активных сессий клиентов
				auto i = this->_mapping.find(eid);
				// Если идентификатор события клиента для конечной точки найден в списке активных сессий клиентов
				if(i != this->_mapping.end()){
					// Извлекаем идентификатор TLS для клиента
					tid = i->second.second;
					// Извлекаем идентификатор события клиента, работающего через прокси
					origin = i->second.first;
				}
			}
			// Если параметры клиента найдены успешно
			if(origin != nullptr){
				/**
				 * Определяем протокол сесии клиента, работающего через прокси
				 */
				switch(static_cast <uint8_t> (origin->protocol)){
					// Если протокол соответствует UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						// Если идентификатор TLS и объект TLS установлены
						if((tid > 0) && (this->_coder != nullptr)){
							// Если шифрование данных TLS выполнено успешно
							if(this->_coder->encrypt(tid, buffer, size))
								// Возвращаем размер отправленных данных
								return size;
							// Выводим результат по умолчанию
							return 0;
						}
						// Инициализируем объект заголовка UDP пакета
						proto::client_socks5_t::udp_head_t udp{};
						/**
						 * Определяем тип данных сесии клиента, работающего через прокси
						 */
						switch(static_cast <uint8_t> (origin->type)){
							// Если тип данных соответствует FQDN
							case static_cast <uint8_t> (net::type_t::FQDN): {
								// Выполняем инициализацию объекта хоста
								udp.host = make_unique <net::attr_fqdn_t> ();
								// Устанавливаем тип адреса события
								udp.host->type = net::type_t::FQDN;
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_fqdn_t *> (udp.host.get())->port = ntohs(origin->fqdn.port);
								// Устанавливаем доменное имя хоста для подключения
								awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain = origin->fqdn.data;
							} break;
							// Если тип данных соответствует IPv4
							case static_cast <uint8_t> (net::type_t::IPV4): {
								// Выполняем инициализацию объекта хоста
								udp.host = make_unique <net::attr_net_t> ();
								// Устанавливаем тип адреса события
								udp.host->type = net::type_t::IPV4;
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(origin->ip4.port);
								// Устанавливаем IP-адрес хоста для подключения
								awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address = origin->ip4.address;
							} break;
							// Если тип данных соответствует IPv6
							case static_cast <uint8_t> (net::type_t::IPV6): {
								// Выполняем инициализацию объекта хоста
								udp.host = make_unique <net::attr_net_t> ();
								// Устанавливаем тип адреса события
								udp.host->type = net::type_t::IPV6;
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(origin->ip6.port);
								// Создаём новый объект адреса клиента IPv6
								awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
								// Устанавливаем IP-адрес хоста для подключения
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address[0], &origin->ip6.address[0], 16);
							} break;
						}
						// Размер буфера данных
						size_t length = 0;
						// Буфер данных запроса
						uint8_t * data = nullptr;
						// Если извлечение буфера данных запроса выполнено успешно
						if(this->_socks5.buffer(&data, length, udp)){
							/**
							 * Определяем тип данных сесии клиента, работающего через прокси
							 */
							switch(static_cast <uint8_t> (origin->type)){
								// Если тип данных соответствует FQDN
								case static_cast <uint8_t> (net::type_t::FQDN):
								// Если тип данных соответствует IPv6
								case static_cast <uint8_t> (net::type_t::IPV6):
									// Устанавливаем размер буфера полезной нагрузки для отправки
									::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV6_PAYLOAD_SIZE));
								break;
								// Если тип данных соответствует IPv4
								case static_cast <uint8_t> (net::type_t::IPV4):
									// Устанавливаем размер буфера полезной нагрузки для отправки
									::__awh_size__ = ::min(size + length, static_cast <size_t> (AWH_MTU_UDP_IPV4_PAYLOAD_SIZE));
								break;
							}
							// Если размер буфера полезной нагрузки достаточно для отправки всех данных
							if(::__awh_size__ == (size + length)){
								// Копируем данные запроса в буфер полезной нагрузки
								::memcpy(&::__awh_buffer__[0], data, length);
								// Добавляем к буферу данных для отправки полезную нагрузку
								::memcpy(&::__awh_buffer__[length], buffer, size);
								// Выполняем отправку данных серверу
								return this->_client->send(eid, ::__awh_buffer__, ::__awh_size__);
							// Если размер буфера полезной нагрузки недостаточно для отправки всех данных
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Message sent by the UDP is too large for the configured MTU values of %zu bytes", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, ::__awh_size__);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Message sent by the UDP is too large for the configured MTU values of %zu bytes", log_t::flag_t::WARNING, ::__awh_size__);
								#endif
							}
						// Если извлечение буфера данных запроса не выполнено
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to generate buffer for UDP packet", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to generate buffer for UDP packet", log_t::flag_t::WARNING);
							#endif
						}
					} break;
					// Если протокол соответствует TCP
					case static_cast <uint8_t> (event::protocol_t::TCP): {
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
						return this->_client->send(this->_eid, buffer, size);
					}
				}
			// Если инициатор запроса не найден для переданного идентификатора события клиента
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
				#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки пропускной способности клиента
 *
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 */
bool awh::client::Socks5::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор клиента установлен
		if(this->_eid > 0){
			// Устанавливаем пропускную способность клиента
			if((result = this->_client->bandwidth(this->_eid, limiting, bandwidth))){
				// Если активных сессий клиентов нет
				if(this->_sessions.empty())
					// Выводим текущий результат установки пропускной способности клиента
					return result;
				// Если активные сессии клиентов присутствуют
				else {
					/**
					 * Перебираем все активные сессии клиентов, работающих через прокси
					 */
					for(auto & session : this->_sessions){
						// Если протокол сесии клиента, работающего через прокси, не соответствует UDP
						if(session.first.protocol != event::protocol_t::UDP)
							// Пропускаем установку пропускной способности клиента так-как мы его уже установили
							continue;
						// Устанавливаем пропускную способность клиента для конечной точки
						this->_client->bandwidth(session.second.first, limiting, bandwidth);
					}
				}
			}
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки параметров авторизации
 *
 * @param username имя пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Socks5::setUser(const string & username, const string & password) noexcept {
	// Выполняем блокировку потока для работы с TLS
	const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Устанавливаем параметры авторизации для объекта клиента
	this->_socks5.setUser(username, password);
}
/**
 * @brief Метод проверки наличия идентификатора события клиента для конечной точки
 *
 * @param eid идентификатор события для проверки
 * @return    результат проверки наличия идентификатора события клиента для конечной точки
 */
bool awh::client::Socks5::isEventIdEndpoint(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с TLS
	const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
	// Если идентификатор события клиента найден в карте соответствия, возвращаем результат
	return (this->_mapping.find(eid) != this->_mapping.end());
}
/**
 * @brief Метод добавления идентификатора события клиента для конечной точки
 *
 * @param eid идентификатор события для добавления
 * @return    результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::client::Socks5::addEventIdEndpoint(const event::id_t eid) noexcept {
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
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(target->size){
				// Для типа IPv4
				case 4:
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
				break;
				// Для типа IPv6
				case 16:
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
				break;
			}
			// Устанавливаем полученный IP-адрес
			awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(target);
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = this->_client->getPort(eid);
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = origin_t().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если протокол соответствует UDP и установлен идентификатор TLS для клиента
			if((this->_tid > 0) && (protocol == event::protocol_t::UDP))
				// Сбрасываем идентификатор TLS для клиента
				this->_tid = 0;
			// Добавляем идентификатор события клиента для конечной точки
			auto ret =  this->_sessions.emplace(endpoint, make_pair(eid, this->_tid));
			// Если идентификатор события клиента для конечной точки добавлен
			if(ret.second)
				// Добавляем идентификатор события клиента в карту соответствия
				return this->_mapping.emplace(eid, make_pair(&ret.first->first, this->_tid)).second;
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления идентификатора события клиента для конечной точки
 *
 * @param eid идентификатор события для добавления
 * @param tid идентификатор TLS для добавления
 * @return    результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::client::Socks5::addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid) noexcept {
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
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(target->size){
				// Для типа IPv4
				case 4:
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
				break;
				// Для типа IPv6
				case 16:
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
				break;
			}
			// Устанавливаем полученный IP-адрес
			awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(target);
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = this->_client->getPort(eid);
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = origin_t().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
			if((tid > 0) && (this->_coder != nullptr)){
				// Устанавливаем функцию обратного вызова на событие ошибок TLS
				this->_coder->on(tid, std::bind(&socks5_t::errorTLS, this, _1, _2, _3));
				/**
				 * Определяем протокол протокол клиента
				 */
				switch(static_cast <uint8_t> (protocol)){
					// Если протокол соответствует UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						// Устанавливаем функцию обратного вызова на событие состояния TLS
						this->_coder->on(tid, std::bind(&socks5_t::stateTLS, this, _1, eid, _2));
						// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
						this->_coder->on(tid, std::bind(&socks5_t::processTLS, this, _1, eid, _2, _3, _4));
					} break;
					// Если протокол соответствует TCP
					case static_cast <uint8_t> (event::protocol_t::TCP): {
						// Устанавливаем идентификатор TLS для клиента
						this->_tid = tid;
						// Устанавливаем функцию обратного вызова на событие состояния TLS
						this->_coder->on(tid, std::bind(&socks5_t::stateTLS, this, _1, this->_eid, _2));
						// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
						this->_coder->on(tid, std::bind(&socks5_t::processTLS, this, _1, this->_eid, _2, _3, _4));
					} break;
				}
			}
			// Если протокол соответствует UDP и установлен идентификатор TLS для клиента
			if((this->_tid > 0) && (protocol == event::protocol_t::UDP))
				// Сбрасываем идентификатор TLS для клиента
				this->_tid = 0;
			// Добавляем идентификатор события клиента для конечной точки
			auto ret = this->_sessions.emplace(endpoint, make_pair(eid, tid));
			// Если идентификатор события клиента для конечной точки добавлен
			if(ret.second)
				// Добавляем идентификатор события клиента в карту соответствия
				return this->_mapping.emplace(eid, make_pair(&ret.first->first, tid)).second;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, tid), log_t::flag_t::CRITICAL, error.what());
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
bool awh::client::Socks5::addEventIdEndpoint(const event::id_t eid, string_view addr, const uint16_t port) noexcept {
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
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::FQDN;
					// Устанавливаем полученный порт
					awh_cast <net::attr_fqdn_t *> (attr.get())->port = port;
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (attr.get())->domain = addr;
				} break;
				// Для типа IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = port;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
				// Для типа IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = port;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если объект параметров подключения для идентификатора события клиента создан
			if(attr != nullptr){
				// Получаем протокол для идентификатора события клиента
				const event::protocol_t protocol = this->_client->protocol(eid);
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = origin_t().from(attr.get(), protocol);
				// Если протокол соответствует UDP и установлен идентификатор TLS для клиента
				if((this->_tid > 0) && (protocol == event::protocol_t::UDP))
					// Сбрасываем идентификатор TLS для клиента
					this->_tid = 0;
				// Добавляем идентификатор события клиента для конечной точки
				auto ret =  this->_sessions.emplace(endpoint, make_pair(eid, this->_tid));
				// Если идентификатор события клиента для конечной точки добавлен
				if(ret.second)
					// Добавляем идентификатор события клиента в карту соответствия
					return this->_mapping.emplace(eid, make_pair(&ret.first->first, this->_tid)).second;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
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
 * @param tid  идентификатор TLS для добавления
 * @param addr адрес хоста для добавления
 * @param port порт хоста для добавления
 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::client::Socks5::addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid, string_view addr, const uint16_t port) noexcept {
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
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::FQDN;
					// Устанавливаем полученный порт
					awh_cast <net::attr_fqdn_t *> (attr.get())->port = port;
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (attr.get())->domain = addr;
				} break;
				// Для типа IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = port;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
				// Для типа IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = port;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если объект параметров подключения для идентификатора события клиента создан
			if(attr != nullptr){
				// Получаем протокол для идентификатора события клиента
				const event::protocol_t protocol = this->_client->protocol(eid);
				// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
				if((tid > 0) && (this->_coder != nullptr)){
					// Устанавливаем функцию обратного вызова на событие ошибок TLS
					this->_coder->on(tid, std::bind(&socks5_t::errorTLS, this, _1, _2, _3));
					/**
					 * Определяем протокол протокол клиента
					 */
					switch(static_cast <uint8_t> (protocol)){
						// Если протокол соответствует UDP
						case static_cast <uint8_t> (event::protocol_t::UDP): {
							// Устанавливаем функцию обратного вызова на событие состояния TLS
							this->_coder->on(tid, std::bind(&socks5_t::stateTLS, this, _1, eid, _2));
							// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
							this->_coder->on(tid, std::bind(&socks5_t::processTLS, this, _1, eid, _2, _3, _4));
						} break;
						// Если протокол соответствует TCP
						case static_cast <uint8_t> (event::protocol_t::TCP): {
							// Устанавливаем идентификатор TLS для клиента
							this->_tid = tid;
							// Устанавливаем функцию обратного вызова на событие состояния TLS
							this->_coder->on(tid, std::bind(&socks5_t::stateTLS, this, _1, this->_eid, _2));
							// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
							this->_coder->on(tid, std::bind(&socks5_t::processTLS, this, _1, this->_eid, _2, _3, _4));
						} break;
					}
				}
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = origin_t().from(attr.get(), protocol);
				// Если протокол соответствует UDP и установлен идентификатор TLS для клиента
				if((this->_tid > 0) && (protocol == event::protocol_t::UDP))
					// Сбрасываем идентификатор TLS для клиента
					this->_tid = 0;
				// Добавляем идентификатор события клиента для конечной точки
				auto ret =  this->_sessions.emplace(endpoint, make_pair(eid, tid));
				// Если идентификатор события клиента для конечной точки добавлен
				if(ret.second)
					// Добавляем идентификатор события клиента в карту соответствия
					return this->_mapping.emplace(eid, make_pair(&ret.first->first, tid)).second;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
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
bool awh::client::Socks5::addEventIdEndpoint(const event::id_t eid, const net::addr_t * addr, const uint16_t port) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события клиента для конечной точки получен и адрес хоста для добавления не пустой
		if((eid > 0) && (addr != nullptr)){
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(addr->size){
				// Для типа IPv4
				case 4: {
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
				} break;
				// Для типа IPv6
				case 16: {
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
					// Создаём новый объект адреса клиента IPv6
					awh_cast <net::attr_net_t *> (attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
				} break;
			}
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = port;
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = origin_t().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если протокол соответствует UDP и установлен идентификатор TLS для клиента
			if((this->_tid > 0) && (protocol == event::protocol_t::UDP))
				// Сбрасываем идентификатор TLS для клиента
				this->_tid = 0;
			// Добавляем идентификатор события клиента для конечной точки
			auto ret = this->_sessions.emplace(endpoint, make_pair(eid, this->_tid));
			// Если идентификатор события клиента для конечной точки добавлен
			if(ret.second)
				// Добавляем идентификатор события клиента в карту соответствия
				return this->_mapping.emplace(eid, make_pair(&ret.first->first, this->_tid)).second;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
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
 * @param tid  идентификатор TLS для добавления
 * @param addr адрес хоста для добавления
 * @param port порт хоста для добавления
 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::client::Socks5::addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid, const net::addr_t * addr, const uint16_t port) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события клиента для конечной точки получен и адрес хоста для добавления не пустой
		if((eid > 0) && (addr != nullptr)){
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(addr->size){
				// Для типа IPv4
				case 4: {
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
				} break;
				// Для типа IPv6
				case 16: {
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
					// Создаём новый объект адреса клиента IPv6
					awh_cast <net::attr_net_t *> (attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
				} break;
			}
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = port;
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = origin_t().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
			if((tid > 0) && (this->_coder != nullptr)){
				// Устанавливаем функцию обратного вызова на событие ошибок TLS
				this->_coder->on(tid, std::bind(&socks5_t::errorTLS, this, _1, _2, _3));
				/**
				 * Определяем протокол протокол клиента
				 */
				switch(static_cast <uint8_t> (protocol)){
					// Если протокол соответствует UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						// Устанавливаем функцию обратного вызова на событие состояния TLS
						this->_coder->on(tid, std::bind(&socks5_t::stateTLS, this, _1, eid, _2));
						// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
						this->_coder->on(tid, std::bind(&socks5_t::processTLS, this, _1, eid, _2, _3, _4));
					} break;
					// Если протокол соответствует TCP
					case static_cast <uint8_t> (event::protocol_t::TCP): {
						// Устанавливаем идентификатор TLS для клиента
						this->_tid = tid;
						// Устанавливаем функцию обратного вызова на событие состояния TLS
						this->_coder->on(tid, std::bind(&socks5_t::stateTLS, this, _1, this->_eid, _2));
						// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
						this->_coder->on(tid, std::bind(&socks5_t::processTLS, this, _1, this->_eid, _2, _3, _4));
					} break;
				}
			}
			// Если протокол соответствует UDP и установлен идентификатор TLS для клиента
			if((this->_tid > 0) && (protocol == event::protocol_t::UDP))
				// Сбрасываем идентификатор TLS для клиента
				this->_tid = 0;
			// Добавляем идентификатор события клиента для конечной точки
			auto ret = this->_sessions.emplace(endpoint, make_pair(eid, tid));
			// Если идентификатор события клиента для конечной точки добавлен
			if(ret.second)
				// Добавляем идентификатор события клиента в карту соответствия
				return this->_mapping.emplace(eid, make_pair(&ret.first->first, tid)).second;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
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
bool awh::client::Socks5::delEventIdEndpoint(const event::id_t eid) noexcept {
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
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(target->size){
				// Для типа IPv4
				case 4:
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
				break;
				// Для типа IPv6
				case 16:
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
				break;
			}
			// Устанавливаем полученный IP-адрес
			awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(target);
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = this->_client->getPort(eid);
			// Получаем протокол для идентификатора события клиента
			const event::protocol_t protocol = this->_client->protocol(eid);
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = origin_t().from(attr.get(), protocol);
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск идентификатора события клиента для конечной точки
			auto i = this->_sessions.find(endpoint);
			// Если идентификатор события клиента для конечной точки найден
			if((result = (i != this->_sessions.end()))){
				// Удаляем идентификатор события клиента для конечной точки
				this->_sessions.erase(i);
				// Выполняем поиск идентификатора события клиента в карте соответствия
				auto j = this->_mapping.find(eid);
				// Если идентификатор события клиента найден в карте соответствия
				if((result = (j != this->_mapping.end())))
					// Удаляем идентификатор события клиента из карты соответствия
					this->_mapping.erase(j);
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
bool awh::client::Socks5::delEventIdEndpoint(const event::id_t eid, string_view addr, const uint16_t port) noexcept {
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
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::FQDN;
					// Устанавливаем полученный порт
					awh_cast <net::attr_fqdn_t *> (attr.get())->port = port;
					// Устанавливаем полученный доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (attr.get())->domain = addr;
				} break;
				// Для типа IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = port;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
				// Для типа IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IP-адреса
					this->_addr = addr;
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
					// Устанавливаем полученный порт
					awh_cast <net::attr_net_t *> (attr.get())->port = port;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::attr_net_t *> (attr.get())->ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если объект параметров подключения для идентификатора события клиента создан
			if(attr != nullptr){
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = origin_t().from(attr.get(), this->_client->protocol(eid));
				// Выполняем поиск идентификатора события клиента для конечной точки
				auto i = this->_sessions.find(endpoint);
				// Если идентификатор события клиента для конечной точки найден
				if((result = (i != this->_sessions.end()))){
					// Удаляем идентификатор события клиента для конечной точки
					this->_sessions.erase(i);
					// Выполняем поиск идентификатора события клиента в карте соответствия
					auto j = this->_mapping.find(eid);
					// Если идентификатор события клиента найден в карте соответствия
					if((result = (j != this->_mapping.end())))
						// Удаляем идентификатор события клиента из карты соответствия
						this->_mapping.erase(j);
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
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
bool awh::client::Socks5::delEventIdEndpoint(const event::id_t eid, const net::addr_t * addr, const uint16_t port) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события клиента для конечной точки получен и адрес хоста для добавления не пустой
		if((eid > 0) && (addr != nullptr)){
			// Создаём объект параметров подключения для идентификатора события клиента
			unique_ptr <net::attr_t> attr = make_unique <net::attr_net_t> ();
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(addr->size){
				// Для типа IPv4
				case 4: {
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV4;
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
				} break;
				// Для типа IPv6
				case 16: {
					// Устанавливаем тип параметров подключения для идентификатора события клиента
					attr->type = net::type_t::IPV6;
					// Создаём новый объект адреса клиента IPv6
					awh_cast <net::attr_net_t *> (attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
				} break;
			}
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = port;
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = origin_t().from(attr.get(), this->_client->protocol(eid));
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск идентификатора события клиента для конечной точки
			auto i = this->_sessions.find(endpoint);
			// Если идентификатор события клиента для конечной точки найден
			if((result = (i != this->_sessions.end()))){
				// Удаляем идентификатор события клиента для конечной точки
				this->_sessions.erase(i);
				// Выполняем поиск идентификатора события клиента в карте соответствия
				auto j = this->_mapping.find(eid);
				// Если идентификатор события клиента найден в карте соответствия
				if((result = (j != this->_mapping.end())))
					// Удаляем идентификатор события клиента из карты соответствия
					this->_mapping.erase(j);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, addr, port), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки идентификатора TLS
 *
 * @param tid идентификатор TLS для установки
 */
void awh::client::Socks5::setSecurityId(const tls::coder_t::id_t tid) noexcept {
	// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
	if((tid > 0) && (this->_coder != nullptr)){
		// Устанавливаем идентификатор TLS для клиента
		this->_tid = tid;
		// Устанавливаем функцию обратного вызова на событие ошибок TLS
		this->_coder->on(this->_tid, std::bind(&socks5_t::errorTLS, this, _1, _2, _3));
		// Устанавливаем функцию обратного вызова на событие состояния TLS
		this->_coder->on(this->_tid, std::bind(&socks5_t::stateTLS, this, _1, this->_eid, _2));
		// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
		this->_coder->on(this->_tid, std::bind(&socks5_t::processTLS, this, _1, this->_eid, _2, _3, _4));
	}
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param coder  объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, coder, fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param dns    объект DNS-резолвера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, dns, fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param dns    объект DNS-резолвера
 * @param coder  объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::client::Socks5::Socks5(unit::client_t * client, unit::dns_t * dns, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 client_t(client, dns, coder, fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Деструктор
 *
 */
awh::client::Socks5::~Socks5() noexcept {}
