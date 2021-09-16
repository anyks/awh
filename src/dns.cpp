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
		// Очищаем базу dns
		if(this->dnsbase != nullptr){
			// Если объект запроса существует то отменяем его
			if(this->reply != nullptr){
				// Выполняем отмену запроса
				evdns_getaddrinfo_cancel(this->reply);
				// Обнуляем указатель
				this->reply = nullptr;
			}
			// Очищаем базу dns
			evdns_base_free(this->dnsbase, 0);
		}
		// Создаем базу dns
		this->dnsbase = evdns_base_new(this->base, 0);
		// Если база dns не создана
		if(this->dnsbase == nullptr)
			// Выводим в лог сообщение
			this->fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, "dns base does not created");
	}
}
/**
 * callback Событие срабатывающееся при получении данных с dns сервера
 * @param errcode ошибка dns сервера
 * @param addr    структура данных с dns сервера
 * @param ctx     объект с данными для запроса
 */
void awh::DNS::callback(const int errcode, struct evutil_addrinfo * addr, void * ctx) noexcept {
	// Если данные получены
	if(ctx != nullptr){
		// Полученные ip адреса
		vector <string> ips;
		// Полученный ip адрес
		const string * ip = nullptr;
		// Получаем объект доменного имени
		domain_t * context = reinterpret_cast <domain_t *> (ctx);
		// Если данные фреймворка существуют
		if((context != nullptr) && (context->fmk != nullptr)){
			// Если возникла ошибка, выводим в лог сообщение
			if(errcode) context->fmk->log("%s %s", fmk_t::log_t::CRITICAL, context->logfile, context->host.c_str(), evutil_gai_strerror(errcode));
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
					if((ai->ai_family == context->family) || (context->family == AF_UNSPEC)){
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
			}
			// Записываем данные в кэш
			context->ips->emplace(context->host, * ip);
			// Выводим готовый результат
			if(context->callback != nullptr) context->callback(* ip);
			// Если объект запроса существует
			if(context->dns->reply != nullptr){
				// Выполняем отмену запроса
				// evdns_getaddrinfo_cancel(context->dns->reply);
				// Обнуляем указатель
				context->dns->reply = nullptr;
			}
		}
		// Удаляем передаваемый объект
		if(context != nullptr) delete context;
	}
}
/**
 * init Метод инициализации DNS резолвера
 * @param host   хост сервера
 * @param family тип интернет протокола AF_INET, AF_INET6 или AF_UNSPEC
 * @param base   объект базы событий
 * @return       база DNS резолвера
 */
struct evdns_base * awh::DNS::init(const string & host, const int family, struct event_base * base) const {
	// Результат работы функции
	struct evdns_base * result = nullptr;
	// Если база событий передана
	if((base != nullptr) && !host.empty() && !this->servers.empty()){
		// Адрес dns сервера
		string dns = "";
		// Структура запроса
		struct evutil_addrinfo hints;
		// Заполняем структуру запроса нулями
		memset(&hints, 0, sizeof(hints));
		// Создаем базу dns
		result = evdns_base_new(base, 0);
		// Переходим по всем нейм серверам и добавляем их
		for(auto & server : this->servers){
			// Определяем тип передаваемого сервера
			switch((u_short) this->nwk->parseHost(server)){
				// Если - это домен или IPv4 адрес
				case (u_short) network_t::type_t::ipv4:
				case (u_short) network_t::type_t::domain: dns = server; break;
				// Если - это IPv6 адрес, переводим ip адрес в полную форму
				case (u_short) network_t::type_t::ipv6: dns = this->nwk->setLowIp6(server); break;
			}
			// Если DNS сервер установлен, добавляем его в базу DNS
			if(!dns.empty() && (evdns_base_nameserver_ip_add(result, dns.c_str()) != 0))
				// Выводим в лог сообщение
				this->fmk->log("name server [%s] does not add", fmk_t::log_t::CRITICAL, this->logfile, dns.c_str());
		}
		// Отлавливаем ошибку
		try {
			// Создаем объект домен
			domain_t * domainData = new domain_t;
			// Устанавливаем тип протокола интернета
			domainData->family = family;
			// Запоминаем объект основного фреймворка
			domainData->fmk = this->fmk;
			// Запоминаем адрес файла для логирования
			domainData->logfile = this->logfile;
			// Запоминаем текущий объект
			domainData->dns = const_cast <dns_t *> (this);
			// Запоминаем название искомого домена
			domainData->host = (this->ips.count(host) > 0 ? this->ips.at(host) : host);
			// Запоминаем объект управления кэшем
			domainData->ips = const_cast <unordered_map <string, string> *> (&this->ips);
			// Устанавливаем тип подключения
			hints.ai_family = AF_UNSPEC;
			// Устанавливаем что это потоковый сокет
			hints.ai_socktype = SOCK_STREAM;
			// Устанавливаем что это tcp подключение
			hints.ai_protocol = IPPROTO_TCP;
			// Устанавливаем флаг подключения что это канонническое имя
			hints.ai_flags = EVUTIL_AI_CANONNAME;
			// Выполняем dns запрос
			struct evdns_getaddrinfo_request * reply = evdns_getaddrinfo(result, host.c_str(), nullptr, &hints, &DNS::callback, domainData);
			// Выводим в лог сообщение
			if(reply == nullptr) this->fmk->log("request for %s returned immediately", fmk_t::log_t::CRITICAL, this->logfile, host.c_str());
		// Если возникает ошибка выделения памяти
		} catch(const bad_alloc &) {
			// Выводим сообщение об ошибке
			this->fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, "memory could not be allocated");
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}

	}
	// Выводим результат
	return result;
}
/**
 * setLogFile Метод установки файла для сохранения логов
 * @param logfile адрес файла для сохранения логов
 */
