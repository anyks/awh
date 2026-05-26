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
 * @brief Метод изменения статуса клиента
 *
 * @param status новый статус клиента
 * @param state  новое временное состояние клиента
 */
void awh::Socks5::status(const event::status_t status, const state_t state) noexcept {
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
								this->_log->debug("Failed to connect to remote server", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::WARNING);
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
									this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::WARNING, this->_eid);
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
								this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_client->getTarget(this->_eid), this->_client->getPort(this->_eid));
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_eid), log_t::flag_t::WARNING, error.c_str());
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
void awh::Socks5::connect(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
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
						this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, std::make_tuple(this->_eid), log_t::flag_t::WARNING);
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
	}
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param eid    идентификатор клиента
 * @param status новый статус клиента
 */
void awh::Socks5::state(const event::id_t eid, const event::status_t status) noexcept {
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
void awh::Socks5::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если событие принадлежит прокси-клиенту
		if(eid == this->_eid){
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status));
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socks5.statusMessage(this->_ctx.status));
							#endif
						// Выполняем функцию обратного вызова
						} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, this->_socks5.statusMessage(this->_ctx.status));
					} break;
					// Если текущее состояние соответствует ожиданию выполнения подключения
					case static_cast <uint8_t> (proto::client_socks5_t::state_t::CONNECT): {
						// Идентификатор инициатора запроса
						const origin_t * origin = nullptr;
						{
							// Выполняем блокировку потока для работы с TLS
							const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
							// Если активные сессии клиентов, работающих через прокси, не пусты
							if(!this->_sessions.empty())
								// Извлекаем идентификатор события клиента, работающего через прокси
								origin = &this->_sessions.begin()->first;
						}
						// Если идентификатор инициатора запроса получен успешно
						if(origin != nullptr){
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
									// Устанавливаем IP-адрес хоста для подключения
									::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address[0], &origin->ip6.address[0], 16);
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
											this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
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
						}
					} break;
					// Если текущее состояние соответствует выполненному рукопожатию
					case static_cast <uint8_t> (proto::client_socks5_t::state_t::HANDSHAKE): {
						// Идентификатор события клиента, работающего через прокси
						event::id_t eid = 0;
						{
							// Выполняем блокировку потока для работы с TLS
							const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
							// Если активные сессии клиентов, работающих через прокси, не пусты
							if(!this->_sessions.empty())
								// Извлекаем идентификатор события клиента, работающего через прокси
								eid = this->_sessions.begin()->second.first;
						}
						// Если идентификатор события клиента, получен успешно
						if(eid > 0){
							// Порт хоста для подключения к удалённому серверу
							uint16_t port = 0;
							// Адрес хоста для подключения к удалённому серверу
							string target = "";
							// Идентификатор инициатора запроса
							const origin_t * origin = nullptr;
							{
								// Выполняем блокировку потока для работы с TLS
								const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
								// Если активные сессии клиентов, работающих через прокси, не пусты
								if(!this->_sessions.empty())
									// Извлекаем идентификатор события клиента, работающего через прокси
									origin = &this->_sessions.begin()->first;
							}
							// Если идентификатор инициатора запроса получен успешно
							if(origin != nullptr){
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
							}
							// Если адрес хоста для подключения к удалённому серверу получен успешно
							if(!target.empty()){
								// Если функция обратного вызова установлена
								if(this->_callback.is("ready"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", eid, this->_client->family(eid), target, target);
								// Если функция обратного вызова установлена
								if(this->_callback.is("launch"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const string &, const uint16_t)> ("launch", target, port);
								// Если объект транспортного уровня безопасности установлен
								if((this->_coder != nullptr) && (this->_tid > 0)){
									// Если рукопожатие TLS не выполнено
									if(!this->_coder->handshake(this->_tid)){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error_tls")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
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
										this->_callback.call <void (const event::id_t, const bool)> ("connect", eid, true);
								}
							// Если адрес хоста для подключения к удалённому серверу не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Client event ID not found", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Client event ID not found", log_t::flag_t::WARNING);
								#endif
							}
						}
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
										this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
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
						this->_log->debug("Failed to parse data from proxy server", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Failed to parse data from proxy server", log_t::flag_t::WARNING);
					#endif
				}
			}
		// Если событие принадлежит клиенту
		} else {
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
							this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
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
					// Идентификатор события клиента, работающего через прокси
					event::id_t eid = 0;
					{
						// Выполняем блокировку потока для работы с TLS
						const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
						// Если активные сессии клиентов, работающих через прокси, не пусты
						if(!this->_sessions.empty())
							// Извлекаем идентификатор события клиента, работающего через прокси
							eid = this->_sessions.begin()->second.first;
					}
					// Если идентификатор события клиента, получен успешно
					if(eid > 0)
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, buffer, size);
					// Если адрес хоста для подключения к удалённому серверу не получен
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Client event ID not found", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
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
		}
	}
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param id    идентификатор TLS
 * @param state состояние TLS
 */
void awh::Socks5::stateTLS(const tls::coder_t::id_t id, const tls::coder_t::state_t state) noexcept {
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
				// Идентификатор события клиента
				event::id_t eid = 0;
				{
					// Выполняем блокировку потока для работы с TLS
					const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
					// Если активные сессии клиентов, работающих через прокси, не пусты
					if(!this->_sessions.empty())
						// Извлекаем идентификатор события клиента, работающего через прокси
						eid = this->_sessions.begin()->second.first;
				}
				// Если идентификатор события клиента получен
				if(eid > 0)
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const bool)> ("connect", eid, true);
				// Если идентификатор события клиента не получен
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Client event ID not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (state)), log_t::flag_t::WARNING);
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
	}
}
/**
 * @brief Метод получения событий шифрования/дешифрования данных TLS
 *
 * @param id     идентификатор TLS
 * @param event  тип события TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 */
