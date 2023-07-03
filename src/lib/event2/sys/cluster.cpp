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
#include <lib/event2/sys/cluster.hpp>

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * callback Метод вывода функции обратного вызова
	 * @param data данные передаваемые процессом
	 */
	void awh::Cluster::Worker::callback(const data_t & data) noexcept {
		// Если функция обратного вызова установлена, выводим её
		if(this->cluster->_messageFn != nullptr)
			// Выводим функцию обратного вызова
			this->cluster->_messageFn(this->wid, data.pid, data.buffer.data(), data.buffer.size());
	}
	/**
	 * child Функция обратного вызова при завершении работы процесса
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Cluster::Worker::child(evutil_socket_t fd, short event) noexcept {
		// Идентификатор упавшего процесса
		pid_t pid = 0;
		// Статус упавшего процесса
		int status = 0;
		// Выполняем получение идентификатора упавшего процесса
		while((pid = waitpid(-1, &status, WNOHANG)) > 0){
			// Если работа процесса завершена
			if(status > 0){
				// Выполняем создание дочернего потока
				std::thread(&worker_t::process, this, pid, status).detach();
				// Выходим из цикла
				break;
			// Если нужно выполнить нормальное завершение работы
			} else {
				// Выполняем остановку работы
				this->cluster->stop(this->wid);
				// Выполняем завершение работы
				exit(status);
			}
		}
	}
	/**
	 * process Метод перезапуска упавшего процесса
	 * @param pid    идентификатор упавшего процесса
	 * @param status статус остановившегося процесса
	 */
	void awh::Cluster::Worker::process(const pid_t pid, const int status) noexcept {
		// Замораживаем поток на период в 5 секунд
		this_thread::sleep_for(5s);
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->mtx);
		// Выполняем поиск работника
		auto jt = this->cluster->_jacks.find(this->wid);
		// Если работник найден
		if(jt != this->cluster->_jacks.end()){
			// Выполняем поиск завершившегося процесса
			for(auto & jack : jt->second){
				// Если процесс найден
				if((jack->end = (jack->pid == pid))){
					// Выполняем остановку чтение сообщений
					jack->mess.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(jack->mfds[0]);
					::close(jack->mfds[1]);
					::close(jack->cfds[0]);
					::close(jack->cfds[1]);
					// Выводим сообщение об ошибке, о невозможности отправкить сообщение
					this->_log->print("child process stopped, pid = %d, status = %x", log_t::flag_t::CRITICAL, jack->pid, status);
					// Если был завершён активный процесс и функция обратного вызова установлена
					if(this->cluster->_processFn != nullptr)
						// Выводим функцию обратного вызова
						this->cluster->_processFn(jt->first, pid, event_t::STOP);
					// Если статус сигнала, ручной остановкой процесса
					if(status == SIGINT){
						// Выполняем остановку работы
						this->cluster->stop(this->wid);
						// Выходим из приложения
						exit(SIGINT);
					// Если время жизни процесса составляет меньше 3-х минут
					} else if((this->cluster->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - jack->date) <= 180000) {
						// Выполняем остановку работы
						this->cluster->stop(this->wid);
						// Выходим из приложения
						exit(EXIT_FAILURE);
					}
					// Выполняем поиск воркера
					auto it = this->cluster->_workers.find(jt->first);
					// Если запрашиваемый воркер найден и флаг автоматического перезапуска активен
					if((it != this->cluster->_workers.end()) && it->second->restart){
						// Получаем индекс упавшего процесса
						const uint16_t index = this->cluster->_pids.at(jack->pid);
						// Удаляем процесс из списка процессов
						this->cluster->_pids.erase(jack->pid);
						// Выполняем создание нового процесса
						this->cluster->fork(it->first, index, it->second->restart);
					// Просто удаляем процесс из списка процессов
					} else this->cluster->_pids.erase(jack->pid);
					// Выходим из цикла
					break;
				}
			}
		}
	}
	/**
	 * message Функция обратного вызова получении сообщений
	 * @param fd    файловый дескриптор (сокет)
	 * @param event возникшее событие
	 */
	void awh::Cluster::Worker::message(evutil_socket_t fd, const int event) noexcept {
		// Бинарный буфер для получения данных
		char buffer[4096];
		// Заполняем буфер нулями
		memset(buffer, 0, sizeof(buffer));
		// Если процесс является родительским
		if(this->cluster->_pid == static_cast <pid_t> (getpid())){
			// Идентификатор процесса приславший сообщение
			pid_t pid = 0;
			// Выполняем поиск текущего работника
			auto jt = this->cluster->_jacks.find(this->wid);
			// Если текущий работник найден
			if(jt != this->cluster->_jacks.end()){
				// Флаг найденного файлового дескриптора
				bool found = false;
				// Переходим по всему списку работников
				for(auto & jack : jt->second){
					// Выполняем поиск файлового дескриптора
					found = (static_cast <evutil_socket_t> (jack->mfds[0]) == fd);
					// Если файловый дескриптор соответствует
					if(found){
						// Получаем идентификатор процесса приславшего сообщение
						pid = jack->pid;
						// Выходим из цикла
						break;
					}
				}
				// Если файловый дескриптор не найден
				if(!found){
					// Выполняем закрытие файлового дескриптора
					::close(fd);
					// Выходим из функции
					return;
				}
			}
			// Выполняем чтение полученного сообщения
			const int bytes = ::read(fd, buffer, sizeof(buffer));
			// Если данные прочитаны правильно
			if(bytes > 0){
				// Создаём объект сообщения
				mess_t message;
				// Выполняем зануление буфера данных полезной нагрузки
				memset(message.payload, 0, sizeof(message.payload));
				// Выполняем извлечение входящих данных
				memcpy(&message, buffer, bytes);
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
						if(this->async)
							// Выполняем отправку полученных данных
							this->_child->send(std::move(data));
						// Выполняем отправку полученных данных напрямую
						else this->callback(std::move(data));
					}
				// Выводим сообщение что данные пришли битые
				} else this->_log->print("[%u] data from child process [%u] arrives corrupted", log_t::flag_t::CRITICAL, this->cluster->_pid, pid);
			// Если данные не прочитаны
			} else this->_log->print("[%u] data from child process [%u] could not be received", log_t::flag_t::CRITICAL, this->cluster->_pid, pid);
		// Если процесс является дочерним
		} else if(this->cluster->_pid == static_cast <pid_t> (getppid())) {
			// Выполняем поиск текущего работника
			auto jt = this->cluster->_jacks.find(this->wid);
			// Если текущий работник найден
			if(jt != this->cluster->_jacks.end()){
				// Получаем индекс текущего процесса
				const uint16_t index = this->cluster->_pids.at(getpid());
				// Получаем объект текущего работника
				jack_t * jack = jt->second.at(index).get();
				// Если файловый дескриптор не соответствует родительскому
				if(jack->cfds[0] != fd){
					// Переходим по всему списку работников
					for(auto & item : jt->second){
						// Если работник не является текущим работником
						if((jack->cfds[0] != item->cfds[0]) && (jack->mfds[1] != item->mfds[1])){
							// Выполняем остановку чтение сообщений
							item->mess.stop();
							// Закрываем файловый дескриптор на чтение из дочернего процесса
							::close(item->cfds[0]);
							// Закрываем файловый дескриптор на запись в основной процесс
							::close(item->mfds[1]);
						}
					}
					// Выполняем остановку чтение сообщений
					jack->mess.stop();
					// Выходим из функции
					return;
				}
				// Выполняем чтение полученного сообщения
				const int bytes = ::read(fd, buffer, sizeof(buffer));
				// Если данные прочитаны правильно
				if(bytes > 0){
					// Создаём объект сообщения
					mess_t message;
					// Выполняем зануление буфера данных полезной нагрузки
					memset(message.payload, 0, sizeof(message.payload));
					// Выполняем извлечение входящих данных
					memcpy(&message, buffer, bytes);
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
							exit(SIGCHLD);
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
							if(this->async)
								// Выполняем отправку полученных данных
								this->_child->send(std::move(data));
							// Выполняем отправку полученных данных напрямую
							else this->callback(std::move(data));
						}
					// Если данные пришли пустыми
					} else {
						// Если нужно завершить работу процесса
						if(message.quit){
							// Останавливаем чтение данных с родительского процесса
							this->cluster->stop(this->wid);
							// Выходим из приложения
							exit(SIGCHLD);
						// Выводим сообщение что данные пришли битые
						} else this->_log->print("[%u] data from main process arrives corrupted", log_t::flag_t::CRITICAL, getpid());
					}
				// Если данные не прочитаны
				} else this->_log->print("[%u] data from main process could not be received", log_t::flag_t::CRITICAL, getpid());
			}
		// Если процесс превратился в зомби
		} else {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
			// Останавливаем чтение данных с родительского процесса
			this->cluster->stop(this->wid);
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
	}
