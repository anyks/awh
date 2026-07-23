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
 * Подключаем заголовочный файл проекта
 */
#include <client/client.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Конструктор
 *
 */
awh::Client::Domain_Name_System::Domain_Name_System() noexcept :
 id(0), alive(15000), client(nullptr) {}
	
/**
 * @brief Конструктор
 *
 */
awh::Client::Identifier::Identifier() noexcept : eid(0), ctl(0) {}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Client::Unit::Unit(const fmk_t * fmk, const log_t * log) noexcept :
 addr(fmk, log), client(fmk, log) {}

/**
 * @brief Метод изменения статуса клиента
 *
 * @param index  индекс очереди запускаемого события
 * @param status новый статус клиента
 */
void awh::Client::status(const uint8_t index, const event::status_t status) noexcept {
	/**
	 * Временное состояние клиента
	 */
	switch(index){
		// Если мы получили статус события клиента
		case 0: {
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("status", status);
			// Если работа клиента запущена
			if(status == event::status_t::LAUNCHED){
				// Выполняем запуск работы клиента, если клиент не запущен
				if(!this->_unit->client.launch(this->_id.eid)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, this->_id.eid);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("This client ID=%u cannot be started", log_t::flag_t::WARNING, this->_id.eid);
						#endif
					}
				// Если клиент запущен удачно, выполняем функцию обратного вызова
				} else this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_unit->client.getTarget(this->_id.eid), this->_unit->client.getTargetPort(this->_id.eid));
			}
		} break;
		// Если мы получили статус события DNS-резолвера
		case 1: {
			/**
			 * В зависимости от статуса события DNS-резолвера выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие DNS-резолвера запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					/**
					 * Определяем принадлежит ли хост, к IP-адресу
					 */
					switch(static_cast <uint8_t> (this->_unit->addr.host(this->_host))){
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Если событие клиента не запущено, запускаем его
							if(awh_cast <unit::unit_t *> (&this->_unit->client)->status(this->_id.eid) == event::status_t::NONE){
								// Устанавливаем адрес хоста целевой машины для клиента
								if(this->_unit->client.setTarget(this->_id.eid, this->_host)){
									// Если событие клиента не запущено, запускаем его
									if(this->_unit->client.commit(this->_id.eid)){
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", this->_unit->client.family(this->_id.eid), this->_host, this->_unit->client.getTarget(this->_id.eid));
										// Запускаем клиента
										this->_unit->client.start();
									}
								}
							}
							// Выходим из функции
							return;
						}
					}
					// Выполняем резолвинг доменного имени
					if(!this->_dns.client->resolve(this->_dns.id, this->_unit->client.family(this->_id.eid), this->_host, this->_dns.alive.load(std::memory_order_acquire))){
						// Создаём текст ошибки резолвинга доменного имени
						const string error = this->_fmk->format("It was not possible to obtain an IP address for the domain name \"%s\"", this->_host.c_str());
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						// Выполняем функцию обратного вызова
						} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::NOT_FOUND, error);
					}
				} break;
				// Если событие DNS-резолвера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Останавливаем клиента
					this->_unit->client.stop();
				break;
			}
		} break;
	}
}
/**
 * @brief Метод обработки событий подключения клиента к удалённому серверу
 *
 * @param    идентификатор клиента
 * @param ok результат подключения
 */
void awh::Client::connect(const event::id_t, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если объект транспортного уровня безопасности установлен
		if((this->_coder != nullptr) && (this->_id.ctl > 0)){
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS не выполнено
				if(!this->_coder->handshake(this->_id.ctl)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error_tls")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("TLS handshake is failed", __PRETTY_FUNCTION__, make_tuple(ok), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("TLS handshake is failed", log_t::flag_t::WARNING);
						#endif
					}
				// Если рукопожатие TLS выполнено успешно, выходим из функции
				} else return;
			}
			// Если подключение успешное
			if(ok)
				// Нужно убить клиент, так-как TLS рукопожатие не выполнено
				this->_unit->client.destroy(this->_id.eid);
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const bool)> ("connect", false);
		// Если объект транспортного уровня безопасности не установлен, выполняем функцию обратного вызова
		} else this->_callback.call <void (const bool)> ("connect", ok);
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param      идентификатор клиента
 * @param size размер данных для записи
 */
