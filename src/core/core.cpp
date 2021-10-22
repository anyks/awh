/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <core/core.hpp>

/**
 * run Функция обратного вызова при активации базы событий
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 * @param ctx   передаваемый контекст
 */
void awh::Core::run(evutil_socket_t fd, short event, void * ctx) noexcept {
	// Если контекст модуля передан
	if(ctx != nullptr){
		// Получаем объект подключения
		core_t * core = reinterpret_cast <core_t *> (ctx);
		// Выполняем удаление событие таймера
		event_del(&core->timeout);
		// Если список воркеров существует
		if(!core->workers.empty()){
			// Переходим по всему списку воркеров
			for(auto & worker : core->workers){
				// Если функция обратного вызова установлена
				if(worker.second->openFn != nullptr)
					// Выполняем функцию обратного вызова
					worker.second->openFn(worker.first, core, worker.second->ctx);
			}
		}
		// Если функция обратного вызова установлена, выполняем
		if(core->callbackFn != nullptr) core->callbackFn(true, core, core->ctx.front());
		// Выводим в консоль информацию
		if(!core->noinfo) core->log->print("[+] start service: pid = %u", log_t::flag_t::INFO, getpid());
		// Если таймер периодического запуска коллбека активирован
		if(core->persist){
			// Создаём событие на активацию базы событий
			event_assign(&core->interval, core->base, -1, EV_TIMEOUT | EV_PERSIST, persistent, core);
			// Очищаем объект таймаута базы событий
			evutil_timerclear(&core->tvInterval);
			// Устанавливаем интервал таймаута
			core->tvInterval.tv_sec = (PERSIST_INTERVAL / 1000);
			// Создаём событие таймаута на активацию базы событий
			event_add(&core->interval, &core->tvInterval);
		}
	}
}
/**
 * reconnect Функция задержки времени на реконнект
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 * @param ctx   передаваемый контекст
 */
void awh::Core::reconnect(evutil_socket_t fd, short event, void * ctx) noexcept {
	// Если контекст модуля передан
	if(ctx != nullptr){
		// Получаем объект воркера
		worker_t * wrk = reinterpret_cast <worker_t *> (ctx);
		// Получаем объект подключения
		core_t * core = const_cast <core_t *> (wrk->core);
		// Выполняем удаление событие таймера
		event_del(&core->timeout);
		// Выполняем новое подключение
		core->connect(wrk->wid);
	}
}
/**
 * persistent Функция персистентного вызова по таймеру
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 * @param ctx   передаваемый контекст
 */
void awh::Core::persistent(evutil_socket_t fd, short event, void * ctx) noexcept {
	// Если контекст модуля передан
	if(ctx != nullptr){
		// Получаем объект подключения
		core_t * core = reinterpret_cast <core_t *> (ctx);
		// Если список воркеров существует
		if(!core->workers.empty()){
			// Переходим по всему списку воркеров
			for(auto & worker : core->workers){
				// Получаем объект воркера
				worker_t * wrk = const_cast <worker_t *> (worker.second);
				// Если функция обратного вызова установлена и адъютанты существуют
				if((wrk->persistFn != nullptr) && !wrk->adjutants.empty()){
					// Переходим по всему списку адъютантов
					for(auto & adj : wrk->adjutants){
						// Выполняем функцию обратного вызова
						wrk->persistFn(adj.first, worker.first, core, worker.second->ctx);
					}
				}
			}
		}
	}
}
/**
 * reconnect Метод запуска переподключения
 * @param wid идентификатор воркера
 */
