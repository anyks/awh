/**
 * @file: dns.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <net/dns.hpp>

/**
 * access Метод проверки на разрешение выполнения операции
 * @param comp  статус сравнения
 * @param hold  статус установки
 * @param equal флаг эквивалентности
 * @return      результат проверки
 */
bool awh::DNS::Holder::access(const set <status_t> & comp, const status_t hold, const bool equal) noexcept {
	// Определяем есть ли фиксированные статусы
	this->flag = this->status->empty();
	// Если результат не получен
	if(!this->flag && !comp.empty())
		// Получаем результат сравнения
		this->flag = (equal ? (comp.count(this->status->top()) > 0) : (comp.count(this->status->top()) < 1));
	// Если результат получен, выполняем холд
	if(this->flag) this->status->push(hold);
	// Выводим результат
	return this->flag;
}
/**
 * ~Holder Деструктор
 */
awh::DNS::Holder::~Holder() noexcept {
	// Если холдирование выполнено
	if(this->flag) this->status->pop();
}
/**
 * createBase Метод создания dns базы
 */
void awh::DNS::createBase() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::SET_BASE}, status_t::CREATE_EVDNS)){
		// Если база событий существует
		if((this->fmk != nullptr) && (this->base != nullptr)){
			// Выполняем удаление модуля DNS резолвера
			this->remove();
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->mtx.base);
			// Создаем базу dns
			this->dbase = evdns_base_new(this->base, 0);
			// Если база dns не создана
			if(this->dbase == nullptr)
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "dns base does not created");
		}
	}
}
/**
 * clearZombie Метод очистки зомби-запросов
 */
void awh::DNS::clearZombie() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::CLEAR, status_t::RESOLVE}, status_t::ZOMBIE)){
		// Если список воркеров не пустой
		if(!this->workers.empty()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->mtx.worker);
			// Список зомби-воркеров
			vector <size_t> zombie;
			// Получаем текущее значение даты
			const time_t date = this->fmk->unixTimestamp();
			// Переходим по всем воркерам
			for(auto & worker : this->workers){
				// Если DNS запрос устарел на 3-и минуты, убиваем его
				if((date - worker.first) >= 180000){
					// Очищаем объект таймаута базы событий
					evutil_timerclear(&worker.second->tv);
					// Удаляем событие таймера
					event_del(&worker.second->ev);
					// Добавляем идентификатор воркеров в список зомби
					zombie.push_back(worker.first);
				}
			}
			// Если список зомби-воркеров существует
			if(!zombie.empty()){
				// Переходим по всему списку зомби-воркеров
				for(auto it = zombie.begin(); it != zombie.end();){
					// Выполняем отмену DNS запроса
					this->cancel(* it);
					// Удаляем зомби-воркер из списка зомби
					it = zombie.erase(it);
				}
			}
		}
	}
}
/**
 * garbage Функция выполняемая по таймеру для чистки мусора
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 * @param ctx   передаваемый контекст
 */
void awh::DNS::garbage(evutil_socket_t fd, short event, void * ctx) noexcept {
	// Если данные получены
	if(ctx != nullptr){
		// Получаем объект воркера резолвинга
		worker_t * wrk = reinterpret_cast <worker_t *> (ctx);
		// Получаем объект модуля DNS резолвера
		dns_t * dns = const_cast <dns_t *> (wrk->dns);
		// Если данные фреймворка существуют
		if(dns->log != nullptr)
			// Если возникла ошибка, выводим в лог сообщение
			dns->log->print("%s request failed", log_t::flag_t::CRITICAL, wrk->host.c_str());
		// Очищаем объект таймаута базы событий
		evutil_timerclear(&wrk->tv);
		// Удаляем событие таймера
		event_del(&wrk->ev);
		// Выполняем отмену DNS запроса
		dns->cancel(wrk->did);
	}
}
/**
 * callback Событие срабатывающееся при получении данных с dns сервера
 * @param error ошибка dns сервера
 * @param addr  структура данных с dns сервера
 * @param ctx   объект с данными для запроса
 */
