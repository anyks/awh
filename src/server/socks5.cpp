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
 * @brief Метод изменения статуса сервера
 *
 * @param status новый статус сервера
 * @param state  новое временное состояние сервера
 */
void awh::server::Socks5::status(const event::status_t status, const state_t state) noexcept {
	/**
	 * Временное состояние сервера
	 */
	switch(static_cast <uint8_t> (state)){
		// Если мы получили статус события сервера
		case static_cast <uint8_t> (state_t::SERVER): {
			// Если функция обратного вызова установлена
			if(this->_callback.is("status"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("status", status);
			// Если работа сервера запущена
			if(status == event::status_t::LAUNCHED){
				// Выполняем запуск работы сервера, если сервер не запущен
				if(!this->_server->launch(this->_eid)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("This server ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::WARNING, this->_eid);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("This server ID=%u cannot be started", log_t::flag_t::WARNING, this->_eid);
						#endif
					}
				// Если сервер запущен удачно
				} else {
					// Если функция обратного вызова установлена
					if(this->_callback.is("launch")){
						/**
						 * Определяем семейство адресов с которым работает сервер
						 */
						switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(this->_eid))){
							// Если сервер работает с адресами IPv4
							case static_cast <uint8_t> (event::family_t::IPV4):
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_server->getAddress(this->_eid, event::address_t::IPV4), this->_server->getPort(this->_eid));
							break;
							// Если сервер работает с адресами IPv6
							case static_cast <uint8_t> (event::family_t::IPV6):
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_server->getAddress(this->_eid, event::address_t::IPV6), this->_server->getPort(this->_eid));
							break;
						}
					}
					// Если список поддерживаемых UDP-серверов не пустой
					if(!this->_servers.empty()){
						/**
						 * Выполняем перебор всего списка поддерживаемых UDP-серверов
						 */
						for(const auto & eid : this->_servers){
							// Выполняем запуск работы сервера, если сервер не запущен
							if(!this->_server->launch(eid)){
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("This server ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::WARNING, eid);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("This server ID=%u cannot be started", log_t::flag_t::WARNING, eid);
									#endif
								}
							// Если сервер запущен удачно
							} else {
								// Если функция обратного вызова установлена
								if(this->_callback.is("launch")){
									/**
									 * Определяем семейство адресов с которым работает сервер
									 */
									switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(eid))){
										// Если сервер работает с адресами IPv4
										case static_cast <uint8_t> (event::family_t::IPV4):
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_server->getAddress(eid, event::address_t::IPV4), this->_server->getPort(eid));
										break;
										// Если сервер работает с адресами IPv6
										case static_cast <uint8_t> (event::family_t::IPV6):
											// Выполняем функцию обратного вызова
											this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_server->getAddress(eid, event::address_t::IPV6), this->_server->getPort(eid));
										break;
									}
								}
							}
						}
					}
				}
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
					// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
					this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolveDNS, this, _1, this->_eid, _2, _3, _4);
					// Выполняем резолвинг хоста текущего сервера
					if(!this->_dns->resolve(this->_dns->issue(), awh_cast <unit::unit_t *> (this->_server)->family(this->_eid), this->_host)){
						// Создаём текст ошибки резолвинга хоста текущего сервера
						const string error = this->_fmk->format("It was not possible to obtain an IP address for the host \"%s\"", this->_host.c_str());
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
					// Останавливаем сервер
					this->_server->stop();
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
void awh::server::Socks5::connectClient(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
			auto i = this->_clients.find(eid);
			// Если идентификатор клиента найден в списке
			if(i != this->_clients.end()){
				// Ищем идентификатор пира в списке сопоставления идентификаторов пиров с удалёнными клиентами
				auto j = this->_peers.find(i->second);
				// Если идентификатор пира найден в списке
				if(j != this->_peers.end()){
					// Размер буфера данных
					size_t size = 0;
					// Буфер данных ответа
					uint8_t * buffer = nullptr;
					// Создаём объект параметров подключения для хоста сервера
					j->second.ctx.host = make_unique <net::attr_net_t> ();
					/**
					 * Определяем семейство адресов для хоста сервера
					 */
					switch(static_cast <uint8_t> (this->_unit.family(eid))){
						// Если семейство адресов соответствует IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Устанавливаем тип адреса как IPv4
							j->second.ctx.host->type = net::type_t::IPV4;
							// Устанавливаем IP-адрес хоста сервера
							this->_server->getAddress(this->_eid, event::address_t::IPV4, awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip);
						} break;
						// Если семейство адресов соответствует IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Устанавливаем тип адреса как IPv6
							j->second.ctx.host->type = net::type_t::IPV6;
							// Создаём новый объект адреса клиента IPv6
							awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							// Устанавливаем IP-адрес хоста сервера
							this->_server->getAddress(this->_eid, event::address_t::IPV6, awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->ip);
						} break;
					}
					// Устанавливаем порт хоста сервера
					awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->port = this->_unit.getInternalPort(eid);
					// Если извлечение буфера данных ответа выполнено успешно
					if(this->_socks5.buffer(&buffer, size, j->second.ctx)){
						// Если отправка ответа прокси-клиенту не выполнена
						if(this->_server->send(i->second, buffer, size) != size){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Failed to send data to client", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
								#endif
							}
							// Удаляем подключённого пира
							this->_server->destroy(i->second);
							// Удаляем пира из списка активных пиров
							this->_peers.erase(j);
							// Удаляем клиента из списка активных клиентов
							this->_clients.erase(i);
						// Если отправка ответа прокси-клиенту выполнена успешно
						} else {
							// Если статус ответа от прокси-сервера соответствует успешному выполнению команды
							if(j->second.ctx.status == proto::socks5_t::status_t::SUCCESS){
								// Выполняем объединение событий пира и принадлежащего ему клиента
								if(!this->_server->splice(i->second, j->second.eid) || !this->_server->splice(j->second.eid, i->second)){
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::WARNING, i->second);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, i->second);
										#endif
									}
									// Выполняем поиск пира, которому принадлежит идентификатор
									auto j = this->_peers.find(i->second);
									// Если пир для этого идентификатора найден
									if(j != this->_peers.end()){
										// Удаляем подключённого пира
										this->_server->destroy(i->second);
										// Удаляем пира из списка активных пиров
										this->_peers.erase(j);
										// Удаляем клиента из списка активных клиентов
										this->_clients.erase(i);
									}
								// Если объединение событий пира и принадлежащего ему клиента выполнено успешно
								} else {
									// Устанавливаем статус успешного выполнения команды
									j->second.ctx.state = proto::socks5_t::state_t::COMPLETED;
									// Если функция обратного вызова установлена
									if(this->_callback.is("connect"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::id_t, const bool)> ("connect", this->_eid, i->second, false);
								}
							// Если статус ответа от прокси-сервера как запрещённый, так и не поддерживаемый
							} else j->second.ctx.state = proto::socks5_t::state_t::BROKEN;
						}
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, ok), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid    идентификатор события клиента
 * @param status новый статус события
 */
void awh::server::Socks5::statusClient(const event::id_t eid, const event::status_t status) noexcept {
	// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
	auto i = this->_clients.find(eid);
	// Если идентификатор клиента найден в списке
	if(i != this->_clients.end()){
		// Извлекаем идентификатор пира, которому принадлежит клиент
		const event::id_t eid = i->second;
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (status)){
			// Если статус уничтожения
			case static_cast <uint8_t> (event::status_t::DESTROYED): {
				// Ищем идентификатор пира в списке сопоставления идентификаторов пиров с удалёнными клиентами
				auto j = this->_peers.find(eid);
				// Если идентификатор пира найден в списке
				if(j != this->_peers.end()){
					// Удаляем пира из списка активных пиров
					this->_peers.erase(j);
					// Удаляем клиента из списка активных клиентов
					this->_clients.erase(i);
					// Удаляем подключённого пира
					this->_server->destroy(eid);
				}
			} break;
		}
		// Если функция обратного вызова установлена
		if(this->_callback.is("state"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
	}
}
/**
 * @brief Метод получения события ошибок
 *
 * @param eid     идентификатор события
 * @param error   код ошибки
 * @param message сообщение об ошибке
 */
void awh::server::Socks5::errorClient(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("error")){
		// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
		auto i = this->_clients.find(eid);
		// Если идентификатор клиента найден в списке
		if(i != this->_clients.end())
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", i->second, error, message);
	}
}
/**
 * @brief Метод обработки событий истечения таймаута клиента
 *
 * @param eid    идентификатор клиента
 * @param action тип действия для истекшего таймаута
 * @param delay  задержка таймаута в миллисекундах
 * @return       нужно ли завершить клиента после истечения таймаута
 */
bool awh::server::Socks5::timeoutClient(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("timeout")){
			// Ищем идентификатор клиента в списке сопоставления идентификаторов клиентов с пирами которым они принадлежат
			auto i = this->_clients.find(eid);
			// Если идентификатор клиента найден в списке
			if(i != this->_clients.end())
				// Выполняем функцию обратного вызова
				return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", i->second, action, delay);
		}
	}
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки события разрешения подключения
 *
 * @param eid идентификатор сервера
 * @param cid идентификатор клиента
 */
