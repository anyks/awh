/**
 * @file: server.cpp
 * @date: 2026-05-17
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
#include <server/server.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Метод изменения статуса сервера
 *
 * @param status новый статус сервера
 * @param state  новое временное состояние сервера
 */
void awh::Server::status(const event::status_t status, const state_t state) noexcept {
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
							this->_log->debug("This server ID=%u cannot be started", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (status)), log_t::flag_t::WARNING, this->_eid);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Выводим сообщение об ошибке
							this->_log->print("This server ID=%u cannot be started", log_t::flag_t::WARNING, this->_eid);
						#endif
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
				case static_cast <uint8_t> (event::status_t::DESTROYED): {
					// Если идентификатор TLS и объект TLS установлены
					if((this->_tid > 0) && (this->_coder != nullptr) && !this->_tls.empty()){
						// Выполняем блокировку потока для работы с TLS
						const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
						// Проходим по всем сопоставлениям идентификаторов клиентов с идентификаторами TLS
						for(auto i = this->_tls.begin(); i != this->_tls.end();){
							// Уничтожаем объект TLS по найденному идентификатору TLS
							this->_coder->destroy(i->second);
							// Удаляем сопоставление идентификатора клиента с идентификатором TLS
							i = this->_tls.erase(i);
						}
					}
					// Останавливаем сервер
					this->_server->stop();
				} break;
			}
		} break;
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid  идентификатор клиента
 * @param size размер данных для записи
 */
void awh::Server::write(const event::id_t eid, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("write"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const size_t)> ("write", eid, size);
	}
}
/**
 * @brief Метод обработки события разрешения подключения
 *
 * @param eid идентификатор сервера
 * @param cid идентификатор клиента
 */
void awh::Server::accept(const event::id_t eid, const event::id_t cid) noexcept {
	// Если объект транспортного уровня безопасности установлен
	if((this->_coder != nullptr) && (this->_tid > 0)){
		// Создаём идентификатор транспортного уровня TLS/DTLS
		tls::coder_t::id_t ctl = this->_coder->transport(this->_tid);
		{
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Добавляем сопоставление идентификатора клиента с идентификатором TLS
			this->_tls.emplace(cid, ctl);
		}
		/**
		 * Определяем семейство адресов с которым работает клиент
		 */
		switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(cid))){
			// Если клиент работает с адресами IPv4
			case static_cast <uint8_t> (event::family_t::IPV4):
				// Устанавливаем клиента TLS для события
				this->_coder->peer(ctl, this->_server->getAddress(cid, event::address_t::IPV4), this->_server->getPort(cid));
			break;
			// Если клиент работает с адресами IPv6
			case static_cast <uint8_t> (event::family_t::IPV6):
				// Устанавливаем клиента TLS для события
				this->_coder->peer(ctl, this->_server->getAddress(cid, event::address_t::IPV6), this->_server->getPort(cid));
			break;
		}
		// Регистрируем функцию обратного вызова на изменение состояния TLS
		this->_coder->on(ctl, std::bind(&server_t::stateTLS, this, _1, cid, _2));
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(ctl, std::bind(&server_t::errorTLS, this, _1, cid, _2, _3));
		// Регистрируем функцию обратного вызова на получение снимка браузера приславшего ClientHello
		this->_coder->on(ctl, std::bind(&server_t::fingerprintTLS, this, _1, cid, _2));
		// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
		this->_coder->on(ctl, std::bind(&server_t::processTLS, this, _1, cid, _2, _3, _4));
		// Если рукопожатие TLS не выполнено
		if(!this->_coder->handshake(ctl)){
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("errorTLS")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("TLS handshake process was not completed", __PRETTY_FUNCTION__, std::make_tuple(eid, cid, ctl), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("TLS handshake process was not completed", log_t::flag_t::WARNING);
				#endif
			}
		}
	// Если объект транспортного уровня безопасности не установлен
	} else {
		// Если функция обратного вызова установлена
		if(this->_callback.is("accept"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", eid, cid, 0);
	}
}
/**
 * @brief Метод обработки событий изменения состояния сервера
 *
 * @param eid    идентификатор сервера
 * @param status новый статус сервера
 */
void awh::Server::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("state"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
		// Если статус сервера изменился на "уничтожен"
		if(status == event::status_t::DESTROYED){
			// Если объект DNS-резолвера установлен
			if(this->_dns != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns->stop();
			// Если объект DNS-резолвера не установлен
			else {
				// Если идентификатор TLS и объект TLS установлены
				if((this->_tid > 0) && (this->_coder != nullptr) && !this->_tls.empty()){
					// Выполняем блокировку потока для работы с TLS
					const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
					// Проходим по всем сопоставлениям идентификаторов клиентов с идентификаторами TLS
					for(auto i = this->_tls.begin(); i != this->_tls.end();){
						// Уничтожаем объект TLS по найденному идентификатору TLS
						this->_coder->destroy(i->second);
						// Удаляем сопоставление идентификатора клиента с идентификатором TLS
						i = this->_tls.erase(i);
					}
				}
				// Останавливаем событие сервера
				this->_server->stop();
			}
		}
	}
}
/**
 * @brief Метод обработки действий сервера
 *
 * @param eid    идентификатор сервера
 * @param action действие сервера
 */
void awh::Server::action(const event::id_t eid, const event::action_t action) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("action"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::action_t)> ("action", eid, action);
	}
}
/**
 * @brief Метод обработки событий получения данных сервером
 *
 * @param eid    идентификатор сервера
 * @param buffer буфер данных сервера
 * @param size   размер данных сервера
 */
