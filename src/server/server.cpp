/**
 * @file: server.cpp
 * @date: 2026-05-17
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация фасада сервера — приём и обслуживание входящих подключений, сборка транспорта,
 *        TLS с несколькими сертификатами и DNS-резолвера,
 *        кластеризация и маршрутизация событий движка ввода-вывода в пользовательские функции обратного вызова
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <server/server.hpp>

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
awh::Server::Domain_Name_System::Domain_Name_System() noexcept :
 id(0), alive(15000), client(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::Server::Identifier::Identifier() noexcept : eid(0), cts(0) {}

/**
 * @brief Конструктор
 *
 */
awh::Server::TLS::TLS() noexcept : coder(nullptr) {}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Server::Unit::Unit(const fmk_t * fmk, const log_t * log) noexcept :
 addr(fmk, log), server(fmk, log), quic(fmk, log) {}

/**
 * @brief Метод проверки рабочего состояния сервера
 *
 * @return результат проверки рабочего состояния
 *
 */
bool awh::Server::active() const noexcept {
	// Если объект DNS-резолвера установлен - проверяем его рабочее состояние
	if(this->_dns.client != nullptr)
		// Выводим результат проверки рабочего состояния DNS-резолвера
		return this->_dns.client->working();
	// Если объект юнита сервера не создан
	if(this->_unit == nullptr)
		// Выводим отрицательный результат
		return false;
	/**
	 * Рабочее состояние проверяется на активном юните транспорта: для транспорта
	 * QUIC работает выделенный юнит, для остальных транспортов - общий юнит сервера
	 */
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.working() : this->_unit->server.working());
}
/**
 * @brief Метод фиксации настроек события сервера на активном юните транспорта
 *
 * @return результат выполнения фиксации
 *
 */
bool awh::Server::commitUnit() noexcept {
	/**
	 * Для систем, где ядро само разводит подключения между процессами кластера
	 */
	#if __AWH_CLUSTER_BALANCE__
		/**
		 * В дочернем процессе кластера обычного сервера перед привязкой пересоздаём
		 * унаследованный слушающий сокет: на Linux/FreeBSD SO_REUSEPORT требует
		 * собственного сокета на каждый процесс, иначе все процессы делят один
		 * унаследованный от мастера дескриптор и балансировки соединений ядром не
		 * происходит. Дескриптор пересоздаётся, а событие (его идентификатор, коллбэки,
		 * опции) сохраняется. Для транспорта QUIC собственный сокет дочернего процесса
		 * поднимается отдельным механизмом юнита QUIC, поэтому он здесь не затрагивается
		 */
		if((this->_protocol != event::protocol_t::QUIC) && (this->clusterMode() == event::mode_t::ENABLED) && (this->clusterFamily() == unit::cluster_t::family_t::CHILDREN))
			// Пересоздаём дескриптор слушающего события дочернего процесса кластера
			this->_unit->server.rebuild(this->_id.eid);
	#endif
	// Выполняем фиксацию настроек события на активном юните транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.commit(this->_id.eid) : this->_unit->server.commit(this->_id.eid));
}
/**
 * @brief Метод запуска работы события сервера на активном юните транспорта
 *
 * @return результат выполнения запуска
 *
 */
bool awh::Server::launchUnit() noexcept {
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.launch(this->_id.eid) : this->_unit->server.launch(this->_id.eid));
}
/**
 * @brief Метод прослушивания порта события сервера на активном юните транспорта
 *
 * @param max максимальный размер очереди ожидания соединений
 * @return    результат выполнения прослушивания
 *
 */
bool awh::Server::listenUnit(const uint32_t max) noexcept {
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.listen(this->_id.eid, max) : this->_unit->server.listen(this->_id.eid, max));
}
/**
 * @brief Метод запуска активного юнита транспорта
 *
 */
void awh::Server::startUnit() noexcept {
	// Запускаем активный юнит транспорта
	if(this->_protocol == event::protocol_t::QUIC)
		this->_unit->quic.start();
	else this->_unit->server.start();
}
/**
 * @brief Метод остановки активного юнита транспорта
 *
 */
void awh::Server::stopUnit() noexcept {
	// Останавливаем активный юнит транспорта
	if(this->_protocol == event::protocol_t::QUIC)
		this->_unit->quic.stop();
	else this->_unit->server.stop();
}
/**
 * @brief Метод получения семейства адресов события сервера на активном юните транспорта
 *
 * @return семейство адресов события сервера
 *
 */
awh::event::family_t awh::Server::familyUnit() const noexcept {
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.family(this->_id.eid) : this->_unit->server.family(this->_id.eid));
}
/**
 * @brief Метод получения статуса события сервера на активном юните транспорта
 *
 * @return статус события сервера
 *
 */
awh::event::status_t awh::Server::statusUnit() const noexcept {
	return ((this->_protocol == event::protocol_t::QUIC) ? awh_cast <const unit::unit_t *> (&this->_unit->quic)->status(this->_id.eid) : awh_cast <const unit::unit_t *> (&this->_unit->server)->status(this->_id.eid));
}
/**
 * @brief Метод получения адреса прослушивания события сервера на активном юните транспорта
 *
 * @param address тип адреса сервера
 * @return        значение адреса прослушивания события сервера
 *
 */
string awh::Server::getAddressUnit(const event::address_t address) const noexcept {
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.getAddress(this->_id.eid, address) : this->_unit->server.getAddress(this->_id.eid, address));
}
/**
 * @brief Метод получения порта прослушивания события сервера на активном юните транспорта
 *
 * @return порт прослушивания события сервера
 *
 */
uint16_t awh::Server::getPortUnit() const noexcept {
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.getPort(this->_id.eid) : this->_unit->server.getPort(this->_id.eid));
}
/**
 * @brief Метод установки адреса прослушивания события сервера на активном юните транспорта
 *
 * @param address тип адреса сервера
 * @param value   значение адреса прослушивания события сервера
 * @return        результат выполнения установки
 *
 */
bool awh::Server::setAddressUnit(const event::address_t address, string_view value) noexcept {
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.setAddress(this->_id.eid, address, value) : this->_unit->server.setAddress(this->_id.eid, address, value));
}

/**
 * @brief Метод изменения статуса сервера
 *
 * @param index  индекс обрабатываемого события
 * @param status новый статус сервера
 *
 */
void awh::Server::status(const uint8_t index, const event::status_t status) noexcept {
	/**
	 * Обрабатываем источник события
	 */
	switch(index){
		// Если мы получили статус события сервера
		case 0: {
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("status", status);
			// Если работа сервера запущена
			if(status == event::status_t::LAUNCHED){
				// Выполняем запуск работы сервера, если сервер не запущен
				if(!this->launchUnit()){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("This server ID=%u cannot be started", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (index), static_cast <uint16_t> (status)), log_t::flag_t::WARNING, this->_id.eid);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("This server ID=%u cannot be started", log_t::flag_t::WARNING, this->_id.eid);
						#endif
					}
				// Если сервер запущен удачно
				} else {
					/**
					 * Определяем семейство адресов, с которым работает сервер
					 */
					switch(static_cast <uint8_t> (this->familyUnit())){
						// Если сервер работает с адресами IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const string &, const uint16_t)> ("launch", this->getAddressUnit(event::address_t::IPV4), this->getPortUnit());
						break;
						// Если сервер работает с адресами IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const string &, const uint16_t)> ("launch", this->getAddressUnit(event::address_t::IPV6), this->getPortUnit());
						break;
					}
					// Если сервер запущен в режиме кластера
					if(this->clusterMode() == event::mode_t::ENABLED){
						// Если DNS-резолвер подключён
						if(this->_dns.client != nullptr){
							// Количество активных DNS-резолверов
							uint16_t count = 0;
							// Если количество активных DNS-резолверов для семейства адресов IPv4 больше нуля
							if((count = this->_dns.client->resolvers(event::family_t::IPV4)) > 0)
								// Выполняем инициализацию DNS-резолвера для текущего сервера
								this->_dns.client->init(event::family_t::IPV4, count);
							// Если количество активных DNS-резолверов для семейства адресов IPv6 больше нуля
							if((count = this->_dns.client->resolvers(event::family_t::IPV6)) > 0)
								// Выполняем инициализацию DNS-резолвера для текущего сервера
								this->_dns.client->init(event::family_t::IPV6, count);
						}
					}
				}
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
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
							// Устанавливаем адрес хоста текущей машины
							if(this->setAddressUnit(event::address_t::IPV4, this->_host)){
								// Если событие сервера не запущено, запускаем его
								if(this->statusUnit() == event::status_t::NONE){
									// Выполняем фиксацию параметров сервера
									if(this->commitUnit()){
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), event::family_t::IPV4, this->_host, this->getAddressUnit(event::address_t::IPV4));
										// Запускаем сервер
										this->startUnit();
									}
								}
								// Выходим из функции
								return;
							}
						} break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Устанавливаем адрес хоста текущей машины
							if(this->setAddressUnit(event::address_t::IPV6, this->_host)){
								// Если событие сервера не запущено, запускаем его
								if(this->statusUnit() == event::status_t::NONE){
									// Выполняем фиксацию параметров сервера
									if(this->commitUnit()){
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), event::family_t::IPV6, this->_host, this->getAddressUnit(event::address_t::IPV6));
										// Запускаем сервер
										this->startUnit();
									}
								}
								// Выходим из функции
								return;
							}
						} break;
					}
					// Выполняем разрешение имени хоста текущего сервера
					if(!this->_dns.client->resolve(this->_dns.id, this->familyUnit(), this->_host, this->_dns.alive.load(std::memory_order_acquire))){
						// Создаём текст ошибки разрешения хоста текущего сервера
						const string error = this->_fmk->format("It was not possible to obtain an IP address for the host \"%s\"", this->_host.c_str());
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
						} else this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", static_cast <event::id_t> (this->_id.eid), event::error_t::NOT_FOUND, error, nullptr);
					}
				} break;
				// Если событие DNS-резолвера остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED): {
					// Если сервер ещё живой
					if(this->_unit != nullptr){
						// Если идентификатор TLS и объект TLS установлены
						if((this->_id.cts > 0) && (this->_tls.coder != nullptr) && !this->_tls.safety.empty()){
							// Временный список идентификаторов TLS, которые нужно удалить
							vector <tls::coder_t::id_t> garbage;
							/**
							 * Проходим по всем сопоставлениям идентификаторов клиентов с идентификаторами TLS
							 */
							for(auto i = this->_tls.safety.begin(); i != this->_tls.safety.end();){
								// Формируем список идентификаторов TLS для удаления
								garbage.push_back(i->second);
								// Удаляем сопоставление идентификатора клиента с идентификатором TLS
								i = this->_tls.safety.erase(i);
							}
							// Если список идентификаторов TLS для удаления не пустой
							if(!garbage.empty()){
								/**
								 * Проходим по всем идентификаторам TLS для удаления
								 */
								for(const auto & id : garbage)
									// Уничтожаем объект TLS по найденному идентификатору TLS
									this->_tls.coder->destroy(id);
							}
						}
						// Останавливаем сервер
						this->stopUnit();
					}
				} break;
			}
		} break;
	}
}
/**
 * @brief Метод обработки события разрешения подключения
 *
 * @param eid идентификатор сервера
 * @param cid идентификатор клиента
 *
 */