void awh::server::Socks5::accept(const event::id_t eid, const event::id_t cid) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если идентификатор сервера соответствует идентификатору socks5-сервера
		if(eid == this->_eid){
			// Добавляем пира в список активных пиров
			auto ret = this->_peers.emplace(cid, peer_t{});
			// Если пир был добавлен успешно
			if(ret.second){
				// Устананавливаем опции события
				if(this->_server->setOptions(cid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
					// Размер буфера данных
					size_t size = 0;
					// Буфер данных запроса
					uint8_t * buffer = nullptr;
					// Если извлечение буфера данных запроса выполнено успешно
					if(this->_socks5.buffer(&buffer, size, ret.first->second.ctx)){
						// Если отправка запроса на прокси-сервер не выполнена
						if(this->_server->send(cid, buffer, size) != size){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Failed to send data to remote server", __PRETTY_FUNCTION__, make_tuple(cid), log_t::flag_t::WARNING);
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
		// Если идентификатор сервера соответствует идентификатору одного из поддерживаемых UDP-серверов
		} else if(this->_servers.find(eid) != this->_servers.end()) {
			/*
			// Если функция обратного вызова установлена
			if(this->_callback.is("accept"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", eid, cid, 0);
			 */
		}
	}
}
/**
 * @brief Метод обработки событий изменения состояния сервера
 *
 * @param eid    идентификатор клиента
 * @param status новый статус сервера
 */
void awh::server::Socks5::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
		// Если статус сервера изменился на "уничтожен"
		if(status == event::status_t::DESTROYED){
			// Если производится завершение работы текущего сервера или одного из поддерживаемых UDP-серверов
			if((eid == this->_eid) || (this->_servers.find(eid) != this->_servers.end())){
				// Если объект DNS-резолвера установлен
				if(this->_dns != nullptr)
					// Останавливаем событие DNS-резолвера
					this->_dns->stop();
				// Если производится завершение работы клиента подключенного к текущему серверу
				else this->_server->stop();
			// Если производится завершение работы клиента подключенного к одному из серверов
			} else {
				// Выполняем поиск пира, которому принадлежит идентификатор
				auto i = this->_peers.find(eid);
				// Если пир для этого идентификатора найден
				if(i != this->_peers.end()){
					// Выполняем поиск идентификатора клиента принадлежащего этому пиру
					auto j = this->_clients.find(i->second.eid);
					// Если идентификатор клиента найден в списке
					if(j != this->_clients.end()){
						// Удаляем подключённого клиента
						this->_unit.destroy(j->first);
						// Удаляем клиента из списка активных клиентов
						this->_clients.erase(j);
					}
					// Удаляем пира из списка активных пиров
					this->_peers.erase(i);
				}
			}
		}
	}
}
/**
 * @brief Метод обработки событий получения данных сервером
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных сервера
 * @param size   размер данных сервера
 */
void awh::server::Socks5::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск пира, которому принадлежит идентификатор
			auto i = this->_peers.find(eid);
			// Если пир для этого идентификатора найден
			if(i != this->_peers.end()){
				// Если текущее состояние соответствует завершённому состоянию
				if(i->second.ctx.state == proto::socks5_t::state_t::COMPLETED){

				// Если текущее состояние находится ещё в процессе общения с socks5 прокси-клиентом
				} else {
					// Если парсинг данных от прокси-клиента выполнен успешно
					if(this->_socks5.parse(buffer, size, i->second.ctx)){
						/**
						 * Определяем состояние парсинга данных от прокси-клиента
						 */
						switch(static_cast <uint8_t> (i->second.ctx.state)){
							// Если текущее состояние соответствует ошибке работе с прокси-сервером
							case static_cast <uint8_t> (proto::socks5_t::state_t::BROKEN): {
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::CRITICAL, this->_socks5.statusMessage(i->second.ctx.status));
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socks5.statusMessage(i->second.ctx.status));
									#endif
								// Выполняем функцию обратного вызова
								} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, this->_socks5.statusMessage(i->second.ctx.status));
								// Удаляем подключённого пира
								this->_server->destroy(eid);
								// Удаляем пира из списка активных пиров
								this->_peers.erase(i);
							} break;
							// Если текущее состояние соответствует выполненному рукопожатию
							case static_cast <uint8_t> (proto::socks5_t::state_t::HANDSHAKE): {
								/**
								 * Определяем команду, которую выполняет прокси-клиент
								 */
								switch(static_cast <uint8_t> (i->second.ctx.command)){
									// Если команда соответствует CONNECT
									case static_cast <uint8_t> (proto::socks5_t::command_t::CONNECT): {
										/**
										 * Определяем тип адреса хоста для подключения
										 */
										switch(static_cast <uint8_t> (i->second.ctx.host->type)){
											// Если тип адреса соответствует FQDN
											case static_cast <uint8_t> (net::type_t::FQDN): {
												// Если DNS-резолвер не установлен или не находится в рабочем состоянии
												if((this->_dns == nullptr) || !this->_dns->working())
													// Устанавливаем статус ошибки, так как команда не поддерживается
													i->second.ctx.status = proto::socks5_t::status_t::NOCOMMAND;
												// Если DNS-резолвер установлен и находится в рабочем состоянии
												else {
													// Выполняем добавление связи DNS-резолвера и идентификатора пира
													auto ret = this->_resolves.emplace(this->_dns->issue(), eid);
													// Выполняем резолвинг хоста текущего сервера
													if(!this->_dns->resolve(ret.first->first, awh_cast <unit::unit_t *> (this->_server)->family(eid), awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->domain)){
														// Создаём текст ошибки резолвинга хоста текущего сервера
														const string error = this->_fmk->format("It was not possible to obtain an IP address for the remote host \"%s\"", awh_cast <net::attr_fqdn_t *> (i->second.ctx.host.get())->domain.c_str());
														// Если функция обратного вызова не установлена
														if(!this->_callback.is("error")){
															/**
															 * Если включён режим отладки
															 */
															#if DEBUG_MODE
																// Выводим сообщение об ошибке
																this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING, error.c_str());
															/**
															 * Если режим отладки не включён
															 */
															#else
																// Выводим сообщение об ошибке
																this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
															#endif
														// Выполняем функцию обратного вызова
														} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::NOT_FOUND, error);
													// Если резолвинг хоста не выполнен, выходим
													} else return;
												}
											} break;
											// Если тип адреса соответствует IPv4
											case static_cast <uint8_t> (net::type_t::IPV4):
											// Если тип адреса соответствует IPv6
											case static_cast <uint8_t> (net::type_t::IPV6): {
												// Получаем семейство адресов для подключения к удалённому серверу
												const event::family_t family = awh_cast <unit::unit_t *> (this->_server)->family(eid);
												// Выполняем создание клиента для подключения к удалённому серверу
												i->second.eid = this->_unit.issue(family, event::type_t::STREAM, event::protocol_t::TCP);
												// Устананавливаем опции события
												if(this->_unit.setOptions(i->second.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
													// Устанавливаем интерфейс для подключения к удалённому серверу
													if(this->_unit.setIface(i->second.eid, this->_interface)){
														// Устанавливаем порт и адрес удалённого сервера для подключения
														if(this->_unit.setPort(i->second.eid, awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port) &&
														   this->_unit.setTarget(i->second.eid, awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->ip.get())){
															// Если функция обратного вызова установлена
															if(this->_callback.is("ready")){
																// Получаем IP-адрес для подключения к удалённому серверу
																const string & address = this->_unit.getTarget(i->second.eid);
																// Выполняем функцию обратного вызова
																this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", eid, family, address, address);
															}
															// Если функция обратного вызова установлена
															if(this->_callback.is("accept"))
																// Выполняем функцию обратного вызова
																this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", this->_eid, eid, 0);
															// Выполняем фиксацию настроек события сервера
															if(this->_unit.commit(i->second.eid)){
																// Если подключение к серверу прошло успешно
																if(this->_unit.connect(i->second.eid)){
																	// Выполняем запуск события
																	if(!this->_unit.launch(i->second.eid)){
																		// Если функция обратного вызова не установлена
																		if(!this->_callback.is("error")){
																			/**
																			 * Если включён режим отладки
																			 */
																			#if DEBUG_MODE
																				// Выводим сообщение об ошибке
																				this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING, eid);
																			/**
																			 * Если режим отладки не включён
																			 */
																			#else
																				// Выводим сообщение об ошибке
																				this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, eid);
																			#endif
																		}
																	// Если резолвинг хоста не выполнен
																	} else {
																		// Добавляем связь между клиентом и пиром которому он принадлежит
																		this->_clients.emplace(i->second.eid, eid);
																		// Выходим из функции
																		return;
																	}
																// Если подключение к серверу не прошло успешно
																} else {
																	// Если функция обратного вызова не установлена
																	if(!this->_callback.is("error")){
																		/**
																		 * Если включён режим отладки
																		 */
																		#if DEBUG_MODE
																			// Выводим сообщение об ошибке
																			this->_log->debug("Connection to the server \"%s:%u\" is failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, this->_unit.getTarget(i->second.eid).c_str(), awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port);
																		/**
																		 * Если режим отладки не включён
																		 */
																		#else
																			// Выводим сообщение об ошибке
																			this->_log->print("Connection to the server \"%s:%u\" is failed", log_t::flag_t::WARNING, this->_unit.getTarget(i->second.eid).c_str(), awh_cast <net::attr_net_t *> (i->second.ctx.host.get())->port);
																		#endif
																	}
																}
															// Если фиксация настроек события сервера не выполнена
															} else {
																// Если функция обратного вызова не установлена
																if(!this->_callback.is("error")){
																	/**
																	 * Если включён режим отладки
																	 */
																	#if DEBUG_MODE
																		// Выводим сообщение об ошибке
																		this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
																	/**
																	 * Если режим отладки не включён
																	 */
																	#else
																		// Выводим сообщение об ошибке
																		this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, eid);
																	#endif
																}
															}
														// Если установка порта и адреса удалённого сервера для подключения не выполнена
														} else {
															// Если функция обратного вызова не установлена
															if(!this->_callback.is("error")){
																/**
																 * Если включён режим отладки
																 */
																#if DEBUG_MODE
																	// Выводим сообщение об ошибке
																	this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
																/**
																 * Если режим отладки не включён
																 */
																#else
																	// Выводим сообщение об ошибке
																	this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, eid);
																#endif
															}
														}
													// Если установка интерфейса для подключения к удалённому серверу не выполнена
													} else {
														// Если функция обратного вызова не установлена
														if(!this->_callback.is("error")){
															/**
															 * Если включён режим отладки
															 */
															#if DEBUG_MODE
																// Выводим сообщение об ошибке
																this->_log->debug("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, this->_interface.c_str(), eid);
															/**
															 * Если режим отладки не включён
															 */
															#else
																// Выводим сообщение об ошибке
																this->_log->print("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, this->_interface.c_str(), eid);
															#endif
														}
													}
												// Если установка опций события не выполнена
												} else {
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING, eid);
														/**
														 * Если режим отладки не включён
														 */
														#else
															// Выводим сообщение об ошибке
															this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, eid);
														#endif
													}
												}
												// Удаляем клиента принадлежащего пиру
												this->_unit.destroy(i->second.eid);
												// Устанавливаем статус ошибки, так как мы получили ошибку
												i->second.ctx.status = proto::socks5_t::status_t::NOADDR;
											} break;
										}
									} break;
									// Если команда соответствует UDP ASSOCIATE
									case static_cast <uint8_t> (proto::socks5_t::command_t::UDP): {

										cout << "^^^^^^^^^^^^^^^^^^^^ Received data from client ID=" << eid << ": " << size << " bytes" << " || " << (u_short) i->second.ctx.command << endl;
									
									} break;
									// В остальных случаях
									default:
										// Устанавливаем статус ошибки, так как команда не поддерживается
										i->second.ctx.status = proto::socks5_t::status_t::NOCOMMAND;
								}
								// Размер буфера данных
								size_t size = 0;
								// Буфер данных ответа
								uint8_t * buffer = nullptr;
								// Если извлечение буфера данных ответа выполнено успешно
								if(this->_socks5.buffer(&buffer, size, i->second.ctx)){
									// Если отправка ответа прокси-клиенту не выполнена
									if(this->_server->send(eid, buffer, size) != size){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("Failed to send data to client", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
											#endif
										}
										// Удаляем подключённого пира
										this->_server->destroy(eid);
										// Удаляем пира из списка активных пиров
										this->_peers.erase(i);
									// Если отправка ответа прокси-клиенту выполнена успешно
									} else {
										// Если статус ответа от прокси-сервера соответствует успешному выполнению команды
										if(i->second.ctx.status == proto::socks5_t::status_t::SUCCESS)
											// Устанавливаем статус успешного выполнения команды
											i->second.ctx.state = proto::socks5_t::state_t::COMPLETED;
										// Если статус ответа от прокси-сервера как запрещённый, так и не поддерживаемый
										else i->second.ctx.state = proto::socks5_t::state_t::BROKEN;
									}
								}
							} break;
							// В остальных случаях, проходим процедуру общения с клиентом в автоматическом режиме
							default: {
								// Размер буфера данных
								size_t size = 0;
								// Буфер данных ответа
								uint8_t * buffer = nullptr;
								// Если извлечение буфера данных ответа выполнено успешно
								if(this->_socks5.buffer(&buffer, size, i->second.ctx)){
									// Если отправка ответа прокси-клиенту не выполнена
									if(this->_server->send(eid, buffer, size) != size){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("Failed to send data to client", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
											#endif
										}
										// Удаляем подключённого пира
										this->_server->destroy(eid);
										// Удаляем пира из списка активных пиров
										this->_peers.erase(i);
									}
								}
							}
						}
					// Если парсинг данных от прокси-клиента не выполнен
					} else {
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to parse data from proxy client", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to parse data from proxy client", log_t::flag_t::WARNING);
							#endif
						// Выполняем функцию обратного вызова
						} else this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, event::error_t::CONNECTION_FAIL, "Failed to parse data from proxy client");
						// Выполняем поиск идентификатора клиента принадлежащего этому пиру
						auto j = this->_clients.find(i->second.eid);
						// Если идентификатор клиента найден в списке
						if(j != this->_clients.end()){
							// Удаляем подключённого клиента
							this->_unit.destroy(j->first);
							// Удаляем клиента из списка активных клиентов
							this->_clients.erase(j);
						}
						// Удаляем подключённого пира
						this->_server->destroy(eid);
						// Удаляем пира из списка активных пиров
						this->_peers.erase(i);
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
			// Выполняем поиск пира, которому принадлежит идентификатор
			auto i = this->_peers.find(eid);
			// Если пир для этого идентификатора найден
			if(i != this->_peers.end()){
				// Выполняем поиск идентификатора клиента принадлежащего этому пиру
				auto j = this->_clients.find(i->second.eid);
				// Если идентификатор клиента найден в списке
				if(j != this->_clients.end()){
					// Удаляем подключённого клиента
					this->_unit.destroy(j->first);
					// Удаляем клиента из списка активных клиентов
					this->_clients.erase(j);
				}
				// Удаляем подключённого пира
				this->_server->destroy(eid);
				// Удаляем пира из списка активных пиров
				this->_peers.erase(i);
			}
		}
	}
}
/**
 * @brief Метод резолвинга доменного имени удалённого хоста в сетевой адрес
 *
 * @param id     идентификатор DNS-запроса
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для резолвинга
 * @param addr   указатель на структуру для хранения результата резолвинга
 */
