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
 * read Функция обратного вызова при чтении данных с сокета
 * @param watcher объект события чтения
 * @param revents идентификатор события
 */
void awh::worker_t::adj_t::read(ev::io & watcher, int revents) noexcept {
	// Получаем объект подключения
	worker_t * wrk = const_cast <worker_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (wrk->core);
	// Если разрешено выполнять чтения данных из сокета
	if(!this->bev.locked.read)
		// Выполняем передачу данных
		core->transfer(core_t::method_t::READ, this->aid);
	// Если выполнять чтение данных запрещено
	else {
		// Останавливаем таймаут ожидания на чтение из сокета
		this->bev.timer.read.stop();
		// Останавливаем таймаут ожидания на запись в сокет
		this->bev.timer.write.stop();
		// Выполняем отключение от сервера
		core->close(this->aid);
	}
}
/**
 * write Функция обратного вызова при записи данных в сокет
 * @param watcher объект события записи
 * @param revents идентификатор события
 */
void awh::worker_t::adj_t::write(ev::io & watcher, int revents) noexcept {
	// Получаем объект подключения
	worker_t * wrk = const_cast <worker_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (wrk->core);
	// Если разрешено выполнять запись данных в сокет
	if(!this->bev.locked.write)
		// Выполняем передачу данных
		core->transfer(core_t::method_t::WRITE, this->aid);
	// Если выполнять запись данных запрещено
	else {
		// Останавливаем таймаут ожидания на чтение из сокета
		this->bev.timer.read.stop();
		// Останавливаем таймаут ожидания на запись в сокет
		this->bev.timer.write.stop();
		// Выполняем отключение от сервера
		core->close(this->aid);
	}
}
/**
 * connect Функция обратного вызова при подключении к серверу
 * @param watcher объект события подключения
 * @param revents идентификатор события
 */
void awh::worker_t::adj_t::connect(ev::io & watcher, int revents) noexcept {
	// Выполняем остановку чтения
	watcher.stop();
	// Получаем объект подключения
	worker_t * wrk = const_cast <worker_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (wrk->core);
	// Останавливаем таймаут ожидания на запись в сокет
	this->bev.timer.write.stop();
	// Выполняем передачу данных об удачном подключении к серверу
	core->connected(this->aid);
}
/**
 * timeout Функция обратного вызова при срабатывании таймаута
 * @param timer   объект события таймаута
 * @param revents идентификатор события
 */
void awh::worker_t::adj_t::timeout(ev::timer & timer, int revents) noexcept {
	// Выполняем остановку таймера
	timer.stop();
	// Получаем объект подключения
	worker_t * wrk = const_cast <worker_t *> (this->parent);
	// Получаем объект ядра клиента
	core_t * core = const_cast <core_t *> (wrk->core);
	// Выполняем передачу данных
	core->timeout(this->aid);
}
/**
 * callback Функция обратного вызова
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::Core::Timer::callback(ev::timer & timer, int revents) noexcept {
	// Выполняем остановку таймера
	timer.stop();
	// Устанавливаем текущий штамп времени
	this->stamp = this->core->fmk->unixTimestamp();
	// Если функция обратного вызова установлена
	if(this->fn != nullptr) this->fn(this->id, this->core);
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
		// Выполняем запуск функции активации базы событий
		std::bind(&awh::Core::launching, this->core)();
		// Выполняем чтение базы событий пока это разрешено
		while(this->work){
			// Если не нужно использовать простой режим чтения
			if(this->easy)
				// Выполняем чтение базы событий
				this->base.run();
			// Выполняем чтение базы событий в простом режиме
			else this->base.run(EVRUN_NOWAIT);
			// Замораживаем поток на период времени частоты обновления базы событий
			this_thread::sleep_for(this->freq);
		}
		// Выполняем остановку функции активации базы событий
		std::bind(&awh::Core::closedown, this->core)();
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
		// Выполняем фриз чтения данных
		ev_suspend(this->base);
	// Продолжаем чтение данных
	else ev_resume(this->base);
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации
 */
