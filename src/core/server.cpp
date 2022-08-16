/**
 * @file: server.cpp
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
#include <core/server.hpp>

/**
 * accept Функция подключения к серверу
 * @param watcher объект события подключения
 * @param revents идентификатор события
 */
void awh::server::worker_t::accept(ev::io & watcher, int revents) noexcept {
	// Получаем объект подключения
	core_t * core = (core_t *) const_cast <awh::core_t *> (this->core);
	// Выполняем подключение клиента
	core->accept(watcher.fd, this->wid);
}
/**
 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
 * @param wid   идентификатор воркера
 * @param pid   идентификатор процесса
 * @param event идентификатор события
 */
void awh::server::Core::cluster(const size_t wid, const pid_t pid, const cluster_t::event_t event) noexcept {
	// Выполняем поиск воркера
	auto it = this->workers.find(wid);
	// Если воркер найден, устанавливаем максимальное количество одновременных подключений
	if(it != this->workers.end()){
		// Получаем объект подключения
		worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
		// Выполняем тип возникшего события
		switch((uint8_t) event){
			// Если производится запуск процесса
			case (uint8_t) cluster_t::event_t::START: {
				// Если процесс является родительским
				if(this->pid == getpid())
					// Выбираем активный рабочий процесс
					this->sendEvent(it->first, 0, pid, event_t::SELECT);
				// Если процесс является дочерним
				else {
					// Запоминаем текущий идентификатор процесса
					this->_pid = pid;
					// Определяем тип сокета
					switch((uint8_t) this->net.sonet){
						// Если тип сокета установлен как UDP
						case (uint8_t) sonet_t::UDP:
							// Выполняем активацию сервера
							this->accept(1, it->first);
						break;
						// Для всех остальных типов сокетов
						default: {
							// Устанавливаем базу событий
							wrk->io.set(this->dispatch.base);
							// Устанавливаем событие на чтение данных подключения
							wrk->io.set <worker_t, &worker_t::accept> (wrk);
							// Устанавливаем сокет для чтения
							wrk->io.set(wrk->addr.fd, ev::READ);
							// Запускаем чтение данных с клиента
							wrk->io.start();
						}
					}
					// Сообщаем родительскому процессу, что дочерний процесс подключён
					this->sendEvent(it->first, 0, this->_pid, event_t::ONLINE);
				}
			} break;
			// Если производится остановка процесса
			case (uint8_t) cluster_t::event_t::STOP: {
				// Выполняем поиск воркера
				auto it = this->_burden.find(wid);
				// Если активный воркер найден
				if(it != this->_burden.end()){
					// Выполняем поиск процесс в списке нагрузки
					auto jt = it->second.find(pid);
					// Если процесс в списке нагрузке найден
					if(jt != it->second.end())
						// Удаляем процесс из списка нагрузки
						it->second.erase(jt);
					// Если список нагрузки пустой, удаляем весь объект
					if(it->second.empty())
						// Удаляем весь объект нагрузок
						this->_burden.erase(it);
				}
				// Если список адъютантов не пустой
				if(!this->_adjutants.empty()){
					// Переходим по всему списку адъютантов
					for(auto it = this->_adjutants.begin(); it != this->_adjutants.end();){
						// Если идентификатор процесса найден
						if(it->second == pid)
							// Удаляем адъютанта из списка
							it = this->_adjutants.erase(it);
						// Иначе продолжаем поиск дальше
						else ++it;
					}
				}
				// Удаляем буферы сообщений
				this->_messages.erase(pid);
			} break;
		}
	}
}
/**
 * message Метод получения сообщения от родительского или дочернего процесса
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса
 * @param buffer буфер получаемых данных
 * @param size   размер получаемых данных
 */
