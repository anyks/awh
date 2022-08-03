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
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * readJack Функция обратного вызова при чтении данных с сокета
	 * @param watcher объект события чтения
	 * @param revents идентификатор события
	 */
	void awh::server::Core::readJack(ev::io & watcher, int revents) noexcept {
		// Бинарный буфер для получения данных
		char buffer[4096];
		// Заполняем буфер нулями
		memset(buffer, 0, sizeof(buffer));
		// Если процесс является родительским
		if(this->pid == getpid()){
			// Создаём объект сообщения
			mess_t message;
			// Выполняем чтение полученного сообщения
			const int bytes = ::read(watcher.fd, buffer, sizeof(buffer));
			// Если сообщение прочитанно
			if(bytes == sizeof(message)){
				// Выполняем извлечение входящих данных
				memcpy(&message, buffer, bytes);
				// Определяем тип события
				switch((uint8_t) message.event){
					// Если событием является подключение
					case (uint8_t) event_t::CONNECT: {
						// Индекс выбранного процесса
						this->index = message.index;
						// Выполняем установку количества подключений
						this->jacks.at(message.index)->count = message.count;
						// Выполняем перебор всех процессов и ищем тот, где меньше всего нагрузка
						for(auto & jack : this->jacks){
							// Если текущее количество подключений меньше чем передано
							if((jack->count < message.count) || (jack->count == 0)){
								// Запоминаем новый индекс процесса
								this->index = jack->index;
								// Выходим из цикла
								break;
							}
						}
						// Выводим сообщени об активном сервисе
						if(!this->noinfo) this->log->print("%d clients are connected to the process, pid = %d", log_t::flag_t::INFO, message.count, message.pid);
						// Если был выбран новый процесс для обработки запросов
						if(this->index != message.index){
							// Устанавливаем тип сообщения активация выбора
							this->event = event_t::SELECT;
							// Выполняем отправку сообщения дочернему процессу
							this->writeJack(this->jacks.at(this->index)->write, ev::WRITE);
							// Устанавливаем тип сообщения деактивация выбора
							this->event = event_t::UNSELECT;
							// Выполняем отправку сообщения дочернему процессу
							this->writeJack(this->jacks.at(message.index)->write, ev::WRITE);
						}
					} break;
					// Если событием является отключение
					case (uint8_t) event_t::DISCONNECT:
						// Выводим сообщени об активном сервисе
						if(!this->noinfo) this->log->print("client disconnected from process, %d connections left, pid = %d", log_t::flag_t::INFO, message.count, message.pid);
						// Выполняем установку количества подключений
						this->jacks.at(message.index)->count = message.count;
					break;
				}
			}
		// Если процесс является дочерним
		} else {
			// Получаем объект текущего работника
			jack_t * jack = this->jacks.at(this->index).get();
			// Если файловый дескриптор не соответствует родительскому
			if(jack->cfds[0] != watcher.fd){
				// Останавливаем чтение
				watcher.stop();
				// Выходим из функции
				return;
			}
			// Создаём объект сообщения
			mess_t message;
			// Выполняем чтение полученного сообщения
			const int bytes = ::read(watcher.fd, buffer, sizeof(buffer));
			// Если сообщение прочитанно
			if(bytes == sizeof(message)){
				// Выполняем извлечение входящих данных
				memcpy(&message, buffer, bytes);
				// Определяем тип события
				switch((uint8_t) message.event){
					// Если событием является выбор работника
					case (uint8_t) event_t::SELECT:
						// Устанавливаем флаг активации перехвата подключения
						this->interception = true;
						// Выводим сообщени об активном сервисе
						if(!this->noinfo) this->log->print("a process has been activated to intercept connections, pid = %d", log_t::flag_t::INFO, getpid());
					break;
					// Если событием является снятия выбора с работника
					case (uint8_t) event_t::UNSELECT:
						// Снимаем флаг активации перехвата подключения
						this->interception = false;
						// Выводим сообщени об активном сервисе
						if(!this->noinfo) this->log->print("a process has been deactivated to intercept connections, pid = %d", log_t::flag_t::INFO, getpid());
					break;
				}
			}
		}
	}
	/**
	 * writeJack Функция обратного вызова при записи данных в сокет
	 * @param watcher объект события записи
	 * @param revents идентификатор события
	 */
	void awh::server::Core::writeJack(ev::io & watcher, int revents) noexcept {
		// Выполняем остановку проверки сокета на запись
		watcher.stop();
		// Если процесс является родительским
		if(this->pid == getpid()){
			// Создаём объект сообщения
			mess_t message;
			// Устанавливаем идентификатор процесса
			message.pid = getpid();
			// Устанавливаем событие сообщения
			message.event = this->event;
			// Выполняем отправку сообщения
			if(::write(watcher.fd, &message, sizeof(message)) <= 0){
				// Определяем тип события
				switch((uint8_t) this->event){
					// Если событием является выбор работника
					case (uint8_t) event_t::SELECT:
						// Выводим сообщение об ошибке, о невозможности отправкить сообщение
						this->log->print("unable to send message to parent process, for [%s] event", log_t::flag_t::WARNING, "SELECT");
					break;
				}
			}
		// Если процесс является дочерним
		} else {
			// Получаем объект текущего работника
			jack_t * jack = this->jacks.at(this->index).get();
			// Если файловый дескриптор не соответствует родительскому
			if(jack->mfds[1] != watcher.fd){
				// Останавливаем чтение
				watcher.stop();
				// Выходим из функции
				return;
			}
			// Создаём объект сообщения
			mess_t message;
			// Устанавливаем идентификатор процесса
			message.pid = getpid();
			// Устанавливаем событие сообщения
			message.event = this->event;
			// Устанавливаем индекс процесса
			message.index = this->index;
			// Определяем тип события
			switch((uint8_t) this->event){
				// Если событием является подключение
				case (uint8_t) event_t::CONNECT:
				// Если событием является отключение
				case (uint8_t) event_t::DISCONNECT: {
					// Переходим по всем активным воркерам
					for(auto & wrk : this->workers)
						// Устанавливаем количество подключений
						message.count += wrk.second->adjutants.size();
				} break;
			}
			// Выполняем отправку сообщения
			if(::write(watcher.fd, &message, sizeof(message)) <= 0){
				// Определяем тип события
				switch((uint8_t) this->event){
					// Если событием является подключение
					case (uint8_t) event_t::CONNECT:
						// Выводим сообщение об ошибке, о невозможности отправкить сообщение
						this->log->print("unable to send message to parent process, for [%s] event", log_t::flag_t::WARNING, "CONNECT");
					break;
					// Если событием является отключение
					case (uint8_t) event_t::DISCONNECT:
						// Выводим сообщение об ошибке, о невозможности отправкить сообщение
						this->log->print("unable to send message to parent process, for [%s] event", log_t::flag_t::WARNING, "DISCONNECT");
					break;
				}
			}
		}
	}
	/**
	 * signal Функция обратного вызова при возникновении сигнала
	 * @param watcher объект события сигнала
	 * @param revents идентификатор события
	 */
	void awh::server::Core::signal(ev::sig & watcher, int revents) noexcept {
		// Останавливаем сигнал
		watcher.stop();
		// Если процесс является родительским
		if(this->pid == getpid()){
			// Выполняем остановку работы
			this->stop();
			// Завершаем работу основного процесса
			exit(watcher.signum);
		// Если процесс является дочерним
		} else {
			// Определяем тип сигнала
			switch(watcher.signum){
				// Если возникает сигнал ручной остановкой процесса
				case SIGINT:
					// Выводим сообщение об завершении работы процесса
					this->log->print("child process was closed, goodbye!", log_t::flag_t::INFO);
				break;
				// Если возникает сигнал ошибки выполнения арифметической операции
				case SIGFPE:
					// Выводим сообщение об завершении работы процесса
					this->log->print("child process was closed with signal [%s]", log_t::flag_t::WARNING, "SIGFPE");
				break;
				// Если возникает сигнал выполнения неверной инструкции
				case SIGILL:
					// Выводим сообщение об завершении работы процесса
					this->log->print("child process was closed with signal [%s]", log_t::flag_t::WARNING, "SIGILL");
				break;
				// Если возникает сигнал запроса принудительного завершения процесса
				case SIGTERM:
					// Выводим сообщение об завершении работы процесса
					this->log->print("child process was closed with signal [%s]", log_t::flag_t::WARNING, "SIGTERM");
				break;
				// Если возникает сигнал сегментации памяти (обращение к несуществующему адресу памяти)
				case SIGSEGV:
					// Выводим сообщение об завершении работы процесса
					this->log->print("child process was closed with signal [%s]", log_t::flag_t::WARNING, "SIGSEGV");
				break;
				// Если возникает сигнал запроса принудительное закрытие приложения из кода программы
				case SIGABRT:
					// Выводим сообщение об завершении работы процесса
					this->log->print("child process was closed with signal [%s]", log_t::flag_t::WARNING, "SIGABRT");
				break;
			}
			// Выполняем остановку работы
			this->stop();
			// Завершаем работу дочернего процесса
			exit(watcher.signum);
		}
	}
	/**
	 * children Функция обратного вызова при завершении работы процесса
	 * @param watcher объект события дочернего процесса
	 * @param revents идентификатор события
	 */
	void awh::server::Core::children(ev::child & watcher, int revents) noexcept {
		// Останавливаем сигнал
		watcher.stop();
		// Индекс завершившегося процесса и идентификатор основного воркера
		size_t index = 0, wid = 0;
		// Выполняем поиск завершившегося процесса
		for(auto & jack : this->jacks){	
			// Если процесс найден
			if(jack->pid == watcher.rpid){
				// Запоминаем идентификатор основного воркера
				wid = jack->wid;
				// Запоминаем индекс завершившегося процесса
				index = jack->index;
				// Останавливаем чтение
				jack->read.stop();
				// Останавливаем запись
				jack->write.stop();
				// Выполняем закрытие файловых дескрипторов
				::close(jack->mfds[0]);
				::close(jack->cfds[1]);
				// Выводим сообщение об ошибке, о невозможности отправкить сообщение
				this->log->print("child process stopped, index = %d, pid = %d, status = %x", log_t::flag_t::CRITICAL, jack->index, jack->pid, watcher.rstatus);
				// Если статус сигнала, ручной остановкой процесса
				if(watcher.rstatus == SIGINT)
					// Выходим из приложения
					exit(SIGINT);
				// Если время жизни процесса составляет меньше 3-х минут
				else if((this->fmk->unixTimestamp() - jack->date) <= 180000)
					// Выходим из приложения
					exit(EXIT_FAILURE);
				// Выходим из цикла
				break;
			}
		}
		// Если воркер получен
		if(wid > 0){
			// Выполняем поиск воркера
			auto it = this->workers.find(wid);
			// Если воркер найден, устанавливаем максимальное количество одновременных подключений
			if(it != this->workers.end()){
				// Если был завершён активный процесс
				if(this->index == index){
					// Выполняем получение количества подключений
					size_t count = 0;
					// Выполняем перебор всех процессов и ищем тот, где меньше всего нагрузка
					for(auto & jack : this->jacks){
						// Если текущее количество подключений меньше чем передано
						if((index != jack->index) && ((jack->count < count) || (count == 0) || (jack->count == 0))){
							// Запоминаем новый индекс процесса
							this->index = jack->index;
							// Если получение количества подключений пустое, запоминаем его
							if(count == 0) count = jack->count;
						}
					}
					// Устанавливаем тип сообщения активация выбора
					this->event = event_t::SELECT;
					// Выполняем отправку сообщения дочернему процессу
					this->writeJack(this->jacks.at(this->index)->write, ev::WRITE);
				}
				// Создаём объект работника
				unique_ptr <jack_t> jack(new jack_t);
				// Устанавливаем идентификатор воркера
				jack->wid = wid;
				// Устанавливаем индекс работника
				jack->index = index;
				// Выполняем подписку на основной канал передачи данных
				if(pipe(jack->mfds) != 0){
					// Выводим в лог сообщение
					this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Выходим принудительно из приложения
					exit(EXIT_FAILURE);
				}
				// Выполняем подписку на дочерний канал передачи данных
				if(pipe(jack->cfds) != 0){
					// Выводим в лог сообщение
					this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Выходим принудительно из приложения
					exit(EXIT_FAILURE);
				}
				// Устанавливаем дочерний процесс
				this->jacks.at(jack->index) = move(jack);
				// Замораживаем поток на период в 5 секунд
				this_thread::sleep_for(5s);
				// Выполняем создание нового процесса
				this->forking(wid, index, true);
			}
		}
	}
