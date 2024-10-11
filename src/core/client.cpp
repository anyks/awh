/**
 * @file: client.cpp
 * @date: 2024-03-09
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

// Подключаем заголовочный файл
#include <core/client.hpp>

/**
 * connect Метод создания подключения к удаленному серверу
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::connect(const uint16_t sid) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(i != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
		// Если подключение ещё не выполнено и выполнение работ разрешено
		if((shm->status.real == scheme_t::mode_t::DISCONNECT) && (shm->status.work == scheme_t::work_t::ALLOW)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Запрещаем выполнение работы
				shm->status.work = scheme_t::work_t::DISALLOW;
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем статус подключения
				shm->status.real = scheme_t::mode_t::PRECONNECT;
				// Получаем URL параметры запроса
				const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
				// Получаем семейство интернет-протоколов
				const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->_settings.family);
				// Если в схеме сети есть подключённые клиенты
				if(!shm->_brokers.empty()){
					// Переходим по всему списку брокера
					for(auto i = shm->_brokers.begin(); i != shm->_brokers.end();){
						// Если блокировка брокера не установлена
						if(this->_busy.find(i->first) == this->_busy.end()){
							// Создаём бъект активного брокера подключения
							awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (i->second.get());
							// Выполняем очистку буфера событий
							this->disable(i->first);
							// Выполняем очистку контекста двигателя
							broker->_ectx.clear();
							// Удаляем брокера из списка подключений
							this->_brokers.erase(i->first);
							// Удаляем брокера из списка
							i = shm->_brokers.erase(i);
						// Если есть хотябы один заблокированный элемент, выходим
						} else {
							// Устанавливаем статус подключения
							shm->status.real = scheme_t::mode_t::DISCONNECT;
							// Выходим из функции
							return;
						}
					}
				}
				// Создаём бъект активного брокера подключения
				std::unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
				// Устанавливаем время жизни подключения
				broker->_addr.alive = shm->keepAlive;
				// Выполняем установку времени ожидания входящих сообщений
				broker->_timeouts.wait = shm->timeouts.wait;
				// Устанавливаем таймаут начтение данных из сокета
				broker->timeout(shm->timeouts.read, engine_t::method_t::READ);
				// Устанавливаем таймаут на запись данных в сокет
				broker->timeout(shm->timeouts.write, engine_t::method_t::WRITE);
				// Устанавливаем таймаут на подключение к серверу
				broker->timeout(shm->timeouts.connect, engine_t::method_t::CONNECT);
				// Определяем тип протокола подключения
				switch(static_cast <uint8_t> (family)){
					// Если тип протокола подключения IPv4
					case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
						// Выполняем перебор всего списка адресов
						for(auto & host : this->_settings.network){
							// Если хост соответствует адресу IPv4
							if(this->_net.host(host) == net_t::type_t::IPV4)
								// Выполняем установку полученного хоста
								broker->_addr.network.push_back(host);
						}
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
						// Выполняем перебор всего списка адресов
						for(auto & host : this->_settings.network){
							// Если хост соответствует адресу IPv4
							if(this->_net.host(host) == net_t::type_t::IPV6)
								// Выполняем установку полученного хоста
								broker->_addr.network.push_back(host);
						}
					} break;
				}
				// Определяем тип сокета
				switch(static_cast <uint8_t> (this->_settings.sonet)){
					// Если тип сокета UDP
					case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
					// Если тип сокета DTLS
					case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
						// Устанавливаем параметры сокета
						broker->_addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					break;
					/**
					 * Если операционной системой является Linux или FreeBSD
					 */
					#if defined(__linux__) || defined(__FreeBSD__)
						// Если тип сокета установлен как SCTP
						case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
							// Устанавливаем параметры сокета
							broker->_addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
						break;
					#endif
					// Если тип сокета TCP
					case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
					// Если тип сокета TCP TLS
					case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
						// Устанавливаем параметры сокета
						broker->_addr.sonet(SOCK_STREAM, IPPROTO_TCP);
					break;
				}
				// Если unix-сокет используется
				if(family == scheme_t::family_t::NIX){
					// Если название unix-сокета ещё не инициализированно
					if(this->_settings.sockname.empty())
						// Выполняем установку названия unix-сокета
						this->sockname();
					// Выполняем инициализацию сокета
					broker->_addr.init(this->_fmk->format("%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()), engine_t::type_t::CLIENT);
				// Если unix-сокет не используется, выполняем инициализацию сокета
				} else broker->_addr.init(url.ip, url.port, (family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::CLIENT);
				// Если сокет подключения получен
				if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
					// Выполняем установку желаемого протокола подключения
					broker->_ectx.proto(this->_settings.proto);
					// Если подключение выполняется по защищённому каналу DTLS
					if(this->_settings.sonet == scheme_t::sonet_t::DTLS)
						// Выполняем получение контекста сертификата
						this->_engine.wrap(broker->_ectx, &broker->_addr, engine_t::type_t::CLIENT);
					// Если подключение выполняется не по защищённому каналу DTLS
					else {
						// Хост сервера для подклчюения
						const char * host = nullptr;
						// Если unix-сокет используется
						if(family == scheme_t::family_t::NIX)
							// Выполняем получение хост сервера
							host = url.host.c_str();
						// Если подключение производится по хосту и порту
						else host = (!url.domain.empty() ? url.domain.c_str() : (!url.ip.empty() ? url.ip.c_str() : nullptr));
						// Если хост сервера получен правильно
						if(host != nullptr){
							// Если функция обратного вызова активации шифрованного SSL канала установлена
							if(!shm->isProxy() && this->_callbacks.is("ssl"))
								// Выполняем активацию шифрованного SSL канала
								this->_engine.encrypted(this->_callbacks.call <bool (const uri_t::url_t &, const uint64_t, const uint16_t)> ("ssl", url, broker->id(), sid), broker->_ectx);
							// Выполняем активацию контекста подключения
							this->_engine.wrap(broker->_ectx, &broker->_addr, host);
						// Если хост сервера не получен
						} else {
							// Разрешаем выполнение работы
							shm->status.work = scheme_t::work_t::ALLOW;
							// Устанавливаем статус подключения
							shm->status.real = scheme_t::mode_t::DISCONNECT;
							// Выводим сообщение об ошибке
							this->_log->print("Connection server host is not set", log_t::flag_t::CRITICAL);
							// Если разрешено выводить информационные сообщения
							if(this->_verb)
								// Выводим сообщение об ошибке
								this->_log->print("Disconnected from server", log_t::flag_t::INFO);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, "Connection server host is not set");
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("disconnect"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const uint64_t, const uint16_t)> ("disconnect", 0, sid);
							// Выходим из функции
							return;
						}
					}
					// Если мы хотим работать в зашифрованном режиме
					if(!shm->isProxy() && (this->_settings.sonet == scheme_t::sonet_t::TLS)){
						// Если сертификаты не приняты, выходим
						if(!this->_engine.encrypted(broker->_ectx)){
							// Разрешаем выполнение работы
							shm->status.work = scheme_t::work_t::ALLOW;
							// Устанавливаем статус подключения
							shm->status.real = scheme_t::mode_t::DISCONNECT;
							// Выводим сообщение об ошибке
							this->_log->print("Encryption mode cannot be activated", log_t::flag_t::CRITICAL);
							// Если разрешено выводить информационные сообщения
							if(this->_verb)
								// Выводим сообщение об ошибке
								this->_log->print("Disconnected from server", log_t::flag_t::INFO);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, "Encryption mode cannot be activated");
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("disconnect"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const uint64_t, const uint16_t)> ("disconnect", 0, sid);
							// Выходим из функции
							return;
						}
					}
					// Если подключение не обёрнуто
					if((broker->_addr.fd == INVALID_SOCKET) || (broker->_addr.fd >= MAX_SOCKETS)){
						// Разрешаем выполнение работы
						shm->status.work = scheme_t::work_t::ALLOW;
						// Устанавливаем статус подключения
						shm->status.real = scheme_t::mode_t::DISCONNECT;
						// Устанавливаем флаг ожидания статуса
						shm->status.wait = scheme_t::mode_t::DISCONNECT;
						// Выводим сообщение об ошибке
						this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, "Wrap engine context is failed");
						// Выполняем переподключение
						this->reconnect(sid);
						// Выходим из функции
						return;
					}
					// Выполняем блокировку потока
					this->_mtx.connect.lock();
					// Выполняем установку базы событий
					broker->base(this->eventBase());
					// Добавляем созданного брокера в список брокеров
					auto ret = shm->_brokers.emplace(broker->id(), std::forward <std::unique_ptr <awh::scheme_t::broker_t>> (broker));
					// Добавляем брокера в список подключений
					this->_brokers.emplace(ret.first->first, ret.first->second.get());
					// Выполняем блокировку потока
					this->_mtx.connect.unlock();
					// Если подключение к серверу не выполнено
					if(!ret.first->second->_addr.connect()){
						// Разрешаем выполнение работы
						shm->status.work = scheme_t::work_t::ALLOW;
						// Устанавливаем статус подключения
						shm->status.real = scheme_t::mode_t::DISCONNECT;
						// Если unix-сокет используется
						if(family == scheme_t::family_t::NIX){
							// Выводим ионформацию об обрыве подключении по unix-сокету
							this->_log->print("Connecting to HOST=%s/%s.sock", log_t::flag_t::CRITICAL, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, this->_fmk->format("Connecting to HOST=%s/%s.sock", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()));
						// Если используется хост и порт
						} else {
							// Выводим ионформацию об обрыве подключении по хосту и порту
							this->_log->print("Connecting to HOST=%s, PORT=%u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, this->_fmk->format("Сonnecting to HOST=%s, PORT=%u", url.ip.c_str(), url.port));
						}
						// Если объект DNS-резолвера установлен
						if(this->_dns != nullptr){
							// Выполняем сброс кэша резолвера
							const_cast <dns_t *> (this->_dns)->flush();
							// Определяем тип подключения
							switch(static_cast <uint8_t> (family)){
								// Если тип протокола подключения IPv4
								case static_cast <uint8_t> (scheme_t::family_t::IPV4):
									// Добавляем бракованный IPv4 адрес в список адресов
									const_cast <dns_t *> (this->_dns)->setToBlackList(AF_INET, url.domain, url.ip); 
								break;
								// Если тип протокола подключения IPv6
								case static_cast <uint8_t> (scheme_t::family_t::IPV6):
									// Добавляем бракованный IPv6 адрес в список адресов
									const_cast <dns_t *> (this->_dns)->setToBlackList(AF_INET6, url.domain, url.ip);
								break;
							}
						}
						// Если доменный адрес установлен
						if(!url.domain.empty())
							// Выполняем очистку IP-адреса
							(shm->isProxy() ? shm->proxy.url.ip.clear() : shm->url.ip.clear());
						// Выполняем отключение от сервера
						this->close(ret.first->first);
						// Выходим из функции
						return;
					}
					// Получаем адрес подключения клиента
					ret.first->second->ip(url.ip);
					// Получаем порт подключения клиента
					ret.first->second->port(url.port);
					// Получаем аппаратный адрес клиента
					ret.first->second->mac(ret.first->second->_addr.mac);
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
					// Если статус подключения не изменился
					if(shm->status.real == scheme_t::mode_t::PRECONNECT){
						// Выполняем установку функции обратного вызова на получении сообщений
						ret.first->second->callback <void (const uint64_t)> ("read", std::bind(&core_t::read, this, _1));
						// Выполняем установку функции обратного вызова на отправку сообщений
						ret.first->second->callback <void (const uint64_t)> ("write", std::bind(static_cast <void (core_t::*)(const uint64_t)> (&core_t::write), this, _1));
						// Выполняем установку функции обратного вызова на получение сигнала закрытия подключения
						ret.first->second->callback <void (const uint64_t)> ("close", std::bind(static_cast <void (core_t::*)(const uint64_t)> (&core_t::close), this, _1));
						// Активируем ожидание подключения
						ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::WRITE);
						// Выполняем установку таймаута ожидания
						ret.first->second->_ectx.timeout(ret.first->second->_timeouts.connect * 1000, engine_t::method_t::READ);
						// Если разрешено выводить информационные сообщения
						if(this->_verb){
							// Если unix-сокет используется
							if(family == scheme_t::family_t::NIX)
								// Выводим ионформацию об удачном подключении к серверу по unix-сокету
								this->_log->print("Good host %s/%s.sock, SOCKET=%d", log_t::flag_t::INFO, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str(), ret.first->second->_addr.fd);
							// Выводим ионформацию об удачном подключении к серверу по хосту и порту
							else this->_log->print("Good host %s [%s:%d], SOCKET=%d", log_t::flag_t::INFO, url.domain.c_str(), url.ip.c_str(), url.port, ret.first->second->_addr.fd);
						}
					}
					// Выходим из функции
					return;
				// Если сокет не создан, выводим в консоль информацию
				} else {
					// Если unix-сокет используется
					if(family == scheme_t::family_t::NIX){
						// Выводим ионформацию об неудачном подключении к серверу по unix-сокету
						this->_log->print("Client cannot be started [%s/%s.sock]", log_t::flag_t::CRITICAL, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, this->_fmk->format("Client cannot be started [%s/%s.sock]", this->_settings.sockpath.c_str(), this->_settings.sockname.c_str()));
					// Если используется хост и порт
					} else {
						// Выводим ионформацию об неудачном подключении к серверу по хосту и порту
						this->_log->print("Client cannot be started [%s:%u]", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, this->_fmk->format("Client cannot be started [%s:%u]", url.ip.c_str(), url.port));
					}
				}
				// Если нужно выполнить автоматическое переподключение
				if(shm->alive){
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					shm->status.real = scheme_t::mode_t::DISCONNECT;
					// Устанавливаем флаг ожидания статуса
					shm->status.wait = scheme_t::mode_t::DISCONNECT;
					// Выполняем переподключение
					this->reconnect(sid);
					// Выходим из функции
					return;
				// Если все попытки исчерпаны
				} else {
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
					// Устанавливаем статус подключения
					shm->status.real = scheme_t::mode_t::DISCONNECT;
					// Если объект DNS-резолвера установлен
					if(this->_dns != nullptr){
						// Выполняем сброс кэша резолвера
						const_cast <dns_t *> (this->_dns)->flush();
						// Определяем тип подключения
						switch(static_cast <uint8_t> (family)){
							// Если тип протокола подключения IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4):
								// Добавляем бракованный IPv4 адрес в список адресов
								const_cast <dns_t *> (this->_dns)->setToBlackList(AF_INET, url.domain, url.ip); 
							break;
							// Если тип протокола подключения IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6):
								// Добавляем бракованный IPv6 адрес в список адресов
								const_cast <dns_t *> (this->_dns)->setToBlackList(AF_INET6, url.domain, url.ip);
							break;
						}
					}
					// Если разрешено выводить информационные сообщения
					if(this->_verb)
						// Выводим сообщение об ошибке
						this->_log->print("Disconnected from server", log_t::flag_t::INFO);
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("disconnect"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const uint16_t)> ("disconnect", 0, sid);
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const bad_alloc &) {
				// Выводим в лог сообщение
				this->_log->print("Client connect: %s", log_t::flag_t::CRITICAL, "memory allocation error");
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		}
	}
}
/**
 * reconnect Метод восстановления подключения
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::reconnect(const uint16_t sid) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(i != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
		// Если параметры URL запроса переданы и выполнение работы разрешено
		if(!shm->url.empty() && (shm->status.wait == scheme_t::mode_t::DISCONNECT) && (shm->status.work == scheme_t::work_t::ALLOW)){
			// Получаем семейство интернет-протоколов
			const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->_settings.family);
			// Определяем тип протокола подключения
			switch(static_cast <uint8_t> (family)){
				// Если тип протокола подключения IPv4
				case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Если тип протокола подключения IPv6
				case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
					// Устанавливаем флаг ожидания статуса
					shm->status.wait = scheme_t::mode_t::RECONNECT;
					// Получаем URL параметры запроса
					const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
					// Если IP-адрес не получен и объект DNS-резолвера установлен
					if(url.ip.empty() && !url.domain.empty() && (this->_dns != nullptr)){
						// Определяем тип протокола подключения
						switch(static_cast <uint8_t> (family)){
							// Если тип протокола подключения IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
								// Выполняем резолвинг домена
								const string & ip = const_cast <dns_t *> (this->_dns)->resolve(AF_INET, url.domain);
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, ip, AF_INET);
							} break;
							// Если тип протокола подключения IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
								// Выполняем резолвинг домена
								const string & ip = const_cast <dns_t *> (this->_dns)->resolve(AF_INET6, url.domain);
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, ip, AF_INET);
							} break;
						}
					// Выполняем запуск системы
					} else if(!url.ip.empty()) {
						// Определяем тип протокола подключения
						switch(static_cast <uint8_t> (family)){
							// Если тип протокола подключения IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4):
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, url.ip, AF_INET);
							break;
							// Если тип протокола подключения IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6):
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, url.ip, AF_INET6);
							break;
						}
					}
				} break;
				// Если тип протокола подключения unix-сокет
				case static_cast <uint8_t> (scheme_t::family_t::NIX):
					// Выполняем подключение заново
					this->connect(sid);
				break;
			}
		}
	}
}
/**
 * launching Метод вызова при активации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::client::Core::launching(const bool mode, const bool status) noexcept {
	// Выполняем функцию в базовом модуле
	node_t::launching(mode, status);
	// Если список схем сети существует
	if(mode && !this->_schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto & scheme : this->_schemes){
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("open"))
				// Устанавливаем полученную функцию обратного вызова
				callback.set <void (const uint16_t)> (scheme.first, this->_callbacks.get <void (const uint16_t)> ("open"), scheme.first);
		}
		// Выполняем все функции обратного вызова
		callback.bind();
	}
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::client::Core::closedown(const bool mode, const bool status) noexcept {
	// Выполняем функцию в базовом модуле
	node_t::closedown(mode, status);
	// Если требуется закрыть подключение
	if(mode)
		// Выполняем отключение всех брокеров
		this->close();
}
/**
 * timeout Метод вызова при срабатывании локального таймаута
 * @param sid  идентификатор схемы сети
 * @param mode режим работы клиента
 */
