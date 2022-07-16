/**
 * @file: core.cpp
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
#include <core/core.hpp>

/**
 * callback Функция обратного вызова
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::Core::Timer::callback(ev::timer & timer, int revents) noexcept {
	// Если функция обратного вызова установлена
	if(this->fn != nullptr) this->fn(this->id, this->core, this->ctx);
	// Выполняем остановку таймера
	timer.stop();
	// Если персистентная работа не установлена, удаляем таймер
	if(!this->persist){
		// Если родительский объект установлен
		if(this->core != nullptr)
			// Удаляем объект таймера
			this->core->timers.erase(this->id);
	// Если нужно продолжить работу таймера
	} else timer.start(this->delay);
}
/**
 * kick Метод отправки пинка
 */
void awh::Core::Dispatch::kick() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx);
	// Выполняем остановку всех событий
	this->base.break_loop(ev::how_t::ALL);
}
/**
 * stop Метод остановки чтения базы событий
 */
void awh::Core::Dispatch::stop() noexcept {
	// Если чтение базы событий уже началось
	if(this->work){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx);
		// Снимаем флаг работы модуля
		this->work = false;
		// Выполняем пинок
		this->kick();
	}
}
/**
 * start Метод запуска чтения базы событий
 */
void awh::Core::Dispatch::start() noexcept {
	// Если чтение базы событий ещё не началось
	if(!this->work){
		// Выполняем блокировку потока
		this->mtx.lock();
		// Устанавливаем флаг работы модуля
		this->work = true;
		// Выполняем разблокировку потока
		this->mtx.unlock();
		// Выполняем чтение базы событий пока это разрешено
		while(this->work){
			// Выполняем чтение базы событий
			if(!this->mode) this->base.run();
			// Если чтение базы событий запрещено
			else {
				// Выполняем остановку всех событий
				this->base.break_loop(ev::how_t::ALL);
				// Замораживаем поток на период времени частоты обновления базы событий
				this_thread::sleep_for(10ms);
			}
		}
	}
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации
 */
void awh::Core::Dispatch::freeze(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx);
	// Выполняем фриз получения данных
	this->mode = mode;
	// Если запрещено использовать простое чтение базы событий
	if(this->mode)
		// Выполняем остановку всех событий
		this->base.break_loop(ev::how_t::ALL);
}
/**
 * setBase Метод установки базы событий
 * @param base база событий
 */
void awh::Core::Dispatch::setBase(struct ev_loop * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx);
	// Выполняем заморозку получения данных
	if(this->work) this->freeze(true);
	// Устанавливаем базу событий
	this->base = ev::loop_ref(base);
	// Выполняем раззаморозку получения данных
	if(this->work) this->freeze(false);
}
/**
 * launching Метод вызова при активации базы событий
 */
void awh::Core::launching() noexcept {
	// Если база событий создана
	if(this->base != nullptr){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.start);
		// Устанавливаем статус сетевого ядра
		this->status = status_t::START;
		// Если список воркеров существует
		if(!this->workers.empty()){
			// Переходим по всему списку воркеров
			for(auto & worker : this->workers){
				// Если функция обратного вызова установлена
				if(worker.second->openFn != nullptr)
					// Выполняем функцию обратного вызова
					worker.second->openFn(worker.first, this, worker.second->ctx);
			}
		}
		// Если функция обратного вызова установлена, выполняем
		if(this->callbackFn != nullptr) this->callbackFn(true, this, this->ctx.front());
		// Выводим в консоль информацию
		if(!this->noinfo) this->log->print("[+] start service: pid = %u", log_t::flag_t::INFO, getpid());
		// Если таймер периодического запуска коллбека активирован, запускаем персистентную работу
		if(this->persist){
			// Устанавливаем базу событий
			this->timer.set(this->base);
			// Устанавливаем функцию обратного вызова
			this->timer.set <core_t, &core_t::persistent> (this);
			// Запускаем работу таймера
			this->timer.start(this->persistInterval / (float) 1000.f);
		}
	}
}
/**
 * persistent Функция персистентного вызова по таймеру
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::Core::persistent(ev::periodic & timer, int revents) noexcept {
	// Если список воркеров существует
	if(!this->workers.empty()){
		// Список идентификаторов адъютантов
		vector <size_t> ids;
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers){
			// Получаем объект воркера
			worker_t * wrk = const_cast <worker_t *> (worker.second);
			// Если функция обратного вызова установлена и адъютанты существуют
			if((wrk->persistFn != nullptr) && !wrk->adjutants.empty()){
				// Выполняем очистку списка адъютантов
				ids.clear();
				// Переходим по всему списку адъютантов и формируем список их идентификаторов
				for(auto & adj : wrk->adjutants) ids.push_back(adj.first);
				// Переходим по всему списку адъютантов и отсылаем им сообщение
				for(auto & aid : ids)
					// Выполняем функцию обратного вызова
					wrk->persistFn(aid, worker.first, this, worker.second->ctx);
			}
		}
	}
}
/**
 * clean Метод буфера событий
 * @param bev буфер событий для очистки
 */