void awh::server::Socks5::resolve(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск идентификатора DNS-запроса для получения идентификатора пира, которому принадлежит этот DNS-запрос
		auto i = this->_resolves.find(id);
		// Если идентификатор DNS-запроса найден
		if(i != this->_resolves.end()){
			// Выполняем поиск пира, которому принадлежит идентификатор пира
			auto j = this->_peers.find(i->second);
			// Если пир для этого идентификатора найден
			if(j != this->_peers.end()){
				// Выполняем создание клиента для подключения к удалённому серверу
				j->second.eid = this->_unit.issue(awh_cast <unit::unit_t *> (this->_server)->family(i->second), event::type_t::STREAM, event::protocol_t::TCP);
				// Устананавливаем опции события
				if(this->_unit.setOptions(j->second.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY)){
					// Устанавливаем интерфейс для подключения к удалённому серверу
					if(this->_unit.setIface(j->second.eid, this->_interface)){
						// Устанавливаем порт и адрес удалённого сервера для подключения
						if(this->_unit.setPort(j->second.eid, awh_cast <net::attr_fqdn_t *> (j->second.ctx.host.get())->port) && this->_unit.setTarget(j->second.eid, addr)){
							// Если функция обратного вызова установлена
							if(this->_callback.is("ready"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", i->second, family, domain, this->_unit.getTarget(j->second.eid));
							// Если функция обратного вызова установлена
							if(this->_callback.is("accept"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", this->_eid, i->second, 0);
							// Выполняем фиксацию настроек события сервера
							if(this->_unit.commit(j->second.eid)){
								// Если подключение к серверу прошло успешно
								if(this->_unit.connect(j->second.eid)){
									// Выполняем запуск события
									if(!this->_unit.launch(j->second.eid)){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error")){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("Creating client for peer ID=%u is failed", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("Creating client for peer ID=%u is failed", log_t::flag_t::WARNING, i->second);
											#endif
										}
									// Если резолвинг хоста не выполнен
									} else {
										// Добавляем связь между клиентом и пиром которому он принадлежит
										this->_clients.emplace(j->second.eid, i->second);
										// Удаляем связь DNS-резолвера и идентификатора пира
										this->_resolves.erase(i);
										// Выходим из функции
										return;
									}
								// Если подключение к серверу не прошло успешно
								} else {
									// Если функция обратного вызова не установлена
									if(!this->_callback.is("error")){
										/**
										 * Если включён режим отладки
										 */
										#if DEBUG_MODE
											// Выводим сообщение об ошибке
											this->_log->debug("Connection to the server \"%s:%u\" is failed", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, domain.c_str(), awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->port);
										/**
										 * Если режим отладки не включён
										 */
										#else
											// Выводим сообщение об ошибке
											this->_log->print("Connection to the server \"%s:%u\" is failed", log_t::flag_t::WARNING, domain.c_str(), awh_cast <net::attr_net_t *> (j->second.ctx.host.get())->port);
										#endif
									}
								}
							// Если фиксация настроек события сервера не выполнена
							} else {
								// Если функция обратного вызова не установлена
								if(!this->_callback.is("error")){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("Client parameters were not committed for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
									/**
									 * Если режим отладки не включён
									 */
									#else
										// Выводим сообщение об ошибке
										this->_log->print("Client parameters were not committed for node with ID=%u", log_t::flag_t::WARNING, i->second);
									#endif
								}
							}
						// Если установка порта и адреса удалённого сервера для подключения не выполнена
						} else {
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("Port and address of the remote server for connection were not set correctly for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Выводим сообщение об ошибке
									this->_log->print("Port and address of the remote server for connection were not set correctly for node with ID=%u", log_t::flag_t::WARNING, i->second);
								#endif
							}
						}
					// Если установка интерфейса для подключения к удалённому серверу не выполнена
					} else {
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, this->_interface.c_str(), i->second);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Network interface \"%s\" for connecting to the remote server could not be established for node with ID=%u", log_t::flag_t::WARNING, this->_interface.c_str(), i->second);
							#endif
						}
					}
				// Если установка опций события не выполнена
				} else {
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Failed to configure client events settings for node with ID=%u", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING, i->second);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Failed to configure client events settings for node with ID=%u", log_t::flag_t::WARNING, i->second);
						#endif
					}
				}
				// Удаляем клиента принадлежащего пиру
				this->_unit.destroy(j->second.eid);
				// Устанавливаем статус ошибки, так как мы получили ошибку
				j->second.ctx.status = proto::socks5_t::status_t::NOADDR;
				// Размер буфера данных
				size_t size = 0;
				// Буфер данных ответа
				uint8_t * buffer = nullptr;
				// Если извлечение буфера данных ответа выполнено успешно
				if(this->_socks5.buffer(&buffer, size, j->second.ctx)){
					// Если отправка ответа прокси-клиенту не выполнена
					if(this->_server->send(i->second, buffer, size) != size){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Failed to send data to client", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Failed to send data to client", log_t::flag_t::WARNING);
							#endif
						}
						// Удаляем подключённого пира
						this->_server->destroy(i->second);
						// Удаляем пира из списка активных пиров
						this->_peers.erase(i->second);
					// Если отправка ответа прокси-клиенту выполнена успешно, устанавливаем статус ответа от прокси-сервера
					} else j->second.ctx.state = proto::socks5_t::state_t::BROKEN;
				}
			}
			// Удаляем связь DNS-резолвера и идентификатора пира
			this->_resolves.erase(i);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
		// Выполняем поиск идентификатора DNS-запроса для получения идентификатора пира, которому принадлежит этот DNS-запрос
		auto i = this->_resolves.find(id);
		// Если идентификатор DNS-запроса найден
		if(i != this->_resolves.end()){
			// Выполняем поиск пира, которому принадлежит идентификатор
			auto j = this->_peers.find(i->second);
			// Если пир для этого идентификатора найден
			if(j != this->_peers.end()){
				// Удаляем подключённого пира
				this->_server->destroy(i->second);
				// Удаляем пира из списка активных пиров
				this->_peers.erase(j);
			}
			// Удаляем связь DNS-резолвера и идентификатора пира
			this->_resolves.erase(i);
		}
	}
}
/**
 * @brief Метод резолвинга доменного имени в сетевой адрес
 *
 * @param id     идентификатор DNS-запроса
 * @param eid    идентификатор события сервера
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для резолвинга
 * @param addr   указатель на структуру для хранения результата резолвинга
 */