void awh::Socks5::processTLS(const tls::coder_t::id_t id, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Обрабатываем тип события TLS
		 */
		switch(static_cast <uint8_t> (event)){
			// Если событие шифрования данных TLS
			case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION): {
				// Отправляем данные обратно клиенту, которые были зашифрованы TLS
				if(!this->_client->send(this->_eid, reinterpret_cast <const char *> (buffer), size)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Если событие дешифрования данных TLS
			case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION): {
				// Идентификатор события клиента
				event::id_t eid = 0;
				{
					// Выполняем блокировку потока для работы с TLS
					const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
					// Если активные сессии клиентов, работающих через прокси, не пусты
					if(!this->_sessions.empty())
						// Извлекаем идентификатор события клиента, работающего через прокси
						eid = this->_sessions.begin()->second.first;
				}
				// Если идентификатор события клиента получен
				if(eid > 0){
					// Если функция обратного вызова установлена
					if(this->_callback.is("read"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, buffer, size);
				// Если идентификатор события клиента не получен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Client event ID not found", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Client event ID not found", log_t::flag_t::WARNING);
					#endif
				}
			} break;
		}
	}
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
	if(eid == this->_eid)
		// Получаем данные от сервера
		return this->_client->recv(this->_eid);
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
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
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
			return this->_sessions.emplace(endpoint, make_pair(eid, 0)).second;
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
 * @param eid идентификатор события для добавления
 * @param tid идентификатор TLS для добавления
 * @return    результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid) noexcept {
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
			// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
			if((tid > 0) && (this->_coder != nullptr)){
				// Устанавливаем идентификатор TLS для клиента
				this->_tid = tid;
				// Устанавливаем функцию обратного вызова на событие состояния TLS
				this->_coder->on(this->_tid, std::bind(&socks5_t::stateTLS, this, _1, _2));
				// Устанавливаем функцию обратного вызова на событие ошибок TLS
				this->_coder->on(this->_tid, std::bind(&socks5_t::errorTLS, this, _1, _2, _3));
				// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
				this->_coder->on(this->_tid, std::bind(&socks5_t::processTLS, this, _1, _2, _3, _4));
			}
			// Добавляем идентификатор события клиента для конечной точки
			return this->_sessions.emplace(endpoint, make_pair(eid, tid)).second;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, tid), log_t::flag_t::CRITICAL, error.what());
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
				return this->_sessions.emplace(endpoint, make_pair(eid, 0)).second;
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
 * @brief Метод добавления идентификатора события клиента для конечной точки
 *
 * @param eid  идентификатор события для добавления
 * @param tid  идентификатор TLS для добавления
 * @param addr адрес хоста для добавления
 * @param port порт хоста для добавления
 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid, string_view addr, const uint16_t port) noexcept {
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
				// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
				if((tid > 0) && (this->_coder != nullptr)){
					// Устанавливаем идентификатор TLS для клиента
					this->_tid = tid;
					// Устанавливаем функцию обратного вызова на событие состояния TLS
					this->_coder->on(this->_tid, std::bind(&socks5_t::stateTLS, this, _1, _2));
					// Устанавливаем функцию обратного вызова на событие ошибок TLS
					this->_coder->on(this->_tid, std::bind(&socks5_t::errorTLS, this, _1, _2, _3));
					// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
					this->_coder->on(this->_tid, std::bind(&socks5_t::processTLS, this, _1, _2, _3, _4));
				}
				// Устанавливаем полученный порт
				awh_cast <net::attr_net_t *> (attr.get())->port = port;
				// Создаём идентификатор конечной точки для идентификатора события клиента
				const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
				// Добавляем идентификатор события клиента для конечной точки
				return this->_sessions.emplace(endpoint, make_pair(eid, tid)).second;
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
 * @brief Метод добавления идентификатора события клиента для конечной точки
 *
 * @param eid  идентификатор события для добавления
 * @param addr адрес хоста для добавления
 * @param port порт хоста для добавления
 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::addEventIdEndpoint(const event::id_t eid, const net::addr_t * addr, const uint16_t port) noexcept {
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
				case 4:
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
				break;
				// Для типа IPv6
				case 16:
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
				break;
			}
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = port;
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Добавляем идентификатор события клиента для конечной точки
			return this->_sessions.emplace(endpoint, make_pair(eid, 0)).second;
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
 * @brief Метод добавления идентификатора события клиента для конечной точки
 *
 * @param eid  идентификатор события для добавления
 * @param tid  идентификатор TLS для добавления
 * @param addr адрес хоста для добавления
 * @param port порт хоста для добавления
 * @return     результат выполнения добавления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::addEventIdEndpoint(const event::id_t eid, tls::coder_t::id_t tid, const net::addr_t * addr, const uint16_t port) noexcept {
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
				case 4:
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
				break;
				// Для типа IPv6
				case 16:
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
				break;
			}
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = port;
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
			if((tid > 0) && (this->_coder != nullptr)){
				// Устанавливаем идентификатор TLS для клиента
				this->_tid = tid;
				// Устанавливаем функцию обратного вызова на событие состояния TLS
				this->_coder->on(this->_tid, std::bind(&socks5_t::stateTLS, this, _1, _2));
				// Устанавливаем функцию обратного вызова на событие ошибок TLS
				this->_coder->on(this->_tid, std::bind(&socks5_t::errorTLS, this, _1, _2, _3));
				// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
				this->_coder->on(this->_tid, std::bind(&socks5_t::processTLS, this, _1, _2, _3, _4));
			}
			// Добавляем идентификатор события клиента для конечной точки
			return this->_sessions.emplace(endpoint, make_pair(eid, tid)).second;
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
 * @brief Метод удаления идентификатора события клиента для конечной точки
 *
 * @param eid  идентификатор события для удаления
 * @param addr адрес хоста для удаления
 * @param port порт хоста для удаления
 * @return     результат выполнения удаления идентификатора события клиента для конечной точки
 */
bool awh::Socks5::delEventIdEndpoint(const event::id_t eid, const net::addr_t * addr, const uint16_t port) noexcept {
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
				case 4:
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
				break;
				// Для типа IPv6
				case 16:
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
				break;
			}
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = port;
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
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
			// Если идентификатор события клиента для конечной точки найден, устанавливаем результат
			result = (this->_sessions.find(endpoint) != this->_sessions.end());
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
					// Создаём объект параметров подключения для идентификатора события клиента
					attr = make_unique <net::attr_net_t> ();
					// Выполняем парсинг IP-адреса
					const_cast <socks5_t *> (this)->_addr = addr;
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
				// Если идентификатор события клиента для конечной точки найден, устанавливаем результат
				result = (this->_sessions.find(endpoint) != this->_sessions.end());
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
 * @param eid  идентификатор события для проверки
 * @param addr адрес хоста для проверки
 * @param port порт хоста для проверки
 * @return     результат проверки наличия идентификатора события клиента для конечной точки
 */
bool awh::Socks5::isEventIdEndpoint(const event::id_t eid, const net::addr_t * addr, const uint16_t port) const noexcept {
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
				case 4:
					// Устанавливаем полученный IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (addr)->address;
				break;
				// Для типа IPv6
				case 16:
					// Устанавливаем полученный IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (addr)->address[0], 16);
				break;
			}
			// Устанавливаем полученный порт
			awh_cast <net::attr_net_t *> (attr.get())->port = port;
			// Создаём идентификатор конечной точки для идентификатора события клиента
			const origin_t endpoint = Origin().from(attr.get(), this->_client->protocol(eid));
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Если идентификатор события клиента для конечной точки найден, устанавливаем результат
			result = (this->_sessions.find(endpoint) != this->_sessions.end());
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
