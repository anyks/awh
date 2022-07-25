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
 * resolver Функция выполнения резолвинга домена
 * @param ip  полученный IP адрес
 * @param wrk объект воркера
 */
void awh::server::Core::resolver(const string & ip, worker_t * wrk) noexcept {
	// Получаем объект ядра подключения
	core_t * core = (core_t *) const_cast <awh::core_t *> (wrk->core);
	// Если IP адрес получен
	if(!ip.empty()){
		// sudo lsof -i -P | grep 1080
		// Обновляем хост сервера
		wrk->host = ip;
		// Получаем сокет сервера
		wrk->fd = core->sockaddr(wrk->host, wrk->port, core->net.family).fd;
		// Если сокет сервера создан
		if(wrk->fd > -1){
			// Выполняем чтение сокета
			if(::listen(wrk->fd, wrk->total) < 0){
				// Выводим в консоль информацию
				if(!core->noinfo) core->log->print("listen service: pid = %u", log_t::flag_t::CRITICAL, getpid());
				// Выходим из функции
				goto Stop;
			}
			// Запоминаем PID родительского процесса
			core->pid = getpid();
			// Выводим сообщение об активации
			if(!core->noinfo) core->log->print("run server [%s:%u]", log_t::flag_t::INFO, wrk->host.c_str(), wrk->port);
			// Устанавливаем базу событий
			wrk->io.set(this->base);
			// Устанавливаем событие на запись данных подключения
			wrk->io.set <worker_t, &worker_t::accept> (wrk);
			// Устанавливаем сокет для записи
			wrk->io.set(wrk->fd, ev::READ);
			// Запускаем запись данных на сервер
			wrk->io.start();
			// Выходим из функции
			return;
		// Если сокет не создан, выводим в консоль информацию
		} else core->log->print("server cannot be started [%s:%u]", log_t::flag_t::CRITICAL, wrk->host.c_str(), wrk->port);
	// Если IP адрес сервера не получен, выводим в консоль информацию
	} else core->log->print("broken host server %s", log_t::flag_t::CRITICAL, wrk->host.c_str());
	// Устанавливаем метку
	Stop:
	// Останавливаем работу сервера
	core->stop();
}
/**
 * close Метод закрытия сокета
 * @param fd файловый дескриптор (сокет) для закрытия
 */
void awh::server::Core::close(const int fd) noexcept {
	// Если сокет активен
	if(fd > -1){
		// Если - это Windows
		#if defined(_WIN32) || defined(_WIN64)
			// Запрещаем работу с сокетом
			shutdown(fd, SD_BOTH);
			// Выполняем закрытие сокета
			closesocket(fd);
		// Если - это Unix
		#else
			// Запрещаем работу с сокетом
			shutdown(fd, SHUT_RDWR);
			// Выполняем закрытие сокета
			::close(fd);
		#endif
	}
}
/**
 * accept Функция подключения к серверу
 * @param fd  файловый дескриптор (сокет) подключившегося клиента
 * @param wid идентификатор воркера
 */