void awh::Server::accept(const event::id_t eid, const event::id_t cid) noexcept {
	// Если объект транспортного уровня безопасности установлен
	if((this->_unit != nullptr) && (this->_tls.coder != nullptr) && (this->_id.cts > 0)){
		// Создаём идентификатор транспортного уровня TLS/DTLS
		tls::coder_t::id_t ctl = this->_tls.coder->transport(this->_id.cts);
		// Добавляем сопоставление идентификатора клиента с идентификатором TLS
		if(this->_tls.safety.emplace(cid, ctl).second){
			/**
			 * Определяем семейство адресов, с которым работает клиент
			 */
			switch(static_cast <uint8_t> (this->_unit->server.family(cid))){
				// Если клиент работает с адресами IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
					// Устанавливаем клиента TLS для события
					this->_tls.coder->peer(ctl, this->_unit->server.getAddress(cid, event::address_t::IPV4), this->_unit->server.getPort(cid));
				break;
				// Если клиент работает с адресами IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
					// Устанавливаем клиента TLS для события
					this->_tls.coder->peer(ctl, this->_unit->server.getAddress(cid, event::address_t::IPV6), this->_unit->server.getPort(cid));
				break;
			}
			// Регистрируем функцию обратного вызова на изменение состояния TLS
			this->_tls.coder->on(ctl, std::bind(&server_t::stateTLS, this, _1, cid, _2));
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_tls.coder->on(ctl, std::bind(&server_t::errorTLS, this, _1, cid, _2, _3));
			// Регистрируем функцию обратного вызова на получение снимка браузера отправившего ClientHello
			this->_tls.coder->on(ctl, std::bind(&server_t::fingerprintTLS, this, _1, cid, _2));
			// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
			this->_tls.coder->on(ctl, std::bind(&server_t::processTLS, this, _1, cid, _2, _3, _4, nullptr));
			// Если рукопожатие TLS не выполнено
			if(!this->_tls.coder->handshake(ctl)){
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error_tls")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("TLS handshake process was not completed", __PRETTY_FUNCTION__, make_tuple(eid, cid), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("TLS handshake process was not completed", log_t::flag_t::WARNING);
					#endif
				}
			// Если рукопожатие TLS выполнено успешно, выходим из функции
			} else return;
		}
		// Уничтожаем подключившегося клиента
		this->_unit->server.destroy(cid);
	// Если объект транспортного уровня безопасности не установлен, выполняем функцию обратного вызова
	} else this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", eid, cid, 0);
}
/**
 * @brief Метод обработки установленного соединения QUIC (RFC 9000)
 *
 * @param cid идентификатор сессии соединения
 *
 */
void awh::Server::opened(const event::id_t cid) noexcept {
	// Если сервер находится в рабочем состоянии
	if(this->active())
		/**
		 * Транслируем приложению принятие нового соединения: рукопожатие TLS 1.3
		 * ведёт само соединение QUIC, поэтому идентификатор TLS-контекста не выдаётся
		 */
		this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", static_cast <event::id_t> (this->_id.eid), cid, 0);
}
/**
 * @brief Метод обработки собранных данных потока соединения QUIC
 *
 * @param cid  идентификатор сессии соединения
 * @param sid  идентификатор потока приложения
 * @param data собранные данные потока
 * @param fin  флаг завершения потока удалённым эндпоинтом
 *
 */
void awh::Server::stream(const event::id_t cid, const uint64_t sid, const string & data, const bool fin) noexcept {
	// Если сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const uint64_t, const string &, const bool)> ("stream", cid, sid, data, fin);
}
/**
 * @brief Метод обработки освобождения буфера отправки потока соединения QUIC (сигнал writable)
 *
 * @param cid идентификатор сессии соединения
 * @param sid идентификатор потока приложения
 *
 */
void awh::Server::writable(const event::id_t cid, const uint64_t sid) noexcept {
	// Если сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова готовности потока принимать данные
		this->_callback.call <void (const event::id_t, const uint64_t)> ("writable", cid, sid);
}
/**
 * @brief Метод обработки принятой датаграммы приложения QUIC (RFC 9221)
 *
 * @param cid  идентификатор сессии соединения
 * @param data данные принятой датаграммы
 *
 */
void awh::Server::message(const event::id_t cid, const string & data) noexcept {
	// Если сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const string &)> ("datagram", cid, data);
}
/**
 * @brief Метод обработки завершения соединения QUIC (RFC 9000 §10)
 *
 * @param cid   идентификатор сессии соединения
 * @param error код ошибки завершения соединения
 *
 */
void awh::Server::closed(const event::id_t cid, const quic::error_t error) noexcept {
	// Если сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const quic::error_t)> ("disconnect", cid, error);
}
/**
 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
 *
 * @param eid  идентификатор события
 * @param info информационные метаданные о дейтаграммном пакете
 *
 */
void awh::Server::traffic(const event::id_t eid, const net::dgram_info_t & info) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const net::dgram_info_t &)> ("traffic", eid, info);
}
/**
 * @brief Метод обработки попыток подключения клиента к удалённому серверу
 *
 * @param domain   доменное имя для разрешения
 * @param attempts количество попыток подключения
 *
 */
void awh::Server::attempts(const unit::dns_t::id_t, const string & domain, const uint8_t attempts) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns.client->working())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const string &, const uint8_t)> ("attempts_dns", domain, attempts);
}
/**
 * @brief Метод обработки неудачного разрешения доменного имени
 *
 * @param id     идентификатор DNS-запроса
 * @param record тип записи DNS
 * @param domain доменное имя
 *
 */
void awh::Server::failure(const unit::dns_t::id_t id, const unit::dns_t::record_t record, const string & domain) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии
	if(this->_dns.client->working())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)> ("failure_dns", id, record, domain);
}
/**
 * @brief Метод разрешения доменного имени удалённого хоста в сетевой адрес
 *
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для разрешения
 * @param addr   указатель на структуру для хранения результата разрешения
 *
 */
void awh::Server::resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	// Если DNS-резолвер установлен и находится в рабочем состоянии
	if((this->_dns.client != nullptr) && this->_dns.client->working()){
		/**
		 * Определяем семейство адресов, с которым работает сервер
		 */
		switch(static_cast <uint8_t> (family)){
			// Если сервер работает с адресами IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Устанавливаем адрес хоста текущей машины
				if((this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setAddress(this->_id.eid, event::address_t::IPV4, addr) : this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV4, addr))){
					// Если событие сервера не запущено, запускаем его
					if(this->statusUnit() == event::status_t::NONE){
						// Выполняем фиксацию параметров сервера
						if(this->commitUnit()){
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), family, domain, this->getAddressUnit(event::address_t::IPV4));
							// Запускаем сервер
							this->startUnit();
						}
					}
				}
			} break;
			// Если сервер работает с адресами IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Устанавливаем адрес хоста текущей машины
				if((this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setAddress(this->_id.eid, event::address_t::IPV6, addr) : this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV6, addr))){
					// Если событие сервера не запущено, запускаем его
					if(this->statusUnit() == event::status_t::NONE){
						// Выполняем фиксацию параметров сервера
						if(this->commitUnit()){
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> ("ready", static_cast <event::id_t> (this->_id.eid), family, domain, this->getAddressUnit(event::address_t::IPV6));
							// Запускаем сервер
							this->startUnit();
						}
					}
				}
			} break;
		}
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid  идентификатор клиента
 * @param size размер данных для записи
 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::write(const event::id_t eid, const size_t size, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const size_t, void *)> ("write", eid, size, ctx);
}
/**
 * @brief Метод обработки событий изменения состояния сервера
 *
 * @param eid    идентификатор клиента
 * @param status новый статус сервера
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::state(const event::id_t eid, const event::status_t status, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active()){
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t, void *)> ("state", eid, status, ctx);
		// Если статус сервера изменился на "уничтожен"
		if(status == event::status_t::DESTROYED){
			// Если производится завершение работы текущего сервера
			if(eid == this->_id.eid){
				// Обнуляем идентификатор сервера
				this->_id.eid = 0;
				// Если идентификатор TLS и объект TLS установлены
				if((this->_unit != nullptr) && (this->_id.cts > 0) && (this->_tls.coder != nullptr) && !this->_tls.safety.empty()){
					// Временный список идентификаторов TLS, которые нужно удалить
					vector <tls::coder_t::id_t> garbage;
					/**
					 * Проходим по всем сопоставлениям идентификаторов клиентов с идентификаторами TLS
					 */
					for(auto i = this->_tls.safety.begin(); i != this->_tls.safety.end();){
						// Формируем список идентификаторов TLS для удаления
						garbage.push_back(i->second);
						// Удаляем сопоставление идентификатора клиента с идентификатором TLS
						i = this->_tls.safety.erase(i);
					}
					// Если список идентификаторов TLS для удаления не пустой
					if(!garbage.empty()){
						/**
						 * Проходим по всем идентификаторам TLS для удаления
						 */
						for(const auto & id : garbage)
							// Уничтожаем объект TLS по найденному идентификатору TLS
							this->_tls.coder->destroy(id);
					}
				}
			// Если производится завершение работы клиента подключенного к текущему серверу
			} else {
				// Если идентификатор TLS и объект TLS установлены
				if((this->_unit != nullptr) && (this->_id.cts > 0) && (this->_tls.coder != nullptr)){
					// Выполняем поиск идентификатора TLS по идентификатору события клиента
					auto i = this->_tls.safety.find(eid);
					// Если для данного идентификатора события клиента найден идентификатор TLS
					if(i != this->_tls.safety.end()){
						// Запоминаем идентификатор TLS для удаления
						const tls::coder_t::id_t ctx = i->second;
						// Удаляем сопоставление идентификатора клиента с идентификатором TLS
						this->_tls.safety.erase(i);
						// Уничтожаем объект TLS по найденному идентификатору TLS
						this->_tls.coder->destroy(ctx);
					}
				}
			}
		}
	}
}
/**
 * @brief Метод обработки действий сервера
 *
 * @param eid    идентификатор клиента
 * @param action действие сервера
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::action(const event::id_t eid, const event::action_t action, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::action_t, void *)> ("action", eid, action, ctx);
}
/**
 * @brief Метод обработки событий получения данных сервером
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных сервера
 * @param size   размер данных сервера
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::read(const event::id_t eid, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active()){
		// Если объект транспортного уровня безопасности установлен
		if((this->_unit != nullptr) && (this->_tls.coder != nullptr) && (this->_id.cts > 0)){
			// Выполняем поиск идентификатора TLS по идентификатору события клиента
			auto i = this->_tls.safety.find(eid);
			// Если для данного идентификатора события клиента найден идентификатор TLS
			if(i != this->_tls.safety.end()){
				// Если данные не расшифрованы
				if(!this->_tls.coder->decrypt(i->second, buffer, size)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error_tls")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("TLS data decryption failed", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size, ctx), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("TLS data decryption failed", log_t::flag_t::WARNING);
						#endif
					}
				}
				// Выходим из функции
				return;
			}
		}
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", eid, buffer, size, ctx);
	}
}
/**
 * @brief Метод обработки события ошибки
 *
 * @param eid     идентификатор события
 * @param error   код ошибки
 * @param message сообщение об ошибке
 * @param ctx     промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::error(const event::id_t eid, const event::error_t error, const string & message, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &, void *)> ("error", eid, error, message, ctx);
}
/**
 * @brief Метод обработки событий доступности/недоступности очереди исходящих данных клиента
 *
 * @param eid    идентификатор клиента
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::available(const event::id_t eid, const event::status_t status, const size_t size, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t, const size_t, void *)> ("available", eid, status, size, ctx);
}
/**
 * @brief Метод обработки событий истечения таймаута клиента
 *
 * @param eid    идентификатор клиента
 * @param action тип действия для истёкшего таймаута
 * @param delay  задержка таймаута в миллисекундах
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 * @return       нужно ли завершить клиента после истечения таймаута
 *
 */
bool awh::Server::timeout(const event::id_t eid, const event::action_t action, const uint32_t delay, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active()){
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("timeout");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid))
			// Выполняем функцию обратного вызова
			return this->_callback.call <bool (const event::id_t, const event::action_t, const uint32_t, void *)> (fid, eid, action, delay, ctx);
	}
	// Возвращаем значение, указывающее на то, что клиента нужно завершить после истечения таймаута
	return true;
}
/**
 * @brief Метод обработки события невозможности отправки данных клиенту
 *
 * @param eid    идентификатор клиента
 * @param error  тип ошибки отправки данных
 * @param buffer данные, которые не получилось отправить
 * @param size   размер данных, которые не получилось отправить
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::spool(const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t, void *)> ("spool", eid, error, buffer, size, ctx);
}
/**
 * @brief Метод обработки события пересоздания процесса
 *
 * @param old старый идентификатор процесса
 * @param pid текущий идентификатор процесса
 *
 */
void awh::Server::rebaseCluster(const pid_t old, const pid_t pid) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const pid_t)> ("cluster_rebase", old, pid);
}
/**
 * @brief Метод получения события завершения работы процесса
 *
 * @param pid    идентификатор процесса
 * @param signal сигнал, с которым завершился процесс
 *
 */