void awh::Server::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если объект транспортного уровня безопасности установлен
		if((this->_coder != nullptr) && (this->_tid > 0)){
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск идентификатора TLS по идентификатору события клиента
			auto i = this->_tls.find(eid);
			// Если для данного идентификатора события клиента найден идентификатор TLS
			if(i != this->_tls.end()){
				// Если данные не расшифрованы
				if(!this->_coder->decrypt(i->second, buffer, size)){
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
				// Выходим из функции
				return;
			}
		}
		// Если функция обратного вызова установлена
		if(this->_callback.is("read"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, buffer, size);
	}
}
/**
 * @brief Метод получения события ошибок
 *
 * @param eid     идентификатор события
 * @param error   код ошибки
 * @param message сообщение об ошибке
 */
void awh::Server::error(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
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
void awh::Server::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
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
bool awh::Server::timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("timeout"))
			// Выполняем функцию обратного вызова
			return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t)> ("timeout", eid, action, delay);
	}
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки события неотправленных данных клиенту
 *
 * @param eid   идентификатор клиента
 * @param error тип ошибки отправки данных
 * @param data  данные, которые не получилось отправить
 * @param size  размер данных, которые не получилось отправить
 */
void awh::Server::spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("spool"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", eid, error, buffer, size);
	}
}
/**
 * @brief Метод обработки события пересоздания процесса
 *
 * @param old старый идентификатор процесса
 * @param pid текущий идентификатор процесса
 */
void awh::Server::rebaseCluster(const pid_t old, const pid_t pid) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("clusterRebase"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const pid_t)> ("clusterRebase", old, pid);
}
/**
 * @brief Метод получения события завершения работы процесса
 *
 * @param pid    идентификатор процесса
 * @param signal сигнал с которым завершился процесс
 */
void awh::Server::exitCluster(const pid_t pid, const int32_t signal) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("clusterExit"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const int32_t)> ("clusterExit", pid, signal);
}
/**
 * @brief Метод обработки события отправки сообщения процессу кластера
 *
 * @param pid  идентификатор процесса
 * @param size размер отправленного сообщения
 */
void awh::Server::sendingCluster(const pid_t pid, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("clusterSending"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const size_t)> ("clusterSending", pid, size);
}
/**
 * @brief Метод обработки событий изменения статуса кластера
 *
 * @param pid    идентификатор события
 * @param status новый статус кластера
 */
void awh::Server::stateCluster(const pid_t pid, const event::status_t status) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("clusterState"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const event::status_t)> ("clusterState", pid, status);
}
/**
 * @brief Метод получения событий активации/деактивации кластера
 *
 * @param pid   идентификатор процесса
 * @param event флаг события кластера
 */