void awh::Core::reconnect(const size_t wid) noexcept {
	// Выполняем поиск воркера
	auto it = this->workers.find(wid);
	// Если воркер найден
	if(it != this->workers.end()){
		// Получаем объект воркера
		worker_t * wrk = const_cast <worker_t *> (it->second);
		// Создаём событие на активацию базы событий
		event_assign(&this->timeout, this->base, -1, EV_TIMEOUT, reconnect, wrk);
		// Очищаем объект таймаута базы событий
		evutil_timerclear(&this->tvTimeout);
		// Устанавливаем интервал таймаута
		this->tvTimeout.tv_sec = 10;
		// Создаём событие таймаута на активацию базы событий
		event_add(&this->timeout, &this->tvTimeout);
	}
}
/**
 * clean Метод буфера событий
 * @param bev буфер событий для очистки
 */
void awh::Core::clean(struct bufferevent * bev) noexcept {
	// Если буфер событий передан
	if(bev != nullptr){
		// Запрещаем чтение запись данных серверу
		bufferevent_disable(bev, EV_WRITE | EV_READ);
		// Получаем файловый дескриптор
		evutil_socket_t fd = bufferevent_getfd(bev);
		// Если - это Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Отключаем подключение для сокета
			if(fd > 0) shutdown(fd, SD_BOTH);
		// Если - это Unix
		#else
			// Отключаем подключение для сокета
			if(fd > 0) shutdown(fd, SHUT_RDWR);
		#endif
		// Закрываем подключение
		if(fd > 0) evutil_closesocket(fd);
		// Удаляем буфер события
		bufferevent_free(bev);
		// Зануляем буфер событий
		bev = nullptr;
	}
}
/**
 * socket Метод создания сокета
 * @param ip     адрес для которого нужно создать сокет
 * @param port   порт сервера для которого нужно создать сокет
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       параметры подключения к серверу
 */