void awh::DNS::setLogFile(const char * logfile) noexcept {
	// Если файл логов передан
	if(logfile != nullptr) this->logfile = logfile;
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
	if(!server.empty() && (this->fmk != nullptr) && (this->dnsbase != nullptr)){
		// Адрес dns сервера
		string dns = "";
		// Определяем тип передаваемого сервера
		switch((u_short) this->nwk->parseHost(server)){
			// Если - это домен или IPv4 адрес
			case (u_short) network_t::type_t::ipv4:
			case (u_short) network_t::type_t::domain: dns = server; break;
			// Если - это IPv6 адрес, переводим ip адрес в полную форму
			case (u_short) network_t::type_t::ipv6: dns = this->nwk->setLowIp6(server); break;
		}
		// Если DNS сервер установлен, добавляем его в базу DNS
		if(!dns.empty() && (evdns_base_nameserver_ip_add(this->dnsbase, dns.c_str()) != 0))
			// Выводим в лог сообщение
			this->fmk->log("name server [%s] does not add", fmk_t::log_t::CRITICAL, this->logfile, dns.c_str());
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
	if(!servers.empty() && evdns_base_clear_nameservers_and_suspend(this->dnsbase))
		// Устанавливаем новый список серверов
		this->setNameServers(servers);
}
/**
 * resolve Метод ресолвинга домена
 * @param host     хост сервера
 * @param family   тип интернет протокола IPv4 или IPv6
 * @param callback функция обратного вызова срабатывающая при получении данных
 */
void awh::DNS::resolve(const string & host, const int family, function <void (const string &)> callback){
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
				// Отлавливаем ошибку
				try {
					// Структура запроса
					struct evutil_addrinfo hints;
					// Заполняем структуру запроса нулями
					memset(&hints, 0, sizeof(hints));
					// Создаем объект домен
					domain_t * domainData = new domain_t;
					// Запоминаем текущий объект
					domainData->dns = this;
					// Запоминаем название искомого домена
					domainData->host = host;
					// Устанавливаем тип протокола интернета
					domainData->family = family;
					// Запоминаем объект основного фреймворка
					domainData->fmk = this->fmk;
					// Запоминаем объект управления кэшем
					domainData->ips = &this->ips;
					// Устанавливаем функцию обратного вызова
					domainData->callback = callback;
					// Запоминаем адрес файла для логирования
					domainData->logfile = this->logfile;
					// Устанавливаем тип подключения
					hints.ai_family = AF_UNSPEC;
					// Устанавливаем что это потоковый сокет
					hints.ai_socktype = SOCK_STREAM;
					// Устанавливаем что это tcp подключение
					hints.ai_protocol = IPPROTO_TCP;
					// Устанавливаем флаг подключения что это канонническое имя
					hints.ai_flags = EVUTIL_AI_CANONNAME;
					// Выполняем dns запрос
					this->reply = evdns_getaddrinfo(this->dnsbase, host.c_str(), nullptr, &hints, &DNS::callback, domainData);
					// Выводим в лог сообщение
					if(this->reply == nullptr) this->fmk->log("request for %s returned immediately", fmk_t::log_t::CRITICAL, this->logfile, host.c_str());
				// Если возникает ошибка выделения памяти
				} catch(const bad_alloc &) {
					// Выводим сообщение об ошибке
					this->fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, "memory could not be allocated");
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
 * @param fmk     объект фреймворка
 * @param nwk     объект методов для работы с сетью
 * @param logfile адрес файла для сохранения логов
 */
awh::DNS::DNS(const fmk_t * fmk, const network_t * nwk, const char * logfile) noexcept {
	// Устанавливаем объект основного фреймворка
	this->fmk = fmk;
	// Устанавливаем объект методов для работы с сетью
	this->nwk = nwk;
	// Устанавливаем адрес файла для сохранения логов
	this->logfile = logfile;
}
/**
 * DNS Конструктор
 * @param fmk     объект фреймворка
 * @param nwk     объект методов для работы с сетью
 * @param base    база событий
 * @param servers массив dns серверов
 * @param logfile адрес файла для сохранения логов
 */
awh::DNS::DNS(const fmk_t * fmk, const network_t * nwk, struct event_base * base, const vector <string> & servers, const char * logfile) noexcept {
	// Устанавливаем объект основного фреймворка
	this->fmk = fmk;
	// Устанавливаем объект методов для работы с сетью
	this->nwk = nwk;
	// Устанавливаем адрес файла для сохранения логов
	this->logfile = logfile;
	// Устанавливаем объект базы
	this->setBase(base);
	// Добавляем список нейм-серверов
	this->setNameServers(servers);
}
/**
 * ~DNS Деструктор
 */
awh::DNS::~DNS() noexcept {
	// Удаляем базу dns
	if(this->dnsbase != nullptr){
		// Если объект запроса существует то отменяем его
		if(this->reply != nullptr){
			// Выполняем отмену запроса
			evdns_getaddrinfo_cancel(this->reply);
			// Обнуляем указатель
			this->reply = nullptr;
		}
		// Очищаем базу dns
		evdns_base_free(this->dnsbase, 0);
		// Обнуляем указатель
		this->dnsbase = nullptr;
	}
}