#endif
/**
 * forking Метод разъяснения (создание дочерних процессов)
 * @param wid   wid идентификатор воркера
 * @param index индекс инициализированного процесса
 * @param stop  флаг остановки итерации создания дочерних процессов
 */
void awh::server::Core::forking(const size_t wid, const size_t index, const size_t stop) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)		
		// Если не все форки созданы
		if(index < this->forks){
			// Выполняем поиск воркера
			auto it = this->workers.find(wid);
			// Если воркер найден, устанавливаем максимальное количество одновременных подключений
			if(it != this->workers.end()){
				// Флаг первичной инициализации
				bool initialization = this->jacks.empty();
				// Получаем объект подключения
				worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (it->second);
				// Если список работников ещё пустой
				if(initialization){
					// Выполняем создание указанное количество работников
					for(size_t i = 0; i < this->forks; i++){
						// Создаём объект работника
						unique_ptr <jack_t> jack(new jack_t);
						// Устанавливаем идентификатор основного воркера
						jack->wid = wid;
						// Устанавливаем индекс работника
						jack->index = i;
						// Выполняем подписку на основной канал передачи данных
						if(pipe(jack->mfds) != 0){
							// Выводим в лог сообщение
							this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выходим принудительно из приложения
							exit(EXIT_FAILURE);
						}
						// Выполняем подписку на дочерний канал передачи данных
						if(pipe(jack->cfds) != 0){
							// Выводим в лог сообщение
							this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выходим принудительно из приложения
							exit(EXIT_FAILURE);
						}
						// Выполняем добавление работника в список работников
						this->jacks.push_back(move(jack));
					}
				}
				// Устанавливаем идентификатор процесса
				pid_t pid = -1;
				// Определяем тип потока
				switch((pid = fork())){
					// Если поток не создан
					case -1: {
						// Выводим в лог сообщение
						this->log->print("child process could not be created", log_t::flag_t::CRITICAL);
						// Выходим принудительно из приложения
						exit(EXIT_FAILURE);
					} break;
					// Если - это дочерний поток значит все нормально
					case 0: {
						// Запоминаем текущий индекс процесса
						this->index = index;
						{
							// Получаем объект текущего работника
							jack_t * jack = this->jacks.at(index).get();
							// Закрываем файловый дескриптор на запись в дочерний процесс
							::close(jack->cfds[1]);
							// Закрываем файловый дескриптор на чтение из основного процесса
							::close(jack->mfds[0]);
							// Устанавливаем идентификатор процесса
							jack->pid = getpid();
							// Устанавливаем время начала жизни процесса
							jack->date = this->fmk->unixTimestamp();
							// Устанавливаем базу событий для чтения
							jack->read.set(this->dispatch.base);
							// Устанавливаем базу событий для записи
							jack->write.set(this->dispatch.base);
							// Устанавливаем событие на чтение данных от основного процесса
							jack->read.set <core_t, &core_t::readJack> (this);
							// Устанавливаем событие на запись данных основному процессу
							jack->write.set <core_t, &core_t::writeJack> (this);
							// Устанавливаем сокет для чтения
							jack->read.set(jack->cfds[0], ev::READ);
							// Устанавливаем сокет для записи
							jack->write.set(jack->mfds[1], ev::WRITE);
							// Запускаем чтение данных с основного процесса
							jack->read.start();
							// Запускаем запись данных основному процессу
							// jack->write.start();
						}{
							// Определяем тип сокета
							switch((uint8_t) this->net.sonet){
								// Если тип сокета установлен как UDP
								case (uint8_t) sonet_t::UDP:
									// Выполняем активацию сервера
									this->accept(1, wid);
								break;
								// Если тип сокета установлен как TCP/IP
								case (uint8_t) sonet_t::TCP: {
									// Устанавливаем базу событий
									wrk->io.set(this->dispatch.base);
									// Устанавливаем событие на чтение данных подключения
									wrk->io.set <worker_t, &worker_t::accept> (wrk);
									// Устанавливаем сокет для чтения
									wrk->io.set(wrk->addr.fd, ev::READ);
									// Запускаем чтение данных с клиента
									wrk->io.start();
								} break;
							}
						}
						// Выполняем активацию базы событий
						ev_loop_fork(this->dispatch.base);
					} break;
					// Если - это родительский процесс
					default: {
						// Получаем объект текущего работника
						jack_t * jack = this->jacks.at(index).get();
						// Закрываем файловый дескриптор на запись в основной процесс
						::close(jack->mfds[1]);
						// Закрываем файловый дескриптор на чтение из дочернего процесса
						::close(jack->cfds[0]);
						// Устанавливаем PID процесса
						jack->pid = pid;
						// Устанавливаем время начала жизни процесса
						jack->date = this->fmk->unixTimestamp();
						// Устанавливаем базу событий для чтения
						jack->read.set(this->dispatch.base);
						// Устанавливаем базу событий для записи
						jack->write.set(this->dispatch.base);
						// Устанавливаем событие на чтение данных от дочернего процесса
						jack->read.set <core_t, &core_t::readJack> (this);
						// Устанавливаем событие на запись данных дочернему процессу
						jack->write.set <core_t, &core_t::writeJack> (this);
						// Устанавливаем сокет для чтения
						jack->read.set(jack->mfds[0], ev::READ);
						// Устанавливаем сокет для записи
						jack->write.set(jack->cfds[1], ev::WRITE);
						// Запускаем чтение данных с дочернего процесса
						jack->read.start();
						// Если это первый воркер, активируем его
						if(initialization){
							{
								// Выполняем игнорирование сигналов SIGPIPE и SIGABRT
								::signal(SIGPIPE, SIG_IGN);
								::signal(SIGABRT, SIG_IGN);
								// Устанавливаем базу событий для сигналов
								this->sig.sint.set(this->dispatch.base);
								this->sig.sfpe.set(this->dispatch.base);
								this->sig.sill.set(this->dispatch.base);
								this->sig.sterm.set(this->dispatch.base);
								this->sig.sabrt.set(this->dispatch.base);
								this->sig.ssegv.set(this->dispatch.base);
								// Устанавливаем событие на отслеживание сигнала
								this->sig.sint.set <core_t, &core_t::signal> (this);
								this->sig.sfpe.set <core_t, &core_t::signal> (this);
								this->sig.sill.set <core_t, &core_t::signal> (this);
								this->sig.sterm.set <core_t, &core_t::signal> (this);
								this->sig.sabrt.set <core_t, &core_t::signal> (this);
								this->sig.ssegv.set <core_t, &core_t::signal> (this);
								// Выполняем отслеживание возникающего сигнала
								this->sig.sint.start(SIGINT);
								this->sig.sfpe.start(SIGFPE);
								this->sig.sill.start(SIGILL);
								this->sig.sterm.start(SIGTERM);
								this->sig.sabrt.start(SIGABRT);
								this->sig.ssegv.start(SIGSEGV);
							}{
								// Устанавливаем тип сообщения
								this->event = event_t::SELECT;
								// Запускаем запись данных дочернему процессу
								jack->write.start();
							}
						}
						// Устанавливаем базу событий
						jack->cw.set(this->dispatch.base);
						// Устанавливаем событие на выход дочернего процесса
						jack->cw.set <core_t, &core_t::children> (this);
						// Выполняем отслеживание статуса дочернего процесса
						jack->cw.start(pid);
						// Продолжаем дальше
						if(!stop) this->forking(wid, index + 1, stop);
					}
				}
			}
		}
	#endif
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
		// Определяем тип сокета
		switch((uint8_t) core->net.sonet){
			// Если тип сокета установлен как UDP
			case (uint8_t) sonet_t::UDP: {
				// Если разрешено выводить информационные сообщения
				if(!core->noinfo){
					// Если unix-сокет используется
					if(core->net.family == family_t::NIX)
						// Выводим информацию о запущенном сервере на unix-сокете
						core->log->print("run server [%s]", log_t::flag_t::INFO, core->net.filename.c_str());
					// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
					else core->log->print("run server [%s:%u]", log_t::flag_t::INFO, wrk->host.c_str(), wrk->port);
				}
				// Получаем тип операционной системы
				const fmk_t::os_t os = core->fmk->os();
				// Если операционная система является Windows или количество процессов всего один
				if((this->interception = ((os == fmk_t::os_t::WIND32) || (os == fmk_t::os_t::WIND64) || (this->forks == 1))))
					// Выполняем активацию сервера
					core->accept(1, wrk->wid);
				// Выполняем создание дочерних процессов
				else this->forking(wrk->wid);
				// Выходим из функции
				return;
			} break;
			// Если тип сокета установлен как TCP/IP
			case (uint8_t) sonet_t::TCP: {
				// Определяем тип протокола подключения
				switch((uint8_t) core->net.family){
					// Если тип протокола подключения IPv4
					case (uint8_t) family_t::IPV4:
						// Устанавливаем сеть, для выхода в интернет
						wrk->addr.network.assign(
							core->net.v4.first.begin(),
							core->net.v4.first.end()
						);
					break;
					// Если тип протокола подключения IPv6
					case (uint8_t) family_t::IPV6: {
						// Устанавливаем использование только IPv6
						wrk->addr.v6only = core->ipV6only;
						// Устанавливаем сеть, для выхода в интернет
						wrk->addr.network.assign(
							core->net.v6.first.begin(),
							core->net.v6.first.end()
						);
					} break;
				}
				// Определяем тип сокета
				switch((uint8_t) core->net.sonet){
					// Если тип сокета UDP
					case (uint8_t) sonet_t::UDP: {
						// Устанавливаем тип сокета
						wrk->addr.type = SOCK_DGRAM;
						// Устанавливаем тип протокола
						wrk->addr.protocol = IPPROTO_UDP;
					} break;
					// Если тип сокета TCP
					case (uint8_t) sonet_t::TCP: {
						// Устанавливаем тип сокета
						wrk->addr.type = SOCK_STREAM;
						// Устанавливаем тип протокола
						wrk->addr.protocol = IPPROTO_TCP;
					} break;
				}
				// Если unix-сокет используется
				if(core->net.family == family_t::NIX)
					// Выполняем инициализацию сокета
					wrk->addr.init(core->net.filename, engine_t::type_t::SERVER);
				// Если unix-сокет не используется, выполняем инициализацию сокета
				else wrk->addr.init(wrk->host, wrk->port, (core->net.family == family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER);
				// Если сокет подключения получен
				if(wrk->addr.fd > -1){
					// Если повесить прослушку на порт не вышло
					if(!wrk->addr.list()) break;
					// Если разрешено выводить информационные сообщения
					if(!core->noinfo){
						// Если unix-сокет используется
						if(core->net.family == family_t::NIX)
							// Выводим информацию о запущенном сервере на unix-сокете
							core->log->print("run server [%s]", log_t::flag_t::INFO, core->net.filename.c_str());
						// Если unix-сокет не используется, выводим сообщение о запущенном сервере за порту
						else core->log->print("run server [%s:%u]", log_t::flag_t::INFO, wrk->host.c_str(), wrk->port);
					}
					// Получаем тип операционной системы
					const fmk_t::os_t os = core->fmk->os();
					// Если операционная система является Windows или количество процессов всего один
					if((this->interception = ((os == fmk_t::os_t::WIND32) || (os == fmk_t::os_t::WIND64) || (this->forks == 1)))){
						// Устанавливаем базу событий
						wrk->io.set(this->dispatch.base);
						// Устанавливаем событие на чтение данных подключения
						wrk->io.set <worker_t, &worker_t::accept> (wrk);
						// Устанавливаем сокет для чтения
						wrk->io.set(wrk->addr.fd, ev::READ);
						// Запускаем чтение данных с клиента
						wrk->io.start();
					// Выполняем создание дочерних процессов
					} else this->forking(wrk->wid);
					// Выходим из функции
					return;
				// Если сокет не создан, выводим в консоль информацию
				} else {
					// Если unix-сокет используется
					if(core->net.family == family_t::NIX)
						// Выводим информацию об незапущенном сервере на unix-сокете
						core->log->print("server cannot be started [%s]", log_t::flag_t::CRITICAL, core->net.filename.c_str());
					// Если unix-сокет не используется, выводим сообщение об незапущенном сервере за порту
					else core->log->print("server cannot be started [%s:%u]", log_t::flag_t::CRITICAL, wrk->host.c_str(), wrk->port);
				}
			} break;
		}
	// Если IP адрес сервера не получен, выводим в консоль информацию
	} else core->log->print("broken host server %s", log_t::flag_t::CRITICAL, wrk->host.c_str());
	// Останавливаем работу сервера
	core->stop();
}
/**
 * accept Функция подключения к серверу
 * @param fd  файловый дескриптор (сокет) подключившегося клиента
 * @param wid идентификатор воркера
 */