const awh::Core::socket_t awh::Core::socket(const string & ip, const u_int port, const int family) const noexcept {
	// Результат работы функции
	socket_t result;
	// Если IP адрес передан
	if(!ip.empty() && (port > 0) && (port <= 65535)){
		// Адрес сервера для биндинга
		string host = "";
		// Размер структуры подключения
		socklen_t size = 0;
		// Объект подключения
		struct sockaddr * sin = nullptr;
		// Определяем тип подключения
		switch(family){
			// Для протокола IPv4
			case AF_INET: {
				// Если ядро является клиентом
				if(this->type != type_t::SERVER){
					// Получаем список ip адресов
					auto ips = this->net.v4.first;
					// Если количество элементов больше 1
					if(ips.size() > 1){
						// рандомизация генератора случайных чисел
						srand(time(0));
						// Получаем ip адрес
						host = ips.at(rand() % ips.size());
					// Выводим только первый элемент
					} else host = ips.front();
					// Очищаем всю структуру для клиента
					memset(&result.client, 0, sizeof(result.client));
					// Устанавливаем протокол интернета
					result.client.sin_family = AF_INET;
					// Устанавливаем произвольный порт для локального подключения
					result.client.sin_port = htons(0);
					// Устанавливаем адрес для локальго подключения
					result.client.sin_addr.s_addr = inet_addr(host.c_str());
					// Запоминаем размер структуры
					size = sizeof(result.client);
					// Запоминаем полученную структуру
					sin = reinterpret_cast <struct sockaddr *> (&result.client);
				}
				// Очищаем всю структуру для сервера
				memset(&result.server, 0, sizeof(result.server));
				// Устанавливаем протокол интернета
				result.server.sin_family = AF_INET;
				// Устанавливаем порт для локального подключения
				result.server.sin_port = htons(port);
				// Устанавливаем адрес для удаленного подключения
				result.server.sin_addr.s_addr = inet_addr(ip.c_str());
				// Если ядро является сервером
				if(this->type == type_t::SERVER){
					// Запоминаем размер структуры
					size = sizeof(result.server);
					// Запоминаем полученную структуру
					sin = reinterpret_cast <struct sockaddr *> (&result.server);
				}
				// Обнуляем серверную структуру
				memset(&result.server.sin_zero, 0, sizeof(result.server.sin_zero));
				// Создаем сокет подключения
				result.fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			} break;
			// Для протокола IPv6
			case AF_INET6: {
				// Если ядро является клиентом
				if(this->type != type_t::SERVER){
					// Получаем список ip адресов
					auto ips = this->net.v6.first;
					// Если количество элементов больше 1
					if(ips.size() > 1){
						// рандомизация генератора случайных чисел
						srand(time(0));
						// Получаем ip адрес
						host = ips.at(rand() % ips.size());
					// Выводим только первый элемент
					} else host = ips.front();
					// Переводим ip адрес в полноценный вид
					host = move(this->nwk.setLowIp6(host));
					// Очищаем всю структуру для клиента
					memset(&result.client6, 0, sizeof(result.client6));
					// Устанавливаем протокол интернета
					result.client6.sin6_family = AF_INET6;
					// Устанавливаем произвольный порт для локального подключения
					result.client6.sin6_port = htons(0);
					// Указываем адрес IPv6 для клиента
					inet_pton(AF_INET6, host.c_str(), &result.client6.sin6_addr);
					// inet_ntop(AF_INET6, &result.client6.sin6_addr, hostClient, sizeof(hostClient));
					// Запоминаем размер структуры
					size = sizeof(result.client6);
					// Запоминаем полученную структуру
					sin = reinterpret_cast <struct sockaddr *> (&result.client6);
				}
				// Очищаем всю структуру для сервера
				memset(&result.server6, 0, sizeof(result.server6));
				// Устанавливаем протокол интернета
				result.server6.sin6_family = AF_INET6;
				// Устанавливаем порт для локального подключения
				result.server6.sin6_port = htons(port);
				// Указываем адрес IPv6 для сервера
				inet_pton(AF_INET6, ip.c_str(), &result.server6.sin6_addr);
				// inet_ntop(AF_INET6, &result.server6.sin6_addr, hostServer, sizeof(hostServer));
				// Если ядро является сервером
				if(this->type == type_t::SERVER){
					// Запоминаем размер структуры
					size = sizeof(result.server6);
					// Запоминаем полученную структуру
					sin = reinterpret_cast <struct sockaddr *> (&result.server6);
				}
				// Создаем сокет подключения
				result.fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			} break;
			// Если тип сети не определен
			default: {
				// Выводим сообщение в консоль
				this->log->print("network not allow from server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
				// Выходим
				return result;
			}
		}
		// Если сокет не создан то выходим
		if(result.fd < 0){
			// Выводим сообщение в консоль
			this->log->print("creating socket to server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
			// Выходим
			return socket_t();
		}
		// Устанавливаем настройки для *Nix подобных систем
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			sockets_t::noSigill(this->log);
			// Отключаем сигнал записи в оборванное подключение
			sockets_t::noSigpipe(result.fd, this->log);
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Включаем отображение сети IPv4 в IPv6
				if(family == AF_INET6) sockets_t::ipV6only(result.fd, this->ipV6only, this->log);
			// Активируем keepalive
			} else sockets_t::keepAlive(result.fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->log);
		// Устанавливаем настройки для OS Windows
		#else
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Включаем отображение сети IPv4 в IPv6
				if(family == AF_INET6) sockets_t::ipV6only(result.fd, this->ipV6only, this->log);
			// Активируем keepalive
			} else sockets_t::keepAlive(result.fd, this->log);
		#endif
		// Если ядро является сервером
		if(this->type == type_t::SERVER)
			// Переводим сокет в не блокирующий режим
			evutil_make_socket_nonblocking(result.fd);
		// Отключаем алгоритм Нейгла для сервера и клиента
		sockets_t::tcpNodelay(result.fd, this->log);
		// Устанавливаем разрешение на закрытие сокета при неиспользовании
		// evutil_make_socket_closeonexec(result.fd);
		// Устанавливаем разрешение на повторное использование сокета
		evutil_make_listen_socket_reuseable(result.fd);
		// Выполняем бинд на сокет
		if(::bind(result.fd, sin, size) < 0){
			// Выводим в лог сообщение
			this->log->print("bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
			// Выходим
			return socket_t();
		}
	}
	// Выводим результат
	return result;
}
/**
 * bind Метод подключения модуля ядра к текущей базе событий
 * @param core модуль ядра для подключения
 */
void awh::Core::bind(Core * core) noexcept {
	// Если модуль ядра передан
	if(core != nullptr){
		// Выполняем блокировку потока
		this->bloking.lock();
		// Устанавливаем базу событий
		core->base = this->base;
		// Выполняем блокировку инициализации базы событий
		core->locker = (core->base != nullptr);
		// Если блокировка базы событий выполнена
		if(core->locker){
			// Добавляем базу событий для DNS резолвера IPv4
			core->dns4.setBase(core->base);
			// Добавляем базу событий для DNS резолвера IPv6
			core->dns6.setBase(core->base);
			// Выполняем установку нейм-серверов для DNS резолвера IPv4
			core->dns4.replaceServers(core->net.v4.second);
			// Выполняем установку нейм-серверов для DNS резолвера IPv6
			core->dns6.replaceServers(core->net.v6.second);
			// Создаём событие на активацию базы событий
			event_assign(&core->timeout, core->base, -1, EV_TIMEOUT, run, core);
			// Очищаем объект таймаута базы событий
			evutil_timerclear(&core->tvTimeout);
			// Устанавливаем таймаут базы событий в 1 секунду
			core->tvTimeout.tv_sec = 1;
			// Создаём событие таймаута на активацию базы событий
			event_add(&core->timeout, &core->tvTimeout);
		}
		// Выполняем разблокировку потока
		this->bloking.unlock();
	}
}
/**
 * unbind Метод отключения модуля ядра от текущей базы событий
 * @param core модуль ядра для отключения
 */
void awh::Core::unbind(Core * core) noexcept {
	// Если модуль ядра передан
	if(core != nullptr){
		// Выполняем блокировку потока
		this->bloking.lock();
		// Если база событий подключена
		if(core->base != nullptr){
			// Отключаем всех клиентов
			core->closeAll();
			// Зануляем базу событий
			core->base = nullptr;
		}
		// Выполняем сброс модуля DNS резолвера IPv4
		core->dns4.reset();
		// Выполняем сброс модуля DNS резолвера IPv6
		core->dns6.reset();
		// Если функция обратного вызова установлена, выполняем
		if(core->callbackFn != nullptr) core->callbackFn(false, core, core->ctx.front());
		// Выполняем сброс блокировки базы событий
		core->locker = false;
		// Выполняем разблокировку потока
		this->bloking.unlock();
	}
}
/**
 * setCallback Метод установки функции обратного вызова при запуске/остановки работы модуля
 * @param ctx      передаваемый объект контекста
 * @param callback функция обратного вызова для установки
 */
void awh::Core::setCallback(void * ctx, function <void (const bool, Core * core, void *)> callback) noexcept {
	// Устанавливаем объект контекста
	this->ctx.front() = ctx;
	// Устанавливаем функцию обратного вызова
	this->callbackFn = callback;
}
/**
 * stop Метод остановки клиента
 */
void awh::Core::stop() noexcept {
	// Если система уже запущена
	if(this->mode){
		// Запрещаем работу WebSocket
		this->mode = false;
		// Выполняем удаление событие таймера
		event_del(&this->timeout);
		// Выполняем отключение всех клиентов
		this->closeAll();
		// Если таймер периодического запуска коллбека активирован
		if(this->persist){
			// Удаляем событие интервала
			event_del(&this->interval);
			// Завершаем работу базы событий
			event_base_loopbreak(this->base);
		// Нормально завершаем работу базы событий
		} else event_base_loopexit(this->base, nullptr);
	}
}

#include <if.hpp>

/**
 * start Метод запуска клиента
 */
void awh::Core::start() noexcept {
	// Если система ещё не запущена
	if(!this->mode && !this->locker){


		ifnet_t net(this->fmk, this->log);

		net.init();

		const auto & hw = net.hws();

		cout << " ********* " << hw.size() << endl;

		for(auto & item : hw) cout << " ==== " << item.first << " == " << item.second << endl;

		// 68:a8:6d:33:4c:e2 // 50:64:2b:32:fe:9c
		cout << " ++++++++= " << net.ip("vtnet0", AF_INET6) << " == " << net.mac(this->nwk.setLowIp6("fe80:1::5c15:97ff:fe60:e6f2"), AF_INET6) << endl;


		// Разрешаем работу WebSocket
		this->mode = true;
		// Создаем новую базу
		this->base = event_base_new();
		// Добавляем базу событий для DNS резолвера IPv4
		this->dns4.setBase(this->base);
		// Добавляем базу событий для DNS резолвера IPv6
		this->dns6.setBase(this->base);
		// Выполняем установку нейм-серверов для DNS резолвера IPv4
		this->dns4.replaceServers(this->net.v4.second);
		// Выполняем установку нейм-серверов для DNS резолвера IPv6
		this->dns6.replaceServers(this->net.v6.second);
		// Создаём событие на активацию базы событий
		event_assign(&this->timeout, this->base, -1, EV_TIMEOUT, run, this);
		// Очищаем объект таймаута базы событий
		evutil_timerclear(&this->tvTimeout);
		// Устанавливаем таймаут базы событий в 1 секунду
		this->tvTimeout.tv_sec = 1;
		// Создаём событие таймаута на активацию базы событий
		event_add(&this->timeout, &this->tvTimeout);
		// Запускаем работу базы событий
		event_base_dispatch(this->base);
		// Выполняем сброс модуля DNS резолвера IPv4
		this->dns4.reset();
		// Выполняем сброс модуля DNS резолвера IPv6
		this->dns6.reset();
		// Выполняем отключение всех адъютантов
		this->closeAll();
		// Удаляем объект базы событий
		event_base_free(this->base);
		// Очищаем все глобальные переменные
		libevent_global_shutdown();
		// Если функция обратного вызова установлена, выполняем
		if(this->callbackFn != nullptr) this->callbackFn(false, this, this->ctx.front());
		// Выводим в консоль информацию
		if(!this->noinfo) this->log->print("[-] stop service: pid = %u", log_t::flag_t::INFO, getpid());
	}
}
/**
 * working Метод проверки на запуск работы
 * @return результат проверки
 */
bool awh::Core::working() const noexcept {
	// Выводим результат проверки
	return this->mode;
}
/**
 * add Метод добавления воркера в биндинг
 * @param worker воркер для добавления
 * @return       идентификатор воркера в биндинге
 */
size_t awh::Core::add(const worker_t * worker) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Выполняем блокировку потока
	this->bloking.lock();
	// Если воркер передан и URL адрес существует
	if(worker != nullptr){
		// Получаем объект воркера
		worker_t * wrk = const_cast <worker_t *> (worker);
		// Получаем идентификатор воркера
		result = (1 + this->workers.size());
		// Устанавливаем родительский объект
		wrk->core = this;
		// Устанавливаем идентификатор воркера
		wrk->wid = result;
		// Добавляем воркер в список
		this->workers.emplace(result, wrk);
	}
	// Выполняем разблокировку потока
	this->bloking.unlock();
	// Выводим результат
	return result;
}
/**
 * closeAll Метод отключения всех воркеров
 */