void awh::Client::write(const event::id_t, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const size_t)> ("write", size);
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param        идентификатор клиента
 * @param status новый статус клиента
 */
void awh::Client::state(const event::id_t, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::status_t)> ("state", status);
		// Если статус клиента изменился на "уничтожен"
		if(status == event::status_t::DESTROYED){
			// Обнуляем идентификатор клиента
			this->_id.eid = 0;
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns.client->stop();
			// Останавливаем событие клиента
			else this->_unit->client.stop();
		}
	}
}
/**
 * @brief Метод обработки действий клиента
 *
 * @param        идентификатор клиента
 * @param action действие клиента
 */
void awh::Client::action(const event::id_t, const event::action_t action) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::action_t)> ("action", action);
}
/**
 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
 *
 * @param      идентификатор события
 * @param info информационные метаданные о дейтаграммном пакете
 */
void awh::Client::traffic(const event::id_t, const net::dgram_info_t & info) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const net::dgram_info_t &)> ("traffic", info);
}
/**
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param        идентификатор клиента
 * @param buffer буфер данных клиента
 * @param size   размер данных клиента
 */
void awh::Client::read(const event::id_t, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если объект транспортного уровня безопасности установлен
		if((this->_coder != nullptr) && (this->_id.ctl > 0)){
			// Если данные не расшифрованы
			if(!this->_coder->decrypt(this->_id.ctl, buffer, size)){
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error_tls")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("TLS decryption data is failed", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("TLS decryption data is failed", log_t::flag_t::WARNING);
					#endif
				}
			}
		// Если объект транспортного уровня безопасности не установлен, выполняем функцию обратного вызова
		} else this->_callback.call <void (const uint8_t *, const size_t)> ("read", buffer, size);
	}
}
/**
 * @brief Метод обработки события ошибки
 *
 * @param         идентификатор события
 * @param error   код ошибки
 * @param message сообщение об ошибке
 */
void awh::Client::error(const event::id_t, const event::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::error_t, const string &)> ("error", error, message);
}
/**
 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
 *
 * @param        идентификатор клиента
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 */
void awh::Client::available(const event::id_t, const event::status_t status, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::status_t, const size_t)> ("available", status, size);
}
/**
 * @brief Метод обработки событий истечения таймаута клиента
 *
 * @param        идентификатор клиента
 * @param action тип действия для истекшего таймаута
 * @param delay  задержка таймаута в миллисекундах
 * @return       нужно ли завершить клиента после истечения таймаута
 */
bool awh::Client::timeout(const event::id_t, const event::action_t action, const uint32_t delay) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("timeout");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid))
			// Выполняем функцию обратного вызова
			return this->_callback.call <bool (const event::action_t, const uint32_t)> (fid, action, delay);
	}
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки попыток подключения клиента к удалённому серверу
 *
 * @param          идентификатор DNS-запроса
 * @param domain   доменное имя для резолвинга
 * @param attempts количество попыток подключения
 */
void awh::Client::attempts(const unit::dns_t::id_t, const string & domain, const uint8_t attempts) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns.client->working())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const string &, const uint8_t)> ("attempts_dns", domain, attempts);
}
/**
 * @brief Метод обработки неудачного резолвинга доменного имени
 *
 * @param        идентификатор DNS-запроса
 * @param record тип записи DNS
 * @param domain доменное имя
 */
void awh::Client::failure(const unit::dns_t::id_t, const unit::dns_t::record_t record, const string & domain) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns.client->working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Останавливаем событие клиента
			this->_unit->client.destroy(this->_id.eid);
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const unit::dns_t::record_t, const string &)> ("failure_dns", record, domain);
	}
}
/**
 * @brief Метод обработки события неотправленных данных клиента
 *
 * @param        идентификатор клиента
 * @param error  тип ошибки отправки данных
 * @param buffer данные, которые не удалось отправить
 * @param size   размер данных, которые не удалось отправить
 */
