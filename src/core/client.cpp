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
	auto it = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(it != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
					for(auto it = shm->_brokers.begin(); it != shm->_brokers.end();){
						// Если блокировка брокера не установлена
						if(this->_busy.find(it->first) == this->_busy.end()){
							// Создаём бъект активного брокера подключения
							awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (it->second.get());
							// Выполняем очистку буфера событий
							this->disable(it->first);
							// Выполняем очистку контекста двигателя
							broker->_ectx.clear();
							// Удаляем брокера из списка подключений
							this->_brokers.erase(it->first);
							// Удаляем брокера из списка
							it = shm->_brokers.erase(it);
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
				unique_ptr <awh::scheme_t::broker_t> broker(new awh::scheme_t::broker_t(sid, this->_fmk, this->_log));
				// Устанавливаем время жизни подключения
				broker->_addr.alive = shm->keepAlive;
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
					// Если тип сокета UDP TLS
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
				if(family == scheme_t::family_t::NIX)
					// Выполняем инициализацию сокета
					broker->_addr.init(this->_settings.sockname, engine_t::type_t::CLIENT);
				// Если unix-сокет не используется, выполняем инициализацию сокета
				else broker->_addr.init(url.ip, url.port, (family == scheme_t::family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::CLIENT);
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
							if((this->_callbacks.is("ssl")))
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
								this->_log->print("%s", log_t::flag_t::INFO, "Disconnected from the server");
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
								this->_log->print("%s", log_t::flag_t::INFO, "Disconnected from the server");
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
						// Запрещаем чтение данных с сервера
						broker->_bev.locked.read = true;
						// Запрещаем запись данных на сервер
						broker->_bev.locked.write = true;
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
					broker->base(this->_dispatch.base);
					// Добавляем созданного брокера в список брокеров
					auto ret = shm->_brokers.emplace(broker->id(), std::forward <unique_ptr <awh::scheme_t::broker_t>> (broker));
					// Добавляем брокера в список подключений
					this->_brokers.emplace(ret.first->first, sid);
					// Выполняем блокировку потока
					this->_mtx.connect.unlock();
					// Если подключение к серверу не выполнено
					if(!ret.first->second->_addr.connect()){
						// Запрещаем чтение данных с сервера
						ret.first->second->_bev.locked.read = true;
						// Запрещаем запись данных на сервер
						ret.first->second->_bev.locked.write = true;
						// Разрешаем выполнение работы
						shm->status.work = scheme_t::work_t::ALLOW;
						// Устанавливаем статус подключения
						shm->status.real = scheme_t::mode_t::DISCONNECT;
						// Если unix-сокет используется
						if(family == scheme_t::family_t::NIX){
							// Выводим ионформацию об обрыве подключении по unix-сокету
							this->_log->print("Connecting to socket = %s", log_t::flag_t::CRITICAL, this->_settings.sockname.c_str());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, this->_fmk->format("Connecting to socket = %s", this->_settings.sockname.c_str()));
						// Если используется хост и порт
						} else {
							// Выводим ионформацию об обрыве подключении по хосту и порту
							this->_log->print("Connecting to host = %s, port = %u", log_t::flag_t::CRITICAL, url.ip.c_str(), url.port);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, this->_fmk->format("Сonnecting to host = %s, port = %u", url.ip.c_str(), url.port));
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
					// Если статус подключения изменился
					if(shm->status.real != scheme_t::mode_t::PRECONNECT){
						// Запрещаем чтение данных с сервера
						ret.first->second->_bev.locked.read = true;
						// Запрещаем запись данных на сервер
						ret.first->second->_bev.locked.write = true;
					// Если статус подключения не изменился
					} else {
						// Выполняем установку функции обратного вызова на получении сообщений
						ret.first->second->callback <void (const uint64_t)> ("read", std::bind(&core_t::read, this, _1));
						// Выполняем установку функции обратного вызова на получение события подключения к серверу
						ret.first->second->callback <void (const uint64_t)> ("connect", std::bind(&core_t::connected, this, _1));
						// Выполняем установку функции обратного вызова на получение таймаута подключения
						ret.first->second->callback <void (const uint64_t, const engine_t::method_t)> ("timeout", std::bind(static_cast <void (core_t::*)(const uint64_t, const engine_t::method_t)> (&core_t::timeout), this, _1, _2));
						// Активируем ожидание подключения
						ret.first->second->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::CONNECT);
						// Если разрешено выводить информационные сообщения
						if(this->_verb){
							// Если unix-сокет используется
							if(family == scheme_t::family_t::NIX)
								// Выводим ионформацию об удачном подключении к серверу по unix-сокету
								this->_log->print("Good host %s, SOCKET=%d", log_t::flag_t::INFO, this->_settings.sockname.c_str(), ret.first->second->_addr.fd);
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
						this->_log->print("Client cannot be started [%s]", log_t::flag_t::CRITICAL, this->_settings.sockname.c_str());
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::CRITICAL, error_t::CONNECT, this->_fmk->format("Client cannot be started [%s]", this->_settings.sockname.c_str()));
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
						this->_log->print("%s", log_t::flag_t::INFO, "Disconnected from the server");
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("disconnect"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const uint16_t)> ("disconnect", 0, sid);
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const bad_alloc &) {
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
	auto it = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(it != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
				auto it = this->_schemes.find(sid);
				// Если идентификатор схемы сети найден
				if(it != this->_schemes.end()){
					// Получаем объект схемы сети
					scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
 * createTimeout Метод создания таймаута
 * @param sid  идентификатор схемы сети
 * @param mode режим работы клиента
 */
void awh::client::Core::createTimeout(const uint16_t sid, const scheme_t::mode_t mode) noexcept {
	// Если идентификатор схемы сети найден
	if(this->has(sid)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск таймера
			auto it = this->_timers.find(sid);
			// Если таймер найден
			if(it != this->_timers.end()){
				// Выполняем создание нового таймаута на 5 секунд
				const uint16_t tid = it->second->timeout(5000);
				// Выполняем добавление функции обратного вызова
				it->second->set <void (const uint16_t, const scheme_t::mode_t)> (tid, std::bind(static_cast <void (core_t::*)(const uint16_t, const scheme_t::mode_t)> (&core_t::timeout), this, sid, mode));
			// Если таймер не найден
			} else {
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.timer);
				// Выполняем создание нового таймера
				auto ret = this->_timers.emplace(sid, unique_ptr <timer_t> (new timer_t(this->_fmk, this->_log)));
				// Выполняем создание нового таймаута на 5 секунд
				const uint16_t tid = ret.first->second->timeout(5000);
				// Выполняем добавление функции обратного вызова
				ret.first->second->set <void (const uint16_t, const scheme_t::mode_t)> (tid, std::bind(static_cast <void (core_t::*)(const uint16_t, const scheme_t::mode_t)> (&core_t::timeout), this, sid, mode));
				// Устанавливаем флаг запрещающий вывод информационных сообщений
				ret.first->second->verbose(false);
				// Выполняем биндинг сетевого ядра таймера
				this->bind(dynamic_cast <awh::core_t *> (ret.first->second.get()));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
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
 * sendTimeout Метод отправки принудительного таймаута
 * @param bid идентификатор брокера
 */
void awh::client::Core::sendTimeout(const uint64_t bid) noexcept {
	// Если блокировка брокера не установлена
	if(this->_busy.find(bid) == this->_busy.end()){
		// Если брокер существует
		if(this->has(bid))
			// Выполняем отключение от сервера
			this->close(bid);
		// Если брокер не существует
		else if(!this->_schemes.empty()) {
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.reset);
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
 * clearTimeout Метод удаления установленного таймаута
 * @param sid идентификатор схемы сети
 */
void awh::client::Core::clearTimeout(const uint16_t sid) noexcept {
	// Выполняем поиск таймера
	auto it = this->_timers.find(sid);
	// Если таймер найден
	if(it != this->_timers.end())
		// Останавливаем работу таймера
		it->second->clear();
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
		// Останавливаем событие подключения к серверу
		const_cast <scheme_t::broker_t *> (broker)->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::CONNECT);
		// Выполняем блокировку на чтение/запись данных
		const_cast <scheme_t::broker_t *> (broker)->_bev.locked = scheme_t::locked_t();
	}
}
/**
 * close Метод отключения всех брокеров
 */
void awh::client::Core::close() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Выполняем перебор всех таймеров
		for(auto & timer : this->_timers)
			// Останавливаем работу таймера
			timer.second->clear();
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
				for(auto it = shm->_brokers.begin(); it != shm->_brokers.end();){
					// Если блокировка брокера не установлена
					if(this->_busy.find(it->first) == this->_busy.end()){
						// Выполняем блокировку брокера
						this->_busy.emplace(it->first);
						// Получаем бъект активного брокера подключения
						awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (it->second.get());
						// Выполняем очистку буфера событий
						this->disable(it->first);
						// Выполняем очистку контекста двигателя
						broker->_ectx.clear();
						// Удаляем брокера из списка подключений
						this->_brokers.erase(it->first);
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("disconnect"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const uint64_t, const uint16_t)> (it->first, this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), it->first, item.first);
						// Удаляем блокировку брокера
						this->_busy.erase(it->first);
						// Удаляем брокера из списка
						it = shm->_brokers.erase(it);
					// Иначе продолжаем дальше
					} else ++it;
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
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если список схем сети активен
	if(!this->_schemes.empty()){
		// Если список таймеров не пустой
		if(!this->_timers.empty()){
			// Выполняем перебор всех таймеров
			for(auto it = this->_timers.begin(); it != this->_timers.end();){
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.timer);
				// Останавливаем работу таймера
				it->second->clear();
				// Выполняем анбиндинг сетевого ядра таймера
				this->unbind(dynamic_cast <awh::core_t *> (it->second.get()));
				// Выполняем удаление таймера
				it = this->_timers.erase(it);
			}
		}
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
	auto it = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(it != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
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
			// Выполняем поиск идентификатора схемы сети
			auto it = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(it != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
				// Выполняем очистку буфера событий
				this->disable(bid);
				// Удаляем установленный таймаут, если он существует
				this->clearTimeout(it->first);
				// Если прокси-сервер активирован но уже переключён на работу с сервером
				if((shm->proxy.type != proxy_t::type_t::NONE) && !shm->isProxy())
					// Выполняем переключение обратно на прокси-сервер
					shm->switchConnect();
				// Выполняем очистку контекста двигателя
				broker->_ectx.clear();
				// Удаляем брокера из списка брокеров
				shm->_brokers.erase(bid);
				// Удаляем брокера из списка подключений
				this->_brokers.erase(bid);
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Устанавливаем статус сетевого ядра
				shm->status.real = scheme_t::mode_t::DISCONNECT;
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("disconnect"))
					// Устанавливаем полученную функцию обратного вызова
					callback.set <void (const uint64_t, const uint16_t)> ("disconnect", this->_callbacks.get <void (const uint64_t, const uint16_t)> ("disconnect"), bid, it->first);
				// Если активированно постоянное подключение
				if(shm->alive)
					// Устанавливаем функцию обратного вызова
					callback.set <void (const uint16_t)> ("reconnect", std::bind(&core_t::reconnect, this, _1), it->first);
			}
		}
		// Удаляем блокировку брокера
		this->_busy.erase(bid);
		// Если функция дисконнекта установлена
		if(callback.is("disconnect")){
			// Если разрешено выводить информационные сообщения
			if(this->_verb)
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::INFO, "Disconnected from the server");
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
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Выполняем поиск схемы сети
	auto i = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(i != this->_schemes.end()){
		// Если список таймеров не пустой
		if(!this->_timers.empty()){
			// Выполняем поиск таймера
			auto it = this->_timers.find(sid);
			// Если таймер найден удачно
			if(it != this->_timers.end()){
				// Выполняем блокировку потока
				const lock_guard <recursive_mutex> lock(this->_mtx.timer);
				// Останавливаем работу таймера
				it->second->clear();
				// Выполняем анбиндинг сетевого ядра таймера
				this->unbind(dynamic_cast <awh::core_t *> (it->second.get()));
				// Выполняем удаление таймера
				this->_timers.erase(it);
			}
		}
		// Объект работы с функциями обратного вызова
		fn_t callback(this->_log);
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
	const lock_guard <recursive_mutex> lock(this->_mtx.proxy);
	// Если идентификатор брокера подключений существует
	if(this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(broker->sid());
		// Если идентификатор схемы сети найден
		if(it != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
					if((this->_callbacks.is("ssl")))
						// Выполняем активацию шифрованного SSL канала
						this->_engine.encrypted(this->_callbacks.call <bool (const uri_t::url_t &, const uint64_t, const uint16_t)> ("ssl", shm->url, bid, it->first), broker->_ectx);
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
				// Активируем ожидание подключения
				broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::CONNECT);
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
		auto it = this->_schemes.find(broker->sid());
		// Если идентификатор схемы сети найден
		if(it != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
			// Если подключение удачное и работа разрешена
			if(shm->status.work == scheme_t::work_t::ALLOW){
				// Снимаем флаг получения данных
				shm->receiving = false;
				// Устанавливаем статус подключения к серверу
				shm->status.real = scheme_t::mode_t::CONNECT;
				// Устанавливаем флаг ожидания статуса
				shm->status.wait = scheme_t::mode_t::DISCONNECT;
				// Выполняем очистку существующих таймаутов
				this->clearTimeout(it->first);
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
						// Запускаем чтение данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
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
						// Запускаем чтение данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
						// Если разрешено выводить информационные сообщения
						if(this->_verb)
							// Выводим в лог сообщение
							this->_log->print("Connect client to server [%s:%d]", log_t::flag_t::INFO, host.c_str(), url.port);
					} break;
					// Если тип протокола подключения unix-сокет
					case static_cast <uint8_t> (scheme_t::family_t::NIX): {
						// Запускаем чтение данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::READ);
						// Если разрешено выводить информационные сообщения
						if(this->_verb)
							// Выводим в лог сообщение
							this->_log->print("Connect client to server [%s]", log_t::flag_t::INFO, this->_settings.sockname.c_str());
					} break;
				}
				// Если подключение производится через, прокси-сервер
				if(shm->isProxy()){
					// Если функция обратного вызова для прокси-сервера установлена
					if(this->_callbacks.is("connectProxy"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const uint16_t)> ("connectProxy", bid, it->first);
				// Если функция обратного вызова установлена
				} else if(this->_callbacks.is("connect"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const uint16_t)> ("connect", bid, it->first);
				// Выходим из функции
				return;
			}
			// Выполняем отключение от сервера
			this->close(bid);
		}
	}
}
/**
 * timeout Метод вызова при срабатывании таймаута
 * @param bid    идентификатор брокера
 * @param method метод режима работы
 */
void awh::client::Core::timeout(const uint64_t bid, const engine_t::method_t method) noexcept {
	// Если идентификатор брокера подключений существует
	if(this->has(bid)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Выполняем поиск идентификатора схемы сети
		auto it = this->_schemes.find(broker->sid());
		// Если идентификатор схемы сети найден
		if(it != this->_schemes.end()){
			// Получаем объект схемы сети
			scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
			// Получаем семейство интернет-протоколов
			const scheme_t::family_t family = (shm->isProxy() ? shm->proxy.family : this->_settings.family);
			// Определяем тип протокола подключения
			switch(static_cast <uint8_t> (family)){
				// Если тип протокола подключения IPv4
				case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Если тип протокола подключения IPv6
				case static_cast <uint8_t> (scheme_t::family_t::IPV6): {
					// Получаем URL параметры запроса
					const uri_t::url_t & url = (shm->isProxy() ? shm->proxy.url : shm->url);
					// Если IP-адрес не получен и объект DNS-резолвера установлен
					if(!shm->receiving && !url.ip.empty() && (this->_dns != nullptr)){
						// Определяем тип протокола подключения
						switch(static_cast <uint8_t> (family)){
							// Резолвер IPv4, добавляем бракованный IPv4 адрес в список адресов
							case static_cast <uint8_t> (scheme_t::family_t::IPV4):
								// Устанавливаем адрес в чёрный список
								const_cast <dns_t *> (this->_dns)->setToBlackList(AF_INET, url.domain, url.ip);
							break;
							// Резолвер IPv6, добавляем бракованный IPv6 адрес в список адресов
							case static_cast <uint8_t> (scheme_t::family_t::IPV6):
								// Устанавливаем адрес в чёрный список
								const_cast <dns_t *> (this->_dns)->setToBlackList(AF_INET6, url.domain, url.ip);
							break;
						}
					}
					// Определяем метод на который сработал таймаут
					switch(static_cast <uint8_t> (method)){
						// Режим работы ЧТЕНИЕ
						case static_cast <uint8_t> (engine_t::method_t::READ): {
							// Выводим сообщение в лог, о таймауте подключения
							this->_log->print("Timeout read data from HOST=%s [%s%d]", log_t::flag_t::WARNING, url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout read data from HOST=%s [%s%d]", url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port));
						} break;
						// Режим работы ЗАПИСЬ
						case static_cast <uint8_t> (engine_t::method_t::WRITE): {
							// Выводим сообщение в лог, о таймауте подключения
							this->_log->print("Timeout write data to HOST=%s [%s%d]", log_t::flag_t::WARNING, url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout write data to HOST=%s [%s%d]", url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port));
						} break;
						// Если передано событие подписки на подключение
						case static_cast <uint8_t> (engine_t::method_t::CONNECT): {
							// Выводим сообщение в лог, о таймауте подключения
							this->_log->print("Timeout connect to HOST=%s [%s%d]", log_t::flag_t::WARNING, url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port);
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout connect to HOST=%s [%s%d]", url.domain.c_str(), (!url.ip.empty() ? (url.ip + ":").c_str() : ""), url.port));
						} break;
					}
				} break;
				// Если тип протокола подключения unix-сокет
				case static_cast <uint8_t> (scheme_t::family_t::NIX): {
					// Определяем метод на который сработал таймаут
					switch(static_cast <uint8_t> (method)){
						// Режим работы ЧТЕНИЕ
						case static_cast <uint8_t> (engine_t::method_t::READ): {
							// Выводим сообщение в лог, о таймауте подключения
							this->_log->print("Timeout read data from HOST=%s", log_t::flag_t::WARNING, this->_settings.sockname.c_str());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout read data from HOST=%s", this->_settings.sockname.c_str()));
						} break;
						// Режим работы ЗАПИСЬ
						case static_cast <uint8_t> (engine_t::method_t::WRITE): {
							// Выводим сообщение в лог, о таймауте подключения
							this->_log->print("Timeout write data to HOST=%s", log_t::flag_t::WARNING, this->_settings.sockname.c_str());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout write data to HOST=%s", this->_settings.sockname.c_str()));
						} break;
						// Если передано событие подписки на подключение
						case static_cast <uint8_t> (engine_t::method_t::CONNECT): {
							// Выводим сообщение в лог, о таймауте подключения
							this->_log->print("Timeout connect to HOST=%s", log_t::flag_t::WARNING, this->_settings.sockname.c_str());
							// Если функция обратного вызова установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const log_t::flag_t, const error_t, const string &)> ("error", log_t::flag_t::WARNING, error_t::TIMEOUT, this->_fmk->format("Timeout connect to HOST=%s", this->_settings.sockname.c_str()));
						} break;
					}
				} break;
			}
			// Останавливаем чтение данных
			broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
			// Останавливаем запись данных
			broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
			// Выполняем отключение от сервера
			this->close(bid);
		}
	}
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
			auto it = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(it != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
				// Если подключение установлено
				if((shm->receiving = (shm->status.real == scheme_t::mode_t::CONNECT))){
					// Останавливаем чтение данных с клиента
					broker->_bev.events.read.stop();
					// Получаем максимальный размер буфера
					const int64_t size = broker->_ectx.buffer(engine_t::method_t::READ);
					// Если размер буфера получен
					if(size > 0){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Количество полученных байт
							int64_t bytes = -1;
							// Создаём буфер входящих данных
							unique_ptr <char []> buffer(new char [size]);
							// Выполняем чтение данных с сокета
							do {
								// Если подключение выполнено
								if(!broker->_bev.locked.read && (shm->status.real == scheme_t::mode_t::CONNECT)){
									// Выполняем обнуление буфера данных
									::memset(buffer.get(), 0, size);
									// Выполняем получение сообщения от клиента
									bytes = broker->_ectx.read(buffer.get(), size);
									// Если данные получены
									if(bytes > 0){
										// Если флаг ожидания входящих сообщений, активирован
										if(broker->_timeouts.read > 0){
											// Определяем тип активного сокета
											switch(static_cast <uint8_t> (this->_settings.sonet)){
												// Если тип сокета установлен как UDP
												case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
												// Если тип сокета установлен как DTLS
												case static_cast <uint8_t> (scheme_t::sonet_t::DTLS): break;
												// Останавливаем таймаут ожидания на чтение из сокета
												default: broker->_bev.timers.read.stop();
											}
										}
										// Если данные считанные из буфера, больше размера ожидающего буфера
										if((broker->_marker.read.max > 0) && (bytes >= broker->_marker.read.max)){
											// Смещение в буфере и отправляемый размер данных
											size_t offset = 0, actual = 0;
											// Выполняем пересылку всех полученных данных
											while((bytes - offset) > 0){
												// Определяем размер отправляемых данных
												actual = ((bytes - offset) >= broker->_marker.read.max ? broker->_marker.read.max : (bytes - offset));
												// Если подключение производится через, прокси-сервер
												if(shm->isProxy()){
													// Если функция обратного вызова для вывода записи существует
													if(this->_callbacks.is("readProxy"))
														// Выводим функцию обратного вызова
														this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("readProxy", buffer.get() + offset, actual, bid, it->first);
												// Если прокси-сервер не используется
												} else if(this->_callbacks.is("read"))
													// Выводим функцию обратного вызова
													this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", buffer.get() + offset, actual, bid, it->first);
												// Увеличиваем смещение в буфере
												offset += actual;
											}
										// Если данных достаточно
										} else {
											// Если подключение производится через, прокси-сервер
											if(shm->isProxy()){
												// Если функция обратного вызова для вывода записи существует
												if(this->_callbacks.is("readProxy"))
													// Выводим функцию обратного вызова
													this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("readProxy", buffer.get(), bytes, bid, it->first);
											// Если прокси-сервер не используется
											} else if(this->_callbacks.is("read"))
												// Выводим функцию обратного вызова
												this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", buffer.get(), bytes, bid, it->first);
										}
										// Если флаг ожидания входящих сообщений, активирован
										if(broker->_timeouts.read > 0){
											// Определяем тип активного сокета
											switch(static_cast <uint8_t> (this->_settings.sonet)){
												// Если тип сокета установлен как UDP
												case static_cast <uint8_t> (scheme_t::sonet_t::UDP):
												// Если тип сокета установлен как DTLS
												case static_cast <uint8_t> (scheme_t::sonet_t::DTLS):
													// Выполняем установку таймаута ожидания
													broker->_ectx.timeout(broker->_timeouts.read * 1000, engine_t::method_t::READ);
												break;
												// Для всех остальных протоколов
												default: {
													// Если время ожидания чтения данных установлено
													if(shm->wait)
														// Запускаем работу таймера
														broker->_bev.timers.read.start(broker->_timeouts.read * 1000);
												}
											}
										}
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
							// Если тип сокета не установлен как UDP, запускаем чтение дальше
							if((this->_settings.sonet != scheme_t::sonet_t::UDP) && this->has(bid))
								// Запускаем чтение данных с клиента
								broker->_bev.events.read.start();
						/**
						 * Если возникает ошибка
						 */
						} catch(const bad_alloc &) {
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						}
					// Выполняем отключение клиента
					} else this->close(bid);
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
 * write Метод записи буфера данных в сокет
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param bid    идентификатор брокера
 */
void awh::client::Core::write(const char * buffer, const size_t size, const uint64_t bid) noexcept {
	// Если данные переданы
	if(this->working() && this->has(bid) && (buffer != nullptr) && (size > 0)){
		// Создаём бъект активного брокера подключения
		awh::scheme_t::broker_t * broker = const_cast <awh::scheme_t::broker_t *> (this->broker(bid));
		// Если сокет подключения активен
		if((broker->_addr.fd != INVALID_SOCKET) && (broker->_addr.fd < MAX_SOCKETS)){
			// Выполняем поиск идентификатора схемы сети
			auto it = this->_schemes.find(broker->sid());
			// Если идентификатор схемы сети найден
			if(it != this->_schemes.end()){
				// Получаем объект схемы сети
				scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
							broker->_ectx.block();
						break;
					}
					// Если данных достаточно для записи в сокет
					if(size >= broker->_marker.write.min){
						// Количество полученных байт
						int64_t bytes = -1;
						// Cмещение в буфере и отправляемый размер данных
						size_t offset = 0, actual = 0, left = 0;
						// Получаем максимальный размер буфера
						int64_t max = broker->_ectx.buffer(engine_t::method_t::WRITE);
						// Если максимальное установленное значение больше размеров буфера для записи, корректируем
						max = ((max > 0) && (broker->_marker.write.max > max) ? max : broker->_marker.write.max);
						// Активируем ожидание записи данных
						broker->events(awh::scheme_t::mode_t::ENABLED, engine_t::method_t::WRITE);
						// Выполняем отправку данных пока всё не отправим
						while((size - offset) > 0){
							// Получаем общий размер буфера данных
							left = (size - offset);
							// Определяем размер отправляемых данных
							actual = (left >= max ? max : left);
							// Выполняем установку таймаута ожидания записи в сокет
							broker->_ectx.timeout(broker->_timeouts.write * 1000, engine_t::method_t::WRITE);
							// Выполняем отправку сообщения клиенту
							bytes = broker->_ectx.write(buffer + offset, actual);
							// Если данные небыли записаны
							if(bytes <= 0){
								// Если запись не выполнена, закрываем подключение
								if(bytes == 0)
									// Выполняем закрытие подключения
									this->close(bid);
								// Выходим из цикла
								break;
							}
							// Увеличиваем смещение в буфере
							offset += bytes;
						}
						// Если дисконнекта от сервера не произошло
						if(bytes > 0){
							// Определяем тип сокета
							switch(static_cast <uint8_t> (this->_settings.sonet)){
								// Если тип сокета установлен как TCP/IP
								case static_cast <uint8_t> (scheme_t::sonet_t::TCP):
								// Если тип сокета установлен как TCP/IP TLS
								case static_cast <uint8_t> (scheme_t::sonet_t::TLS):
								// Если тип сокета установлен как SCTP
								case static_cast <uint8_t> (scheme_t::sonet_t::SCTP):
									// Переводим сокет в неблокирующий режим
									broker->_ectx.noblock();
								break;
							}
						}
						// Останавливаем ожидание записи данных
						broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
						// Если подключение производится через, прокси-сервер
						if(shm->isProxy()){
							// Если функция обратного вызова для вывода записи существует
							if(this->_callbacks.is("writeProxy"))
								// Выводим функцию обратного вызова
								this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("writeProxy", buffer, offset, bid, it->first);
						// Если функция обратного вызова на запись данных установлена
						} else if(this->_callbacks.is("write"))
							// Выводим функцию обратного вызова
							this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", buffer, offset, bid, it->first);
					// Если данных недостаточно для записи в сокет
					} else {
						// Останавливаем ожидание записи данных
						broker->events(awh::scheme_t::mode_t::DISABLED, engine_t::method_t::WRITE);
						// Если подключение производится через, прокси-сервер
						if(shm->isProxy()){
							// Если функция обратного вызова для вывода записи существует
							if(this->_callbacks.is("writeProxy"))
								// Выводим функцию обратного вызова
								this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("writeProxy", nullptr, 0, bid, it->first);
						// Если функция обратного вызова на запись данных установлена
						} else if(this->_callbacks.is("write"))
							// Выводим функцию обратного вызова
							this->_callbacks.call <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", nullptr, 0, bid, it->first);
					}
					// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
					if((this->_settings.sonet == scheme_t::sonet_t::UDP) && this->has(bid))
						// Запускаем чтение данных с клиента
						broker->_bev.events.read.start();
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
}
/**
 * work Метод запуска работы подключения клиента
 * @param sid    идентификатор схемы сети
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 */
void awh::client::Core::work(const uint16_t sid, const string & ip, const int family) noexcept {
	// Выполняем поиск идентификатора схемы сети
	auto it = this->_schemes.find(sid);
	// Если идентификатор схемы сети найден
	if(it != this->_schemes.end()){
		// Получаем объект схемы сети
		scheme_t * shm = dynamic_cast <scheme_t *> (const_cast <awh::scheme_t *> (it->second));
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
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::node_t(fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::CLIENT;
}
/**
 * Core Конструктор
 * @param dns объект DNS-резолвера
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Core::Core(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept : awh::node_t(dns, fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->_type = engine_t::type_t::CLIENT;
}