void awh::Core::closeAll() noexcept {
	// Если список подключений активен
	if(!this->workers.empty()){
		// Переходим по всему списку подключений
		for(auto & worker : this->workers){
			// Если в воркере есть подключённые клиенты
			if(!worker.second->adjutants.empty()){
				// Получаем объект воркера
				worker_t * wrk = const_cast <worker_t *> (worker.second);
				// Переходим по всему списку адъютанта
				for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
					// Получаем объект адъютанта
					worker_t::adj_t * adj = const_cast <worker_t::adj_t *> (&it->second);
					// Выполняем очистку буфера событий
					this->clean(adj->bev);
					// Выводим функцию обратного вызова
					if(wrk->disconnectFn != nullptr)
						// Выполняем функцию обратного вызова
						wrk->disconnectFn(it->first, worker.first, this, wrk->ctx);
					// Удаляем адъютанта из списка
					it = wrk->adjutants.erase(it);
				}
			}
		}
	}
	// Выполняем очистку списка адъютантов
	this->adjutants.clear();
}
/**
 * removeAll Метод удаления всех воркеров
 */
void awh::Core::removeAll() noexcept {
	// Выполняем отключение всех клиентов
	this->closeAll();
	// Выполняем удаление всех воркеров
	this->workers.clear();
}
/**
 * remove Метод удаления воркера из биндинга
 * @param wid идентификатор воркера
 */
