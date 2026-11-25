/**
 * @file httpproxy.cpp
 * @date 2026-11-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация клиента HTTP-прокси — установка туннеля через прокси-сервер методом CONNECT,
 *        пересылка обычных HTTP-запросов в absolute-form, авторизация на прокси (Basic/Digest/Bearer)
 *        и прозрачное проксирование прикладного трафика поверх базового клиента
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартный заголовочный файл
 */
#include <cstring>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

/**
 * Подключаем заголовочный файл модуля
 */
#include <client/http/proxy.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Метод изменения статуса клиента
 *
 * @param index  индекс очереди запускаемого события
 * @param status новый статус клиента
 *
 */
void awh::client::HttpProxy::status(const uint8_t index, const event::status_t status) noexcept {
	/**
	 * Временное состояние клиента
	 */
	switch(index){
		// Если мы получили статус события клиента
		case 0: {
			/**
			 * Определяем состояние клиента
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Подключаем клиента к прокси-серверу
					if(!this->_unit->client.connect(static_cast <event::id_t> (this->_id.eid))){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("Failed to connect to proxy server", __PRETTY_FUNCTION__, {
									static_cast <uint16_t> (status),
									static_cast <uint16_t> (index)
								}, log::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("Failed to connect to proxy server", log::flag_t::WARNING);
							#endif
						}
					// Если подключение выполнено
					} else {
						// Запускаем работу клиента, если клиент не запущен
						if(!this->_unit->client.launch(this->_id.eid)){
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("This client ID=%u cannot be started", __PRETTY_FUNCTION__, {
										static_cast <uint16_t> (status),
										static_cast <uint16_t> (index)
									}, log::flag_t::WARNING, static_cast <event::id_t> (this->_id.eid));
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("This client ID=%u cannot be started", log::flag_t::WARNING, this->_id.eid);
								#endif
							}
						// Если клиент запущен удачно, вызываем функцию обратного вызова
						} else this->_callback.call <void (const string &, const uint16_t)> ("launch", this->_unit->client.getTarget(this->_id.eid), this->_unit->client.getTargetPort(this->_id.eid));
					}
				} break;
				// Если событие клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("status", status);
				break;
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
							// Устанавливаем адрес прокси-сервера для клиента
							if(this->_unit->client.setTarget(this->_id.eid, this->_host)){
								/**
								 * В зависимости от статуса события клиента выполняем запуск
								 */
								switch(static_cast <uint8_t> (awh_cast <unit::unit_t *> (&this->_unit->client)->status(this->_id.eid))){
									// Если событие клиента не запущено, запускаем его
									case static_cast <uint8_t> (event::status_t::NONE): {
										// Если событие клиента не запущено, запускаем его
										if(this->_unit->client.commit(this->_id.eid)){
											// Вызываем функцию обратного вызова
											this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", this->_unit->client.family(this->_id.eid), this->_host, this->_unit->client.getTarget(this->_id.eid));
											// Запускаем клиента
											this->_unit->client.start();
										}
									} break;
									// Если событие клиента инициализировано, запускаем его
									case static_cast <uint8_t> (event::status_t::INITIAL):
									// Если событие находится в состоянии успешного подключения
									case static_cast <uint8_t> (event::status_t::SUCCESS):
										// Запускаем клиента
										this->_unit->client.start();
									break;
								}
							}
							// Выходим из функции
							return;
						}
					}
					// Разрешаем доменное имя прокси-сервера
					if(!this->_dns.client->resolve(this->_dns.id, this->_unit->client.family(this->_id.eid), this->_host, this->_dns.alive.load(std::memory_order_acquire))){
						// Создаём текст ошибки разрешения доменного имени
						const string error = fmk::format("It was not possible to obtain an IP address for the domain name \"%s\"", this->_host.c_str());
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("%s", __PRETTY_FUNCTION__, {
									static_cast <uint16_t> (status),
									static_cast <uint16_t> (index)
								}, log::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("%s", log::flag_t::WARNING, error.c_str());
							#endif
						// Вызываем функцию обратного вызова
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
 * @brief Метод обработки событий подключения клиента к прокси-серверу
 *
 * @param eid идентификатор клиента
 * @param ok  результат подключения
 *
 */
void awh::client::HttpProxy::connect(const event::id_t eid, const bool ok) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если подключение выполнено успешно
		if(ok){
			// Если объект транспортного уровня безопасности к прокси установлен
			if(this->_id.ctl > 0){
				// Если рукопожатие TLS к прокси не выполнено
				if(!tls::coder::handshake(this->_id.ctl)){
					// Если функция обратного вызова не установлена
					if(!this->_callback.is("error_tls")){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Записываем ошибку в лог
							log::debug("TLS handshake is failed", __PRETTY_FUNCTION__, {eid, ok}, log::flag_t::WARNING);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							log::print("TLS handshake is failed", log::flag_t::WARNING);
						#endif
					}
					// Уничтожаем событие клиента
					this->_unit->client.destroy(eid);
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const bool)> ("connect", false);
				}
			// Если объект транспортного уровня безопасности к прокси не установлен
			} else {
				// Если режим работы — туннель
				if(this->_ctx.mode == http::proxy::client_t::mode_t::TUNNEL)
					// Отправляем запрос CONNECT к прокси-серверу
					this->sendRequest();
				// Если режим работы — пересылка, соединение готово к приёму запросов
				else this->_callback.call <void (const bool)> ("connect", true);
			}
		// Если подключение не выполнено, вызываем функцию обратного вызова
		} else this->_callback.call <void (const bool)> ("connect", false);
	}
}
/**
 * @brief Метод обработки событий записи данных клиентом
 *
 * @param eid  идентификатор клиента
 * @param size размер данных для записи
 *
 */
