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
		// Если запрещено выполнять чтение данных из сокета
		if(this->bev.locked.read)
			// Останавливаем чтение данных
			core->disabled(core_t::method_t::READ, this->aid);
		// Если запрещено выполнять запись данных в сокет
		if(this->bev.locked.write)
			// Останавливаем запись данных
			core->disabled(core_t::method_t::WRITE, this->aid);
		// Если запрещено и читать и записывать в сокет
		if(this->bev.locked.read && this->bev.locked.write)
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
		// Если запрещено выполнять чтение данных из сокета
		if(this->bev.locked.read)
			// Останавливаем чтение данных
			core->disabled(core_t::method_t::READ, this->aid);
		// Если запрещено выполнять запись данных в сокет
		if(this->bev.locked.write)
			// Останавливаем запись данных
			core->disabled(core_t::method_t::WRITE, this->aid);
		// Если запрещено и читать и записывать в сокет
		if(this->bev.locked.read && this->bev.locked.write)
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
	// Останавливаем подключение
	core->disabled(core_t::method_t::CONNECT, this->aid);
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
	// Если база событий проинициализированна
	if(this->init){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx);
		// Выполняем остановку всех событий
		this->base.break_loop(ev::how_t::ALL);
	}
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
	// Выполняем блокировку потока
	this->mtx.lock();
	// Если чтение базы событий ещё не началось
	if(!this->work && this->init){
		// Устанавливаем флаг работы модуля
		this->work = true;
		// Выполняем разблокировку потока
		this->mtx.unlock();
		// Выполняем запуск функции активации базы событий
		std::bind(&awh::Core::launching, this->core)();
		// Выполняем чтение базы событий пока это разрешено
		while(this->work){
			// Если база событий проинициализированна
			if(this->init){
				// Если не нужно использовать простой режим чтения
				if(!this->easy)
					// Выполняем чтение базы событий
					this->base.run();
				// Выполняем чтение базы событий в простом режиме
				else this->base.run(EVRUN_NOWAIT);
			}
			// Замораживаем поток на период времени частоты обновления базы событий
			this_thread::sleep_for(this->freq);
		}
		// Выполняем остановку функции активации базы событий
		std::bind(&awh::Core::closedown, this->core)();
	// Выполняем разблокировку потока
	} else this->mtx.unlock();
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации
 */