void awh::DNS::callback(const int error, struct evutil_addrinfo * addr, void * ctx) noexcept {
	// Если данные получены
	if(ctx != nullptr){
		// Получаем объект воркера резолвинга
		worker_t * wrk = reinterpret_cast <worker_t *> (ctx);
		// Получаем объект модуля DNS резолвера
		dns_t * dns = const_cast <dns_t *> (wrk->dns);
		// Список полученных IP адресов
		vector <string> ips;
		// Если возникла ошибка, выводим в лог сообщение
		if(error && (dns->log != nullptr)) dns->log->print("%s %s", log_t::flag_t::CRITICAL, wrk->host.c_str(), evutil_gai_strerror(error));
		// Если ошибки не возникало
		else {
			// Создаем буфер для получения ip адреса
			char buffer[128];
			// Полученный IP адрес
			const char * ip = nullptr;
			// Создаем структуру данных, доменного имени
			struct evutil_addrinfo * ai;
			// Переходим по всей структуре и выводим ip адреса
			for(ai = addr; ai; ai = ai->ai_next){
				// Заполняем буфер нулями
				memset(buffer, 0, sizeof(buffer));
				// Если это искомый тип интернет протокола
				if((ai->ai_family == wrk->family) || (wrk->family == AF_UNSPEC)){
					// Получаем структуру для указанного интернет протокола
					switch(ai->ai_family){
						// Если это IPv4 адрес
						case AF_INET: {
							// Создаем структуру для получения ip адреса
							struct sockaddr_in * sin = (struct sockaddr_in *) ai->ai_addr;
							// Выполняем запрос и получает ip адрес
							ip = evutil_inet_ntop(ai->ai_family, &sin->sin_addr, buffer, 128);
						} break;
						// Если это IPv6 адрес
						case AF_INET6: {
							// Создаем структуру для получения ip адреса
							struct sockaddr_in6 * sin6 = (struct sockaddr_in6 *) ai->ai_addr;
							// Выполняем запрос и получает ip адрес
							ip = evutil_inet_ntop(ai->ai_family, &sin6->sin6_addr, buffer, 128);
						} break;
					}
					// Если IP адрес получен и не находится в чёрном списке
					if((ip != nullptr) && (dns->blacklist.empty() || (dns->blacklist.count(ip) < 1))){
						// Добавляем полученный IP адрес в список
						ips.push_back(ip);
						// Выполняем блокировку потока
						dns->mtx.cache.lock();
						// Записываем данные в кэш
						dns->cache.emplace(wrk->host, ip);
						// Выполняем разблокировку потока
						dns->mtx.cache.unlock();
					}
				}
			}
		}
		// Полученный ip адрес
		const string * ip = nullptr;
		// Если ip адреса получены, выводим ip адрес в случайном порядке
		if(!ips.empty()){
			// Если количество элементов больше 1
			if(ips.size() > 1){
				// Рандомизация генератора случайных чисел
				srand(time(nullptr));
				// Получаем ip адрес
				ip = &ips.at(rand() % ips.size());
			// Выводим только первый элемент
			} else ip = &ips.front();
		// Если чёрный список доменных имён не пустой
		} else if(!dns->blacklist.empty() && !error) {
			// Выполняем блокировку потока
			dns->mtx.blacklist.lock();
			// Выполняем очистку чёрного списка
			dns->blacklist.clear();
			// Выполняем разблокировку потока
			dns->mtx.blacklist.unlock();
			// Пробуем получить IP адреса заново
			callback(error, addr, ctx);
			// Выходим из функции
			return;
		}
		// Очищаем объект таймаута базы событий
		evutil_timerclear(&wrk->tv);
		// Удаляем событие таймера
		event_del(&wrk->ev);
		// Очищаем структуру данных домена
		if(!error) evutil_freeaddrinfo(addr);
		// Получаем контекст воркера
		auto context = wrk->context;
		// Функция обратного вызова
		auto callback = wrk->callback;
		// Выполняем блокировку потока
		dns->mtx.worker.lock();
		// Удаляем домен из списка доменов
		dns->workers.erase(wrk->did);
		// Выполняем разблокировку потока
		dns->mtx.worker.unlock();
		// Если функция обратного вызова передана
		if(callback != nullptr)
			// Выводим полученный IP адрес
			callback(ip != nullptr ? (* ip) : "", context);
	}
}
/**
 * clear Метод сброса кэша резолвера
 */
void awh::DNS::clear() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::REMOVE}, status_t::CLEAR)){
		// Выполняем сброс кэша DNS резолвера
		this->flush();
		// Выполняем отмену выполненных запросов
		this->cancel();
		// Убиваем зомби-процессы если такие имеются
		this->clearZombie();
		// Выполняем блокировку потока
		this->mtx.servers.lock();
		// Выполняем сброс списка IP адресов
		this->servers.clear();
		// Выполняем разблокировку потока
		this->mtx.servers.unlock();
	}
}
/**
 * flush Метод сброса кэша DNS резолвера
 */
void awh::DNS::flush() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::CLEAR}, status_t::FLUSH)){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.cache);
		// Выполняем сброс кэша полученных IP адресов
		this->cache.clear();
	}
}
/**
 * remove Метод удаления параметров модуля DNS резолвера
 */