void awh::Core::Dispatch::easily(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx);
	// Устанавливаем флаг активации простого чтения базы событий
	this->easy = mode;
	// Выполняем пинок
	this->kick();
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
 * setFrequency Метод установки частоты обновления базы событий
 * @param msec частота обновления базы событий в миллисекундах
 */
void awh::Core::Dispatch::setFrequency(const uint8_t msec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx);
	// Если количество миллисекунд передано больше 0
	if((this->easy = (msec > 0)))
		// Устанавливаем частоту обновления базы событий
		this->freq = chrono::milliseconds(msec);
	// Выполняем сброс частоты обновления базы событий
	else this->freq = 10ms;
}
/**
 * launching Метод вызова при активации базы событий
 */
void awh::Core::launching() noexcept {
	// Если база событий создана
	if(this->base != nullptr){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.status);
		// Устанавливаем статус сетевого ядра
		this->status = status_t::START;
		// Если список воркеров существует
		if(!this->workers.empty()){
			// Переходим по всему списку воркеров
			for(auto & worker : this->workers){
				// Если функция обратного вызова установлена
				if(worker.second->openFn != nullptr)
					// Выполняем функцию обратного вызова
					worker.second->openFn(worker.first, this);
			}
		}
		// Если функция обратного вызова установлена, выполняем
		if(this->callbackFn != nullptr) this->callbackFn(true, this);
		// Выводим в консоль информацию
		if(!this->noinfo) this->log->print("[+] start service: pid = %u", log_t::flag_t::INFO, getpid());
		// Если таймер периодического запуска коллбека активирован, запускаем персистентную работу
		if(this->persist){
			// Устанавливаем приоритет выполнения
			ev_set_priority(&this->timer.io, 2);
			// Устанавливаем базу событий
			this->timer.io.set(this->base);
			// Устанавливаем текущий штамп времени
			this->timer.stamp = this->fmk->unixTimestamp();
			// Устанавливаем время задержки персистентного вызова
			this->timer.delay = (this->persistInterval / (float) 1000.f);
			// Устанавливаем функцию обратного вызова
			this->timer.io.set <core_t, &core_t::persistent> (this);
			// Запускаем работу таймера
			this->timer.io.start(this->timer.delay);
		}
	}
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 */
void awh::Core::closedown() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.status);
	// Выполняем отключение всех адъютантов
	this->close();
	// Устанавливаем статус сетевого ядра
	this->status = status_t::STOP;
	// Если функция обратного вызова установлена, выполняем
	if(this->callbackFn != nullptr) this->callbackFn(false, this);
	// Выводим в консоль информацию
	if(!this->noinfo) this->log->print("[-] stop service: pid = %u", log_t::flag_t::INFO, getpid());
}
/**
 * executeTimers Метод принудительного исполнения работы таймеров
 */
void awh::Core::executeTimers() noexcept {
	// Если персистентный таймер или пользовательские таймеры активны
	if((this->type == type_t::CLIENT) && (this->persist || !this->timers.empty())){
		// Выполняем получение текущего значения времени
		const time_t date = this->fmk->unixTimestamp();
		// Если таймер периодического запуска коллбека активирован
		if(this->persist){
			// Если таймер не исполнился в заданное время
			if(((date - this->timer.stamp) / (float) 1000.f) >= this->timer.delay)
				// Выполняем функцию обратного вызова таймера
				this->persistent(this->timer.io, ev::TIMER);
		}
		// Если список таймеров не пустой
		if(!this->timers.empty()){
			// Переходим по всем таймерам
			for(auto it = this->timers.begin(); it != this->timers.end();){
				// Если таймер не исполнился в заданное время
				if(((date - it->second->stamp) / (float) 1000.f) >= it->second->delay){
					// Выполняем остановку таймера
					it->second->io.stop();
					// Устанавливаем текущий штамп времени
					it->second->stamp = date;
					// Если функция обратного вызова установлена
					if(it->second->fn != nullptr)
						// Выполняем функцию обратного вызова
						it->second->fn(it->first, this);
					// Если персистентная работа не установлена, удаляем таймер
					if(!it->second->persist)
						// Удаляем объект таймера
						it = this->timers.erase(it);
					// Если нужно продолжить работу таймера
					else {
						// Запускаем таймер снова
						it->second->io.start(it->second->delay);
						// Выполняем смещение итератора
						++it;
					}
				// Выполняем смещение итератора
				} else ++it;
			}
		}
	}
}
/**
 * persistent Функция персистентного вызова по таймеру
 * @param timer   объект события таймера
 * @param revents идентификатор события
 */