void awh::Server::eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept {
	// Если функция получения событий кластера установлена
	if(this->_callback.is("clusterEvents"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const unit::cluster_t::event_t)>  ("clusterEvents", pid, event);
}
/**
 * @brief Метод обработки события получения сообщения от процесса кластера
 *
 * @param pid  идентификатор процесса
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 */
void awh::Server::messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("clusterMessage"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("clusterMessage", pid, data, size);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
 *
 * @param pid    идентификатор процесса
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 */
void awh::Server::availableCluster(const pid_t pid, const event::status_t status, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("clusterAvailable"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("clusterAvailable", pid, status, size);
}
/**
 * @brief Метод обработки событий ошибок кластера
 *
 * @param pid         идентификатор процесса
 * @param error       тип ошибки
 * @param description описание ошибки
 */
void awh::Server::errorCluster(const pid_t pid, const event::error_t error, const string & description) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("clusterError"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("clusterError", pid, error, description);
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param id    идентификатор TLS
 * @param eid   идентификатор клиента
 * @param state состояние TLS
 */
void awh::Server::stateTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("stateTLS"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const tls::coder_t::id_t, const event::id_t, const tls::coder_t::state_t)> ("stateTLS", id, eid, state);
		/**
		 * Обрабатываем входящие состояния DTLS
		 */
		switch(static_cast <uint8_t> (state)){
			// Если состояние ошибки транспортного уровня
			case static_cast <uint8_t> (tls::coder_t::state_t::FAILED): {
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("errorTLS")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("TLS is failed", __PRETTY_FUNCTION__, std::make_tuple(id, eid, static_cast <uint16_t> (state)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("TLS is failed", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Если состояние уничтожения объекта транспортного уровня
			case static_cast <uint8_t> (tls::coder_t::state_t::DESTROYED): {
				// Выполняем блокировку потока для работы с TLS
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Удаляем сопоставление идентификатора клиента с идентификатором TLS
				this->_tls.erase(eid);
			} break;
			// Если состояние рукопожатия успешно завершено
			case static_cast <uint8_t> (tls::coder_t::state_t::HANDSHAKED): {
				// Если функция обратного вызова установлена
				if(this->_callback.is("accept"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", this->_eid, eid, id);
			} break;
		}
	}
}
/**
 * @brief Метод получения отпечатка TLS
 *
 * @param id      идентификатор TLS
 * @param eid     идентификатор клиента
 * @param browser информация о браузере для отпечатка TLS
 */
void awh::Server::fingerprintTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::fgp_t::browser_t & browser) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова установлена
		if(this->_callback.is("fingerprintTLS"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const tls::coder_t::id_t, const event::id_t, const tls::fgp_t::browser_t &)> ("fingerprintTLS", id, eid, browser);
	}
}
/**
 * @brief Метод получения ошибок TLS
 *
 * @param id      идентификатор TLS
 * @param eid     идентификатор клиента
 * @param error   код ошибки TLS
 * @param message сообщение об ошибке TLS
 */
void awh::Server::errorTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("errorTLS")){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, eid, static_cast <uint16_t> (error), message), log_t::flag_t::CRITICAL, message.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, message.c_str());
			#endif
		// Выполняем функцию обратного вызова
		} else this->_callback.call <void (const tls::coder_t::id_t, const event::id_t, const tls::coder_t::error_t, const string &)> ("errorTLS", id, eid, error, message);
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
void awh::Server::processTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->_dns != nullptr ? this->_dns->working() : this->_server->working()){
		/**
		 * Обрабатываем тип события TLS
		 */
		switch(static_cast <uint8_t> (event)){
			// Если событие шифрования данных TLS
			case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION): {
				// Отправляем данные обратно клиенту, которые были зашифрованы TLS
				if(!this->_server->send(eid, reinterpret_cast <const char *> (buffer), size)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Data cannot be sent to the server", __PRETTY_FUNCTION__, std::make_tuple(id, eid, buffer, size), log_t::flag_t::WARNING);
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
					this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, buffer, size);
			} break;
		}
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
void awh::Server::resolveDNS(const unit::dns_t::id_t id, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns->working()){
		/**
		 * Определяем семейство адресов с которым работает сервер
		 */
		switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(this->_eid))){
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
			} break;
		}
	}
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::Server::stop() noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0){
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr)
			// Останавливаем событие DNS-резолвера
			this->_dns->stop();
		// Если объект DNS-резолвера не установлен
		else {
			// Если идентификатор TLS и объект TLS установлены
			if((this->_tid > 0) && (this->_coder != nullptr) && !this->_tls.empty()){
				// Выполняем блокировку потока для работы с TLS
				const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Проходим по всем сопоставлениям идентификаторов клиентов с идентификаторами TLS
				for(auto i = this->_tls.begin(); i != this->_tls.end();){
					// Уничтожаем объект TLS по найденному идентификатору TLS
					this->_coder->destroy(i->second);
					// Удаляем сопоставление идентификатора клиента с идентификатором TLS
					i = this->_tls.erase(i);
				}
			}
			// Останавливаем событие сервера
			this->_server->stop();
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
/**
 * @brief Метод запуска сервера
 *
 */
void awh::Server::start() noexcept {
	// Если идентификатор сервера установлен
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
/**
 * @brief Метод перевода события в режим прослушивания входящих соединений
 *
 * @param max максимальное количество входящих соединений
 * @return    результат выполнения перевода в режим прослушивания
 */
bool awh::Server::listen(const uint16_t max) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Переводим событие сервера в режим прослушивания входящих соединений
		return this->_server->listen(this->_eid, max);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(max), log_t::flag_t::WARNING);
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
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::Server::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для объекта блокировки
	this->_mtx.enabled = mode;
	// Устанавливаем режим безопасности работы потоков для функций обратного вызова
	this->_callback.threadSafety(mode);
	// Устанавливаем режим безопасности работы потоков для объекта сервера
	this->_server->threadSafety(mode);
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
void awh::Server::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на событие получение данных от клиента
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при отправке данных клиенту
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова на событие готовности сервера к работе
	this->_callback.set("ready", callback);
	// Выполняем установку функции обратного вызова при изменении состояния сервера
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова на событие неотправленных данных клиентом
	this->_callback.set("spool", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова на событие изменения статуса сервера
	this->_callback.set("status", callback);
	// Выполняем установку функции обратного вызова на событие изменения состояния сервера
	this->_callback.set("action", callback);
	// Выполняем установку функции обратного вызова при принятии входящего соединения от клиента
	this->_callback.set("accept", callback);
	// Выполняем установку функции обратного вызова на событие истечения таймаута клиента
	this->_callback.set("timeout", callback);
	// Выполняем установку функции обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_callback.set("available", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок TLS
	this->_callback.set("errorTLS", callback);
	// Выполняем установку функции обратного вызова на событие получения состояния TLS
	this->_callback.set("stateTLS", callback);
	// Выполняем установку функции обратного вызова на событие получения отпечатка ClientHello TLS
	this->_callback.set("fingerprintTLS", callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса кластера
	this->_callback.set("clusterExit", callback);
	// Выполняем установку функции обратного вызова при получении ошибок кластера
	this->_callback.set("clusterError", callback);
	// Выполняем установку функции обратного вызова при получении состояния процесса кластера
	this->_callback.set("clusterState", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса кластера
	this->_callback.set("clusterRebase", callback);
	// Выполняем установку функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКЕ процесса кластера
	this->_callback.set("clusterEvents", callback);
	// Выполняем установку функции обратного вызова при отправке сообщения кластера
	this->_callback.set("clusterSending", callback);
	// Выполняем установку функции обратного вызова при получении сообщения кластера
	this->_callback.set("clusterMessage", callback);
	// Выполняем установку функции обратного вызова при получении доступности размера очереди сообщений кластера
	this->_callback.set("clusterAvailable", callback);
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @return сетевой интерфейс сервера
 */
string awh::Server::getIface() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Извлекаем сетевой интерфейс сервера
		return this->_server->getIface(this->_eid);
	// Если идентификатор сервера не установлен
	else {
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
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::Server::setIface(string_view name) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Устанавливаем сетевой интерфейс сервера
		return this->_server->setIface(this->_eid, name);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::WARNING);
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
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 */
bool awh::Server::membership(const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Устанавливаем активацию/деактивацию мультикаст группы для сервера
		return this->_server->membership(this->_eid, mode, group, source, port);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (mode), group, source, port), log_t::flag_t::WARNING);
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
 * @brief Метод активации/деактивации мультикаст группы
 *
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 */
bool awh::Server::membership(const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Устанавливаем активацию/деактивацию мультикаст группы для сервера
		return this->_server->membership(this->_eid, mode, group, source, port);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (mode), group, source, port), log_t::flag_t::WARNING);
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
 * @brief Метод приостановки работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения приостановки работы
 */
bool awh::Server::pause(const event::id_t eid) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Приостанавливаем событие клиента
		return this->_server->pause(eid);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
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
 * @brief Метод возобновления работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 */
bool awh::Server::resume(const event::id_t eid) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Возобновляем событие клиента
		return this->_server->resume(eid);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
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
 * @brief Метод уничтожения события клиента
 *
 * @param eid идентификатор события клиента для уничтожения
 */
void awh::Server::destroy(const event::id_t eid) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid)){
		// Если идентификатор TLS и объект TLS установлены
		if((this->_tid > 0) && (this->_coder != nullptr)){
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск идентификатора TLS по идентификатору события клиента
			auto i = this->_tls.find(eid);
			// Если для данного идентификатора события клиента найден идентификатор TLS
			if(i != this->_tls.end()){
				// Уничтожаем объект TLS по найденному идентификатору TLS
				this->_coder->destroy(i->second);
				// Удаляем сопоставление идентификатора клиента с идентификатором TLS
				this->_tls.erase(i);
			}
		}
		// Уничтожаем событие клиента
		this->_server->destroy(eid);
	}
}
/**
 * @brief Метод получения данных от клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 */
