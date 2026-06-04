/**
 * @file: client.cpp
 * @date: 2026-04-05
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
#include <client/client.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Метод изменения статуса клиента
 *
 * @param status новый статус клиента
 * @param state  новое временное состояние клиента
 */
void awh::Client::status(const event::status_t status, const state_t state) noexcept {
	/**
	 * Временное состояние клиента
	 */
	switch(static_cast <uint8_t> (state)){
		// Если мы получили статус события клиента
		case static_cast <uint8_t> (state_t::CLIENT): {
			// Если функция обратного вызова установлена
			if(this->_callback.is("status"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("status", status);
			// Если работа клиента запущена
			if(status == event::status_t::LAUNCHED){
				// Выполняем запуск работы клиента, если клиент не запущен
				if(!this->_client->launch(this->_eid)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (state)), log_t::flag_t::WARNING, this->_eid);
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
		// Если мы получили статус события DNS-резолвера
		case static_cast <uint8_t> (state_t::RESOLVER): {
			/**
			 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие DNS-резолвера запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					/**
					 * Определяем принадлежит ли хост, к IP-адресу
					 */
					switch(static_cast <uint8_t> (this->_addr.host(this->_host))){
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Устанавливаем адрес хоста целевой машины для клиента
							if(this->_client->setTarget(this->_eid, this->_host)){
								/**
								 * В зависимости от статуса события клиента выполняем запуск
								 */
								switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_client)->status(this->_eid))){
									// Если событие клиента не запущено, запускаем его
									case static_cast <uint8_t> (event::status_t::NONE): {
										// Если событие клиента не запущено, запускаем его
										if(this->_client->commit(this->_eid)){
											// Если функция обратного вызова установлена
											if(this->_callback.is("ready"))
												// Выполняем функцию обратного вызова
												this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, this->_client->family(this->_eid), this->_host, this->_client->getTarget(this->_eid));
											// Запускаем клиента
											this->_client->start();
										}
									} break;
									// Если событие клиента инициализировано, запускаем его
									case static_cast <uint8_t> (event::status_t::INITIAL):
									// Если событие находится в состоянии успешного подключения
									case static_cast <uint8_t> (event::status_t::SUCCESS):
										// Запускаем клиента
										this->_client->start();
									break;
								}
							}
							// Выходим из функции
							return;
						}
					}
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
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (status), static_cast <uint16_t> (state)), log_t::flag_t::WARNING, error.c_str());
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
void awh::Client::connect(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если объект транспортного уровня безопасности установлен
		if((this->_coder != nullptr) && (this->_tid > 0)){
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS не выполнено
				if(!this->_coder->handshake(this->_tid)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error_tls")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(this->_eid), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
						#endif
					}
				}
			}
		// Если объект транспортного уровня безопасности не установлен
		} else {
			// Если функция обратного вызова установлена
			if(this->_callback.is("connect"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const bool)> ("connect", eid, ok);
		}
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid  идентификатор клиента
 * @param size размер данных для записи
 */
void awh::Client::write(const event::id_t eid, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("write"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const size_t)> ("write", eid, size);
	}
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param eid    идентификатор клиента
 * @param status новый статус клиента
 */
void awh::Client::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
		// Если статус клиента изменился на "уничтожен"
		if(status == event::status_t::DESTROYED){
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
 * @brief Метод обработки действий клиента
 *
 * @param eid    идентификатор клиента
 * @param action действие клиента
 */
void awh::Client::action(const event::id_t eid, const event::action_t action) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("action"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::action_t)> ("action", eid, action);
	}
}
/**
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных клиента
 * @param size   размер данных клиента
 */
void awh::Client::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
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
			if(this->_callback.is("read"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, buffer, size);
		}
	}
}
/**
 * @brief Метод получения события ошибок
 *
 * @param eid     идентификатор события
 * @param error   код ошибки
 * @param message сообщение об ошибке
 */
