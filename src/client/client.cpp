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
 * @brief Метод обработки событий подключения клиента к удалённому серверу
 *
 * @param eid идентификатор клиента
 * @param ok  результат подключения
 */
void awh::Client::connect(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если объект транспортного уровня безопасности установлен
		if((this->_tls != nullptr) && (this->_tid > 0)){
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS не выполнено
				if(!this->_tls->handshake(this->_tid)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("errorTLS")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, std::make_tuple(this->_eid), log_t::flag_t::WARNING);
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
		if((this->_tls != nullptr) && (this->_tid > 0)){
			// Если данные не расшифрованы
			if(!this->_tls->decrypt(this->_tid, buffer, size)){
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("errorTLS")){
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
void awh::Client::stateTLS(const tls_t::id_t id, const tls_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("stateTLS"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const tls_t::id_t, const tls_t::state_t)> ("stateTLS", id, state);
		// Если состояние рукопожатия успешно завершено
		if(state == tls_t::state_t::HANDSHAKED){
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
void awh::Client::errorTLS(const tls_t::id_t id, const tls_t::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("errorTLS")){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(this->_eid), log_t::flag_t::CRITICAL, message.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, message.c_str());
			#endif
		// Выполняем функцию обратного вызова
		} else this->_callback.call <void (const tls_t::id_t, const tls_t::error_t, const string &)> ("errorTLS", id, error, message);
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
void awh::Client::processTLS([[maybe_unused]] const tls_t::id_t id, const tls_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_client->working()){
		/**
		 * Обрабатываем тип события TLS
		 */
		switch(static_cast <uint8_t> (event)){
			// Если событие шифрования данных TLS
			case static_cast <uint8_t> (tls_t::event_t::ENCRYPTION): {
				// Отправляем данные обратно клиенту, которые были зашифрованы TLS
				if(!this->_client->send(this->_eid, reinterpret_cast <const char *> (buffer), size)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, std::make_tuple(this->_eid, buffer, size), log_t::flag_t::WARNING);
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
			case static_cast <uint8_t> (tls_t::event_t::DECRYPTION): {
				// Если функция обратного вызова установлена
				if(this->_callback.is("read"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", this->_eid, buffer, size);
			} break;
		}
	}
}
/**
 * @brief Метод получения события DNS-резолвера
 *
 * @param status статус события DNS-резолвера
 */
void awh::Client::statusDNS(const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns->working()){
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
		if(this->_callback.is("attemptsDNS"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const string &, const uint8_t)> ("attemptsDNS", domain, attempts);
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
						/**
						 * В зависимости от статуса события клиента выполняем запуск
						 */
						switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_client)->status(this->_eid))){
							// Если событие клиента инициализировано, запускаем его
							case static_cast <uint8_t> (event::status_t::INITIAL):
							// Если событие находится в состоянии успешного подключения
							case static_cast <uint8_t> (event::status_t::SUCCESS): {
								// Если событие клиента не запущено, запускаем процесс клиента
								if(this->_client->launch(this->_eid))
									// Запускаем клиента
									this->_client->start();
							} break;
						}
					}
				} break;
				// Если событие клиента инициализировано, запускаем его
				case static_cast <uint8_t> (event::status_t::INITIAL):
				// Если событие находится в состоянии успешного подключения
				case static_cast <uint8_t> (event::status_t::SUCCESS): {
					// Если событие клиента не запущено, запускаем процесс клиента
					if(this->_client->launch(this->_eid))
						// Запускаем клиента
						this->_client->start();
				} break;
				// Если событие находится в состоянии в ожидании подключения
				case static_cast <uint8_t> (event::status_t::PENDING):
				// Если событие находится в состояние запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
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
/**
 * @brief Метод запуска клиента
 *
 */
void awh::Client::start() noexcept {
	// Если идентификатор клиента установлен
	if(this->_eid > 0){
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr){
			/**
			 * В зависимости от статуса события DNS-резолвера выполняем запуск
			 */
			switch(static_cast <uint8_t> (this->_dns->status())){
				// Если событие DNS-резолвера не запущено, запускаем его
				case static_cast <uint8_t> (event::status_t::NONE): {
					// Если событие DNS-резолвера не запущено, запускаем его
					if(this->_dns->commit())
						// Запускаем событие DNS-резолвера
						this->_dns->start();
				} break;
				// Если событие DNS-резолвера инициализировано, запускаем его
				case static_cast <uint8_t> (event::status_t::INITIAL):
					// Запускаем событие DNS-резолвера
					this->_dns->start();
				break;
			}
		// Если объект DNS-резолвера не установлен
		} else {
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
						/**
						 * В зависимости от статуса события клиента выполняем запуск
						 */
						switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_client)->status(this->_eid))){
							// Если событие клиента инициализировано, запускаем его
							case static_cast <uint8_t> (event::status_t::INITIAL):
							// Если событие находится в состоянии успешного подключения
							case static_cast <uint8_t> (event::status_t::SUCCESS): {
								// Если событие клиента не запущено, запускаем процесс клиента
								if(this->_client->launch(this->_eid))
									// Запускаем клиента
									this->_client->start();
							} break;
						}
					}
				} break;
				// Если событие клиента инициализировано, запускаем его
				case static_cast <uint8_t> (event::status_t::INITIAL):
				// Если событие находится в состоянии успешного подключения
				case static_cast <uint8_t> (event::status_t::SUCCESS): {
					// Если событие клиента не запущено, запускаем процесс клиента
					if(this->_client->launch(this->_eid))
						// Запускаем клиента
						this->_client->start();
				} break;
				// Если событие находится в состоянии в ожидании подключения
				case static_cast <uint8_t> (event::status_t::PENDING):
				// Если событие находится в состояние запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED):
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
/**
 * @brief Метод приостановки работы клиента
 *
 * @return результат выполнения приостановки работы
 */
bool awh::Client::pause() noexcept {
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @return результат выполнения возобновления работы
 */
bool awh::Client::resume() noexcept {
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод мультиподключения клиентов к удалённым хостам
 *
 * @return результат выполнения подключения
 */
bool awh::Client::connect() noexcept {
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
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера
 *
 * @return результат выполнения отключения
 */
bool awh::Client::disconnect() noexcept {
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
	if((this->_tid > 0) && (this->_tls != nullptr))
		// Устанавливаем режим безопасности работы потоков для объекта TLS
		this->_tls->threadSafety(this->_tid, mode);
	// Если идентификатор TLS не установлен, но объект TLS установлен
	else if((this->_tid == 0) && (this->_tls != nullptr)) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("TLS ID is not set", __PRETTY_FUNCTION__, std::make_tuple(mode), log_t::flag_t::WARNING);
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
	// Выполняем установку функции обратного вызова при отправке данных клиентом
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова на событие готовности клиента к работе
	this->_callback.set("ready", callback);
	// Выполняем установку функции обратного вызова при изменении состояния клиента
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова на событие неотправленных данных клиента
	this->_callback.set("spool", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова на событие изменения состояния клиента
	this->_callback.set("action", callback);
	// Выполняем установку функции обратного вызова при подключении клиента к удалённому серверу
	this->_callback.set("connect", callback);
	// Выполняем установку функции обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_callback.set("available", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок TLS
	this->_callback.set("errorTLS", callback);
	// Выполняем установку функции обратного вызова на событие получения состояния TLS
	this->_callback.set("stateTLS", callback);
	// Выполняем установку функции обратного вызова на событие завершения попыток резолвинга доменного имени DNS-резолвером
	this->_callback.set("attemptsDNS", callback);
}
/**
 * @brief Метод отправки данных серверу
 *
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных серверу
 */
size_t awh::Client::send(const void * buffer, const size_t size) noexcept {
	// Если идентификатор TLS и объект TLS установлены
	if((this->_tid > 0) && (this->_tls != nullptr)){
		// Если шифрование данных TLS выполнено успешно
		if(this->_tls->encrypt(this->_tid, buffer, size))
			// Возвращаем размер отправленных данных
			return size;
		// Выводим результат по умолчанию
		return 0;
	}
	// Выполняем отправку данных серверу
	return this->_client->send(this->_eid, buffer, size);
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
	/**
	 * Определяем тип полученного IP-адреса
	 */
	switch(static_cast <uint8_t> (this->_addr.host(target))){
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
					this->_log->debug("Client ID is not set", __PRETTY_FUNCTION__, std::make_tuple(target), log_t::flag_t::WARNING);
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
 * @brief Метод установки идентификатора события клиента
 *
 * @param eid идентификатор события для установки
 */
void awh::Client::setEventId(const event::id_t eid) noexcept {
	// Устанавливаем идентификатор события для клиента
	this->_eid = eid;
}
/**
 * @brief Метод установки идентификатора TLS
 *
 * @param tid идентификатор TLS для установки
 */
void awh::Client::setSecurityId(const tls_t::id_t tid) noexcept {
	// Если идентификатор TLS для установки передан и объект транспортного уровня безопасности установлен
	if((tid > 0) && (this->_tls != nullptr)){
		// Устанавливаем идентификатор TLS для клиента
		this->_tid = tid;
		// Устанавливаем функцию обратного вызова на событие состояния TLS
		this->_tls->on(this->_tid, std::bind(&client_t::stateTLS, this, _1, _2));
		// Устанавливаем функцию обратного вызова на событие ошибок TLS
		this->_tls->on(this->_tid, std::bind(&client_t::errorTLS, this, _1, _2, _3));
		// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
		this->_tls->on(this->_tid, std::bind(&client_t::processTLS, this, _1, _2, _3, _4));
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
 _host{""}, _tid(0), _eid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _tls(nullptr), _dns(nullptr), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
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
 * @param tls    объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Client::Client(unit::client_t * client, tls_t * tls, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _tid(0), _eid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _tls(tls), _dns(nullptr), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
		// Если объект транспортного уровня безопасности не установлен
		if(this->_tls == nullptr){
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
 _host{""}, _tid(0), _eid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _tls(nullptr), _dns(dns), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
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
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_client->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
		this->_client->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
		this->_client->on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr){
			// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
			this->_dns->on <void (const event::status_t)> ("status", &client_t::statusDNS, this, _1);
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
 * @param tls    объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Client::Client(unit::client_t * client, unit::dns_t * dns, tls_t * tls, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _tid(0), _eid(0), _addr(fmk, log), _callback(fmk, log),
 _timeoutDNS(3000), _tls(tls), _dns(dns), _client(client), _fmk(fmk), _log(log) {
	// Если объект клиента установлен
	if(this->_client != nullptr){
		// Если объект транспортного уровня безопасности не установлен
		if(this->_tls == nullptr){
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
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
		this->_client->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
		this->_client->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
		this->_client->on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr){
			// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
			this->_dns->on <void (const event::status_t)> ("status", &client_t::statusDNS, this, _1);
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