void awh::Core::Dispatch::freeze(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx);
	// Если база событий проинициализированна
	if(this->init){
		// Выполняем фриз получения данных
		this->mode = mode;
		// Если запрещено использовать простое чтение базы событий
		if(this->mode)
			// Выполняем фриз чтения данных
			ev_suspend(this->base);
		// Продолжаем чтение данных
		else ev_resume(this->base);
	}
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
	if(this->work){
		// Выполняем заморозку базы событий
		this->freeze(true);
		// Выполняем деактивации инициализации базы событий
		this->init = false;
		/// Выполняем пинок
		this->kick();
	}
	// Если база событий передана
	if(base != nullptr){
		// Устанавливаем базу событий
		this->base = ev::loop_ref(base);
		// Выполняем активацию инициализации базы событий
		this->init = true;
	}
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
		// Останавливаем чтение данных
		const_cast <core_t *> (this)->disabled(method_t::READ, it->first);
		// Останавливаем запись данных
		const_cast <core_t *> (this)->disabled(method_t::WRITE, it->first);
		// Выполняем блокировку на чтение/запись данных
		adj->bev.locked = worker_t::locked_t();
		// Если сокет активен
		if(adj->bev.socket > -1){
			/**
			 * Если операционной системой является Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Запрещаем работу с сокетом
				shutdown(adj->bev.socket, SD_BOTH);
				// Выполняем закрытие сокета
				closesocket(adj->bev.socket);
			/**
			 * Если операционной системой является Nix-подобная
			 */
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
 * @return параметры подключения к серверу
 */
const awh::Core::sockaddr_t awh::Core::sockaddr() const noexcept {
	// Результат работы функции
	sockaddr_t result;
	// Если требуется использовать unix-сокет
	if(this->isSetUnixSocket()){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Если сокет в файловой системе уже существует, удаляем его
				if(fs_t::issock(this->unixSocket))
					// Удаляем файл сокета
					::unlink(this->unixSocket.c_str());
			}
			// Устанавливаем протокол интернета
			result.unix.sun_family = AF_UNIX;
			// Очищаем всю структуру для сервера
			memset(&result.unix.sun_path, 0, sizeof(result.unix.sun_path));
			// Копируем адрес сокета сервера
			strncpy(result.unix.sun_path, this->unixSocket.c_str(), sizeof(result.unix.sun_path));
			// Создаем сокет подключения
			result.socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
			// Если сокет не создан то выходим
			if(result.socket < 0){
				// Выводим сообщение в консоль
				this->log->print("creating socket %s", log_t::flag_t::CRITICAL, this->unixSocket.c_str());
				// Выходим
				return sockaddr_t();
			}
			// Выполняем игнорирование сигнала неверной инструкции процессора
			this->socket.noSigill();
			// Отключаем сигнал записи в оборванное подключение
			this->socket.noSigpipe(result.socket);
			// Устанавливаем разрешение на повторное использование сокета
			this->socket.reuseable(result.socket);
			// Переводим сокет в не блокирующий режим
			this->socket.nonBlocking(result.socket);
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Получаем размер объекта сокета
				const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(result.unix.sun_path));
				// Выполняем бинд на сокет
				if(::bind(result.socket, (struct sockaddr *) &result.unix, size) < 0){
					// Выводим в лог сообщение
					this->log->print("bind local network [%s]", log_t::flag_t::CRITICAL, this->unixSocket.c_str());
					// Выходим
					return sockaddr_t();
				}
			}
		#endif
	}
	// Выводим результат
	return result;
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
					result.client.sin_family = family;
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
				result.server.sin_family = family;
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
				result.socket = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
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
					result.client6.sin6_family = family;
					// Устанавливаем произвольный порт для локального подключения
					result.client6.sin6_port = htons(0);
					// Указываем адрес IPv6 для клиента
					inet_pton(family, host.c_str(), &result.client6.sin6_addr);
					// inet_ntop(family, &result.client6.sin6_addr, hostClient, sizeof(hostClient));
					// Запоминаем размер структуры
					size = sizeof(result.client6);
					// Запоминаем полученную структуру
					sin = reinterpret_cast <struct sockaddr *> (&result.client6);
				}
				// Очищаем всю структуру для сервера
				memset(&result.server6, 0, sizeof(result.server6));
				// Устанавливаем протокол интернета
				result.server6.sin6_family = family;
				// Устанавливаем порт для локального подключения
				result.server6.sin6_port = htons(port);
				// Указываем адрес IPv6 для сервера
				inet_pton(family, ip.c_str(), &result.server6.sin6_addr);
				// inet_ntop(family, &result.server6.sin6_addr, hostServer, sizeof(hostServer));
				// Если ядро является сервером
				if(this->type == type_t::SERVER){
					// Запоминаем размер структуры
					size = sizeof(result.server6);
					// Запоминаем полученную структуру
					sin = reinterpret_cast <struct sockaddr *> (&result.server6);
				}
				// Создаем сокет подключения
				result.socket = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
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
		if(result.socket < 0){
			// Выводим сообщение в консоль
			this->log->print("creating socket to server = %s, port = %u", log_t::flag_t::CRITICAL, ip.c_str(), port);
			// Выходим
			return sockaddr_t();
		}
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			this->socket.noSigill();
			// Отключаем сигнал записи в оборванное подключение
			this->socket.noSigpipe(result.socket);
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Включаем отображение сети IPv4 в IPv6
				if(family == AF_INET6) this->socket.ipV6only(result.socket, this->ipV6only);
			// Активируем keepalive
			} else this->socket.keepAlive(result.socket, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl);
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Если ядро является сервером
			if(this->type == type_t::SERVER){
				// Включаем отображение сети IPv4 в IPv6
				if(family == AF_INET6) this->socket.ipV6only(result.socket, this->ipV6only);
			// Активируем keepalive
			} else this->socket.keepAlive(result.socket);
		#endif
		// Если ядро является сервером
		if(this->type == type_t::SERVER)
			// Переводим сокет в не блокирующий режим
			this->socket.nonBlocking(result.socket);
		// Отключаем алгоритм Нейгла для сервера и клиента
		this->socket.tcpNodelay(result.socket);
		// Устанавливаем разрешение на закрытие сокета при неиспользовании
		// this->socket.closeonexec(result.socket);
		// Устанавливаем разрешение на повторное использование сокета
		this->socket.reuseable(result.socket);
		// Если ядро является сервером, устанавливаем хост
		if(this->type == type_t::SERVER){
			// Объект для работы с сетевым интерфейсом
			ifnet_t ifnet(this->fmk, this->log);
			// Получаем настоящий хост сервера
			host = ifnet.ip(family);
		}
		// Выполняем бинд на сокет
		if(::bind(result.socket, sin, size) < 0){
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
	// Выполняем блокировку потока
	this->mtx.status.lock();
	// Если система уже запущена
	if(this->mode && (this->base != nullptr)){
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
	// Выполняем разблокировку потока
	} else this->mtx.status.unlock();
}
/**
 * start Метод запуска клиента
 */