void awh::Core::clean(worker_t::bev_t & bev) noexcept {
	// Выполняем блокировку на чтение/запись данных
	bev.locked = worker_t::locked_t();
	// Выполняем остановку чтения буфера событий
	ev_io_stop(this->base, &bev.event.read);
	// Выполняем остановку записи буфера событий
	ev_io_stop(this->base, &bev.event.write);
	// Если сокет активен
	if(bev.socket > 0){
		// Если - это Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Запрещаем работу с сокетом
			shutdown(bev.socket, SD_BOTH);
			// Выполняем закрытие сокета
			closesocket(bev.socket);
		// Если - это Unix
		#else
			// Запрещаем работу с сокетом
			shutdown(bev.socket, SHUT_RDWR);
			// Выполняем закрытие сокета
			close(bev.socket);
		#endif
		// Выполняем сброс сокета
		bev.socket = -1;
	}
}
/**
 * sockaddr Метод создания адресного пространства сокета
 * @param ip     адрес для которого нужно создать сокет
 * @param port   порт сервера для которого нужно создать сокет
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       параметры подключения к серверу
 */
const awh::Core::sockaddr_t awh::Core::sockaddr(const string & ip, const u_int port, const int family) const noexcept {
	// Результат работы функции
	sockaddr_t result;
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
			return sockaddr_t();
		}
		// Устанавливаем настройки для *Nix подобных систем
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			this->socket.noSigill();
			// Отключаем сигнал записи в оборванное подключение
			this->socket.noSigpipe(result.fd);
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Включаем отображение сети IPv4 в IPv6
				if(family == AF_INET6) this->socket.ipV6only(result.fd, this->ipV6only);
			// Активируем keepalive
			} else this->socket.keepAlive(result.fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl);
		// Устанавливаем настройки для OS Windows
		#else
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Включаем отображение сети IPv4 в IPv6
				if(family == AF_INET6) this->socket.ipV6only(result.fd, this->ipV6only);
			// Активируем keepalive
			} else this->socket.keepAlive(result.fd);
		#endif
		// Если ядро является сервером
		if(this->type == type_t::SERVER)
			// Переводим сокет в не блокирующий режим
			this->socket.nonBlocking(result.fd);
		// Отключаем алгоритм Нейгла для сервера и клиента
		this->socket.tcpNodelay(result.fd);
		// Устанавливаем разрешение на закрытие сокета при неиспользовании
		// this->socket.closeonexec(result.fd);
		// Устанавливаем разрешение на повторное использование сокета
		this->socket.reuseable(result.fd);
		// Выполняем бинд на сокет
		if(::bind(result.fd, sin, size) < 0){
			// Выводим в лог сообщение
			this->log->print("bind local network [%s]", log_t::flag_t::CRITICAL, host.c_str());
			// Выходим
			return sockaddr_t();
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
	/*
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->mtx.bind);
	// Если модуль ядра передан
	if((core != nullptr) && (this->base != nullptr)){
		// Если база событий активна и она отличается от текущей базы событий
		if((core->base != nullptr) && (core->base != this->base)){
			// Выполняем остановку базы событий
			core->stop();
			// Выполняем блокировку потока
			this->mtx.core.lock();
			// Добавляем ядро в список подключенных ядер
			this->cores.emplace(core, &core->base);
			// Выполняем разблокировку потока
			this->mtx.core.unlock();
		}
		// Выполняем блокировку потока
		this->mtx.start.lock();
		// Устанавливаем флаг запуска
		core->mode = true;
		// Если база событий не установлена
		if(core->base != this->base){
			// Блокируем информационные сообщения для клиента
			core->noinfo = true;
			// Устанавливаем базу событий
			core->base = this->base;
			// Добавляем базу событий для DNS резолвера IPv4
			core->dns4.setBase(core->base);
			// Добавляем базу событий для DNS резолвера IPv6
			core->dns6.setBase(core->base);
			// Выполняем установку нейм-серверов для DNS резолвера IPv4
			core->dns4.replaceServers(core->net.v4.second);
			// Выполняем установку нейм-серверов для DNS резолвера IPv6
			core->dns6.replaceServers(core->net.v6.second);
		}
		// Выполняем разблокировку потока
		this->mtx.start.unlock();
		// Выполняем запуск управляющей функции
		core->launching();
	}
	*/
}
/**
 * unbind Метод отключения модуля ядра от текущей базы событий
 * @param core модуль ядра для отключения
 */