void awh::client::Core::timeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept {
	// Если идентификатор схемы сети найден
	if(this->has(sid)){
		// Определяем режим работы клиента
		switch(static_cast <uint8_t> (mode)){
			// Если режим работы клиента - это подключение
			case static_cast <uint8_t> (scheme_t::mode_t::CONNECT):
				// Выполняем новое подключение
				this->connect(sid);
			break;
			// Если режим работы клиента - это переподключение
			case static_cast <uint8_t> (scheme_t::mode_t::RECONNECT): {
				// Выполняем поиск идентификатора схемы сети
				auto i = this->_schemes.find(sid);
				// Если идентификатор схемы сети найден
				if(i != this->_schemes.end()){
					// Получаем объект схемы сети
					scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
					// Устанавливаем флаг ожидания статуса
					shm->status.wait = scheme_t::mode_t::DISCONNECT;
					// Выполняем новую попытку подключиться
					this->reconnect(sid);
				}
			} break;
		}
	}
}
/**
 * clearTimeout Метод удаления таймера ожидания получения данных
 * @param bid идентификатор брокера
 */
void awh::client::Core::clearTimeout(const uint64_t bid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx.receive);
	// Выполняем поиск активных таймаутов
	auto i = this->_receive.find(bid);
	// Если таймаут найден
	if(i != this->_receive.end()){
		// Выполняем удаление активных таймеров
		this->_timer.clear(i->second);
		// Удаляем таймаут из базы таймаутов
		this->_receive.erase(i);
	}
}
/**
 * clearTimeout Метод удаления таймера подключения или переподключения
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::clearTimeout(const uint16_t sid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx.timeout);
	// Выполняем поиск активных таймаутов
	auto i = this->_timeouts.find(sid);
	// Если таймаут найден
	if(i != this->_timeouts.end()){
		// Выполняем удаление активных таймеров
		this->_timer.clear(i->second);
		// Удаляем таймаут из базы таймаутов
		this->_timeouts.erase(i);
	}
}
/**
 * createTimeout Метод создания таймаута ожидания получения данных
 * @param bid  идентификатор брокера
 * @param msec время ожидания получения данных в миллисекундах
 */