void awh::server::Core::accept(const int fd, const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if((wid > 0) && (fd >= 0)){
		// Выполняем поиск воркера
		auto it = this->workers.find(wid);
		// Если воркер найден, устанавливаем максимальное количество одновременных подключений
		if(it != this->workers.end()){
			// Сокет подключившегося клиента
			int socket = -1;
			// IP и MAC адрес подключения
			string ip = "", mac = "";
			// Получаем объект подключения
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Определяем тип подключения
			switch(this->net.family){
				// Для протокола IPv4
				case AF_INET: {
					// Структура получения
					struct sockaddr_in client;
					// Размер структуры подключения
					socklen_t len = sizeof(client);
					// Определяем разрешено ли подключение к прокси серверу
					socket = ::accept(fd, reinterpret_cast <struct sockaddr *> (&client), &len);
					// Если сокет не создан тогда выходим
					if(socket < 0) return;
					// Получаем данные подключившегося клиента
					ip = this->ifnet.ip((struct sockaddr *) &client, AF_INET);
					// Если IP адрес получен пустой, устанавливаем адрес сервера
					if(ip.compare("0.0.0.0") == 0) ip = this->ifnet.ip(AF_INET);
					// Получаем данные мак адреса клиента
					mac = this->ifnet.mac(ip, AF_INET);
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Структура получения
					struct sockaddr_in6 client;
					// Размер структуры подключения
					socklen_t len = sizeof(client);
					// Определяем разрешено ли подключение к прокси серверу
					socket = ::accept(fd, reinterpret_cast <struct sockaddr *> (&client), &len);
					// Если сокет не создан тогда выходим
					if(socket < 0) return;
					// Получаем данные подключившегося клиента
					ip = this->ifnet.ip((struct sockaddr *) &client, AF_INET6);
					// Если IP адрес получен пустой, устанавливаем адрес сервера
					if(ip.compare("::") == 0) ip = this->ifnet.ip(AF_INET6);
					// Получаем данные мак адреса клиента
					mac = this->ifnet.mac(ip, AF_INET6);
				} break;
			}
			// Устанавливаем настройки для *Nix подобных систем
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->socket.noSigill();
				// Отключаем сигнал записи в оборванное подключение
				this->socket.noSigpipe(socket);
			#endif
			// Устанавливаем разрешение на повторное использование сокета
			this->socket.reuseable(socket);
			// Отключаем алгоритм Нейгла для сервера и клиента
			this->socket.tcpNodelay(socket);
			// Переводим сокет в не блокирующий режим
			this->socket.nonBlocking(socket);
			// Создаём бъект адъютанта
			unique_ptr <awh::worker_t::adj_t> adj(new awh::worker_t::adj_t(wrk, this->fmk, this->log));
			// Выполняем удаление контекста SSL
			this->ssl.clear(adj->ssl);
			// Выполняем получение контекста сертификата
			adj->ssl = this->ssl.init();
			// Выполняем блокировку потока
			this->mtx.accept.lock();
			// Запоминаем файловый дескриптор
			adj->bev.socket = socket;
			// Устанавливаем идентификатор адъютанта
			adj->aid = this->fmk->unixTimestamp();
			// Выполняем блокировку потока
			this->mtx.accept.unlock();
			// Если защищённый режим работы разрешён
			if(adj->ssl.mode){
				// Выполняем обёртывание сокета в BIO SSL
				BIO * bio = BIO_new_socket(adj->bev.socket, BIO_NOCLOSE);
				// Если BIO SSL создано
				if(bio != nullptr){
					// Устанавливаем блокирующий режим ввода/вывода для сокета
					BIO_set_nbio(bio, 1);
					// Выполняем установку BIO SSL
					SSL_set_bio(adj->ssl.ssl, bio, bio);
					// Устанавливаем флаг работы в асинхронном режиме
					SSL_set_mode(adj->ssl.ssl, SSL_MODE_ASYNC);
					// Устанавливаем флаг записи в буфер порциями
					SSL_set_mode(adj->ssl.ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
					// Выполняем активацию клиента SSL
					SSL_set_accept_state(adj->ssl.ssl);
					// Выполняем проверку на подключение
					const int error = SSL_accept(adj->ssl.ssl);
					// Если возникла ошибка
					if(error <= 0){
						// Очищаем ошибки SSL
						ERR_clear_error();
						// Выполняем чтение ошибки OpenSSL
						const int code = SSL_get_error(adj->ssl.ssl, error);
						// Выполняем проверку на подключение
						if(code == SSL_ERROR_WANT_ACCEPT){
							// Выполняем попытку снова
							this->accept(fd, wid);
							// Выходим из функции
							return;
						// Если возникла другая ошибка
						} else if(code != SSL_ERROR_NONE) {
							// Получаем данные описание ошибки
							u_long error = 0;
							// Выполняем чтение ошибок OpenSSL
							while((error = ERR_get_error()))
								// Выводим в лог сообщение
								this->log->print("SSL: %s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
							// Выполняем закрытие подключения
							this->close(adj->bev.socket);
							// Выходим из функции
							return;
						}
					}
				// Если BIO SSL не создано
				} else {
					// Выполняем закрытие подключения
					this->close(adj->bev.socket);
					// Выводим сообщение об ошибке
					this->log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
					// Выходим из функции
					return;
				}
			}
			// Если функция обратного вызова установлена
			if(wrk->acceptFn != nullptr){
				// Выполняем проверку, разрешено ли клиенту подключиться к серверу
				if(!wrk->acceptFn(ip, mac, it->first, this)){
					// Выполняем закрытие сокета
					this->close(adj->bev.socket);
					// Выводим в лог сообщение
					this->log->print("broken client, host = %s, mac = %s, socket = %d", log_t::flag_t::WARNING, ip.c_str(), mac.c_str(), socket);
					// Выходим из функции
					return;
				}
			}
			// Выполняем блокировку потока
			this->mtx.accept.lock();
			// Добавляем созданного адъютанта в список адъютантов
			auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
			// Добавляем адъютанта в список подключений
			this->adjutants.emplace(ret.first->first, ret.first->second.get());
			// Выполняем блокировку потока
			this->mtx.accept.unlock();
			// Запоминаем IP адрес
			ret.first->second->ip = move(ip);
			// Запоминаем MAC адрес
			ret.first->second->mac = move(mac);
			// Запускаем чтение данных
			this->enabled(method_t::READ, ret.first->first, wrk->wait);
			// Выводим в консоль информацию
			if(!this->noinfo) this->log->print("client connect to server, host = %s, mac = %s, socket = %d", log_t::flag_t::INFO, ret.first->second->ip.c_str(), ret.first->second->mac.c_str(), ret.first->second->bev.socket);
			// Выполняем функцию обратного вызова
			if(wrk->connectFn != nullptr) wrk->connectFn(ret.first->first, wrk->wid, this);
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
		const lock_guard <recursive_mutex> lock(this->mtx.close);
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers){
			// Если в воркере есть подключённые клиенты
			if(!worker.second->adjutants.empty()){
				// Получаем объект воркера
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (worker.second);
				// Переходим по всему списку адъютанта
				for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->locking.count(it->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(it->first);
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Выполняем удаление контекста SSL
						this->ssl.clear(it->second->ssl);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(it->first);
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(it->first, worker.first, this);
						// Удаляем блокировку адъютанта
						this->locking.erase(it->first);
						// Удаляем адъютанта из списка
						it = wrk->adjutants.erase(it);
					// Иначе продолжаем дальше
					} else ++it;
				}
			}
		}
	}
}
/**
 * remove Метод удаления всех воркеров
 */
void awh::server::Core::remove() noexcept {
	// Если список воркеров активен
	if(!this->workers.empty()){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.close);
		// Переходим по всему списку воркеров
		for(auto it = this->workers.begin(); it != this->workers.end();){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если в воркере есть подключённые клиенты
			if(!wrk->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto jt = wrk->adjutants.begin(); jt != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->locking.count(jt->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(jt->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем удаление контекста SSL
						this->ssl.clear(adj->ssl);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(jt->first, it->first, this);
						// Удаляем блокировку адъютанта
						this->locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = wrk->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Останавливаем работу сервера
			wrk->io.stop();
			// Выполняем закрытие сокета
			this->close(wrk->fd);
			// Сбрасываем сокет
			wrk->fd = -1;
			// Выполняем удаление воркера
			it = this->workers.erase(it);
		}
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
			// Получаем объект подключения
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);


			// Структура определяющая тип адреса
			struct sockaddr_in serv_addr;

			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем резолвинг доменного имени
				struct hostent * server = gethostbyname(wrk->host.c_str());
			#else
				// Выполняем резолвинг доменного имени
				struct hostent * server = gethostbyname2(wrk->host.c_str(), AF_INET);
			#endif

			// Заполняем структуру типа адреса нулями
			memset(&serv_addr, 0, sizeof(serv_addr));
			// Устанавливаем что удаленный адрес это ИНТЕРНЕТ
			serv_addr.sin_family = AF_INET;
			// Выполняем копирование данных типа подключения
			memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
			// Получаем IP адрес
			char * ip = inet_ntoa(serv_addr.sin_addr);

			printf("IP address: %s\n", ip);

			// Выполняем запуск системы
			resolver(ip, wrk);

			/*
			// Определяем тип подключения
			switch(this->net.family){
				// Резолвер IPv4, создаём резолвер
				case AF_INET: this->dns4.resolve(wrk, wrk->host, AF_INET, resolver); break;
				// Резолвер IPv6, создаём резолвер
				case AF_INET6: this->dns6.resolve(wrk, wrk->host, AF_INET6, resolver); break;
			}
			*/
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
		const lock_guard <recursive_mutex> lock(this->mtx.close);
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
					if(this->locking.count(jt->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(jt->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (jt->second.get());
						// Выполняем очистку буфера событий
						this->clean(jt->first);
						// Выполняем удаление контекста SSL
						this->ssl.clear(adj->ssl);
						// Выводим функцию обратного вызова
						if(wrk->disconnectFn != nullptr)
							// Выполняем функцию обратного вызова
							wrk->disconnectFn(jt->first, it->first, this);
						// Удаляем адъютанта из списка подключений
						this->adjutants.erase(jt->first);
						// Удаляем блокировку адъютанта
						this->locking.erase(jt->first);
						// Удаляем адъютанта из списка
						jt = wrk->adjutants.erase(jt);
					// Иначе продолжаем дальше
					} else ++jt;
				}
			}
			// Останавливаем работу сервера
			wrk->io.stop();
			// Выполняем закрытие сокета
			this->close(wrk->fd);
			// Сбрасываем сокет
			wrk->fd = -1;
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
	const lock_guard <recursive_mutex> lock(this->mtx.close);
	// Если блокировка адъютанта не установлена
	if(this->locking.count(aid) < 1){
		// Выполняем блокировку адъютанта
		this->locking.emplace(aid);
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
			// Выполняем удаление контекста SSL
			this->ssl.clear(adj->ssl);
			// Удаляем адъютанта из списка адъютантов
			wrk->adjutants.erase(aid);
			// Удаляем адъютанта из списка подключений
			this->adjutants.erase(aid);
			// Выводим сообщение об ошибке
			if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnect client from server");
			// Выводим функцию обратного вызова
			if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this);
		}
		// Удаляем блокировку адъютанта
		this->locking.erase(aid);
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
		// Выводим сообщение в лог, о таймауте подключения
		this->log->print("timeout host [%s]", log_t::flag_t::WARNING, adj->ip.c_str());
		// Останавливаем чтение данных
		this->disabled(method_t::READ, it->first);
		// Останавливаем запись данных
		this->disabled(method_t::WRITE, it->first);
		// Выполняем отключение клиента
		this->close(aid);
	}
}
/**
 * write Функция обратного вызова при записи данных в сокет
 * @param method метод режима работы
 * @param aid    идентификатор адъютанта
 */
void awh::server::Core::transfer(const method_t method, const size_t aid) noexcept {
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
			case (uint8_t) method_t::READ: {
				// Количество полученных байт
				int64_t bytes = -1;
				// Создаём буфер входящих данных
				char buffer[BUFFER_SIZE];
				// Останавливаем таймаут ожидания на чтение из сокета
				adj->bev.timer.read.stop();
				// Выполняем зануление буфера
				memset(buffer, 0, sizeof(buffer));
				// Если защищённый режим работы разрешён
				if(adj->ssl.mode){
					// Выполняем очистку ошибок OpenSSL
					ERR_clear_error();
					// Выполняем чтение из защищённого сокета
					bytes = SSL_read(adj->ssl.ssl, buffer, sizeof(buffer));
				// Выполняем чтение данных из сокета
				} else bytes = recv(adj->bev.socket, buffer, sizeof(buffer), 0);
				// Останавливаем таймаут ожидания на чтение из сокета
				adj->bev.timer.read.stop();
				// Если время ожидания записи данных установлено
				if(adj->timeouts.read > 0)
					// Запускаем ожидание чтения данных с сервера
					adj->bev.timer.read.start(adj->timeouts.read);
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
							if(wrk->readFn != nullptr)
								// Выводим функцию обратного вызова
								wrk->readFn(buffer + offset, actual, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
							// Увеличиваем смещение в буфере
							offset += actual;
						}
					// Если данных достаточно и функция обратного вызова на получение данных установлена
					} else if(wrk->readFn != nullptr)
						// Выводим функцию обратного вызова
						wrk->readFn(buffer, bytes, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
				// Если данные не могут быть прочитаны
				} else {
					// Если произошла ошибка
					if(bytes < 0){
						// Если защищённый режим работы разрешён
						if(adj->ssl.mode){
							// Получаем данные описание ошибки
							if(SSL_get_error(adj->ssl.ssl, bytes) == SSL_ERROR_WANT_READ)
								// Выполняем пропуск попытки
								return;
							// Иначе выводим сообщение об ошибке
							else this->error(bytes, aid);
						// Если защищённый режим работы запрещён
						} else if(errno == EAGAIN) return;
						// Выполняем отключение клиента
						this->close(aid);
					}
					// Если подключение разорвано
					if(bytes == 0)
						// Выполняем отключение клиента
						this->close(aid);
				}
			} break;
			// Если производится запись данных
			case (uint8_t) method_t::WRITE: {
				// Останавливаем запись данных
				this->disabled(method_t::WRITE, it->first);
				// Если функция обратного вызова на запись данных установлена
				if(wrk->writeFn != nullptr){
					// Разрешаем выполнять запись в сокет
					adj->bev.locked.write = false;
					// Выводим функцию обратного вызова
					wrk->writeFn(aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
				}
			} break;
		}
	}
}
/**
 * setBandwidth Метод установки пропускной способности сети
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Core::setBandwidth(const size_t aid, const string & read, const string & write) noexcept {
	// Выполняем извлечение адъютанта
	auto it = this->adjutants.find(aid);
	// Если адъютант получен
	if(it != this->adjutants.end()){
		// Если - это Unix
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (adj->parent);
			// Получаем размер буфера на чтение
			const int rcv = (!read.empty() ? this->fmk->sizeBuffer(read) : 0);
			// Получаем размер буфера на запись
			const int snd = (!write.empty() ? this->fmk->sizeBuffer(write) : 0);
			// Устанавливаем размер буфера
			if(adj->bev.socket > 0) this->socket.bufferSize(adj->bev.socket, rcv, snd, wrk->total);
		// Если - это Windows
		#else
			// Блокируем вывод переменных
			(void) read;
			(void) write;
		#endif
	}
}
/**
 * setIpV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Core::setIpV6only(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.system);
	// Устанавливаем флаг использования только сети IPv6
	this->ipV6only = mode;
}
/**
 * setTotal Метод установки максимального количества одновременных подключений
 * @param wid   идентификатор воркера
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Core::setTotal(const size_t wid, const u_short total) noexcept {
	// Если идентификатор воркера передан
	if(wid > 0){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->mtx.system);
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
			const lock_guard <recursive_mutex> lock(this->mtx.system);
			// Получаем объект подключения
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
			// Если порт передан, устанавливаем
			if(port > 0) wrk->port = port;
			// Если хост передан, устанавливаем
			if(!host.empty()) wrk->host = host;
			// Иначе получаем IP адрес сервера автоматически
			else wrk->host = this->ifnet.ip(this->net.family);
		}
	}
}
/**
 * setCert Метод установки файлов сертификата
 * @param cert  корневой сертификат
 * @param key   приватный ключ сертификата
 * @param chain файл цепочки сертификатов
 */
void awh::server::Core::setCert(const string & cert, const string & key, const string & chain) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.system);
	// Устанавливаем файлы сертификата
	this->ssl.setCert(cert, key, chain);
}
/**
 * Core Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), pid(0), ifnet(fmk, log) {
	// Устанавливаем тип запускаемого ядра
	this->type = type_t::SERVER;
}
/**
 * ~Core Деструктор
 */
awh::server::Core::~Core() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