void awh::server::Core::accept(const int fd, const size_t wid) noexcept {
	// Если идентификатор воркера передан
	if((wid > 0) && (fd >= 0) && this->interception){
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
						case (uint8_t) family_t::IPV6: {
							// Устанавливаем использование только IPv6
							adj->addr.v6only = this->ipV6only;
							// Устанавливаем сеть, для выхода в интернет
							adj->addr.network.assign(
								this->net.v6.first.begin(),
								this->net.v6.first.end()
							);
						} break;
					}
					// Определяем тип сокета
					switch((uint8_t) this->net.sonet){
						// Если тип сокета UDP
						case (uint8_t) sonet_t::UDP: {
							// Устанавливаем тип сокета
							adj->addr.type = SOCK_DGRAM;
							// Устанавливаем тип протокола
							adj->addr.protocol = IPPROTO_UDP;
						} break;
						// Если тип сокета TCP
						case (uint8_t) sonet_t::TCP: {
							// Устанавливаем тип сокета
							adj->addr.type = SOCK_STREAM;
							// Устанавливаем тип протокола
							adj->addr.protocol = IPPROTO_TCP;
						} break;
					}
					// Если unix-сокет используется
					if(this->net.family == family_t::NIX)
						// Выполняем инициализацию сокета
						adj->addr.init(this->net.filename, engine_t::type_t::SERVER);
					// Если unix-сокет не используется, выполняем инициализацию сокета
					else adj->addr.init(wrk->host, wrk->port, (this->net.family == family_t::IPV6 ? AF_INET6 : AF_INET), engine_t::type_t::SERVER);
					// Выполняем разрешение подключения
					if(adj->addr.accept(adj->addr.fd, 0)){
						// Получаем адрес подключения клиента
						adj->ip = adj->addr.ip;
						// Получаем аппаратный адрес клиента
						adj->mac = adj->addr.mac;
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->unixTimestamp();
						// Выполняем получение контекста сертификата
						this->engine.wrap(adj->engine, &adj->addr, this->net.family != family_t::NIX);
						// Если подключение не обёрнуто
						if(adj->addr.fd < 0){
							// Выводим сообщение об ошибке
							this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
							// Выходим из функции
							return;
						}
						// Выполняем блокировку потока
						this->mtx.accept.lock();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->mtx.accept.unlock();
						// Если процесс не является основным
						if((this->pid != getpid()) && !this->jacks.empty()){
							// Устанавливаем активное событие подключения
							this->event = event_t::CONNECT;
							// Выполняем разрешение на отправку сообщения
							this->jacks.at(this->index)->write.start();
						}
						// Запускаем чтение данных
						this->enabled(method_t::READ, ret.first->first);
						// Выполняем функцию обратного вызова
						if(wrk->connectFn != nullptr) wrk->connectFn(ret.first->first, wrk->wid, this);
						// Выходим из функции
						return;
					// Подключение не установлено, выводим сообщение об ошибке
					} else this->log->print("accepting for client is broken", log_t::flag_t::CRITICAL);
				} break;
				// Если тип сокета установлен как TCP/IP
				case (uint8_t) sonet_t::TCP: {
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
					// Определяем тип сокета
					switch((uint8_t) this->net.sonet){
						// Если тип сокета UDP
						case (uint8_t) sonet_t::UDP: {
							// Устанавливаем тип сокета
							adj->addr.type = SOCK_DGRAM;
							// Устанавливаем тип протокола
							adj->addr.protocol = IPPROTO_UDP;
						} break;
						// Если тип сокета TCP
						case (uint8_t) sonet_t::TCP: {
							// Устанавливаем тип сокета
							adj->addr.type = SOCK_STREAM;
							// Устанавливаем тип протокола
							adj->addr.protocol = IPPROTO_TCP;
						} break;
					}
					// Выполняем разрешение подключения
					if(adj->addr.accept(wrk->addr)){
						// Получаем адрес подключения клиента
						adj->ip = adj->addr.ip;
						// Получаем аппаратный адрес клиента
						adj->mac = adj->addr.mac;
						// Устанавливаем идентификатор адъютанта
						adj->aid = this->fmk->unixTimestamp();
						// Выполняем получение контекста сертификата
						this->engine.wrap(adj->engine, &adj->addr, this->net.family != family_t::NIX);
						// Если подключение не обёрнуто
						if(adj->addr.fd < 0){
							// Выводим сообщение об ошибке
							this->log->print("wrap engine context is failed", log_t::flag_t::CRITICAL);
							// Выходим
							break;
						}
						// Выполняем блокировку потока
						this->mtx.accept.lock();
						// Добавляем созданного адъютанта в список адъютантов
						auto ret = wrk->adjutants.emplace(adj->aid, move(adj));
						// Добавляем адъютанта в список подключений
						this->adjutants.emplace(ret.first->first, ret.first->second.get());
						// Выполняем блокировку потока
						this->mtx.accept.unlock();
						// Если процесс не является основным
						if((this->pid != getpid()) && !this->jacks.empty()){
							// Устанавливаем активное событие подключения
							this->event = event_t::CONNECT;
							// Выполняем разрешение на отправку сообщения
							this->jacks.at(this->index)->write.start();
						}
						// Запускаем чтение данных
						this->enabled(method_t::READ, ret.first->first);
						// Если вывод информационных данных не запрещён
						if(!this->noinfo)
							// Выводим в консоль информацию
							this->log->print(
								"client connect to server, host = %s, mac = %s, socket = %d, pid = %d",
								log_t::flag_t::INFO,
								ret.first->second->ip.c_str(),
								ret.first->second->mac.c_str(),
								ret.first->second->addr.fd, getpid()
							);
						// Выполняем функцию обратного вызова
						if(wrk->connectFn != nullptr) wrk->connectFn(ret.first->first, wrk->wid, this);
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
		const lock_guard <recursive_mutex> lock(this->mtx.close);
		// Переходим по всему списку воркеров
		for(auto & worker : this->workers){
			// Получаем объект воркера
			worker_t * wrk = (worker_t *) const_cast <awh::worker_t *> (worker.second);
			// Если в воркере есть подключённые клиенты
			if(!wrk->adjutants.empty()){
				// Переходим по всему списку адъютанта
				for(auto it = wrk->adjutants.begin(); it != wrk->adjutants.end();){
					// Если блокировка адъютанта не установлена
					if(this->locking.count(it->first) < 1){
						// Выполняем блокировку адъютанта
						this->locking.emplace(it->first);
						// Получаем объект адъютанта
						awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second.get());
						// Выполняем очистку буфера событий
						this->clean(it->first);
						// Выполняем очистку контекста двигателя
						adj->engine.clear();
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
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Если процесс не является основным
					if((this->pid != getpid()) && !this->jacks.empty()){
						// Устанавливаем активное событие отключения
						this->event = event_t::DISCONNECT;
						// Выполняем разрешение на отправку сообщения
						this->jacks.at(this->index)->write.start();
					}
				#endif
			}
			// Останавливаем работу сервера
			wrk->io.stop();
			// Выполняем закрытие подключение сервера
			wrk->addr.close();
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
						// Выполняем очистку контекста двигателя
						adj->engine.clear();
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
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Если процесс не является основным
					if((this->pid != getpid()) && !this->jacks.empty()){
						// Устанавливаем активное событие отключения
						this->event = event_t::DISCONNECT;
						// Выполняем разрешение на отправку сообщения
						this->jacks.at(this->index)->write.start();
					}
				#endif
			}
			// Останавливаем работу сервера
			wrk->io.stop();
			// Выполняем закрытие подключение сервера
			wrk->addr.close();
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
			}

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
						// Выполняем очистку контекста двигателя
						adj->engine.clear();
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
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Если процесс не является основным
					if((this->pid != getpid()) && !this->jacks.empty()){
						// Устанавливаем активное событие отключения
						this->event = event_t::DISCONNECT;
						// Выполняем разрешение на отправку сообщения
						this->jacks.at(this->index)->write.start();
					}
				#endif
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
	const lock_guard <recursive_mutex> lock(this->mtx.close);
	// Если тип сокета установлен как TCP/IP, останавливаем чтение
	if(this->net.sonet == sonet_t::TCP){
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
				// Выполняем очистку контекста двигателя
				adj->engine.clear();
				// Удаляем адъютанта из списка адъютантов
				wrk->adjutants.erase(aid);
				// Удаляем адъютанта из списка подключений
				this->adjutants.erase(aid);
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Если процесс не является основным
					if((this->pid != getpid()) && !this->jacks.empty()){
						// Устанавливаем активное событие отключения
						this->event = event_t::DISCONNECT;
						// Выполняем разрешение на отправку сообщения
						this->jacks.at(this->index)->write.start();
					}
				#endif
				// Выводим сообщение об ошибке
				if(!core->noinfo) this->log->print("%s", log_t::flag_t::INFO, "disconnect client from server");
				// Выводим функцию обратного вызова
				if(wrk->disconnectFn != nullptr) wrk->disconnectFn(aid, wrk->wid, this);
			}
			// Удаляем блокировку адъютанта
			this->locking.erase(aid);
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
				// Если тип сокета установлен как UDP, останавливаем чтение
				if(this->net.sonet == sonet_t::UDP)
					// Останавливаем чтение данных с клиента
					adj->bev.event.read.stop();
				// Выполняем перебор бесконечным циклом пока это разрешено
				while(!adj->bev.locked.read){
					// Выполняем получение сообщения от клиента
					bytes = adj->engine.read(buffer, sizeof(buffer));
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
						if((bytes != 0) && (this->net.sonet == sonet_t::TCP))
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
						// Если нужно повторить попытку
						if(bytes == -2) continue;
						// Если нужно выйти из цикла
						else if(bytes == -1) break;
						// Если нужно завершить работу
						else if(bytes == 0) {
							// Выполняем отключение клиента
							this->close(aid);
							// Выходим из цикла
							break;
						}
					}
					// Выходим из цикла
					break;
				}
			} break;
			// Если производится запись данных
			case (uint8_t) method_t::WRITE: {
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
						bytes = adj->engine.write(adj->buffer.data() + offset, actual);
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
							// Выходим из цикла
							break;
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
					if(adj->buffer.empty()) this->disabled(method_t::WRITE, aid);
					// Если функция обратного вызова на запись данных установлена
					if(wrk->writeFn != nullptr)
						// Выводим функцию обратного вызова
						wrk->writeFn(buffer.data(), buffer.size(), aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
				// Если данных недостаточно для записи в сокет
				} else {
					// Останавливаем запись данных
					this->disabled(method_t::WRITE, aid);
					// Если функция обратного вызова на запись данных установлена
					if(wrk->writeFn != nullptr)
						// Выводим функцию обратного вызова
						wrk->writeFn(nullptr, 0, aid, wrk->wid, reinterpret_cast <awh::core_t *> (this));
				}
				// Если тип сокета установлен как UDP, и данных для записи больше нет, запускаем чтение
				if(adj->buffer.empty() && (this->net.sonet == sonet_t::UDP))
					// Запускаем чтение данных с клиента
					adj->bev.event.read.start();
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
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Получаем объект адъютанта
			awh::worker_t::adj_t * adj = const_cast <awh::worker_t::adj_t *> (it->second);
			// Устанавливаем размер буфера
			adj->engine.buffer(
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
 * setForks Метод установки количества процессов
 * @param forks количество рабочих процессов
 */
void awh::server::Core::setForks(const size_t forks) noexcept {
	// Если количество процессов не передано
	if(forks == 0)
		// Устанавливаем максимальное количество
		this->forks = std::thread::hardware_concurrency();
	// Иначе устанавливаем указанное количество процессов
	else this->forks = forks;
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
 * setCert Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::server::Core::setCert(const string & chain, const string & key) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->mtx.system);
	// Устанавливаем файлы сертификата
	this->engine.setCertificate(chain, key);
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
awh::server::Core::Core(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), pid(-1), index(0), event(event_t::NONE), interception(false), forks(1), ipV6only(false) {
	// Устанавливаем идентификатор процесса
	this->pid = getpid();
	// Устанавливаем тип запускаемого ядра
	this->type = engine_t::type_t::SERVER;
}
/**
 * ~Core Деструктор
 */
awh::server::Core::~Core() noexcept {
	// Выполняем остановку сервера
	this->stop();
}