void awh::server::Core::message(const size_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept {
	// Создаём объект данных
	data_t data;
	// Извлекаем данные сообщения
	memcpy(&data, buffer, size);
	// Если процесс является родительским
	if(this->pid == getpid()){
		// Определяем тип события
		switch((uint8_t) data.event){
			// Если событием является подключение дочернего процесса
			case (uint8_t) event_t::ONLINE: {
				// Выполняем поиск активного воркера
				auto it = this->_burden.find(wid);
				// Если воркер существует
				if(it != this->_burden.end())
					// Добавляем в список нулевую нагрузку
					it->second.emplace(pid, 0);
				// Если воркер не существует, выполняем создание объекта нагрузки
				else {
					// Создаём объект нагрузки
					map <pid_t, size_t> _burden;
					// Выполняем инициализацию нагрузки
					_burden.emplace(pid, 0);
					// Выполняем установку объекта нагрузки
					this->_burden.emplace(wid, move(_burden));
				}
			} break;
			// Если событием является подключение
			case (uint8_t) event_t::CONNECT: {
				// Устанавливаем идентификатор выбранного процесса
				this->_pid = pid;
				// Выполняем поиск воркера
				auto it = this->_burden.find(wid);
				// Если активный воркер найден
				if(it != this->_burden.end()){
					// Выполняем поиск процесс в списке нагрузки
					auto jt = it->second.find(this->_pid);
					// Если процесс в списке нагрузке найден
					if(jt != it->second.end())
						// Устанавливаем количество подключений
						jt->second = data.count;
					// Если процесс в списке нагрузке не наден
					else it->second.emplace(this->_pid, (size_t) data.count);
					// Выполняем перебор всех процессов и ищем тот, где меньше всего нагрузка
					for(auto & item : it->second){
						// Если текущее количество подключений меньше чем передано
						if((item.second < data.count) || (item.second == 0)){
							// Запоминаем новый индекс процесса
							this->_pid = item.first;
							// Выходим из цикла
							break;
						}
					}
					// Выводим сообщени об активном сервисе
					if(!this->noinfo) this->log->print("%d clients are connected to the process, pid = %d", log_t::flag_t::INFO, data.count, pid);
					// Если был выбран новый процесс для обработки запросов
					if(this->_pid != pid){
						// Выполняем переключение процесса на активную работу
						this->sendEvent(wid, 0, this->_pid, event_t::SELECT);
						// Снимаем процесс с активной работы
						this->sendEvent(wid, 0, pid, event_t::UNSELECT);
					}
					// Добавляем подключившегося клиента в список клиентов
					this->_adjutants.emplace((size_t) data.aid, pid);
				}
			} break;
			// Если событием является отключение
			case (uint8_t) event_t::DISCONNECT: {
				// Выводим сообщени об активном сервисе
				if(!this->noinfo) this->log->print("client disconnected from process, %d connections left, pid = %d", log_t::flag_t::INFO, data.count, pid);
				// Выполняем поиск воркера
				auto it = this->_burden.find(wid);
				// Если активный воркер найден
				if(it != this->_burden.end()){
					// Выполняем поиск процесс в списке нагрузки
					auto jt = it->second.find(pid);
					// Если процесс в списке нагрузке найден
					if(jt != it->second.end())
						// Устанавливаем количество подключений
						jt->second = data.count;
					// Если такой процесс не существует, создаём новый
					else it->second.emplace(pid, (size_t) data.count);
					// Удаляем отключившегося клиента из списока клиентов
					this->_adjutants.erase(data.aid);
				}
			} break;
			// Если событием является сообщение
			case (uint8_t) event_t::MESSAGE: {
				// Выполняем поиск предыдущей части сообщения
				auto it = this->_messages.find(pid);
				// Если данные предыдущей части сообщения найдены
				if((it != this->_messages.end()) && (it->second.count(data.aid) > 0)){
					// Выполняем поиск идентификатор адъютанта
					auto jt = it->second.find(data.aid);
					// Добавляем очередную часть сообщения в буфер
					jt->second.insert(jt->second.end(), data.buffer, data.buffer + data.size);
					// Если данные получены полностью
					if(data.fin){
						// Выполняем поиск воркера
						auto kt = this->workers.find(wid);
						// Если воркер найден, устанавливаем максимальное количество одновременных подключений
						if(kt != this->workers.end()){
							// Получаем объект подключения
							worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (kt->second);
							// Если функция обратного вызова на получение данных установлена
							if(wrk->callback.mess != nullptr)
								// Выводим функцию обратного вызова
								wrk->callback.mess(jt->second.data(), jt->second.size(), wrk->wid, data.aid, pid, reinterpret_cast <awh::core_t *> (this));
						}
						// Удаляем буфер данных адъютанта
						it->second.erase(jt);
						// Если адъютантов в списке процесса больше нет
						if(it->second.empty())
							// Очищаем буфер сообщения
							this->_messages.erase(it);
					}
				// Если данные предыдущей части сообщения не найдены
				} else {
					// Если данные получены полностью
					if(data.fin){
						// Выполняем поиск воркера
						auto jt = this->workers.find(wid);
						// Если воркер найден, устанавливаем максимальное количество одновременных подключений
						if(jt != this->workers.end()){
							// Получаем объект подключения
							worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (jt->second);
							// Если функция обратного вызова на получение данных установлена
							if(wrk->callback.mess != nullptr)
								// Выводим функцию обратного вызова
								wrk->callback.mess((const char *) data.buffer, data.size, wrk->wid, data.aid, pid, reinterpret_cast <awh::core_t *> (this));
						}
					// Если данные получены не полностью
					} else {
						// Выполняем инициализацию буфера сообщения
						auto ret1 = this->_messages.emplace(pid, map <size_t, vector <char>> ());
						// Добавляем адъютанта в объект буфера данных
						auto ret2 = ret1.first->second.emplace((size_t) data.aid, vector <char> ());
						// Добавляем в буфер полученные данные
						ret2.first->second.insert(ret2.first->second.end(), data.buffer, data.buffer + data.size);
					}
				}
			} break;
		}
	// Если процесс является дочерним
	} else {
		// Определяем тип события
		switch((uint8_t) data.event){
			// Если событием является выбор работника
			case (uint8_t) event_t::SELECT: {
				// Устанавливаем флаг активации перехвата подключения
				this->_interception = true;
				// Выводим сообщени об активном сервисе
				if(!this->noinfo) this->log->print("a process has been activated to intercept connections, pid = %d", log_t::flag_t::INFO, getpid());
			} break;
			// Если событием является снятия выбора с работника
			case (uint8_t) event_t::UNSELECT: {
				// Снимаем флаг активации перехвата подключения
				this->_interception = false;
				// Выводим сообщени об активном сервисе
				if(!this->noinfo) this->log->print("a process has been deactivated to intercept connections, pid = %d", log_t::flag_t::INFO, getpid());
			} break;
			// Если событием является сообщение
			case (uint8_t) event_t::MESSAGE: {
				// Выполняем поиск предыдущей части сообщения
				auto it = this->_messages.find(pid);
				// Если данные предыдущей части сообщения найдены
				if((it != this->_messages.end()) && (it->second.count(data.aid) > 0)){
					// Выполняем поиск идентификатор адъютанта
					auto jt = it->second.find(data.aid);
					// Добавляем очередную часть сообщения в буфер
					jt->second.insert(jt->second.end(), data.buffer, data.buffer + data.size);
					// Если данные получены полностью
					if(data.fin){
						// Выполняем поиск воркера
						auto kt = this->workers.find(wid);
						// Если воркер найден, устанавливаем максимальное количество одновременных подключений
						if(kt != this->workers.end()){
							// Получаем объект подключения
							worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (kt->second);
							// Если функция обратного вызова на получение данных установлена
							if(wrk->callback.mess != nullptr)
								// Выводим функцию обратного вызова
								wrk->callback.mess(jt->second.data(), jt->second.size(), wrk->wid, data.aid, pid, reinterpret_cast <awh::core_t *> (this));
						}
						// Удаляем буфер данных адъютанта
						it->second.erase(jt);
						// Если адъютантов в списке процесса больше нет
						if(it->second.empty())
							// Очищаем буфер сообщения
							this->_messages.erase(it);
					}
				// Если данные предыдущей части сообщения не найдены
				} else {
					// Если данные получены полностью
					if(data.fin){
						// Выполняем поиск воркера
						auto jt = this->workers.find(wid);
						// Если воркер найден, устанавливаем максимальное количество одновременных подключений
						if(jt != this->workers.end()){
							// Получаем объект подключения
							worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (jt->second);
							// Если функция обратного вызова на получение данных установлена
							if(wrk->callback.mess != nullptr)
								// Выводим функцию обратного вызова
								wrk->callback.mess((const char *) data.buffer, data.size, wrk->wid, data.aid, pid, reinterpret_cast <awh::core_t *> (this));
						}
					// Если данные получены не полностью
					} else {
						// Выполняем инициализацию буфера сообщения
						auto ret1 = this->_messages.emplace(pid, map <size_t, vector <char>> ());
						// Добавляем адъютанта в объект буфера данных
						auto ret2 = ret1.first->second.emplace((size_t) data.aid, vector <char> ());
						// Добавляем в буфер полученные данные
						ret2.first->second.insert(ret2.first->second.end(), data.buffer, data.buffer + data.size);
					}
				}
			} break;
		}
	}
}
/**
 * sendEvent Метод отправки события родительскому и дочернему процессу
 * @param wid   идентификатор воркера
 * @param aid   идентификатор адъютанта
 * @param pid   идентификатор процесса
 * @param event активное событие на сервере
 */
void awh::server::Core::sendEvent(const size_t wid, const size_t aid, const pid_t pid, const event_t event) noexcept {	
	// Создаём объект сообщения
	data_t data;
	// Устанавливаем идентификатор адъютанта
	data.aid = aid;
	// Устанавливаем событие сообщения
	data.event = event;
	// Если процесс является родительским
	if(this->pid == getpid())
		// Отправляем сообщение дочернему процессу
		this->_cluster.send(wid, pid, (const char *) &data, sizeof(data));
	// Если процесс является дочерним
	else {
		// Определяем тип события
		switch((uint8_t) data.event){
			// Если событием является подключение
			case (uint8_t) event_t::CONNECT:
			// Если событием является отключение
			case (uint8_t) event_t::DISCONNECT: {
				// Переходим по всем активным воркерам
				for(auto & wrk : this->workers)
					// Устанавливаем количество подключений
					data.count += wrk.second->adjutants.size();
			} break;
		}
		// Отправляем сообщение дочернему процессу
		this->_cluster.send(wid, (const char *) &data, sizeof(data));
	}
}
/**
 * sendMessage Метод отправки сообщения родительскому процессу
 * @param wid    идентификатор воркера
 * @param aid    идентификатор адъютанта
 * @param pid    идентификатор процесса
 * @param buffer буфер передаваемых данных
 * @param size   размер передаваемых данных
 */
void awh::server::Core::sendMessage(const size_t wid, const size_t aid, const pid_t pid, const char * buffer, const size_t size) noexcept {	
	// Создаём объект сообщения
	data_t data;
	// Устанавливаем идентификатор адъютанта
	data.aid = aid;
	// Количество переданных данных
	size_t bytes = 0, actual = 0;
	// Устанавливаем событие сообщения
	data.event = event_t::MESSAGE;
	// Если процесс является дочерним и данные переданы
	if((buffer != nullptr) && (size > 0) && (this->pid != getpid())){
		// Если данные ещё не все переданы, выполняем передачу
		while(bytes < size){
			// Получаем размер передаваемых данных
			actual = ((size - bytes) > sizeof(data.buffer) ? sizeof(data.buffer) : (size - bytes));
			// Устанавливаем размер передаваемых данных
			data.size = actual;
			// Устанавливаем флаг финального заголовка
			data.fin = ((bytes + actual) == size);
			// Выполняем зануление буфера данных
			memset(data.buffer, 0, sizeof(data.buffer));
			// Выполняем копирование данных в буфер сообщения
			memcpy(data.buffer, buffer + bytes, actual);
			// Отправляем сообщение дочернему процессу
			this->_cluster.send(wid, (const char *) &data, sizeof(data));
			// Устанавливаем задержку передачи данных в 10мс
			this_thread::sleep_for(10ms);
			// Увеличиваем размер переданных данных
			bytes += actual;
		}
	// Если сообщение отсылается родительским процессовм
	} else if((buffer != nullptr) && (size > 0) && (this->pid == getpid()) && this->_cluster.working(wid)) {		
		// Выполняем поиск идентификатора процесса по его адъютанту
		auto it = this->_adjutants.find(aid);
		// Если идентификатор процесса получен
		if(it != this->_adjutants.end()){
			// Если данные ещё не все переданы, выполняем передачу
			while(bytes < size){
				// Получаем размер передаваемых данных
				actual = ((size - bytes) > sizeof(data.buffer) ? sizeof(data.buffer) : (size - bytes));
				// Устанавливаем размер передаваемых данных
				data.size = actual;
				// Устанавливаем флаг финального заголовка
				data.fin = ((bytes + actual) == size);
				// Выполняем зануление буфера данных
				memset(data.buffer, 0, sizeof(data.buffer));
				// Выполняем копирование данных в буфер сообщения
				memcpy(data.buffer, buffer + bytes, actual);
				// Отправляем сообщение дочернему процессу
				this->_cluster.send(wid, it->second, (const char *) &data, sizeof(data));
				// Устанавливаем задержку передачи данных в 10мс
				this_thread::sleep_for(10ms);
				// Увеличиваем размер переданных данных
				bytes += actual;
			}
		}
	}
}
/**
 * accept Функция подключения к серверу
 * @param fd  файловый дескриптор (сокет) подключившегося клиента
 * @param wid идентификатор воркера
 */
void awh::server::Core::accept(const int fd, const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if((wid > 0) && (fd >= 0) && this->_interception){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект подключения
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Определяем тип сокета
			switch((uint8_t) this->net.sonet){
				// Если тип сокета установлен как UDP
				case (uint8_t) sonet_t::UDP: {
					// Создаём бъект адъютанта
					unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
					// Определяем тип протокола подключения
					switch((uint8_t) this->net.family){
						// Если тип протокола подключения IPv4
						case (uint8_t) family_t::IPV4:
							// Устанавливаем сеть, для выхода в интернет
							adj->addr.network.assign(
								this->net.v4.first.begin(),
								this->net.v4.first.end()
							);
						break;
						// Если тип протокола подключения IPv6
						case (uint8_t) family_t::IPV6:
							// Устанавливаем сеть, для выхода в интернет
							adj->addr.network.assign(
								this->net.v6.first.begin(),
								this->net.v6.first.end()
							);
						break;
					}
					// Устанавливаем параметры сокета
					adj->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					// Если unix-сокет используется
					if(this->net.family == family_t::NIX)
						// Выполняем инициализацию сокета
						adj->addr.init(this->net.filename, engine_t::type_t::SERVER);
					// Если unix-сокет не используется, выполняем инициализацию сокета
					else adj->addr.init(wrk->host, wrk->port, (this->net.family == family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_ipV6only);
					// Выполняем разрешение подключения
					if(adj->addr.accept(adj->addr.fd, 0)){
						// Получаем адрес подключения клиента
						adj->ip = adj->addr.ip;
						// Получаем аппаратный адрес клиента
						adj->mac = adj->addr.mac;
						// Получаем порт подключения клиента
						adj->port = adj->addr.port;
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->nanoTimestamp();
						// Выполняем получение контекста сертификата
						this->engine.wrapServer(adj->ectx, &adj->addr);
						// Если подключение не обёрнуто
						if(adj->addr.fd < 0){
							// Выводим сообщение об ошибке
							this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
							// Выходим из функции
							return;
						}
						// Выполняем блокировку потока
						this->_mtx.accept.lock();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->_mtx.accept.unlock();
						// Если процесс не является основным
						if((this->pid != getpid()) && this->_cluster.working(wrk->wid))
							// Выполняем разрешение на отправку сообщения
							this->sendEvent(wrk->wid, ret.first->first, this->_pid, event_t::CONNECT);
						// Добавляем подлючившегося клиента в список клиентов
						else this->_adjutants.emplace(ret.first->first, getpid());
						// Запускаем чтение данных
						this->enabled(engine_t::method_t::READ, ret.first->first);
						// Выполняем функцию обратного вызова
						if(wrk->callback.connect != nullptr) wrk->callback.connect(ret.first->first, wrk->wid, this);
						// Выходим из функции
						return;
					// Подключение не установлено, выводим сообщение об ошибке
					} else this->log->print("accepting for client is broken", log_t::flag_t::CRITICAL);
				} break;
				// Если подключение зашифрованно
				case (uint8_t) sonet_t::DTLS: {
					// Если количество подключившихся клиентов, больше максимально-допустимого количества клиентов
					if(wrk->adjutants.size() >= (size_t) wrk->total){
						// Выводим в консоль информацию
						this->log->print("the number of simultaneous connections, cannot exceed the maximum allowed number of %d", log_t::flag_t::WARNING, wrk->total);
						// Выходим
						break;
					}
					/**
					 * Если операционной системой является MS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Запускаем запись данных на сервер (Для Windows)
						wrk->io.start();
					#endif
					// Создаём бъект адъютанта
					unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
					// Выполняем получение контекста сертификата
					this->engine.wrap(adj->ectx, &wrk->addr, engine_t::type_t::SERVER);
					// Выполняем ожидание входящих подключений
					this->engine.wait(adj->ectx);
					// Устанавливаем параметры сокета
					adj->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
					// Если прикрепление клиента к серверу выполнено
					if(adj->addr.attach(wrk->addr)){
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->nanoTimestamp();
						// Выполняем прикрепление контекста клиента к контексту сервера
						this->engine.attach(adj->ectx, &adj->addr);
						// Получаем адрес подключения клиента
						adj->ip = adj->addr.ip;
						// Получаем аппаратный адрес клиента
						adj->mac = adj->addr.mac;
						// Получаем порт подключения клиента
						adj->port = adj->addr.port;
						// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
						if((wrk->callback.accept != nullptr) && !wrk->callback.accept(adj->ip, adj->mac, adj->port, wrk->wid, this)){
							// Выводим сообщение об ошибке
							this->log->print(
								"access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::WARNING,
								adj->ip.c_str(),
								adj->port,
								adj->mac.c_str(),
								adj->addr.fd,
								getpid()
							);
							// Выходим
							break;
						}
						// Выполняем блокировку потока
						this->_mtx.accept.lock();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->_mtx.accept.unlock();
						// Если процесс не является основным
						if((this->pid != getpid()) && this->_cluster.working(wrk->wid))
							// Выполняем разрешение на отправку сообщения
							this->sendEvent(wrk->wid, ret.first->first, this->_pid, event_t::CONNECT);
						// Добавляем подлючившегося клиента в список клиентов
						else this->_adjutants.emplace(ret.first->first, getpid());
						// Запускаем чтение данных
						this->enabled(engine_t::method_t::READ, ret.first->first);
						// Если вывод информационных данных не запрещён
						if(!this->noinfo)
							// Выводим в консоль информацию
							this->log->print(
								"connect to server client [%s:%d], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::INFO,
								ret.first->second->ip.c_str(),
								ret.first->second->port,
								ret.first->second->mac.c_str(),
								ret.first->second->addr.fd, getpid()
							);
						// Выполняем функцию обратного вызова
						if(wrk->callback.connect != nullptr) wrk->callback.connect(ret.first->first, wrk->wid, this);
					// Подключение не установлено, выводим сообщение об ошибке
					} else this->log->print("accepting for client is broken", log_t::flag_t::CRITICAL);
				} break;
				// Если тип сокета установлен как TCP/IP
				case (uint8_t) sonet_t::TCP:
				// Если тип сокета установлен как TCP/IP TLS
				case (uint8_t) sonet_t::TLS:
				// Если тип сокета установлен как SCTP DTLS
				case (uint8_t) sonet_t::SCTP: {
					// Если количество подключившихся клиентов, больше максимально-допустимого количества клиентов
					if(wrk->adjutants.size() >= (size_t) wrk->total){
						// Выводим в консоль информацию
						this->log->print("the number of simultaneous connections, cannot exceed the maximum allowed number of %d", log_t::flag_t::WARNING, wrk->total);
						// Выходим
						break;
					}
					// Создаём бъект адъютанта
					unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
					// Устанавливаем время жизни подключения
					adj->addr.alive = wrk->keepAlive;
					// Определяем тип сокета
					switch((uint8_t) this->net.sonet){
						/**
						 * Если операционной системой является Linux или FreeBSD
						 */
						#if defined(__linux__) || defined(__FreeBSD__)
							// Если тип сокета установлен как SCTP
							case (uint8_t) sonet_t::SCTP:
								// Устанавливаем параметры сокета
								adj->addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
							break;
						#endif
						// Для всех остальных типов сокетов
						default:
							// Устанавливаем параметры сокета
							adj->addr.sonet(SOCK_STREAM, IPPROTO_TCP);
					}
					// Выполняем разрешение подключения
					if(adj->addr.accept(wrk->addr)){
						// Получаем адрес подключения клиента
						adj->ip = adj->addr.ip;
						// Получаем аппаратный адрес клиента
						adj->mac = adj->addr.mac;
						// Получаем порт подключения клиента
						adj->port = adj->addr.port;
						// Если функция обратного вызова проверки подключения установлена, выполняем проверку, если проверка не пройдена?
						if((wrk->callback.accept != nullptr) && !wrk->callback.accept(adj->ip, adj->mac, adj->port, wrk->wid, this)){
							// Выводим сообщение об ошибке
							this->log->print(
								"access to the server is denied for the client [%s:%d], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::WARNING,
								adj->ip.c_str(),
								adj->port,
								adj->mac.c_str(),
								adj->addr.fd,
								getpid()
							);
							// Выходим
							break;
						}
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->nanoTimestamp();
						// Выполняем получение контекста сертификата
						this->engine.wrapServer(adj->ectx, &adj->addr);
						// Если мы хотим работать в зашифрованном режиме
						if(this->net.sonet == sonet_t::TLS){
							// Если сертификаты не приняты, выходим
							if(!this->engine.isTLS(adj->ectx)){
								// Выводим сообщение об ошибке
								this->log->print("encryption mode cannot be activated", log_t::flag_t::CRITICAL);
								// Выходим
								break;
							}
						}
						// Если подключение не обёрнуто
						if(adj->addr.fd < 0){
							// Выводим сообщение об ошибке
							this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
							// Выходим
							break;
						}
						// Выполняем блокировку потока
						this->_mtx.accept.lock();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->_mtx.accept.unlock();
						// Если процесс не является основным
						if((this->pid != getpid()) && this->_cluster.working(wrk->wid))
							// Выполняем разрешение на отправку сообщения
							this->sendEvent(wrk->wid, ret.first->first, this->_pid, event_t::CONNECT);
						// Добавляем подлючившегося клиента в список клиентов
						else this->_adjutants.emplace(ret.first->first, getpid());
						// Запускаем чтение данных
						this->enabled(engine_t::method_t::READ, ret.first->first);
						// Если вывод информационных данных не запрещён
						if(!this->noinfo)
							// Выводим в консоль информацию
							this->log->print(
								"connect to server client [%s:%d], mac = %s, socket = %d, pid = %d",
								log_t::flag_t::INFO,
								ret.first->second->ip.c_str(),
								ret.first->second->port,
								ret.first->second->mac.c_str(),
								ret.first->second->addr.fd, getpid()
							);
						// Выполняем функцию обратного вызова
						if(wrk->callback.connect != nullptr) wrk->callback.connect(ret.first->first, wrk->wid, this);
					// Если подключение не установлено, выводим сообщение об ошибке
					} else this->log->print("accepting for client is broken", log_t::flag_t::CRITICAL);
				} break;
			}
		}
	}
}
/**
 * close Метод отключения всех воркеров
 */