void awh::client::HttpProxy::write(const event::id_t, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false))
		// Вызываем функцию обратного вызова
		this->_callback.call <void (const size_t)> ("write", size);
}
/**
 * @brief Метод обработки событий изменения состояния клиента
 *
 * @param eid    идентификатор клиента
 * @param status новый статус клиента
 *
 */
void awh::client::HttpProxy::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Вызываем функцию обратного вызова
		this->_callback.call <void (const event::status_t)> ("state", status);
		// Если статус клиента изменился на "уничтожен"
		if((eid == this->_id.eid) && (status == event::status_t::DESTROYED)){
			// Если режим работы — пересылка, уведомляем парсер о закрытии соединения
			if(this->_ctx.mode == http::proxy::client_t::mode_t::FORWARD)
				// Завершаем разбор тела, кадрируемого закрытием соединения
				this->_proxy.eof();
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
 * @brief Метод обработки событий получения данных клиентом
 *
 * @param eid    идентификатор клиента
 * @param buffer буфер данных клиента
 * @param size   размер данных клиента
 *
 */
void awh::client::HttpProxy::read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если туннель установлен и активен
			if(this->_ctx.state == http::proxy::client_t::state_t::COMPLETED)
				// Выдаём данные туннеля прикладному обмену
				this->tunnel(buffer, size);
			// Если текущее состояние находится ещё в процессе общения с прокси-сервером
			else {
				// Смещение потреблённых байт в буфере
				size_t offset = 0;
				// Число байт, потреблённых последним разбором
				size_t consumed = 0;
				/**
				 * Разбираем ответы прокси: конвейер в режиме пересылки, один ответ в туннеле
				 */
				do {
					// Разбираем данные ответа прокси
					consumed = this->_proxy.parse(buffer + offset, size - offset, this->_ctx);
					// Продвигаем смещение на число потреблённых байт
					offset += consumed;
					// Хвост буфера за потреблёнными байтами
					const uint8_t * tail = buffer + offset;
					// Размер хвоста буфера
					const size_t rest = size - offset;
					/**
					 * Определяем состояние обмена с прокси
					 */
					switch(static_cast <uint8_t> (this->_ctx.state)){
						// Если текущее состояние соответствует ошибке обмена с прокси
						case static_cast <uint8_t> (http::proxy::client_t::state_t::BROKEN): {
							// Создаём текст ошибки обмена с прокси
							const string error = ((this->_ctx.code > 0) ? fmk::format("Proxy server returned status %u", this->_ctx.code) : fmk::format("Failed to parse data from proxy server: %s", string(this->_proxy.error()).c_str()));
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("%s", __PRETTY_FUNCTION__, {eid, buffer, size}, log::flag_t::CRITICAL, error.c_str());
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("%s", log::flag_t::CRITICAL, error.c_str());
								#endif
							// Вызываем функцию обратного вызова
							} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::CONNECTION_FAIL, error);
							// Вызываем функцию обратного вызова
							this->_callback.call <void (const bool)> ("connect", false);
						} break;
						// Если текущее состояние соответствует повторной авторизации
						case static_cast <uint8_t> (http::proxy::client_t::state_t::AUTH):
							// Отправляем повторный запрос с учётными данными
							this->sendRequest();
						break;
						// Если текущее состояние соответствует успешной установке туннеля
						case static_cast <uint8_t> (http::proxy::client_t::state_t::HANDSHAKE): {
							// Порт хоста для подключения к удалённому серверу
							uint16_t port = 0;
							// Адрес хоста для подключения к удалённому серверу
							string target = "";
							// Если конечная точка клиента установлена
							if(this->_endpoint != nullptr){
								/**
								 * Определяем тип данных адреса конечной точки
								 */
								switch(static_cast <uint8_t> (this->_endpoint->type)){
									// Если тип данных соответствует FQDN
									case static_cast <uint8_t> (net::type_t::FQDN): {
										// Устанавливаем порт хоста для дальнейшего использования
										port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->port;
										// Устанавливаем доменное имя хоста для дальнейшего использования
										target = awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->domain;
									} break;
									// Если тип данных соответствует IPv4
									case static_cast <uint8_t> (net::type_t::IPV4):
									// Если тип данных соответствует IPv6
									case static_cast <uint8_t> (net::type_t::IPV6): {
										// Устанавливаем IP-адрес хоста для дальнейшего использования
										this->_unit->addr.source(awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip.get());
										// Устанавливаем адрес хоста для дальнейшего использования
										target = static_cast <string> (this->_unit->addr);
										// Устанавливаем порт хоста для дальнейшего использования
										port = awh_cast <net::attr_net_t *> (this->_endpoint.get())->port;
									} break;
								}
							}
							// Если адрес хоста для подключения к удалённому серверу получен успешно
							if(!target.empty()){
								// Доменное имя хоста для callback ready
								string domain = target;
								// Если конечная точка клиента установлена как FQDN
								if((this->_endpoint != nullptr) && (this->_endpoint->type == net::type_t::FQDN))
									// Устанавливаем доменное имя хоста
									domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->domain;
								// Устанавливаем состояние обмена как "завершённый"
								this->_ctx.state = http::proxy::client_t::state_t::COMPLETED;
								// Вызываем функцию обратного вызова
								this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", this->_unit->client.family(eid), domain, target);
								// Вызываем функцию обратного вызова
								this->_callback.call <void (const string &, const uint16_t)> ("launch", target, port);
								// Если объект транспортного уровня безопасности к цели установлен
								if(this->_ctl > 0){
									// Если рукопожатие TLS к цели не выполнено
									if(!tls::coder::handshake(this->_ctl)){
										// Если функция обратного вызова не установлена
										if(!this->_callback.is("error_tls")){
											/**
											 * Если включён режим отладки
											 */
											#if defined(DEBUG_MODE)
												// Записываем ошибку в лог
												log::debug("TLS handshake is failed", __PRETTY_FUNCTION__, {eid, buffer, size}, log::flag_t::WARNING);
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Записываем ошибку в лог
												log::print("TLS handshake is failed", log::flag_t::WARNING);
											#endif
										}
									}
								// Если объект транспортного уровня безопасности к цели не установлен
								} else
									// Вызываем функцию обратного вызова
									this->_callback.call <void (const bool)> ("connect", true);
								// Если в хвосте буфера присутствуют ранние данные туннеля
								if(rest > 0)
									// Выдаём ранние данные туннеля прикладному обмену
									this->tunnel(tail, rest);
							// Если адрес хоста для подключения к удалённому серверу не получен
							} else {
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Записываем ошибку в лог
									log::debug("Client endpoint is not set", __PRETTY_FUNCTION__, {eid, buffer, size}, log::flag_t::WARNING);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									log::print("Client endpoint is not set", log::flag_t::WARNING);
								#endif
								// Вызываем функцию обратного вызова
								this->_callback.call <void (const bool)> ("connect", false);
							}
						} break;
					}
					// Если режим пересылки и очередь ждёт ответа — отправляем неотправленные запросы
					if((this->_ctx.mode == http::proxy::client_t::mode_t::FORWARD) && (this->_ctx.state == http::proxy::client_t::state_t::REQUEST))
						// Отправляем запросы конвейера (в т.ч. повторную авторизацию)
						this->sendRequest();
					// В режиме туннеля разбирается один ответ, хвост уходит в туннель
					if(this->_ctx.mode == http::proxy::client_t::mode_t::TUNNEL)
						// Выходим из цикла разбора
						break;
				/**
				 * Продолжаем разбор конвейера, пока есть байты и очередь ждёт ответа
				 */
				} while((offset < size) && (this->_ctx.state == http::proxy::client_t::state_t::REQUEST) && (consumed > 0));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {eid, buffer, size}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод разрешения доменного имени прокси-сервера в сетевой адрес
 *
 * @param id     идентификатор DNS-запроса
 * @param family семейство адресов (IPv4/IPv6)
 * @param domain доменное имя для разрешения
 * @param addr   указатель на структуру для хранения результата разрешения
 *
 */