void awh::DNS::remove() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::CREATE_EVDNS}, status_t::REMOVE)){
		// Очищаем базу dns
		if(this->dbase != nullptr){
			// Выполняем сброс кэша резолвера
			this->clear();
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->mtx.base);
			// Очищаем базу dns
			evdns_base_free(this->dbase, 0);
			// Зануляем базу DNS
			this->dbase = nullptr;
		}
	}
}
/**
 * cancel Метод отмены выполнения запроса
 * @param did идентификатор DNS запроса
 */
void awh::DNS::cancel(const size_t did) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::RESOLVE}, status_t::CANCEL, false)){
		// Если список воркеров не пустой
		if(!this->workers.empty()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->mtx.worker);
			// Если идентификатор DNS запроса передан
			if(did > 0){
				// Выполняем поиск воркера
				auto it = this->workers.find(did);
				// Если воркер найден
				if(it != this->workers.end()){
					// Если объект запроса существует
					if(it->second->reply != nullptr)
						// Выполняем отмену запроса
						evdns_getaddrinfo_cancel(it->second->reply);
					// Иначе выполняем принудительный возврат
					else {
						// Выводим пустой IP адрес
						it->second->callback("", it->second->context);
						// Удаляем объект воркера
						this->workers.erase(it);
					}
				}
			// Если нужно остановить работу всех DNS запросов
			} else {
				// Переходим по всем воркерам
				for(auto it = this->workers.begin(); it != this->workers.end();){
					// Если объект запроса существует
					if(it->second->reply != nullptr){
						// Выполняем отмену запроса
						evdns_getaddrinfo_cancel(it->second->reply);
						// Продолжаем перебор
						++it;
					// Иначе выполняем принудительный возврат
					} else {
						// Выводим пустой IP адрес
						it->second->callback("", it->second->context);
						// Удаляем объект воркера
						it = this->workers.erase(it);
					}
				}
			}
		}
	}
}
/**
 * updateNameServers Метод обновления списка нейм-серверов
 */
void awh::DNS::updateNameServers() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({}, status_t::NSS_UPDATE)){
		// Если список DNS серверов не пустой
		if(!this->servers.empty() && (this->dbase != nullptr)){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->mtx.evdns);
			// Выполняем очистку предыдущих DNS серверов
			evdns_base_clear_nameservers_and_suspend(this->dbase);
			// Переходим по всем нейм серверам и добавляем их
			for(auto & server : this->servers) this->setNameServer(server);
		}
	}
}
/**
 * clearBlackList Метод очистки чёрного списка
 */
void awh::DNS::clearBlackList() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.blacklist);
	// Выполняем очистку чёрного списка
	this->blacklist.clear();
}
/**
 * delInBlackList Метод удаления IP адреса из чёрного списока
 * @param ip адрес для удаления из чёрного списка
 */
void awh::DNS::delInBlackList(const string & ip) noexcept {
	// Если IP адрес передан
	if(!ip.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.blacklist);
		// Выполняем поиск IP адреса в чёрном списке
		auto it = this->blacklist.find(ip);
		// Если IP адрес найден в чёрном списке, удаляем его
		if(it != this->blacklist.end()) this->blacklist.erase(it);
	}
}
/**
 * setToBlackList Метод добавления IP адреса в чёрный список
 * @param ip адрес для добавления в чёрный список
 */
void awh::DNS::setToBlackList(const string & ip) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.blacklist);
	// Если IP адрес передан, добавляем IP адрес в чёрный список
	if(!ip.empty()) this->blacklist.emplace(ip);
}
/**
 * setBase Установка базы событий
 * @param base объект базы событий
 */
void awh::DNS::setBase(struct event_base * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({}, status_t::SET_BASE)){
		// Если база передана
		if(base != nullptr){
			// Выполняем блокировку потока
			this->mtx.base.lock();
			// Создаем базу событий
			this->base = base;
			// Выполняем разблокировку потока
			this->mtx.base.unlock();
			// Создаем dns базу
			this->createBase();
		}
	}
}
/**
 * setNameServer Метод добавления сервера dns
 * @param server ip адрес dns сервера
 */