void awh::server::Socks5::resolveDNS(const unit::dns_t::id_t id, const event::id_t eid, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns->working()){
		// Если идентификатор события сервера соответствует идентификатору сервера текущего объекта
		if(eid == this->_eid){
			/**
			 * Определяем семейство адресов с которым работает сервер
			 */
			switch(static_cast <uint8_t> (family)){
				// Если сервер работает с адресами IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Устанавливаем адрес хоста целевой текущей машины
					if(this->_server->setAddress(this->_eid, event::address_t::IPV4, addr)){
						/**
						 * В зависимости от статуса события сервера выполняем запуск
						 */
						switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->status(this->_eid))){
							// Если событие сервера не запущено, запускаем его
							case static_cast <uint8_t> (event::status_t::NONE): {
								// Если событие сервера не запущено, запускаем его
								if(this->_server->commit(this->_eid)){
									// Если функция обратного вызова установлена
									if(this->_callback.is("ready"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, family, domain, this->_server->getAddress(this->_eid, event::address_t::IPV4));
									// Если список поддерживаемых UDP-серверов пустой
									if(this->_servers.empty()){
										// Запускаем сервер
										this->_server->start();
										// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
										this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
									// Если список поддерживаемых UDP-серверов не пустой
									} else {
										// Получаем идентификатор UDP-сервера
										const event::id_t eid = (* this->_servers.begin());
										// Выполняем поиск хоста принадлежащему этому UDP-серверу
										auto i = this->_hosts.find(eid);
										// Если хост для этого UDP-сервера найден
										if(i != this->_hosts.end()){
											// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
											this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolveDNS, this, _1, eid, _2, _3, _4);
											// Выполняем резолвинг хоста текущего сервера
											if(!this->_dns->resolve(this->_dns->issue(), family, i->second)){
												// Создаём текст ошибки резолвинга хоста текущего сервера
												const string error = this->_fmk->format("It was not possible to obtain an IP address for the host \"%s\"", i->second.c_str());
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING, error.c_str());
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
										}
									}
								}
							} break;
							// Если событие сервера инициализировано, запускаем его
							case static_cast <uint8_t> (event::status_t::INITIAL):
							// Если событие находится в состоянии успешного подключения
							case static_cast <uint8_t> (event::status_t::SUCCESS): {
								// Запускаем сервер
								this->_server->start();
								// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
								this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
							} break;
						}
					}
				} break;
				// Если сервер работает с адресами IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Устанавливаем адрес хоста целевой текущей машины
					if(this->_server->setAddress(this->_eid, event::address_t::IPV6, addr)){
						/**
						 * В зависимости от статуса события сервера выполняем запуск
						 */
						switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->status(this->_eid))){
							// Если событие сервера не запущено, запускаем его
							case static_cast <uint8_t> (event::status_t::NONE): {
								// Если событие сервера не запущено, запускаем его
								if(this->_server->commit(this->_eid)){
									// Если функция обратного вызова установлена
									if(this->_callback.is("ready"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, family, domain, this->_server->getAddress(this->_eid, event::address_t::IPV6));
									// Если список поддерживаемых UDP-серверов пустой
									if(this->_servers.empty()){
										// Запускаем сервер
										this->_server->start();
										// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
										this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
									// Если список поддерживаемых UDP-серверов не пустой
									} else {
										// Получаем идентификатор UDP-сервера
										const event::id_t eid = (* this->_servers.begin());
										// Выполняем поиск хоста принадлежащему этому UDP-серверу
										auto i = this->_hosts.find(eid);
										// Если хост для этого UDP-сервера найден
										if(i != this->_hosts.end()){
											// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
											this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolveDNS, this, _1, eid, _2, _3, _4);
											// Выполняем резолвинг хоста текущего сервера
											if(!this->_dns->resolve(this->_dns->issue(), family, i->second)){
												// Создаём текст ошибки резолвинга хоста текущего сервера
												const string error = this->_fmk->format("It was not possible to obtain an IP address for the host \"%s\"", i->second.c_str());
												// Если функция обратного вызова не установлена
												if(!this->_callback.is("error")){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING, error.c_str());
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
										}
									}
								}
							} break;
							// Если событие сервера инициализировано, запускаем его
							case static_cast <uint8_t> (event::status_t::INITIAL):
							// Если событие находится в состоянии успешного подключения
							case static_cast <uint8_t> (event::status_t::SUCCESS): {
								// Запускаем сервер
								this->_server->start();
								// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
								this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
							} break;
						}
					}
				} break;
			}
		// Если идентификатор события сервера принадлежит UDP-серверу текущего объекта
		} else {
			/**
			 * Определяем семейство адресов с которым работает сервер
			 */
			switch(static_cast <uint8_t> (family)){
				// Если сервер работает с адресами IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Устанавливаем адрес хоста целевой текущей машины
					if(this->_server->setAddress(eid, event::address_t::IPV4, addr)){
						/**
						 * В зависимости от статуса события сервера выполняем запуск
						 */
						switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->status(eid))){
							// Если событие сервера не запущено, запускаем его
							case static_cast <uint8_t> (event::status_t::NONE): {
								// Если событие сервера не запущено, запускаем его
								if(this->_server->commit(eid)){
									// Если функция обратного вызова установлена
									if(this->_callback.is("ready"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", eid, family, domain, this->_server->getAddress(eid, event::address_t::IPV4));
									// Выполняем поиск хоста принадлежащему этому UDP-серверу
									auto i = this->_servers.find(eid);
									// Если хост для этого UDP-сервера найден
									if(i != this->_servers.end()){
										// Переходим к следующему UDP-серверу, если это не последний UDP-сервер в списке
										if(++i != this->_servers.end()){
											// Получаем идентификатор UDP-сервера
											const event::id_t eid = (* i);
											// Выполняем поиск хоста принадлежащему этому UDP-серверу
											auto i = this->_hosts.find(eid);
											// Если хост для этого UDP-сервера найден
											if(i != this->_hosts.end()){
												// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
												this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolveDNS, this, _1, eid, _2, _3, _4);
												// Выполняем резолвинг хоста текущего сервера
												if(!this->_dns->resolve(this->_dns->issue(), family, i->second)){
													// Создаём текст ошибки резолвинга хоста текущего сервера
													const string error = this->_fmk->format("It was not possible to obtain an IP address for the host \"%s\"", i->second.c_str());
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING, error.c_str());
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
											}
										// Если все UDP-серверы в списке были проверены
										} else {
											// Запускаем сервер
											this->_server->start();
											// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
											this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
										}
									}
								}
							} break;
							// Если событие сервера инициализировано, запускаем его
							case static_cast <uint8_t> (event::status_t::INITIAL):
							// Если событие находится в состоянии успешного подключения
							case static_cast <uint8_t> (event::status_t::SUCCESS): {
								// Запускаем сервер
								this->_server->start();
								// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
								this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
							} break;
						}
					}
				} break;
				// Если сервер работает с адресами IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Устанавливаем адрес хоста целевой текущей машины
					if(this->_server->setAddress(eid, event::address_t::IPV6, addr)){
						/**
						 * В зависимости от статуса события сервера выполняем запуск
						 */
						switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->status(eid))){
							// Если событие сервера не запущено, запускаем его
							case static_cast <uint8_t> (event::status_t::NONE): {
								// Если событие сервера не запущено, запускаем его
								if(this->_server->commit(eid)){
									// Если функция обратного вызова установлена
									if(this->_callback.is("ready"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", eid, family, domain, this->_server->getAddress(eid, event::address_t::IPV6));
									// Выполняем поиск хоста принадлежащему этому UDP-серверу
									auto i = this->_servers.find(eid);
									// Если хост для этого UDP-сервера найден
									if(i != this->_servers.end()){
										// Переходим к следующему UDP-серверу, если это не последний UDP-сервер в списке
										if(++i != this->_servers.end()){
											// Получаем идентификатор UDP-сервера
											const event::id_t eid = (* i);
											// Выполняем поиск хоста принадлежащему этому UDP-серверу
											auto i = this->_hosts.find(eid);
											// Если хост для этого UDP-сервера найден
											if(i != this->_hosts.end()){
												// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
												this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolveDNS, this, _1, eid, _2, _3, _4);
												// Выполняем резолвинг хоста текущего сервера
												if(!this->_dns->resolve(this->_dns->issue(), family, i->second)){
													// Создаём текст ошибки резолвинга хоста текущего сервера
													const string error = this->_fmk->format("It was not possible to obtain an IP address for the host \"%s\"", i->second.c_str());
													// Если функция обратного вызова не установлена
													if(!this->_callback.is("error")){
														/**
														 * Если включён режим отладки
														 */
														#if DEBUG_MODE
															// Выводим сообщение об ошибке
															this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING, error.c_str());
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
											}
										// Если все UDP-серверы в списке были проверены
										} else {
											// Запускаем сервер
											this->_server->start();
											// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
											this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
										}
									}
								}
							} break;
							// Если событие сервера инициализировано, запускаем его
							case static_cast <uint8_t> (event::status_t::INITIAL):
							// Если событие находится в состоянии успешного подключения
							case static_cast <uint8_t> (event::status_t::SUCCESS): {
								// Запускаем сервер
								this->_server->start();
								// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
								this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server::Socks5::resolve, this, _1, _2, _3, _4);
							} break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::server::Socks5::stop() noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если идентификатор сервера установлен
		if(this->_eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns->stop();
			// Если объект DNS-резолвера не установлен, останавливаем событие сервера
			else this->_server->stop();
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::server::Socks5::start() noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_server->working()){
		// Если функция обратного вызова для аутентификации установлена
		if(this->_callback.is("auth"))
			// Устанавливаем функцию обратного вызова для аутентификации
			this->_socks5.on(this->_callback.get <bool (const string &, const string &)> ("auth"));
		// Если идентификатор сервера установлен
		if(this->_eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns != nullptr)
				// Запускаем событие DNS-резолвера
				this->_dns->start();
			// Если объект DNS-резолвера не установлен
			else {
				/**
				 * В зависимости от статуса события сервера выполняем запуск
				 */
				switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->status(this->_eid))){
					// Если событие сервера не запущено, запускаем его
					case static_cast <uint8_t> (event::status_t::NONE): {
						// Если событие сервера не запущено, запускаем его
						if(this->_server->commit(this->_eid)){
							// Если функция обратного вызова установлена
							if(this->_callback.is("ready")){
								// Хост текущего сервера
								string host = "";
								/**
								 * Определяем семейство адресов с которым работает сервер
								 */
								switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(this->_eid))){
									// Если сервер работает с адресами Unix Domain Socket
									case static_cast <uint8_t> (event::family_t::UDS):
										// Извлекаем адрес хоста текущей машины для адресов Unix Domain Socket
										host = ::move(this->_server->getAddress(this->_eid, event::address_t::UDS));
									break;
									// Если сервер работает с адресами IPv4
									case static_cast <uint8_t> (event::family_t::IPV4):
										// Извлекаем адрес хоста текущей машины для адресов IPv4
										host = ::move(this->_server->getAddress(this->_eid, event::address_t::IPV4));
									break;
									// Если сервер работает с адресами IPv6
									case static_cast <uint8_t> (event::family_t::IPV6):
										// Извлекаем адрес хоста текущей машины для адресов IPv6
										host = ::move(this->_server->getAddress(this->_eid, event::address_t::IPV6));
									break;
								}
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, this->_server->family(this->_eid), host, host);
								// Если список серверов UDP не пустой
								if(!this->_servers.empty()){
									// Выполняем итерацию по списку серверов UDP
									for(const auto & eid : this->_servers){
										// Если событие сервера не запущено, запускаем его
										if(this->_server->commit(eid)){
											/**
											 * Определяем семейство адресов с которым работает сервер
											 */
											switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(eid))){
												// Если сервер работает с адресами Unix Domain Socket
												case static_cast <uint8_t> (event::family_t::UDS):
													// Извлекаем адрес хоста текущей машины для адресов Unix Domain Socket
													host = ::move(this->_server->getAddress(eid, event::address_t::UDS));
												break;
												// Если сервер работает с адресами IPv4
												case static_cast <uint8_t> (event::family_t::IPV4):
													// Извлекаем адрес хоста текущей машины для адресов IPv4
													host = ::move(this->_server->getAddress(eid, event::address_t::IPV4));
												break;
												// Если сервер работает с адресами IPv6
												case static_cast <uint8_t> (event::family_t::IPV6):
													// Извлекаем адрес хоста текущей машины для адресов IPv6
													host = ::move(this->_server->getAddress(eid, event::address_t::IPV6));
												break;
											}
											// Выполняем функцию обратного вызова для сервера UDP
											this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", eid, this->_server->family(eid), host, host);
										}
									}
								}
							}
							// Запускаем сервер
							this->_server->start();
						}
					} break;
					// Если событие сервера инициализировано, запускаем его
					case static_cast <uint8_t> (event::status_t::INITIAL):
					// Если событие находится в состоянии успешного подключения
					case static_cast <uint8_t> (event::status_t::SUCCESS):
						// Запускаем сервер
						this->_server->start();
					break;
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::server::Socks5::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для объекта блокировки
	this->_mtx.enabled = mode;
	// Устанавливаем режим безопасности работы потоков для объекта сервера
	server_t::threadSafety(mode);
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
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_server->pause(i->first))
			// Приостанавливаем работу события клиента, принадлежащего подключённому пиру
			return this->_server->pause(i->second.eid);
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
 * @brief Метод возобновления работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 */