void awh::client::HttpProxy::resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если DNS-резолвер находится в рабочем состоянии
		if(this->_dns.client->working()){
			// Если событие клиента не запущено, запускаем его
			if(awh_cast <unit::unit_t *> (&this->_unit->client)->status(this->_id.eid) == event::status_t::NONE){
				// Устанавливаем адрес прокси-сервера для клиента
				if(this->_unit->client.setTarget(this->_id.eid, addr)){
					// Фиксация параметров клиента в сетевом движке выполнена удачно
					if(this->_unit->client.commit(this->_id.eid)){
						// Вызываем функцию обратного вызова
						this->_callback.call <void (const event::family_t, const string &, const string &)> ("ready", family, domain, this->_unit->client.getTarget(this->_id.eid));
						// Запускаем клиента
						this->_unit->client.start();
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
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {static_cast <uint16_t> (family), domain}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения состояния TLS к прокси-серверу
 *
 * @param ctl   идентификатор TLS
 * @param state состояние TLS
 *
 */
void awh::client::HttpProxy::stateTLS(const tls::coder::id_t, const tls::coder::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Вызываем функцию обратного вызова
		this->_callback.call <void (const tls::coder::state_t)> ("state_tls", state);
		// Если состояние рукопожатия TLS к прокси успешно завершено
		if(state == tls::coder::state_t::HANDSHAKED){
			// Если режим работы — туннель
			if(this->_ctx.mode == http::proxy::client_t::mode_t::TUNNEL)
				// Отправляем запрос CONNECT к прокси-серверу
				this->sendRequest();
			// Если режим работы — пересылка, соединение готово к приёму запросов
			else this->_callback.call <void (const bool)> ("connect", true);
		}
	}
}
/**
 * @brief Метод получения событий шифрования/дешифрования данных TLS к прокси
 *
 * @param ctl    идентификатор TLS
 * @param event  тип события TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 *
 */
void awh::client::HttpProxy::processTLS(const tls::coder::id_t, const tls::coder::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS к прокси
				case static_cast <uint8_t> (tls::coder::event_t::ENCRYPTION):
					// Ставим шифротекст в очередь отправки прокси-серверу
					this->residueTLS(buffer, size);
				break;
				// Если событие дешифрования данных TLS к прокси
				case static_cast <uint8_t> (tls::coder::event_t::DECRYPTION):
					// Обрабатываем расшифрованные данные как данные обмена с прокси
					this->read(this->_id.eid, buffer, size);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {static_cast <uint16_t> (event), buffer, size}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения состояния TLS к целевому хосту
 *
 * @param ctl   идентификатор TLS
 * @param state состояние TLS
 *
 */
void awh::client::HttpProxy::stateTargetTLS(const tls::coder::id_t, const tls::coder::state_t state) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если состояние рукопожатия TLS к цели успешно завершено
		if(state == tls::coder::state_t::HANDSHAKED)
			// Вызываем функцию обратного вызова
			this->_callback.call <void (const bool)> ("connect", true);
	}
}
/**
 * @brief Метод получения ошибок TLS к целевому хосту
 *
 * @param ctl     идентификатор TLS
 * @param error   код ошибки TLS
 * @param message сообщение об ошибке TLS
 *
 */
void awh::client::HttpProxy::errorTargetTLS(const tls::coder::id_t, const tls::coder::error_t error, const string & message) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("error_tls")){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {static_cast <uint16_t> (error), message}, log::flag_t::CRITICAL, message.c_str());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, message.c_str());
			#endif
		// Вызываем функцию обратного вызова
		} else this->_callback.call <void (const tls::coder::error_t, const string &)> ("error_tls", error, message);
	}
}
/**
 * @brief Метод получения событий шифрования/дешифрования данных TLS к цели
 *
 * @param ctl    идентификатор TLS
 * @param event  тип события TLS
 * @param buffer буфер данных для события шифрования/дешифрования TLS
 * @param size   размер данных для события шифрования/дешифрования TLS
 *
 */