void awh::Core::start() noexcept {
	// Выполняем блокировку потока
	this->mtx.status.lock();
	// Если система ещё не запущена
	if(!this->mode && (this->base != nullptr)){
		// Разрешаем работу WebSocket
		this->mode = !this->mode;
		// Выполняем разблокировку потока
		this->mtx.status.unlock();
		// Выполняем запуск чтения базы событий
		this->dispatch.start();		
	// Выполняем разблокировку потока
	} else this->mtx.status.unlock();
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
		/**
		 * Если операционной системой является MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Создаем новую базу
			this->base = ev_default_loop(0);
		/**
		 * Если операционной системой является Linux
		 */
		#elif __linux__
			// Создаем новую базу
			this->base = ev_loop_new(ev_recommended_backends() | EVBACKEND_EPOLL);
		/**
		 * Если операционной системой является FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Создаем новую базу
			this->base = ev_loop_new(ev_recommended_backends() | EVBACKEND_KQUEUE);
		#endif
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
		/**
		 * Если операционной системой является MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Создаем новую базу
			this->base = ev_default_loop(0);
		/**
		 * Если операционной системой является Linux
		 */
		#elif __linux__
			// Создаем новую базу
			this->base = ev_loop_new(ev_recommended_backends() | EVBACKEND_EPOLL);
		/**
		 * Если операционной системой является FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Создаем новую базу
			this->base = ev_loop_new(ev_recommended_backends() | EVBACKEND_KQUEUE);
		#endif
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
			// Если защищённый режим работы разрешён
			if(it->second->ssl.mode){
				// Получаем данные описание ошибки
				const int error = SSL_get_error(it->second->ssl.ssl, bytes);
				// Определяем тип ошибки
				switch(error){
					// Если был возвращён ноль
					case SSL_ERROR_ZERO_RETURN: {
						// Если удалённая сторона произвела закрытие подключения
						if(SSL_get_shutdown(it->second->ssl.ssl) & SSL_RECEIVED_SHUTDOWN)
							// Выводим в лог сообщение
							this->log->print("the remote side closed the connection", log_t::flag_t::INFO);
					} break;
					// Если произошла ошибка вызова
					case SSL_ERROR_SYSCALL: {
						// Получаем данные описание ошибки
						u_long error = ERR_get_error();
						// Если ошибка получена
						if(error != 0){
							// Выводим в лог сообщение
							this->log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
							/**
							 * Выполняем извлечение остальных ошибок
							 */
							do {
								// Выводим в лог сообщение
								this->log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
							// Если ещё есть ошибки
							} while((error = ERR_get_error()));
						// Если данные записаны неверно
						} else if(bytes == -1)
							// Выводим в лог сообщение
							this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					} break;
					// Для всех остальных ошибок
					default: {
						// Получаем данные описание ошибки
						u_long error = 0;
						// Выполняем чтение ошибок OpenSSL
						while((error = ERR_get_error()))
							// Выводим в лог сообщение
							this->log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
					}
				};
			// Если произошла ошибка
			} else if(bytes == -1)
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
		}
	}
}
/**
 * disabled Метод деактивации метода события сокета
 * @param method метод события сокета
 * @param aid    идентификатор адъютанта
 */