void awh::client::Core::createTimeout(const uint64_t bid, const time_t msec) noexcept {
	// Если идентификатор брокера найден
	if(this->has(bid) && (msec > 0)){
		// Идентификатор таймера
		uint16_t tid = 0;
		// Выполняем поиск активных таймаутов
		auto i = this->_receive.find(bid);
		// Если таймаут найден
		if(i != this->_receive.end()){
			// Выполняем удаление активного таймера
			this->_timer.clear(i->second);
			// Выполняем создание нового таймаута
			i->second = tid = this->_timer.timeout(msec);
		// Если таймаут ещё не создан
		} else {
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx.receive);
			// Выполняем создание нового таймаута
			this->_receive.emplace(bid, (tid = this->_timer.timeout(msec)));
		}
		// Выполняем добавление функции обратного вызова
		this->_timer.set <void (const uint64_t)> (tid, std::bind(static_cast <void (core_t::*)(const uint64_t)> (&core_t::close), this, bid));
	}
}
/**
 * createTimeout Метод создания таймаута подключения или переподключения
 * @param sid  идентификатор схемы сети
 * @param mode режим работы клиента
 */
void awh::client::Core::createTimeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept {
	// Если идентификатор схемы сети найден
	if(this->has(sid)){
		// Идентификатор таймера
		uint16_t tid = 0;
		// Выполняем поиск активных таймаутов
		auto i = this->_timeouts.find(sid);
		// Если таймаут найден
		if(i != this->_timeouts.end()){
			// Выполняем удаление активного таймера
			this->_timer.clear(i->second);
			// Выполняем создание нового таймаута на 5 секунд
			i->second = tid = this->_timer.timeout(5000);
		// Если таймаут ещё не создан
		} else {
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx.timeout);
			// Выполняем создание нового таймаута на 5 секунд
			this->_timeouts.emplace(sid, (tid = this->_timer.timeout(5000)));
		}
		// Выполняем добавление функции обратного вызова
		this->_timer.set <void (const uint16_t, const scheme_t::mode_t)> (tid, std::bind(static_cast <void (core_t::*)(const uint16_t, const scheme_t::mode_t)> (&core_t::timeout), this, sid, mode));
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Core::stop() noexcept {
	// Если система уже запущена
	if(this->working()){
		// Выполняем закрытие подключения
		this->close();
		// Выполняем остановку работы сервера
		node_t::stop();
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::Core::start() noexcept {
	// Если система ещё не запущена
	if(!this->working())
		// Выполняем запуск работы сервера
		node_t::start();
}
/**
 * reset Метод принудительного сброса подключения
 * @param bid идентификатор брокера
 */
void awh::client::Core::reset(const uint64_t bid) noexcept {
	// Если блокировка брокера не установлена
	if(this->_busy.find(bid) == this->_busy.end()){
		// Если брокер существует
		if(this->has(bid))
			// Выполняем отключение от сервера
			this->close(bid);
		// Если брокер не существует
		else if(!this->_schemes.empty()) {
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx.reset);
			// Переходим по всему списку схем сети
			for(auto & item : this->_schemes){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (item.second));
				// Если выполнение работ разрешено
				if(shm->status.work == scheme_t::work_t::ALLOW)
					// Запрещаем выполнение работы
					shm->status.work = scheme_t::work_t::DISALLOW;
				// Если работы запрещены, выходим
				else return;
			}
			// Выполняем отключение всех подключённых брокеров
			this->close();
			// Выполняем пинок базе событий
			this->_dispatch.kick();
			// Переходим по всему списку схем сети
			for(auto & item : this->_schemes){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (item.second));
				// Устанавливаем статус подключения
				shm->status.real = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
			}
			// Переходим по всему списку схем сети
			for(auto & item : this->_schemes){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (item.second));
				// Если выполнение работ запрещено
				if(shm->status.work == scheme_t::work_t::DISALLOW)
					// Разрешаем выполнение работы
					shm->status.work = scheme_t::work_t::ALLOW;
				// Если нужно выполнить автоматическое переподключение
				if(shm->alive)
					// Выполняем переподключение
					this->reconnect(item.first);
			}
		}
	}
}
/**
 * disable Метод остановки активности брокера подключения
 * @param bid идентификатор брокера
 */