void awh::client::HttpProxy::processTargetTLS(const tls::coder::id_t, const tls::coder::event_t event, const uint8_t * buffer, const size_t size) noexcept {
	// Если DNS-резолвер находится в рабочем состоянии или клиент находится в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS к цели
				case static_cast <uint8_t> (tls::coder::event_t::ENCRYPTION): {
					// Если отправка шифротекста через туннель не выполнена
					if(this->sendToProxy(buffer, size) != size){
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Записываем ошибку в лог
								log::debug("Failed to send data to remote server", __PRETTY_FUNCTION__, {static_cast <uint16_t> (event), buffer, size}, log::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								log::print("Failed to send data to remote server", log::flag_t::WARNING);
							#endif
						}
					}
				} break;
				// Если событие дешифрования данных TLS к цели
				case static_cast <uint8_t> (tls::coder::event_t::DECRYPTION):
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const uint8_t *, const size_t)> ("read", buffer, size);
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {static_cast <uint16_t> (event), buffer, size}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод выдачи данных туннеля прикладному обмену
 *
 * @param buffer буфер данных туннеля
 * @param size   размер данных туннеля
 *
 */
void awh::client::HttpProxy::tunnel(const uint8_t * buffer, const size_t size) noexcept {
	// Если объект транспортного уровня безопасности к цели установлен
	if(this->_ctl > 0){
		// Если данные туннеля не расшифрованы
		if(!tls::coder::decrypt(this->_ctl, buffer, size)){
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error_tls")){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("TLS decryption data is failed", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("TLS decryption data is failed", log::flag_t::WARNING);
				#endif
			}
		}
	// Если объект транспортного уровня безопасности к цели не установлен, вызываем функцию обратного вызова
	} else this->_callback.call <void (const uint8_t *, const size_t)> ("read", buffer, size);
}
/**
 * @brief Метод отправки запроса к прокси-серверу
 *
 */