void awh::Core::disabled(const method_t method, const size_t aid) noexcept {
	// Если работа базы событий продолжается
	if(this->working()){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Определяем метод события сокета
			switch((uint8_t) method){
				// Если событием является чтение
				case (uint8_t) method_t::READ: {
					// Запрещаем чтение данных из сокета
					adj->bev.locked.read = true;
					// Останавливаем ожидание чтения данных
					adj->bev.timer.read.stop();
					// Останавливаем чтение данных
					adj->bev.event.read.stop();
				} break;
				// Если событием является запись
				case (uint8_t) method_t::WRITE: {
					// Запрещаем запись данных в сокет
					adj->bev.locked.write = true;
					// Останавливаем ожидание записи данных
					adj->bev.timer.write.stop();
					// Останавливаем запись данных
					adj->bev.event.write.stop();
				} break;
				// Если событием является подключение
				case (uint8_t) method_t::CONNECT: {
					// Останавливаем ожидание подключения
					adj->bev.timer.connect.stop();
					// Останавливаем подключение
					adj->bev.event.connect.stop();
				} break;
			}
		}
	}
}
/**
 * enabled Метод активации метода события сокета
 * @param method  метод события сокета
 * @param aid     идентификатор адъютанта
 * @param timeout флаг активации таймаута
 */