void awh::Core::unbind(Core * core) noexcept {
	/*
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->mtx.bind);
	// Если модуль ядра передан
	if((core != nullptr) && (core->base == this->base) && (core != this)){
		// Выполняем блокировку потока
		this->mtx.stop.lock();
		// Запрещаем работу WebSocket
		core->mode = false;
		// Выполняем разблокировку потока
		this->mtx.stop.unlock();
		// Выполняем блокировку инициализации базы событий
		core->dispatch.freeze(true);
		// Выполняем остановку всех таймеров
		core->clearTimers();
		// Выполняем блокировку потока
		this->mtx.stop.lock();
		// Если таймер периодического запуска коллбека активирован
		if(core->persist){
			// Очищаем объект таймаута базы событий
			evutil_timerclear(&core->event.tv);
			// Удаляем событие интервала
			event_del(&core->event.ev);
		}
		// Выполняем разблокировку потока
		this->mtx.stop.unlock();
		// Отключаем всех клиентов
		core->close();
		// Зануляем базу событий
		core->base = nullptr;
		// Выполняем удаление модуля DNS резолвера IPv4
		core->dns4.remove();
		// Выполняем удаление модуля DNS резолвера IPv6
		core->dns6.remove();
		// Размораживаем работу базы событий
		core->dispatch.freeze(false);
		// Выполняем блокировку потока
		this->mtx.stop.lock();
		// Устанавливаем статус сетевого ядра
		core->status = status_t::STOP;
		// Выполняем разблокировку потока
		this->mtx.stop.unlock();
		// Выполняем блокировку потока
		this->mtx.core.lock();
		// Ищем ядро в списке подключённых ядер
		auto it = this->cores.find(core);
		// Если ядро в списке подключённых ядер найдено
		if(it != this->cores.end()){
			// Восстанавливаем базу событий для данного ядра
			core->base = (* it->second);
			// Удаляем ядро из списка ядер
			this->cores.erase(it);
		}
		// Выполняем разблокировку потока
		this->mtx.core.unlock();
		// Если функция обратного вызова установлена, выполняем
		if(core->callbackFn != nullptr) core->callbackFn(false, core, core->ctx.front());
	}
	*/
}
/**
 * setCallback Метод установки функции обратного вызова при запуске/остановки работы модуля
 * @param ctx      передаваемый объект контекста
 * @param callback функция обратного вызова для установки
 */
void awh::Core::setCallback(void * ctx, function <void (const bool, Core * core, void *)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Устанавливаем объект контекста
	this->ctx.front() = ctx;
	// Устанавливаем функцию обратного вызова
	this->callbackFn = callback;
}
/**
 * stop Метод остановки клиента
 */
void awh::Core::stop() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.stop);
	// Если система уже запущена
	if(this->mode && (this->base != nullptr)){
		// Запрещаем работу WebSocket
		this->mode = false;
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		this->clearTimers();
		// Если таймер периодического запуска коллбека активирован
		if(this->persist)
			// Останавливаем работу персистентного таймера
			this->timer.stop();
		// Выполняем отключение всех клиентов
		this->close();
		// Выполняем остановку чтения базы событий
		this->dispatch.stop();
	}
}
/**
 * start Метод запуска клиента
 */