bool awh::Server::recv(const event::id_t eid) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Получаем данные от клиента
		return this->_server->recv(eid);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
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
size_t awh::Server::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid)){
		// Если идентификатор TLS и объект TLS установлены
		if((this->_tid > 0) && (this->_coder != nullptr)){
			// Выполняем блокировку потока для работы с TLS
			const locker_t <std::shared_mutex> lock(this->_mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск идентификатора TLS по идентификатору события клиента
			auto i = this->_tls.find(eid);
			// Если для данного идентификатора события клиента найден идентификатор TLS
			if(i != this->_tls.end()){
				// Если шифрование данных TLS выполнено успешно
				if(this->_coder->encrypt(i->second, buffer, size))
					// Возвращаем размер отправленных данных
					return size;
			}
			// Выводим результат по умолчанию
			return 0;
		}
		// Отправляем данные клиенту
		return this->_server->send(eid, buffer, size);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
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
 * @brief Метод перемещения данных между сервером и другим событием
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения перемещения
 */
bool awh::Server::splice(const event::id_t eid, const event::id_t dest) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Перемещаем данные между событиями
		return this->_server->splice(eid, dest);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, dest), log_t::flag_t::WARNING);
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
 * @brief Метод получения опций клиента
 *
 * @param eid идентификатор события клиента
 * @return    опции клиента
 */