void awh::client::HttpProxy::sendRequest() noexcept {
	// Если режим работы — пересылка, отправляем все неотправленные запросы конвейера
	if(this->_ctx.mode == http::proxy::client_t::mode_t::FORWARD){
		// Размер буфера данных запроса
		size_t size = 0;
		// Буфер данных запроса
		uint8_t * buffer = nullptr;
		/**
		 * Отправляем запросы конвейера по порядку, пока они есть в очереди
		 */
		while(this->_proxy.buffer(&buffer, size, this->_ctx)){
			// Если отправка запроса на прокси-сервер не выполнена
			if(this->sendToProxy(buffer, size) != size){
				// Если функция обратного вызова не установлена
				if(!this->_callback.is("error")){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Записываем ошибку в лог
						log::debug("Failed to send data to proxy server", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						log::print("Failed to send data to proxy server", log::flag_t::WARNING);
					#endif
				}
				// Прерываем отправку конвейера
				break;
			}
			// Освобождаем отправленные байты запроса
			this->_proxy.consume(size);
		}
		// Выходим из функции
		return;
	}
	// Если конечная точка установлена, а хост в контексте ещё не заполнен
	if((this->_endpoint != nullptr) && (this->_ctx.host == nullptr)){
		/**
		 * Определяем тип адреса конечной точки
		 */
		switch(static_cast <uint8_t> (this->_endpoint->type)){
			// Если тип данных соответствует FQDN
			case static_cast <uint8_t> (net::type_t::FQDN): {
				// Создаём объект параметров адреса целевого хоста
				this->_ctx.host = make_unique <net::attr_fqdn_t> ();
				// Устанавливаем тип адреса целевого хоста
				this->_ctx.host->type = net::type_t::FQDN;
				// Устанавливаем порт целевого хоста
				awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->port = awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->port;
				// Устанавливаем доменное имя целевого хоста
				awh_cast <net::attr_fqdn_t *> (this->_ctx.host.get())->domain = awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->domain;
			} break;
			// Если тип данных соответствует IPv4
			case static_cast <uint8_t> (net::type_t::IPV4): {
				// Создаём объект параметров адреса целевого хоста
				this->_ctx.host = make_unique <net::attr_net_t> ();
				// Устанавливаем тип адреса целевого хоста
				this->_ctx.host->type = net::type_t::IPV4;
				// Устанавливаем порт целевого хоста
				awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.get())->port;
				// Создаём новый объект адреса IPv4
				awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
				// Устанавливаем IP-адрес целевого хоста
				awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip.get())->address;
			} break;
			// Если тип данных соответствует IPv6
			case static_cast <uint8_t> (net::type_t::IPV6): {
				// Создаём объект параметров адреса целевого хоста
				this->_ctx.host = make_unique <net::attr_net_t> ();
				// Устанавливаем тип адреса целевого хоста
				this->_ctx.host->type = net::type_t::IPV6;
				// Устанавливаем порт целевого хоста
				awh_cast <net::attr_net_t *> (this->_ctx.host.get())->port = awh_cast <net::attr_net_t *> (this->_endpoint.get())->port;
				// Создаём новый объект адреса IPv6
				awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
				// Устанавливаем IP-адрес целевого хоста
				::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_ctx.host.get())->ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip.get())->address[0], 16);
			} break;
		}
	}
	// Размер буфера данных запроса
	size_t size = 0;
	// Буфер данных запроса
	uint8_t * buffer = nullptr;
	// Если извлечение буфера данных запроса выполнено успешно
	if(this->_proxy.buffer(&buffer, size, this->_ctx)){
		// Если отправка запроса на прокси-сервер не выполнена
		if(this->sendToProxy(buffer, size) != size){
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("Failed to send data to proxy server", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Failed to send data to proxy server", log::flag_t::WARNING);
				#endif
			}
			// Вызываем функцию обратного вызова
			this->_callback.call <void (const bool)> ("connect", false);
		// Если запрос отправлен, освобождаем отправленные байты
		} else this->_proxy.consume(size);
	// Если сборка запроса к прокси не выполнена
	} else {
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("error")){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Failed to build request to proxy server", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Failed to build request to proxy server", log::flag_t::WARNING);
			#endif
		// Вызываем функцию обратного вызова
		} else this->_callback.call <void (const event::error_t, const string &)> ("error", event::error_t::CONNECTION_FAIL, "Failed to build request to proxy server");
		// Вызываем функцию обратного вызова
		this->_callback.call <void (const bool)> ("connect", false);
	}
}
/**
 * @brief Метод отправки данных прокси-серверу
 *
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных прокси-серверу
 *
 */