void awh::Core::enabled(const method_t method, const size_t aid, const bool timeout) noexcept {
	// Если работа базы событий продолжается
	if(this->working()){
		// Выполняем извлечение адъютанта
		auto it = this->adjutants.find(aid);
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Если сокет подключения активен
			if(adj->bev.socket > -1){
				// Получаем объект подключения
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
				// Определяем метод события сокета
				switch((uint8_t) method){
					// Если событием является чтение
					case (uint8_t) method_t::READ: {
						// Разрешаем чтение данных из сокета
						adj->bev.locked.read = false;
						// Устанавливаем размер детектируемых байт на чтение
						adj->marker.read = wrk->marker.read;
						// Устанавливаем время ожидания чтения данных
						adj->timeouts.read = wrk->timeouts.read;
						// Устанавливаем приоритет выполнения для события на чтение
						ev_set_priority(&adj->bev.event.read, -2);
						// Устанавливаем базу событий
						adj->bev.event.read.set(this->base);
						// Устанавливаем сокет для чтения
						adj->bev.event.read.set(adj->bev.socket, ev::READ);
						// Устанавливаем событие на чтение данных подключения
						adj->bev.event.read.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::read> (adj);
						// Запускаем чтение данных
						adj->bev.event.read.start();
						// Если флаг ожидания входящих сообщений, активирован
						if(timeout && (adj->timeouts.read > 0)){
							// Устанавливаем приоритет выполнения для таймаута на чтение
							ev_set_priority(&adj->bev.timer.read, 0);
							// Устанавливаем базу событий
							adj->bev.timer.read.set(this->base);
							// Устанавливаем событие на таймаут чтения данных подключения
							adj->bev.timer.read.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::timeout> (adj);
							// Запускаем ожидание чтения данных
							adj->bev.timer.read.start(adj->timeouts.read);
						}
					} break;
					// Если событием является запись
					case (uint8_t) method_t::WRITE: {
						// Разрешаем запись данных в сокет
						adj->bev.locked.write = false;
						// Устанавливаем размер детектируемых байт на запись
						adj->marker.write = wrk->marker.write;
						// Устанавливаем время ожидания записи данных
						adj->timeouts.write = wrk->timeouts.write;
						// Устанавливаем приоритет выполнения для события на запись
						ev_set_priority(&adj->bev.event.write, -2);
						// Устанавливаем базу событий
						adj->bev.event.write.set(this->base);
						// Устанавливаем сокет для записи
						adj->bev.event.write.set(adj->bev.socket, ev::WRITE);
						// Устанавливаем событие на запись данных подключения
						adj->bev.event.write.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::write> (adj);
						// Запускаем запись данных
						adj->bev.event.write.start();
						// Если флаг ожидания исходящих сообщений, активирован
						if(timeout && (adj->timeouts.write > 0)){
							// Устанавливаем приоритет выполнения для таймаута на запись
							ev_set_priority(&adj->bev.timer.write, 0);
							// Устанавливаем базу событий
							adj->bev.timer.write.set(this->base);
							// Устанавливаем событие на таймаут записи данных подключения
							adj->bev.timer.write.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::timeout> (adj);
							// Запускаем ожидание записи данных
							adj->bev.timer.write.start(adj->timeouts.write);
						}
					} break;
					// Если событием является подключение
					case (uint8_t) method_t::CONNECT: {
						// Устанавливаем время ожидания записи данных
						adj->timeouts.connect = wrk->timeouts.connect;
						// Устанавливаем приоритет выполнения для события на чтения
						ev_set_priority(&adj->bev.event.connect, -2);
						// Устанавливаем базу событий
						adj->bev.event.connect.set(this->base);
						// Устанавливаем сокет для записи
						adj->bev.event.connect.set(adj->bev.socket, ev::WRITE);
						// Устанавливаем событие подключения
						adj->bev.event.connect.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::connect> (adj);
						// Выполняем запуск подключения
						adj->bev.event.connect.start();
						// Если время ожидания записи данных установлено
						if(timeout && (adj->timeouts.connect > 0)){
							// Устанавливаем базу событий
							adj->bev.timer.connect.set(this->base);
							// Устанавливаем событие на запись данных подключения
							adj->bev.timer.connect.set <awh::worker_t::adj_t, &awh::worker_t::adj_t::timeout> (adj);
							// Запускаем запись данных на сервер
							adj->bev.timer.connect.start(adj->timeouts.connect);
						}
					} break;
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
		// Если адъютант получен
		if(it != this->adjutants.end()){
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Добавляем буфер данных для записи
			adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
			// Если запись в сокет заблокирована
			if(adj->bev.locked.write){
				/**
				 * Если операционной системой является Nix-подобная
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Разрешаем выполнение записи в сокет
					this->enabled(method_t::WRITE, it->first);
				/**
				 * Если операционной системой является MS Windows
				 */
				#else
					// Если сокет подключения активен
					if(adj->bev.socket > -1){
						// Разрешаем запись данных в сокет
						adj->bev.locked.write = false;
						// Выполняем передачу данных
						this->transfer(core_t::method_t::WRITE, it->first);
					}
				#endif
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
				// Если нужно заблокировать метод
				if(mode)
					// Запрещаем чтение данных из сокета
					const_cast <worker_t::adj_t *> (it->second)->bev.locked.read = true;
				// Если нужно разблокировать метод
				else const_cast <worker_t::adj_t *> (it->second)->bev.locked.read = false;
			} break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE:
				// Если нужно заблокировать метод
				if(mode)
					// Запрещаем запись данных в сокет
					const_cast <worker_t::adj_t *> (it->second)->bev.locked.write = true;
				// Если нужно разблокировать метод
				else const_cast <worker_t::adj_t *> (it->second)->bev.locked.write = false;
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
				const_cast <worker_t::adj_t *> (it->second)->timeouts.read = seconds;
			break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE:
				// Устанавливаем время ожидания на исходящие данные
				const_cast <worker_t::adj_t *> (it->second)->timeouts.write = seconds;
			break;
			// Режим работы ПОДКЛЮЧЕНИЕ
			case (uint8_t) method_t::CONNECT:
				// Устанавливаем время ожидания на исходящие данные
				const_cast <worker_t::adj_t *> (it->second)->timeouts.connect = seconds;
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
				const_cast <worker_t::adj_t *> (it->second)->marker.read.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->marker.read.max = max;
				// Если минимальный размер данных для чтения, не установлен
				if(it->second->marker.read.min == 0)
					// Устанавливаем размер минимальных для чтения данных по умолчанию
					const_cast <worker_t::adj_t *> (it->second)->marker.read.min = BUFFER_READ_MIN;
			} break;
			// Режим работы ЗАПИСЬ
			case (uint8_t) method_t::WRITE: {
				// Устанавливаем минимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->marker.write.min = min;
				// Устанавливаем максимальный размер байт
				const_cast <worker_t::adj_t *> (it->second)->marker.write.max = max;
				// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
				if(it->second->marker.write.max == 0)
					// Устанавливаем размер максимальных записываемых данных по умолчанию
					const_cast <worker_t::adj_t *> (it->second)->marker.write.max = BUFFER_WRITE_MAX;
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
 * unsetUnixSocket Метод удаления unix-сокета
 * @return результат выполнения операции
 */
bool awh::Core::unsetUnixSocket() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.unix);
	// Результат работы функции
	bool result = false;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если сервер в данный момент не работает
		if((result = !this->working()))
			// Выполняем очистку unix-сокета
			this->unixSocket.clear();
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выводим в лог сообщение
		this->log->print("Microsoft Windows does not support Unix sockets", log_t::flag_t::CRITICAL);
		// Выходим принудительно из приложения
		exit(EXIT_FAILURE);
	#endif
	// Выводим результат
	return result;
}
/**
 * setUnixSocket Метод установки адреса файла unix-сокета
 * @param socket адрес файла unix-сокета
 * @return       результат установки unix-сокета
 */
bool awh::Core::setUnixSocket(const string & socket) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.unix);
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если адрес unix-сокета передан
		if(!socket.empty())
			// Выполняем установку unix-сокета
			this->unixSocket = this->fmk->format("/tmp/%s.sock", this->fmk->toLower(socket).c_str());
		// Если адрес unix-сокета не передан
		else this->unixSocket = this->fmk->format("/tmp/%s.sock", this->fmk->toLower(unixServerName.c_str()).c_str());
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выводим в лог сообщение
		this->log->print("Microsoft Windows does not support Unix sockets", log_t::flag_t::CRITICAL);
		// Выходим принудительно из приложения
		exit(EXIT_FAILURE);
	#endif
	// Выводим результат
	return !this->unixSocket.empty();
}
/**
 * isSetUnixSocket Метод проверки установки unix-сокета
 * @return результат проверки установки unix-сокета
 */
bool awh::Core::isSetUnixSocket() const noexcept {
	// Выполняем проверку на установку unix-сокета
	return !this->unixSocket.empty();
}
/**
 * isActiveUnixSocket Метод проверки активного unix-сокета
 * @param socket адрес файла unix-сокета
 * @return       результат проверки активного unix-сокета
 */
bool awh::Core::isActiveUnixSocket(const string & socket) const noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.unix);
	// Результат работы функции
	bool result = false;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если адрес unix-сокета передан
		if(!socket.empty())
			// Выполняем установку unix-сокета
			result = fs_t::issock(this->fmk->format("/tmp/%s.sock", this->fmk->toLower(socket).c_str()));
		// Если адрес unix-сокета не передан
		else result = fs_t::issock(this->fmk->format("/tmp/%s.sock", this->fmk->toLower(AWH_SHORT_NAME).c_str()));
	/**
	 * Если операционной системой является MS Windows
	 */
	#else
		// Выводим в лог сообщение
		this->log->print("Microsoft Windows does not support Unix sockets", log_t::flag_t::CRITICAL);
		// Выходим принудительно из приложения
		exit(EXIT_FAILURE);
	#endif
	// Выводим результат
	return result;
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
 * setNameServer Метод добавления названия сервера
 * @param name название сервера для добавления
 */
