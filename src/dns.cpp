/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <dns.hpp>

/**
 * createBase Метод создания dns базы
 */
void awh::DNS::createBase() noexcept {
	// Если база событий существует
	if((this->fmk != nullptr) && (this->base != nullptr)){
		// Выполняем сброс модуля DNS резолвера
		this->reset();
		// Создаем базу dns
		this->dbase = evdns_base_new(this->base, 0);
		// Если база dns не создана
		if(this->dbase == nullptr)
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "dns base does not created");
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
		// Получаем объект доменного имени
		dom_t * dom = reinterpret_cast <dom_t *> (ctx);
		// Получаем объект модуля DNS резолвера
		dns_t * dns = const_cast <dns_t *> (dom->dns);
		// Если данные фреймворка существуют
		if((dom->fmk != nullptr) && (dom->log != nullptr)){
			// Полученные ip адреса
			vector <string> ips;
			// Если возникла ошибка, выводим в лог сообщение
			if(error) dom->log->print("%s %s", log_t::flag_t::CRITICAL, dom->host.c_str(), evutil_gai_strerror(error));
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
					if((ai->ai_family == dom->family) || (dom->family == AF_UNSPEC)){
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
						// Запоминаем полученный ip адрес
						if(ip != nullptr) ips.push_back(ip);
					}
				}
				// Очищаем структуру данных домена
				evutil_freeaddrinfo(addr);
			}
			// Полученный ip адрес
			const string * ip = nullptr;
			// Если ip адреса получены, выводим ip адрес в случайном порядке
			if(!ips.empty()){
				// Если количество элементов больше 1
				if(ips.size() > 1){
					// рандомизация генератора случайных чисел
					srand(time(0));
					// Получаем ip адрес
					ip = &ips.at(rand() % ips.size());
				// Выводим только первый элемент
				} else ip = &ips.front();
				// Записываем данные в кэш
				dns->ips.emplace(dom->host, * ip);
			}
			// Выводим готовый результат
			if(dom->callback != nullptr) dom->callback(* ip);
			// Если объект запроса существует
			if(dns->reply != nullptr){
				// Выполняем отмену запроса
				// evdns_getaddrinfo_cancel(dom->dns->reply);
				// Обнуляем указатель
				dns->reply = nullptr;
			}
		}
		// Удаляем домен из списка доменов
		dns->doms.erase(dom->id);
	}
}
/**
 * init Метод инициализации DNS резолвера
 * @param host   хост сервера
 * @param family тип интернет протокола AF_INET, AF_INET6 или AF_UNSPEC
 * @param base   объект базы событий
 * @return       база DNS резолвера
 */