void awh::Server::exitCluster(const pid_t pid, const int32_t signal) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const int32_t)> ("cluster_exit", pid, signal);
}
/**
 * @brief Метод обработки события отправки сообщения процессу кластера
 *
 * @param pid  идентификатор процесса
 * @param size размер отправленного сообщения
 *
 */
void awh::Server::sendingCluster(const pid_t pid, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const size_t)> ("cluster_sending", pid, size);
}
/**
 * @brief Метод обработки событий изменения статуса кластера
 *
 * @param pid    идентификатор события
 * @param status новый статус кластера
 *
 */
void awh::Server::stateCluster(const pid_t pid, const event::status_t status) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t)> ("cluster_state", pid, status);
}
/**
 * @brief Метод обработки событий активации/деактивации кластера
 *
 * @param pid   идентификатор процесса
 * @param event флаг события кластера
 *
 */
void awh::Server::eventsCluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const unit::cluster_t::event_t)>  ("cluster_events", pid, event);
}
/**
 * @brief Метод обработки события получения сообщения от процесса кластера
 *
 * @param pid  идентификатор процесса
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 *
 */
void awh::Server::messageCluster(const pid_t pid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", pid, data, size);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
 *
 * @param pid    идентификатор процесса
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 *
 */
void awh::Server::availableCluster(const pid_t pid, const event::status_t status, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", pid, status, size);
}
/**
 * @brief Метод обработки событий ошибок кластера
 *
 * @param pid         идентификатор процесса
 * @param error       тип ошибки
 * @param description описание ошибки
 *
 */
void awh::Server::errorCluster(const pid_t pid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("cluster_error", pid, error, description);
}
/**
 * @brief Метод получения состояния TLS
 *
 * @param id    идентификатор TLS
 * @param eid   идентификатор клиента
 * @param state состояние TLS
 *
 */
void awh::Server::stateTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active()){
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const tls::coder_t::id_t, const event::id_t, const tls::coder_t::state_t)> ("state_tls", id, eid, state);
		/**
		 * Обрабатываем входящие состояния DTLS
		 */
		switch(static_cast <uint8_t> (state)){
			// Если состояние ошибки транспортного уровня
			case static_cast <uint8_t> (tls::coder_t::state_t::FAILED): {
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error_tls")){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("TLS failed", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (state)), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("TLS failed", log_t::flag_t::WARNING);
					#endif
				}
				// Уничтожаем подключившегося клиента
				this->_unit->server.destroy(eid);
			} break;
			// Если состояние уничтожения объекта транспортного уровня
			case static_cast <uint8_t> (tls::coder_t::state_t::DESTROYED): {
				// Если сервер ещё живой
				if(this->_unit != nullptr){
					// Если список сопоставлений идентификаторов клиентов с идентификаторами TLS не пустой
					if(!this->_tls.safety.empty()){
						// Выполняем поиск идентификатора TLS по идентификатору события клиента
						auto i = this->_tls.safety.find(eid);
						// Если для данного идентификатора события клиента найден идентификатор TLS
						if(i != this->_tls.safety.end())
							// Удаляем сопоставление идентификатора клиента с идентификатором TLS
							this->_tls.safety.erase(i);
					}
					// Уничтожаем подключившегося клиента
					this->_unit->server.destroy(eid);
				}
			} break;
			// Если состояние рукопожатия успешно завершено
			case static_cast <uint8_t> (tls::coder_t::state_t::HANDSHAKED):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", static_cast <event::id_t> (this->_id.eid), eid, id);
			break;
		}
	}
}
/**
 * @brief Метод получения отпечатка TLS
 *
 * @param id      идентификатор TLS
 * @param eid     идентификатор клиента
 * @param browser информация о браузере для отпечатка TLS
 *
 */
void awh::Server::fingerprintTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::fgp_t::browser_t & browser) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active())
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const tls::coder_t::id_t, const event::id_t, const tls::fgp_t::browser_t &)> ("fingerprint_tls", id, eid, browser);
}
/**
 * @brief Метод обработки ошибок TLS
 *
 * @param id      идентификатор TLS
 * @param eid     идентификатор клиента
 * @param error   код ошибки TLS
 * @param message сообщение об ошибке TLS
 *
 */
void awh::Server::errorTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active()){
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("error_tls")){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (error), message), log_t::flag_t::CRITICAL, message.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, message.c_str());
			#endif
		// Выполняем функцию обратного вызова
		} else this->_callback.call <void (const tls::coder_t::id_t, const event::id_t, const tls::coder_t::error_t, const string &)> ("error_tls", id, eid, error, message);
	}
}
/**
 * @brief Метод обработки событий шифрования/дешифрования данных TLS
 *
 * @param id     идентификатор TLS
 * @param eid    идентификатор клиента
 * @param event  тип события TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
 *
 */
void awh::Server::processTLS(const tls::coder_t::id_t id, const event::id_t eid, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или сервер находится в рабочем состоянии
	if(this->active()){
		/**
		 * Обрабатываем тип события TLS
		 */
		switch(static_cast <uint8_t> (event)){
			// Если событие шифрования данных TLS
			case static_cast <uint8_t> (tls::coder_t::event_t::ENCRYPTION): {
				// Отправляем данные обратно клиенту, которые были зашифрованы TLS
				if(!this->_unit->server.send(eid, reinterpret_cast <const char *> (buffer), size)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error")){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("Encrypted data cannot be sent to the client", __PRETTY_FUNCTION__, make_tuple(id, eid, static_cast <uint16_t> (event), buffer, size, ctx), log_t::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Encrypted data cannot be sent to the client", log_t::flag_t::WARNING);
						#endif
					}
				}
			} break;
			// Если событие дешифрования данных TLS
			case static_cast <uint8_t> (tls::coder_t::event_t::DECRYPTION):
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", eid, buffer, size, ctx);
			break;
		}
	}
}
/**
 * @brief Метод очистки чёрного списка события
 *
 * @param eid идентификатор события
 * @return    результат выполнения очистки
 *
 */
bool awh::Server::clearBlacklist(const event::id_t eid) noexcept {
	// Выполняем очистку чёрного списка события
	return this->_unit->server.clearBlacklist(eid);
}
/**
 * @brief Метод очистки белого списка события
 *
 * @param eid идентификатор события
 * @return    результат выполнения очистки
 *
 */
bool awh::Server::clearWhitelist(const event::id_t eid) noexcept {
	// Выполняем очистку белого списка события
	return this->_unit->server.clearWhitelist(eid);
}
/**
 * @brief Метод добавления адреса в чёрный список события
 *
 * @param eid   идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 *
 */
bool awh::Server::addToBlacklist(const event::id_t eid, string_view value) noexcept {
	// Выполняем добавление адреса в чёрный список события
	return this->_unit->server.addToBlacklist(eid, value);
}
/**
 * @brief Метод добавления адреса в белый список события
 *
 * @param eid   идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 *
 */
bool awh::Server::addToWhitelist(const event::id_t eid, string_view value) noexcept {
	// Выполняем добавление адреса в белый список события
	return this->_unit->server.addToWhitelist(eid, value);
}
/**
 * @brief Метод удаления адреса из чёрного списка события
 *
 * @param eid   идентификатор события
 * @param value адрес для удаления из чёрного списка
 * @return      результат выполнения удаления
 *
 */
bool awh::Server::removeFromBlacklist(const event::id_t eid, string_view value) noexcept {
	// Выполняем удаление адреса из чёрного списка события
	return this->_unit->server.removeFromBlacklist(eid, value);
}
/**
 * @brief Метод удаления адреса из белого списка события
 *
 * @param eid   идентификатор события
 * @param value адрес для удаления из белого списка
 * @return      результат выполнения удаления
 *
 */
bool awh::Server::removeFromWhitelist(const event::id_t eid, string_view value) noexcept {
	// Выполняем удаление адреса из белого списка события
	return this->_unit->server.removeFromWhitelist(eid, value);
}
/**
 * @brief Метод получения чёрного списка события
 *
 * @param eid идентификатор события
 * @return    чёрный список события
 *
 */
const unordered_map <string, awh::event::address_t> & awh::Server::getFromBlacklist(const event::id_t eid) const noexcept {
	// Выполняем получение чёрного списка события
	return this->_unit->server.getFromBlacklist(eid);
}
/**
 * @brief Метод получения белого списка события
 *
 * @param eid идентификатор события
 * @return    белый список события
 *
 */