void awh::Core::setNameServer(const string & name) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.main);
	// Если название сервера передано
	if(!name.empty())
		// Устанавливаем новое название сервера
		this->unixServerName = name;
	// Иначе устанавливаем название сервера по умолчанию
	else this->unixServerName = AWH_SHORT_NAME;
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
	// Выполняем остановку персистентного таймера
	this->timer.io.stop();
	// Выполняем удаление активных таймеров
	this->timers.clear();
	// Выполняем удаление списка активных ядер
	this->cores.clear();
	// Выполняем удаление списка активных воркеров
	this->workers.clear();
	// Выполняем удаление активных адъютантов
	this->adjutants.clear();
	// Если база событий существует
	if(this->base != nullptr){
		// Удаляем объект базы событий
		ev_loop_destroy(this->base);
		// Обнуляем базу событий
		this->base = nullptr;
	}
	// Устанавливаем статус сетевого ядра
	this->status = status_t::STOP;
	// Если требуется использовать unix-сокет и ядро является сервером
	if(this->isSetUnixSocket() && (this->type == type_t::SERVER)){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Если сокет в файловой системе уже существует, удаляем его
			if(fs_t::issock(this->unixSocket))
				// Удаляем файл сокета
				::unlink(this->unixSocket.c_str());
		#endif
	}
	// Выполняем разблокировку потока
	this->mtx.status.unlock();
}