void awh::Client::spool(const event::id_t, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::send_error_t, const uint8_t *, const size_t)> ("spool", error, buffer, size);
}
/**
 * @brief Метод резолвинга доменного имени в сетевой адрес
 *
 * @param        идентификатор DNS-запроса
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для резолвинга
 * @param addr   указатель на структуру для хранения результата резолвинга
 */
void awh::Client::resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns.client->working()){
		// Если событие клиента не запущено, запускаем его
		if(awh_cast <unit::unit_t *> (&this->_unit->client)->status(this->_id.eid) == event::status_t::NONE){
			// Устанавливаем адрес хоста целевой машины для клиента
			if(this->_unit->client.setTarget(this->_id.eid, addr)){
				// Выполняем фиксацию параметров клиента
				if(this->_unit->client.commit(this->_id.eid)){
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", family, domain, this->_unit->client.getTarget(this->_id.eid));
					// Запускаем клиента
					this->_unit->client.start();
				}
			}
		}
	}
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param       идентификатор TLS
 * @param state состояние TLS
 */
void awh::Client::stateTLS(const tls::coder_t::id_t, const tls::coder_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const tls::coder_t::state_t)> ("state_tls", state);
		// Если состояние рукопожатия успешно завершено
		if(state == tls::coder_t::state_t::HANDSHAKED)
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const bool)> ("connect", true);
	}
}
/**
 * @brief Метод получения ошибок TLS
 *
 * @param         идентификатор TLS
 * @param error   код ошибки TLS
 * @param message сообщение об ошибке TLS
 */
void awh::Client::errorTLS(const tls::coder_t::id_t, const tls::coder_t::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("error_tls")){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (error), message), log_t::flag_t::CRITICAL, message.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, message.c_str());
			#endif
		// Выполняем функцию обратного вызова
		} else this->_callback.call <void (const tls::coder_t::error_t, const string &)> ("error_tls", error, message);
	}
}
/**
  @brief Метод получения событий шифрования/дешифрования данных TLS
 *
 * @param        идентификатор TLS
 * @param event  тип события TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 * @param size   размер полезной нагрузки в буфере для события шифрования/дешифрования TLS
 */
void awh::Client::processTLS(const tls::coder_t::id_t, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Обрабатываем тип события TLS
		 */
		switch(static_cast <uint8_t> (event)){
			// Если событие шифрования данных TLS
			case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION): {
				// Отправляем данные обратно клиенту, которые были зашифрованы TLS
				if(!this->_unit->client.send(this->_id.eid, reinterpret_cast <const char *> (buffer), size)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (event), buffer, size), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Data cannot be sent to the server", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Если событие дешифрования данных TLS
			case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint8_t *, const size_t)> ("read", buffer, size);
			break;
		}
	}
}
/**
 * @brief Метод остановки клиента
 *
 */
void awh::Client::stop() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns.client->stop();
			// Останавливаем событие клиента
			else this->_unit->client.stop();
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод запуска клиента
 *
 */
void awh::Client::start() noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Запускаем событие DNS-резолвера
				this->_dns.client->start();
			// Если объект DNS-резолвера не установлен
			else {
				// Если событие клиента не запущено, запускаем его
				if(awh_cast <unit::unit_t *> (&this->_unit->client)->status(this->_id.eid) == event::status_t::NONE){
					// Выполняем фиксацию параметров клиента
					if(this->_unit->client.commit(this->_id.eid)){
						// Выполняем получение идентификатора функции обратного вызова
						const callback_t::id_t fid = this->_callback.id("ready");
						// Если функция обратного вызова установлена
						if(this->_callback.is(fid)){
							// Получаем адрес хоста целевой машины для клиента
							const string & host = this->_unit->client.getTarget(this->_id.eid);
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::family_t, const string &, const string &)> (fid, this->_unit->client.family(this->_id.eid), host, host);
						}
						// Запускаем клиента
						this->_unit->client.start();
					}
				}
			}
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
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
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Приостанавливаем событие клиента
			return this->_unit->client.pause(this->_id.eid);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @return результат выполнения возобновления работы
 */