void awh::Core::start() noexcept {
	// Если система ещё не запущена
	if(!this->mode && (this->base != nullptr)){
		// Выполняем блокировку потока
		this->mtx.start.lock();
		// Разрешаем работу WebSocket
		this->mode = !this->mode;
		/*
		// Определяем тип подключения
		switch(this->net.family){
			// Резолвер IPv4, создаём резолвер
			case AF_INET: {
				// Добавляем базу событий для DNS резолвера IPv4
				this->dns4.setBase(this->base);
				// Выполняем установку нейм-серверов для DNS резолвера IPv4
				this->dns4.replaceServers(this->net.v4.second);
			} break;
			// Резолвер IPv6, создаём резолвер
			case AF_INET6: {
				// Добавляем базу событий для DNS резолвера IPv6
				this->dns4.setBase(this->base);
				// Выполняем установку нейм-серверов для DNS резолвера IPv6
				this->dns4.replaceServers(this->net.v4.second);
			} break;
		}
		*/
		// Выполняем разблокировку потока
		this->mtx.start.unlock();
		// Выполняем запуск управляющей функции
		this->launching();
		// Выполняем запуск чтения базы событий
		this->dispatch.start();
		// Выполняем отключение всех адъютантов
		this->close();
		// Выполняем блокировку потока
		this->mtx.stop.lock();
		// Устанавливаем статус сетевого ядра
		this->status = status_t::STOP;
		// Выполняем разблокировку потока
		this->mtx.stop.unlock();
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
	// Если воркер передан и URL адрес существует
	if(worker != nullptr){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.worker);
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
	// Выводим результат
	return result;
}
/**
 * close Метод отключения всех воркеров
 */
void awh::Core::close() noexcept {
	/** Реализация метода не требуется **/
}
/**
 * remove Метод удаления всех воркеров
 */
void awh::Core::remove() noexcept {
	/** Реализация метода не требуется **/
}
/**
 * run Метод запуска сервера воркером
 * @param wid идентификатор воркера
 */
void awh::Core::run(const size_t wid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) wid;
}
/**
 * open Метод открытия подключения воркером
 * @param wid идентификатор воркера
 */
void awh::Core::open(const size_t wid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) wid;
}
/**
 * remove Метод удаления воркера из биндинга
 * @param wid идентификатор воркера
 */