void awh::Core::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден
		if(it != this->workers.end()) this->workers.erase(it);
	}
}
/**
 * close Метод закрытия подключения воркера
 * @param aid идентификатор адъютанта
 */
void awh::Core::close(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		worker_t::adj_t * adj = const_cast <worker_t::adj_t *> (it->second);
		// Получаем объект воркера
		worker_t * wrk = const_cast <worker_t *> (adj->parent);
		// Если событие сервера существует
		if(adj->bev != nullptr){
			// Выполняем очистку буфера событий
			this->clean(adj->bev);
			// Устанавливаем что событие удалено
			adj->bev = nullptr;
		}
		// Выполняем удаление контекста SSL
		this->ssl.clear(adj->ssl);
		// Удаляем адъютанта из списка адъютантов
		wrk->adjutants.erase(aid);
		// Удаляем адъютанта из списка подключений
		this->adjutants.erase(aid);
		// Выводим сообщение об ошибке
		if(!this->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
		// Выводим функцию обратного вызова
		if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this, wrk->ctx);
	}
}
/**
 * write Метод записи буфера данных воркером
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param aid    идентификатор адъютанта
 */
void awh::Core::write(const char * buffer, const size_t size, const size_t aid) noexcept {
	// Если данные переданы
	if((buffer != nullptr) && (size > 0)){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем максимальное количество байт для детекции
			const size_t max = (it->second->markWrite.max > 0 ? it->second->markWrite.max : 0);
			// Получаем минимальное количество байт для детекции
			const size_t min = (it->second->markWrite.min > 0 ? it->second->markWrite.min : size);
			// Устанавливаем размер записываемых данных
			bufferevent_setwatermark(it->second->bev, EV_WRITE, min, max);
			// Активируем разрешение на запись и чтение
			bufferevent_enable(it->second->bev, EV_WRITE);
			// Отправляем серверу сообщение
			bufferevent_write(it->second->bev, buffer, size);
		}
	}
}
/**
 * setLockMethod Метод блокировки метода режима работы
 * @param method метод режима работы
 * @param mode   флаг блокировки метода
 * @param aid    идентификатор адъютанта
 */