void awh::server::Core::close() noexcept {
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (worker.second);
			// Если в воркере есть подключённые клиенты
			if(!wrk->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->_locking.count(it->first) < 1){
						// Выполняем блокировку адъютанта
						this->_locking.emplace(it->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second.get());
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Выполняем очистку контекста двигателя
						adj->ectx.clear();
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(it->first);
						// Выводим функцию обратного вызова
						if(wrk->callback.disconnect != nullptr)
							// Выполняем функцию обратного вызова
							wrk->callback.disconnect(it->first, worker.first, this);
						// Если процесс не является основным
						if((this->pid != getpid()) && this->_cluster.working(wrk->wid))
							// Выполняем разрешение на отправку сообщения
							this->sendEvent(wrk->wid, it->first, this->_pid, event_t::DISCONNECT);
						// Удаляем отключившегося клиента из списока клиентов
						else this->_adjutants.erase(it->first);
						// Удаляем блокировку адъютанта
						this->_locking.erase(it->first);
						// Удаляем адъютанта из списка
						it = wrk->adjutants.erase(it);
					// Иначе продолжаем дальше
					} else ++it;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(wrk->wid);
			}
			// Останавливаем работу сервера
			wrk->io.stop();
			// Выполняем закрытие подключение сервера
			wrk->addr.close();
		}
		// Выполняем очистку буфера сообщений дочерних процессов
		this->_messages.clear();
	}
}
/**
 * remove Метод удаления всех воркеров
 */