void awh::client::Core::disable(const uint64_t bid) noexcept {
	// Если брокер существует
	if(this->has(bid)){
		// Выполняем извлечение брокера подключений
		const scheme_t::broker_t * broker = this->broker(bid);
		// Останавливаем событие чтение данных
		const_cast <scheme_t::broker_t *> (broker)->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
		// Останавливаем событие запись данных
		const_cast <scheme_t::broker_t *> (broker)->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
	}
}
/**
 * close Метод отключения всех брокеров
 */
void awh::client::Core::close() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock1(this->_mtx.close);
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock2(node_t::_mtx.main);
	const lock_guard <std::recursive_mutex> lock3(node_t::_mtx.send);
	// Останавливаем работу таймера
	this->_timer.clear();
	{
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx.timeout);
		// Очищаем список таймаутов
		this->_timeouts.clear();
	}{
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx.receive);
		// Очищаем список таймаутов ожидания получения данных
		this->_receive.clear();
	}
	// Если список схем сети активен
	if(!this->_schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto & item : this->_schemes){
			// Если в схеме сети есть подключённые клиенты
			if(!item.second->_brokers.empty()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (item.second));
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем статус сетевого ядра
				shm->status.real = scheme_t::mode_t::DISCONNECT;
				// Если прокси-сервер активирован но уже переключён на работу с сервером
				if((shm->proxy.type != proxy_t::type_t::NONE) && !shm->isProxy())
					// Выполняем переключение обратно на прокси-сервер
					shm->switchConnect();
				// Переходим по всему списку брокера
				for(auto i = shm->_brokers.begin(); i != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_busy.find(i->first) == this->_busy.end()){
						// Выполняем блокировку брокера
						this->_busy.emplace(i->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (i->second.get());
						// Выполняем очистку буфера событий
						this->disable(i->first);
						// Выполняем очистку контекста двигателя
						broker->_ectx.clear();
						// Удаляем брокера из списка подключений
						this->_brokers.erase(i->first);
						// Ещем для указанного потока очередь полезной нагрузки
						auto j = this->_payloads.find(i->first);
						// Если для потока очередь полезной нагрузки получена
						if(j != this->_payloads.end())
							// Выполняем удаление всей очереди
							this->_payloads.erase(j);
						// Ищем для указанного брокера список используемой памяти полезной нагрузки
						auto k = this->_available.find(i->first);
						// Если для брокера размер используемой памяти полезной нагрузки получен
						if(k != this->_available.end())
							// Выполняем удаление записи используемой памяти полезной нагрузки
							this->_available.erase(k);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t)> (i->first, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), i->first, item.first);
						// Удаляем блокировку брокера
						this->_busy.erase(i->first);
						// Удаляем брокера из списка
						i = shm->_brokers.erase(i);
					// Иначе продолжаем дальше
					} else ++i;
				}
			}
		}
		// Выполняем все функции обратного вызова
		callback.bind();
	}
}
/**
 * remove Метод удаления всех схем сети
 */