void awh::Core::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.worker);
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
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
}
/**
 * setBandwidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::Core::setBandwidth(const size_t aid, const string & read, const string & write) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
	(void) read;
	(void) write;
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Core::rebase() noexcept {
	/*
	// Если система уже запущена
	if(this->mode && (this->base != nullptr)){
	*/
		/**
		 * Timer Структура таймера
		 */
	/*
		typedef struct Timer {
			void * ctx;                                               // Передаваемый контекст
			time_t delay;                                             // Задержка времени в миллисекундах
			function <void (const u_short, Core *, void *)> callback; // Функция обратного вызова
	*/
			/**
			 * Timer Конструктор
			 */
	/*
			Timer() noexcept : ctx(nullptr), delay(0), callback(nullptr) {}
		} timer_t;
		// Список дочерних таймеров
		multimap <Core *, timer_t> childTimers;
		// Список пересоздаваемых таймеров
		vector <timer_t> mainTimers(this->timers.size());
	*/
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
	/*
		if(!this->timers.empty()){
			// Индекс текущего таймера
			size_t index = 0;
			// Переходим по всем таймерам
			for(auto it = this->timers.begin(); it != this->timers.end();){
				// Выполняем блокировку потока
				this->mtx.timer.lock();
				// Очищаем объект таймаута базы событий
				evutil_timerclear(&it->second.event.tv);
				// Удаляем событие таймера
				event_del(&it->second.event.ev);
				// Устанавливаем передаваемый контекст
				mainTimers.at(index).ctx = it->second.ctx;
				// Устанавливаем задержку времени в миллисекундах
				mainTimers.at(index).delay = it->second.delay;
				// Устанавливаем функцию обратного вызова
				mainTimers.at(index).callback = it->second.callback;
				// Удаляем таймер из списка
				it = this->timers.erase(it);
				// Выполняем разблокировку потока
				this->mtx.timer.unlock();
				// Увеличиваем значение индекса
				index++;
			}
		}
		// Выполняем отключение всех клиентов
		this->close();
		// Если список подключённых ядер не пустой
		if(!this->cores.empty()){
			// Выполняем блокировку потока
			this->mtx.core.lock();
			// Переходим по всему списку ядер
			for(auto & core : this->cores){
				// Выполняем пересоздание всех баз дочерних объектов
				core.first->close();
	*/
				/**
				 * Если запрещено использовать простое чтение базы событий
				 * Выполняем остановку всех таймеров
				 */
	/*
				if(!core.first->timers.empty()){
					// Переходим по всем таймерам
					for(auto it = core.first->timers.begin(); it != core.first->timers.end();){
						// Создаём объект таймера
						timer_t timer;
						// Выполняем блокировку потока
						core.first->mtx.timer.lock();
						// Очищаем объект таймаута базы событий
						evutil_timerclear(&it->second.event.tv);
						// Удаляем событие таймера
						event_del(&it->second.event.ev);
						// Устанавливаем передаваемый контекст
						timer.ctx = it->second.ctx;
						// Устанавливаем задержку времени в миллисекундах
						timer.delay = it->second.delay;
						// Устанавливаем функцию обратного вызова
						timer.callback = it->second.callback;
						// Добавляем таймер в список таймеров
						childTimers.emplace(core.first, move(timer));
						// Удаляем таймер из списка
						it = core.first->timers.erase(it);
						// Выполняем разблокировку потока
						core.first->mtx.timer.unlock();
					}
				}
				// Если таймер периодического запуска коллбека активирован
				if(core.first->persist){
					// Очищаем объект таймаута базы событий
					evutil_timerclear(&core.first->event.tv);
					// Удаляем событие интервала
					event_del(&core.first->event.ev);
				}
				// Выполняем ожидание завершения работы потоков
				if(core.first->multi) core.first->pool.wait();
				// Устанавливаем статус сетевого ядра
				core.first->status = status_t::STOP;
				// Если функция обратного вызова установлена, выполняем
				if(core.first->callbackFn != nullptr) core.first->callbackFn(false, core.first, core.first->ctx.front());
			}
			// Выполняем разблокировку потока
			this->mtx.core.unlock();
		}
		// Выполняем ожидание завершения работы потоков
		if(this->multi) this->pool.wait();
		// Выполняем остановку работы
		this->stop();
		// Выполняем блокировку потока
		this->mtx.main.lock();
		// Удаляем объект базы событий
		event_base_free(this->base);
		// Обнуляем базу событий
		this->base = nullptr;		
		// Создаем новую базу
		this->base = event_base_new();
		// Выполняем разблокировку потока
		this->mtx.main.unlock();
		// Выполняем установку базы событий
		this->dispatch.setBase(&this->base);
		// Если база событий создана
		if(this->base != nullptr){
			// Если список таймеров получен
			if(!mainTimers.empty()){
				// Переходим по всему списку таймеров
				for(auto & timer : mainTimers)
					// Создаём новый таймер
					this->setTimeout(timer.ctx, timer.delay, timer.callback);
				// Выполняем очистку списка таймеров
				mainTimers.clear();
				// Выполняем освобождение выделенной памяти
				vector <timer_t> ().swap(mainTimers);
			}
			// Если список подключённых ядер не пустой
			if(!this->cores.empty()){
				// Выполняем блокировку потока
				this->mtx.core.lock();
				// Переходим по всему списку ядер
				for(auto & core : this->cores){
					// Устанавливаем базу событий
					core.first->base = this->base;
					// Добавляем базу событий для DNS резолвера IPv4
					core.first->dns4.setBase(core.first->base);
					// Добавляем базу событий для DNS резолвера IPv6
					core.first->dns6.setBase(core.first->base);
					// Выполняем установку нейм-серверов для DNS резолвера IPv4
					core.first->dns4.replaceServers(core.first->net.v4.second);
					// Выполняем установку нейм-серверов для DNS резолвера IPv6
					core.first->dns6.replaceServers(core.first->net.v6.second);
					// Выполняем поиск текущего ядра
					auto ret = childTimers.equal_range(core.first);
					// Выполняем перебор всех активных таймеров
					for(auto it = ret.first; it != ret.second; ++it)
						// Создаём новый таймер
						core.first->setTimeout(it->second.ctx, it->second.delay, it->second.callback);
					// Выполняем запуск управляющей функции
					core.first->launching();
				}
				// Выполняем очистку списка таймеров
				childTimers.clear();
				// Выполняем освобождение выделенной памяти
				multimap <Core *, timer_t> ().swap(childTimers);
				// Выполняем разблокировку потока
				this->mtx.core.unlock();
			}
			// Выполняем запуск работы
			this->start();
		// Выходим принудительно из приложения
		} else exit(EXIT_FAILURE);
	}
	*/
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
		// Если адъютант получен и запись в сокет разрешена
		if((it != this->adjutants.end()) && !it->second->bev.locked.write && (it->second->bev.socket > -1)){
			// Если размер записываемых данных соответствует
			if(size >= it->second->markWrite.min){
				// Если количество записываемых данных менье максимального занчения
				if(size <= it->second->markWrite.max)
					// Выполняем отправку сообщения в сокет
					send(it->second->bev.socket, buffer, size, 0);
				// Иначе выполняем дробление передаваемых данных
				else {
					// Смещение в буфере и отправляемый размер данных
					size_t offset = 0, actual = 0;
					// Выполняем отправку данных пока всё не отправим
					while((size - offset) > 0){
						// Определяем размер отправляемых данных
						actual = ((size - offset) >= it->second->markWrite.max ? it->second->markWrite.max : (size - offset));
						// Выполняем отправку сообщения в сокет
						send(it->second->bev.socket, buffer + offset, actual, 0);
						// Увеличиваем смещение в буфере
						offset += actual;
					}
				}
			}
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
					// Активируем разрешение на чтение
					const_cast <worker_t::adj_t *> (it->second)->bev.locked.read = false;
				// Если нужно заблокировать метод
				else const_cast <worker_t::adj_t *> (it->second)->bev.locked.read = true;
			} break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE:
				// Если нужно разблокировать метод
				if(mode)
					// Активируем разрешение на запись
					const_cast <worker_t::adj_t *> (it->second)->bev.locked.write = false;
				// Если нужно заблокировать метод
				else const_cast <worker_t::adj_t *> (it->second)->bev.locked.write = true;
			break;
		}
	}
}
/**
 * setDataTimeout Метод установки таймаута ожидания появления данных
 * @param method  метод режима работы
 * @param seconds время ожидания в секундах
 * @param aid     идентификатор адъютанта
 */