void awh::Core::setLockMethod(const method_t method, const bool mode, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Определяем метод режима работы
		switch((uint8_t) method){
			// Режим работы ЧТЕНИЕ
			case (uint8_t) method_t::READ: {
				// Если нужно разблокировать метод
				if(mode)
					// Активируем разрешение на запись и чтение
					bufferevent_enable(it->second->bev, EV_READ);
				// Если нужно заблокировать метод
				else bufferevent_disable(it->second->bev, EV_READ);
			} break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE:
				// Если нужно разблокировать метод
				if(mode)
					// Активируем разрешение на запись и чтение
					bufferevent_enable(it->second->bev, EV_WRITE);
				// Если нужно заблокировать метод
				else bufferevent_disable(it->second->bev, EV_WRITE);
			break;
		}
	}
}
/**
 * setTimeout Метод установки таймаута ожидания появления данных
 * @param method  метод режима работы
 * @param seconds время ожидания в секундах
 * @param aid     идентификатор адъютанта
 */
void awh::Core::setTimeout(const method_t method, const time_t seconds, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Определяем метод режима работы
		switch((uint8_t) method){
			// Режим работы ЧТЕНИЕ
			case (uint8_t) method_t::READ: const_cast <worker_t::adj_t *> (it->second)->timeRead = seconds; break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE: const_cast <worker_t::adj_t *> (it->second)->timeWrite = seconds; break;
		}
		// Устанавливаем таймаут ожидания поступления данных
		struct timeval readTimeout = {it->second->timeRead, 0};
		// Устанавливаем таймаут ожидания записи данных
		struct timeval writeTimeout = {it->second->timeWrite, 0};
		// Устанавливаем таймаут получения данных
		bufferevent_set_timeouts(
			it->second->bev,
			(it->second->timeRead > 0 ? &readTimeout : nullptr),
			(it->second->timeWrite > 0 ? &writeTimeout : nullptr)
		);
	}
}
/**
 * setMark Метод установки маркера на размер детектируемых байт
 * @param method метод режима работы
 * @param min    минимальный размер детектируемых байт
 * @param min    максимальный размер детектируемых байт
 * @param aid    идентификатор адъютанта
 */