void awh::DNS::setNameServer(const string & server) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::NSS_SET, status_t::NSS_UPDATE}, status_t::NS_SET)){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.evdns);
		// Если dns сервер передан
		if(!server.empty() && (this->fmk != nullptr) && (this->base != nullptr) && (this->dbase != nullptr)){
			// Адрес имени сервера
			string ns = "";
			// Определяем тип передаваемого сервера
			switch((uint8_t) this->nwk->parseHost(server)){
				// Если хост является доменом или IPv4 адресом
				case (uint8_t) network_t::type_t::IPV4:
				case (uint8_t) network_t::type_t::DOMNAME: ns = server; break;
				// Если хост является IPv6 адресом, переводим ip адрес в полную форму
				case (uint8_t) network_t::type_t::IPV6: ns = this->nwk->setLowIp6(server); break;
			}
			// Если DNS сервер установлен, добавляем его в базу DNS
			if(!ns.empty() && (evdns_base_nameserver_ip_add(this->dbase, ns.c_str()) != 0))
				// Выводим в лог сообщение
				this->log->print("name server [%s] does not add", log_t::flag_t::CRITICAL, ns.c_str());
		}
	}
}
/**
 * setNameServers Метод добавления серверов dns
 * @param server ip адреса dns серверов
 */
void awh::DNS::setNameServers(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({status_t::NSS_REP}, status_t::NSS_SET)){
		// Если нейм сервера переданы
		if(!servers.empty()){
			// Выполняем блокировку потока
			this->mtx.servers.lock();
			// Запоминаем dns сервера
			this->servers = servers;
			// Выполняем разблокировку потока
			this->mtx.servers.unlock();
			// Переходим по всем нейм серверам и добавляем их
			for(auto & server : this->servers) this->setNameServer(server);
		}
	}
}
/**
 * replaceServers Метод замены существующих серверов dns
 * @param servers ip адреса dns серверов
 */
void awh::DNS::replaceServers(const vector <string> & servers) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({}, status_t::NSS_REP)){
		// Если нейм сервера переданы, удаляем все настроенные серверы имён и приостанавливаем все ожидающие решения
		if(!servers.empty()){
			// Получаем количество серверов DNS находящихся в базе
			const u_short count = (this->dbase != nullptr ? evdns_base_count_nameservers(this->dbase) : 0);
			// Если список DNS серверов отличается
			if(this->servers.empty() || (count == 0) || !equal(servers.begin(), servers.end(), this->servers.begin())){
				// Выполняем блокировку потока
				this->mtx.evdns.lock();
				// Выполняем очистку предыдущих DNS серверов
				if(count > 0) evdns_base_clear_nameservers_and_suspend(this->dbase);
				// Выполняем разблокировку потока
				this->mtx.evdns.unlock();
				// Устанавливаем новый список серверов
				this->setNameServers(servers);
			}
		}
	}
}
/**
 * resolve Метод ресолвинга домена
 * @param ctx      передаваемый контекст
 * @param host     хост сервера
 * @param family   тип интернет протокола IPv4 или IPv6
 * @param callback функция обратного вызова срабатывающая при получении данных
 * @return         идентификатор DNS запроса
 */