bool awh::Client::resume() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Возобновляем событие клиента
			return this->_unit->client.resume(this->_id.eid);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод уничтожения события клиента
 *
 */
void awh::Client::destroy() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Уничтожаем событие клиента
			return this->_unit->client.destroy(this->_id.eid);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод проверки, жив ли клиент
 *
 * @return результат проверки
 */
bool awh::Client::isAlive() const noexcept {
	// Возвращаем результат проверки активности клиента
	return (this->_id.eid > 0);
}
/**
 * @brief Метод подключения клиента к удалённому хосту
 *
 * @return результат выполнения подключения
 */
bool awh::Client::connect() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Подключаем клиента к удалённому серверу
			return this->_unit->client.connect(this->_id.eid);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера
 *
 * @return результат выполнения отключения
 */
bool awh::Client::disconnect() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Отключаем клиента от удалённого сервера
			return this->_unit->client.disconnect(this->_id.eid);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
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
	// Выполняем установку функции обратного вызова при получении информационных метаданных о дейтаграммном пакете
	this->_callback.set("traffic", callback);
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
	// Выполняем установку функции обратного вызова на событие неудачного резолвинга доменного имени DNS-резолвером
	this->_callback.set("failure_dns", callback);
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
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Выполняем получение данных от сервера
			return this->_unit->client.recv(this->_id.eid);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
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
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0){
			// Если идентификатор TLS и объект TLS установлены
			if((this->_id.ctl > 0) && (this->_coder != nullptr)){
				// Если шифрование данных TLS выполнено успешно
				if(this->_coder->encrypt(this->_id.ctl, buffer, size))
					// Возвращаем размер отправленных данных
					return size;
				// Возвращаем значение по умолчанию
				return 0;
			}
			// Выполняем отправку данных серверу
			return this->_unit->client.send(this->_id.eid, buffer, size);
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод объединения данных между клиентом и другим событием
 *
 * @param eid    идентификатор события
 * @param direct направление объединения данных (клиент -> событие, событие -> клиент)
 * @return       результат выполнения объединения
 */
bool awh::Client::splice(const event::id_t eid, const event::direct_t direct) noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0){
		/**
		 * Обрабатываем направление объединения данных
		 */
		switch(static_cast <uint8_t> (direct)){
			// Если направление объединения данных от клиента к событию
			case static_cast <uint8_t> (event::direct_t::FORWARD):
				// Выполняем объединение данных между клиентом и другим событием
				return this->_unit->client.splice(this->_id.eid, eid);
			// Если направление объединения данных от события к клиенту
			case static_cast <uint8_t> (event::direct_t::REVERSE):
				// Выполняем объединение данных между другим событием и клиентом
				return this->_unit->client.splice(eid, this->_id.eid);
		}
	// Если идентификатор клиента не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (direct)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения опций клиента
 *
 * @return опции клиента
 */
uint16_t awh::Client::getOptions() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Извлекаем опции клиента
		return this->_unit->client.getOptions(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций клиента
 *
 * @param options опции клиента для установки
 * @return        результат выполнения установки
 */
bool awh::Client::setOptions(const uint16_t options) noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Устанавливаем опции клиента
		return this->_unit->client.setOptions(this->_id.eid, options);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(options), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции клиента
 *
 * @param option опция клиента для установки
 * @param mode   режим установки опции клиента
 * @return       результат выполнения установки
 */
bool awh::Client::setOption(const uint16_t option, const bool mode) noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Устанавливаем опцию клиента
		return this->_unit->client.setOption(this->_id.eid, option, mode);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(option, mode), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
 *
 * @return метаданные последнего принятого дейтаграммного пакета
 */
awh::net::dgram_info_t awh::Client::getTrafficInfo() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Получаем метаданные последнего принятого дейтаграммного пакета
		return this->_unit->client.getTrafficInfo(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return net::dgram_info_t();
}
/**
 * @brief Метод получения количества хопов последнего принятого пакета
 *
 * @return количество хопов последнего принятого пакета
 */
uint8_t awh::Client::getCountHops() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Получаем количество хопов последнего принятого пакета
		return this->_unit->client.getCountHops(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки количества хопов последнего принятого пакета
 *
 * @param hops количество хопов последнего принятого пакета
 * @return     результат выполнения установки
 */
bool awh::Client::setCountHops(const uint8_t hops) noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Устанавливаем количество хопов последнего принятого пакета
		return this->_unit->client.setCountHops(this->_id.eid, hops);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (hops)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @return максимальное количество хопов
 */
awh::event::hops_t awh::Client::getHops() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Получаем максимальное количество хопов
		return this->_unit->client.getHops(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 */
bool awh::Client::setHops(const event::hops_t hops) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем максимальное количество хопов
			return this->_unit->client.setHops(this->_id.eid, hops);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (hops)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса клиента
 *
 * @return сетевой интерфейс клиента
 */
string awh::Client::getIface() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Извлекаем сетевой интерфейс клиента
		return this->_unit->client.getIface(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
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
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем сетевой интерфейс клиента
			return this->_unit->client.setIface(this->_id.eid, name);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения внутреннего порта события
 *
 * @return внутренний порт события
 */
uint16_t awh::Client::getSourcePort() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Извлекаем внутренний порт события для клиента
		return this->_unit->client.getSourcePort(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки внутреннего порта события
 *
 * @param port внутренний порт события
 * @return     результат выполнения установки
 */
bool awh::Client::setSourcePort(const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем внутренний порт события для клиента
			return this->_unit->client.setSourcePort(this->_id.eid, port);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта удаленного сервера
 *
 * @return порт удаленного сервера
 */
uint16_t awh::Client::getTargetPort() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Извлекаем порт удаленного сервера для клиента
		return this->_unit->client.getTargetPort(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта удаленного сервера
 *
 * @param port порт удаленного сервера для установки
 * @return     результат выполнения установки
 */
bool awh::Client::setTargetPort(const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем порт удаленного сервера для клиента
			return this->_unit->client.setTargetPort(this->_id.eid, port);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Извлекаем адрес хоста целевой машины для клиента
		return this->_unit->client.getTarget(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::Client::setTarget(string_view target) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		/**
		 * Определяем тип полученного IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_unit->addr.host(target))){
			// Для типа Unix Domain Socket
			case static_cast <uint8_t> (net_addr_t::type_t::FS):
			// Для типа IPv4
			case static_cast <uint8_t> (net_addr_t::type_t::IPV4):
			// Для типа IPv6
			case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
				// Если идентификатор клиента установлен
				if(this->_id.eid > 0){
					// Устанавливаем адрес хоста целевой машины для клиента
					result = this->_unit->client.setTarget(this->_id.eid, target);
					// Если адрес установлен успешно, сохраняем его
					if(result)
						// Сохраняем адрес хоста целевой машины для клиента
						this->_host = this->_unit->client.getTarget(this->_id.eid);
				// Если идентификатор клиента не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(target), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
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
	// Возвращаем результат
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
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0){
			// Устанавливаем адрес хоста целевой машины для клиента
			const bool result = this->_unit->client.setTarget(this->_id.eid, target);
			// Если адрес установлен успешно, сохраняем его
			if(result)
				// Сохраняем адрес хоста целевой машины для клиента
				this->_host = this->_unit->client.getTarget(this->_id.eid);
			// Возвращаем результат установки адреса хоста целевой машины для клиента
			return result;
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Извлекаем адрес хоста целевой машины для клиента
		return this->_unit->client.getTarget(this->_id.eid, target);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Извлекаем адрес клиента
		return this->_unit->client.getAddress(this->_id.eid, address);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
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
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем адрес клиента
			return this->_unit->client.setAddress(this->_id.eid, address, value);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address), value), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
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
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем адрес клиента
			return this->_unit->client.setAddress(this->_id.eid, address, value);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Извлекаем адрес клиента
		return this->_unit->client.getAddress(this->_id.eid, address, value);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима трансляции пакетов клиента
 *
 * @return режим трансляции пакетов (unicast, multicast, broadcast)
 */
awh::event::delivery_mode_t awh::Client::getDelivery() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Извлекаем режим трансляции пакетов клиента
		return this->_unit->client.getDelivery(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::delivery_mode_t::NONE;
}
/**
 * @brief Метод установки режима трансляции пакетов клиента
 *
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 */
bool awh::Client::setDelivery(const event::delivery_mode_t delivery) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем режим трансляции пакетов клиента
			return this->_unit->client.setDelivery(this->_id.eid, delivery);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (delivery)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Извлекаем размер буфера клиента
		return this->_unit->client.getBufferSize(this->_id.eid, action);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Устанавливаем размер буфера клиента
		return this->_unit->client.setBufferSize(this->_id.eid, action, size);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action), size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения времени жизни DNS запроса
 *
 * @return время жизни DNS запроса в миллисекундах
 */
uint32_t awh::Client::getAliveDNS() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Возвращаем время жизни DNS запроса для клиента
		return this->_dns.alive.load(std::memory_order_acquire);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки времени жизни DNS запроса
 *
 * @param alive время жизни DNS запроса в миллисекундах
 */
void awh::Client::setAliveDNS(const uint32_t alive) noexcept {
	// Устанавливаем время жизни DNS запроса для клиента
	this->_dns.alive.store(alive, std::memory_order_release);
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @return режим использования таймаута на чтение события
 */
awh::event::usage_t awh::Client::getUsageReadTimeout() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Извлекаем режим использования таймаута на чтение события
		return this->_unit->client.getUsageReadTimeout(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 */
void awh::Client::setUsageReadTimeout(const event::usage_t usage) noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Устанавливаем режим использования таймаута на чтение события
		this->_unit->client.setUsageReadTimeout(this->_id.eid, usage);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (usage)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
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
	if(this->_id.eid > 0)
		// Извлекаем таймаут клиента
		return this->_unit->client.getTimeout(this->_id.eid, action);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Устанавливаем таймаут клиента
		this->_unit->client.setTimeout(this->_id.eid, action, timeout);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
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
	if(this->_id.eid > 0)
		// Устанавливаем пропускную способность клиента
		return this->_unit->client.bandwidth(this->_id.eid, limiting, bandwidth);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
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
	if(this->_id.eid > 0)
		// Устанавливаем параметры keep-alive для клиента
		return this->_unit->client.keepAlive(this->_id.eid, cnt, idle, intvl);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(cnt, idle, intvl), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @return значение DSCP
 */
awh::event::dscp_t awh::Client::getDifferentiatedServicesCodePoint() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Получаем значение DSCP для клиента
		return this->_unit->client.getDifferentiatedServicesCodePoint(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::dscp_t::CS0;
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param dscp значение DSCP
 * @return     результат работы функции
 */
bool awh::Client::setDifferentiatedServicesCodePoint(const event::dscp_t dscp) const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Устанавливаем значение DSCP для клиента
		return this->_unit->client.setDifferentiatedServicesCodePoint(this->_id.eid, dscp);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (dscp)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
 *
 * @return режим обнаружения максимального размера пакета (MTU)
 */
awh::event::mtu_discover_t awh::Client::getMaximumTransmissionUnitDiscover() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Получаем режим обнаружения максимального размера пакета (MTU) для клиента
		return this->_unit->client.getMaximumTransmissionUnitDiscover(this->_id.eid);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::mtu_discover_t::NONE;
}
/**
 * @brief Метод установки обнаружения максимального размера пакета (MTU)
 *
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 */
bool awh::Client::setMaximumTransmissionUnitDiscover(const event::mtu_discover_t mode) const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Устанавливаем режим обнаружения максимального размера пакета (MTU) для клиента
		return this->_unit->client.setMaximumTransmissionUnitDiscover(this->_id.eid, mode);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 */
bool awh::Client::membership(const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем активацию/деактивацию мультикаст группы для клиента
			return this->_unit->client.membership(this->_id.eid, mode, group, source, port);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode), group, source, port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 */
bool awh::Client::membership(const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0)
			// Устанавливаем активацию/деактивацию мультикаст группы для клиента
			return this->_unit->client.membership(this->_id.eid, mode, group, source, port);
		// Если идентификатор клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode), group, source, port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод инициализации клиента
 *
 * @param family   семейство адресов
 * @param type     тип события
 * @param protocol протокол события
 * @return         идентификатор созданного клиента
 */
awh::event::id_t awh::Client::init(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Если идентификатор клиента не установлен
	if(this->_id.eid == 0){
		// Если DNS-резолвер установлен
		if(this->_dns.client != nullptr)
			// Выдаём новый идентификатор DNS-резолвера для клиента
			this->_dns.id = this->_dns.client->issue();
		// Инициализируем нового события клиента
		this->_id.eid = this->_unit->client.issue(family, type, protocol);
	// Если идентификатор клиента не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("This client has already been initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("This client has already been initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return this->_id.eid;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Client::Client(const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _coder(nullptr), _fmk(fmk), _log(log) {
	// Создаём объект юнита клиента
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
	this->_unit->client.on <void (const event::status_t)> ("status", &client_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие записи данных
	this->_unit->client.on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие обработки действий клиента
	this->_unit->client.on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->client.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &client_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения данных клиентом
	this->_unit->client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_unit->client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_unit->client.on <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
	this->_unit->client.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_unit->client.on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
}
/**
 * @brief Конструктор
 *
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Client::Client(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _coder(nullptr), _fmk(fmk), _log(log) {
	// Создаём объект юнита клиента
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
	this->_unit->client.on <void (const event::status_t)> ("status", &client_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие записи данных
	this->_unit->client.on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие обработки действий клиента
	this->_unit->client.on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->client.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &client_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения данных клиентом
	this->_unit->client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_unit->client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_unit->client.on <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
	this->_unit->client.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_unit->client.on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
	// Устанавливаем переданный объект DNS-резолвера для клиента
	this->_dns.client = dns;
	// Если объект DNS-резолвера установлен
	if(this->_dns.client != nullptr){
		// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
		this->_dns.client->on <void (const event::status_t)> ("status", &client_t::status, this, 1, _1);
		// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
		this->_dns.client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки попыток подключения клиента к удалённому серверу
		this->_dns.client->on <void (const unit::dns_t::id_t, const string &, const uint8_t)> ("attempts", &client_t::attempts, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки событий неотправленных данных DNS-резолвера
		this->_dns.client->on <void (const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)>  ("failure", &client_t::failure, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
		this->_dns.client->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &client_t::resolve, this, _1, _2, _3, _4);
	// Если объект DNS-резолвера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("DNS resolver object is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("DNS resolver object is not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Конструктор
 *
 * @param ctl   идентификатор контекста безопасности
 * @param coder объект транспортного уровня безопасности
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 */
awh::Client::Client(const tls::coder_t::id_t ctl, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _coder(coder), _fmk(fmk), _log(log) {
	/**
	 * Устанавливаем идентификатор контекста безопасности и подписываемся
	 * на события транспортного уровня: до запуска клиента менять их некому,
	 * поэтому проверок рабочего состояния здесь не требуется
	 */
	if((ctl > 0) && (coder != nullptr)){
		// Устанавливаем идентификатор контекста безопасности
		this->_id.ctl = ctl;
		// Устанавливаем функцию обратного вызова на событие состояния TLS
		coder->on(this->_id.ctl, std::bind(&client_t::stateTLS, this, _1, _2));
		// Устанавливаем функцию обратного вызова на событие ошибок TLS
		coder->on(this->_id.ctl, std::bind(&client_t::errorTLS, this, _1, _2, _3));
		// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
		coder->on(this->_id.ctl, std::bind(&client_t::processTLS, this, _1, _2, _3, _4));
	}
	// Если объект транспортного уровня безопасности не установлен
	if(this->_coder == nullptr){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("TLS object is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("TLS object is not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
	// Создаём объект юнита клиента
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
	this->_unit->client.on <void (const event::status_t)> ("status", &client_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие записи данных
	this->_unit->client.on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие обработки действий клиента
	this->_unit->client.on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->client.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &client_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения данных клиентом
	this->_unit->client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_unit->client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_unit->client.on <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
	this->_unit->client.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_unit->client.on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
}
/**
 * @brief Конструктор
 *
 * @param ctl   идентификатор контекста безопасности
 * @param coder объект транспортного уровня безопасности
 * @param dns   объект DNS-резолвера
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 */
awh::Client::Client(const tls::coder_t::id_t ctl, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _coder(coder), _fmk(fmk), _log(log) {
	/**
	 * Устанавливаем идентификатор контекста безопасности и подписываемся
	 * на события транспортного уровня: до запуска клиента менять их некому,
	 * поэтому проверок рабочего состояния здесь не требуется
	 */
	if((ctl > 0) && (coder != nullptr)){
		// Устанавливаем идентификатор контекста безопасности
		this->_id.ctl = ctl;
		// Устанавливаем функцию обратного вызова на событие состояния TLS
		coder->on(this->_id.ctl, std::bind(&client_t::stateTLS, this, _1, _2));
		// Устанавливаем функцию обратного вызова на событие ошибок TLS
		coder->on(this->_id.ctl, std::bind(&client_t::errorTLS, this, _1, _2, _3));
		// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
		coder->on(this->_id.ctl, std::bind(&client_t::processTLS, this, _1, _2, _3, _4));
	}
	// Если объект транспортного уровня безопасности не установлен
	if(this->_coder == nullptr){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("TLS object is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("TLS object is not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
	// Создаём объект юнита клиента
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса клиента
	this->_unit->client.on <void (const event::status_t)> ("status", &client_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие записи данных
	this->_unit->client.on <void (const event::id_t, const size_t)> ("write", &client_t::write, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения состояния клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t)> ("state", &client_t::state, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие обработки действий клиента
	this->_unit->client.on <void (const event::id_t, const event::action_t)> ("action", &client_t::action, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->client.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &client_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения данных клиентом
	this->_unit->client.on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &client_t::read, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибок клиента
	this->_unit->client.on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута клиента
	this->_unit->client.on <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &client_t::timeout, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_unit->client.on <void (const event::id_t, const event::status_t, const size_t)> ("available", &client_t::available, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие неотправленных данных клиента
	this->_unit->client.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &client_t::spool, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие подключения клиента к удалённому серверу
	this->_unit->client.on <void (const event::id_t, const bool)> ("connect", static_cast <void (client_t::*)(const event::id_t, const bool)>(&client_t::connect), this, _1, _2);
	// Устанавливаем переданный объект DNS-резолвера для клиента
	this->_dns.client = dns;
	// Если объект DNS-резолвера установлен
	if(this->_dns.client != nullptr){
		// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
		this->_dns.client->on <void (const event::status_t)> ("status", &client_t::status, this, 1, _1);
		// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
		this->_dns.client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &client_t::error, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки попыток подключения клиента к удалённому серверу
		this->_dns.client->on <void (const unit::dns_t::id_t, const string &, const uint8_t)> ("attempts", &client_t::attempts, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки событий неотправленных данных DNS-резолвера
		this->_dns.client->on <void (const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)>  ("failure", &client_t::failure, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
		this->_dns.client->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &client_t::resolve, this, _1, _2, _3, _4);
	// Если объект DNS-резолвера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("DNS resolver object is not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("DNS resolver object is not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Деструктор
 *
 */
awh::Client::~Client() noexcept {
	// Удаляем объект юнита клиента
	this->_unit.reset(nullptr);
}