uint16_t awh::Server::getOptions(const event::id_t eid) const noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Получаем опции клиента
		return this->_server->getOptions(eid);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
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
	return 0;
}
/**
 * @brief Метод установки опций клиента
 *
 * @param eid     идентификатор события клиента
 * @param options опции клиента для установки
 * @return        результат выполнения установки
 */
bool awh::Server::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Устанавливаем опции клиента
		return this->_server->setOptions(eid, options);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, options), log_t::flag_t::WARNING);
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
 * @brief Метод установки опции клиента
 *
 * @param eid    идентификатор события клиента
 * @param option опция клиента для установки
 * @param mode   режим установки опции клиента
 * @return       результат выполнения установки
 */
bool awh::Server::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Устанавливаем опцию клиента
		return this->_server->setOption(eid, option, mode);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, option, mode), log_t::flag_t::WARNING);
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
 * @brief Метод получения порта сервера
 *
 * @return порт сервера
 */
uint16_t awh::Server::getPort() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Извлекаем порт текущего сервера
		return this->_server->getPort(this->_eid);
	// Если идентификатор сервера не установлен
	else {
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
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта сервера
 *
 * @param port порт сервера для установки
 * @return     результат выполнения установки
 */
bool awh::Server::setPort(const uint16_t port) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Устанавливаем порт текущего сервера
		return this->_server->setPort(this->_eid, port);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(port), log_t::flag_t::WARNING);
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
 * @brief Метод получения порта удаленного клиента
 *
 * @param eid идентификатор события клиента
 * @return    порт удаленного клиента
 */
uint16_t awh::Server::getPort(const event::id_t eid) const noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Извлекаем порт удаленного клиента
		return this->_server->getPort(eid);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
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
	return 0;
}
/**
 * @brief Метод получения адреса хоста текущей машины
 *
 * @return адрес хоста текущей машины
 */
string awh::Server::getHost() const noexcept {
	// Выводим адрес хоста текущей машины для сервера
	return this->_host;
}
/**
 * @brief Метод установки адреса хоста текущей машины
 *
 * @param host адрес хоста текущей машины
 * @return     результат выполнения установки
 */