bool awh::server::Socks5::resume(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_server->resume(i->first))
			// Возобновляем работу события клиента, принадлежащего подключённому пиру
			return this->_server->resume(i->second.eid);
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
 * @brief Метод уничтожения события клиента
 *
 * @param eid идентификатор события клиента для уничтожения
 */
void awh::server::Socks5::destroy(const event::id_t eid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end()){
			// Уничтожаем событие клиента, принадлежащего подключённому пиру
			this->_server->destroy(i->second.eid);
			// Уничтожаем событие подключённого пира
			this->_server->destroy(i->first);
			// Удаляем идентификатор события подключённого пира из списка
			this->_peers.erase(i);
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
			// Извлекаем опции для события клиента, принадлежащего подключённому пиру
			return this->_server->getOptions(i->second.eid);
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
	return 0;
}
/**
 * @brief Метод установки опций клиента
 *
 * @param eid     идентификатор события клиента
 * @param options опции клиента для установки
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем опции для события клиента, принадлежащего подключённому пиру
			return this->_server->setOptions(i->second.eid, options);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, options), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки опции клиента
 *
 * @param eid    идентификатор события клиента
 * @param option опция клиента для установки
 * @param mode   режим установки опции клиента
 * @return       результат выполнения установки
 */
bool awh::server::Socks5::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем опцию для события клиента, принадлежащего подключённому пиру
			return this->_server->setOption(i->second.eid, option, mode);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, option, mode), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения сетевого интерфейса для подключения к сети клиентов
 *
 * @return сетевой интерфейс сервера
 */