const unordered_map <string, awh::event::address_t> & awh::Server::getFromWhitelist(const event::id_t eid) const noexcept {
	// Выполняем получение белого списка события
	return this->_unit->server.getFromWhitelist(eid);
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::Server::stop() noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Останавливаем событие DNS-резолвера
				this->_dns.client->stop();
			// Если объект DNS-резолвера не установлен
			else {
				// Если сервер ещё живой
				if(this->_unit != nullptr){
					// Если идентификатор TLS и объект TLS установлены
					if((this->_id.cts > 0) && (this->_tls.coder != nullptr) && !this->_tls.safety.empty()){
						// Временный список идентификаторов TLS, которые нужно удалить
						vector <tls::coder_t::id_t> garbage;
						/**
						 * Проходим по всем сопоставлениям идентификаторов клиентов с идентификаторами TLS
						 */
						for(auto i = this->_tls.safety.begin(); i != this->_tls.safety.end();){
							// Формируем список идентификаторов TLS для удаления
							garbage.push_back(i->second);
							// Удаляем сопоставление идентификатора клиента с идентификатором TLS
							i = this->_tls.safety.erase(i);
						}
						// Если список идентификаторов TLS для удаления не пустой
						if(!garbage.empty()){
							/**
							 * Проходим по всем идентификаторам TLS для удаления
							 */
							for(const auto & id : garbage)
								// Уничтожаем объект TLS по найденному идентификатору TLS
								this->_tls.coder->destroy(id);
						}
					}
					// Останавливаем событие сервера
					this->stopUnit();
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::Server::start() noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Если объект DNS-резолвера установлен
			if(this->_dns.client != nullptr)
				// Запускаем событие DNS-резолвера
				this->_dns.client->start();
			// Если объект DNS-резолвера не установлен
			else {
				// Если событие сервера не запущено, запускаем его
				if(this->statusUnit() == event::status_t::NONE){
					// Выполняем фиксацию параметров сервера
					if(this->commitUnit()){
						// Выполняем получение идентификатора функции обратного вызова
						const callback_t::id_t fid = this->_callback.id("ready");
						// Если функция обратного вызова установлена
						if(this->_callback.is(fid)){
							// Хост текущего сервера
							string host = "";
							/**
							 * Определяем семейство адресов, с которым работает сервер
							 */
							switch(static_cast <uint8_t> (this->familyUnit())){
								// Если сервер работает с адресами Unix Domain Socket
								case static_cast <uint8_t> (event::family_t::UDS):
									// Извлекаем адрес хоста текущей машины для адресов Unix Domain Socket
									host = ::move(this->_unit->server.getAddress(this->_id.eid, event::address_t::UDS));
								break;
								// Если сервер работает с адресами IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
									// Извлекаем адрес хоста текущей машины для адресов IPv4
									host = ::move(this->getAddressUnit(event::address_t::IPV4));
								break;
								// Если сервер работает с адресами IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
									// Извлекаем адрес хоста текущей машины для адресов IPv6
									host = ::move(this->getAddressUnit(event::address_t::IPV6));
								break;
							}
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::family_t, const string &, const string &)> (fid, static_cast <event::id_t> (this->_id.eid), this->familyUnit(), host, host);
						}
						// Запускаем сервер
						this->startUnit();
					}
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения приостановки работы
 *
 */
bool awh::Server::pause(const event::id_t eid) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active()){
		// Если идентификатор клиента найден в списке обслуживаемых клиентов
		if((eid != this->_id.eid) && (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
			// Приостанавливаем событие клиента
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.pause(eid) : this->_unit->server.pause(eid));
		// Если идентификатор клиента не найден в списке обслуживаемых клиентов
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 *
 */
bool awh::Server::resume(const event::id_t eid) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active()){
		// Если идентификатор клиента найден в списке обслуживаемых клиентов
		if((eid != this->_id.eid) && (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
			// Возобновляем событие клиента
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.resume(eid) : this->_unit->server.resume(eid));
		// Если идентификатор клиента не найден в списке обслуживаемых клиентов
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод уничтожения события клиента или сервера
 *
 * @param eid идентификатор события клиента для уничтожения
 *
 */
void awh::Server::destroy(const event::id_t eid) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active()){
		// Если идентификатор сервера установлен
		if(eid == this->_id.eid)
			// Уничтожаем событие сервера
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.destroy(this->_id.eid) : this->_unit->server.destroy(this->_id.eid));
		// Если идентификатор клиента найден в списке обслуживаемых клиентов
		else if((this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid))) {
			// Если сервер ещё живой
			if(this->_unit != nullptr){
				// Если идентификатор TLS и объект TLS установлены
				if((this->_id.cts > 0) && (this->_tls.coder != nullptr)){
					// Выполняем поиск идентификатора TLS по идентификатору события клиента
					auto i = this->_tls.safety.find(eid);
					// Если для данного идентификатора события клиента найден идентификатор TLS
					if(i != this->_tls.safety.end()){
						// Запоминаем идентификатор TLS для удаления
						const tls::coder_t::id_t ctx = i->second;
						// Удаляем сопоставление идентификатора клиента с идентификатором TLS
						this->_tls.safety.erase(i);
						// Уничтожаем объект TLS по найденному идентификатору TLS
						this->_tls.coder->destroy(ctx);
					}
				}
				// Уничтожаем событие клиента
				(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.destroy(eid) : this->_unit->server.destroy(eid));
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
}
/**
 * @brief Метод проверки, жив ли клиент или сервер
 *
 * @param eid идентификатор события клиента для проверки
 * @return    результат проверки
 *
 */
bool awh::Server::isAlive(const event::id_t eid) const noexcept {
	// Переменная результата
	bool result = false;
	// Если идентификатор события не соответствует текущему серверу
	if(!(result = (eid == this->_id.eid)))
		// Проверяем, жив ли клиент по идентификатору события
		result = (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid));
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки промежуточного контекста события подключённого клиента
 *
 * @param eid идентификатор события сервера
 * @param ctx указатель на контекст события
 * @return    результат выполнения установки
 *
 */
bool awh::Server::setContext(const event::id_t eid, void * ctx) noexcept {
	// Устанавливаем промежуточный контекст события подключённого клиента
	const bool result = (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setContext(eid, ctx) : this->_unit->server.setContext(eid, ctx));
	// Если промежуточный контекст события подключённого клиента успешно установлен
	if(result){
		// Выполняем поиск идентификатора TLS по идентификатору события клиента
		auto i = this->_tls.safety.find(eid);
		// Если для данного идентификатора события клиента найден идентификатор TLS
		if((this->_tls.coder != nullptr) && (i != this->_tls.safety.end()))
			// Устанавливаем функцию обратного вызова на событие шифрования/дешифрования данных TLS
			this->_tls.coder->on(i->second, std::bind(&server_t::processTLS, this, _1, eid, _2, _3, _4, ctx));
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод перевода события в режим прослушивания входящих соединений
 *
 * @param max максимальное количество входящих соединений
 * @return    результат выполнения перевода в режим прослушивания
 *
 */
bool awh::Server::listen(const uint16_t max) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0)
			// Переводим событие сервера в режим прослушивания входящих соединений
			return this->listenUnit(max);
		// Если идентификатор сервера не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(max), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
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
 *
 */
void awh::Server::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на событие получения данных от клиента
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при отправке данных клиенту
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова на событие готовности сервера к работе
	this->_callback.set("ready", callback);
	// Выполняем установку функции обратного вызова при изменении состояния сервера
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова на событие невозможности отправки данных клиенту
	this->_callback.set("spool", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова на событие изменения статуса сервера
	this->_callback.set("status", callback);
	// Выполняем установку функции обратного вызова на событие изменения состояния сервера
	this->_callback.set("action", callback);
	// Выполняем установку функции обратного вызова на событие запуска сервера
	this->_callback.set("launch", callback);
	// Выполняем установку функции обратного вызова при принятии входящего соединения от клиента
	this->_callback.set("accept", callback);
	// Выполняем установку функции обратного вызова при получении информационных метаданных о дейтаграммном пакете
	this->_callback.set("traffic", callback);
	// Выполняем установку функции обратного вызова на событие истечения таймаута клиента
	this->_callback.set("timeout", callback);
	// Выполняем установку функции обратного вызова на событие доступности/недоступности очереди исходящих данных клиента
	this->_callback.set("available", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок TLS
	this->_callback.set("error_tls", callback);
	// Выполняем установку функции обратного вызова на событие получения состояния TLS
	this->_callback.set("state_tls", callback);
	// Выполняем установку функции обратного вызова на событие получения отпечатка ClientHello TLS
	this->_callback.set("fingerprint_tls", callback);
	// Выполняем установку функции обратного вызова на событие неудачного разрешения доменного имени DNS-резолвером
	this->_callback.set("failure_dns", callback);
	// Выполняем установку функции обратного вызова на событие завершения попыток разрешения доменного имени DNS-резолвером
	this->_callback.set("attempts_dns", callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса кластера
	this->_callback.set("cluster_exit", callback);
	// Выполняем установку функции обратного вызова при получении ошибок кластера
	this->_callback.set("cluster_error", callback);
	// Выполняем установку функции обратного вызова при получении состояния процесса кластера
	this->_callback.set("cluster_state", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса кластера
	this->_callback.set("cluster_rebase", callback);
	// Выполняем установку функции обратного вызова при запуске/остановке процесса кластера
	this->_callback.set("cluster_events", callback);
	// Выполняем установку функции обратного вызова при отправке сообщения кластера
	this->_callback.set("cluster_sending", callback);
	// Выполняем установку функции обратного вызова при получении сообщения кластера
	this->_callback.set("cluster_message", callback);
	// Выполняем установку функции обратного вызова при получении доступности размера очереди сообщений кластера
	this->_callback.set("cluster_available", callback);
}
/**
 * @brief Метод получения данных от клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 *
 */
bool awh::Server::recv(const event::id_t eid) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active()){
		// Если идентификатор клиента найден в списке обслуживаемых клиентов
		if((eid != this->_id.eid) && (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
			// Получаем данные от клиента
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.recv(eid) : this->_unit->server.recv(eid));
		// Если идентификатор клиента не найден в списке обслуживаемых клиентов
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных клиенту
 *
 * @param eid    идентификатор события клиента
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных клиенту
 *
 */
size_t awh::Server::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active()){
		// Для транспорта QUIC отправляем данные в сессию соединения потоком по умолчанию
		if(this->_protocol == event::protocol_t::QUIC)
			// Отправляем данные в сессию соединения QUIC
			return this->_unit->quic.send(eid, buffer, size);
		// Если идентификатор клиента найден в списке обслуживаемых клиентов
		if((this->_unit != nullptr) && (eid != this->_id.eid) && this->_unit->server.isActual(eid)){
			// Если идентификатор TLS и объект TLS установлены
			if((this->_id.cts > 0) && (this->_tls.coder != nullptr)){
				// Выполняем поиск идентификатора TLS по идентификатору события клиента
				auto i = this->_tls.safety.find(eid);
				// Если для данного идентификатора события клиента найден идентификатор TLS
				if(i != this->_tls.safety.end()){
					// Если шифрование данных TLS выполнено успешно
					if(this->_tls.coder->encrypt(i->second, buffer, size))
						// Возвращаем размер отправленных данных
						return size;
				}
				// Возвращаем значение по умолчанию
				return 0;
			}
			// Отправляем данные клиенту
			return this->_unit->server.send(eid, buffer, size);
		// Если идентификатор клиента не найден в списке обслуживаемых клиентов
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(eid, buffer, size), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод открытия потока приложения соединения QUIC
 *
 * @param cid  идентификатор сессии соединения
 * @param mode режим однонаправленного потока
 * @return     идентификатор открытого потока
 *
 */
uint64_t awh::Server::open(const event::id_t cid, const bool mode) noexcept {
	// Если сервер находится в рабочем состоянии и транспорт является QUIC
	if(this->active() && (this->_protocol == event::protocol_t::QUIC))
		// Выводим идентификатор открытого потока приложения сессии соединения
		return this->_unit->quic.open(cid, mode);
	// Для транспортов без мультиплексирования соединение представляет собой единственный поток
	return 0;
}
/**
 * @brief Метод отправки данных в поток приложения соединения QUIC
 *
 * @param cid    идентификатор сессии соединения
 * @param sid    идентификатор потока приложения
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @param fin    флаг завершения потока
 * @return       количество байт данных, поставленных в очередь отправки
 *
 */
size_t awh::Server::send(const event::id_t cid, const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
	// Если сервер находится в рабочем состоянии
	if(this->active()){
		// Для транспорта QUIC отправляем данные в указанный поток сессии соединения
		if(this->_protocol == event::protocol_t::QUIC){
			// Если постановка данных потока в очередь отправки выполнена
			// Возвращаем число поставленных в очередь данных (частичный приём)
			return this->_unit->quic.send(cid, sid, string_view(reinterpret_cast <const char *> (buffer), size), fin);
		}
		// Для транспортов без мультиплексирования идентификатор потока не используется
		return this->send(cid, buffer, size);
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки водяных меток буфера отправки потоков сессии соединения QUIC (backpressure)
 *
 * @param cid  идентификатор сессии соединения
 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
 * @param low  нижняя водяная метка (порог сигнала "writable")
 *
 */
void awh::Server::sendWaterMarks(const event::id_t cid, const size_t high, const size_t low) noexcept {
	// Если сервер в рабочем состоянии и транспорт является QUIC
	if(this->active() && (this->_protocol == event::protocol_t::QUIC))
		// Устанавливаем водяные метки буфера отправки потоков сессии соединения
		this->_unit->quic.sendWaterMarks(cid, high, low);
}
/**
 * @brief Метод назначения pull-источника данных потока сессии соединения QUIC (RFC 9000 §2.2)
 *
 * @param cid    идентификатор сессии соединения
 * @param sid    идентификатор потока приложения
 * @param source pull-источник данных тела потока
 *
 */
void awh::Server::dataSource(const event::id_t cid, const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept {
	// Если сервер в рабочем состоянии и транспорт является QUIC
	if(this->active() && (this->_protocol == event::protocol_t::QUIC))
		// Назначаем pull-источник данных тела потока сессии соединения
		this->_unit->quic.dataSource(cid, sid, source);
}
/**
 * @brief Метод отправки датаграммы приложения соединению QUIC (RFC 9221)
 *
 * @param cid    идентификатор сессии соединения
 * @param buffer буфер данных датаграммы для отправки
 * @param size   размер данных датаграммы для отправки
 * @return       результат отправки
 *
 */
bool awh::Server::datagram(const event::id_t cid, const void * buffer, const size_t size) noexcept {
	// Если сервер находится в рабочем состоянии и транспорт является QUIC
	if(this->active() && (this->_protocol == event::protocol_t::QUIC))
		// Выполняем отправку датаграммы приложения соединению
		return this->_unit->quic.datagram(cid, string_view(reinterpret_cast <const char *> (buffer), size));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения предельного размера отправляемой датаграммы QUIC (RFC 9221 §3)
 *
 * @param cid идентификатор сессии соединения
 * @return    предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
 *
 */
size_t awh::Server::datagrams(const event::id_t cid) const noexcept {
	// Если сервер находится в рабочем состоянии и транспорт является QUIC
	if(this->active() && (this->_protocol == event::protocol_t::QUIC))
		// Выводим предельный размер данных отправляемой датаграммы сессии соединения
		return this->_unit->quic.datagrams(cid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод завершения соединения QUIC приложением (RFC 9000 §10.2)
 *
 * @param cid    идентификатор сессии соединения
 * @param code   код ошибки приложения
 * @param reason человекочитаемая причина завершения
 *
 */
void awh::Server::close(const event::id_t cid, const uint64_t code, string_view reason) noexcept {
	// Если сервер находится в рабочем состоянии и транспорт является QUIC
	if(this->active() && (this->_protocol == event::protocol_t::QUIC))
		// Выполняем завершение соединения приложением
		this->_unit->quic.close(cid, code, reason);
}
/**
 * @brief Метод установки локальных транспортных параметров соединений QUIC (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 *
 */
void awh::Server::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры соединений QUIC
	this->_unit->quic.params(params);
}
/**
 * @brief Метод установки проверки адреса клиента через пакет Retry QUIC (RFC 9000 §8.1.2)
 *
 * @param mode режим проверки адреса клиента
 *
 */
void awh::Server::retry(const bool mode) noexcept {
	// Устанавливаем режим проверки адреса клиента через пакет Retry QUIC
	this->_unit->quic.retry(mode);
}
/**
 * @brief Метод установки уведомления о перегрузке пути QUIC (RFC 9000 §13.4)
 *
 * @param mode режим уведомления о перегрузке пути
 *
 */
void awh::Server::ecn(const bool mode) noexcept {
	// Устанавливаем режим уведомления о перегрузке пути QUIC
	this->_unit->quic.ecn(mode);
}
/**
 * @brief Метод объединения данных между сервером и другим событием
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения объединения
 *
 */
bool awh::Server::splice(const event::id_t eid, const event::id_t dest) noexcept {
	// Если объединяемые события не принадлежат текущему серверу
	if((eid != this->_id.eid) && (dest != this->_id.eid)){
		// Если один из идентификаторов события принадлежит к пирам текущего сервера
		if((this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)) ||
		   (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(dest) : this->_unit->server.isActual(dest)))
			// Объединяем данные между событиями на активном юните транспорта
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.splice(eid, dest) : this->_unit->server.splice(eid, dest));
	// Если идентификатор клиента не найден в списке обслуживаемых клиентов
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Client ID is not found", __PRETTY_FUNCTION__, make_tuple(eid, dest), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Client ID is not found", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения опций сервера или клиента
 *
 * @param eid идентификатор события сервера или клиента
 * @return    опции сервера или клиента
 *
 */
uint16_t awh::Server::getOptions(const event::id_t eid) const noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Получаем опции сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getOptions(eid) : this->_unit->server.getOptions(eid));
	// Если идентификатор сервера или клиента не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций сервера или клиента
 *
 * @param eid     идентификатор события сервера или клиента
 * @param options опции сервера или клиента для установки
 * @return        результат выполнения установки
 *
 */
bool awh::Server::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Устанавливаем опции сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setOptions(eid, options) : this->_unit->server.setOptions(eid, options));
	// Если идентификатор сервера или клиента не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, options), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции сервера или клиента
 *
 * @param eid    идентификатор события сервера или клиента
 * @param option опция сервера или клиента для установки
 * @param mode   режим установки опции сервера или клиента
 * @return       результат выполнения установки
 *
 */
bool awh::Server::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Устанавливаем опцию сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setOption(eid, option, mode) : this->_unit->server.setOption(eid, option, mode));
	// Если идентификатор сервера или клиента не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, option, mode), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
 *
 * @return метаданные последнего принятого дейтаграммного пакета
 *
 */
awh::net::dgram_info_t awh::Server::getTrafficInfo() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Получаем метаданные последнего принятого дейтаграммного пакета
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getTrafficInfo(this->_id.eid) : this->_unit->server.getTrafficInfo(this->_id.eid));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return net::dgram_info_t();
}
/**
 * @brief Метод получения количества хопов последнего принятого пакета
 *
 * @return количество хопов последнего принятого пакета
 *
 */
uint8_t awh::Server::getCountHops() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Получаем количество хопов последнего принятого пакета
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getCountHops(this->_id.eid) : this->_unit->server.getCountHops(this->_id.eid));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
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
 *
 */
bool awh::Server::setCountHops(const uint8_t hops) noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем количество хопов последнего принятого пакета
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setCountHops(this->_id.eid, hops) : this->_unit->server.setCountHops(this->_id.eid, hops));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (hops)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param eid идентификатор события сервера
 * @return    максимальное количество хопов
 *
 */
awh::event::hops_t awh::Server::getHops(const event::id_t eid) const noexcept {
	// Если идентификатор сервера или клиента установлен
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Получаем максимальное количество хопов
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getHops(eid) : this->_unit->server.getHops(eid));
	// Если идентификатор сервера или клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события сервера
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 *
 */
bool awh::Server::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера или клиента установлен
		if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
			// Устанавливаем максимальное количество хопов
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setHops(eid, hops) : this->_unit->server.setHops(eid, hops));
		// Если идентификатор сервера или клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (hops)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @return сетевой интерфейс сервера
 *
 */
string awh::Server::getIface() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем сетевой интерфейс сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getIface(this->_id.eid) : this->_unit->server.getIface(this->_id.eid));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем результат
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 */
bool awh::Server::setIface(string_view name) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0)
			// Устанавливаем сетевой интерфейс сервера
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setIface(this->_id.eid, name) : this->_unit->server.setIface(this->_id.eid, name));
		// Если идентификатор сервера не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(name), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта сервера
 *
 * @return порт сервера
 *
 */
uint16_t awh::Server::getPort() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем порт текущего сервера
		return this->getPortUnit();
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта сервера
 *
 * @param port порт сервера для установки
 * @return     результат выполнения установки
 *
 */
bool awh::Server::setPort(const uint16_t port) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Если включён режим кластера
			if(this->clusterMode() == event::mode_t::ENABLED){
				// Если транспорт является QUIC, выполняем установку диапазона портов кластера
				if(this->_protocol == event::protocol_t::QUIC){
					// Устанавливаем диапазон портов кластера
					this->clusterRange(port, port);
					// Возвращаем результат выполнения установки
					return true;
				}
				// Устанавливаем порт текущего сервера
				return this->_unit->server.setPort(this->_id.eid, port);
			}
			// Устанавливаем порт текущего сервера
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setPort(this->_id.eid, port) : this->_unit->server.setPort(this->_id.eid, port));
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта подключённого клиента
 *
 * @param eid идентификатор события клиента
 * @return    порт подключённого клиента
 *
 */
uint16_t awh::Server::getPort(const event::id_t eid) const noexcept {
	// Если идентификатор подключённого клиента является активным
	if((this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Извлекаем порт подключённого клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getPort(eid) : this->_unit->server.getPort(eid));
	// Если идентификатор подключённого клиента не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or client is not initialized", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or client is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения адреса хоста текущей машины
 *
 * @return адрес хоста текущей машины
 *
 */
const string & awh::Server::getHost() const noexcept {
	// Возвращаем адрес хоста текущей машины
	return this->_host;
}
/**
 * @brief Метод установки адреса хоста текущей машины
 *
 * @param host адрес хоста текущей машины
 * @return     результат выполнения установки
 *
 */
bool awh::Server::setHost(string_view host) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		/**
		 * Определяем тип полученного IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_unit->addr.host(host))){
			// Для типа Unix Domain Socket
			case static_cast <uint8_t> (net_addr_t::type_t::FS): {
				// Если идентификатор сервера установлен
				if(this->_id.eid > 0){
					// Устанавливаем адрес хоста целевой машины для сервера
					result = this->_unit->server.setAddress(this->_id.eid, event::address_t::UDS, host);
					// Если адрес установлен успешно, сохраняем его
					if(result)
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::UDS);
				// Если идентификатор сервера не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(host), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Для типа IPv4
			case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
				// Если идентификатор сервера установлен
				if(this->_id.eid > 0){
					// Устанавливаем адрес хоста целевой машины для сервера
					result = (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setAddress(this->_id.eid, event::address_t::IPV4, host) : this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV4, host));
					// Если адрес установлен успешно, сохраняем его
					if(result)
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->getAddressUnit(event::address_t::IPV4);
				// Если идентификатор сервера не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(host), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
					#endif
				}
			} break;
			// Для типа IPv6
			case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
				// Если идентификатор сервера установлен
				if(this->_id.eid > 0){
					// Устанавливаем адрес хоста целевой машины для сервера
					result = (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setAddress(this->_id.eid, event::address_t::IPV6, host) : this->_unit->server.setAddress(this->_id.eid, event::address_t::IPV6, host));
					// Если адрес установлен успешно, сохраняем его
					if(result)
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->getAddressUnit(event::address_t::IPV6);
				// Если идентификатор сервера не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(host), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
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
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param address тип адреса сервера
 * @return        значение адреса сервера
 *
 */
string awh::Server::getAddress(const event::address_t address) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем адрес сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getAddress(this->_id.eid, address) : this->_unit->server.getAddress(this->_id.eid, address));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::Server::setAddress(const event::address_t address, string_view value) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Устанавливаем адрес сервера
			if((result = (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setAddress(this->_id.eid, address, value) : this->_unit->server.setAddress(this->_id.eid, address, value)))){
				/**
				 * Определяем семейство адресов, с которым работает сервер
				 */
				switch(static_cast <uint8_t> (this->familyUnit())){
					// Если сервер работает с адресами Unix Domain Socket
					case static_cast <uint8_t> (event::family_t::UDS):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::UDS);
					break;
					// Если сервер работает с адресами IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->getAddressUnit(event::address_t::IPV4);
					break;
					// Если сервер работает с адресами IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->getAddressUnit(event::address_t::IPV6);
					break;
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address), value), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения адреса сервера или клиента
 *
 * @param eid     идентификатор события сервера или клиента
 * @param address тип адреса сервера или клиента
 * @return        значение адреса сервера или клиента
 *
 */
string awh::Server::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Извлекаем адрес сервера или удалённого клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getAddress(eid, address) : this->_unit->server.getAddress(eid, address));
	// Если идентификатор сервера или клиента не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::Server::setAddress(const event::address_t address, const net::addr_t * value) noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0){
			// Устанавливаем адрес сервера
			if((result = (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setAddress(this->_id.eid, address, value) : this->_unit->server.setAddress(this->_id.eid, address, value)))){
				/**
				 * Определяем семейство адресов, с которым работает сервер
				 */
				switch(static_cast <uint8_t> (this->familyUnit())){
					// Если сервер работает с адресами IPv4
					case static_cast <uint8_t> (event::family_t::IPV4):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->getAddressUnit(event::address_t::IPV4);
					break;
					// Если сервер работает с адресами IPv6
					case static_cast <uint8_t> (event::family_t::IPV6):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->getAddressUnit(event::address_t::IPV6);
					break;
					// Если сервер работает с адресами Unix Domain Socket
					case static_cast <uint8_t> (event::family_t::UDS):
						// Сохраняем адрес хоста целевой машины для сервера
						this->_host = this->_unit->server.getAddress(this->_id.eid, event::address_t::UDS);
					break;
				}
			}
		// Если идентификатор сервера не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param address тип адреса сервера
 * @param value   объект для извлечения адреса сервера
 * @return        результат выполнения извлечения адреса сервера
 *
 */
bool awh::Server::getAddress(const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем адрес сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getAddress(this->_id.eid, address, value) : this->_unit->server.getAddress(this->_id.eid, address, value));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса сервера или клиента
 *
 * @param eid     идентификатор события сервера или клиента
 * @param address тип адреса сервера или клиента
 * @param value   объект для извлечения адреса сервера или клиента
 * @return        результат выполнения извлечения адреса сервера или клиента
 *
 */
bool awh::Server::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Извлекаем адрес сервера или удалённого клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getAddress(eid, address, value) : this->_unit->server.getAddress(eid, address, value));
	// Если идентификатор сервера или клиента не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (address)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @return    MTU сетевого интерфейса
 *
 */
uint16_t awh::Server::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Если идентификатор сервера является активным
	if(eid == this->_id.eid)
		// Извлекаем MTU сетевого интерфейса
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getMaximumTransmissionUnit(eid) : this->_unit->server.getMaximumTransmissionUnit(eid));
	// Если идентификатор сервера не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 *
 */
bool awh::Server::setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept {
	// Если идентификатор сервера является активным
	if(eid == this->_id.eid)
		// Устанавливаем MTU сетевого интерфейса
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setMaximumTransmissionUnit(eid, mtu) : this->_unit->server.setMaximumTransmissionUnit(eid, mtu));
	// Если идентификатор сервера не найден
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, mtu), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима трансляции пакетов сервера или клиента
 *
 * @param eid идентификатор события сервера или клиента
 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
 *
 */
