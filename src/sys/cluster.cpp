/**
 * @file: server.cpp
 * @date: 2022-08-08
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <sys/cluster.hpp>

/**
 * message Функция обратного вызова получении сообщений
 * @param watcher объект события чтения
 * @param revents идентификатор события
 */
void awh::Cluster::Worker::message(ev::io & watcher, int revents) noexcept {
	// Бинарный буфер для получения данных
	char buffer[4096];
	// Заполняем буфер нулями
	memset(buffer, 0, sizeof(buffer));
	// Если процесс является родительским
	if(this->cluster->_pid == getpid()){
		// Создаём объект сообщения
		mess_t message;
		// Выполняем зануление буфера данных полезной нагрузки
		memset(message.payload, 0, sizeof(message.payload));
		// Выполняем чтение полученного сообщения
		const int bytes = ::read(watcher.fd, buffer, sizeof(buffer));
		// Если данные прочитаны правильно
		if(bytes > 0){
			// Выполняем извлечение входящих данных
			memcpy(&message, buffer, bytes);
			// Если функция обратного вызова установлена, выводим её
			if(this->cluster->_callback.message != nullptr)
				// Выводим функцию обратного вызова
				this->cluster->_callback.message(this->wid, message.pid, (const char *) message.payload, sizeof(message.payload));
		// Если данные не прочитаны
		} else this->cluster->_log->print("data from child process could not be received", log_t::flag_t::CRITICAL);
	// Если процесс является дочерним
	} else if(this->cluster->_pid == (pid_t) getppid()) {
		// Выполняем поиск текущего работника
		auto jt = this->cluster->_jacks.find(this->wid);
		// Если текущий работник найден
		if(jt != this->cluster->_jacks.end()){
			// Получаем индекс текущего процесса
			const uint16_t index = this->cluster->_pids.at(getpid());
			// Получаем объект текущего работника
			jack_t * jack = jt->second.at(index).get();
			// Если файловый дескриптор не соответствует родительскому
			if(jack->cfds[0] != watcher.fd){
				// Останавливаем чтение
				watcher.stop();
				// Переходим по всему списку работников
				for(auto & item : jt->second){
					// Если работник не является текущим работником
					if((jack->cfds[0] != item->cfds[0]) && (jack->mfds[1] != item->mfds[1])){
						// Останавливаем чтение
						item->mess.stop();
						// Закрываем файловый дескриптор на чтение из дочернего процесса
						::close(item->cfds[0]);
						// Закрываем файловый дескриптор на запись в основной процесс
						::close(item->mfds[1]);
					}
				}
				// Выходим из функции
				return;
			}
			// Создаём объект сообщения
			mess_t message;
			// Выполняем зануление буфера данных полезной нагрузки
			memset(message.payload, 0, sizeof(message.payload));
			// Выполняем чтение полученного сообщения
			const int bytes = ::read(watcher.fd, buffer, sizeof(buffer));
			// Если данные прочитаны правильно
			if(bytes > 0){
				// Выполняем извлечение входящих данных
				memcpy(&message, buffer, bytes);
				// Если нужно завершить работу процесса
				if(message.stop){
					// Останавливаем чтение данных с родительского процесса
					this->cluster->stop(this->wid);
					/**
					 * Если операционной системой не является Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						// Выходим из приложения
						exit(SIGCHLD);
					/**
					 * Если операционной системой является Windows
					 */
					#else
						// Выходим из приложения
						exit(SIGINT);
					#endif
				// Если функция обратного вызова установлена, выводим её
				} else if(this->cluster->_callback.message != nullptr)
					// Выводим функцию обратного вызова
					this->cluster->_callback.message(this->wid, message.pid, (const char *) message.payload, sizeof(message.payload));
			// Если данные не прочитаны
			} else this->cluster->_log->print("data from main process could not be received", log_t::flag_t::CRITICAL);
		}
	// Если процесс превратился в зомби
	} else {
		// Процесс превратился в зомби, самоликвидируем его
		this->cluster->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * child Функция обратного вызова при завершении работы процесса
	 * @param watcher объект события дочернего процесса
	 * @param revents идентификатор события
	 */
	void awh::Cluster::Worker::child(ev::child & watcher, int revents) noexcept {
		// Останавливаем сигнал
		watcher.stop();
		// Выполняем поиск работника
		auto jt = this->cluster->_jacks.find(this->wid);
		// Если работник найден
		if(jt != this->cluster->_jacks.end()){
			// Выполняем поиск завершившегося процесса
			for(auto & jack : jt->second){
				// Если процесс найден
				if(jack->pid == watcher.rpid){
					// Останавливаем чтение
					jack->mess.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(jack->mfds[0]);
					::close(jack->cfds[1]);
					// Выводим сообщение об ошибке, о невозможности отправкить сообщение
					this->cluster->_log->print("child process stopped, pid = %d, status = %x", log_t::flag_t::CRITICAL, jack->pid, watcher.rstatus);
					// Если был завершён активный процесс и функция обратного вызова установлена
					if(this->cluster->_callback.process != nullptr)
						// Выводим функцию обратного вызова
						this->cluster->_callback.process(jt->first, watcher.rpid, event_t::STOP);
					// Если статус сигнала, ручной остановкой процесса
					if(watcher.rstatus == SIGINT)
						// Выходим из приложения
						exit(SIGINT);
					// Если время жизни процесса составляет меньше 3-х минут
					else if((this->cluster->_fmk->unixTimestamp() - jack->date) <= 180000)
						// Выходим из приложения
						exit(EXIT_FAILURE);
					// Выходим из цикла
					break;
				}
			}
			// Выполняем поиск воркера
			auto it = this->cluster->_workers.find(jt->first);
			// Если запрашиваемый воркер найден и флаг автоматического перезапуска активен
			if((it != this->cluster->_workers.end()) && it->second.restart){
				// Создаём объект работника
				unique_ptr <jack_t> jack(new jack_t);
				// Выполняем подписку на основной канал передачи данных
				if(pipe(jack->mfds) != 0){
					// Выводим в лог сообщение
					this->cluster->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Выходим принудительно из приложения
					exit(EXIT_FAILURE);
				}
				// Выполняем подписку на дочерний канал передачи данных
				if(pipe(jack->cfds) != 0){
					// Выводим в лог сообщение
					this->cluster->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Выходим принудительно из приложения
					exit(EXIT_FAILURE);
				}
				// Получаем индекс упавшего процесса
				const uint16_t index = this->cluster->_pids.at(watcher.rpid);
				// Удаляем процесс из списка процессов
				this->cluster->_pids.erase(watcher.rpid);
				// Устанавливаем дочерний процесс
				jt->second.at(index) = move(jack);
				// Замораживаем поток на период в 5 секунд
				this_thread::sleep_for(5s);
				// Выполняем создание нового процесса
				this->cluster->fork(it->first, index, it->second.restart);
			// Просто удаляем процесс из списка процессов
			} else this->cluster->_pids.erase(watcher.rpid);
		}
	}
#endif
/**
 * fork Метод отделения от основного процесса (создание дочерних процессов)
 * @param wid   идентификатор воркера
 * @param index индекс инициализированного процесса
 * @param stop  флаг остановки итерации создания дочерних процессов
 */
void awh::Cluster::fork(const size_t wid, const uint16_t index, const bool stop) noexcept {	
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем поиск воркера
		auto it = this->_workers.find(wid);
		// Если воркер найден
		if(it != this->_workers.end()){
			// Если не все форки созданы
			if(index < it->second.count){
				// Флаг первичной инициализации
				bool initialization = false;
				// Выполняем поиск работника
				auto jt = this->_jacks.find(it->first);
				// Выполняем проверку, проведена ли инициализация
				initialization = ((jt == this->_jacks.end()) || jt->second.empty());
				// Если список работников ещё пустой
				if(initialization){
					// Удаляем список дочерних процессов
					this->_pids.clear();
					// Если список работников еще не инициализирован
					if(jt == this->_jacks.end()){
						// Выполняем инициализацию списка работников
						this->_jacks.emplace(it->first, vector <unique_ptr <jack_t>> ());
						// Выполняем поиск работника
						jt = this->_jacks.find(it->first);
					}
					// Выполняем создание указанное количество работников
					for(size_t i = 0; i < it->second.count; i++){
						// Создаём объект работника
						unique_ptr <jack_t> jack(new jack_t);
						// Выполняем подписку на основной канал передачи данных
						if(pipe(jack->mfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выходим принудительно из приложения
							exit(EXIT_FAILURE);
						}
						// Выполняем подписку на дочерний канал передачи данных
						if(pipe(jack->cfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выходим принудительно из приложения
							exit(EXIT_FAILURE);
						}
						// Выполняем добавление работника в список работников
						jt->second.push_back(move(jack));
					}
				}
				// Устанавливаем идентификатор процесса
				pid_t pid = -1;
				// Определяем тип потока
				switch((pid = ::fork())){
					// Если поток не создан
					case -1: {
						// Выводим в лог сообщение
						this->_log->print("child process could not be created", log_t::flag_t::CRITICAL);
						// Выходим принудительно из приложения
						exit(EXIT_FAILURE);
					} break;
					// Если - это дочерний поток значит все нормально
					case 0: {
						// Если процесс является дочерним
						if(this->_pid == (pid_t) getppid()){
							// Получаем идентификатор текущего процесса
							const pid_t pid = getpid();
							// Добавляем в список дочерних процессов, идентификатор процесса
							this->_pids.emplace(pid, index);
							// Активируем флаг запуска кластера
							it->second.working = true;
							{
								// Получаем объект текущего работника
								jack_t * jack = jt->second.at(index).get();
								// Закрываем файловый дескриптор на запись в дочерний процесс
								::close(jack->cfds[1]);
								// Закрываем файловый дескриптор на чтение из основного процесса
								::close(jack->mfds[0]);
								// Устанавливаем идентификатор процесса
								jack->pid = pid;
								// Устанавливаем время начала жизни процесса
								jack->date = this->_fmk->unixTimestamp();
								// Устанавливаем базу событий для чтения
								jack->mess.set(this->_base);
								// Устанавливаем событие на чтение данных от основного процесса
								jack->mess.set <worker_t, &worker_t::message> (&it->second);
								// Устанавливаем сокет для чтения
								jack->mess.set(jack->cfds[0], ev::READ);
								// Запускаем чтение данных с основного процесса
								jack->mess.start();
								// Если функция обратного вызова установлена, выводим её
								if(this->_callback.process != nullptr)
									// Выводим функцию обратного вызова
									this->_callback.process(it->first, pid, event_t::START);
							}
							// Выполняем активацию базы событий
							ev_loop_fork(this->_base);
						// Если процесс превратился в зомби
						} else {
							// Процесс превратился в зомби, самоликвидируем его
							this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
							// Выходим из приложения
							exit(EXIT_FAILURE);
						}
					} break;
					// Если - это родительский процесс
					default: {
						// Добавляем в список дочерних процессов, идентификатор процесса
						this->_pids.emplace(pid, index);
						// Получаем объект текущего работника
						jack_t * jack = jt->second.at(index).get();
						// Закрываем файловый дескриптор на запись в основной процесс
						::close(jack->mfds[1]);
						// Закрываем файловый дескриптор на чтение из дочернего процесса
						::close(jack->cfds[0]);
						// Устанавливаем PID процесса
						jack->pid = pid;
						// Устанавливаем время начала жизни процесса
						jack->date = this->_fmk->unixTimestamp();
						// Устанавливаем базу событий для чтения
						jack->mess.set(this->_base);
						// Устанавливаем событие на чтение данных от дочернего процесса
						jack->mess.set <worker_t, &worker_t::message> (&it->second);
						// Устанавливаем сокет для чтения
						jack->mess.set(jack->mfds[0], ev::READ);
						// Запускаем чтение данных с дочернего процесса
						jack->mess.start();
						// Если функция обратного вызова установлена, выводим её
						if(initialization && (this->_callback.process != nullptr)){
							// Активируем флаг запуска кластера
							it->second.working = true;
							// Выводим функцию обратного вызова
							this->_callback.process(it->first, pid, event_t::START);
						}
						// Устанавливаем базу событий
						jack->cw.set(this->_base);
						// Устанавливаем событие на выход дочернего процесса
						jack->cw.set <worker_t, &worker_t::child> (&it->second);
						// Выполняем отслеживание статуса дочернего процесса
						jack->cw.start(pid);
						// Продолжаем дальше
						if(!stop) this->fork(it->first, index + 1, stop);
					}
				}
			}
		}
	#endif
}
/**
 * working Метод проверки на запуск работы кластера
 * @param wid идентификатор воркера
 * @return    результат работы проверки
 */
bool awh::Cluster::working(const size_t wid) const noexcept {
	// Выполняем поиск воркера
	auto it = this->_workers.find(wid);
	// Если воркер найден
	if(it != this->_workers.end())
		// Выводим результат проверки
		return it->second.working;
	// Сообщаем, что проверка не выполнена
	return false;
}
/**
 * send Метод отправки сообщения родительскому процессу
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::Cluster::send(const size_t wid, const char * buffer, const size_t size) noexcept {
	// Получаем идентификатор текущего процесса
	const pid_t pid = getpid();
	// Если процесс превратился в зомби
	if((this->_pid != pid) && (this->_pid != (pid_t) getppid())){
		// Процесс превратился в зомби, самоликвидируем его
		this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, pid);
		// Выходим из приложения
		exit(EXIT_FAILURE);
	// Если процесс не является родительским
	} else if((this->_pid != pid) && (size > 0)) {
		// Если отправляемый размер данных умещается в наш буфер сообщения
		if(size <= sizeof(mess_t::payload)){
			// Выполняем поиск работников
			auto jt = this->_jacks.find(wid);
			// Если работник найден
			if((jt != this->_jacks.end()) && (this->_pids.count(pid) > 0)){
				// Создаём объект сообщения
				mess_t message;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = pid;
				// Выполняем зануление буфер данных полезной нагрузки
				memset(message.payload, 0, sizeof(message.payload));
				// Выполняем копирование данных бинарного буфера
				memcpy(message.payload, buffer, size);
				// Выполняем отправку сообщения дочернему процессу
				::write(jt->second.at(this->_pids.at(pid))->mfds[1], &message, sizeof(message));
			}
		// Выводим в лог сообщение
		} else this->_log->print("transfer data size is %zu bytes, buffer size is %zu bytes", log_t::flag_t::WARNING, size, sizeof(mess_t::payload));
	}
}
/**
 * send Метод отправки сообщения дочернему процессу
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::Cluster::send(const size_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept {
	// Если процесс является родительским
	if((this->_pid == getpid()) && (size > 0)){
		// Если отправляемый размер данных умещается в наш буфер сообщения
		if(size <= sizeof(mess_t::payload)){
			// Выполняем поиск работников
			auto jt = this->_jacks.find(wid);
			// Если работник найден
			if((jt != this->_jacks.end()) && (this->_pids.count(pid) > 0)){
				// Создаём объект сообщения
				mess_t message;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Выполняем зануление буфер данных полезной нагрузки
				memset(message.payload, 0, sizeof(message.payload));
				// Выполняем копирование данных бинарного буфера
				memcpy(message.payload, buffer, size);
				// Выполняем отправку сообщения дочернему процессу
				::write(jt->second.at(this->_pids.at(pid))->cfds[1], &message, sizeof(message));
			}
		// Выводим в лог сообщение
		} else this->_log->print("transfer data size is %zu bytes, buffer size is %zu bytes", log_t::flag_t::WARNING, size, sizeof(mess_t::payload));
	// Если процесс превратился в зомби
	} if((this->_pid != getpid()) && (this->_pid != (pid_t) getppid())) {
		// Процесс превратился в зомби, самоликвидируем его
		this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * broadcast Метод отправки сообщения всем дочерним процессам
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::Cluster::broadcast(const size_t wid, const char * buffer, const size_t size) noexcept {
	// Если процесс является родительским
	if((this->_pid == getpid()) && (size > 0)){
		// Если отправляемый размер данных умещается в наш буфер сообщения
		if(size <= sizeof(mess_t::payload)){
			// Выполняем поиск работников
			auto jt = this->_jacks.find(wid);
			// Если работник найден
			if((jt != this->_jacks.end()) && !jt->second.empty()){
				// Создаём объект сообщения
				mess_t message;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Выполняем зануление буфер данных полезной нагрузки
				memset(message.payload, 0, sizeof(message.payload));
				// Выполняем копирование данных бинарного буфера
				memcpy(message.payload, buffer, size);
				// Переходим по всем дочерним процессам
				for(auto & jack : jt->second)
					// Выполняем отправку сообщения дочернему процессу
					::write(jack->cfds[1], &message, sizeof(message));
			}
		// Выводим в лог сообщение
		} else this->_log->print("transfer data size is %zu bytes, buffer size is %zu bytes", log_t::flag_t::WARNING, size, sizeof(mess_t::payload));
	// Если процесс превратился в зомби
	} if((this->_pid != getpid()) && (this->_pid != (pid_t) getppid())) {
		// Процесс превратился в зомби, самоликвидируем его
		this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * clear Метод очистки всех выделенных ресурсов
 */
void awh::Cluster::clear() noexcept {
	// Удаляем список дочерних процессов
	this->_pids.clear();
	// Если список работников не пустой
	if(!this->_jacks.empty()){
		// Переходим по всем работникам
		for(auto & item : this->_jacks)
			// Выполняем остановку процессов
			this->stop(item.first);
		// Выполняем очистку списка работников
		this->_jacks.clear();
		// Выполняем освобождение выделенной памяти
		map <size_t, vector <unique_ptr <jack_t>>> ().swap(this->_jacks);
	}
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем освобождение выделенной памяти
	map <size_t, worker_t> ().swap(this->_workers);
}
/**
 * stop Метод остановки кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::stop(const size_t wid) noexcept {
	// Выполняем поиск работников
	auto jt = this->_jacks.find(wid);
	// Если работник найден
	if((jt != this->_jacks.end()) && !jt->second.empty()){
		// Выполняем поиск воркера
		auto it = this->_workers.find(jt->first);
		// Если процесс является родительским
		if(this->_pid == getpid()){
			// Флаг перезапуска
			bool restart = false;
			// Если воркер найден, получаем флаг перезапуска
			if(it != this->_workers.end()){
				// Получаем флаг перезапуска
				restart = it->second.restart;
				// Снимаем флаг перезапуска процесса
				it->second.restart = false;
			}
			// Создаём объект сообщения
			mess_t message;
			// Устанавливаем флаг остановки процесса
			message.stop = true;
			// Устанавливаем пид процесса отправившего сообщение
			message.pid = this->_pid;
			// Переходим по всему списку работников
			for(auto & jack : jt->second){
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Останавливаем обработку получения статуса процессов
					jack->cw.stop();
				#endif
				// Останавливаем чтение данных с дочернего процесса
				jack->mess.stop();
				// Выполняем отправку сообщения дочернему процессу
				::write(jack->cfds[1], &message, sizeof(message));
				// Выполняем закрытие файловых дескрипторов
				::close(jack->mfds[0]);
				::close(jack->cfds[1]);
			}
			// Если воркер найден, возвращаем флаг перезапуска
			if(it != this->_workers.end())
				// Возвращаем значение флага автоматического перезапуска процесса
				it->second.restart = restart;
			// Очищаем список работников
			jt->second.clear();
		// Если процесс является дочерним
		} else if(this->_pid == (pid_t) getppid()) {
			// Переходим по всему списку работников
			for(auto & jack : jt->second){
				// Останавливаем чтение данных с родительского процесса
				jack->mess.stop();
				// Выполняем закрытие файловых дескрипторов
				::close(jack->cfds[0]);
				::close(jack->mfds[1]);
			}
			// Очищаем список работников
			jt->second.clear();
		// Если процесс превратился в зомби
		} else {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
		// Если воркер найден, снимаем флаг запуска кластера
		if(it != this->_workers.end())
			// Снимаем флаг запуска кластера
			it->second.working = false;
		// Удаляем список дочерних процессов
		this->_pids.clear();
	}
}
/**
 * start Метод запуска кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::start(const size_t wid) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end())
		// Выполняем запуск процесса
		this->fork(it->first);
}
/**
 * restart Метод установки флага перезапуска процессов
 * @param wid  идентификатор воркера
 * @param mode флаг перезапуска процессов
 */
void awh::Cluster::restart(const size_t wid, const bool mode) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end())
		// Устанавливаем флаг автоматического перезапуска процесса
		it->second.restart = mode;
}
/**
 * base Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Cluster::base(struct ev_loop * base) noexcept {
	// Переходим по всем активным воркерам
	for(auto & worker : this->_workers)
		// Выполняем остановку процессов
		this->stop(worker.first);
	// Выполняем установку базы событий
	this->_base = base;
}
/**
 * count Метод получения максимального количества процессов
 * @param wid идентификатор воркера
 * @return    максимальное количество процессов
 */
uint16_t awh::Cluster::count(const size_t wid) const noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end())
		// Выводим максимально-возможное количество процессов
		return it->second.count;
	// Выводим результат
	return 0;
}
/**
 * count Метод установки максимального количества процессов
 * @param wid   идентификатор воркера
 * @param count максимальное количество процессов
 */