void awh::Client::error(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, message);
	}
}
/**
 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
 *
 * @param eid    идентификатор клиента
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 */
void awh::Client::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("available"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t, const size_t)> ("available", eid, status, size);
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
bool awh::Client::timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("timeout"))
			// Выполняем функцию обратного вызова
			return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", eid, action, delay);
	}
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки события неотправленных данных клиента
 *
 * @param eid   идентификатор клиента
 * @param error тип ошибки отправки данных
 * @param data  данные, которые не получилось отправить
 * @param size  размер данных, которые не получилось отправить
 */
void awh::Client::spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("spool"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", eid, error, buffer, size);
	}
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param id    идентификатор TLS
 * @param state состояние TLS
 */
void awh::Client::stateTLS(const tls::coder_t::id_t id, const tls::coder_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state_tls"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const tls::coder_t::id_t, const tls::coder_t::state_t)> ("state_tls", id, state);
		// Если состояние рукопожатия успешно завершено
		if(state == tls::coder_t::state_t::HANDSHAKED){
			// Если функция обратного вызова установлена
			if(this->_callback.is("connect"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const bool)> ("connect", this->_eid, true);
		}
	}
}
/**
 * @brief Метод получения ошибок TLS
 *
 * @param id      идентификатор TLS
 * @param error   код ошибки TLS
 * @param message сообщение об ошибке TLS
 */
void awh::Client::errorTLS(const tls::coder_t::id_t id, const tls::coder_t::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("error_tls")){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (error), message), log_t::flag_t::CRITICAL, message.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, message.c_str());
			#endif
		// Выполняем функцию обратного вызова
		} else this->_callback.call <void (const tls::coder_t::id_t, const tls::coder_t::error_t, const string &)> ("error_tls", id, error, message);
	}
}
/**
  @brief Метод получения событий шифрования/дешифрования данных TLS
 *
 * @param id     идентификатор TLS
 * @param event  тип события TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 */
void awh::Client::processTLS([[maybe_unused]] const tls::coder_t::id_t id, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
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
							this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(this->_eid, buffer, size), log_t::flag_t::WARNING);
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
				// Если функция обратного вызова установлена
				if(this->_callback.is("read"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, buffer, size);
			} break;
		}
	}
}
/**
 * @brief Метод обработки попыток подключения клиента к удалённому серверу
 *
 * @param id       идентификатор DNS-запроса
 * @param domain   доменное имя для резолвинга
 * @param attempts количество попыток подключения
 */
void awh::Client::attemptsDNS([[maybe_unused]] const unit::dns_t::id_t id, const string & domain, const uint8_t attempts) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("attempts_dns"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const string &, const uint8_t)> ("attempts_dns", domain, attempts);
	}
}
/**
 * @brief Метод резолвинга доменного имени в сетевой адрес
 *
 * @param id     идентификатор DNS-запроса
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для резолвинга
 * @param addr   указатель на структуру для хранения результата резолвинга
 */
void awh::Client::resolveDNS([[maybe_unused]] const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns->working()){
		// Устанавливаем адрес хоста целевой машины для клиента
		if(this->_client->setTarget(this->_eid, addr)){
			/**
			 * В зависимости от статуса события клиента выполняем запуск
			 */
			switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_client)->status(this->_eid))){
				// Если событие клиента не запущено, запускаем его
				case static_cast <uint8_t> (event::status_t::NONE): {
					// Если событие клиента не запущено, запускаем его
					if(this->_client->commit(this->_eid)){
						// Если функция обратного вызова установлена
						if(this->_callback.is("ready"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, family, domain, this->_client->getTarget(this->_eid));
						// Запускаем клиента
						this->_client->start();
					}
				} break;
				// Если событие клиента инициализировано, запускаем его
				case static_cast <uint8_t> (event::status_t::INITIAL):
				// Если событие находится в состоянии успешного подключения
				case static_cast <uint8_t> (event::status_t::SUCCESS):
					// Запускаем клиента
					this->_client->start();
				break;
			}
		}
	}
}
/**
 * @brief Метод остановки клиента
 *
 */
void awh::Client::stop() noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns->stop();
			// Останавливаем событие клиента
			else this->_client->stop();
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
	}
}
/**
 * @brief Метод запуска клиента
 *
 */