size_t awh::client::HttpProxy::sendToProxy(const void * buffer, const size_t size) noexcept {
	// Если объект транспортного уровня безопасности к прокси установлен
	if(this->_id.ctl > 0){
		// Если шифрование данных TLS к прокси выполнено успешно
		if(tls::coder::encrypt(this->_id.ctl, buffer, size))
			// Возвращаем размер отправленных данных
			return size;
		// Возвращаем значение по умолчанию
		return 0;
	}
	// Отправляем данные прокси-серверу
	return this->_unit->client.send(this->_id.eid, buffer, size);
}
/**
 * @brief Метод приостановки работы клиента
 *
 * @return результат выполнения приостановки работы
 *
 */
bool awh::client::HttpProxy::pause() noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если идентификатор клиента установлен
			if(this->_id.eid > 0){
				// Приостанавливаем событие клиента
				result = this->_unit->client.pause(this->_id.eid);
			// Если идентификатор клиента не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Client is not initialized", log::flag_t::WARNING);
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод возобновления работы клиента
 *
 * @return результат выполнения возобновления работы
 *
 */
bool awh::client::HttpProxy::resume() noexcept {
	// Переменная результата
	bool result = false;
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если идентификатор клиента установлен
			if(this->_id.eid > 0){
				// Возобновляем работу события клиента
				result = this->_unit->client.resume(this->_id.eid);
			// Если идентификатор клиента не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Записываем ошибку в лог
					log::debug("Client is not initialized", __PRETTY_FUNCTION__, {}, log::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log::print("Client is not initialized", log::flag_t::WARNING);
				#endif
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод мультиподключения клиентов к удалённым хостам (заглушка для клиента HTTP-прокси)
 *
 * @return результат выполнения подключения
 *
 */
bool awh::client::HttpProxy::connect() noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера (заглушка для клиента HTTP-прокси)
 *
 * @return результат выполнения отключения
 *
 */
bool awh::client::HttpProxy::disconnect() noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения данных от сервера
 *
 * @return результат получения данных
 *
 */
bool awh::client::HttpProxy::recv() noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем данные от прокси-сервера
			return this->_unit->client.recv(this->_id.eid);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
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
 *
 */
size_t awh::client::HttpProxy::send(const void * buffer, const size_t size) noexcept {
	// Если DNS-резолвер или клиент находятся в рабочем состоянии
	if(this->_dns.client != nullptr ? this->_dns.client->working() : (this->_unit != nullptr ? this->_unit->client.working() : false)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если режим работы — туннель
			if(this->_ctx.mode == http::proxy::client_t::mode_t::TUNNEL){
				// Если туннель ещё не установлен
				if(this->_ctx.state != http::proxy::client_t::state_t::COMPLETED)
					// Данные не отправлены
					return 0;
				// Если объект транспортного уровня безопасности к цели установлен
				if(this->_ctl > 0){
					// Если шифрование данных TLS к цели выполнено успешно
					if(tls::coder::encrypt(this->_ctl, buffer, size))
						// Возвращаем размер отправленных данных
						return size;
					// Возвращаем значение по умолчанию
					return 0;
				}
				// Отправляем данные через туннель прокси-серверу
				return this->sendToProxy(buffer, size);
			// Если режим работы — пересылка
			} else {
				// Принимаем запросы пользователя для пересылки (конвейер)
				if(!this->_proxy.forward(buffer, size, this->_ctx))
					// Данные не приняты
					return 0;
				// Отправляем собранные запросы конвейера прокси-серверу
				this->sendRequest();
				// Возвращаем размер принятых данных
				return size;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {buffer, size}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод объединения данных между клиентом и другим событием (заглушка для клиента HTTP-прокси)
 *
 * @return результат выполнения объединения
 *
 */
bool awh::client::HttpProxy::splice(const event::id_t, const event::direct_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 */
bool awh::client::HttpProxy::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если идентификатор клиента установлен
		if(this->_id.eid > 0){
			// Устанавливаем пропускную способность клиента
			result = this->_unit->client.bandwidth(this->_id.eid, limiting, bandwidth);
		// Если идентификатор клиента не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("Client is not initialized", __PRETTY_FUNCTION__, {static_cast <uint16_t> (limiting), bandwidth}, log::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("Client is not initialized", log::flag_t::WARNING);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {static_cast <uint16_t> (limiting), bandwidth}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента HTTP-прокси)
 *
 * @return результат выполнения установки
 *
 */
bool awh::client::HttpProxy::membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента HTTP-прокси)
 *
 * @return результат выполнения установки
 *
 */
bool awh::client::HttpProxy::membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept {
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки контекста безопасности TLS к целевому хосту
 *
 * @param ctl идентификатор контекста безопасности TLS к целевому хосту
 * @return    результат выполнения установки
 *
 */
bool awh::client::HttpProxy::targetTLS(const tls::coder::id_t ctl) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		// Если идентификатор контекста безопасности установлен
		if(ctl > 0){
			// Устанавливаем идентификатор контекста безопасности TLS к цели
			this->_ctl = ctl;
			// Подписываемся на событие состояния TLS к цели
			tls::coder::on(this->_ctl, std::bind(&http_proxy_t::stateTargetTLS, this, _1, _2));
			// Подписываемся на событие ошибок TLS к цели
			tls::coder::on(this->_ctl, std::bind(&http_proxy_t::errorTargetTLS, this, _1, _2, _3));
			// Подписываемся на событие шифрования/дешифрования данных TLS к цели
			tls::coder::on(this->_ctl, std::bind(&http_proxy_t::processTargetTLS, this, _1, _2, _3, _4));
			// Возвращаем положительный результат
			return true;
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки токена доступа BEARER для авторизации на прокси
 *
 * @param token токен доступа для авторизации на прокси
 *
 */
void awh::client::HttpProxy::setToken(const string & token) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working())
		// Устанавливаем токен доступа для объекта клиента
		this->_proxy.setToken(token);
}
/**
 * @brief Метод установки параметров авторизации на прокси
 *
 * @param username имя пользователя для авторизации на прокси
 * @param password пароль пользователя для авторизации на прокси
 *
 */
void awh::client::HttpProxy::setUser(const string & username, const string & password) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working())
		// Устанавливаем параметры авторизации для объекта клиента
		this->_proxy.setUser(username, password);
}
/**
 * @brief Метод установки режима превентивной авторизации на прокси
 *
 * @param mode флаг превентивной отправки учётных данных
 *
 */
void awh::client::HttpProxy::preemptive(const bool mode) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working())
		// Устанавливаем режим превентивной авторизации для объекта клиента
		this->_proxy.preemptive(mode);
}
/**
 * @brief Метод установки режима работы через прокси
 *
 * @param mode режим работы (TUNNEL — туннель CONNECT, FORWARD — пересылка запросов)
 *
 */
void awh::client::HttpProxy::mode(const http::proxy::client_t::mode_t mode) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working())
		// Устанавливаем режим работы через прокси
		this->_ctx.mode = mode;
}
/**
 * @brief Метод установки конечной точки клиента
 *
 * @param attr параметры подключения для установки конечной точки
 * @return     результат выполнения установки конечной точки
 *
 */
bool awh::client::HttpProxy::endpoint(const net::attr_t * attr) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если указатель на адрес конечной точки не пустой
			if(attr != nullptr){
				// Сбрасываем объект атрибутов конечной точки
				this->_endpoint.reset(nullptr);
				// Сбрасываем адрес целевого хоста в контексте обмена
				this->_ctx.host.reset(nullptr);
				/**
				 * Определяем тип полученного адреса
				 */
				switch(static_cast <uint8_t> (attr->type)){
					// Для типа FQDN
					case static_cast <uint8_t> (net::type_t::FQDN): {
						// Создаём объект параметров подключения конечной точки
						this->_endpoint = make_unique <net::attr_fqdn_t> ();
						// Устанавливаем тип параметров подключения конечной точки
						this->_endpoint->type = net::type_t::FQDN;
						// Устанавливаем полученный порт
						awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->port = awh_cast <const net::attr_fqdn_t *> (attr)->port;
						// Устанавливаем полученное доменное имя
						awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->domain = awh_cast <const net::attr_fqdn_t *> (attr)->domain;
					} break;
					// Для типа IPv4
					case static_cast <uint8_t> (net::type_t::IPV4): {
						// Получаем объект переданных атрибутов сетевого адреса
						const net::attr_net_t * source = awh_cast <const net::attr_net_t *> (attr);
						// Если в переданных атрибутах IP-адрес не установлен
						if(source->ip == nullptr)
							// Прерываем выполнение
							break;
						// Создаём объект параметров подключения конечной точки
						this->_endpoint = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения конечной точки
						this->_endpoint->type = net::type_t::IPV4;
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (this->_endpoint.get())->port = source->port;
						// Создаём новый объект адреса IPv4
						awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем полученный IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (source->ip.get())->address;
					} break;
					// Для типа IPv6
					case static_cast <uint8_t> (net::type_t::IPV6): {
						// Получаем объект переданных атрибутов сетевого адреса
						const net::attr_net_t * source = awh_cast <const net::attr_net_t *> (attr);
						// Если в переданных атрибутах IP-адрес не установлен
						if(source->ip == nullptr)
							// Прерываем выполнение
							break;
						// Создаём объект параметров подключения конечной точки
						this->_endpoint = make_unique <net::attr_net_t> ();
						// Устанавливаем тип параметров подключения конечной точки
						this->_endpoint->type = net::type_t::IPV6;
						// Создаём новый объект адреса IPv6
						awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем полученный порт
						awh_cast <net::attr_net_t *> (this->_endpoint.get())->port = source->port;
						// Устанавливаем полученный IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (source->ip.get())->address[0], 16);
					} break;
				}
				// Возвращаем результат наличия объекта атрибутов конечной точки
				return ((this->_endpoint != nullptr) && (this->_endpoint->type != net::type_t::NONE));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки конечной точки клиента
 *
 * @param addr адрес хоста для установки
 * @param port порт хоста для установки
 * @return     результат выполнения установки конечной точки
 *
 */
bool awh::client::HttpProxy::endpoint(string_view addr, const uint16_t port) noexcept {
	// Если DNS-резолвер или клиент находятся в нерабочем состоянии
	if(this->_dns.client != nullptr ? !this->_dns.client->working() : !this->_unit->client.working()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если адрес для установки конечной точки не пустой
			if(!addr.empty()){
				// Сбрасываем объект атрибутов конечной точки
				this->_endpoint.reset(nullptr);
				// Сбрасываем адрес целевого хоста в контексте обмена
				this->_ctx.host.reset(nullptr);
				// Разбираем IP-адрес
				if(this->_unit->addr.parse(addr)){
					/**
					 * Определяем тип полученного IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_unit->addr.type())){
						// Для типа FQDN
						case static_cast <uint8_t> (net_addr_t::type_t::FQDN): {
							// Создаём объект параметров подключения конечной точки
							this->_endpoint = make_unique <net::attr_fqdn_t> ();
							// Устанавливаем тип параметров подключения конечной точки
							this->_endpoint->type = net::type_t::FQDN;
							// Устанавливаем полученный порт
							awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->port = port;
							// Устанавливаем полученное доменное имя хоста для подключения
							awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->domain = addr;
						} break;
						// Для типа IPv4
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
							// Создаём объект параметров подключения конечной точки
							this->_endpoint = make_unique <net::attr_net_t> ();
							// Устанавливаем тип параметров подключения конечной точки
							this->_endpoint->type = net::type_t::IPV4;
							// Устанавливаем полученный порт
							awh_cast <net::attr_net_t *> (this->_endpoint.get())->port = port;
							// Устанавливаем полученный IP-адрес
							awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip = this->_unit->addr.source(net_addr_t::endian_t::LITTLE);
						} break;
						// Для типа IPv6
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Создаём объект параметров подключения конечной точки
							this->_endpoint = make_unique <net::attr_net_t> ();
							// Устанавливаем тип параметров подключения конечной точки
							this->_endpoint->type = net::type_t::IPV6;
							// Устанавливаем полученный порт
							awh_cast <net::attr_net_t *> (this->_endpoint.get())->port = port;
							// Устанавливаем полученный IP-адрес
							awh_cast <net::attr_net_t *> (this->_endpoint.get())->ip = this->_unit->addr.source(net_addr_t::endian_t::LITTLE);
						} break;
					}
				// Если распарсить адрес не удалось, значит будем считать, что это FQDN
				} else {
					// Создаём объект параметров подключения конечной точки
					this->_endpoint = make_unique <net::attr_fqdn_t> ();
					// Устанавливаем тип параметров подключения конечной точки
					this->_endpoint->type = net::type_t::FQDN;
					// Устанавливаем полученный порт
					awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->port = port;
					// Устанавливаем полученное доменное имя хоста для подключения
					awh_cast <net::attr_fqdn_t *> (this->_endpoint.get())->domain = addr;
				}
				// Возвращаем результат наличия объекта атрибутов конечной точки
				return (this->_endpoint != nullptr);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Записываем ошибку в лог
				log::debug("%s", __PRETTY_FUNCTION__, {addr, port}, log::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log::print("%s", log::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Конструктор
 *
 */
awh::client::HttpProxy::HttpProxy() noexcept : HttpProxy(0, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param dns объект DNS-резолвера
 *
 */
awh::client::HttpProxy::HttpProxy(unit::dns_t * dns) noexcept : HttpProxy(0, dns) {}
/**
 * @brief Конструктор
 *
 * @param ctl идентификатор контекста безопасности TLS к прокси-серверу
 *
 */
awh::client::HttpProxy::HttpProxy(const tls::coder::id_t ctl) noexcept : HttpProxy(ctl, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param ctl идентификатор контекста безопасности TLS к прокси-серверу
 * @param dns объект DNS-резолвера
 *
 */
awh::client::HttpProxy::HttpProxy(const tls::coder::id_t ctl, unit::dns_t * dns) noexcept :
 client_t(ctl, dns), _ctl(0), _endpoint(nullptr) {
	// Подписываем кодек на выдачу тела ответа прокси прикладному обмену
	this->_proxy.on(http::parser_http_t::data_callback_t([this](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Вызываем функцию обратного вызова чтения данных
		this->_callback.call <void (const uint8_t *, const size_t)> ("read", static_cast <const uint8_t *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Подписываем кодек на выдачу головы ответа прокси прикладному обмену
	this->_proxy.on(http::proxy::client_t::response_callback_t([this](const uint16_t code, const vector <pair <string, string>> & headers) noexcept {
		// Вызываем функцию обратного вызова готовности головы ответа
		this->_callback.call <void (const uint16_t, const vector <pair <string, string>> &)> ("response", code, headers);
	}));
}
/**
 * @brief Деструктор
 *
 */
awh::client::HttpProxy::~HttpProxy() noexcept = default;