awh::event::delivery_mode_t awh::Server::getDelivery(const event::id_t eid) const noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Извлекаем режим трансляции пакетов сервера или удалённого клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getDelivery(eid) : this->_unit->server.getDelivery(eid));
	// Если идентификатор сервера или клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::delivery_mode_t::NONE;
}
/**
 * @brief Метод установки режима трансляции пакетов сервера или клиента
 *
 * @param eid      идентификатор события сервера или клиента
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 *
 */
bool awh::Server::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера или клиента является активным
		if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
			// Устанавливаем режим трансляции пакетов сервера или удалённого клиента
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setDelivery(eid, delivery) : this->_unit->server.setDelivery(eid, delivery));
		// Если идентификатор сервера или клиента не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (delivery)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера сервера или клиента
 *
 * @param eid    идентификатор события сервера или клиента
 * @param action тип действия сервера или клиента
 * @return       размер буфера сервера или клиента
 *
 */
size_t awh::Server::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Извлекаем размер буфера сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getBufferSize(eid, action) : this->_unit->server.getBufferSize(eid, action));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера сервера или клиента
 *
 * @param eid    идентификатор события сервера или клиента
 * @param action тип действия сервера или клиента
 * @param size   размер буфера сервера или клиента
 * @return       результат выполнения установки
 *
 */