void awh::Core::setDataTimeout(const method_t method, const time_t seconds, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Определяем метод режима работы
		switch((uint8_t) method){
			// Режим работы ЧТЕНИЕ
			case (uint8_t) method_t::READ:
				// Устанавливаем время ожидания на входящие данные
				const_cast <worker_t::adj_t *> (it->second)->timeRead = seconds;
			break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE:
				// Устанавливаем время ожидания на исходящие данные
				const_cast <worker_t::adj_t *> (it->second)->timeWrite = seconds;
			break;
		}
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
			} break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE: {
				// Устанавливаем минимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markWrite.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markWrite.max = max;
			} break;
		}
	}
}
/**
 * clearTimers Метод очистки всех таймеров
 */
void awh::Core::clearTimers() noexcept {
	// Если список таймеров существует
	if(!this->timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.timer);
		// Переходим по всем таймерам
		for(auto it = this->timers.begin(); it != this->timers.end();){
			// Выполняем остановку таймера
			it->second->timer.stop();
			// Удаляем таймер из списка
			it = this->timers.erase(it);
		}
	}
}
/**
 * clearTimer Метод очистки таймера
 * @param id идентификатор таймера для очистки
 */
void awh::Core::clearTimer(const u_short id) noexcept {
	// Если список таймеров существует
	if(!this->timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.timer);
		// Выполняем поиск идентификатора таймера
		auto it = this->timers.find(id);
		// Если идентификатор таймера найден
		if(it != this->timers.end()){
			// Выполняем остановку таймера
			it->second->timer.stop();
			// Удаляем объект таймера
			this->timers.erase(it);
		}
	}
}
/**
 * setTimeout Метод установки таймаута
 * @param ctx      передаваемый контекст
 * @param delay    задержка времени в миллисекундах
 * @param callback функция обратного вызова
 * @return         идентификатор созданного таймера
 */