size_t awh::DNS::resolve(void * ctx, const string & host, const int family, function <void (const string, void *)> callback) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.hold);
	// Создаём объект холдирования
	hold_t hold(&this->status);
	// Если статус работы DNS резолвера соответствует
	if(hold.access({}, status_t::RESOLVE)){
		// Если домен передан
		if(!host.empty() && (this->fmk != nullptr)){
			// Результат работы регулярного выражения
			smatch match;
			// Устанавливаем правило регулярного выражения
			regex e("^\\[?(\\d{1,3}(?:\\.\\d{1,3}){3}|[a-f\\d\\:]{2,39})\\]?$", regex::ECMAScript | regex::icase);
			// Выполняем поиск протокола
			regex_search(host, match, e);
			// Если данные найдены
			if(match.empty()){
				// Получаем количество IP адресов в кэше
				const size_t count = this->cache.count(host);
				// Если количество адресов всего 1
				if(count == 1){
					// Выполняем проверку запрашиваемого хоста в кэше
					auto it = this->cache.find(host);
					// Если хост найден, выводим его
					if((it != this->cache.end()) && (this->blacklist.empty() || (this->blacklist.count(it->second) < 1)))
						// Выводим полученный IP адрес
						callback(it->second, ctx);
					// Выполняем запрос IP адреса
					else goto Resolve;
				// Если доменных имён в кэше больше 1-го
				} else if(count > 1) {
					// Список полученных IP адресов
					vector <string> ips;
					// Получаем диапазон IP адресов в кэше
					auto ret = this->cache.equal_range(host);
					// Переходим по всему списку IP адресов
					for(auto it = ret.first; it != ret.second; ++it){
						// Если чёрный список пустой, или IP адрес в нём не значится
						if(this->blacklist.empty() || (this->blacklist.count(it->second) < 1))
							// Добавляем IP адрес в список адресов пригодных для работы
							ips.push_back(it->second);
					}
					// Если список IP адресов получен
					if(!ips.empty()){
						// рандомизация генератора случайных чисел
						srand(time(0));
						// Получаем ip адрес
						callback(ips.at(rand() % ips.size()), ctx);
					// Выполняем запрос IP адреса
					} else goto Resolve;
				// Если адрес не найден то запрашиваем его с резолвера
				} else {
					// Устанавливаем метку получения IP адреса
					Resolve:
					// Выполняем блокировку потока
					this->mtx.worker.lock();
					// Получаем идентификатор воркера
					result = this->fmk->unixTimestamp();
					// Добавляем воркер резолвинга в список воркеров
					auto ret = this->workers.emplace(result, unique_ptr <worker_t> (new worker_t()));
					// Выполняем разблокировку потока
					this->mtx.worker.unlock();
					// Запоминаем текущий объект
					ret.first->second->dns = this;
					// Запоминаем название искомого домена
					ret.first->second->host = host;
					// Формируем идентификатор объекта
					ret.first->second->did = result;
					// Устанавливаем передаваемый контекст
					ret.first->second->context = ctx;
					// Устанавливаем тип протокола интернета
					ret.first->second->family = family;
					// Устанавливаем функцию обратного вызова
					ret.first->second->callback = callback;
					// Устанавливаем тип подключения
					ret.first->second->hints.ai_family = AF_UNSPEC;
					// Устанавливаем что это потоковый сокет
					ret.first->second->hints.ai_socktype = SOCK_STREAM;
					// Устанавливаем что это tcp подключение
					ret.first->second->hints.ai_protocol = IPPROTO_TCP;
					// Устанавливаем флаг подключения что это канонническое имя
					ret.first->second->hints.ai_flags = EVUTIL_AI_CANONNAME;
					// Выполняем dns запрос
					ret.first->second->reply = evdns_getaddrinfo(this->dbase, host.c_str(), nullptr, &ret.first->second->hints, &dns_t::callback, ret.first->second.get());
					// Если DNS запрос не создан
					if(ret.first->second->reply == nullptr){
						// Выполняем блокировку потока
						this->mtx.worker.lock();
						// Удаляем объект воркера
						this->workers.erase(result);
						// Выполняем разблокировку потока
						this->mtx.worker.unlock();
						// Выводим в лог сообщение
						this->log->print("request for %s returned immediately", log_t::flag_t::CRITICAL, host.c_str());
						// Выводим функцию обратного вызова
						callback("", ctx);
					// Если DNS запрос удачно создан
					} else {
						// Создаём событие на ожидание выполнения запроса
						event_assign(&ret.first->second->ev, this->base, -1, EV_TIMEOUT, &garbage, ret.first->second.get());
						// Очищаем объект таймаута базы событий
						evutil_timerclear(&ret.first->second->tv);
						// Устанавливаем таймаут базы событий в 30 секунд
						ret.first->second->tv.tv_sec = 30;
						// Создаём событие таймаута на ожидание выполнения запроса
						event_add(&ret.first->second->ev, &ret.first->second->tv);
					}
				}
			// Если передан IP адрес то возвращаем его
			} else callback(match[1].str(), ctx);
		}
		// Убиваем зомби-процессы если такие имеются
		this->clearZombie();
	}
	// Выводим результат
	return result;
}
/**
 * DNS Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param nwk объект методов для работы с сетью
 */
awh::DNS::DNS(const fmk_t * fmk, const log_t * log, const network_t * nwk) noexcept {
	// Устанавливаем объект основного фреймворка
	this->fmk = fmk;
	// Устанавливаем объект методов для работы с сетью
	this->nwk = nwk;
	// Устанавливаем объект для работы с логами
	this->log = log;
}
/**
 * DNS Конструктор
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 * @param nwk     объект методов для работы с сетью
 * @param base    база событий
 * @param servers массив dns серверов
 */
awh::DNS::DNS(const fmk_t * fmk, const log_t * log, const network_t * nwk, struct event_base * base, const vector <string> & servers) noexcept {
	// Устанавливаем объект основного фреймворка
	this->fmk = fmk;
	// Устанавливаем объект методов для работы с сетью
	this->nwk = nwk;
	// Устанавливаем объект для работы с логами
	this->log = log;
	// Устанавливаем объект базы
	this->setBase(base);
	// Добавляем список нейм-серверов
	this->setNameServers(servers);
}
/**
 * ~DNS Деструктор
 */
awh::DNS::~DNS() noexcept {
	// Выполняем удаление модуля DNS резолвера
	this->remove();
}