bool awh::Server::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Устанавливаем размер буфера сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setBufferSize(eid, action, size) : this->_unit->server.setBufferSize(eid, action, size));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения времени жизни DNS запроса
 *
 * @return время жизни DNS запроса в миллисекундах
 *
 */
uint32_t awh::Server::getAliveDNS() const noexcept {
	// Если идентификатор клиента установлен
	if(this->_id.eid > 0)
		// Возвращаем время жизни DNS запроса
		return this->_dns.alive.load(std::memory_order_acquire);
	// Если идентификатор клиента не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки времени жизни DNS запроса
 *
 * @param alive время жизни DNS запроса в миллисекундах
 *
 */
void awh::Server::setAliveDNS(const uint32_t alive) noexcept {
	// Устанавливаем время жизни DNS запроса
	this->_dns.alive.store(alive, std::memory_order_release);
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @return режим использования таймаута на чтение события
 *
 */
awh::event::usage_t awh::Server::getUsageReadTimeout() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем режим использования таймаута на чтение события
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getUsageReadTimeout(this->_id.eid) : this->_unit->server.getUsageReadTimeout(this->_id.eid));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод получения режима использования таймаута на чтение события сервера или клиента
 *
 * @param eid идентификатор события сервера или клиента
 * @return    режим использования таймаута на чтение события сервера или клиента
 *
 */
awh::event::usage_t awh::Server::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Извлекаем режим использования таймаута на чтение события сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getUsageReadTimeout(eid) : this->_unit->server.getUsageReadTimeout(eid));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 *
 */
void awh::Server::setUsageReadTimeout(const event::usage_t usage) noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем режим использования таймаута на чтение события
		(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setUsageReadTimeout(this->_id.eid, usage) : this->_unit->server.setUsageReadTimeout(this->_id.eid, usage));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (usage)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки режима использования таймаута на чтение события сервера или клиента
 *
 * @param eid   идентификатор события сервера или клиента
 * @param usage режим использования таймаута на чтение события сервера или клиента (reusable или disposable)
 *
 */
void awh::Server::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Устанавливаем режим использования таймаута на чтение события сервера или клиента
		(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setUsageReadTimeout(eid, usage) : this->_unit->server.setUsageReadTimeout(eid, usage));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (usage)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод получения таймаута сервера
 *
 * @param action тип действия сервера
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::Server::getTimeout(const event::action_t action) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Извлекаем таймаут сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getTimeout(this->_id.eid, action) : this->_unit->server.getTimeout(this->_id.eid, action));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения таймаута сервера или клиента
 *
 * @param eid    идентификатор события сервера или клиента
 * @param action тип действия сервера или клиента
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::Server::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Извлекаем таймаут сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getTimeout(eid, action) : this->_unit->server.getTimeout(eid, action));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута сервера
 *
 * @param action  тип действия сервера
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::Server::setTimeout(const event::action_t action, const uint32_t timeout) noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем таймаут сервера
		(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setTimeout(this->_id.eid, action, timeout) : this->_unit->server.setTimeout(this->_id.eid, action, timeout));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки таймаута сервера или клиента
 *
 * @param eid     идентификатор события сервера или клиента
 * @param action  тип действия сервера или клиента
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::Server::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Устанавливаем таймаут сервера или клиента
		(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setTimeout(eid, action, timeout) : this->_unit->server.setTimeout(eid, action, timeout));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (action), timeout), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
}
/**
 * @brief Метод установки пропускной способности сервера
 *
 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 */
bool awh::Server::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем пропускную способность сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.bandwidth(this->_id.eid, limiting, bandwidth) : this->_unit->server.bandwidth(this->_id.eid, limiting, bandwidth));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки пропускной способности сервера или клиента
 *
 * @param eid       идентификатор события сервера или клиента
 * @param limiting  режим ограничения пропускной способности сервера или клиента (egress или ingress)
 * @param bandwidth пропускная способность сервера или клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 */
bool awh::Server::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Устанавливаем пропускную способность сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.bandwidth(eid, limiting, bandwidth) : this->_unit->server.bandwidth(eid, limiting, bandwidth));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (limiting), bandwidth), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров keep-alive для сервера или клиента
 *
 * @param eid   идентификатор события сервера или клиента
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 *
 */
bool awh::Server::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если идентификатор сервера или клиента является активным
	if((eid == this->_id.eid) || (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.isActual(eid) : this->_unit->server.isActual(eid)))
		// Устанавливаем параметры keep-alive для сервера или клиента
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.keepAlive(eid, cnt, idle, intvl) : this->_unit->server.keepAlive(eid, cnt, idle, intvl));
	// Если идентификатор сервера или клиента не найден в списке обслуживаемых клиентов
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server or сlient is not initialized", __PRETTY_FUNCTION__, make_tuple(eid, cnt, idle, intvl), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server or сlient is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @return значение DSCP
 *
 */
awh::event::dscp_t awh::Server::getDifferentiatedServicesCodePoint() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Получаем значение DSCP для сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getDifferentiatedServicesCodePoint(this->_id.eid) : this->_unit->server.getDifferentiatedServicesCodePoint(this->_id.eid));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
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
 *
 */
bool awh::Server::setDifferentiatedServicesCodePoint(const event::dscp_t dscp) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем значение DSCP для сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setDifferentiatedServicesCodePoint(this->_id.eid, dscp) : this->_unit->server.setDifferentiatedServicesCodePoint(this->_id.eid, dscp));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (dscp)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения обнаружения максимального размера пакета (MTU)
 *
 * @return режим обнаружения максимального размера пакета (MTU)
 *
 */
awh::event::mtu_discover_t awh::Server::getMaximumTransmissionUnitDiscover() const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Получаем режим обнаружения максимального размера пакета (MTU) для сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.getMaximumTransmissionUnitDiscover(this->_id.eid) : this->_unit->server.getMaximumTransmissionUnitDiscover(this->_id.eid));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
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
 *
 */
bool awh::Server::setMaximumTransmissionUnitDiscover(const event::mtu_discover_t mode) const noexcept {
	// Если идентификатор сервера установлен
	if(this->_id.eid > 0)
		// Устанавливаем режим обнаружения максимального размера пакета (MTU) для сервера
		return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.setMaximumTransmissionUnitDiscover(this->_id.eid, mode) : this->_unit->server.setMaximumTransmissionUnitDiscover(this->_id.eid, mode));
	// Если идентификатор сервера не установлен
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
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
 *
 */
bool awh::Server::membership(const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0)
			// Устанавливаем активацию/деактивацию мультикаст группы для сервера
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.membership(this->_id.eid, mode, group, source, port) : this->_unit->server.membership(this->_id.eid, mode, group, source, port));
		// Если идентификатор сервера не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode), group, source, port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
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
 *
 */