void awh::server::Core::remove() noexcept {
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Переходим по всему списку воркеров
		for(auto it = this->workers.begin(); it != this->workers.end();){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если в воркере есть подключённые клиенты
			if(!wrk->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto jt = wrk->adjutants.begin(); jt != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->_locking.count(jt->first) < 1){
						// Выполняем блокировку адъютанта
						this->_locking.emplace(jt->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем очистку контекста двигателя
						adj->ectx.clear();
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Выводим функцию обратного вызова
						if(wrk->callback.disconnect != nullptr)
							// Выполняем функцию обратного вызова
							wrk->callback.disconnect(jt->first, it->first, this);
						// Если процесс не является основным
						if((this->pid != getpid()) && this->_cluster.working(wrk->wid))
							// Выполняем разрешение на отправку сообщения
							this->sendEvent(wrk->wid, jt->first, this->_pid, event_t::DISCONNECT);
						// Удаляем отключившегося клиента из списока клиентов
						else this->_adjutants.erase(jt->first);
						// Удаляем блокировку адъютанта
						this->_locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = wrk->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
				// Останавливаем работу кластера
				this->_cluster.stop(wrk->wid);
			}
			// Останавливаем работу сервера
			wrk->io.stop();
			// Выполняем закрытие подключение сервера
			wrk->addr.close();
			// Выполняем удаление воркера
			it = this->workers.erase(it);
		}
		// Выполняем очистку буфера сообщений дочерних процессов
		this->_messages.clear();
	}
}
/**
 * run Метод запуска сервера воркером
 * @param wid идентификатор воркера
 */