bool awh::Server::setHost(string_view host) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Определяем тип полученного IP-адреса
	 */
	switch(static_cast <uint8_t> (this->_addr.host(host))){
		// Для типа Unix Domain Socket
		case static_cast <uint8_t> (net_addr_t::type_t::FS): {
			// Если идентификатор сервера установлен
			if(this->_eid > 0){
				// Устанавливаем адрес хоста целевой машины для сервера
				result = this->_server->setAddress(this->_eid, event::address_t::UDS, host);
				// Если адрес установлен успешно, сохраняем его
				if(result)
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::UDS);
			// Если идентификатор сервера не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(host), log_t::flag_t::WARNING);
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
			// Если идентификатор сервера установлен
			if(this->_eid > 0){
				// Устанавливаем адрес хоста целевой машины для сервера
				result = this->_server->setAddress(this->_eid, event::address_t::IPV4, host);
				// Если адрес установлен успешно, сохраняем его
				if(result)
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::IPV4);
			// Если идентификатор сервера не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(host), log_t::flag_t::WARNING);
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
			// Если идентификатор сервера установлен
			if(this->_eid > 0){
				// Устанавливаем адрес хоста целевой машины для сервера
				result = this->_server->setAddress(this->_eid, event::address_t::IPV6, host);
				// Если адрес установлен успешно, сохраняем его
				if(result)
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::IPV6);
			// Если идентификатор сервера не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(host), log_t::flag_t::WARNING);
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
			// Если адрес не является IP-адресом, устанавливаем его как есть
			if((result = !host.empty()))
				// Устанавливаем адрес хоста целевой машины для сервера
				this->_host = host;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param address тип адреса сервера
 * @return        значение адреса сервера
 */
string awh::Server::getAddress(const event::address_t address) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Извлекаем адрес сервера
		return this->_server->getAddress(this->_eid, address);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::Server::setAddress(const event::address_t address, string_view value) noexcept {
	// Результат работы функции
	bool result = false;
	// Если идентификатор сервера установлен
	if(this->_eid > 0){
		// Устанавливаем адрес сервера
		if((result = this->_server->setAddress(this->_eid, address, value))){
			/**
			 * Определяем семейство адресов с которым работает сервер
			 */
			switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(this->_eid))){
				// Если сервер работает с адресами Unix Domain Socket
				case static_cast <uint8_t> (event::family_t::UDS):
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::UDS);
				break;
				// Если сервер работает с адресами IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::IPV4);
				break;
				// Если сервер работает с адресами IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::IPV6);
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
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (address), value), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @return        значение адреса клиента
 */
string awh::Server::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Извлекаем адрес удаленного клиента
		return this->_server->getAddress(eid, address);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 */
bool awh::Server::setAddress(const event::address_t address, const net::addr_t * value) noexcept {
	// Результат работы функции
	bool result = false;
	// Если идентификатор сервера установлен
	if(this->_eid > 0){
		// Устанавливаем адрес сервера
		if((result = this->_server->setAddress(this->_eid, address, value))){
			/**
			 * Определяем семейство адресов с которым работает сервер
			 */
			switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (this->_server)->family(this->_eid))){
				// Если сервер работает с адресами IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::IPV4);
				break;
				// Если сервер работает с адресами IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::IPV6);
				break;
				// Если сервер работает с адресами Unix Domain Socket
				case static_cast <uint8_t> (event::family_t::UDS):
					// Сохраняем адрес хоста целевой машины для сервера
					this->_host = this->_server->getAddress(this->_eid, event::address_t::UDS);
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
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   объект для извлечения адреса сервера
 * @return        результат выполнения извлечения адреса сервера
 */
bool awh::Server::getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Извлекаем адрес сервера
		return this->_server->getAddress(this->_eid, address, value);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
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
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   объект для извлечения адреса клиента
 * @return        результат выполнения извлечения адреса клиента
 */
bool awh::Server::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Извлекаем адрес удаленного клиента
		return this->_server->getAddress(eid, address, value);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
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
 * @brief Метод получения размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       размер буфера клиента
 */
size_t awh::Server::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Извлекаем размер буфера клиента
		return this->_server->getBufferSize(eid, action);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
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
 * @brief Метод установки размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @param size   размер буфера клиента
 * @return       результат выполнения установки
 */
bool awh::Server::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Устанавливаем размер буфера клиента
		return this->_server->setBufferSize(eid, action, size);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (action), size), log_t::flag_t::WARNING);
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
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @return режим использования таймаута на чтение события
 */
awh::event::usage_t awh::Server::getUsageReadTimeout() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Извлекаем режим использования таймаута на чтение события
		return this->_server->getUsageReadTimeout(this->_eid);
	// Если идентификатор сервера не установлен
	else {
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
	// Выводим результат по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод получения режима использования таймаута на чтение события клиента
 *
 * @param eid идентификатор события клиента
 * @return    режим использования таймаута на чтение события клиента
 */
awh::event::usage_t awh::Server::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Извлекаем режим использования таймаута на чтение события клиента
		return this->_server->getUsageReadTimeout(eid);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
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
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 */
void awh::Server::setUsageReadTimeout(const event::usage_t usage) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Устанавливаем режим использования таймаута на чтение события
		this->_server->setUsageReadTimeout(this->_eid, usage);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (usage)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки режима использования таймаута на чтение события клиента
 *
 * @param eid   идентификатор события клиента
 * @param usage режим использования таймаута на чтение события клиента (reusable или disposable)
 */
void awh::Server::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Устанавливаем режим использования таймаута на чтение события клиента
		this->_server->setUsageReadTimeout(eid, usage);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (usage)), log_t::flag_t::WARNING);
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
 * @brief Метод получения таймаута сервера
 *
 * @param action тип действия сервера
 * @return       значение таймаута в миллисекундах
 */
uint32_t awh::Server::getTimeout(const event::action_t action) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Извлекаем таймаут сервера
		return this->_server->getTimeout(this->_eid, action);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод получения таймаута клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       значение таймаута в миллисекундах
 */
uint32_t awh::Server::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Извлекаем таймаут клиента
		return this->_server->getTimeout(eid, action);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
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
 * @brief Метод установки таймаута сервера
 *
 * @param action  тип действия сервера
 * @param timeout значение таймаута в миллисекундах
 */
void awh::Server::setTimeout(const event::action_t action, const uint32_t timeout) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Устанавливаем таймаут сервера
		this->_server->setTimeout(this->_eid, action, timeout);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server ID is not set", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки таймаута клиента
 *
 * @param eid     идентификатор события клиента
 * @param action  тип действия клиента
 * @param timeout значение таймаута в миллисекундах
 */
void awh::Server::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Устанавливаем таймаут клиента
		this->_server->setTimeout(eid, action, timeout);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
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
 * @brief Метод установки пропускной способности сервера
 *
 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 */
bool awh::Server::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если идентификатор сервера установлен
	if(this->_eid > 0)
		// Устанавливаем пропускную способность сервера
		this->_server->bandwidth(this->_eid, limiting, bandwidth);
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server ID is not set", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
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
 * @brief Метод установки пропускной способности клиента
 *
 * @param eid       идентификатор события клиента
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 */
bool awh::Server::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Устанавливаем пропускную способность клиента
		this->_server->bandwidth(eid, limiting, bandwidth);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
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
 * @brief Метод установки параметров keep-alive для клиента
 *
 * @param eid   идентификатор события клиента
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 */
bool awh::Server::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если идентификатор клиента найден в списке обслуживаемых клиентов
	if((eid != this->_eid) && this->_server->isActual(eid))
		// Устанавливаем параметры keep-alive для клиента
		this->_server->keepAlive(eid, cnt, idle, intvl);
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, std::make_tuple(eid, cnt, idle, intvl), log_t::flag_t::WARNING);
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
 * @brief Метод установки идентификатора события сервера
 *
 * @param eid идентификатор события для установки
 */