void awh::Core::persistent(ev::timer & timer, int revents) noexcept {
	// Выполняем остановку таймера
	timer.stop();
	// Устанавливаем текущий штамп времени
	this->timer.stamp = this->fmk->unixTimestamp();
	// Если список воркеров существует
	if(!this->workers.empty()){
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers){
			// Получаем объект воркера
			worker_t * wrk = const_cast <worker_t *> (worker.second);
			// Если функция обратного вызова установлена и адъютанты существуют
			if((wrk->persistFn != nullptr) && !wrk->adjutants.empty()){
				// Переходим по всему списку адъютантов и формируем список их идентификаторов
				for(auto & adj : wrk->adjutants)
					// Выполняем функцию обратного вызова
					wrk->persistFn(adj.first, worker.first, this);
			}
		}
	}
	// Устанавливаем время задержки персистентного вызова
	this->timer.delay = (this->persistInterval / (float) 1000.f);
	// Если нужно продолжить работу таймера
	timer.start(this->timer.delay);
}
/**
 * clean Метод буфера событий
 * @param aid идентификатор адъютанта
 */
void awh::Core::clean(const size_t aid) const noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Выполняем остановку таймера на чтение данных
		adj->bev.timer.read.stop();
		// Выполняем остановку таймера на запись данных
		adj->bev.timer.write.stop();
		// Выполняем остановку чтения буфера событий
		adj->bev.event.read.stop();
		// Выполняем остановку записи буфера событий
		adj->bev.event.write.stop();
		// Выполняем блокировку на чтение/запись данных
		adj->bev.locked = worker_t::locked_t();
		// Если сокет активен
		if(adj->bev.socket > -1){
			// Если - это Windows
			#if defined(_WIN32) || defined(_WIN64)
				// Запрещаем работу с сокетом
				shutdown(adj->bev.socket, SD_BOTH);
				// Выполняем закрытие сокета
				closesocket(adj->bev.socket);
			// Если - это Unix
			#else
				// Запрещаем работу с сокетом
				shutdown(adj->bev.socket, SHUT_RDWR);
				// Выполняем закрытие сокета
				::close(adj->bev.socket);
			#endif
			// Выполняем сброс сокета
			adj->bev.socket = -1;
		}
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
		core->mtx.status.lock();
		// Устанавливаем флаг запуска
		core->mode = true;
		// Если база событий не установлена
		if(core->base != this->base){
			// Устанавливаем базу событий
			core->base = this->base;
			/*
			// Добавляем базу событий для DNS резолвера IPv4
			core->dns4.setBase(core->base);
			// Добавляем базу событий для DNS резолвера IPv6
			core->dns6.setBase(core->base);
			// Выполняем установку нейм-серверов для DNS резолвера IPv4
			core->dns4.replaceServers(core->net.v4.second);
			// Выполняем установку нейм-серверов для DNS резолвера IPv6
			core->dns6.replaceServers(core->net.v6.second);
			*/
		}
		// Выполняем разблокировку потока
		core->mtx.status.unlock();
		// Выполняем запуск управляющей функции
		core->launching();
	}
}
/**
 * unbind Метод отключения модуля ядра от текущей базы событий
 * @param core модуль ядра для отключения
 */