void awh::server::Core::run(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Устанавливаем базу событий кластера
			this->_cluster.base(this->dispatch.base);
			// Выполняем инициализацию кластера
			this->_cluster.init(it->first, this->_clusterSize);
			// Получаем объект подключения
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если хост сервера не указан
			if(wrk->host.empty()){
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet(this->fmk, this->log);
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения unix-сокет
					case (uint8_t) family_t::NIX:
					// Если тип протокола подключения IPv4
					case (uint8_t) family_t::IPV4:
						// Обновляем хост сервера
						wrk->host = ifnet.ip(AF_INET);
					break;
					// Если тип протокола подключения IPv6
					case (uint8_t) family_t::IPV6:
						// Обновляем хост сервера
						wrk->host = ifnet.ip(AF_INET6);
					break;
				}
			// Если хост сервера является доменным именем
			} else {
				// Определяем тип передаваемого сервера
				switch((uint8_t) this->nwk.parseHost(wrk->host)){
					// Если хост является аппаратным адресом сетевого интерфейса
					case (uint8_t) network_t::type_t::MAC: break;
					// Если хост является доменом или IPv4 адресом
					case (uint8_t) network_t::type_t::IPV4: break;
					// Если хост является доменом или IPv6 адресом
					case (uint8_t) network_t::type_t::IPV6: break;
					// Если хост является адресом/Маски сети
					case (uint8_t) network_t::type_t::NETWORK: break;
					// Если хост является адресом в файловой системе
					case (uint8_t) network_t::type_t::ADDRESS: break;
					// Если хост является HTTP методом
					case (uint8_t) network_t::type_t::HTTPMETHOD: break;
					// Если хост является HTTP адресом
					case (uint8_t) network_t::type_t::HTTPADDRESS: break;
					// Значит скорее всего, садрес является доменным именем
					default: {
						// Определяем тип протокола подключения
						switch((uint8_t) this->net.family){
							// Если тип протокола подключения IPv4
							case (uint8_t) family_t::IPV4: {
								/**
								 * Методы только для OS Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Выполняем резолвинг доменного имени
									struct hostent * host = gethostbyname(wrk->host.c_str());
								/**
								 * Если операционной системой является Nix-подобная
								 */
								#else
									// Выполняем резолвинг доменного имени
									struct hostent * host = gethostbyname2(wrk->host.c_str(), AF_INET);
								#endif
								// Если полученный IP адрес не соответствует сети IPv4
								if(host->h_addrtype != AF_INET){
									// Объект для работы с сетевым интерфейсом
									ifnet_t ifnet(this->fmk, this->log);
									// Устанавливаем хост сервера
									wrk->host = ifnet.ip(AF_INET);
								// Если IP адрес соответствует сети IPv4
								} else {
									// Создаём объект сервера
									struct sockaddr_in server;
									// Очищаем всю структуру для сервера
									memset(&server, 0, sizeof(server));
									// Создаем буфер для получения ip адреса
									char buffer[INET_ADDRSTRLEN];
									// Заполняем структуру нулями
									memset(buffer, 0, sizeof(buffer));
									// Устанавливаем протокол интернета
									server.sin_family = AF_INET;
									// Выполняем копирование данных типа подключения
									memcpy(&server.sin_addr.s_addr, host->h_addr, host->h_length);
									// Копируем полученные данные
									inet_ntop(AF_INET, &server.sin_addr, buffer, sizeof(buffer));
									// Устанавливаем хост сервера
									wrk->host = buffer;
								}
							} break;
							// Если тип протокола подключения IPv6
							case (uint8_t) family_t::IPV6: {
								/**
								 * Методы только для OS Windows
								 */
								#if defined(_WIN32) || defined(_WIN64)
									// Выполняем резолвинг доменного имени
									struct hostent * host = gethostbyname(wrk->host.c_str());
								/**
								 * Если операционной системой является Nix-подобная
								 */
								#else
									// Выполняем резолвинг доменного имени
									struct hostent * host = gethostbyname2(wrk->host.c_str(), AF_INET6);
								#endif
								// Если полученный IP адрес не соответствует сети IPv6
								if(host->h_addrtype != AF_INET6){
									// Объект для работы с сетевым интерфейсом
									ifnet_t ifnet(this->fmk, this->log);
									// Устанавливаем хост сервера
									wrk->host = ifnet.ip(AF_INET6);
								// Если IP адрес соответствует сети IPv6
								} else {
									// Создаём объект сервера
									struct sockaddr_in6 server;
									// Очищаем всю структуру для сервера
									memset(&server, 0, sizeof(server));
									// Создаем буфер для получения ip адреса
									char buffer[INET6_ADDRSTRLEN];
									// Заполняем структуру нулями
									memset(buffer, 0, sizeof(buffer));
									// Устанавливаем протокол интернета
									server.sin6_family = AF_INET6;
									// Выполняем копирование данных типа подключения
									memcpy(&server.sin6_addr.s6_addr, host->h_addr, host->h_length);
									// Копируем полученные данные
									inet_ntop(AF_INET6, &server.sin6_addr, buffer, sizeof(buffer));
									// Устанавливаем хост сервера
									wrk->host = buffer;
								}
							} break;
						}
					}
				}
			}
			// Определяем тип передаваемого сервера
			switch((uint8_t) this->nwk.parseHost(wrk->host)){
				// Если хост является доменом или IPv4 адресом
				case (uint8_t) network_t::type_t::IPV4:
					// Выполняем запуск сервера
					this->resolving(wrk->wid, wrk->host, AF_INET, 0);
				break;
				// Если хост является доменом или IPv6 адресом
				case (uint8_t) network_t::type_t::IPV6:
					// Выполняем запуск сервера
					this->resolving(wrk->wid, wrk->host, AF_INET6, 0);
				break;
			}


			this->dns.on([](const string & ip, const int family, const size_t did) noexcept {
				cout << " ============================= " << ip << endl;
			});

			cout << " --------------------- " << this->dns.resolve("testnet.binance.vision", AF_INET) << endl;
			
			// cout << " --------------------- " << this->dns.resolve("api.binance.com", AF_INET) << endl;

			
			{
				dns_t::serv_t serv;
				serv.host = "ns1.yandex.net";

				this->dns.server(AF_INET, serv);
			}

		}
	}
}
/**
 * remove Метод удаления воркера
 * @param wid идентификатор воркера
 */