struct evdns_base * awh::DNS::init(const string & host, const int family, struct event_base * base) const noexcept {
	// Результат работы функции
	struct evdns_base * result = nullptr;
	// Если база событий передана
	if((base != nullptr) && !host.empty() && !this->servers.empty()){
		// Адрес dns сервера
		string dns = "";
		// Создаем базу dns
		result = evdns_base_new(base, 0);
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : this->servers){
			// Определяем тип передаваемого сервера
			switch((uint8_t) this->nwk->parseHost(server)){
				// Если хост является доменом или IPv4 адресом
				case (uint8_t) network_t::type_t::IPV4:
				case (uint8_t) network_t::type_t::DOMNAME: dns = server; break;
				// Если хост является IPv6 адресом, переводим ip адрес в полную форму
				case (uint8_t) network_t::type_t::IPV6: dns = this->nwk->setLowIp6(server); break;
			}
			// Если DNS сервер установлен, добавляем его в базу DNS
			if(!dns.empty() && (evdns_base_nameserver_ip_add(result, dns.c_str()) != 0))
				// Выводим в лог сообщение
				this->log->print("name server [%s] does not add", log_t::flag_t::CRITICAL, dns.c_str());
		}
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаем объект домен
			dom_t dom;
			// Запоминаем название искомого домена
			dom.host = host;
			// Устанавливаем тип протокола интернета
			dom.family = family;
			// Запоминаем объект основного фреймворка
			dom.fmk = this->fmk;
			// Запоминаем объект для работы с логами
			dom.log = this->log;
			// Формируем идентификатор объекта
			dom.id = this->fmk->unixTimestamp();
			// Запоминаем текущий объект
			dom.dns = const_cast <dns_t *> (this);
			// Структура запроса
			struct evutil_addrinfo hints;
			// Заполняем структуру запроса нулями
			memset(&hints, 0, sizeof(hints));
			// Устанавливаем тип подключения
			hints.ai_family = AF_UNSPEC;
			// Устанавливаем что это потоковый сокет
			hints.ai_socktype = SOCK_STREAM;
			// Устанавливаем что это tcp подключение
			hints.ai_protocol = IPPROTO_TCP;
			// Устанавливаем флаг подключения что это канонническое имя
			hints.ai_flags = EVUTIL_AI_CANONNAME;
			// Добавляем доменное имя в список доменов
			auto ret = this->doms.emplace(dom.id, move(dom));
			// Выполняем dns запрос
			struct evdns_getaddrinfo_request * reply = evdns_getaddrinfo(result, host.c_str(), nullptr, &hints, &dns_t::callback, &ret.first->second);
			// Выводим в лог сообщение
			if(reply == nullptr) this->log->print("request for %s returned immediately", log_t::flag_t::CRITICAL, host.c_str());
		// Если возникает ошибка выделения памяти
		} catch(const bad_alloc &) {
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}

	}
	// Выводим результат
	return result;
}
/**
 * reset Метод сброса параметров модуля DNS резолвера
 */
void awh::DNS::reset() noexcept {
	// Очищаем базу dns
	if(this->dbase != nullptr){
		// Если объект запроса существует то отменяем его
		if(this->reply != nullptr){
			// Выполняем отмену запроса
			evdns_getaddrinfo_cancel(this->reply);
			// Обнуляем указатель
			this->reply = nullptr;
		}
		// Очищаем базу dns
		evdns_base_free(this->dbase, 0);
		// Зануляем базу DNS
		this->dbase = nullptr;
	}
}
/**
 * clear Метод сброса кэша резолвера
 */
void awh::DNS::clear() noexcept {
	// Выполняем сброс списка IP адресов
	this->servers.clear();
}
/**
 * updateNameServers Метод обновления списка нейм-серверов
 */
void awh::DNS::updateNameServers() noexcept {
	// Если список DNS серверов не пустой
	if(!this->servers.empty() && (this->dbase != nullptr)){
		// Выполняем очистку предыдущих DNS серверов
		evdns_base_clear_nameservers_and_suspend(this->dbase);
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : this->servers) this->setNameServer(server);
	}
}
/**
 * setBase Установка базы событий
 * @param base объект базы событий
 */
void awh::DNS::setBase(struct event_base * base) noexcept {
	// Если база передана
	if(base != nullptr){
		// Создаем базу событий
		this->base = base;
		// Создаем dns базу
		this->createBase();
	}
}
/**
 * setNameServer Метод добавления сервера dns
 * @param server ip адрес dns сервера
 */
void awh::DNS::setNameServer(const string & server) noexcept {
	// Если dns сервер передан
	if(!server.empty() && (this->fmk != nullptr) && (this->dbase != nullptr)){
		// Адрес dns сервера
		string dns = "";
		// Определяем тип передаваемого сервера
		switch((uint8_t) this->nwk->parseHost(server)){
			// Если хост является доменом или IPv4 адресом
			case (uint8_t) network_t::type_t::IPV4:
			case (uint8_t) network_t::type_t::DOMNAME: dns = server; break;
			// Если хост является IPv6 адресом, переводим ip адрес в полную форму
			case (uint8_t) network_t::type_t::IPV6: dns = this->nwk->setLowIp6(server); break;
		}
		// Если DNS сервер установлен, добавляем его в базу DNS
		if(!dns.empty() && (evdns_base_nameserver_ip_add(this->dbase, dns.c_str()) != 0))
			// Выводим в лог сообщение
			this->log->print("name server [%s] does not add", log_t::flag_t::CRITICAL, dns.c_str());
	}
}
/**
 * setNameServers Метод добавления серверов dns
 * @param server ip адреса dns серверов
 */