void awh::Core::setMark(const method_t method, const size_t min, const size_t max, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Определяем метод режима работы
		switch((uint8_t) method){
			// Режим работы ЧТЕНИЕ
			case (uint8_t) method_t::READ: {
				// Устанавливаем минимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markRead.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markRead.max = max;
				// Устанавливаем размер считываемых данных
				bufferevent_setwatermark(it->second->bev, EV_READ, it->second->markRead.min, it->second->markRead.max);
			} break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE: {
				// Устанавливаем минимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markWrite.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markWrite.max = max;
				// Устанавливаем размер записываемых данных
				bufferevent_setwatermark(it->second->bev, EV_WRITE, it->second->markWrite.min, it->second->markWrite.max);
			} break;
		}
	}
}
/**
 * setDefer Метод установки флага отложенных вызовов событий сокета
 * @param mode флаг отложенных вызовов событий сокета
 */
void awh::Core::setDefer(const bool mode) noexcept {
	// Устанавливаем флаг отложенных вызовов событий сокета
	this->defer = mode;
}
/**
 * setNoInfo Метод установки флага запрета вывода информационных сообщений
 * @param mode флаг запрета вывода информационных сообщений
 */
void awh::Core::setNoInfo(const bool mode) noexcept {
	// Устанавливаем флаг запрета вывода информационных сообщений
	this->noinfo = mode;
}
/**
 * setPersist Метод установки персистентного флага
 * @param mode флаг персистентного запуска каллбека
 */
void awh::Core::setPersist(const bool mode) noexcept {
	// Выполняем установку флага персистентного запуска каллбека
	this->persist = mode;
}
/**
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::setVerifySSL(const bool mode) noexcept {
	// Выполняем установку флага проверки домена
	this->ssl.setVerify(mode);
}
/**
 * setFamily Метод установки тип протокола интернета
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Core::setFamily(const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
}
/**
 * setCA Метод установки CA-файла корневого SSL сертификата
 * @param cafile адрес CA-файла
 * @param capath адрес каталога где находится CA-файл
 */
void awh::Core::setCA(const string & cafile, const string & capath) noexcept {
	// Устанавливаем адрес CA-файла
	this->ssl.setCA(cafile, capath);
}
/**
 * setNet Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Core::setNet(const vector <string> & ip, const vector <string> & ns, const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
	// Определяем тип интернет-протокола
	switch(this->net.family){
		// Если - это интернет-протокол IPv4
		case AF_INET: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v4.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v4.second.assign(ns.cbegin(), ns.cend());
		} break;
		// Если - это интернет-протокол IPv6
		case AF_INET6: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v6.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v6.second.assign(ns.cbegin(), ns.cend());
		} break;
	}
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Выполняем остановку сервиса
	this->stop();
}