void awh::server::Core::remove(const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.close);
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если в воркере есть подключённые клиенты
			if(!wrk->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto jt = wrk->adjutants.begin(); jt != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->_locking.count(jt->first) < 1){
						// Выполняем блокировку адъютанта
						this->_locking.emplace(jt->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем очистку контекста двигателя
						adj->ectx.clear();
						// Выводим функцию обратного вызова
						if(wrk->callback.disconnect != nullptr)
							// Выполняем функцию обратного вызова
							wrk->callback.disconnect(jt->first, it->first, this);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Если процесс не является основным
						if((this->pid != getpid()) && this->_cluster.working(wrk->wid))
							// Выполняем разрешение на отправку сообщения
							this->sendEvent(wrk->wid, jt->first, this->_pid, event_t::DISCONNECT);
						// Удаляем отключившегося клиента из списока клиентов
						else this->_adjutants.erase(jt->first);
						// Удаляем блокировку адъютанта
						this->_locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = wrk->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Останавливаем работу сервера
			wrk->io.stop();
			// Выполняем закрытие подключение сервера
			wrk->addr.close();
			// Выполняем удаление воркера
			this->workers.erase(wid);
		}
	}
}
/**
 * close Метод закрытия подключения воркера
 * @param aid идентификатор адъютанта
 */
void awh::server::Core::close(const size_t aid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.close);
	// Если тип сокета установлен как не UDP, останавливаем чтение
	if(this->net.sonet != sonet_t::UDP){
		// Если блокировка адъютанта не установлена
		if(this->_locking.count(aid) < 1){
			// Выполняем блокировку адъютанта
			this->_locking.emplace(aid);
			// Выполняем извлечение адъютанта
			auto it = this->adjutants.find(aid);
			// Если адъютант получен
			if(it != this->adjutants.end()){
				// Получаем объект адъютанта
				awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
				// Получаем объект воркера
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
				// Получаем объект ядра клиента
				const core_t * core = reinterpret_cast <const core_t *> (wrk->core);
				// Выполняем очистку буфера событий
				this->clean(aid);
				// Выполняем очистку контекста двигателя
				adj->ectx.clear();
				// Удаляем адъютанта из списка адъютантов
				wrk->adjutants.erase(aid);
				// Удаляем адъютанта из списка подключений
				this->adjutants.erase(aid);
				// Если процесс не является основным
				if((this->pid != getpid()) && this->_cluster.working(wrk->wid))
					// Выполняем разрешение на отправку сообщения
					this->sendEvent(wrk->wid, aid, this->_pid, event_t::DISCONNECT);
				// Удаляем отключившегося клиента из списока клиентов
				else this->_adjutants.erase(aid);
				// Выводим сообщение об ошибке
				if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnect client from server");
				// Выводим функцию обратного вызова
				if(wrk->callback.disconnect != nullptr) wrk->callback.disconnect(aid, wrk->wid, this);
			}
			// Удаляем блокировку адъютанта
			this->_locking.erase(aid);
		}
	}
}
/**
 * timeout Функция обратного вызова при срабатывании таймаута
 * @param aid идентификатор адъютанта
 */
void awh::server::Core::timeout(const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект подключения
		worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Определяем тип протокола подключения
		switch((uint8_t) this->net.family){
			// Если тип протокола подключения IPv4
			case (uint8_t) family_t::IPV4:
			// Если тип протокола подключения IPv6
			case (uint8_t) family_t::IPV6:
				// Выводим сообщение в лог, о таймауте подключения
				this->log->print("timeout host = %s, mac = %s", log_t::flag_t::WARNING, adj->ip.c_str(), adj->mac.c_str());
			break;
			// Если тип протокола подключения unix-сокет
			case (uint8_t) family_t::NIX:
				// Выводим сообщение в лог, о таймауте подключения
				this->log->print("timeout host %s", log_t::flag_t::WARNING, this->net.filename.c_str());
			break;
		}
		// Останавливаем чтение данных
		this->disabled(engine_t::method_t::READ, it->first);
		// Останавливаем запись данных
		this->disabled(engine_t::method_t::WRITE, it->first);
		// Выполняем отключение клиента
		this->close(aid);
	}
}
/**
 * write Функция обратного вызова при записи данных в сокет
 * @param method метод режима работы
 * @param aid    идентификатор адъютанта
 */