void awh::DNS::setNameServers(const vector <string> & servers) noexcept {
	// Если нейм сервера переданы
	if(!servers.empty()){
		// Запоминаем dns сервера
		this->servers = servers;
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : this->servers) this->setNameServer(server);
	}
}
/**
 * replaceServers Метод замены существующих серверов dns
 * @param servers ip адреса dns серверов
 */
void awh::DNS::replaceServers(const vector <string> & servers) noexcept {
	// Если нейм сервера переданы, удаляем все настроенные серверы имён и приостанавливаем все ожидающие решения
	if(!servers.empty()){
		// Если база DNS созданы
		if(this->dbase != nullptr)
			// Выполняем очистку предыдущих DNS серверов
			evdns_base_clear_nameservers_and_suspend(this->dbase);
		// Устанавливаем новый список серверов
		this->setNameServers(servers);
	}
}
/**
 * resolve Метод ресолвинга домена
 * @param host     хост сервера
 * @param family   тип интернет протокола IPv4 или IPv6
 * @param callback функция обратного вызова срабатывающая при получении данных
 */
void awh::DNS::resolve(const string & host, const int family, function <void (const string &)> callback) noexcept {
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
			// Запрашиваем данные домена из кэша
			const string & ip = (this->ips.count(host) > 0 ? this->ips.at(host) : "");
			// Если ip адрес найден тогда выводим результат
			if(!ip.empty()) callback(ip);
			// Если адрес не найден то запрашиваем его с резолвера
			else {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Создаем объект домен
					dom_t dom;
					// Запоминаем текущий объект
					dom.dns = this;
					// Запоминаем название искомого домена
					dom.host = host;
					// Устанавливаем тип протокола интернета
					dom.family = family;
					// Запоминаем объект основного фреймворка
					dom.fmk = this->fmk;
					// Запоминаем объект для работы с логами
					dom.log = this->log;
					// Устанавливаем функцию обратного вызова
					dom.callback = callback;
					// Формируем идентификатор объекта
					dom.id = this->fmk->unixTimestamp();
					// Структура запроса
					struct evutil_addrinfo hints;
					// Заполняем структуру запроса нулями
					memset(&hints, 0, sizeof(hints));
					// Устанавливаем тип подключения
					hints.ai_family = AF_UNSPEC;
					// Устанавливаем что это потоковый сокет
					hints.ai_socktype = SOCK_STREAM;
					// Устанавливаем что это tcp подключение
					hints.ai_protocol = IPPROTO_TCP;
					// Устанавливаем флаг подключения что это канонническое имя
					hints.ai_flags = EVUTIL_AI_CANONNAME;
					// Добавляем доменное имя в список доменов
					auto ret = this->doms.emplace(dom.id, move(dom));
					// Выполняем dns запрос
					this->reply = evdns_getaddrinfo(this->dbase, host.c_str(), nullptr, &hints, &dns_t::callback, &ret.first->second);
					// Выводим в лог сообщение
					if(this->reply == nullptr) this->log->print("request for %s returned immediately", log_t::flag_t::CRITICAL, host.c_str());
				// Если возникает ошибка выделения памяти
				} catch(const bad_alloc &) {
					// Выводим сообщение об ошибке
					this->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
					// Выходим из приложения
					exit(EXIT_FAILURE);
				}
			}
		// Если передан домен то возвращаем его
		} else callback(match[1].str());
	}
	// Выходим
	return;
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
	// Выполняем сброс модуля DNS резолвера
	this->reset();
}
