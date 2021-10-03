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

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * winSocketInit Метод инициализации WinSock
	 */
	void awh::Core::winSocketInit() const noexcept {
		// Если winSock ещё не инициализирован
		if(!this->winSock){
			// Идентификатор ошибки
			int error = 0;
			// Объект данных запроса
			WSADATA wsaData;
			// Выполняем инициализацию сетевого контекста
			if((error = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){ // 0x202
				// Сообщаем, что сетевой контекст не поднят
				this->log->print("WSAStartup failed with error: %d", log_t::flag_t::CRITICAL, error);
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Выполняем проверку версии WinSocket
			if((2 != LOBYTE(wsaData.wVersion)) || (2 != HIBYTE(wsaData.wVersion))){
				// Сообщаем, что версия WinSocket не подходит
				this->log->print("%s", log_t::flag_t::CRITICAL, "WSADATA version is not correct");
				// Очищаем сетевой контекст
				this->winSocketClean();
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Запоминаем, что winSock уже инициализирован
			this->winSock = true;
		}
	}
	/**
	 * winSocketClean Метод очистки WinSock
	 */
	void awh::Core::winSocketClean() const noexcept {
		// Очищаем сетевой контекст
		WSACleanup();
		// Запоминаем, что winSock не инициализирован
		this->winSock = false;
	}
#endif
/**
 * delay Метод фриза потока на указанное количество секунд
 * @param seconds количество секунд для фриза потока
 */
void awh::Core::delay(const size_t seconds) const noexcept {
	// Если количество секунд передано
	if(seconds){
		// Если операционной системой является Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем фриз потока
			::Sleep(seconds * 1000);
		// Для всех остальных операционных систем
		#else
			// Выполняем фриз потока
			::sleep(seconds);
		#endif
	}
}
/**
 * clean Метод буфера событий
 * @param bev буфер событий для очистки
 */
void awh::Core::clean(struct bufferevent * bev) noexcept {
	// Если буфер событий передан
	if(bev != nullptr){
		// Получаем файловый дескриптор
		evutil_socket_t fd = bufferevent_getfd(bev);
		// Запрещаем чтение запись данных серверу
		bufferevent_disable(bev, EV_WRITE | EV_READ);
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
				// Очищаем всю структуру для сервера
				memset(&result.server, 0, sizeof(result.server));
				// Устанавливаем протокол интернета
				result.client.sin_family = AF_INET;
				result.server.sin_family = AF_INET;
				// Устанавливаем произвольный порт для локального подключения
				result.client.sin_port = htons(0);
				// Устанавливаем порт для локального подключения
				result.server.sin_port = htons(port);
				// Устанавливаем адрес для локальго подключения
				result.client.sin_addr.s_addr = inet_addr(host.c_str());
				// Устанавливаем адрес для удаленного подключения
				result.server.sin_addr.s_addr = inet_addr(ip.c_str());
				// Обнуляем серверную структуру
				memset(&result.server.sin_zero, 0, sizeof(result.server.sin_zero));
				// Запоминаем размер структуры
				size = sizeof(result.client);
				// Запоминаем полученную структуру
				sin = reinterpret_cast <struct sockaddr *> (&result.client);
				// Создаем сокет подключения
				result.fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			} break;
			// Для протокола IPv6
			case AF_INET6: {
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
				host = move(this->nwk->setLowIp6(host));
				// Буфер содержащий адрес IPv6
				// char hostClient[INET6_ADDRSTRLEN], hostServer[INET6_ADDRSTRLEN];
				// Очищаем всю структуру для клиента
				memset(&result.client6, 0, sizeof(result.client6));
				// Очищаем всю структуру для сервера
				memset(&result.server6, 0, sizeof(result.server6));
				// Неважно, IPv4 или IPv6
				result.client6.sin6_family = AF_INET6;
				result.server6.sin6_family = AF_INET6;
				// Устанавливаем произвольный порт для локального подключения
				result.client6.sin6_port = htons(0);
				// Устанавливаем порт для локального подключения
				result.server6.sin6_port = htons(port);
				// Указываем адреса
				inet_pton(AF_INET6, host.c_str(), &result.client6.sin6_addr);
				inet_pton(AF_INET6, ip.c_str(), &result.server6.sin6_addr);
				// Устанавливаем адреса
				// inet_ntop(AF_INET6, &result.client6.sin6_addr, hostClient, sizeof(hostClient));
				// inet_ntop(AF_INET6, &result.server6.sin6_addr, hostServer, sizeof(hostServer));
				// Запоминаем размер структуры
				size = sizeof(result.client6);
				// Запоминаем полученную структуру
				sin = reinterpret_cast <struct sockaddr *> (&result.client6);
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
			return result;
		}
		// Если - это Unix
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем игнорирование сигнала неверной инструкции процессора
			sockets_t::noSigill(this->log);
			// Устанавливаем разрешение на повторное использование сокета
			sockets_t::reuseable(result.fd, this->log);
			// Отключаем сигнал записи в оборванное подключение
			sockets_t::noSigpipe(result.fd, this->log);
			// Отключаем алгоритм Нейгла для сервера и клиента
			sockets_t::tcpNodelay(result.fd, this->log);
			// Разблокируем сокет
			sockets_t::nonBlocking(result.fd, this->log);
			// Активируем keepalive
			sockets_t::keepAlive(result.fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->log);
		// Если - это Windows
		#else
			// Выполняем инициализацию WinSock
			this->winSocketInit();
			// Переводим сокет в блокирующий режим
			// sockets_t::blocking(result.fd);
			evutil_make_socket_nonblocking(result.fd);
			// evutil_make_socket_closeonexec(result.fd);
			evutil_make_listen_socket_reuseable(result.fd);
		#endif
		// Выполняем бинд на сокет
		if(::bind(result.fd, sin, size) < 0){
			// Выводим в лог сообщение
			this->log->print("bind local network [%s] error", log_t::flag_t::CRITICAL, host.c_str());
			// Выходим
			return result;
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
			try {
				// Резолвер IPv4, создаём резолвер
				if(core->dns4 != nullptr) core->dns4 = new dns_t(core->fmk, core->log, core->nwk, core->base, core->net.v4.second);
				// Резолвер IPv6, создаём резолвер
				if(core->dns6 != nullptr) core->dns6 = new dns_t(core->fmk, core->log, core->nwk, core->base, core->net.v6.second);
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
				if(core->startFn != nullptr) core->startFn(core, core->ctx);
			// Если происходит ошибка то игнорируем её
			} catch(const bad_alloc&) {
				// Выводим сообщение об ошибке
				core->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
			}
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
		// Отключаем всех клиентов
		if(core->base != nullptr) core->closeAll();
		// Удаляем DNS сервера IPv4
		if(core->dns4 != nullptr) delete core->dns4;
		// Удаляем DNS сервера IPv6
		if(core->dns6 != nullptr) delete core->dns6;
		// Зануляем базу событий
		core->base = nullptr;
		// Если функция обратного вызова установлена, выполняем
		if(core->stopFn != nullptr) core->stopFn(core, core->ctx);
		// Выполняем сброс блокировки базы событий
		core->locker = false;
		// Выполняем разблокировку потока
		this->bloking.unlock();
	}
}
/**
 * setStopCallback Метод установки функции обратного вызова при завершении работы модуля
 * @param callback функция обратного вызова для установки
 */
void awh::Core::setStopCallback(function <void (Core * core, void *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->stopFn = callback;
}
/**
 * setStartCallback Метод установки функции обратного вызова при запуске работы модуля
 * @param callback функция обратного вызова для установки
 */
void awh::Core::setStartCallback(function <void (Core * core, void *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->startFn = callback;
}
/**
 * stop Метод остановки клиента
 */
void awh::Core::stop() noexcept {
	// Если система уже запущена
	if(this->mode){
		// Запрещаем работу WebSocket
		this->mode = false;
		// Выполняем удаление всех воркеров
		this->removeAll();
		// Завершаем работу базы событий
		event_base_loopbreak(this->base);
		// Если - это Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Очищаем сетевой контекст
			this->winSocketClean();
		#endif
	}
}
/**
 * start Метод запуска клиента
 */
void awh::Core::start() noexcept {
	// Если система ещё не запущена
	if(!this->mode && !this->locker){
		try {
			// Разрешаем работу WebSocket
			this->mode = true;
			// Создаем новую базу
			this->base = event_base_new();
			// Резолвер IPv4, создаём резолвер
			this->dns4 = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v4.second);
			// Резолвер IPv6, создаём резолвер
			this->dns6 = new dns_t(this->fmk, this->log, this->nwk, this->base, this->net.v6.second);
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
			if(this->startFn != nullptr) this->startFn(this, this->ctx);
			// Выводим в консоль информацию
			this->log->print("[+] start service: pid = %u", log_t::flag_t::INFO, getpid());
			// Запускаем работу базы событий
			event_base_loop(this->base, EVLOOP_NO_EXIT_ON_EMPTY);
			// Удаляем dns IPv4 резолвер
			delete this->dns4;
			// Удаляем dns IPv6 резолвер
			delete this->dns6;
			// Зануляем DNS IPv4 объект
			this->dns4 = nullptr;
			// Зануляем DNS IPv6 объект
			this->dns6 = nullptr;
			// Удаляем объект базы событий
			event_base_free(this->base);
			// Очищаем все глобальные переменные
			libevent_global_shutdown();
			// Если функция обратного вызова установлена, выполняем
			if(this->stopFn != nullptr) this->stopFn(this, this->ctx);
			// Выводим в консоль информацию
			this->log->print("[-] stop service: pid = %u", log_t::flag_t::INFO, getpid());
		// Если происходит ошибка то игнорируем её
		} catch(const bad_alloc&) {
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		}
	}
}
/**
 * isStart Метод проверки на запуск бинда TCP/IP
 * @return результат проверки
 */
bool awh::Core::isStart() const noexcept {
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
					// Выполняем очистку буфера событий
					this->clean(it->second->bev);
					// Выводим функцию обратного вызова
					if(wrk->closeFn != nullptr)
						// Выполняем функцию обратного вызова
						wrk->closeFn(worker.first, this, wrk->ctx);
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
		// Получаем объект воркера
		worker_t * wrk = const_cast <worker_t *> (it->second->parent);
		// Если событие сервера существует
		if(it->second->bev != nullptr){
			// Выполняем очистку буфера событий
			this->clean(it->second->bev);
			// Устанавливаем что событие удалено
			const_cast <worker_t::adj_t *> (it->second)->bev = nullptr;
		}
		// Удаляем адъютанта из списка адъютантов
		wrk->adjutants.erase(aid);
		// Удаляем адъютанта из списка подключений
		this->adjutants.erase(aid);
		// Выводим сообщение об ошибке
		this->log->print("%s", log_t::flag_t::INFO, "disconnected from the server");
		// Выводим функцию обратного вызова
		if(wrk->closeFn != nullptr) wrk->closeFn(wrk->wid, this, wrk->ctx);
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
			// Получаем минимальное количество байт для детекции
			const size_t min = (it->second->markWrite.min > 0 ? it->second->markWrite.min : size);
			// Получаем максимальное количество байт для детекции
			const size_t max = (it->second->markWrite.max > 0 ? it->second->markWrite.max : size);
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
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Core::setVerifySSL(const bool mode) noexcept {
	// Выполняем установку флага проверки домена
	this->ssl->setVerify(mode);
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
	this->ssl->setCA(cafile, capath);
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
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : fmk(fmk), log(log) {
	try {
		// Создаём объект для работы с сетью
		this->nwk = new network_t(this->fmk);
		// Создаём объект URI
		this->uri = new uri_t(this->fmk, this->nwk);
		// Создаём объект для работы с SSL
		this->ssl = new ssl_t(this->fmk, this->log, this->uri);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Core Деструктор
 */
awh::Core::~Core() noexcept {
	// Если объект для работы с SSL создан
	if(this->ssl != nullptr) delete this->ssl;
	// Удаляем объект для работы с URI
	if(this->uri != nullptr) delete this->uri;
	// Удаляем объект для работы с сетью
	if(this->nwk != nullptr) delete this->nwk;
	// Если объект DNS IPv4 резолвера создан
	if(this->dns4 != nullptr) delete this->dns4;
	// Если объект DNS IPv6 резолвера создан
	if(this->dns6 != nullptr) delete this->dns6;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