void awh::server::Core::transfer(const engine_t::method_t method, const size_t aid) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Получаем объект адъютанта
		awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
		// Получаем объект подключения
		worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
		// Определяем метод работы
		switch((uint8_t) method){
			// Если производится чтение данных
			case (uint8_t) engine_t::method_t::READ: {
				// Количество полученных байт
				int64_t bytes = -1;
				// Создаём буфер входящих данных
				char buffer[BUFFER_SIZE];
				// Останавливаем чтение данных с клиента
				adj->bev.event.read.stop();
				// Выполняем перебор бесконечным циклом пока это разрешено
				while(!adj->bev.locked.read){
					// Выполняем получение сообщения от клиента
					bytes = adj->ectx.read(buffer, sizeof(buffer));
					// Если время ожидания чтения данных установлено
					if(wrk->wait && (adj->timeouts.read > 0)){
						// Устанавливаем время ожидания на получение данных
						adj->bev.timer.read.repeat = adj->timeouts.read;
						// Запускаем повторное ожидание
						adj->bev.timer.read.again();
					// Останавливаем таймаут ожидания на чтение из сокета
					} else adj->bev.timer.read.stop();
					/**
					 * Если операционной системой является MS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Запускаем чтение данных снова (Для Windows)
						if((bytes != 0) && (this->net.sonet != sonet_t::UDP))
							// Запускаем чтение снова
							adj->bev.event.read.start();
					#endif
					// Если данные получены
					if(bytes > 0){
						// Если данные считанные из буфера, больше размера ожидающего буфера
						if((adj->marker.write.max > 0) && (bytes >= adj->marker.write.max)){
							// Смещение в буфере и отправляемый размер данных
							size_t offset = 0, actual = 0;
							// Выполняем пересылку всех полученных данных
							while((bytes - offset) > 0){
								// Определяем размер отправляемых данных
								actual = ((bytes - offset) >= adj->marker.write.max ? adj->marker.write.max : (bytes - offset));
								// Если функция обратного вызова на получение данных установлена
								if(wrk->callback.read != nullptr)
									// Выводим функцию обратного вызова
									wrk->callback.read(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
								// Увеличиваем смещение в буфере
								offset += actual;
							}
						// Если данных достаточно и функция обратного вызова на получение данных установлена
						} else if(wrk->callback.read != nullptr)
							// Выводим функцию обратного вызова
							wrk->callback.read(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
					// Если данные не могут быть прочитаны
					} else {
						// Если нужно повторить попытку
						if(bytes == -2) continue;
						// Если нужно выйти из цикла
						else if(bytes == -1) break;
						// Если нужно завершить работу
						else if(bytes == 0) {
							// Выполняем отключение клиента
							this->close(aid);
							// Выходим из функции
							return;
						}
					}
					// Выходим из цикла
					break;
				}
				// Если тип сокета не установлен как UDP, запускаем чтение дальше
				if((this->net.sonet != sonet_t::UDP) && (this->adjutants.count(aid) > 0))
					// Запускаем чтение данных с клиента
					adj->bev.event.read.start();
			} break;
			// Если производится запись данных
			case (uint8_t) engine_t::method_t::WRITE: {
				// Останавливаем таймаут ожидания на запись в сокет
				adj->bev.timer.write.stop();
				// Если данных достаточно для записи в сокет
				if(adj->buffer.size() >= adj->marker.write.min){
					// Получаем буфер отправляемых данных
					vector <char> buffer;
					// Количество полученных байт
					int64_t bytes = -1;
					// Cмещение в буфере и отправляемый размер данных
					size_t offset = 0, actual = 0, size = 0;
					// Выполняем отправку данных пока всё не отправим
					while(!adj->bev.locked.write && ((adj->buffer.size() - offset) > 0)){
						// Получаем общий размер буфера данных
						size = (adj->buffer.size() - offset);
						// Определяем размер отправляемых данных
						actual = ((size >= adj->marker.write.max) ? adj->marker.write.max : size);
						// Выполняем отправку сообщения клиенту
						bytes = adj->ectx.write(adj->buffer.data() + offset, actual);
						// Если данные записаны удачно
						if(bytes > 0)
							// Добавляем записанные байты в буфер
							buffer.insert(buffer.end(), adj->buffer.data() + offset, (adj->buffer.data() + offset) + bytes);
						// Если время ожидания записи данных установлено
						if(adj->timeouts.write > 0){
							// Устанавливаем время ожидания на запись данных
							adj->bev.timer.write.repeat = adj->timeouts.write;
							// Запускаем повторное ожидание
							adj->bev.timer.write.again();
						// Останавливаем таймаут ожидания на запись в сокет
						} else adj->bev.timer.write.stop();
						// Если нужно повторить попытку
						if(bytes == -2) continue;
						// Если нужно выйти из цикла
						else if(bytes == -1) break;
						// Если нужно завершить работу
						else if(bytes == 0) {
							// Выполняем отключение клиента
							this->close(aid);
							// Выходим из функции
							return;
						}
						// Увеличиваем смещение в буфере
						offset += actual;
					}
					// Если данных в буфере больше чем количество записанных байт
					if(adj->buffer.size() > offset)
						// Выполняем удаление из буфера данных количество отправленных байт
						adj->buffer.assign(adj->buffer.begin() + offset, adj->buffer.end());
					// Иначе просто очищаем буфер данных
					else adj->buffer.clear();
					// Останавливаем запись данных
					if(adj->buffer.empty()) this->disabled(engine_t::method_t::WRITE, aid);
					// Если функция обратного вызова на запись данных установлена
					if(wrk->callback.write != nullptr)
						// Выводим функцию обратного вызова
						wrk->callback.write(buffer.data(), buffer.size(), aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
				// Если данных недостаточно для записи в сокет
				} else {
					// Останавливаем запись данных
					this->disabled(engine_t::method_t::WRITE, aid);
					// Если функция обратного вызова на запись данных установлена
					if(wrk->callback.write != nullptr)
						// Выводим функцию обратного вызова
						wrk->callback.write(nullptr, 0, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
				}
				// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
				if(adj->buffer.empty() && (this->net.sonet == sonet_t::UDP) && (this->adjutants.count(aid) > 0))
					// Запускаем чтение данных с клиента
					adj->bev.event.read.start();
			} break;
		}
	}
}
/**
 * resolving Метод получения IP адреса доменного имени
 * @param wid    идентификатор воркера
 * @param ip     адрес интернет-подключения
 * @param family тип интернет-протокола AF_INET, AF_INET6
 * @param did    идентификатор DNS запроса
 */
void awh::server::Core::resolving(const size_t wid, const string & ip, const int family, const size_t did) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Получаем объект подключения
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если IP адрес получен
			if(!ip.empty()){
				// sudo lsof -i -P | grep 1080
				// Обновляем хост сервера
				wrk->host = ip;
				// Определяем тип сокета
				switch((uint8_t) this->net.sonet){
					// Если тип сокета установлен как UDP
					case (uint8_t) sonet_t::UDP: {
						// Если разрешено выводить информационные сообщения
						if(!this->noinfo){
							// Если unix-сокет используется
							if(this->net.family == family_t::NIX)
								// Выводим информацию о запущенном сервере на unix-сокете
								this->log->print("run server [%s]", log_t::flag_t::INFO, this->net.filename.c_str());
							// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
							else this->log->print("run server [%s:%u]", log_t::flag_t::INFO, wrk->host.c_str(), wrk->port);
						}
						// Получаем тип операционной системы
						const fmk_t::os_t os = this->fmk->os();
						// Если операционная система является Windows или количество процессов всего один
						if((this->_interception = (this->_cluster.count(wrk->wid) == 1)))
							// Выполняем активацию сервера
							this->accept(1, wrk->wid);
						// Выполняем запуск кластера
						else this->_cluster.start(wrk->wid);
						// Выходим из функции
						return;
					} break;
					// Для всех остальных типов сокетов
					default: {
						// Определяем тип протокола подключения
						switch((uint8_t) this->net.family){
							// Если тип протокола подключения IPv4
							case (uint8_t) family_t::IPV4:
								// Устанавливаем сеть, для выхода в интернет
								wrk->addr.network.assign(
									this->net.v4.first.begin(),
									this->net.v4.first.end()
								);
							break;
							// Если тип протокола подключения IPv6
							case (uint8_t) family_t::IPV6:
								// Устанавливаем сеть, для выхода в интернет
								wrk->addr.network.assign(
									this->net.v6.first.begin(),
									this->net.v6.first.end()
								);
							break;
						}
						// Определяем тип сокета
						switch((uint8_t) this->net.sonet){
							// Если тип сокета установлен как UDP TLS
							case (uint8_t) sonet_t::DTLS:
								// Устанавливаем параметры сокета
								wrk->addr.sonet(SOCK_DGRAM, IPPROTO_UDP);
							break;
							/**
							 * Если операционной системой является Linux или FreeBSD
							 */
							#if defined(__linux__) || defined(__FreeBSD__)
								// Если тип сокета установлен как SCTP
								case (uint8_t) sonet_t::SCTP:
									// Устанавливаем параметры сокета
									wrk->addr.sonet(SOCK_STREAM, IPPROTO_SCTP);
								break;
							#endif
							// Для всех остальных типов сокетов
							default:
								// Устанавливаем параметры сокета
								wrk->addr.sonet(SOCK_STREAM, IPPROTO_TCP);
						}
						// Если unix-сокет используется
						if(this->net.family == family_t::NIX)
							// Выполняем инициализацию сокета
							wrk->addr.init(this->net.filename, engine_t::type_t::SERVER);
						// Если unix-сокет не используется, выполняем инициализацию сокета
						else wrk->addr.init(wrk->host, wrk->port, (this->net.family == family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER, this->_ipV6only);
						// Если сокет подключения получен
						if(wrk->addr.fd > -1){
							// Если повесить прослушку на порт не вышло, выходим из условия
							if(!wrk->addr.list()) break;
							// Если разрешено выводить информационные сообщения
							if(!this->noinfo){
								// Если unix-сокет используется
								if(this->net.family == family_t::NIX)
									// Выводим информацию о запущенном сервере на unix-сокете
									this->log->print("run server [%s]", log_t::flag_t::INFO, this->net.filename.c_str());
								// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
								else this->log->print("run server [%s:%u]", log_t::flag_t::INFO, wrk->host.c_str(), wrk->port);
							}
							// Получаем тип операционной системы
							const fmk_t::os_t os = this->fmk->os();
							// Если операционная система является Windows или количество процессов всего один
							if((this->_interception = (this->_cluster.count(wrk->wid) == 1))){
								// Устанавливаем базу событий
								wrk->io.set(this->dispatch.base);
								// Устанавливаем событие на чтение данных подключения
								wrk->io.set <worker_t, &worker_t::accept> (wrk);
								// Устанавливаем сокет для чтения
								wrk->io.set(wrk->addr.fd, ev::READ);
								// Запускаем чтение данных с клиента
								wrk->io.start();
							// Выполняем запуск кластера
							} else this->_cluster.start(wrk->wid);
							// Выходим из функции
							return;
						// Если сокет не создан, выводим в консоль информацию
						} else {
							// Если unix-сокет используется
							if(this->net.family == family_t::NIX)
								// Выводим информацию об незапущенном сервере на unix-сокете
								this->log->print("server cannot be started [%s]", log_t::flag_t::CRITICAL, this->net.filename.c_str());
							// Если unix-сокет не используется, выводим сообщение об незапущенном сервере за порту
							else this->log->print("server cannot be started [%s:%u]", log_t::flag_t::CRITICAL, wrk->host.c_str(), wrk->port);
						}
					}
				}
			// Если IP адрес сервера не получен, выводим в консоль информацию
			} else this->log->print("broken host server %s", log_t::flag_t::CRITICAL, wrk->host.c_str());
			// Останавливаем работу сервера
			this->stop();
		}
	}
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Core::bandWidth(const size_t aid, const string & read, const string & write) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Устанавливаем размер буфера
			adj->ectx.buffer(
				(!read.empty() ? this->fmk->sizeBuffer(read) : 0),
				(!write.empty() ? this->fmk->sizeBuffer(write) : 0),
				reinterpret_cast <const worker_t *> (adj->parent)->total
			);
		/**
		 * Если операционной системой является MS Windows
		 */
		#else
			// Блокируем вывод переменных
			(void) read;
			(void) write;
		#endif
	}
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Core::ipV6only(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx.main);
	// Устанавливаем флаг использования только сети IPv6
	this->_ipV6only = mode;
}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::Core::clusterSize(const size_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Устанавливаем количество рабочих процессов кластера
		this->_clusterSize = size;
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param wid   идентификатор воркера
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Core::total(const size_t wid, const u_short total) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx.main);
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end())
			// Устанавливаем максимальное количество одновременных подключений
			((worker_t *) const_cast <awh::worker_t *> (it->second))->total = total;
	}
}
/**
 * init Метод инициализации сервера
 * @param wid  идентификатор воркера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Core::init(const size_t wid, const u_int port, const string & host) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx.main);
			// Получаем объект подключения
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если порт передан, устанавливаем
			if(port > 0) wrk->port = port;
			// Если хост передан, устанавливаем
			if(!host.empty()) wrk->host = host;
			// Иначе получаем IP адрес сервера автоматически
			else {
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet(this->fmk, this->log);
				// Определяем тип протокола подключения
				switch((uint8_t) this->net.family){
					// Если тип протокола подключения unix-сокет
					case (uint8_t) family_t::NIX:
					// Если тип протокола подключения IPv4
					case (uint8_t) family_t::IPV4:
						// Обновляем хост сервера
						wrk->host = ifnet.ip(AF_INET);
					break;
					// Если тип протокола подключения IPv6
					case (uint8_t) family_t::IPV6:
						// Обновляем хост сервера
						wrk->host = ifnet.ip(AF_INET6);
					break;
				}
			}
		}
	}
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), _pid(0), _cluster(fmk, log), _ipV6only(false), _interception(false), _clusterSize(1) {
	// Устанавливаем тип запускаемого ядра
	this->type = engine_t::type_t::SERVER;
	// Устанавливаем функцию получения статуса кластера
	this->_cluster.on(std::bind(&core_t::cluster, this, _1, _2, _3));
	// Устанавливаем функцию получения сообщений кластера
	this->_cluster.on(std::bind(&core_t::message, this, _1, _2, _3, _4));
}
/**
 * ~Core Деструктор
 */
awh::server::Core::~Core() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