bool awh::Server::membership(const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	if(!this->active()){
		// Если идентификатор сервера установлен
		if(this->_id.eid > 0)
			// Устанавливаем активацию/деактивацию мультикаст группы для сервера
			return (this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.membership(this->_id.eid, mode, group, source, port) : this->_unit->server.membership(this->_id.eid, mode, group, source, port));
		// Если идентификатор сервера не установлен
		else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (mode), group, source, port), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод инициализации сервера
 *
 * @param family   семейство адресов
 * @param type     тип события
 * @param protocol протокол события
 * @return         идентификатор созданного сервера
 *
 */
awh::event::id_t awh::Server::init(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Если идентификатор сервера не установлен
	if(this->_id.eid == 0){
		// Выдаём новый идентификатор DNS-резолвера для клиента
		this->_dns.id = this->_dns.client->issue();
		// Запоминаем протокол транспорта сервера
		this->_protocol = protocol;
		// Запоминаем тип сокета транспорта сервера (для QUIC соединения работают поверх дейтаграммного сокета)
		this->_type = ((protocol == event::protocol_t::QUIC) ? event::type_t::DATAGRAM : type);
		/**
		 * Для транспорта QUIC событие создаётся на выделенном юните сервера QUIC:
		 * ему же переносится шаблон контекста безопасности, а слой записей
		 * TLS-over-stream в дальнейшем обходится (RFC 9001). Остальные транспорты
		 * создают событие на общем юните сервера
		 */
		if(protocol == event::protocol_t::QUIC){
			// Передаём шаблон контекста безопасности соединениям выделенного юнита сервера QUIC
			if((this->_tls.coder != nullptr) && (this->_id.cts > 0))
				this->_unit->quic.context(* this->_tls.coder, this->_id.cts);
			// Инициализируем новое событие сервера на выделенном юните сервера QUIC
			this->_id.eid = this->_unit->quic.issue(family, type, protocol);
		// Инициализируем новое событие сервера на общем юните сервера
		} else this->_id.eid = this->_unit->server.issue(family, type, protocol);
	// Если идентификатор сервера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("This server has already been initialized", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (type), static_cast <uint16_t> (protocol)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("This server has already been initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return this->_id.eid;
}
/**
 * @brief Метод установки названия кластера
 *
 * @param name название кластера для установки
 *
 */
void awh::Server::clusterName(string_view name) noexcept {
	// Устанавливаем название кластера на активном юните транспорта
	(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.clusterName(name) : this->_unit->server.clusterName(name));
}
/**
 * @brief Метод получения семейства кластера
 *
 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
 *
 */
awh::unit::cluster_t::family_t awh::Server::clusterFamily() const noexcept {
	// Получаем семейство кластера с активного юнита транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterFamily() : this->_unit->server.clusterFamily());
}
/**
 * @brief Метод получения режима активации кластера
 *
 * @return режим активации кластера
 *
 */
awh::event::mode_t awh::Server::clusterMode() const noexcept {
	// Получаем режим активации кластера с активного юнита транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterMode() : this->_unit->server.clusterMode());
}
/**
 * @brief Метод установки количества процессов кластера
 *
 * @param mode флаг активации/деактивации кластера
 * @param size количество рабочих процессов
 *
 */
void awh::Server::clusterMode(const event::mode_t mode) noexcept {
	// Устанавливаем режим активации кластера на активном юните транспорта
	(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.clusterMode(mode) : this->_unit->server.clusterMode(mode));
}
/**
 * @brief Метод получения максимального количества процессов
 *
 * @return максимальное количество процессов
 *
 */
uint16_t awh::Server::clusterCount() const noexcept {
	// Получаем максимальное количество процессов для кластера с активного юнита транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterCount() : this->_unit->server.clusterCount());
}
/**
 * @brief Метод установки максимального количества процессов
 *
 * @param count максимальное количество процессов
 *
 */
void awh::Server::clusterCount(const uint16_t count) noexcept {
	// Устанавливаем максимальное количество процессов для кластера на активном юните транспорта
	(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.clusterCount(count) : this->_unit->server.clusterCount(count));
}
/**
 * @brief Метод получения списка дочерних процессов
 *
 * @return список дочерних процессов
 *
 */
unordered_set <pid_t> awh::Server::clusterWorkers() const noexcept {
	// Получаем список дочерних процессов для кластера с активного юнита транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterWorkers() : this->_unit->server.clusterWorkers());
}
/**
 * @brief Метод установки диапазона портов для выделения дочерним процессам кластера
 *
 * @param begin начальный порт диапазона (0 - использовать порт прослушивания)
 * @param end   конечный порт диапазона (0 - использовать порт прослушивания)
 *
 */
void awh::Server::clusterRange(const uint16_t begin, const uint16_t end) noexcept {
	/**
	 * Диапазон портов применяется только к транспорту QUIC: у остальных транспортов
	 * дочерние процессы наследуют общий сокет прослушивания родительского процесса
	 */
	if(this->_protocol == event::protocol_t::QUIC)
		// Устанавливаем диапазон портов для выделения дочерним процессам на юните QUIC
		this->_unit->quic.clusterRange(begin, end);
	// Устанавливаем единственный порт прослушивания, так-как не поддерживается диапазон портов
	else this->setPort(begin);
}
/**
 * @brief Метод получения списка дочерних процессов, не получивших порт прослушивания
 *
 * @return список идентификаторов дочерних процессов, работающих в холостую
 *
 */
unordered_set <pid_t> awh::Server::clusterIdle() const noexcept {
	/**
	 * Список работающих в холостую дочерних процессов ведётся только для транспорта
	 * QUIC: у остальных транспортов дочерние процессы наследуют общий сокет
	 */
	if(this->_protocol == event::protocol_t::QUIC)
		// Возвращаем список дочерних процессов, не получивших порт прослушивания
		return this->_unit->quic.clusterIdle();
	// Возвращаем значение по умолчанию
	return {};
}
/**
 * @brief Метод отправки порта прослушивания конкретному дочернему процессу кластера
 *
 * @param pid  идентификатор дочернего процесса
 * @param port порт прослушивания для дочернего процесса
 * @return     результат отправки порта дочернему процессу
 *
 */
bool awh::Server::clusterAssign(const pid_t pid, const uint16_t port) noexcept {
	/**
	 * Ручная доотправка порта дочернему процессу поддерживается только транспортом
	 * QUIC: у остальных транспортов сокет прослушивания наследуется от родительского
	 * процесса, поэтому отдельная раздача портов не требуется
	 */
	if(this->_protocol == event::protocol_t::QUIC)
		// Отправляем порт прослушивания дочернему процессу на юните QUIC
		return this->_unit->quic.clusterAssign(pid, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки сообщения родительскому процессу
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::Server::clusterSend(const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active())
		// Отправляем сообщение родительскому процессу через активный юнит транспорта
		return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterSend(buffer, size) : this->_unit->server.clusterSend(buffer, size));
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения дочернему процессу
 *
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::Server::clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active())
		// Отправляем сообщение дочернему процессу через активный юнит транспорта
		return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterSend(pid, buffer, size) : this->_unit->server.clusterSend(pid, buffer, size));
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(pid, buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения всем дочерним процессам
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::Server::clusterBroadcast(const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или сервер находятся в рабочем состоянии
	if(this->active())
		// Отправляем сообщение всем дочерним процессам через активный юнит транспорта
		return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterBroadcast(buffer, size) : this->_unit->server.clusterBroadcast(buffer, size));
	// Если DNS-резолвер или сервер находятся в нерабочем состоянии
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Server is not initialized", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Server is not initialized", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки флага автоматического возрождения процессов
 *
 * @param mode флаг возрождения процессов
 *
 */
void awh::Server::clusterRebirth(const bool mode) noexcept {
	// Устанавливаем флаг автоматического возрождения процессов для кластера на активном юните транспорта
	(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.clusterRebirth(mode) : this->_unit->server.clusterRebirth(mode));
}
/**
 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
 *
 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
 *
 */
void awh::Server::clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept {
	// Устанавливаем параметры защиты от цикла перезапусков процессов кластера на активном юните транспорта
	(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.clusterRebirthLimit(limit, window) : this->_unit->server.clusterRebirthLimit(limit, window));
}
/**
 * @brief Метод получения типа протокола передачи данных между воркерами
 *
 * @return тип протокола передачи данных между воркерами
 *
 */
awh::event::type_t awh::Server::clusterGetTypeEventMessage() const noexcept {
	// Выполняем получение типа протокола передачи данных между воркерами с активного юнита транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterGetTypeEventMessage() : this->_unit->server.clusterGetTypeEventMessage());
}
/**
 * @brief Метод установки типа протокола передачи данных между воркерами
 *
 * @param type тип протокола передачи данных между воркерами для установки
 *
 */
void awh::Server::clusterSetTypeEventMessage(const event::type_t type) noexcept {
	// Выполняем установку типа протокола передачи данных между воркерами на активном юните транспорта
	(this->_protocol == event::protocol_t::QUIC ? this->_unit->quic.clusterSetTypeEventMessage(type) : this->_unit->server.clusterSetTypeEventMessage(type));
}
/**
 * @brief Метод получения размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @return       размер буфера события
 *
 */
size_t awh::Server::clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept {
	// Получаем размер буфера события с активного юнита транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterGetBufferSize(pid, action) : this->_unit->server.clusterGetBufferSize(pid, action));
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 *
 */
bool awh::Server::clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept {
	// Устанавливаем размер буфера события на активном юните транспорта
	return ((this->_protocol == event::protocol_t::QUIC) ? this->_unit->quic.clusterSetBufferSize(pid, action, size) : this->_unit->server.clusterSetBufferSize(pid, action, size));
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Server::Server(const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _protocol(event::protocol_t::NONE), _type(event::type_t::NONE), _fmk(fmk), _log(log) {
	// Создаём объект юнита сервера
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
	this->_unit->server.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
	this->_unit->server.on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
	/**
	 * Функции обратного вызова выделенного юнита сервера QUIC: юнит QUIC работает
	 * собственной событийной моделью (сессии соединений поверх одного UDP-сокета,
	 * адресуемые по Connection ID), поэтому его колбэки подписываются отдельно
	 */
	// Устанавливаем функцию обратного вызова на событие изменения статуса выделенного юнита сервера QUIC
	this->_unit->quic.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие установленного соединения QUIC (транслируется как принятие соединения)
	this->_unit->quic.on <void (const event::id_t)> ("open", &server_t::opened, this, _1);
	// Устанавливаем функцию обратного вызова на событие собранных данных потока соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", &server_t::stream, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на освобождение буфера отправки потока
	this->_unit->quic.on <void (const event::id_t, const uint64_t)> ("writable", &server_t::writable, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие принятой датаграммы приложения QUIC
	this->_unit->quic.on <void (const event::id_t, const string &)> ("datagram", &server_t::message, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие завершения соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const quic::error_t)> ("close", &server_t::closed, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->server.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &server_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие записи данных!
	this->_unit->server.on <void (const event::id_t, const size_t, void *)> ("write", &server_t::write, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, void *)> ("state", &server_t::state, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие обработки действий сервера
	this->_unit->server.on <void (const event::id_t, const event::action_t, void *)> ("action", &server_t::action, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие получения данных сервером
	this->_unit->server.on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &server_t::read, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие ошибок сервера
	this->_unit->server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &server_t::error, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
	this->_unit->server.on <bool (const event::id_t, const event::action_t, const uint32_t, void *)> ("timeout", &server_t::timeout, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, const size_t, void *)> ("available", &server_t::available, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие невозможности отправки данных сервером
	this->_unit->server.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t, void *)> ("spool", &server_t::spool, this, _1, _2, _3, _4, _5);
	// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
	this->_unit->server.on <void (const pid_t, const int32_t)> ("cluster_exit", &server_t::exitCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
	this->_unit->server.on <void (const pid_t, const pid_t)> ("cluster_rebase", &server_t::rebaseCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
	this->_unit->server.on <void (const pid_t, const size_t)> ("cluster_sending", &server_t::sendingCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
	this->_unit->server.on <void (const pid_t, const event::status_t)> ("cluster_state", &server_t::stateCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
	this->_unit->server.on <void (const pid_t, const unit::cluster_t::event_t)> ("cluster_events", &server_t::eventsCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
	this->_unit->server.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server_t::messageCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
	this->_unit->server.on <void (const pid_t, const event::error_t, const string &)> ("cluster_error", &server_t::errorCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
	this->_unit->server.on <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", &server_t::availableCluster, this, _1, _2, _3);
	// Дублируем подписки событий кластера на выделенный юнит QUIC (неактивный юнит эти события не эмитит)
	this->_unit->quic.on <void (const pid_t, const int32_t)> ("cluster_exit", &server_t::exitCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const pid_t)> ("cluster_rebase", &server_t::rebaseCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const size_t)> ("cluster_sending", &server_t::sendingCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const event::status_t)> ("cluster_state", &server_t::stateCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const unit::cluster_t::event_t)> ("cluster_events", &server_t::eventsCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server_t::messageCluster, this, _1, _2, _3);
	this->_unit->quic.on <void (const pid_t, const event::error_t, const string &)> ("cluster_error", &server_t::errorCluster, this, _1, _2, _3);
	this->_unit->quic.on <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", &server_t::availableCluster, this, _1, _2, _3);
}
/**
 * @brief Конструктор
 *
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Server::Server(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _protocol(event::protocol_t::NONE), _type(event::type_t::NONE), _fmk(fmk), _log(log) {
	// Создаём объект юнита сервера
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
	this->_unit->server.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
	this->_unit->server.on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
	/**
	 * Функции обратного вызова выделенного юнита сервера QUIC: юнит QUIC работает
	 * собственной событийной моделью (сессии соединений поверх одного UDP-сокета,
	 * адресуемые по Connection ID), поэтому его колбэки подписываются отдельно
	 */
	// Устанавливаем функцию обратного вызова на событие изменения статуса выделенного юнита сервера QUIC
	this->_unit->quic.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие установленного соединения QUIC (транслируется как принятие соединения)
	this->_unit->quic.on <void (const event::id_t)> ("open", &server_t::opened, this, _1);
	// Устанавливаем функцию обратного вызова на событие собранных данных потока соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", &server_t::stream, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на освобождение буфера отправки потока
	this->_unit->quic.on <void (const event::id_t, const uint64_t)> ("writable", &server_t::writable, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие принятой датаграммы приложения QUIC
	this->_unit->quic.on <void (const event::id_t, const string &)> ("datagram", &server_t::message, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие завершения соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const quic::error_t)> ("close", &server_t::closed, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->server.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &server_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие записи данных!
	this->_unit->server.on <void (const event::id_t, const size_t, void *)> ("write", &server_t::write, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, void *)> ("state", &server_t::state, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие обработки действий сервера
	this->_unit->server.on <void (const event::id_t, const event::action_t, void *)> ("action", &server_t::action, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие получения данных сервером
	this->_unit->server.on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &server_t::read, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие ошибок сервера
	this->_unit->server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &server_t::error, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
	this->_unit->server.on <bool (const event::id_t, const event::action_t, const uint32_t, void *)> ("timeout", &server_t::timeout, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, const size_t, void *)> ("available", &server_t::available, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие невозможности отправки данных сервером
	this->_unit->server.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t, void *)> ("spool", &server_t::spool, this, _1, _2, _3, _4, _5);
	// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
	this->_unit->server.on <void (const pid_t, const int32_t)> ("cluster_exit", &server_t::exitCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
	this->_unit->server.on <void (const pid_t, const pid_t)> ("cluster_rebase", &server_t::rebaseCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
	this->_unit->server.on <void (const pid_t, const size_t)> ("cluster_sending", &server_t::sendingCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
	this->_unit->server.on <void (const pid_t, const event::status_t)> ("cluster_state", &server_t::stateCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
	this->_unit->server.on <void (const pid_t, const unit::cluster_t::event_t)> ("cluster_events", &server_t::eventsCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
	this->_unit->server.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server_t::messageCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
	this->_unit->server.on <void (const pid_t, const event::error_t, const string &)> ("cluster_error", &server_t::errorCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
	this->_unit->server.on <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", &server_t::availableCluster, this, _1, _2, _3);
	// Устанавливаем переданный объект DNS-резолвера для клиента
	this->_dns.client = dns;
	// Если объект DNS-резолвера установлен
	if(this->_dns.client != nullptr){
		// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
		this->_dns.client->on <void (const event::status_t)> ("status", &server_t::status, this, 1, _1);
		// Устанавливаем функции обратного вызова для обработки попыток подключения клиента к удалённому серверу
		this->_dns.client->on <void (const unit::dns_t::id_t, const string &, const uint8_t)> ("attempts", &server_t::attempts, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
		this->_dns.client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3, nullptr);
		// Устанавливаем функции обратного вызова для обработки событий невозможности отправки данных DNS-резолвером
		this->_dns.client->on <void (const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)>  ("failure", &server_t::failure, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки разрешения доменного имени в сетевой адрес
		this->_dns.client->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server_t::resolve, this, _1, _2, _3, _4);
	// Если объект DNS-резолвера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("DNS resolver object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("DNS resolver object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::_exit(EXIT_FAILURE);
	}
}
/**
 * @brief Конструктор
 *
 * @param cts   идентификатор шаблона контекста безопасности
 * @param coder объект транспортного уровня безопасности
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 *
 */
awh::Server::Server(const tls::coder_t::id_t cts, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _protocol(event::protocol_t::NONE), _type(event::type_t::NONE), _fmk(fmk), _log(log) {
	// Устанавливаем идентификатор шаблона контекста безопасности
	this->_id.cts = cts;
	// Устанавливаем объект транспортного уровня безопасности для сервера
	this->_tls.coder = coder;
	// Если объект транспортного уровня безопасности не установлен
	if(this->_tls.coder == nullptr){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("TLS object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("TLS object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::_exit(EXIT_FAILURE);
	}
	// Создаём объект юнита сервера
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
	this->_unit->server.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
	this->_unit->server.on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
	/**
	 * Функции обратного вызова выделенного юнита сервера QUIC: юнит QUIC работает
	 * собственной событийной моделью (сессии соединений поверх одного UDP-сокета,
	 * адресуемые по Connection ID), поэтому его колбэки подписываются отдельно
	 */
	// Устанавливаем функцию обратного вызова на событие изменения статуса выделенного юнита сервера QUIC
	this->_unit->quic.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие установленного соединения QUIC (транслируется как принятие соединения)
	this->_unit->quic.on <void (const event::id_t)> ("open", &server_t::opened, this, _1);
	// Устанавливаем функцию обратного вызова на событие собранных данных потока соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", &server_t::stream, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на освобождение буфера отправки потока
	this->_unit->quic.on <void (const event::id_t, const uint64_t)> ("writable", &server_t::writable, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие принятой датаграммы приложения QUIC
	this->_unit->quic.on <void (const event::id_t, const string &)> ("datagram", &server_t::message, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие завершения соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const quic::error_t)> ("close", &server_t::closed, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->server.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &server_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие записи данных!
	this->_unit->server.on <void (const event::id_t, const size_t, void *)> ("write", &server_t::write, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, void *)> ("state", &server_t::state, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие обработки действий сервера
	this->_unit->server.on <void (const event::id_t, const event::action_t, void *)> ("action", &server_t::action, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие получения данных сервером
	this->_unit->server.on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &server_t::read, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие ошибок сервера
	this->_unit->server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &server_t::error, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
	this->_unit->server.on <bool (const event::id_t, const event::action_t, const uint32_t, void *)> ("timeout", &server_t::timeout, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, const size_t, void *)> ("available", &server_t::available, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие невозможности отправки данных сервером
	this->_unit->server.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t, void *)> ("spool", &server_t::spool, this, _1, _2, _3, _4, _5);
	// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
	this->_unit->server.on <void (const pid_t, const int32_t)> ("cluster_exit", &server_t::exitCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
	this->_unit->server.on <void (const pid_t, const pid_t)> ("cluster_rebase", &server_t::rebaseCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
	this->_unit->server.on <void (const pid_t, const size_t)> ("cluster_sending", &server_t::sendingCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
	this->_unit->server.on <void (const pid_t, const event::status_t)> ("cluster_state", &server_t::stateCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
	this->_unit->server.on <void (const pid_t, const unit::cluster_t::event_t)> ("cluster_events", &server_t::eventsCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
	this->_unit->server.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server_t::messageCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
	this->_unit->server.on <void (const pid_t, const event::error_t, const string &)> ("cluster_error", &server_t::errorCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
	this->_unit->server.on <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", &server_t::availableCluster, this, _1, _2, _3);
	// Дублируем подписки событий кластера на выделенный юнит QUIC (неактивный юнит эти события не эмитит)
	this->_unit->quic.on <void (const pid_t, const int32_t)> ("cluster_exit", &server_t::exitCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const pid_t)> ("cluster_rebase", &server_t::rebaseCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const size_t)> ("cluster_sending", &server_t::sendingCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const event::status_t)> ("cluster_state", &server_t::stateCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const unit::cluster_t::event_t)> ("cluster_events", &server_t::eventsCluster, this, _1, _2);
	this->_unit->quic.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server_t::messageCluster, this, _1, _2, _3);
	this->_unit->quic.on <void (const pid_t, const event::error_t, const string &)> ("cluster_error", &server_t::errorCluster, this, _1, _2, _3);
	this->_unit->quic.on <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", &server_t::availableCluster, this, _1, _2, _3);
}
/**
 * @brief Конструктор
 *
 * @param cts   идентификатор шаблона контекста безопасности
 * @param coder объект транспортного уровня безопасности
 * @param dns   объект DNS-резолвера
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 *
 */
awh::Server::Server(const tls::coder_t::id_t cts, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
 _host{""}, _callback(fmk, log), _unit(nullptr), _protocol(event::protocol_t::NONE), _type(event::type_t::NONE), _fmk(fmk), _log(log) {
	// Устанавливаем идентификатор шаблона контекста безопасности
	this->_id.cts = cts;
	// Устанавливаем объект транспортного уровня безопасности для сервера
	this->_tls.coder = coder;
	// Если объект транспортного уровня безопасности не установлен
	if(this->_tls.coder == nullptr){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("TLS object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("TLS object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::_exit(EXIT_FAILURE);
	}
	// Создаём объект юнита сервера
	this->_unit = make_unique <unit_t> (fmk, log);
	// Устанавливаем функцию обратного вызова на событие изменения статуса сервера
	this->_unit->server.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие принятия нового соединения сервером
	this->_unit->server.on <void (const event::id_t, const event::id_t)> ("accept", &server_t::accept, this, _1, _2);
	/**
	 * Функции обратного вызова выделенного юнита сервера QUIC: юнит QUIC работает
	 * собственной событийной моделью (сессии соединений поверх одного UDP-сокета,
	 * адресуемые по Connection ID), поэтому его колбэки подписываются отдельно
	 */
	// Устанавливаем функцию обратного вызова на событие изменения статуса выделенного юнита сервера QUIC
	this->_unit->quic.on <void (const event::status_t)> ("status", &server_t::status, this, 0, _1);
	// Устанавливаем функцию обратного вызова на событие установленного соединения QUIC (транслируется как принятие соединения)
	this->_unit->quic.on <void (const event::id_t)> ("open", &server_t::opened, this, _1);
	// Устанавливаем функцию обратного вызова на событие собранных данных потока соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", &server_t::stream, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на освобождение буфера отправки потока
	this->_unit->quic.on <void (const event::id_t, const uint64_t)> ("writable", &server_t::writable, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие принятой датаграммы приложения QUIC
	this->_unit->quic.on <void (const event::id_t, const string &)> ("datagram", &server_t::message, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие завершения соединения QUIC
	this->_unit->quic.on <void (const event::id_t, const quic::error_t)> ("close", &server_t::closed, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие информационных метаданных о дейтаграммном пакете
	this->_unit->server.on <void (const event::id_t, const net::dgram_info_t &)> ("traffic", &server_t::traffic, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие записи данных!
	this->_unit->server.on <void (const event::id_t, const size_t, void *)> ("write", &server_t::write, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие изменения состояния сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, void *)> ("state", &server_t::state, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие обработки действий сервера
	this->_unit->server.on <void (const event::id_t, const event::action_t, void *)> ("action", &server_t::action, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие получения данных сервером
	this->_unit->server.on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &server_t::read, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие ошибок сервера
	this->_unit->server.on <void (const event::id_t, const event::error_t, const string &, void *)> ("error", &server_t::error, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие истечения таймаута сервера
	this->_unit->server.on <bool (const event::id_t, const event::action_t, const uint32_t, void *)> ("timeout", &server_t::timeout, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных сервера
	this->_unit->server.on <void (const event::id_t, const event::status_t, const size_t, void *)> ("available", &server_t::available, this, _1, _2, _3, _4);
	// Устанавливаем функцию обратного вызова на событие невозможности отправки данных сервером
	this->_unit->server.on <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t, void *)> ("spool", &server_t::spool, this, _1, _2, _3, _4, _5);
	// Устанавливаем функцию обратного вызова на событие завершения работы процесса кластера
	this->_unit->server.on <void (const pid_t, const int32_t)> ("cluster_exit", &server_t::exitCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие пересоздания процесса кластера
	this->_unit->server.on <void (const pid_t, const pid_t)> ("cluster_rebase", &server_t::rebaseCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие отправки сообщения процессу кластера
	this->_unit->server.on <void (const pid_t, const size_t)> ("cluster_sending", &server_t::sendingCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие изменения статуса процесса кластера
	this->_unit->server.on <void (const pid_t, const event::status_t)> ("cluster_state", &server_t::stateCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие активации/деактивации процесса кластера
	this->_unit->server.on <void (const pid_t, const unit::cluster_t::event_t)> ("cluster_events", &server_t::eventsCluster, this, _1, _2);
	// Устанавливаем функцию обратного вызова на событие получения сообщения от процесса кластера
	this->_unit->server.on <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", &server_t::messageCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие ошибки процесса кластера
	this->_unit->server.on <void (const pid_t, const event::error_t, const string &)> ("cluster_error", &server_t::errorCluster, this, _1, _2, _3);
	// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих сообщений кластера
	this->_unit->server.on <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", &server_t::availableCluster, this, _1, _2, _3);
	// Устанавливаем переданный объект DNS-резолвера для клиента
	this->_dns.client = dns;
	// Если объект DNS-резолвера установлен
	if(this->_dns.client != nullptr){
		// Устанавливаем функции обратного вызова для обработки событий статуса DNS-резолвера
		this->_dns.client->on <void (const event::status_t)> ("status", &server_t::status, this, 1, _1);
		// Устанавливаем функции обратного вызова для обработки попыток подключения клиента к удалённому серверу
		this->_dns.client->on <void (const unit::dns_t::id_t, const string &, const uint8_t)> ("attempts", &server_t::attempts, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки событий ошибок DNS-резолвера
		this->_dns.client->on <void (const event::id_t, const event::error_t, const string &)> ("error", &server_t::error, this, _1, _2, _3, nullptr);
		// Устанавливаем функции обратного вызова для обработки событий невозможности отправки данных DNS-резолвером
		this->_dns.client->on <void (const unit::dns_t::id_t, const unit::dns_t::record_t, const string &)>  ("failure", &server_t::failure, this, _1, _2, _3);
		// Устанавливаем функции обратного вызова для обработки разрешения доменного имени в сетевой адрес
		this->_dns.client->on <void (const unit::dns_t::id_t, const event::family_t, const string &, const net::addr_t *)> ("address", &server_t::resolve, this, _1, _2, _3, _4);
	// Если объект DNS-резолвера не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("DNS resolver object not set", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("DNS resolver object not set", log_t::flag_t::CRITICAL);
		#endif
		// Выходим из приложения
		::_exit(EXIT_FAILURE);
	}
}
/**
 * @brief Деструктор
 *
 */
awh::Server::~Server() noexcept {
	// Удаляем объект юнита сервера
	this->_unit.reset(nullptr);
}