void awh::Client::start() noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns != nullptr)
				// Запускаем событие DNS-резолвера
				this->_dns->start();
			// Если объект DNS-резолвера не установлен
			else {
				/**
				 * В зависимости от статуса события клиента выполняем запуск
				 */
				switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_client)->status(this->_eid))){
					// Если событие клиента не запущено, запускаем его
					case static_cast <uint8_t> (event::status_t::NONE): {
						// Если событие клиента не запущено, запускаем его
						if(this->_client->commit(this->_eid)){
							// Если функция обратного вызова установлена
							if(this->_callback.is("ready")){
								// Получаем адрес хоста целевой машины для клиента
								const string & host = this->_client->getTarget(this->_eid);
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", this->_eid, this->_client->family(this->_eid), host, host);
							}
							// Запускаем клиента
							this->_client->start();
						}
					} break;
					// Если событие клиента инициализировано, запускаем его
					case static_cast <uint8_t> (event::status_t::INITIAL):
					// Если событие находится в состоянии успешного подключения
					case static_cast <uint8_t> (event::status_t::SUCCESS):
						// Запускаем клиента
						this->_client->start();
					break;
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
	}
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @return результат выполнения приостановки работы
 */
bool awh::Client::pause() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Приостанавливаем событие клиента
			return this->_client->pause(this->_eid);
		// Если идентификатор клиента не установлен
		else {
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
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @return результат выполнения возобновления работы
 */
bool awh::Client::resume() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Возобновляем событие клиента
			return this->_client->resume(this->_eid);
		// Если идентификатор клиента не установлен
		else {
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
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод мультиподключения клиентов к удалённым хостам
 *
 * @return результат выполнения подключения
 */
bool awh::Client::connect() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Подключаем клиента к удалённому серверу
			return this->_client->connect(this->_eid);
		// Если идентификатор клиента не установлен
		else {
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
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера
 *
 * @return результат выполнения отключения
 */
bool awh::Client::disconnect() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Отключаем клиента от удалённого сервера
			return this->_client->disconnect(this->_eid);
		// Если идентификатор клиента не установлен
		else {
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
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::Client::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для функций обратного вызова
	this->_callback.threadSafety(mode);
	// Устанавливаем режим безопасности работы потоков для объекта клиента
	this->_client->threadSafety(mode);
	// Если объект DNS-резолвера установлен
	if(this->_dns != nullptr)
		// Устанавливаем режим безопасности работы потоков для объекта DNS-резолвера
		this->_dns->threadSafety(mode);
	// Если идентификатор TLS и объект TLS установлены
	if((this->_tid > 0) && (this->_coder != nullptr))
		// Устанавливаем режим безопасности работы потоков для объекта TLS
		this->_coder->threadSafety(this->_tid, mode);
	// Если идентификатор TLS не установлен, но объект TLS установлен
	else if((this->_tid == 0) && (this->_coder != nullptr)) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("TLS ID is not set", __PRETTY_FUNCTION__, make_tuple(mode), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("TLS ID is not set", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::Client::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на событие получение данных от сервера
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при отправке данных на сервер
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова на событие готовности клиента к работе
	this->_callback.set("ready", callback);
	// Выполняем установку функции обратного вызова при изменении состояния клиента
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова на событие неотправленных данных клиентом
	this->_callback.set("spool", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова на событие изменения статуса клиента
	this->_callback.set("status", callback);
	// Выполняем установку функции обратного вызова на событие изменения состояния клиента
	this->_callback.set("action", callback);
	// Выполняем установку функции обратного вызова на событие запуска клиента
	this->_callback.set("launch", callback);
	// Выполняем установку функции обратного вызова на событие истечения таймаута клиента
	this->_callback.set("timeout", callback);
	// Выполняем установку функции обратного вызова при подключении клиента к удалённому серверу
	this->_callback.set("connect", callback);
	// Выполняем установку функции обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_callback.set("available", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок TLS
	this->_callback.set("error_tls", callback);
	// Выполняем установку функции обратного вызова на событие получения состояния TLS
	this->_callback.set("state_tls", callback);
	// Выполняем установку функции обратного вызова на событие завершения попыток резолвинга доменного имени DNS-резолвером
	this->_callback.set("attempts_dns", callback);
}
/**
 * @brief Метод получения данных от сервера
 *
 * @return результат получения данных
 */
bool awh::Client::recv() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working())
		// Выполняем получение данных от сервера
		return this->_client->recv(this->_eid);
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных серверу
 *
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных серверу
 */
size_t awh::Client::send(const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
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
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод получения сетевого интерфейса клиента
 *
 * @return сетевой интерфейс клиента
 */
string awh::Client::getIface() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем сетевой интерфейс клиента
		return this->_client->getIface(this->_eid);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса клиента
 *
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::Client::setIface(string_view name) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Устанавливаем сетевой интерфейс клиента
			return this->_client->setIface(this->_eid, name);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения порта удаленного сервера
 *
 * @return порт удаленного сервера
 */
uint16_t awh::Client::getPort() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем порт удаленного сервера для клиента
		return this->_client->getPort(this->_eid);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта удаленного сервера
 *
 * @param port порт удаленного сервера для установки
 * @return     результат выполнения установки
 */
bool awh::Client::setPort(const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Устанавливаем порт удаленного сервера для клиента
			return this->_client->setPort(this->_eid, port);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения внутреннего порта события
 *
 * @return внутренний порт события
 */
uint16_t awh::Client::getInternalPort() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем внутренний порт события для клиента
		return this->_client->getInternalPort(this->_eid);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки внутреннего порта события
 *
 * @param port внутренний порт события
 * @return     результат выполнения установки
 */
bool awh::Client::setInternalPort(const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Устанавливаем внутренний порт события для клиента
			return this->_client->setInternalPort(this->_eid, port);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @return адрес хоста целевой машины
 */
string awh::Client::getTarget() const noexcept {
	// Если сохранённый адрес хоста целевой машины для клиента не пустой
	if(!this->_host.empty())
		// Возвращаем сохранённый адрес хоста целевой машины для клиента
		return this->_host;
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем адрес хоста целевой машины для клиента
		return this->_client->getTarget(this->_eid);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::Client::setTarget(string_view target) noexcept {
	// Результат работы функции
	bool result = false;
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		/**
		 * Определяем тип полученного IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_addr.host(target))){
			// Для типа Unix Domain Socket
			case static_cast <uint8_t> (net_addr_t::type_t::FS):
			// Для типа IPv4
			case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
			// Для типа IPv6
			case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
				// Если идентификатор клиента установлен
				if(this->_eid > 0){
					// Устанавливаем адрес хоста целевой машины для клиента
					result = this->_client->setTarget(this->_eid, target);
					// Если адрес установлен успешно, сохраняем его
					if(result)
						// Сохраняем адрес хоста целевой машины для клиента
						this->_host = this->_client->getTarget(this->_eid);
				// Если идентификатор клиента не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(target), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Для остальных типов адресов
			default: {
				// Если адрес не является IP-адресом, устанавливаем его как есть
				if((result = !target.empty()))
					// Устанавливаем адрес хоста целевой машины для клиента
					this->_host = target;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::Client::setTarget(const net::addr_t * target) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0){
			// Устанавливаем адрес хоста целевой машины для клиента
			const bool result = this->_client->setTarget(this->_eid, target);
			// Если адрес установлен успешно, сохраняем его
			if(result)
				// Сохраняем адрес хоста целевой машины для клиента
				this->_host = this->_client->getTarget(this->_eid);
			// Возвращаем результат установки адреса хоста целевой машины для клиента
			return result;
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
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 */
bool awh::Client::getTarget(unique_ptr <net::addr_t> & target) const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем адрес хоста целевой машины для клиента
		return this->_client->getTarget(this->_eid, target);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param address тип адреса клиента
 * @return        значение адреса клиента
 */
string awh::Client::getAddress(const event::address_t address) const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем адрес клиента
		return this->_client->getAddress(this->_eid, address);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param address тип адреса клиента
 * @param value   значение адреса клиента
 * @return        результат выполнения установки
 */
bool awh::Client::setAddress(const event::address_t address, string_view value) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Устанавливаем адрес клиента
			return this->_client->setAddress(this->_eid, address, value);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address), value), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param address тип адреса клиента
 * @param value   значение адреса клиента
 * @return        результат выполнения установки
 */
bool awh::Client::setAddress(const event::address_t address, const net::addr_t * value) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор клиента установлен
		if(this->_eid > 0)
			// Устанавливаем адрес клиента
			return this->_client->setAddress(this->_eid, address, value);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param address тип адреса клиента
 * @param value   объект для извлечения адреса клиента
 * @return        результат выполнения извлечения адреса клиента
 */
bool awh::Client::getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем адрес клиента
		return this->_client->getAddress(this->_eid, address, value);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера клиента
 *
 * @param action тип действия клиента
 * @return       размер буфера клиента
 */
size_t awh::Client::getBufferSize(const event::action_t action) const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем размер буфера клиента
		return this->_client->getBufferSize(this->_eid, action);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера клиента
 *
 * @param action тип действия клиента
 * @param size   размер буфера клиента
 * @return       результат выполнения установки
 */
bool awh::Client::setBufferSize(const event::action_t action, const size_t size) noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Устанавливаем размер буфера клиента
		return this->_client->setBufferSize(this->_eid, action, size);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action), size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод получения таймаута резолвинга доменного имени
 *
 * @return таймаут резолвинга доменного имени в миллисекундах
 */
uint32_t awh::Client::getTimeoutDNS() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Возвращаем таймаут резолвинга доменного имени для клиента
		return this->_timeoutDNS.load(std::memory_order_acquire);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута резолвинга доменного имени
 *
 * @param timeout таймаут резолвинга доменного имени в миллисекундах
 */
void awh::Client::setTimeoutDNS(const uint32_t timeout) noexcept {
	// Устанавливаем таймаут резолвинга доменного имени для клиента
	this->_timeoutDNS.store(timeout, std::memory_order_release);
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @return режим использования таймаута на чтение события
 */
awh::event::usage_t awh::Client::getUsageReadTimeout() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем режим использования таймаута на чтение события
		return this->_client->getUsageReadTimeout(this->_eid);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 */
void awh::Client::setUsageReadTimeout(const event::usage_t usage) noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Устанавливаем режим использования таймаута на чтение события
		this->_client->setUsageReadTimeout(this->_eid, usage);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (usage)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод получения таймаута клиента
 *
 * @param action тип действия клиента
 * @return       значение таймаута в миллисекундах
 */
uint32_t awh::Client::getTimeout(const event::action_t action) const noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Извлекаем таймаут клиента
		return this->_client->getTimeout(this->_eid, action);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута клиента
 *
 * @param action  тип действия клиента
 * @param timeout значение таймаута в миллисекундах
 */
void awh::Client::setTimeout(const event::action_t action, const uint32_t timeout) noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Устанавливаем таймаут клиента
		this->_client->setTimeout(this->_eid, action, timeout);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 */
bool awh::Client::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Устанавливаем пропускную способность клиента
		return this->_client->bandwidth(this->_eid, limiting, bandwidth);
	// Если идентификатор клиента не установлен
	else {
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров keep-alive для клиента
 *
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::Client::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0)
		// Устанавливаем параметры keep-alive для клиента
		return this->_client->keepAlive(this->_eid, cnt, idle, intvl);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, make_tuple(cnt, idle, intvl), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод установки идентификатора события клиента
 *
 * @param eid идентификатор события для установки
 */
void awh::Client::setEventId(const event::id_t eid) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working())
		// Устанавливаем идентификатор события для клиента
		this->_eid = eid;
}
/**
 * @brief Метод установки идентификатора TLS
 *
 * @param tid идентификатор TLS для установки
 */
void awh::Client::setSecurityId(const tls::coder_t::id_t tid) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns != nullptr ? !this->_dns->working() : !this->_client->working()){
		// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
		if((tid > 0) && (this->_coder != nullptr)){
			// Устанавливаем идентификатор TLS для клиента
			this->_tid = tid;
			// Устанавливаем функцию обратного вызова на событие состояния TLS
			this->_coder->on(this->_tid, std::bind(&client_t::stateTLS, this, _1, _2));
			// Устанавливаем функцию обратного вызова на событие ошибок TLS
			this->_coder->on(this->_tid, std::bind(&client_t::errorTLS, this, _1, _2, _3));
			// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
			this->_coder->on(this->_tid, std::bind(&client_t::processTLS, this, _1, _2, _3, _4));
		}
	}
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Client::Client(unit::client_t * client, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _dns(nullptr), _coder(nullptr), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
		// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
		this->_client->on <void (const event::status_t)> ("status", &client_t::status, this, _1, state_t::CLIENT);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_client->on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
		this->_client->on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий клиента
		this->_client->on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных клиентом
		this->_client->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок клиента
		this->_client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
		this->_client->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_client->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
		this->_client->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
		this->_client->on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
	// Если объект клиента не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param coder  объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Client::Client(unit::client_t * client, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _dns(nullptr), _coder(coder), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
		// Если объект транспортного уровня безопасности не установлен
		if(this->_coder == nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("TLS object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("TLS object not set", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
		// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
		this->_client->on <void (const event::status_t)> ("status", &client_t::status, this, _1, state_t::CLIENT);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_client->on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
		this->_client->on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий клиента
		this->_client->on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных клиентом
		this->_client->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок клиента
		this->_client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
		this->_client->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_client->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
		this->_client->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
		this->_client->on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
	// Если объект клиента не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Конструктор
 *
 * @param client объект юнита клиента
 * @param dns    объект DNS-резолвера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Client::Client(unit::client_t * client, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""},  _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _dns(dns), _coder(nullptr), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
		// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
		this->_client->on <void (const event::status_t)> ("status", &client_t::status, this, _1, state_t::CLIENT);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_client->on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
		this->_client->on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий клиента
		this->_client->on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных клиентом
		this->_client->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок клиента
		this->_client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
		this->_client->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_client->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
		this->_client->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
		this->_client->on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr){
			// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
			this->_dns->on <void (const event::status_t)> ("status", &client_t::status, this, _1, state_t::RESOLVER);
			// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
			this->_dns->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
			// Устанавливаем функции обратного вызова для обработки попыток подключения клиента к удалённому серверу
			this->_dns->on <void (const unit::dns_t::id_t, const string &, const uint8_t)> ("attempts", &client_t::attemptsDNS, this, _1, _2, _3);
			// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
			this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &client_t::resolveDNS, this, _1, _2, _3, _4);
		// Если объект DNS-резолвера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("DNS resolver object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("DNS resolver object not set", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	// Если объект клиента не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
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
awh::Client::Client(unit::client_t * client, unit::dns_t * dns, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _dns(dns), _coder(coder), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
		// Если объект транспортного уровня безопасности не установлен
		if(this->_coder == nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("TLS object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("TLS object not set", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
		// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
		this->_client->on <void (const event::status_t)> ("status", &client_t::status, this, _1, state_t::CLIENT);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_client->on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
		this->_client->on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий клиента
		this->_client->on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных клиентом
		this->_client->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок клиента
		this->_client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
		this->_client->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_client->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
		this->_client->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
		this->_client->on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr){
			// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
			this->_dns->on <void (const event::status_t)> ("status", &client_t::status, this, _1, state_t::RESOLVER);
			// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
			this->_dns->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
			// Устанавливаем функции обратного вызова для обработки попыток подключения клиента к удалённому серверу
			this->_dns->on <void (const unit::dns_t::id_t, const string &, const uint8_t)> ("attempts", &client_t::attemptsDNS, this, _1, _2, _3);
			// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
			this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &client_t::resolveDNS, this, _1, _2, _3, _4);
		// Если объект DNS-резолвера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("DNS resolver object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("DNS resolver object not set", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	// Если объект клиента не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Деструктор
 *
 */
awh::Client::~Client() noexcept {}