string awh::server::Socks5::getIface() const noexcept {
	// Выводим сетевой интерфейс для подключения к сети клиентов
	return this->_interface;
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @param eid идентификатор события сервера
 * @return    сетевой интерфейс сервера
 */
string awh::server::Socks5::getIface(const event::id_t eid) const noexcept {
	// Если идентификатор события сервера соответствует идентификатору socks5-сервера
	if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end()))
		// Извлекаем сетевой интерфейс для сервера
		return this->_server->getIface(eid);
	// Если идентификатор сервера не установлен
	else {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Извлекаем сетевой интерфейс для события пира
			return this->_server->getIface(i->second.eid);
		// Если идентификатор события подключённого пира не найден
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Server or Peer ID is not set", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Server or Peer ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса для подключения к сети клиентов
 *
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setIface(string_view name) noexcept {
	// Результат работы функции
	bool result = false;
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if((result = (this->_dns != nullptr ? !this->_dns->working() : !this->_server->working())))
		// Устанавливаем сетевой интерфейс для подключения к сети клиентов
		this->_interface = name;
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param eid  идентификатор события сервера
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setIface(const event::id_t eid, string_view name) noexcept {
	// Если идентификатор события сервера соответствует идентификатору socks5-сервера
	if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
		// Если DNS-резолвер или сервер находятся в нерабочем состоянии
		if(this->_dns != nullptr ? !this->_dns->working() : !this->_server->working())
			// Устанавливаем сетевой интерфейс сервера
			return this->_server->setIface(eid, name);
	// Если идентификатор сервера не установлен
	} else {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Устанавливаем сетевой интерфейс для события пира
			return this->_server->setIface(i->second.eid, name);
		// Если идентификатор события подключённого пира не найден
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Server or Peer ID is not set", __PRETTY_FUNCTION__, make_tuple(eid, name), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Server or Peer ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения порта удаленного клиента или текущего сервера
 *
 * @param eid идентификатор события клиента или сервера
 * @return    порт удаленного клиента или текущего сервера
 */
uint16_t awh::server::Socks5::getPort(const event::id_t eid) const noexcept {
	// Если идентификатор события сервера соответствует идентификатору socks5-сервера
	if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end()))
		// Получаем порт сервера
		return this->_server->getPort(eid);
	// Если идентификатор сервера не установлен
	else {
		// Выполняем блокировку потока для работы с локальными данными
		const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if(i != this->_peers.end())
			// Получаем порт удалённого клиента принадлежащего подключённому пиру
			return this->_server->getPort(i->second.eid);
		// Если идентификатор события подключённого пира не найден
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Server or Peer ID is not set", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Server or Peer ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта сервера
 *
 * @param eid  идентификатор события сервера
 * @param port порт сервера для установки
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setPort(const event::id_t eid, const uint16_t port) noexcept {
	// Если идентификатор события сервера соответствует идентификатору socks5-сервера
	if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
		// Если DNS-резолвер или сервер находятся в нерабочем состоянии
		if(this->_dns != nullptr ? !this->_dns->working() : !this->_server->working())
			// Устанавливаем порт сервера
			return this->_server->setPort(eid, port);
	// Если идентификатор сервера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, make_tuple(eid, port), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения внутреннего порта события
 *
 * @param eid идентификатор события клиента
 * @return    внутренний порт события
 */
uint16_t awh::server::Socks5::getInternalPort(const event::id_t eid) const noexcept {
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
			// Извлекаем внутренний порт события пира
			return this->_unit.getPort(i->first);
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
	return 0;
}
/**
 * @brief Метод получения адреса хоста текущей машины
 *
 * @param eid идентификатор события сервера
 * @return    адрес хоста текущей машины
 */
const string & awh::server::Socks5::getHost(const event::id_t eid) const noexcept {
	// Результат работы функции
	const static string result = "";
	// Если идентификатор события сервера соответствует идентификатору socks5-сервера
	if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if(this->_eid == eid)
			// Возвращаем адрес хоста текущей машины для сервера
			return this->_host;
		// Возвращаем адрес хоста текущей машины для указанного UDP-сервера
		else return this->_hosts.at(eid);
	// Если идентификатор сервера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * @brief Метод установки адреса хоста текущей машины
 *
 * @param eid  идентификатор события сервера
 * @param host адрес хоста текущей машины
 * @return     результат выполнения установки
 */
bool awh::server::Socks5::setHost(const event::id_t eid, string_view host) noexcept {
	// Результат работы функции
	bool result = false;
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_server->working()){
		/**
		 * Определяем тип полученного IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_addr.host(host))){
			// Для типа Unix Domain Socket
			case static_cast <uint8_t> (net_addr_t::type_t::FS): {
				// Если идентификатор события сервера соответствует идентификатору socks5-сервера
				if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
					// Устанавливаем адрес хоста целевой машины для сервера
					result = this->_server->setAddress(eid, event::address_t::UDS, host);
					// Если адрес установлен успешно
					if(result){
						// Если идентификатор события сервера соответствует идентификатору socks5-сервера
						if(this->_eid == eid)
							// Сохраняем адрес хоста целевой машины для сервера
							this->_host = this->_server->getAddress(eid, event::address_t::UDS);
						// Если идентификатор события сервера соответствует идентификатору UDP-сервера
						else {
							// Выполняем поиск идентификатор события UDP-сервера
							auto i = this->_hosts.find(eid);
							// Если идентификатор события UDP-сервера найден
							if(i != this->_hosts.end())
								// Сохраняем адрес хоста целевой машины для сервера
								i->second = this->_server->getAddress(eid, event::address_t::UDS);
							// Сохраняем его для указанного UDP-сервера
							else this->_hosts.emplace(eid, this->_server->getAddress(eid, event::address_t::UDS));
						}
					}
				// Если идентификатор сервера не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, make_tuple(eid, host), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Для типа IPv4
			case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
				// Если идентификатор события сервера соответствует идентификатору socks5-сервера
				if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
					// Устанавливаем адрес хоста целевой машины для сервера
					result = this->_server->setAddress(eid, event::address_t::IPV4, host);
					// Если адрес установлен успешно
					if(result){
						// Если идентификатор события сервера соответствует идентификатору socks5-сервера
						if(this->_eid == eid)
							// Сохраняем адрес хоста целевой машины для сервера
							this->_host = this->_server->getAddress(eid, event::address_t::IPV4);
						// Если идентификатор события сервера соответствует идентификатору UDP-сервера
						else {
							// Выполняем поиск идентификатор события UDP-сервера
							auto i = this->_hosts.find(eid);
							// Если идентификатор события UDP-сервера найден
							if(i != this->_hosts.end())
								// Сохраняем адрес хоста целевой машины для сервера
								i->second = this->_server->getAddress(eid, event::address_t::IPV4);
							// Сохраняем его для указанного UDP-сервера
							else this->_hosts.emplace(eid, this->_server->getAddress(eid, event::address_t::IPV4));
						}
					}
				// Если идентификатор сервера не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, make_tuple(eid, host), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Для типа IPv6
			case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
				// Если идентификатор события сервера соответствует идентификатору socks5-сервера
				if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
					// Устанавливаем адрес хоста целевой машины для сервера
					result = this->_server->setAddress(eid, event::address_t::IPV6, host);
					// Если адрес установлен успешно, сохраняем его
					if(result){
						// Если идентификатор события сервера соответствует идентификатору socks5-сервера
						if(this->_eid == eid)
							// Сохраняем адрес хоста целевой машины для сервера
							this->_host = this->_server->getAddress(eid, event::address_t::IPV6);
						// Если идентификатор события сервера соответствует идентификатору UDP-сервера
						else {
							// Выполняем поиск идентификатор события UDP-сервера
							auto i = this->_hosts.find(eid);
							// Если идентификатор события UDP-сервера найден
							if(i != this->_hosts.end())
								// Сохраняем адрес хоста целевой машины для сервера
								i->second = this->_server->getAddress(eid, event::address_t::IPV6);
							// Сохраняем его для указанного UDP-сервера
							else this->_hosts.emplace(eid, this->_server->getAddress(eid, event::address_t::IPV6));
						}
					}
				// Если идентификатор сервера не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, make_tuple(eid, host), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Для остальных типов адресов
			default: {
				// Если идентификатор события сервера соответствует идентификатору socks5-сервера
				if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
					// Если адрес не является IP-адресом, устанавливаем его как есть
					if((result = !host.empty())){
						// Если идентификатор события сервера соответствует идентификатору socks5-сервера
						if(this->_eid == eid)
							// Устанавливаем адрес хоста целевой машины для сервера
							this->_host = host;
						// Если идентификатор события сервера соответствует идентификатору UDP-сервера
						else {
							// Выполняем поиск идентификатор события UDP-сервера
							auto i = this->_hosts.find(eid);
							// Если идентификатор события UDP-сервера найден
							if(i != this->_hosts.end())
								// Сохраняем адрес хоста целевой машины для сервера
								i->second = host;
							// Сохраняем его для указанного UDP-сервера
							else this->_hosts.emplace(eid, host);
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
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события клиента
 * @return    адрес хоста целевой машины
 */
string awh::server::Socks5::getTarget(const event::id_t eid) const noexcept {
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
			// Извлекаем адрес хоста целевой машины для клиента принадлежащего этому пиру
			return this->_unit.getTarget(i->second.eid);
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
	return "";
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 */
bool awh::server::Socks5::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
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
			// Извлекаем адрес хоста целевой машины для клиента принадлежащего этому пиру
			return this->_unit.getTarget(i->second.eid, target);
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
 * @brief Метод установки адреса для подключения к сети клиентов
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::address_t address, string_view value) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип адреса события
		 */
		switch(static_cast <uint8_t> (address)){
			// Если тип адреса принадлежит к MAC-адресам
			case static_cast <uint8_t> (event::address_t::MAC): {
				// Выполняем блокировку потока для работы с локальными данными
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Устанавливаем полученный MAC-адрес
				if(this->_addr.parse(value, net_addr_t::type_t::MAC)){
					/**
					 * Определяем семейство адресов
					 */
					switch(static_cast <uint8_t> (this->_server->family(this->_eid))){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
							// Извлекаем переданный MAC-адрес
							src.mac = ::move(this->_addr.source());
							// Извлекаем сетевой интерфейс из объекта адреса
							this->_eth.addr.fillSource(event::node_t::SERVER, src);
							// Если сетевой интерфейс успешно получен
							if((result = !src.iface.empty()))
								// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
								this->_interface = ::move(src.iface);
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
							// Извлекаем переданный MAC-адрес
							src.mac = ::move(this->_addr.source());
							// Извлекаем сетевой интерфейс из объекта адреса
							this->_eth.addr.fillSource(event::node_t::SERVER, src);
							// Если сетевой интерфейс успешно получен
							if((result = !src.iface.empty()))
								// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
								this->_interface = ::move(src.iface);
						} break;
					}
				}
			} break;
			// Если тип адреса принадлежит к IPv4-адресам
			case static_cast <uint8_t> (event::address_t::IPV4): {
				// Выполняем блокировку потока для работы с локальными данными
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Устанавливаем полученный IPv4-адрес
				if(this->_addr.parse(value, net_addr_t::type_t::IPV4)){
					// Временный объект для извлечения сетевого интерфейса
					net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
					// Извлекаем сетевой интерфейс из объекта адреса
					this->_eth.addr.fillSource(this->_addr.source().get(), src);
					// Если сетевой интерфейс успешно получен
					if((result = !src.iface.empty()))
						// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
						this->_interface = ::move(src.iface);
				}
			} break;
			// Если тип адреса принадлежит к IPv6-адресам
			case static_cast <uint8_t> (event::address_t::IPV6): {
				// Выполняем блокировку потока для работы с локальными данными
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Устанавливаем полученный IPv6-адрес
				if(this->_addr.parse(value, net_addr_t::type_t::IPV6)){
					// Временный объект для извлечения сетевого интерфейса
					net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
					// Извлекаем сетевой интерфейс из объекта адреса
					this->_eth.addr.fillSource(this->_addr.source().get(), src);
					// Если сетевой интерфейс успешно получен
					if((result = !src.iface.empty()))
						// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
						this->_interface = ::move(src.iface);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address), value), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
			// Если DNS-резолвер или сервер находятся в нерабочем состоянии
			if(this->_dns != nullptr ? !this->_dns->working() : !this->_server->working())
				// Устанавливаем внутренний адрес socks5-сервера
				return this->_server->setAddress(eid, address, value);
		// Если идентификатор события передан другой
		} else {
			// Выполняем блокировку потока для работы с локальными данными
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end())
				// Устанавливаем внутренний адрес удалённого клиента подключённого пира
				return this->_server->setAddress(i->second.eid, address, value);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address), value), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки адреса для подключения к сети клиентов
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::address_t address, const net::addr_t * value) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип адреса события
		 */
		switch(static_cast <uint8_t> (address)){
			// Если тип адреса принадлежит к MAC-адресам
			case static_cast <uint8_t> (event::address_t::MAC): {
				/**
				 * Определяем семейство адресов
				 */
				switch(static_cast <uint8_t> (this->_server->family(this->_eid))){
					// Для семейства IPv4
					case static_cast <uint8_t> (event::family_t::IPV4): {
						// Временный объект для извлечения сетевого интерфейса
						net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
						// Устанавливаем MAC-адрес для извлечения сетевого интерфейса
						::memcpy(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], &awh_cast <const net::addr_mac_t *> (value)->address[0], 6);
						// Извлекаем сетевой интерфейс из объекта адреса
						this->_eth.addr.fillSource(event::node_t::SERVER, src);
						// Если сетевой интерфейс успешно получен
						if((result = !src.iface.empty()))
							// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
							this->_interface = ::move(src.iface);
					} break;
					// Для семейства IPv6
					case static_cast <uint8_t> (event::family_t::IPV6): {
						// Временный объект для извлечения сетевого интерфейса
						net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
						// Устанавливаем MAC-адрес для извлечения сетевого интерфейса
						::memcpy(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], &awh_cast <const net::addr_mac_t *> (value)->address[0], 6);
						// Извлекаем сетевой интерфейс из объекта адреса
						this->_eth.addr.fillSource(event::node_t::SERVER, src);
						// Если сетевой интерфейс успешно получен
						if((result = !src.iface.empty()))
							// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
							this->_interface = ::move(src.iface);
					} break;
				}
			} break;
			// Если тип адреса принадлежит к IPv4-адресам
			case static_cast <uint8_t> (event::address_t::IPV4): {
				// Временный объект для извлечения сетевого интерфейса
				net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
				// Извлекаем сетевой интерфейс из объекта адреса
				this->_eth.addr.fillSource(value, src);
				// Если сетевой интерфейс успешно получен
				if((result = !src.iface.empty()))
					// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
					this->_interface = ::move(src.iface);
			} break;
			// Если тип адреса принадлежит к IPv6-адресам
			case static_cast <uint8_t> (event::address_t::IPV6): {
				// Временный объект для извлечения сетевого интерфейса
				net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
				// Извлекаем сетевой интерфейс из объекта адреса
				this->_eth.addr.fillSource(value, src);
				// Если сетевой интерфейс успешно получен
				if((result = !src.iface.empty()))
					// Устанавливаем внутренний сетевой интерфейс для подключения к сети клиентов
					this->_interface = ::move(src.iface);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::server::Socks5::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end())){
			// Если DNS-резолвер или сервер находятся в нерабочем состоянии
			if(this->_dns != nullptr ? !this->_dns->working() : !this->_server->working())
				// Устанавливаем внутренний адрес socks5-сервера
				return this->_server->setAddress(eid, address, value);
		// Если идентификатор события передан другой
		} else {
			// Выполняем блокировку потока для работы с локальными данными
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск идентификатор события подключённого пира
			auto i = this->_peers.find(eid);
			// Если идентификатор события подключённого пира найден
			if(i != this->_peers.end())
				// Устанавливаем внутренний адрес удалённого клиента подключённого пира
				return this->_server->setAddress(i->second.eid, address, value);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения адреса для подключения к сети клиентов
 *
 * @param address тип адреса клиента или сервера
 * @return        значение адреса клиента или сервера
 */
string awh::server::Socks5::getAddress(const event::address_t address) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если сетевой интерфейс для подключения к сети клиентов установлен
		if(!this->_interface.empty()){
			/**
			 * Определяем тип адреса события
			 */
			switch(static_cast <uint8_t> (address)){
				// Если тип адреса принадлежит к MAC-адресам
				case static_cast <uint8_t> (event::address_t::MAC): {
					/**
					 * Определяем семейство адресов
					 */
					switch(static_cast <uint8_t> (this->_server->family(this->_eid))){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
							// Устанавливаем имя сетевого интерфейса
							src.iface = this->_interface;
							// Выполняем извлечение сетевых параметров
							this->_eth.addr.fillSource(src);
							// Если MAC-адрес успешно получен
							if(::memcmp(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
								// Выполняем блокировку потока для работы с локальными данными
								const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
								// Устанавливаем полученный MAC-адрес в объект события
								const_cast <socks5_t *> (this)->_addr.source(src.mac.get());
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
							// Устанавливаем имя сетевого интерфейса
							src.iface = this->_interface;
							// Выполняем извлечение сетевых параметров
							this->_eth.addr.fillSource(src);
							// Если MAC-адрес успешно получен
							if(::memcmp(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0){
								// Выполняем блокировку потока для работы с локальными данными
								const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
								// Устанавливаем полученный MAC-адрес в объект события
								const_cast <socks5_t *> (this)->_addr.source(src.mac.get());
								// Выводим результат работы функции
								return static_cast <string> (this->_addr);
							}
						} break;
					}
				} break;
				// Если тип адреса принадлежит к IPv4-адресам
				case static_cast <uint8_t> (event::address_t::IPV4): {
					// Временный объект для извлечения сетевого интерфейса
					net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
					// Устанавливаем имя сетевого интерфейса
					src.iface = this->_interface;
					// Выполняем извлечение сетевых параметров
					this->_eth.addr.fillSource(src);
					// Если IP-адрес успешно получен
					if(awh_cast <net::addr_net_ipv4_t *> (src.ip.get())->address > 0){
						// Выполняем блокировку потока для работы с локальными данными
						const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Устанавливаем IP-адрес в источник сетевого адреса
						const_cast <socks5_t *> (this)->_addr.source(src.ip.get());
						// Выводим результат работы функции
						return static_cast <string> (this->_addr);
					}
				} break;
				// Если тип адреса принадлежит к IPv6-адресам
				case static_cast <uint8_t> (event::address_t::IPV6): {
					// Временный объект для извлечения сетевого интерфейса
					net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
					// Устанавливаем имя сетевого интерфейса
					src.iface = this->_interface;
					// Выполняем извлечение сетевых параметров
					this->_eth.addr.fillSource(src);
					// Если IP-адрес успешно получен
					if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (src.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0){
						// Выполняем блокировку потока для работы с локальными данными
						const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Устанавливаем IP-адрес в источник сетевого адреса
						const_cast <socks5_t *> (this)->_addr.source(src.ip.get());
						// Выводим результат работы функции
						return static_cast <string> (this->_addr);
					}
				} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод получения адреса клиента или текущего сервера
 *
 * @param eid     идентификатор события клиента или сервера
 * @param address тип адреса клиента или сервера
 * @return        значение адреса клиента или сервера
 */
string awh::server::Socks5::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end()))
			// Получаем адрес внутренний адрес socks5-сервера
			return this->_server->getAddress(eid, address);
		// Если идентификатор события передан другой
		else {
			// Выполняем блокировку потока для работы с локальными данными
			const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Если идентификатор события подключённого пира найден
			if(this->_peers.find(eid) != this->_peers.end())
				// Получаем адрес подключённого пира
				return this->_server->getAddress(eid, address);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод получения адреса для подключения к сети клиентов
 *
 * @param address тип адреса клиента или сервера
 * @param value   объект для извлечения адреса клиента или сервера
 * @return        результат выполнения извлечения адреса клиента или сервера
 */
bool awh::server::Socks5::getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если сетевой интерфейс для подключения к сети клиентов установлен
		if(!this->_interface.empty()){
			/**
			 * Определяем тип адреса события
			 */
			switch(static_cast <uint8_t> (address)){
				// Если тип адреса принадлежит к MAC-адресам
				case static_cast <uint8_t> (event::address_t::MAC): {
					/**
					 * Определяем семейство адресов
					 */
					switch(static_cast <uint8_t> (this->_server->family(this->_eid))){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
							// Устанавливаем имя сетевого интерфейса
							src.iface = this->_interface;
							// Выполняем извлечение сетевых параметров
							this->_eth.addr.fillSource(src);
							// Если MAC-адрес успешно получен
							if((result = (::memcmp(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0)))
								// Устанавливаем MAC-адрес в источник сетевого адреса
								value = ::move(src.mac);
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
							// Устанавливаем имя сетевого интерфейса
							src.iface = this->_interface;
							// Выполняем извлечение сетевых параметров
							this->_eth.addr.fillSource(src);
							// Если MAC-адрес успешно получен
							if((result = (::memcmp(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0)))
								// Устанавливаем MAC-адрес в источник сетевого адреса
								value = ::move(src.mac);
						} break;
					}
				} break;
				// Если тип адреса принадлежит к IPv4-адресам
				case static_cast <uint8_t> (event::address_t::IPV4): {
					// Временный объект для извлечения сетевого интерфейса
					net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
					// Устанавливаем имя сетевого интерфейса
					src.iface = this->_interface;
					// Выполняем извлечение сетевых параметров
					this->_eth.addr.fillSource(src);
					// Если IP-адрес успешно получен
					if((result = (awh_cast <net::addr_net_ipv4_t *> (src.ip.get())->address > 0)))
						// Устанавливаем IP-адрес в источник сетевого адреса
						value = ::move(src.ip);
				} break;
				// Если тип адреса принадлежит к IPv6-адресам
				case static_cast <uint8_t> (event::address_t::IPV6): {
					// Временный объект для извлечения сетевого интерфейса
					net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
					// Устанавливаем имя сетевого интерфейса
					src.iface = this->_interface;
					// Выполняем извлечение сетевых параметров
					this->_eth.addr.fillSource(src);
					// Если IP-адрес успешно получен
					if((result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (src.ip.get())->address[0], (uint8_t[16]){0}, 16) != 0)))
						// Устанавливаем IP-адрес в источник сетевого адреса
						value = ::move(src.ip);
				} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения адреса клиента или текущего сервера
 *
 * @param eid     идентификатор события клиента или сервера
 * @param address тип адреса клиента или сервера
 * @param value   объект для извлечения адреса клиента или сервера
 * @return        результат выполнения извлечения адреса клиента или сервера
 */
bool awh::server::Socks5::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если идентификатор события сервера соответствует идентификатору socks5-сервера
		if((this->_eid == eid) || (this->_servers.find(eid) != this->_servers.end()))
			// Получаем адрес внутренний адрес socks5-сервера
			return this->_server->getAddress(eid, address, value);
		// Если идентификатор события передан другой
		else {
			// Выполняем блокировку потока для работы с локальными данными
			const locker_t <std::shared_mutex> lock(const_cast <socks5_t *> (this)->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Если идентификатор события подключённого пира найден
			if(this->_peers.find(eid) != this->_peers.end())
				// Получаем адрес подключённого пира
				return this->_server->getAddress(eid, address, value);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::CRITICAL, error.what());
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
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
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
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_server->bandwidth(i->first, limiting, bandwidth))
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
		const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Выполняем поиск идентификатор события подключённого пира
		auto i = this->_peers.find(eid);
		// Если идентификатор события подключённого пира найден
		if((i != this->_peers.end()) && this->_server->keepAlive(i->first, cnt, idle, intvl))
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
				case static_cast <uint8_t> (event::protocol_t::TCP): {
					// Устанавливаем идентификатор события для сервера
					this->_eid = eid;
					/**
					 * Определяем семейство адресов
					 */
					switch(static_cast <uint8_t> (this->_server->family(this->_eid))){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv4_t> ());
							// Выполняем извлечение сетевых параметров
							this->_eth.addr.fillSource(event::node_t::NONE, src);
							// Если MAC-адрес успешно получен
							if(::memcmp(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0)
								// Устанавливаем имя сетевого интерфейса для подключения к сети клиентов
								this->_interface = ::move(src.iface);
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Временный объект для извлечения сетевого интерфейса
							net::src_t src(::make_unique <net::addr_net_ipv6_t> ());
							// Выполняем извлечение сетевых параметров
							this->_eth.addr.fillSource(event::node_t::NONE, src);
							// Если MAC-адрес успешно получен
							if(::memcmp(&awh_cast <net::addr_mac_t *> (src.mac.get())->address[0], (uint8_t[6]){0}, 6) != 0)
								// Устанавливаем имя сетевого интерфейса для подключения к сети клиентов
								this->_interface = ::move(src.iface);
						} break;
					}
				} break;
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
 server_t(server, fmk, log), _interface{""}, _unit(fmk, log), _eth(fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_unit.on <void (const event::id_t, const bool)> ("connect", &server::socks5_t::connectClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_unit.on <void (const event::id_t, const event::status_t)> ("state", &server::socks5_t::statusClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_unit.on <void (const event::id_t, const event::error_t, const string &)> ("error", &server::socks5_t::errorClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_unit.on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server::socks5_t::timeoutClient, this, _1, _2, _3);
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
 server_t(server, dns, fmk, log), _interface{""}, _unit(fmk, log), _eth(fmk, log), _socks5(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_unit.on <void (const event::id_t, const bool)> ("connect", &server::socks5_t::connectClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_unit.on <void (const event::id_t, const event::status_t)> ("state", &server::socks5_t::statusClient, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_unit.on <void (const event::id_t, const event::error_t, const string &)> ("error", &server::socks5_t::errorClient, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_unit.on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server::socks5_t::timeoutClient, this, _1, _2, _3);
}
/**
 * @brief Деструктор
 *
 */
awh::server::Socks5::~Socks5() noexcept {}