void awh::client::Core::remove() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock1(this->_mtx.close);
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock2(node_t::_mtx.main);
	const lock_guard <std::recursive_mutex> lock3(node_t::_mtx.send);
	// Останавливаем работу таймера
	this->_timer.clear();
	{
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx.timeout);
		// Очищаем список таймаутов
		this->_timeouts.clear();
	}{
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx.receive);
		// Очищаем список таймаутов ожидания получения данных
		this->_receive.clear();
	}
	// Если список схем сети активен
	if(!this->_schemes.empty()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Переходим по всему списку схем сети
		for(auto i = this->_schemes.begin(); i != this->_schemes.end();){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Устанавливаем флаг ожидания статуса
			shm->status.wait = scheme_t::mode_t::DISCONNECT;
			// Устанавливаем статус сетевого ядра
			shm->status.real = scheme_t::mode_t::DISCONNECT;
			// Если в схеме сети есть подключённые клиенты
			if(!shm->_brokers.empty()){
				// Переходим по всему списку брокера
				for(auto j = shm->_brokers.begin(); j!= shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_busy.find(j->first) == this->_busy.end()){
						// Выполняем блокировку брокера
						this->_busy.emplace(j->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
						// Выполняем очистку буфера событий
						this->disable(j->first);
						// Выполняем очистку контекста двигателя
						broker->_ectx.clear();
						// Удаляем брокера из списка подключений
						this->_brokers.erase(j->first);
						// Ещем для указанного потока очередь полезной нагрузки
						auto k = this->_payloads.find(j->first);
						// Если для потока очередь полезной нагрузки получена
						if(k != this->_payloads.end())
							// Выполняем удаление всей очереди
							this->_payloads.erase(k);
						// Ищем для указанного брокера список используемой памяти полезной нагрузки
						auto l = this->_available.find(j->first);
						// Если для брокера размер используемой памяти полезной нагрузки получен
						if(l != this->_available.end())
							// Выполняем удаление записи используемой памяти полезной нагрузки
							this->_available.erase(l);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t)> (j->first, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), j->first, i->first);
						// Удаляем блокировку брокера
						this->_busy.erase(j->first);
						// Удаляем брокера из списка
						j = shm->_brokers.erase(j);
					// Иначе продолжаем дальше
					} else ++j;
				}
			}
			// Выполняем удаление схемы сети
			i = this->_schemes.erase(i);
		}
		// Выполняем все функции обратного вызова
		callback.bind();
	}
}
/**
 * open Метод открытия подключения
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::open(const uint16_t sid) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(i != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
		// Если параметры URL запроса переданы и выполнение работы разрешено
		if(!shm->url.empty() && (shm->status.wait == scheme_t::mode_t::DISCONNECT) && (shm->status.work == scheme_t::work_t::ALLOW)){
			// Получаем семейство интернет-протоколов
			const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->_settings.family);
			// Определяем тип протокола подключения
			switch(static_cast <uint8_t> (family)){
				// Если тип протокола подключения IPv4
				case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Если тип протокола подключения IPv6
				case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
					// Устанавливаем флаг ожидания статуса
					shm->status.wait = scheme_t::mode_t::CONNECT;
					// Получаем URL параметры запроса
					const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
					// Если IP-адрес не получен и объект DNS-резолвера установлен
					if(url.ip.empty() && !url.domain.empty() && (this->_dns != nullptr)){
						// Определяем тип протокола подключения
						switch(static_cast <uint8_t> (this->_settings.family)){
							// Если тип протокола подключения IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
								// Выполняем резолвинг домена
								const string & ip = const_cast <dns_t *> (this->_dns)->resolve(AF_INET, url.domain);
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, ip, AF_INET);
							} break;
							// Если тип протокола подключения IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
								// Выполняем резолвинг домена
								const string & ip = const_cast <dns_t *> (this->_dns)->resolve(AF_INET6, url.domain);
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, ip, AF_INET);
							} break;
						}
					// Выполняем запуск системы
					} else if(!url.ip.empty()) {
						// Определяем тип протокола подключения
						switch(static_cast <uint8_t> (this->_settings.family)){
							// Если тип протокола подключения IPv4
							case static_cast <uint8_t> (scheme_t::family_t::IPV4):
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, url.ip, AF_INET);
							break;
							// Если тип протокола подключения IPv6
							case static_cast <uint8_t> (scheme_t::family_t::IPV6):
								// Выполняем подключения к полученному IP-адресу
								this->work(sid, url.ip, AF_INET6);
							break;
						}
					}
				} break;
				// Если тип протокола подключения unix-сокет
				case static_cast <uint8_t> (scheme_t::family_t::NIX): {
					// Если требуется подключение через прокси-сервер
					if(shm->isProxy())
						// Создаём unix-сокет
						this->sockname(shm->proxy.url.host);
					// Выполняем подключение заново
					this->connect(sid);
				} break;
			}
		}
	}
}
/**
 * close Метод закрытия подключения
 * @param bid идентификатор брокера
 */
void awh::client::Core::close(const uint64_t bid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx.close);
	// Если блокировка брокера не установлена
	if(this->_busy.find(bid) == this->_busy.end()){
		// Выполняем блокировку брокера
		this->_busy.emplace(bid);
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Если идентификатор брокера подключений существует
		if(this->has(bid)){
			// Создаём бъект активного брокера подключения
			awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
			// Если таймер ожидания получения данных установлен
			if(broker->_timeouts.read > 0)
				// Выполняем удаление таймаута
				this->clearTimeout(bid);
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Выполняем очистку буфера событий
				this->disable(bid);
				// Выполняем удаление таймаута
				this->clearTimeout(i->first);
				// Если прокси-сервер активирован но уже переключён на работу с сервером
				if((shm->proxy.type != proxy_t::type_t::NONE) && !shm->isProxy())
					// Выполняем переключение обратно на прокси-сервер
					shm->switchConnect();
				// Выполняем очистку контекста двигателя
				broker->_ectx.clear();
				// Выполняем удаление параметров активного брокера
				node_t::remove(bid);
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем статус сетевого ядра
				shm->status.real = scheme_t::mode_t::DISCONNECT;
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("disconnect"))
					// Устанавливаем полученную функцию обратного вызова
					callback.set <void (const uint64_t, const uint16_t)> ("disconnect", this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), bid, i->first);
				// Если активированно постоянное подключение
				if(shm->alive)
					// Устанавливаем функцию обратного вызова
					callback.set <void (const uint16_t)> ("reconnect", std::bind(&core_t::reconnect, this, _1), i->first);
			}
		}
		// Удаляем блокировку брокера
		this->_busy.erase(bid);
		// Если функция дисконнекта установлена
		if(callback.is("disconnect")){
			// Если разрешено выводить информационные сообщения
			if(this->_verb)
				// Выводим сообщение об ошибке
				this->_log->print("Disconnected from server", log_t::flag_t::INFO);
			// Выполняем функцию обратного вызова дисконнекта
			callback.bind("disconnect");
			// Если функция реконнекта установлена
			if(callback.is("reconnect"))
				// Выполняем автоматическое переподключение
				callback.bind("reconnect");
		}
	}
}
/**
 * remove Метод удаления схемы сети
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::remove(const uint16_t sid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock1(this->_mtx.close);
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock2(node_t::_mtx.main);
	const lock_guard <std::recursive_mutex> lock3(node_t::_mtx.send);
	// Выполняем поиск схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(i != this->_schemes.end()){
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
		// Выполняем удаление таймаута
		this->clearTimeout(i->first);
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
		// Устанавливаем флаг ожидания статуса
		shm->status.wait = scheme_t::mode_t::DISCONNECT;
		// Устанавливаем статус сетевого ядра
		shm->status.real = scheme_t::mode_t::DISCONNECT;
		// Если в схеме сети есть подключённые клиенты
		if(!shm->_brokers.empty()){
			// Переходим по всему списку брокера
			for(auto j = shm->_brokers.begin(); j != shm->_brokers.end();){
				// Если таймер ожидания получения данных установлен
				if(j->second->_timeouts.read > 0)
					// Выполняем удаление таймаута
					this->clearTimeout(j->first);
				// Если блокировка брокера не установлена
				if(this->_busy.find(j->first) == this->_busy.end()){
					// Выполняем блокировку брокера
					this->_busy.emplace(j->first);
					// Получаем бъект активного брокера подключения
					awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (j->second.get());
					// Выполняем очистку буфера событий
					this->disable(j->first);
					// Выполняем очистку контекста двигателя
					broker->_ectx.clear();
					// Удаляем брокера из списка подключений
					this->_brokers.erase(j->first);
					// Ещем для указанного потока очередь полезной нагрузки
					auto k = this->_payloads.find(j->first);
					// Если для потока очередь полезной нагрузки получена
					if(k != this->_payloads.end())
						// Выполняем удаление всей очереди
						this->_payloads.erase(k);
					// Ищем для указанного брокера список используемой памяти полезной нагрузки
					auto l = this->_available.find(j->first);
					// Если для брокера размер используемой памяти полезной нагрузки получен
					if(l != this->_available.end())
						// Выполняем удаление записи используемой памяти полезной нагрузки
						this->_available.erase(l);
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("disconnect"))
						// Устанавливаем полученную функцию обратного вызова
						callback.set <void (const uint64_t, const uint16_t)> (j->first, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), j->first, i->first);
					// Удаляем блокировку брокера
					this->_busy.erase(j->first);
					// Удаляем брокера из списка
					j = shm->_brokers.erase(j);
				// Иначе продолжаем дальше
				} else ++j;
			}
		}
		// Выполняем удаление уоркера из списка
		this->_schemes.erase(i);
		// Выполняем все функции обратного вызова
		callback.bind();
	}
}
/**
 * switchProxy Метод переключения с прокси-сервера
 * @param bid идентификатор брокера
 */
