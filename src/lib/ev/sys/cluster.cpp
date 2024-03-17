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
#include <lib/ev/sys/cluster.hpp>

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * callback Метод вывода функции обратного вызова
	 * @param data данные передаваемые процессом
	 */
	void awh::Cluster::Worker::callback(const data_t & data) noexcept {
		// Если функция обратного вызова установлена
		if(this->cluster->_callbacks.is("message"))
			// Выполняем функцию обратного вызова
			this->cluster->_callbacks.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->wid, data.pid, data.buffer.data(), data.buffer.size());
	}
	/**
	 * message Функция обратного вызова получении сообщений
	 * @param watcher объект события чтения
	 * @param revents идентификатор события
	 */
	void awh::Cluster::Worker::message(ev::io & watcher, int revents) noexcept {
		// Бинарный буфер для получения данных
		char buffer[4096];
		// Заполняем буфер нулями
		::memset(buffer, 0, sizeof(buffer));
		// Если процесс является родительским
		if(this->cluster->_pid == static_cast <pid_t> (::getpid())){
			// Идентификатор процесса приславший сообщение
			pid_t pid = 0;
			// Выполняем поиск текущего брокера
			auto i = this->cluster->_jacks.find(this->wid);
			// Если текущий брокер найден
			if(i != this->cluster->_jacks.end()){
				// Флаг найденного файлового дескриптора
				bool found = false;
				// Переходим по всему списку брокеров
				for(auto & broker : i->second){
					// Выполняем поиск файлового дескриптора
					found = (broker->mfds[0] == watcher.fd);
					// Если файловый дескриптор соответствует
					if(found){
						// Получаем идентификатор процесса приславшего сообщение
						pid = broker->pid;
						// Выходим из цикла
						break;
					}
				}
				// Если файловый дескриптор не найден
				if(!found){
					// Останавливаем чтение
					watcher.stop();
					// Выполняем закрытие файлового дескриптора
					::close(watcher.fd);
					// Выходим из функции
					return;
				}
			}
			// Выполняем чтение полученного сообщения
			const int bytes = ::read(watcher.fd, buffer, sizeof(buffer));
			// Если данные прочитаны правильно
			if(bytes > 0){
				// Создаём объект сообщения
				mess_t message;
				// Выполняем зануление буфера данных полезной нагрузки
				::memset(message.payload, 0, sizeof(message.payload));
				// Выполняем извлечение входящих данных
				::memcpy(&message, buffer, bytes);
				// Если размер данных соответствует
				if((message.size > 0) && (message.size <= sizeof(message.payload))){
					// Выполняем добавление полученных данных в общий буфер
					this->_buffer.insert(this->_buffer.end(), message.payload, message.payload + message.size);
					// Если передана последняя порция
					if(message.end){
						// Создаём объект передаваемых данных
						data_t data;
						// Устанавливаем идентификатор процесса
						data.pid = message.pid;
						// Устанавливаем буфер передаваемых данных
						data.buffer.assign(this->_buffer.begin(), this->_buffer.end());
						// Выполняем очистку буфера сообщений
						this->_buffer.clear();
						// Если асинхронный режим работы активирован
						if(this->async){
							// Выполняем запуск работы дочернего процесса
							this->startChild();
							// Выполняем отправку полученных данных
							this->_child->send(std::move(data));
						// Если асинхронный режим работы деактивирован
						} else {
							// Выполняем остановку работы дочернего процесса
							this->stopChild();
							// Выполняем отправку полученных данных напрямую
							this->callback(std::move(data));
						}
					}
				// Выводим сообщение что данные пришли битые
				} else this->_log->print("[%u] Data from child process [%u] arrives corrupted", log_t::flag_t::CRITICAL, this->cluster->_pid, pid);
			// Если данные не прочитаны
			} else {
				// Выводим сообщение об ошибке в лог
				this->_log->print("[%u] Data from child process [%u] could not be received", log_t::flag_t::CRITICAL, this->cluster->_pid, pid);
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выходим из функции
					return;
				/**
				 * Если режим отладки не активирован
				 */
				#else
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				#endif
			}
		// Если процесс является дочерним
		} else if(this->cluster->_pid == static_cast <pid_t> (::getppid())) {
			// Выполняем поиск текущего брокера
			auto i = this->cluster->_jacks.find(this->wid);
			// Если текущий брокер найден
			if(i != this->cluster->_jacks.end()){
				// Получаем индекс текущего процесса
				const uint16_t index = this->cluster->_pids.at(::getpid());
				// Получаем объект текущего брокера
				broker_t * broker = i->second.at(index).get();
				// Если файловый дескриптор не соответствует родительскому
				if(broker->cfds[0] != watcher.fd){
					// Останавливаем чтение
					watcher.stop();
					// Переходим по всему списку брокеров
					for(auto & item : i->second){
						// Если брокер не является текущим брокером
						if((broker->cfds[0] != item->cfds[0]) && (broker->mfds[1] != item->mfds[1])){
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
				// Выполняем чтение полученного сообщения
				const int bytes = ::read(watcher.fd, buffer, sizeof(buffer));
				// Если данные прочитаны правильно
				if(bytes > 0){
					// Создаём объект сообщения
					mess_t message;
					// Выполняем зануление буфера данных полезной нагрузки
					::memset(message.payload, 0, sizeof(message.payload));
					// Выполняем извлечение входящих данных
					::memcpy(&message, buffer, bytes);
					// Если размер данных соответствует
					if((message.size > 0) && (message.size <= sizeof(message.payload))){
						// Выполняем добавление полученных данных в общий буфер
						this->_buffer.insert(this->_buffer.end(), message.payload, message.payload + message.size);
						// Если нужно завершить работу процесса
						if(message.quit){
							// Если передана последняя порция
							if(message.end){
								// Создаём объект передаваемых данных
								data_t data;
								// Устанавливаем идентификатор процесса
								data.pid = message.pid;
								// Устанавливаем буфер передаваемых данных
								data.buffer.assign(this->_buffer.begin(), this->_buffer.end());
								// Выполняем очистку буфера сообщений
								this->_buffer.clear();
								// Выполняем отправку полученных данных
								this->callback(std::move(data));
							}
							// Останавливаем чтение данных с родительского процесса
							this->cluster->stop(this->wid);
							// Выходим из приложения
							::exit(SIGCHLD);
						// Если передана последняя порция
						} else if(message.end) {
							// Создаём объект передаваемых данных
							data_t data;
							// Устанавливаем идентификатор процесса
							data.pid = message.pid;
							// Устанавливаем буфер передаваемых данных
							data.buffer.assign(this->_buffer.begin(), this->_buffer.end());
							// Выполняем очистку буфера сообщений
							this->_buffer.clear();
							// Если асинхронный режим работы активирован
							if(this->async){
								// Выполняем запуск работы дочернего процесса
								this->startChild();
								// Выполняем отправку полученных данных
								this->_child->send(std::move(data));
							// Если асинхронный режим работы деактивирован
							} else {
								// Выполняем остановку работы дочернего процесса
								this->stopChild();
								// Выполняем отправку полученных данных напрямую
								this->callback(std::move(data));
							}
						}
					// Если данные пришли пустыми
					} else {
						// Если нужно завершить работу процесса
						if(message.quit){
							// Останавливаем чтение данных с родительского процесса
							this->cluster->stop(this->wid);
							// Выходим из приложения
							::exit(SIGCHLD);
						// Выводим сообщение что данные пришли битые
						} else this->_log->print("[%u] Data from main process arrives corrupted", log_t::flag_t::CRITICAL, ::getpid());
					}
				// Если данные не прочитаны
				} else {
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from main process could not be received", log_t::flag_t::CRITICAL, ::getpid());
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выходим из функции
						return;
					/**
					 * Если режим отладки не активирован
					 */
					#else
						// Выходим из приложения
						exit(EXIT_FAILURE);
					#endif
				}
			}
		// Если процесс превратился в зомби
		} else {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			// Останавливаем чтение данных с родительского процесса
			this->cluster->stop(this->wid);
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выходим из функции
				return;
			/**
			 * Если режим отладки не активирован
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
	}
	/**
	 * child Функция обратного вызова при завершении работы процесса
	 * @param watcher объект события дочернего процесса
	 * @param revents идентификатор события
	 */
	void awh::Cluster::Worker::child(ev::child & watcher, int revents) noexcept {
		// Останавливаем сигнал
		watcher.stop();
		// Выполняем создание дочернего потока
		std::thread(&worker_t::process, this, watcher.rpid, watcher.rstatus).detach();
	}
	/**
	 * process Метод перезапуска упавшего процесса
	 * @param pid    идентификатор упавшего процесса
	 * @param status статус остановившегося процесса
	 */
	void awh::Cluster::Worker::process(const pid_t pid, const int status) noexcept {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->mtx);
		// Выполняем поиск брокера
		auto j = this->cluster->_jacks.find(this->wid);
		// Если брокер найден
		if(j != this->cluster->_jacks.end()){
			// Выполняем поиск завершившегося процесса
			for(auto & broker : j->second){
				// Если процесс найден
				if((broker->end = (broker->pid == pid))){
					// Останавливаем чтение данных с дочернего процесса
					broker->mess.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(broker->cfds[0]);
					::close(broker->mfds[1]);
					// Если статус сигнала, ручной остановкой процесса
					if(status == SIGINT){
						// Выполняем остановку работы
						this->cluster->stop(this->wid);
						// Выходим из приложения
						::exit(SIGINT);
					// Если время жизни процесса составляет меньше 3-х минут
					} else if((this->cluster->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - broker->date) <= 180000){
						// Выполняем остановку работы
						this->cluster->stop(this->wid);
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Выводим сообщение об ошибке, о невозможности отправкить сообщение
					// this->_log->print("Child process stopped, PID=%s, STATUS=%u", log_t::flag_t::CRITICAL, broker->pid, status);
					// Если функция обратного вызова установлена
					if(this->cluster->_callbacks.is("process"))
						// Выполняем функцию обратного вызова
						this->cluster->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", j->first, pid, event_t::STOP);
					// Выполняем поиск воркера
					auto i = this->cluster->_workers.find(j->first);
					// Если запрашиваемый воркер найден и флаг автоматического перезапуска активен
					if((i != this->cluster->_workers.end()) && i->second->restart){
						// Получаем индекс упавшего процесса
						const uint16_t index = this->cluster->_pids.at(broker->pid);
						// Удаляем процесс из списка процессов
						this->cluster->_pids.erase(broker->pid);
						// Выполняем создание нового процесса
						this->cluster->fork(i->first, index, i->second->restart);
					// Просто удаляем процесс из списка процессов
					} else this->cluster->_pids.erase(broker->pid);
					// Выходим из цикла
					break;
				}
			}
		}
	}
#endif
/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * stopChild Метод остановки работы дочернего процесса
	 */
	void awh::Cluster::Worker::stopChild() noexcept {
		// Если объект дочерних потоков создан
		if(this->_child != nullptr){
			// Удаляем его
			delete this->_child;
			// Выполняем зануление объекта дочернего потока
			this->_child = nullptr;
		}
	}
	/**
	 * startChild Метод запуска работы дочернего процесса
	 */
	void awh::Cluster::Worker::startChild() noexcept {
		// Если объект дочерних потоков ещё не создан
		if(this->_child == nullptr){
			// Выполняем создание объекта дочернего потока
			this->_child = new child_t <data_t> ();
			// Выполняем установку функции обратного вызова при получении сообщения
			this->_child->on(std::bind(&worker_t::callback, this, _1));
		}
	}
#endif
/**
 * ~Worker Деструктор
 */
awh::Cluster::Worker::~Worker() noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем остановку работы дочернего процесса
		this->stopChild();
	#endif
}
/**
 * fork Метод отделения от основного процесса (создание дочерних процессов)
 * @param wid   идентификатор воркера
 * @param index индекс инициализированного процесса
 * @param stop  флаг остановки итерации создания дочерних процессов
 */
void awh::Cluster::fork(const uint16_t wid, const uint16_t index, const bool stop) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем поиск воркера
		auto i = this->_workers.find(wid);
		// Если воркер найден
		if(i != this->_workers.end()){
			// Если не все форки созданы
			if(index < i->second->count){
				// Выполняем поиск брокера
				auto j = this->_jacks.find(i->first);
				// Если список брокеров ещё пустой
				if((j == this->_jacks.end()) || j->second.empty()){
					// Удаляем список дочерних процессов
					this->_pids.clear();
					// Если список брокеров еще не инициализирован
					if(j == this->_jacks.end()){
						// Выполняем инициализацию списка брокеров
						this->_jacks.emplace(i->first, vector <unique_ptr <broker_t>> ());
						// Выполняем поиск брокера
						j = this->_jacks.find(i->first);
					}
					// Выполняем создание указанное количество брокеров
					for(size_t index = 0; index < i->second->count; index++){
						// Создаём объект брокера
						unique_ptr <broker_t> broker(new broker_t);
						// Выполняем подписку на основной канал передачи данных
						if(::pipe(broker->mfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
							// Выходим принудительно из приложения
							::exit(EXIT_FAILURE);
						}
						// Выполняем подписку на дочерний канал передачи данных
						if(::pipe(broker->cfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
							// Выходим принудительно из приложения
							::exit(EXIT_FAILURE);
						}
						// Выполняем добавление брокера в список брокеров
						j->second.push_back(std::move(broker));
					}
				}
				// Если процесс завершил свою работу
				if(j->second.at(index)->end){
					// Создаём объект брокера
					unique_ptr <broker_t> broker(new broker_t);
					// Выполняем подписку на основной канал передачи данных
					if(::pipe(broker->mfds) != 0){
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						// Выполняем поиск завершившегося процесса
						for(auto & broker : j->second)
							// Выполняем остановку чтение сообщений
							broker->mess.stop();
						// Выполняем остановку работы
						this->stop(i->first);
						// Выходим принудительно из приложения
						::exit(EXIT_FAILURE);
					}
					// Выполняем подписку на дочерний канал передачи данных
					if(::pipe(broker->cfds) != 0){
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						// Выполняем поиск завершившегося процесса
						for(auto & broker : j->second)
							// Выполняем остановку чтение сообщений
							broker->mess.stop();
						// Выполняем остановку работы
						this->stop(i->first);
						// Выходим принудительно из приложения
						::exit(EXIT_FAILURE);
					}
					// Устанавливаем нового брокера
					j->second.at(index) = std::move(broker);
				}
				// Устанавливаем идентификатор процесса
				pid_t pid = -1;
				// Определяем тип потока
				switch((pid = ::fork())){
					// Если поток не создан
					case -1: {
						// Выводим в лог сообщение
						this->_log->print("Child process could not be created", log_t::flag_t::CRITICAL);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выходим из функции
							return;
						/**
						 * Если режим отладки не активирован
						 */
						#else
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						#endif
					} break;
					// Если - это дочерний поток значит все нормально
					case 0: {
						// Если процесс является дочерним
						if((i->second->working = (this->_pid == static_cast <pid_t> (::getppid())))){
							// Получаем идентификатор текущего процесса
							const pid_t pid = ::getpid();
							// Добавляем в список дочерних процессов, идентификатор процесса
							this->_pids.emplace(pid, index);
							{
								// Получаем объект текущего брокера
								broker_t * broker = j->second.at(index).get();
								// Выполняем перебор всего списка брокеров
								for(size_t i = 0; i < j->second.size(); i++){
									// Если индекс брокера совпадает
									if(i == static_cast <uint16_t> (index)){
										// Закрываем файловый дескриптор на запись в дочерний процесс
										::close(j->second.at(i)->cfds[1]);
										// Закрываем файловый дескриптор на чтение из основного процесса
										::close(j->second.at(i)->mfds[0]);
									// Закрываем все файловые дескрипторы для всех остальных брокеров
									} else {
										// Закрываем файловый дескриптор на запись в дочерний процесс
										::close(j->second.at(i)->cfds[0]);
										::close(j->second.at(i)->cfds[1]);
										// Закрываем файловый дескриптор на чтение из основного процесса
										::close(j->second.at(i)->mfds[0]);
										::close(j->second.at(i)->mfds[1]);
									}
								}
								// Устанавливаем идентификатор процесса
								broker->pid = pid;
								// Устанавливаем время начала жизни процесса
								broker->date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
								// Устанавливаем базу событий для чтения
								broker->mess.set(this->_base);
								// Устанавливаем событие на чтение данных от основного процесса
								broker->mess.set <worker_t, &worker_t::message> (i->second.get());
								// Устанавливаем сокет для чтения
								broker->mess.set(broker->cfds[0], ev::READ);
								// Запускаем чтение данных с основного процесса
								broker->mess.start();
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("process"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, pid, event_t::START);
							}
							// Выполняем активацию базы событий
							ev_loop_fork(this->_base);
						// Если процесс превратился в зомби
						} else {
							// Процесс превратился в зомби, самоликвидируем его
							this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выходим из функции
								return;
							/**
							 * Если режим отладки не активирован
							 */
							#else
								// Выходим из приложения
								::exit(EXIT_FAILURE);
							#endif
						}
					} break;
					// Если процесс является родительским
					default: {
						// Добавляем в список дочерних процессов, идентификатор процесса
						this->_pids.emplace(pid, index);
						// Получаем объект текущего брокера
						broker_t * broker = j->second.at(index).get();
						// Закрываем файловый дескриптор на запись в основной процесс
						::close(broker->mfds[1]);
						// Закрываем файловый дескриптор на чтение из дочернего процесса
						::close(broker->cfds[0]);
						// Устанавливаем PID-процесса
						broker->pid = pid;
						// Устанавливаем время начала жизни процесса
						broker->date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
						// Устанавливаем базу событий для чтения
						broker->mess.set(this->_base);
						// Устанавливаем событие на чтение данных от дочернего процесса
						broker->mess.set <worker_t, &worker_t::message> (i->second.get());
						// Устанавливаем сокет для чтения
						broker->mess.set(broker->mfds[0], ev::READ);
						// Запускаем чтение данных с дочернего процесса
						broker->mess.start();
						// Если нужно отслеживать падение дочернего процесса
						if(this->_trackCrash){
							// Устанавливаем базу событий
							broker->cw.set(this->_base);
							// Устанавливаем событие на выход дочернего процесса
							broker->cw.set <worker_t, &worker_t::child> (i->second.get());
							// Выполняем отслеживание статуса дочернего процесса
							broker->cw.start(pid);
						}
						// Если не все процессы созданы
						if(!stop)
							// Создаём следующий дочерний процесс
							this->fork(i->first, index + 1, stop);
					}
				}
			// Если все процессы удачно созданы
			} else if((i->second->working = !stop)) {
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("process"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, this->_pid, event_t::START);
			}
		}
	#endif
}
/**
 * working Метод проверки на запуск работы кластера
 * @param wid идентификатор воркера
 * @return    результат работы проверки
 */
bool awh::Cluster::working(const uint16_t wid) const noexcept {
	// Выполняем поиск воркера
	auto i = this->_workers.find(wid);
	// Если воркер найден
	if(i != this->_workers.end())
		// Выводим результат проверки
		return i->second->working;
	// Сообщаем, что проверка не выполнена
	return false;
}
/**
 * pids Метод получения списка дочерних процессов
 * @param wid идентификатор воркера
 * @return    список дочерних процессов
 */
set <pid_t> awh::Cluster::pids(const uint16_t wid) const noexcept {
	// Результат работы функции
	set <pid_t> result;
	// Выполняем поиск брокеров
	auto i = this->_jacks.find(wid);
	// Если брокер найден
	if((i != this->_jacks.end()) && !i->second.empty()){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Переходим по всему списку брокеров
			for(auto & broker : i->second)
				// Выполняем формирование списка процессов
				result.emplace(broker->pid);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки сообщения родительскому процессу
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::Cluster::send(const uint16_t wid, const char * buffer, const size_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Получаем идентификатор текущего процесса
		const pid_t pid = ::getpid();
		// Если процесс превратился в зомби
		if((this->_pid != pid) && (this->_pid != static_cast <pid_t> (::getppid()))){
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, pid);
			// Выполняем остановку работы
			this->stop(wid);
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выходим из функции
				return;
			/**
			 * Если режим отладки не активирован
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		// Если процесс не является родительским
		} else if((this->_pid != pid) && (size > 0)) {
			// Выполняем поиск брокеров
			auto i = this->_jacks.find(wid);
			// Если брокер найден
			if((i != this->_jacks.end()) && (this->_pids.count(pid) > 0)){
				// Создаём объект сообщения
				mess_t message;
				// Смещение в буфере
				size_t offset = 0;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = pid;
				// Выполняем отправку всего сообщения частами
				do {
					// Выполняем определение размера отправляемого сообщения
					message.size = ((size - offset) >= sizeof(message.payload) ? sizeof(message.payload) : (size - offset));
					// Выполняем установку флага конца чанка
					message.end = ((offset + message.size) == size);
					// Выполняем зануление буфера полезной нагрузки
					::memset(message.payload, 0, sizeof(message.payload));
					// Выполняем копирование данные полезной нагрузки
					::memcpy(message.payload, buffer + offset, message.size);
					// Выполняем отправку сообщения дочернему процессу
					::write(i->second.at(this->_pids.at(pid))->mfds[1], &message, sizeof(message));
					// Выполняем увеличение смещения в буфере
					offset += message.size;
				} while(offset < size);
			}
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * send Метод отправки сообщения дочернему процессу
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::Cluster::send(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if((this->_pid == static_cast <pid_t> (::getpid())) && (size > 0)){
			// Выполняем поиск брокеров
			auto i = this->_jacks.find(wid);
			// Если брокер найден
			if((i != this->_jacks.end()) && (this->_pids.count(pid) > 0)){
				// Создаём объект сообщения
				mess_t message;
				// Смещение в буфере
				size_t offset = 0;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Выполняем отправку всего сообщения частами
				do {
					// Выполняем определение размера отправляемого сообщения
					message.size = ((size - offset) >= sizeof(message.payload) ? sizeof(message.payload) : (size - offset));
					// Выполняем установку флага конца чанка
					message.end = ((offset + message.size) == size);
					// Выполняем зануление буфера полезной нагрузки
					::memset(message.payload, 0, sizeof(message.payload));
					// Выполняем копирование данные полезной нагрузки
					::memcpy(message.payload, buffer + offset, message.size);
					// Выполняем отправку сообщения дочернему процессу
					::write(i->second.at(this->_pids.at(pid))->cfds[1], &message, sizeof(message));
					// Выполняем увеличение смещения в буфере
					offset += message.size;
				} while(offset < size);
			}
		// Если процесс превратился в зомби
		} else if((this->_pid != static_cast <pid_t> (::getpid())) && (this->_pid != static_cast <pid_t> (::getppid()))) {
			// Выполняем остановку работы
			this->stop(wid);
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выходим из функции
				return;
			/**
			 * Если режим отладки не активирован
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * broadcast Метод отправки сообщения всем дочерним процессам
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 */
void awh::Cluster::broadcast(const uint16_t wid, const char * buffer, const size_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if((this->_pid == static_cast <pid_t> (::getpid())) && (size > 0)){
			// Выполняем поиск брокеров
			auto i = this->_jacks.find(wid);
			// Если брокер найден
			if((i != this->_jacks.end()) && !i->second.empty()){
				// Создаём объект сообщения
				mess_t message;
				// Смещение в буфере
				size_t offset = 0;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Выполняем отправку всего сообщения частами
				do {
					// Выполняем определение размера отправляемого сообщения
					message.size = ((size - offset) >= sizeof(message.payload) ? sizeof(message.payload) : (size - offset));
					// Выполняем установку флага конца чанка
					message.end = ((offset + message.size) == size);
					// Выполняем зануление буфера полезной нагрузки
					::memset(message.payload, 0, sizeof(message.payload));
					// Выполняем копирование данные полезной нагрузки
					::memcpy(message.payload, buffer + offset, message.size);
					// Переходим по всем дочерним процессам
					for(auto & broker : i->second){
						// Если идентификатор процесса не нулевой
						if(broker->pid > 0)
							// Выполняем отправку сообщения дочернему процессу
							::write(broker->cfds[1], &message, sizeof(message));
					}
					// Выполняем увеличение смещения в буфере
					offset += message.size;
				} while(offset < size);
			}
		// Если процесс превратился в зомби
		} else if((this->_pid != ::getpid()) && (this->_pid != static_cast <pid_t> (::getppid()))) {
			// Выполняем остановку работы
			this->stop(wid);
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выходим из функции
				return;
			/**
			 * Если режим отладки не активирован
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
	/**
	 * Если операционной системой является Windows
	 */
	#else
		// Выводим предупредительное сообщение в лог
		this->_log->print("MS Windows OS, does not support cluster mode", log_t::flag_t::WARNING);
	#endif
}
/**
 * clear Метод очистки всех выделенных ресурсов
 */
void awh::Cluster::clear() noexcept {
	// Удаляем список дочерних процессов
	this->_pids.clear();
	// Если список брокеров не пустой
	if(!this->_jacks.empty()){
		// Переходим по всем брокерам
		for(auto & item : this->_jacks)
			// Выполняем остановку процессов
			this->stop(item.first);
		// Выполняем очистку списка брокеров
		this->_jacks.clear();
		// Выполняем освобождение выделенной памяти
		map <uint16_t, vector <unique_ptr <broker_t>>> ().swap(this->_jacks);
	}
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем освобождение выделенной памяти
	map <uint16_t, unique_ptr <worker_t>> ().swap(this->_workers);
}
/**
 * close Метод закрытия всех подключений
 */
void awh::Cluster::close() noexcept {
	// Если список брокеров не пустой
	if(!this->_jacks.empty()){
		// Переходим по всем брокерам
		for(auto & item : this->_jacks){
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Переходим по всему списку брокеров
				for(auto & broker : item.second){
					// Останавливаем обработку получения статуса процессов
					broker->cw.stop();
					// Останавливаем чтение данных с дочернего процесса
					broker->mess.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(broker->cfds[0]);
					::close(broker->mfds[1]);
				}
			#endif
			// Очищаем список брокеров
			item.second.clear();
		}
	}
}
/**
 * close Метод закрытия всех подключений
 * @param wid идентификатор воркера
 */
void awh::Cluster::close(const uint16_t wid) noexcept {
	// Выполняем поиск брокеров
	auto i = this->_jacks.find(wid);
	// Если брокер найден
	if((i != this->_jacks.end()) && !i->second.empty()){
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Переходим по всему списку брокеров
			for(auto & broker : i->second){
				// Останавливаем обработку получения статуса процессов
				broker->cw.stop();
				// Останавливаем чтение данных с дочернего процесса
				broker->mess.stop();
				// Выполняем закрытие файловых дескрипторов
				::close(broker->cfds[0]);
				::close(broker->mfds[1]);
			}
		#endif
		// Очищаем список брокеров
		i->second.clear();
	}
}
/**
 * stop Метод остановки кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::stop(const uint16_t wid) noexcept {
	// Выполняем поиск брокеров
	auto j = this->_jacks.find(wid);
	// Если брокер найден
	if((j != this->_jacks.end()) && !j->second.empty()){
		// Выполняем поиск воркера
		auto i = this->_workers.find(j->first);
		// Если процесс является родительским
		if(this->_pid == static_cast <pid_t> (::getpid())){
			// Флаг перезапуска
			bool restart = false;
			// Если воркер найден, получаем флаг перезапуска
			if(i != this->_workers.end()){
				// Получаем флаг перезапуска
				restart = i->second->restart;
				// Снимаем флаг перезапуска процесса
				i->second->restart = false;
			}
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Создаём объект сообщения
				mess_t message;
				// Устанавливаем флаг остановки процесса
				message.quit = true;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Переходим по всему списку брокеров
				for(auto & broker : j->second)
					// Выполняем отправку сообщения дочернему процессу
					::write(broker->cfds[1], &message, sizeof(message));
				// Выполняем закрытие подключения передачи сообщений
				this->close(wid);
			#endif
			// Если воркер найден, возвращаем флаг перезапуска
			if(i != this->_workers.end())
				// Возвращаем значение флага автоматического перезапуска процесса
				i->second->restart = restart;
		// Если процесс является дочерним
		} else if(this->_pid == static_cast <pid_t> (::getppid()))
			// Выполняем закрытие подключения передачи сообщений
			this->close(wid);
		// Если процесс превратился в зомби
		else {
			// Выполняем закрытие подключения передачи сообщений
			this->close(wid);
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выходим из функции
				return;
			/**
			 * Если режим отладки не активирован
			 */
			#else
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			#endif
		}
		// Если воркер найден, снимаем флаг запуска кластера
		if(i != this->_workers.end())
			// Снимаем флаг запуска кластера
			i->second->working = false;
		// Удаляем список дочерних процессов
		this->_pids.clear();
	}
}
/**
 * start Метод запуска кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::start(const uint16_t wid) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(wid);
	// Если вокер найден
	if(i != this->_workers.end())
		// Выполняем запуск процесса
		this->fork(i->first);
}
/**
 * restart Метод установки флага перезапуска процессов
 * @param wid  идентификатор воркера
 * @param mode флаг перезапуска процессов
 */
void awh::Cluster::restart(const uint16_t wid, const bool mode) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(wid);
	// Если вокер найден
	if(i != this->_workers.end())
		// Устанавливаем флаг автоматического перезапуска процесса
		i->second->restart = mode;
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
 * trackCrash Метод отключения отслеживания падения дочерних процессов
 * @param mode флаг отслеживания падения дочерних процессов
 */
void awh::Cluster::trackCrash(const bool mode) noexcept {
	// Выполняем установку флага отслеживания падения дочерних процессов
	this->_trackCrash = mode;
}
/**
 * async Метод установки флага асинхронного режима работы
 * @param wid  идентификатор воркера
 * @param mode флаг асинхронного режима работы
 */
void awh::Cluster::async(const uint16_t wid, const bool mode) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(wid);
	// Если вокер найден
	if(i != this->_workers.end())
		// Устанавливаем флаг асинхронного режима работы
		i->second->async = mode;
}
/**
 * count Метод получения максимального количества процессов
 * @param wid идентификатор воркера
 * @return    максимальное количество процессов
 */
uint16_t awh::Cluster::count(const uint16_t wid) const noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(wid);
	// Если вокер найден
	if(i != this->_workers.end())
		// Выводим максимально-возможное количество процессов
		return i->second->count;
	// Выводим результат
	return 0;
}
/**
 * count Метод установки максимального количества процессов
 * @param wid   идентификатор воркера
 * @param count максимальное количество процессов
 */
void awh::Cluster::count(const uint16_t wid, const uint16_t count) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(wid);
	// Если вокер найден
	if(i != this->_workers.end()){
		// Если количество процессов не передано
		if(count == 0)
			// Устанавливаем максимальное количество ядер доступных в системе
			i->second->count = (std::thread::hardware_concurrency() / 2);
		// Устанавливаем максимальное количество процессов
		else i->second->count = count;
		// Если количество процессов не установлено
		if(i->second->count == 0)
			// Устанавливаем один рабочий процесс
			i->second->count = 1;
	}
}
/**
 * init Метод инициализации воркера
 * @param wid   идентификатор воркера
 * @param count максимальное количество процессов
 */
void awh::Cluster::init(const uint16_t wid, const uint16_t count) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(wid);
	// Если воркер не найден
	if(i == this->_workers.end()){
		// Добавляем воркер в список воркеров
		auto ret = this->_workers.emplace(wid, unique_ptr <worker_t> (new worker_t(this->_log)));
		// Устанавливаем идентификатор воркера
		ret.first->second->wid = wid;
		// Устанавливаем родительский объект кластера
		ret.first->second->cluster = this;
	}
	// Выполняем установку максимально-возможного количества процессов
	this->count(wid, count);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::Cluster::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
	this->_callbacks.set("process", callbacks);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callbacks.set("message", callbacks);
}