void awh::Cluster::count(const size_t wid, const uint16_t count) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end()){
		// Если количество процессов не передано
		if(count == 0)
			// Устанавливаем максимальное количество ядер доступных в системе
			it->second.count = std::thread::hardware_concurrency();
		// Устанавливаем максимальное количество процессов
		else it->second.count = count;
	}
}
/**
 * init Метод инициализации воркера
 * @param wid   идентификатор воркера
 * @param count максимальное количество процессов
 */
void awh::Cluster::init(const size_t wid, const uint16_t count) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если воркер не найден
	if(it == this->_workers.end()){
		// Добавляем воркер в список воркеров
		auto ret = this->_workers.emplace(wid, worker_t());
		// Устанавливаем идентификатор воркера
		ret.first->second.wid = wid;
		// Устанавливаем родительский объект кластера
		ret.first->second.cluster = this;
	}
	// Выполняем установку максимально-возможного количества процессов
	this->count(wid, count);
}
/**
 * onMessage Метод установки функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
 * @param callback функция обратного вызова
 */
void awh::Cluster::on(function <void (const size_t, const pid_t, const event_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.process = callback;
}
/**
 * onMessage Метод установки функции обратного вызова при получении сообщения
 * @param callback функция обратного вызова
 */
void awh::Cluster::on(function <void (const size_t, const pid_t, const char *, const size_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.message = callback;
}