u_short awh::Core::setTimeout(void * ctx, const time_t delay, function <void (const u_short, Core *, void *)> callback) noexcept {
	// Результат работы функции
	u_short result = 0;
	// Если данные переданы
	if((delay > 0) && (callback != nullptr) && (this->base != nullptr)){
		// Выполняем блокировку потока
		this->mtx.timer.lock();
		// Создаём объект таймера
		auto ret = this->timers.emplace(this->timers.size() + 1, unique_ptr <timer_t> (new timer_t));
		// Выполняем разблокировку потока
		this->mtx.timer.unlock();
		// Получаем идентификатор таймера
		result = ret.first->first;
		// Устанавливаем передаваемый контекст
		ret.first->second->ctx = ctx;
		// Устанавливаем родительский объект
		ret.first->second->core = this;
		// Устанавливаем идентификатор таймера
		ret.first->second->id = result;
		// Устанавливаем функцию обратного вызова
		ret.first->second->fn = callback;
		// Устанавливаем задержку времени в миллисекундах
		ret.first->second->delay = (delay / (float) 1000.f);
		// Устанавливаем базу событий
		ret.first->second->timer.set(this->base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->timer.set <timer_t, &timer_t::callback> (ret.first->second.get());
		// Запускаем работу таймера
		ret.first->second->timer.start(ret.first->second->delay);
	}
	// Выводим результат
	return result;
}
/**
 * setInterval Метод установки интервала времени
 * @param ctx      передаваемый контекст
 * @param delay    задержка времени в миллисекундах
 * @param callback функция обратного вызова
 * @return         идентификатор созданного таймера
 */
u_short awh::Core::setInterval(void * ctx, const time_t delay, function <void (const u_short, Core *, void *)> callback) noexcept {
	// Результат работы функции
	u_short result = 0;
	// Если данные переданы
	if((delay > 0) && (callback != nullptr) && (this->base != nullptr)){
		// Выполняем блокировку потока
		this->mtx.timer.lock();
		// Создаём объект таймера
		auto ret = this->timers.emplace(this->timers.size() + 1, unique_ptr <timer_t> (new timer_t));
		// Выполняем разблокировку потока
		this->mtx.timer.unlock();
		// Получаем идентификатор таймера
		result = ret.first->first;
		// Устанавливаем передаваемый контекст
		ret.first->second->ctx = ctx;
		// Устанавливаем родительский объект
		ret.first->second->core = this;
		// Устанавливаем идентификатор таймера
		ret.first->second->id = result;
		// Устанавливаем функцию обратного вызова
		ret.first->second->fn = callback;
		// Устанавливаем флаг персистентной работы
		ret.first->second->persist = true;
		// Устанавливаем задержку времени в миллисекундах
		ret.first->second->delay = (delay / (float) 1000.f);
		// Устанавливаем базу событий
		ret.first->second->timer.set(this->base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->timer.set <timer_t, &timer_t::callback> (ret.first->second.get());
		// Запускаем работу таймера
		ret.first->second->timer.start(ret.first->second->delay);
	}
	// Выводим результат
	return result;
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации заморозки чтения данных
 */
void awh::Core::freeze(const bool mode) noexcept {
	// Устанавливаем режим заморозки чтения данных
	this->dispatch.freeze(mode);
}
/**
 * setNoInfo Метод установки флага запрета вывода информационных сообщений
 * @param mode флаг запрета вывода информационных сообщений
 */
void awh::Core::setNoInfo(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Устанавливаем флаг запрета вывода информационных сообщений
	this->noinfo = mode;
}
/**
 * setPersist Метод установки персистентного флага
 * @param mode флаг персистентного запуска каллбека
 */
void awh::Core::setPersist(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Выполняем установку флага персистентного запуска каллбека
	this->persist = mode;
}
/**
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::setVerifySSL(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Выполняем установку флага проверки домена
	this->ssl.setVerify(mode);
}
/**
 * setMultiThreads Метод активации режима мультипотоковой обработки данных
 * @param mode флаг мультипотоковой обработки
 */
void awh::Core::setMultiThreads(const bool mode) noexcept {
	// Выполняем блокировку потока
	this->mtx.main.lock();
	// Устанавливаем флаг мультипотоковой обработки
	this->multi = mode;
	// Выполняем блокировку потока
	this->mtx.main.unlock();
	// Устанавливаем частоту обновления базы событий
	if(this->multi)
		// Выполняем инициализацию пула потоков
		this->pool.init();
	// Выполняем ожидание завершения работы потоков
	else this->pool.wait();
}
/**
 * setPersistInterval Метод установки персистентного таймера
 * @param itv интервал персистентного таймера в миллисекундах
 */
void awh::Core::setPersistInterval(const time_t itv) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Устанавливаем интервал персистентного таймера
	this->persistInterval = itv;
}
/**
 * setFamily Метод установки тип протокола интернета
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Core::setFamily(const int family) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
}
/**
 * setCA Метод установки CA-файла корневого SSL сертификата
 * @param cafile адрес CA-файла
 * @param capath адрес каталога где находится CA-файл
 */
void awh::Core::setCA(const string & cafile, const string & capath) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
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
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
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
			// Выполняем установку нейм-серверов для DNS резолвера IPv4
			// this->dns4.replaceServers(this->net.v4.second);
		} break;
		// Если - это интернет-протокол IPv6
		case AF_INET6: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v6.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v6.second.assign(ns.cbegin(), ns.cend());
			// Выполняем установку нейм-серверов для DNS резолвера IPv6
			// this->dns6.replaceServers(this->net.v6.second);
		} break;
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : nwk(fmk), uri(fmk, &nwk), ssl(fmk, log, &uri), /* dns4(fmk, log, &nwk), dns6(fmk, log, &nwk),*/ socket(log), fmk(fmk), log(log) {
	// Создаем новую базу
	this->base = ev_default_loop(0);
	// Если база событий создана
	if(this->base != nullptr){
		/*
		// Добавляем базу событий для DNS резолвера IPv4
		this->dns4.setBase(this->base);
		// Добавляем базу событий для DNS резолвера IPv6
		this->dns6.setBase(this->base);
		// Выполняем установку нейм-серверов для DNS резолвера IPv4
		this->dns4.replaceServers(this->net.v4.second);
		// Выполняем установку нейм-серверов для DNS резолвера IPv6
		this->dns6.replaceServers(this->net.v6.second);
		*/
		// Выполняем установку базы событий
		this->dispatch.setBase(this->base);
	// Выходим принудительно из приложения
	} else exit(EXIT_FAILURE);
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Выполняем ожидание завершения работы потоков
	if(this->multi) this->pool.wait();
	// Выполняем остановку сервиса
	this->stop();
	/*
	// Выполняем удаление модуля DNS резолвера IPv4
	this->dns4.remove();
	// Выполняем удаление модуля DNS резолвера IPv6
	this->dns6.remove();
	// Если список подключённых ядер не пустой
	if(!this->cores.empty()){
		// Выполняем блокировку потока
		this->mtx.core.lock();
		// Переходим по всему списку подключённых ядер
		for(auto it = this->cores.begin(); it != this->cores.end();){
			// Выполняем блокировку потока
			it->first->mtx.stop.lock();
			// Если база событий подключена, удаляем её
			if((* it->second) != nullptr){
				// Удаляем объект базы событий
				event_base_free(* it->second);
				// Обнуляем базу событий
				(* it->second) = nullptr;
			}
			// Выполняем разблокировку потока
			it->first->mtx.stop.unlock();
			// Удаляем ядро из списка ядер
			it = this->cores.erase(it);
		}
		// Выполняем разблокировку потока
		this->mtx.core.unlock();
	}
	*/
	// Выполняем блокировку потока
	this->mtx.stop.lock();
	// Если база событий существует
	if(this->base != nullptr){
		// Удаляем объект базы событий
		ev_loop_destroy(this->base);
		// Обнуляем базу событий
		this->base = nullptr;
	}
	// Устанавливаем статус сетевого ядра
	this->status = status_t::STOP;
	// Выполняем разблокировку потока
	this->mtx.stop.unlock();
}