void awh::Core::unbind(Core * core) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(core->mtx.bind);
	// Если модуль ядра передан
	if((core != nullptr) && (core->base == this->base) && (core != this)){
		// Выполняем блокировку потока
		core->mtx.status.lock();
		// Запрещаем работу WebSocket
		core->mode = false;
		// Выполняем разблокировку потока
		core->mtx.status.unlock();
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		core->clearTimers();
		// Выполняем блокировку потока
		core->mtx.status.lock();
		// Если таймер периодического запуска коллбека активирован
		if(core->persist)
			// Останавливаем работу персистентного таймера
			core->timer.io.stop();
		// Выполняем разблокировку потока
		core->mtx.status.unlock();
		// Выполняем отключение всех клиентов
		core->close();
		// Выполняем блокировку потока
		core->mtx.status.lock();
		// Зануляем базу событий
		core->base = nullptr;
		// Выполняем разблокировку потока
		core->mtx.status.unlock();
		
		/*
		// Выполняем удаление модуля DNS резолвера IPv4
		core->dns4.remove();
		// Выполняем удаление модуля DNS резолвера IPv6
		core->dns6.remove();
		*/

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
		// Запускаем метод деактивации базы событий
		core->closedown();
	}
}
/**
 * setCallback Метод установки функции обратного вызова при запуске/остановки работы модуля
 * @param callback функция обратного вызова для установки
 */
void awh::Core::setCallback(function <void (const bool, Core * core)> callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Устанавливаем функцию обратного вызова
	this->callbackFn = callback;
}
/**
 * stop Метод остановки клиента
 */