void awh::Server::setEventId(const event::id_t eid) noexcept {
	// Устанавливаем идентификатор события для сервера
	this->_eid = eid;
}
/**
 * @brief Метод установки идентификатора TLS шаблона
 *
 * @param tid идентификатор TLS шаблона для установки
 */
void awh::Server::setSecurityId(const tls::coder_t::id_t tid) noexcept {
	// Устанавливаем идентификатор TLS шаблона для сервера
	this->_tid = tid;
}
/**
 * @brief Конструктор
 *
 * @param server объект юнита сервера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Server::Server(unit::server_t * server, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _dns(nullptr), _coder(nullptr), _server(server), _fmk(fmk), _log(log)  {
	// Если объект сервера установлен
	if(this->_server != nullptr){
		// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
		this->_server->on <void (const event::status_t)> ("status", &server_t::status, this, _1, state_t::SERVER);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_server->on <void (const event::id_t, const size_t)> ("write", &server_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
		this->_server->on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
		this->_server->on <void (const event::id_t, const event::status_t)> ("state", &server_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий сервера
		this->_server->on <void (const event::id_t, const event::action_t)> ("action", &server_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных сервером
		this->_server->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &server_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок сервера
		this->_server->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
		this->_server->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
		this->_server->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &server_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных сервера
		this->_server->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &server_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
		this->_server->on <void (const pid_t, const int32_t)> ("clusterExit", &server_t::exitCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
		this->_server->on <void (const pid_t, const pid_t)> ("clusterRebase", &server_t::rebaseCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
		this->_server->on <void (const pid_t, const size_t)> ("clusterSending", &server_t::sendingCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
		this->_server->on <void (const pid_t, const event::status_t)> ("clusterState", &server_t::stateCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
		this->_server->on <void (const pid_t, const unit::cluster_t::event_t)> ("clusterEvents", &server_t::eventsCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
		this->_server->on <void (const pid_t, const uint8_t *, const size_t)> ("clusterMessage", &server_t::messageCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
		this->_server->on <void (const pid_t, const event::error_t, const string &)> ("clusterError", &server_t::errorCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
		this->_server->on <void (const pid_t, const event::status_t, const size_t)> ("clusterAvailable", &server_t::availableCluster, this, _1, _2, _3);
	// Если объект сервера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Конструктор
 *
 * @param server объект юнита сервера
 * @param coder  объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Server::Server(unit::server_t * server, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _dns(nullptr), _coder(coder), _server(server), _fmk(fmk), _log(log)  {
	// Если объект сервера установлен
	if(this->_server != nullptr){
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
		// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
		this->_server->on <void (const event::status_t)> ("status", &server_t::status, this, _1, state_t::SERVER);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_server->on <void (const event::id_t, const size_t)> ("write", &server_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
		this->_server->on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
		this->_server->on <void (const event::id_t, const event::status_t)> ("state", &server_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий сервера
		this->_server->on <void (const event::id_t, const event::action_t)> ("action", &server_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных сервером
		this->_server->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &server_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок сервера
		this->_server->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
		this->_server->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
		this->_server->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &server_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных сервера
		this->_server->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &server_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
		this->_server->on <void (const pid_t, const int32_t)> ("clusterExit", &server_t::exitCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
		this->_server->on <void (const pid_t, const pid_t)> ("clusterRebase", &server_t::rebaseCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
		this->_server->on <void (const pid_t, const size_t)> ("clusterSending", &server_t::sendingCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
		this->_server->on <void (const pid_t, const event::status_t)> ("clusterState", &server_t::stateCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
		this->_server->on <void (const pid_t, const unit::cluster_t::event_t)> ("clusterEvents", &server_t::eventsCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
		this->_server->on <void (const pid_t, const uint8_t *, const size_t)> ("clusterMessage", &server_t::messageCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
		this->_server->on <void (const pid_t, const event::error_t, const string &)> ("clusterError", &server_t::errorCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
		this->_server->on <void (const pid_t, const event::status_t, const size_t)> ("clusterAvailable", &server_t::availableCluster, this, _1, _2, _3);
	// Если объект клиента не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Конструктор
 *
 * @param server объект юнита сервера
 * @param dns    объект DNS-резолвера
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Server::Server(unit::server_t * server, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _dns(dns), _coder(nullptr), _server(server), _fmk(fmk), _log(log)  {
	// Если объект сервера установлен
	if(this->_server != nullptr){
		// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
		this->_server->on <void (const event::status_t)> ("status", &server_t::status, this, _1, state_t::SERVER);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_server->on <void (const event::id_t, const size_t)> ("write", &server_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
		this->_server->on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
		this->_server->on <void (const event::id_t, const event::status_t)> ("state", &server_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий сервера
		this->_server->on <void (const event::id_t, const event::action_t)> ("action", &server_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных сервером
		this->_server->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &server_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок сервера
		this->_server->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
		this->_server->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
		this->_server->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &server_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных сервера
		this->_server->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &server_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
		this->_server->on <void (const pid_t, const int32_t)> ("clusterExit", &server_t::exitCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
		this->_server->on <void (const pid_t, const pid_t)> ("clusterRebase", &server_t::rebaseCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
		this->_server->on <void (const pid_t, const size_t)> ("clusterSending", &server_t::sendingCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
		this->_server->on <void (const pid_t, const event::status_t)> ("clusterState", &server_t::stateCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
		this->_server->on <void (const pid_t, const unit::cluster_t::event_t)> ("clusterEvents", &server_t::eventsCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
		this->_server->on <void (const pid_t, const uint8_t *, const size_t)> ("clusterMessage", &server_t::messageCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
		this->_server->on <void (const pid_t, const event::error_t, const string &)> ("clusterError", &server_t::errorCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
		this->_server->on <void (const pid_t, const event::status_t, const size_t)> ("clusterAvailable", &server_t::availableCluster, this, _1, _2, _3);
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr){
			// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
			this->_dns->on <void (const event::status_t)> ("status", &server_t::status, this, _1, state_t::RESOLVER);
			// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
			this->_dns->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3);
			// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
			this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server_t::resolveDNS, this, _1, _2, _3, _4);
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
	// Если объект сервера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Конструктор
 *
 * @param server объект юнита сервера
 * @param dns    объект DNS-резолвера
 * @param coder  объект транспортного уровня безопасности
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::Server::Server(unit::server_t * server, unit::dns_t * dns, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _eid(0), _tid(0), _addr(fmk, log), _callback(fmk, log),
 _dns(dns), _coder(coder), _server(server), _fmk(fmk), _log(log)  {
	// Если объект сервера установлен
	if(this->_server != nullptr){
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
		// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
		this->_server->on <void (const event::status_t)> ("status", &server_t::status, this, _1, state_t::SERVER);
		// Устанавливаем функцию обратного вызова на событие записи данных!
		this->_server->on <void (const event::id_t, const size_t)> ("write", &server_t::write, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
		this->_server->on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
		this->_server->on <void (const event::id_t, const event::status_t)> ("state", &server_t::state, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие обработки действий сервера
		this->_server->on <void (const event::id_t, const event::action_t)> ("action", &server_t::action, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения данных сервером
		this->_server->on <void (const event::id_t, const uint8_t *, const size_t)> ("read", &server_t::read, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибок сервера
		this->_server->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
		this->_server->on <void (const event::id_t, const event::action_t, const uint32_t)> ("timeout", &server_t::timeout, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
		this->_server->on <void (const event::id_t, const event::status_t, const size_t)> ("available", &server_t::available, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие неотправленных данных сервера
		this->_server->on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)> ("spool", &server_t::spool, this, _1, _2, _3, _4);
		// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
		this->_server->on <void (const pid_t, const int32_t)> ("clusterExit", &server_t::exitCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
		this->_server->on <void (const pid_t, const pid_t)> ("clusterRebase", &server_t::rebaseCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
		this->_server->on <void (const pid_t, const size_t)> ("clusterSending", &server_t::sendingCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
		this->_server->on <void (const pid_t, const event::status_t)> ("clusterState", &server_t::stateCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
		this->_server->on <void (const pid_t, const unit::cluster_t::event_t)> ("clusterEvents", &server_t::eventsCluster, this, _1, _2);
		// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
		this->_server->on <void (const pid_t, const uint8_t *, const size_t)> ("clusterMessage", &server_t::messageCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
		this->_server->on <void (const pid_t, const event::error_t, const string &)> ("clusterError", &server_t::errorCluster, this, _1, _2, _3);
		// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
		this->_server->on <void (const pid_t, const event::status_t, const size_t)> ("clusterAvailable", &server_t::availableCluster, this, _1, _2, _3);
		// Если объект DNS-резолвера установлен
		if(this->_dns != nullptr){
			// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
			this->_dns->on <void (const event::status_t)> ("status", &server_t::status, this, _1, state_t::RESOLVER);
			// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
			this->_dns->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3);
			// Устанавливаем функции обратного вызова для обработки резолвинга доменного имени в сетевой адрес
			this->_dns->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server_t::resolveDNS, this, _1, _2, _3, _4);
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
	// Если объект сервера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("Server object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("Server object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * @brief Деструктор
 *
 */
awh::Server::~Server() noexcept {}