#endif
/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * init Метод инициализации процесса переброски сообщений
	 */
	void awh::Cluster::Worker::init() noexcept {
		// Выполняем создание объекта дочернего потока
		this->_child = new child_t <data_t> ();
		// Выполняем установку функции обратного вызова при получении сообщения
		this->_child->on(std::bind(&worker_t::callback, this, _1));
	}
#endif
/**
 * ~Worker Деструктор
 */
awh::Cluster::Worker::~Worker() noexcept {
	// Если объект дочерних потоков создан
	if(this->_child != nullptr)
		// Удаляем его
		delete this->_child;
}
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
			if(index < it->second->count){
				// Выполняем поиск работника
				auto jt = this->_jacks.find(it->first);
				// Если список работников ещё пустой
				if((jt == this->_jacks.end()) || jt->second.empty()){
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
					for(size_t i = 0; i < it->second->count; i++){
						// Создаём объект работника
						unique_ptr <jack_t> jack(new jack_t(this->_log));
						// Выполняем подписку на основной канал передачи данных
						if(::pipe(jack->mfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выходим принудительно из приложения
							exit(EXIT_FAILURE);
						}
						// Выполняем подписку на дочерний канал передачи данных
						if(::pipe(jack->cfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выходим принудительно из приложения
							exit(EXIT_FAILURE);
						}
						// Выполняем добавление работника в список работников
						jt->second.push_back(std::move(jack));
					}
				}
				// Если процесс завершил свою работу
				if(jt->second.at(index)->end){
					// Создаём объект работника
					unique_ptr <jack_t> jack(new jack_t(this->_log));
					// Выполняем подписку на основной канал передачи данных
					if(::pipe(jack->mfds) != 0){
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
						// Выполняем поиск завершившегося процесса
						for(auto & jack : jt->second)
							// Выполняем остановку чтение сообщений
							jack->mess.stop();
						// Выполняем остановку работы
						this->stop(it->first);
						// Выходим принудительно из приложения
						exit(EXIT_FAILURE);
					}
					// Выполняем подписку на дочерний канал передачи данных
					if(::pipe(jack->cfds) != 0){
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
						// Выполняем поиск завершившегося процесса
						for(auto & jack : jt->second)
							// Выполняем остановку чтение сообщений
							jack->mess.stop();
						// Выполняем остановку работы
						this->stop(it->first);
						// Выходим принудительно из приложения
						exit(EXIT_FAILURE);
					}
					// Устанавливаем нового работника
					jt->second.at(index) = std::move(jack);
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
						if((it->second->working = (this->_pid == static_cast <pid_t> (getppid())))){
							// Получаем идентификатор текущего процесса
							const pid_t pid = getpid();
							// Добавляем в список дочерних процессов, идентификатор процесса
							this->_pids.emplace(pid, index);
							{
								// Получаем объект текущего работника
								jack_t * jack = jt->second.at(index).get();
								// Выполняем перебор всего списка работников
								for(size_t i = 0; i < jt->second.size(); i++){
									// Если индекс работника совпадает
									if(i == static_cast <uint16_t> (index)){
										// Закрываем файловый дескриптор на запись в дочерний процесс
										::close(jt->second.at(i)->cfds[1]);
										// Закрываем файловый дескриптор на чтение из основного процесса
										::close(jt->second.at(i)->mfds[0]);
									// Закрываем все файловые дескрипторы для всех остальных работников
									} else {
										// Закрываем файловый дескриптор на запись в дочерний процесс
										::close(jt->second.at(i)->cfds[0]);
										::close(jt->second.at(i)->cfds[1]);
										// Закрываем файловый дескриптор на чтение из основного процесса
										::close(jt->second.at(i)->mfds[0]);
										::close(jt->second.at(i)->mfds[1]);
									}
								}
								// Устанавливаем идентификатор процесса
								jack->pid = pid;
								// Устанавливаем время начала жизни процесса
								jack->date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
								// Устанавливаем базу событий для чтения
								jack->mess.set(this->_base);
								// Устанавливаем сокет для чтения
								jack->mess.set(jack->cfds[0], EV_READ);
								// Устанавливаем событие на чтение данных от основного процесса
								jack->mess.set(std::bind(&worker_t::message, it->second.get(), _1, _2));
								// Запускаем чтение данных с основного процесса
								jack->mess.start();
								// Если асинхронный режим работы активирован
								if(it->second->async)
									// Выполняем запуск дочерних потоков по переброски сообщений
									it->second->init();
								// Если функция обратного вызова установлена, выводим её
								if(this->_processFn != nullptr)
									// Выводим функцию обратного вызова
									this->_processFn(it->first, pid, event_t::START);
							}
							// Выполняем активацию базы событий
							event_reinit(this->_base);
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
						jack->date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
						// Устанавливаем базу событий для чтения
						jack->mess.set(this->_base);
						// Устанавливаем сокет для чтения
						jack->mess.set(jack->mfds[0], EV_READ);
						// Устанавливаем событие на чтение данных от основного процесса
						jack->mess.set(std::bind(&worker_t::message, it->second.get(), _1, _2));
						// Если не нужно останавливаться на создании процессов
						if(!stop)
							// Выполняем создание новых процессов
							this->fork(it->first, index + 1, stop);
						// Выполняем запуск работы чтения данных с дочерних процессов
						else jack->mess.start();
					}
				}
			// Если все процессы удачно созданы
			} else if((it->second->working = !stop)) {
				// Выполняем поиск работника
				auto jt = this->_jacks.find(it->first);
				// Если идентификатор воркера получен
				if(jt != this->_jacks.end()){
					// Если асинхронный режим работы активирован
					if(it->second->async)
						// Выполняем запуск дочерних потоков по переброски сообщений
						it->second->init();
					// Устанавливаем базу событий для перехвата сигналов дочерних процессов
					this->_event.set(this->_base);
					// Устанавливаем тип отслеживаемого сигнала
					this->_event.set(-1, SIGCHLD);
					// Устанавливаем событие на получение сигналов дочерних процессов
					this->_event.set(std::bind(&worker_t::child, it->second.get(), _1, _2));
					// Выполняем запуск отслеживания сигналов дочерних процессов
					this->_event.start();
					// Выполняем перебор всех доступных работников
					for(auto & jack : jt->second)
						// Выполняем запуск работы чтения данных с дочерних процессов
						jack->mess.start();
					// Если функция обратного вызова установлена, выводим её
					if(this->_processFn != nullptr)
						// Выводим функцию обратного вызова
						this->_processFn(it->first, this->_pid, event_t::START);
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
		return it->second->working;
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
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Получаем идентификатор текущего процесса
		const pid_t pid = getpid();
		// Если процесс превратился в зомби
		if((this->_pid != pid) && (this->_pid != static_cast <pid_t> (getppid()))){
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
						memset(message.payload, 0, sizeof(message.payload));
						// Выполняем копирование данные полезной нагрузки
						memcpy(message.payload, buffer + offset, message.size);
						// Выполняем отправку сообщения дочернему процессу
						::write(jt->second.at(this->_pids.at(pid))->mfds[1], &message, sizeof(message));
						// Выполняем увеличение смещения в буфере
						offset += message.size;
					} while(offset < size);
				}
			// Выводим в лог сообщение
			} else this->_log->print("transfer data size is %zu bytes, buffer size is %zu bytes", log_t::flag_t::CRITICAL, size, sizeof(mess_t::payload));
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
void awh::Cluster::send(const size_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if((this->_pid == static_cast <pid_t> (getpid())) && (size > 0)){
			// Если отправляемый размер данных умещается в наш буфер сообщения
			if(size <= sizeof(mess_t::payload)){
				// Выполняем поиск работников
				auto jt = this->_jacks.find(wid);
				// Если работник найден
				if((jt != this->_jacks.end()) && (this->_pids.count(pid) > 0)){
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
						memset(message.payload, 0, sizeof(message.payload));
						// Выполняем копирование данные полезной нагрузки
						memcpy(message.payload, buffer + offset, message.size);
						// Выполняем отправку сообщения дочернему процессу
						::write(jt->second.at(this->_pids.at(pid))->cfds[1], &message, sizeof(message));
						// Выполняем увеличение смещения в буфере
						offset += message.size;
					} while(offset < size);
				}
			// Выводим в лог сообщение
			} else this->_log->print("transfer data size is %zu bytes, buffer size is %zu bytes", log_t::flag_t::CRITICAL, size, sizeof(mess_t::payload));
		// Если процесс превратился в зомби
		} else if((this->_pid != static_cast <pid_t> (getpid())) && (this->_pid != static_cast <pid_t> (getppid()))) {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
			// Выходим из приложения
			exit(EXIT_FAILURE);
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
void awh::Cluster::broadcast(const size_t wid, const char * buffer, const size_t size) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if((this->_pid == static_cast <pid_t> (getpid())) && (size > 0)){
			// Если отправляемый размер данных умещается в наш буфер сообщения
			if(size <= sizeof(mess_t::payload)){
				// Выполняем поиск работников
				auto jt = this->_jacks.find(wid);
				// Если работник найден
				if((jt != this->_jacks.end()) && !jt->second.empty()){
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
						memset(message.payload, 0, sizeof(message.payload));
						// Выполняем копирование данные полезной нагрузки
						memcpy(message.payload, buffer + offset, message.size);
						// Переходим по всем дочерним процессам
						for(auto & jack : jt->second){
							// Если идентификатор процесса не нулевой
							if(jack->pid > 0)
								// Выполняем отправку сообщения дочернему процессу
								::write(jack->cfds[1], &message, sizeof(message));
						}
						// Выполняем увеличение смещения в буфере
						offset += message.size;
					} while(offset < size);
				}
			// Выводим в лог сообщение
			} else this->_log->print("transfer data size is %zu bytes, buffer size is %zu bytes", log_t::flag_t::CRITICAL, size, sizeof(mess_t::payload));
		// Если процесс превратился в зомби
		} else if((this->_pid != getpid()) && (this->_pid != static_cast <pid_t> (getppid()))) {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("the process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, getpid());
			// Выходим из приложения
			exit(EXIT_FAILURE);
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
	map <size_t, unique_ptr <worker_t>> ().swap(this->_workers);
}
/**
 * stop Метод остановки кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::stop(const size_t wid) noexcept {
	return;
	// Выполняем поиск работников
	auto jt = this->_jacks.find(wid);
	// Если работник найден
	if((jt != this->_jacks.end()) && !jt->second.empty()){
		// Выполняем поиск воркера
		auto it = this->_workers.find(jt->first);
		// Если процесс является родительским
		if(this->_pid == static_cast <pid_t> (getpid())){
			// Флаг перезапуска
			bool restart = false;
			// Если воркер найден, получаем флаг перезапуска
			if(it != this->_workers.end()){
				// Получаем флаг перезапуска
				restart = it->second->restart;
				// Снимаем флаг перезапуска процесса
				it->second->restart = false;
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
				// Переходим по всему списку работников
				for(auto & jack : jt->second){
					// Выполняем отправку сообщения дочернему процессу
					::write(jack->cfds[1], &message, sizeof(message));
					// Выполняем остановку чтение сообщений
					jack->mess.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(jack->mfds[0]);
					::close(jack->cfds[1]);
				}
				// Выполняем остановку отслеживания сигналов дочерних процессов
				this->_event.stop();
			#endif
			// Если воркер найден, возвращаем флаг перезапуска
			if(it != this->_workers.end())
				// Возвращаем значение флага автоматического перезапуска процесса
				it->second->restart = restart;
			// Очищаем список работников
			jt->second.clear();
		// Если процесс является дочерним
		} else if(this->_pid == static_cast <pid_t> (getppid())) {
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Переходим по всему списку работников
				for(auto & jack : jt->second){
					// Выполняем остановку чтение сообщений
					jack->mess.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(jack->cfds[0]);
					::close(jack->mfds[1]);
				}
			#endif
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
			it->second->working = false;
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
		it->second->restart = mode;
}
/**
 * base Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Cluster::base(struct event_base * base) noexcept {
	// Переходим по всем активным воркерам
	for(auto & worker : this->_workers)
		// Выполняем остановку процессов
		this->stop(worker.first);
	// Выполняем установку базы событий
	this->_base = base;
}
/**
 * async Метод установки флага асинхронного режима работы
 * @param wid  идентификатор воркера
 * @param mode флаг асинхронного режима работы
 */
void awh::Cluster::async(const size_t wid, const bool mode) noexcept {
	// Выполняем поиск идентификатора воркера
	auto it = this->_workers.find(wid);
	// Если вокер найден
	if(it != this->_workers.end())
		// Устанавливаем флаг асинхронного режима работы
		it->second->async = mode;
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
		return it->second->count;
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
			it->second->count = std::thread::hardware_concurrency();
		// Устанавливаем максимальное количество процессов
		else it->second->count = count;
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
 * onMessage Метод установки функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
 * @param callback функция обратного вызова
 */
void awh::Cluster::on(function <void (const size_t, const pid_t, const event_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_processFn = callback;
}
/**
 * on Метод установки функции обратного вызова при получении сообщения
 * @param callback функция обратного вызова
 */
void awh::Cluster::on(function <void (const size_t, const pid_t, const char *, const size_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_messageFn = callback;
}