void awh::client::Core::switchProxy(const uint64_t bid) noexcept {
	// Определяем тип производимого подключения
	switch(static_cast <uint8_t> (this->_settings.sonet)){
		// Если подключение производится по протоколу TCP
		case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
		// Если подключение производится по протоколу TLS
		case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
		// Если подключение производится по протоколу SCTP
		case static_cast <uint8_t> (scheme_t::sonet_t::SCTP): break;
		// Если активирован любой другой протокол, выходим из функции
		default: return;
	}
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx.proxy);
	// Если идентификатор брокера подключений существует
	if(this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(broker->sid());
		// Если идентификатор схемы сети найден
		if(i != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Если прокси-сервер активирован но ещё не переключён на работу с сервером
			if((shm->proxy.type != proxy_t::type_t::NONE) && shm->isProxy()){
				// Выполняем переключение на работу с сервером
				shm->switchConnect();
				// Хост сервера для подклчюения
				const char * host = nullptr;
				// Если unix-сокет используется
				if(shm->proxy.family == scheme_t::family_t::NIX)
					// Выполняем получение хост сервера
					host = shm->url.host.c_str();
				// Если подключение производится по хосту и порту
				else host = (!shm->url.domain.empty() ? shm->url.domain.c_str() : (!shm->url.ip.empty() ? shm->url.ip.c_str() : nullptr));
				// Если хост сервера получен правильно
				if(host != nullptr){
					// Выполняем установку желаемого протокола подключения
					broker->_ectx.proto(this->_settings.proto);
					// Если функция обратного вызова активации шифрованного SSL канала установлена
					if(this->_callbacks.is("ssl"))
						// Выполняем активацию шифрованного SSL канала
						this->_engine.encrypted(this->_callbacks.call <bool (const uri_t::url_t &, const uint64_t, const uint16_t)> ("ssl", shm->url, bid, i->first), broker->_ectx);
					// Выполняем получение контекста сертификата
					this->_engine.wrap(broker->_ectx, broker->_ectx, host);
					// Если подключение не обёрнуто
					if((broker->_addr.fd == INVALID_SOCKET) || (broker->_addr.fd >= MAX_SOCKETS)){
						// Выводим сообщение об ошибке
						this->_log->print("Wrap engine context is failed", log_t::flag_t::CRITICAL);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, "Wrap engine context is failed");
						// Выходим из функции
						return;
					}
				// Если хост сервера не получен
				} else {
					// Выводим сообщение об ошибке
					this->_log->print("Connection server host is not set", log_t::flag_t::CRITICAL);
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, "Connection server host is not set");
					// Выходим из функции
					return;
				}
				// Останавливаем чтение данных
				broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
				// Останавливаем запись данных
				broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
			}
		}
	}
}
/**
 * connected Метод вызова при удачном подключении к серверу
 * @param bid идентификатор брокера
 */
void awh::client::Core::connected(const uint64_t bid) noexcept {
	// Если идентификатор брокера подключений существует
	if(this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Выполняем поиск идентификатора схемы сети
		auto i = this->_schemes.find(broker->sid());
		// Если идентификатор схемы сети найден
		if(i != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
			// Если подключение удачное и работа разрешена
			if(shm->status.work == scheme_t::work_t::ALLOW){
				// Снимаем флаг получения данных
				shm->receiving = false;
				// Устанавливаем статус подключения к серверу
				shm->status.real = scheme_t::mode_t::CONNECT;
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Выполняем удаление таймаута
				this->clearTimeout(i->first);
				// Получаем семейство интернет-протоколов
				const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->_settings.family);
				// Определяем тип протокола подключения
				switch(static_cast <uint8_t> (family)){
					// Если тип протокола подключения IPv4
					case static_cast <uint8_t> (scheme_t::family_t::IPV4): {
						// Получаем URL параметры запроса
						const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
						// Получаем хост сервера
						const string & host = (!url.ip.empty() ? url.ip : url.domain);
						// Если объект DNS-резолвера установлен
						if(this->_dns != nullptr)
							// Выполняем отмену ранее выполненных запросов DNS
							const_cast <dns_t *> (this->_dns)->cancel(AF_INET);
						// Активируем чтение данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
						// Деактивируем ожидание записи данных
						broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
						// Выполняем создание буфера полезной нагрузки
						this->createBuffer(broker->id());
						// Если разрешено выводить информационные сообщения
						if(this->_verb)
							// Выводим в лог сообщение
							this->_log->print("Connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
					} break;
					// Если тип протокола подключения IPv6
					case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
						// Получаем URL параметры запроса
						const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
						// Получаем хост сервера
						const string & host = (!url.ip.empty() ? url.ip : url.domain);
						// Если объект DNS-резолвера установлен
						if(this->_dns != nullptr)
							// Выполняем отмену ранее выполненных запросов DNS
							const_cast <dns_t *> (this->_dns)->cancel(AF_INET6);
						// Активируем чтение данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
						// Деактивируем ожидание записи данных
						broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
						// Выполняем создание буфера полезной нагрузки
						this->createBuffer(broker->id());
						// Если разрешено выводить информационные сообщения
						if(this->_verb)
							// Выводим в лог сообщение
							this->_log->print("Connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
					} break;
					// Если тип протокола подключения unix-сокет
					case static_cast <uint8_t> (scheme_t::family_t::NIX): {
						// Активируем чтение данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
						// Деактивируем ожидание записи данных
						broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
						// Выполняем создание буфера полезной нагрузки
						this->createBuffer(broker->id());
						// Если разрешено выводить информационные сообщения
						if(this->_verb)
							// Выводим в лог сообщение
							this->_log->print("Connect client to server [%s/%s.sock]", log_t::flag_t::INFO, this->_settings.sockpath.c_str(), this->_settings.sockname.c_str());
					} break;
				}
				// Если подключение производится через, прокси-сервер
				if(shm->isProxy()){
					// Если функция обратного вызова для прокси-сервера установлена
					if(this->_callbacks.is("connectProxy"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const uint16_t)> ("connectProxy", bid, i->first);
				// Если функция обратного вызова установлена
				} else if(this->_callbacks.is("connect"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const uint16_t)> ("connect", bid, i->first);
				// Выходим из функции
				return;
			}
			// Выполняем отключение от сервера
			this->close(bid);
		}
	}
}
/**
 * send Метод асинхронной отправки буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 * @return       результат отправки сообщения
 */
bool awh::client::Core::send(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(this->working() && this->has(bid)){
		// Флаг активация ожидания готовности сокета
		bool waiting = false;
		// Определяем режим отправки сообщений
		switch(static_cast <uint8_t> (this->_sending)){
			// Если установлен флаг отправки отложенных сообщений
			case static_cast <uint8_t> (sending_t::DEFFER):
				// Выполняем отправку сообщения асинхронным методом
				waiting = result = node_t::send(buffer, size, bid);
			break;
			// Если установлен флаг отправки мгновенных сообщений
			case static_cast <uint8_t> (sending_t::INSTANT): {
				// Ещем для указанного потока очередь полезной нагрузки
				auto i = this->_payloads.find(bid);
				// Если для потока очередь полезной нагрузки получена
				if((waiting = ((i != this->_payloads.end()) && !i->second.empty() && ((i->second.front().offset - i->second.front().pos) > 0))))
					// Выполняем отправку сообщения асинхронным методом
					result = node_t::send(buffer, size, bid);
				// Если очередь ещё не существует
				else {
					// Выполняем отправку данных клиенту
					const size_t bytes = this->write(buffer, size, bid);
					// Если данные отправлены не полностью
					if((waiting = (bytes < size)))
						// Выполняем отправку сообщения асинхронным методом
						result = node_t::send(buffer + bytes, size - bytes, bid);
					// Если все данные добавлены успешно
					else result = (bytes == size);
				}
			} break;
		}
		// Если необходимо активировать ожидание готовности сокета для записи
		if(waiting && this->has(bid)){
			// Создаём бъект активного брокера подключения
			awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
			// Если сокет подключения активен
			if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS))
				// Запускаем ожидание записи данных
				broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::WRITE);
		}
	}
	// Сообщаем, что отправить сообщение неудалось
	return result;
}
/**
 * read Метод чтения данных для брокера
 * @param bid идентификатор брокера
 */