void awh::Core::stop() noexcept {
	// Если система уже запущена
	if(this->mode && (this->base != nullptr)){
		// Выполняем блокировку потока
		this->mtx.status.lock();
		// Запрещаем работу WebSocket
		this->mode = false;
		// Выполняем разблокировку потока
		this->mtx.status.unlock();
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		this->clearTimers();
		// Выполняем блокировку потока
		this->mtx.status.lock();
		// Если таймер периодического запуска коллбека активирован
		if(this->persist)
			// Останавливаем работу персистентного таймера
			this->timer.io.stop();
		// Выполняем разблокировку потока
		this->mtx.status.unlock();
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
		this->mtx.status.lock();
		// Разрешаем работу WebSocket
		this->mode = !this->mode;
		// Выполняем разблокировку потока
		this->mtx.status.unlock();
		// Выполняем запуск чтения базы событий
		this->dispatch.start();
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
 * close Метод закрытия подключения воркера
 * @param aid идентификатор адъютанта
 */
void awh::Core::close(const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
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
 * timeout Функция обратного вызова при срабатывании таймаута
 * @param aid идентификатор адъютанта
 */
void awh::Core::timeout(const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
}
/**
 * connected Функция обратного вызова при удачном подключении к серверу
 * @param aid идентификатор адъютанта
 */
void awh::Core::connected(const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
}
/**
 * write Функция обратного вызова при записи данных в сокет
 * @param method метод режима работы
 * @param aid    идентификатор адъютанта
 */
void awh::Core::transfer(const method_t method, const size_t aid) noexcept {
	// Экранируем ошибку неиспользуемой переменной
	(void) aid;
	(void) method;
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
	// Если система уже запущена
	if(this->mode && (this->base != nullptr)){
		/**
		 * Timer Структура таймера
		 */
		typedef struct Timer {
			time_t delay;                               // Задержка времени в миллисекундах
			function <void (const u_short, Core *)> fn; // Функция обратного вызова
			/**
			 * Timer Конструктор
			 */
			Timer() noexcept : delay(0), fn(nullptr) {}
		} timer_t;
		// Список пересоздаваемых таймеров
		vector <timer_t> mainTimers(this->timers.size());
		/**
		 * Если запрещено использовать простое чтение базы событий
		 * Выполняем остановку всех таймеров
		 */
		if(!this->timers.empty()){
			// Индекс текущего таймера
			size_t index = 0;
			// Переходим по всем таймерам
			for(auto it = this->timers.begin(); it != this->timers.end();){
				// Выполняем блокировку потока
				this->mtx.timer.lock();
				// Выполняем остановку таймера
				it->second->io.stop();
				// Устанавливаем функцию обратного вызова
				mainTimers.at(index).fn = it->second->fn;
				// Устанавливаем задержку времени в миллисекундах
				mainTimers.at(index).delay = it->second->delay;
				// Удаляем таймер из списка
				it = this->timers.erase(it);
				// Выполняем разблокировку потока
				this->mtx.timer.unlock();
				// Увеличиваем значение индекса
				index++;
			}
		}
		// Выполняем остановку работы
		this->stop();
		// Выполняем блокировку потока
		this->mtx.main.lock();
		// Удаляем объект базы событий
		ev_loop_destroy(this->base);
		// Создаем новую базу
		this->base = ev_default_loop(0);
		// Выполняем разблокировку потока
		this->mtx.main.unlock();
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
			// Если список таймеров получен
			if(!mainTimers.empty()){
				// Переходим по всему списку таймеров
				for(auto & timer : mainTimers)
					// Создаём новый таймер
					this->setTimeout(timer.delay, timer.fn);
				// Выполняем очистку списка таймеров
				mainTimers.clear();
				// Выполняем освобождение выделенной памяти
				vector <timer_t> ().swap(mainTimers);
			}
			// Выполняем запуск работы
			this->start();
		// Выходим принудительно из приложения
		} else exit(EXIT_FAILURE);
	// Если система ещё не запущена
	} else {
		// Выполняем блокировку потока
		this->mtx.main.lock();
		// Если база событий уже создана
		if(this->base != nullptr)
			// Удаляем объект базы событий
			ev_loop_destroy(this->base);
		// Создаем новую базу
		this->base = ev_default_loop(0);
		// Выполняем разблокировку потока
		this->mtx.main.unlock();
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
}
/**
 * error Метод вывода описание ошибок
 * @param bytes количество записанных/прочитанных байт в сокет
 * @param aid   идентификатор адъютанта
 */
void awh::Core::error(const int64_t bytes, const size_t aid) const noexcept {
	// Если работа базы событий продолжается
	if(this->working()){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Если SSL клиент разрешён
			if(it->second->ssl.mode){
				// Получаем данные описание ошибки
				const int error = SSL_get_error(it->second->ssl.ssl, bytes);
				// Определяем тип ошибки
				switch(error){
					// Если ошибка ожидания записи
					case SSL_ERROR_WANT_WRITE:
						// Выводим в лог сообщение
						this->log->print("SSL: unable to write data to server", log_t::flag_t::CRITICAL);
					break;
					// Если ошибка чтения данных
					case SSL_ERROR_WANT_READ:
						// Выводим в лог сообщение
						this->log->print("SSL: unable to read data from server", log_t::flag_t::CRITICAL);
					break;
					// Если был возвращён ноль
					case SSL_ERROR_ZERO_RETURN: {
						// Если удалённая сторона произвела закрытие подключения
						if(SSL_get_shutdown(it->second->ssl.ssl) & SSL_RECEIVED_SHUTDOWN)
							// Выводим в лог сообщение
							this->log->print("SSL: the remote side closed the connection", log_t::flag_t::INFO);
					} break;
					// Если произошла ошибка вызова
					case SSL_ERROR_SYSCALL: {
						// Получаем данные описание ошибки
						u_long error = ERR_get_error();
						// Если ошибка получена
						if(error != 0){
							// Выводим в лог сообщение
							this->log->print("SSL: %s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
							/**
							 * Выполняем извлечение остальных ошибок
							 */
							do {
								// Выводим в лог сообщение
								this->log->print("SSL: %s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
							// Если ещё есть ошибки
							} while((error = ERR_get_error()));
						// Если данные записаны неверно
						} else if(bytes == -1) {
							// Определяем тип ошибки
							switch(errno){
								// Если произведена неудачная запись в PIPE
								case EPIPE:
									// Выводим в лог сообщение
									this->log->print("SSL: EPIPE", log_t::flag_t::WARNING);
								break;
								// Если произведён сброс подключения
								case ECONNRESET:
									// Выводим в лог сообщение
									this->log->print("SSL: ECONNRESET", log_t::flag_t::WARNING);
								break;
								// Для остальных ошибок
								default:
									// Выводим в лог сообщение
									this->log->print("SSL: %s", log_t::flag_t::CRITICAL, strerror(errno));
							}
						}
					} break;
					// Если произошла ошибка шифрования
					case SSL_ERROR_SSL:
					// Для всех остальных ошибок
					case SSL_ERROR_NONE:
					// Если произошла ошибка сертификата
					case SSL_ERROR_WANT_X509_LOOKUP:
					// Для всех остальных ошибок
					default: {
						// Получаем данные описание ошибки
						u_long error = 0;
						// Выполняем чтение ошибок OpenSSL
						while((error = ERR_get_error()))
							// Выводим в лог сообщение
							this->log->print("SSL: %s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
					}
				};
			// Если произошла ошибка
			} else if(bytes == -1) {	
				// Определяем тип ошибки
				switch(errno){
					// Если произведена неудачная запись в PIPE
					case EPIPE:
						// Выводим в лог сообщение
						this->log->print("EPIPE", log_t::flag_t::WARNING);
					break;
					// Если произведён сброс подключения
					case ECONNRESET:
						// Выводим в лог сообщение
						this->log->print("ECONNRESET", log_t::flag_t::WARNING);
					break;
					// Для остальных ошибок
					default:
						// Выводим в лог сообщение
						this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
				}
			}
		}
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
		// Если адъютант получен и запись в сокет разрешена
		if((it != this->adjutants.end()) && !it->second->bev.locked.write && (it->second->bev.socket > -1)){
			// Если размер записываемых данных соответствует
			if(size >= it->second->markWrite.min){
				// Количество отправленных байт
				int64_t bytes = -1;
				// Если количество записываемых данных менье максимального занчения
				if(size <= it->second->markWrite.max){
					// Если SSL клиент разрешён
					if(it->second->ssl.mode)
						// Выполняем отправку сообщения через защищённый канал
						bytes = SSL_write(it->second->ssl.ssl, buffer, size);
					// Выполняем отправку сообщения в сокет
					else bytes = send(it->second->bev.socket, buffer, size, 0);
				// Иначе выполняем дробление передаваемых данных
				} else {
					// Смещение в буфере и отправляемый размер данных
					size_t offset = 0, actual = 0;
					// Выполняем отправку данных пока всё не отправим
					while((size - offset) > 0){
						// Определяем размер отправляемых данных
						actual = ((size - offset) >= it->second->markWrite.max ? it->second->markWrite.max : (size - offset));
						// Если SSL клиент разрешён
						if(it->second->ssl.mode)
							// Выполняем отправку сообщения через защищённый канал
							bytes = SSL_write(it->second->ssl.ssl, buffer + offset, actual);
						// Выполняем отправку сообщения в сокет
						else bytes = send(it->second->bev.socket, buffer + offset, actual, 0);
						// Увеличиваем смещение в буфере
						offset += actual;
					}
				}
				// Если байты не были записаны в сокет
				if(bytes <= 0){
					// Выполняем обработку ошибок
					this->error(bytes, aid);
					// Выполняем отключение от сервера
					this->close(it->second->aid);
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
				// Если минимальный размер данных для чтения, не установлен
				if(it->second->markRead.min == 0)
					// Устанавливаем размер минимальных для чтения данных по умолчанию
					const_cast <worker_t::adj_t *> (it->second)->markRead.min = BUFFER_READ_MIN;
			} break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE: {
				// Устанавливаем минимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markWrite.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->markWrite.max = max;
				// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
				if(it->second->markWrite.max == 0)
					// Устанавливаем размер максимальных записываемых данных по умолчанию
					const_cast <worker_t::adj_t *> (it->second)->markWrite.max = BUFFER_WRITE_MAX;
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
			it->second->io.stop();
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
			it->second->io.stop();
			// Удаляем объект таймера
			this->timers.erase(it);
		}
	}
}
/**
 * setTimeout Метод установки таймаута
 * @param delay    задержка времени в миллисекундах
 * @param callback функция обратного вызова
 * @return         идентификатор созданного таймера
 */
u_short awh::Core::setTimeout(const time_t delay, function <void (const u_short, Core *)> callback) noexcept {
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
		// Устанавливаем приоритет выполнения
		ev_set_priority(&ret.first->second->io, 1);
		// Устанавливаем родительский объект
		ret.first->second->core = this;
		// Устанавливаем идентификатор таймера
		ret.first->second->id = result;
		// Устанавливаем функцию обратного вызова
		ret.first->second->fn = callback;
		// Устанавливаем задержку времени в миллисекундах
		ret.first->second->delay = (delay / (float) 1000.f);
		// Устанавливаем текущий штамп времени
		ret.first->second->stamp = this->fmk->unixTimestamp();
		// Устанавливаем базу событий
		ret.first->second->io.set(this->base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->io.set <timer_t, &timer_t::callback> (ret.first->second.get());
		// Запускаем работу таймера
		ret.first->second->io.start(ret.first->second->delay);
	}
	// Выводим результат
	return result;
}
/**
 * setInterval Метод установки интервала времени
 * @param delay    задержка времени в миллисекундах
 * @param callback функция обратного вызова
 * @return         идентификатор созданного таймера
 */
u_short awh::Core::setInterval(const time_t delay, function <void (const u_short, Core *)> callback) noexcept {
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
		// Устанавливаем приоритет выполнения
		ev_set_priority(&ret.first->second->io, 1);
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
		// Устанавливаем текущий штамп времени
		ret.first->second->stamp = this->fmk->unixTimestamp();
		// Устанавливаем базу событий
		ret.first->second->io.set(this->base);
		// Устанавливаем функцию обратного вызова
		ret.first->second->io.set <timer_t, &timer_t::callback> (ret.first->second.get());
		// Запускаем работу таймера
		ret.first->second->io.start(ret.first->second->delay);
	}
	// Выводим результат
	return result;
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации простого чтения базы событий
 */
void awh::Core::easily(const bool mode) noexcept {
	// Определяем запущено ли ядро сети
	const bool start = this->mode;
	// Если ядро сети уже запущено, останавливаем его
	if(start) this->stop();
	// Устанавливаем режим чтения базы событий
	this->dispatch.easily(mode);
	// Если ядро сети уже было запущено, запускаем его
	if(start) this->start();
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
 * setFrequency Метод установки частоты обновления базы событий
 * @param msec частота обновления базы событий в миллисекундах
 */
void awh::Core::setFrequency(const uint8_t msec) noexcept {
	// Устанавливаем частоту чтения базы событий
	this->dispatch.setFrequency(msec);
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
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log, const int family) noexcept : nwk(fmk), uri(fmk, &nwk), ssl(fmk, log, &uri), /* dns4(fmk, log, &nwk), dns6(fmk, log, &nwk),*/ socket(log), dispatch(this), fmk(fmk), log(log) {
	// Выполняем создание базы событий
	this->rebase();
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
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
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Выполняем остановку сервиса
	this->stop();
	/*
	// Выполняем удаление модуля DNS резолвера IPv4
	this->dns4.remove();
	// Выполняем удаление модуля DNS резолвера IPv6
	this->dns6.remove();
	*/
	// Выполняем блокировку потока
	this->mtx.status.lock();
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
	this->mtx.status.unlock();
}