void awh::client::Core::read(const uint64_t bid) noexcept {
	// Если данные переданы
	if(this->working() && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Если подключение установлено
				if((shm->receiving = (shm->status.real == scheme_t::mode_t::CONNECT))){
					// Выполняем отключение приёма данных на этот сокет
					broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
					/**
					 * Выполняем чтение данных с сокета
					 */
					do {
						// Если подключение выполнено и чтение данных разрешено
						if((broker->_payload.size > 0) && (shm->status.real == scheme_t::mode_t::CONNECT)){
							// Выполняем получение сообщения от клиента
							const int64_t bytes = broker->_ectx.read(broker->_payload.data.get(), broker->_payload.size);
							// Если данные получены
							if(bytes > 0){
								// Если таймер ожидания получения данных установлен
								if((broker->_timeouts.wait > 0) || (broker->_timeouts.read > 0))
									// Выполняем удаление таймаута
									this->clearTimeout(bid);
								// Если подключение производится через, прокси-сервер
								if(shm->isProxy()){
									// Если функция обратного вызова для вывода записи существует
									if(this->_callbacks.is("readProxy"))
										// Выводим функцию обратного вызова
										this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("readProxy", broker->_payload.data.get(), static_cast <size_t> (bytes), bid, i->first);
								// Если прокси-сервер не используется
								} else if(this->_callbacks.is("read"))
									// Выводим функцию обратного вызова
									this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", broker->_payload.data.get(), static_cast <size_t> (bytes), bid, i->first);
							// Если данные небыли получены
							} else if(bytes <= 0) {
								// Если чтение не выполнена, закрываем подключение
								if(bytes == 0)
									// Выполняем закрытие подключения
									this->close(bid);
								// Выходим из цикла
								break;
							}
						// Если запись не выполнена, входим
						} else break;
					// Выполняем чтение до тех пор, пока всё не прочитаем
					} while(this->has(bid));
					// Если подключение ещё не разорванно
					if(this->has(bid)){
						// Если время ожиданий входящих сообщений установлено
						if(broker->_timeouts.wait > 0)
							// Выполняем создание таймаута ожидания получения данных
							this->createTimeout(bid, broker->_timeouts.wait * 1000);
						// Выполняем активацию отслеживания получения данных с этого сокета
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
					}
				// Если подключение завершено
				} else {
					// Останавливаем чтение данных
					broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
					// Останавливаем запись данных
					broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
					// Выполняем отключение от сервера
					this->close(bid);
				}
			// Если схема сети не существует
			} else {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Connection Broker %llu does not belong to a non-existent network diagram", log_t::flag_t::CRITICAL, bid);
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, this->_fmk->format("Connection Broker %llu does not belong to a non-existent network diagram", bid));
			}
		// Если файловый дескриптор сломан, значит с памятью что-то не то
		} else if(broker->_addr.fd > 65535) {
			// Удаляем из памяти объект брокера
			node_t::remove(bid);
			// Выводим в лог сообщение
			this->_log->print("Socket for read is not initialized", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::PROTOCOL, "Socket for read is not initialized");
		}
	}
}
/**
 * write Метод записи данных в брокер
 * @param bid идентификатор брокера
 */
void awh::client::Core::write(const uint64_t bid) noexcept {
	// Если данные переданы
	if(this->working() && this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Останавливаем детектирования возможности записи в сокет
			broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Если статус подключения не изменился
				if(shm->status.real == scheme_t::mode_t::PRECONNECT)
					// Выполняем запуск подключения
					this->connected(bid);
				// Если подключение уже выполненно
				else {
					// Ещем для указанного потока очередь полезной нагрузки
					auto i = this->_payloads.find(bid);
					// Если для потока очередь полезной нагрузки получена
					if((i != this->_payloads.end()) && !i->second.empty()){
						// Если есть данные для отправки
						if((i->second.front().offset - i->second.front().pos) > 0){
							// Выполняем запись в сокет
							const size_t bytes = this->write(i->second.front().data.get() + i->second.front().pos, i->second.front().offset - i->second.front().pos, bid);
							// Если данные записаны удачно
							if((bytes > 0) && this->has(bid)){
								// Увеличиваем смещение в бинарном буфере
								i->second.front().pos += bytes;
								// Если все данные записаны успешно, тогда удаляем результат
								if(i->second.front().pos == i->second.front().offset)
									// Выполняем освобождение памяти хранения полезной нагрузки
									this->available(bid);
							}
							// Если опередей полезной нагрузки нет, отключаем событие ожидания записи
							if(this->_payloads.find(bid) != this->_payloads.end()){
								// Если сокет подключения активен
								if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS))
									// Запускаем ожидание записи данных
									broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::WRITE);
							}
						// Если данных для отправки больше нет и сокет подключения активен
						} else if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS))
							// Останавливаем детектирования возможности записи в сокет
							broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
					}
				}
			// Если схема сети не существует
			} else {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Connection Broker %llu does not belong to a non-existent network diagram", log_t::flag_t::CRITICAL, bid);
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, this->_fmk->format("Connection Broker %llu does not belong to a non-existent network diagram", bid));
			}
		// Если файловый дескриптор сломан, значит с памятью что-то не то
		} else if(broker->_addr.fd > 65535) {
			// Удаляем из памяти объект брокера
			node_t::remove(bid);
			// Выводим в лог сообщение
			this->_log->print("Socket for write is not initialized", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::PROTOCOL, "Socket for write is not initialized");
		}
	}
}
/**
 * write Метод записи буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 * @return       количество отправленных байт
 */
size_t awh::client::Core::write(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные переданы
	if(this->working() && this->has(bid) && (buffer != nullptr) && (size > 0)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Останавливаем детектирования возможности записи в сокет
			broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
			// Выполняем поиск идентификатора схемы сети
			auto i = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(i != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
				// Если подключение установлено
				if((shm->receiving = (shm->status.real == scheme_t::mode_t::CONNECT))){
					// Определяем тип сокета
					switch(static_cast <uint8_t> (this->_settings.sonet)){
						// Если тип сокета установлен как TCP/IP
						case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
						// Если тип сокета установлен как TCP/IP TLS
						case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
						// Если тип сокета установлен как SCTP
						case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
							// Переводим сокет в блокирующий режим
							broker->_ectx.blocking(engine_t::mode_t::ENABLED);
						break;
					}
					// Получаем максимальный размер буфера
					const int32_t max = broker->_ectx.buffer(engine_t::method_t::WRITE);
					// Если в буфере нет места
					if(max > 0){
						// Определяем тип сокета
						switch(static_cast <uint8_t> (this->_settings.sonet)){
							// Если тип сокета установлен как TCP/IP
							case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
							// Если тип сокета установлен как TCP/IP TLS
							case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
							// Если тип сокета установлен как UDP
							case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
							// Если тип сокета установлен как DTLS
							case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
							// Если тип сокета установлен как SCTP
							case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
								// Выполняем установку таймаута ожидания записи в сокет
								broker->_ectx.timeout(broker->_timeouts.write * 1000, engine_t::method_t::WRITE);
							break;
						}
						// Выполняем отправку сообщения клиенту
						const int64_t bytes = broker->_ectx.write(buffer, (size >= static_cast <size_t> (max) ? static_cast <size_t> (max) : size));
						// Если данные удачно отправленны
						if(bytes > 0){
							// Запоминаем количество записанных байт
							result = static_cast <size_t> (bytes);
							// Если таймер ожидания получения данных установлен
							if(broker->_timeouts.read > 0)
								// Выполняем создание таймаута ожидания получения данных
								this->createTimeout(bid, broker->_timeouts.read * 1000);
						// Если запись не выполнена, закрываем подключение
						} else if(bytes == 0)
							// Выполняем закрытие подключения
							this->close(bid);
						// Если дисконнекта не произошло
						if(bytes != 0){
							// Определяем тип сокета
							switch(static_cast <uint8_t> (this->_settings.sonet)){
								// Если тип сокета установлен как TCP/IP
								case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
								// Если тип сокета установлен как TCP/IP TLS
								case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
								// Если тип сокета установлен как SCTP
								case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
									// Переводим сокет в неблокирующий режим
									broker->_ectx.blocking(engine_t::mode_t::DISABLED);
								break;
							}
						}
						// Если данные удачно отправленны
						if(bytes > 0){
							// Если подключение производится через, прокси-сервер
							if(shm->isProxy()){
								// Если функция обратного вызова для вывода записи существует
								if(this->_callbacks.is("writeProxy"))
									// Выводим функцию обратного вызова
									this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("writeProxy", buffer, static_cast <size_t> (bytes), bid, i->first);
							// Если функция обратного вызова на запись данных установлена
							} else if(this->_callbacks.is("write"))
								// Выводим функцию обратного вызова
								this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", buffer, static_cast <size_t> (bytes), bid, i->first);
						}
					}
				// Если подключение завершено
				} else {
					// Останавливаем чтение данных
					broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
					// Останавливаем запись данных
					broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
					// Выполняем отключение от сервера
					this->close(bid);
				}
			// Если схема сети не существует
			} else {
				// Выводим сообщение в лог, о таймауте подключения
				this->_log->print("Connection Broker %llu does not belong to a non-existent network diagram", log_t::flag_t::CRITICAL, bid);
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::PROTOCOL, this->_fmk->format("Connection Broker %llu does not belong to a non-existent network diagram", bid));
			}
		// Если файловый дескриптор сломан, значит с памятью что-то не то
		} else if(broker->_addr.fd > 65535) {
			// Удаляем из памяти объект брокера
			node_t::remove(bid);
			// Выводим в лог сообщение
			this->_log->print("Socket for write is not initialized", log_t::flag_t::WARNING);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::PROTOCOL, "Socket for write is not initialized");
		}
	}
	// Выводим результат
	return result;
}
/**
 * work Метод запуска работы подключения клиента
 * @param sid    идентификатор схемы сети
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::client::Core::work(const uint16_t sid, const string & ip, const int32_t family) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(i != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (i->second));
		// Если IP-адрес получен
		if(!ip.empty()){
			// Если прокси-сервер активен
			if(shm->isProxy()){
				// Запоминаем полученный IP-адрес для прокси-сервера
				shm->proxy.url.ip = ip;
				// Устанавливаем тип интернет-протокола AF_INET, AF_INET6
				shm->proxy.url.family = family;
			// Если прокси-сервер не активен
			} else {
				// Запоминаем полученный IP-адрес
				shm->url.ip = ip;
				// Устанавливаем тип интернет-протокола AF_INET, AF_INET6
				shm->url.family = family;
			}
			// Определяем режим работы клиента
			switch(static_cast <uint8_t> (shm->status.wait)){
				// Если режим работы клиента - это подключение
				case static_cast <uint8_t> (scheme_t::mode_t::CONNECT):
					// Выполняем новое подключение к серверу
					this->connect(sid);
				break;
				// Если режим работы клиента - это переподключение
				case static_cast <uint8_t> (scheme_t::mode_t::RECONNECT):
					// Выполняем ещё одну попытку переподключиться к серверу
					this->createTimeout(sid, scheme_t::mode_t::CONNECT);
				break;
			}
			// Выходим из функции
			return;
		// Если IP-адрес не получен но нужно поддерживать постоянное подключение
		} else if(shm->alive) {
			// Если ожидание переподключения не остановлено ранее
			if(shm->status.wait != scheme_t::mode_t::DISCONNECT)
				// Выполняем ещё одну попытку переподключиться к серверу
				this->createTimeout(sid, scheme_t::mode_t::RECONNECT);
			// Выходим из функции, чтобы попытаться подключиться ещё раз
			return;
		}
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("disconnect"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const uint64_t, const uint16_t)> ("disconnect", 0, sid);
	}
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param bid идентификатор брокера
 * @param sec интервал времени в секундах
 */
void awh::client::Core::waitMessage(const uint64_t bid, const time_t sec) noexcept {
	// Если идентификатор брокера подключения передан
	if(bid > 0){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx.receive);
		// Выполняем удаление таймаута
		this->clearTimeout(bid);
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS))
			// Выполняем установку времени ожидания входящих сообщений
			broker->_timeouts.wait = sec;
	}
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param bid     идентификатор брокера
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::Core::waitTimeDetect(const uint64_t bid, const time_t read, const time_t write, const time_t connect) noexcept {
	// Если идентификатор брокера подключения передан
	if(bid > 0){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx.receive);
		// Выполняем удаление таймаута
		this->clearTimeout(bid);
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Устанавливаем количество секунд на чтение
			broker->_timeouts.read = read;
			// Устанавливаем количество секунд на запись
			broker->_timeouts.write = write;
			// Устанавливаем количество секунд на подключение
			broker->_timeouts.connect = connect;
		}
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::node_t(fmk, log), _timer(fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::CLIENT;
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_timer.verbose(false);
	// Выполняем биндинг сетевого ядра таймера
	this->bind(dynamic_cast <awh::core_t *> (&this->_timer));
}
/**
 * Core Конструктор
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Core::Core(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept : awh::node_t(dns, fmk, log), _timer(fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::CLIENT;
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_timer.verbose(false);
	// Выполняем биндинг сетевого ядра таймера
	this->bind(dynamic_cast <awh::core_t *> (&this->_timer));
}
