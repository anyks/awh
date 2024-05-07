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
	 * write Метод записи буфера данных в сокет
	 * @param watcher объект события чтения
	 * @param revents идентификатор события
	 */
	void awh::Cluster::Worker::write(ev::io & watcher, int revents) noexcept {
		// Ещем для указанного потока очередь полезной нагрузки
		auto i = this->cluster->_payloads.find(this->wid);
		// Если для потока очередь полезной нагрузки получена
		if((i != this->cluster->_payloads.end()) && !i->second.empty()){
			// Если есть данные для отправки
			if((i->second.front().offset - i->second.front().pos) > 0){
				// Останавливаем детектирования возможности записи в сокет
				watcher.stop();
				// Выполняем запись в сокет
				const size_t bytes = ::write(watcher.fd, i->second.front().data.get() + i->second.front().pos, i->second.front().offset - i->second.front().pos);
				// Если данные записаны удачно
				if(bytes > 0){
					// Увеличиваем смещение в бинарном буфере
					i->second.front().pos += bytes;
					// Если все данные записаны успешно, тогда удаляем результат
					if(i->second.front().pos == i->second.front().offset){
						// Выполняем удаление буфера буфера полезной нагрузки
						i->second.pop();
						// Если очередь полностью пустая
						if(i->second.empty())
							// Выполняем удаление всей очереди
							this->cluster->_payloads.erase(i);
					}
				}
				// Если опередей полезной нагрузки нет, отключаем событие ожидания записи
				if(this->cluster->_payloads.find(this->wid) != this->cluster->_payloads.end()){
					// Если сокет подключения активен
					if((watcher.fd != INVALID_SOCKET) && (watcher.fd < MAX_SOCKETS))
						// Запускаем ожидание записи данных
						watcher.start();
				}
			// Если данных для отправки больше нет и сокет подключения активен
			} else if((watcher.fd != INVALID_SOCKET) && (watcher.fd < MAX_SOCKETS))
				// Останавливаем детектирования возможности записи в сокет
				watcher.stop();
		// Останавливаем детектирования возможности записи в сокет
		} else watcher.stop();
	}
	/**
	 * message Функция обратного вызова получении сообщений
	 * @param watcher объект события чтения
	 * @param revents идентификатор события
	 */
	void awh::Cluster::Worker::message(ev::io & watcher, int revents) noexcept {
		// Если процесс является родительским
		if(this->cluster->_pid == static_cast <pid_t> (::getpid())){
			// Идентификатор процесса приславший сообщение
			pid_t pid = 0;
			// Выполняем поиск текущего брокера
			auto i = this->cluster->_brokers.find(this->wid);
			// Если текущий брокер найден
			if(i != this->cluster->_brokers.end()){
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
			// Размер входящих сообщений
			const int bytes = ::read(watcher.fd, this->_buffer, sizeof(this->_buffer));
			// Если сообщение получено полностью
			if(bytes != 0){
				// Если данные прочитанны правильно
				if(bytes > 0){
					// Выполняем поиск буфера полезной нагрузки
					auto i = this->_payload.find(pid);
					// Если буфер полезной нагрузки найден
					if(i != this->_payload.end())
						// Добавляем полученные данные в смарт-буфер
						i->second->buffer.emplace(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
					// Если буфер полезной нагрузки не найден
					else {
						// Выполняем создание буфера полезной нагрузки
						auto ret = this->_payload.emplace(pid, unique_ptr <payload_t> (new payload_t));
						// Добавляем полученные данные в смарт-буфер
						ret.first->second->buffer.emplace(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
					}
					// Выполняем препарирование полученных данных
					this->prepare(pid, family_t::MASTER);
				}
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
			auto i = this->cluster->_brokers.find(this->wid);
			// Если текущий брокер найден
			if(i != this->cluster->_brokers.end()){
				// Получаем индекс текущего процесса
				const uint16_t index = this->cluster->_pids.at(::getpid());
				// Получаем объект текущего брокера
				broker_t * broker = i->second.at(index).get();
				// Если файловый дескриптор не соответствует родительскому
				if(broker->cfds[0] != watcher.fd){
					// Останавливаем чтение
					watcher.stop();
					// Выполняем остановку подписки на отправку данных
					broker->send.stop();
					// Переходим по всему списку брокеров
					for(auto & item : i->second){
						// Если брокер не является текущим брокером
						if((broker->cfds[0] != item->cfds[0]) && (broker->mfds[1] != item->mfds[1])){
							// Останавливаем чтение
							item->mess.stop();
							// Выполняем остановку подписки на отправку данных
							item->send.stop();
							// Закрываем файловый дескриптор на чтение из дочернего процесса
							::close(item->cfds[0]);
							// Закрываем файловый дескриптор на запись в основной процесс
							::close(item->mfds[1]);
						}
					}
					// Выходим из функции
					return;
				}
				// Размер входящих сообщений
				const int bytes = ::read(watcher.fd, this->_buffer, sizeof(this->_buffer));
				// Если сообщение получено полностью
				if(bytes != 0){
					// Если данные прочитанны правильно
					if(bytes > 0){
						// Выполняем поиск буфера полезной нагрузки
						auto i = this->_payload.find(this->cluster->_pid);
						// Если буфер полезной нагрузки найден
						if(i != this->_payload.end())
							// Добавляем полученные данные в смарт-буфер
							i->second->buffer.emplace(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
						// Если буфер полезной нагрузки не найден
						else {
							// Выполняем создание буфера полезной нагрузки
							auto ret = this->_payload.emplace(this->cluster->_pid, unique_ptr <payload_t> (new payload_t));
							// Добавляем полученные данные в смарт-буфер
							ret.first->second->buffer.emplace(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
						}
						// Выполняем препарирование полученных данных
						this->prepare(this->cluster->_pid, family_t::CHILDREN);
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
						::exit(EXIT_FAILURE);
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
	 * child Метод обратного вызова при завершении работы процесса
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
		auto j = this->cluster->_brokers.find(this->wid);
		// Если брокер найден
		if(j != this->cluster->_brokers.end()){
			// Выполняем поиск завершившегося процесса
			for(auto & broker : j->second){
				// Если процесс найден
				if((broker->end = (broker->pid == pid))){
					// Останавливаем чтение данных с дочернего процесса
					broker->mess.stop();
					// Выполняем остановку подписки на отправку данных
					broker->send.stop();
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
	 * prepare Метод извлечения данных из полученного буфера
	 * @param pid    идентификатор упавшего процесса
	 * @param family идентификатор семейства кластера
	 */
	void awh::Cluster::Worker::prepare(const pid_t pid, const family_t family) noexcept {
		// Выполняем поиск полезной нагрузки для указанного процесса
		auto i = this->_payload.find(pid);
		// Если полезная нагрузка получена
		if(i != this->_payload.end()){
			// Определяем семейство кластера
			switch(static_cast <uint8_t> (family)){
				// Если идентификатор семейства кластера является мастер-процессом
				case static_cast <uint8_t> (family_t::MASTER): {
					// Определяем стейт ожидаемого входящего сообщения
					switch(static_cast <uint8_t> (i->second->state)){
						// Если ожидаемое входящее сообщение является заголовком
						case static_cast <uint8_t> (state_t::HEAD): {
							// Получаем размер извлекаемых данных
							const size_t size = sizeof(mess_t);
							// Если данные прочитаны правильно
							if(!i->second->buffer.empty() && (i->second->buffer.size() >= size)){
								// Выполняем извлечение входящих данных
								::memcpy(&i->second->message, i->second->buffer.data(), size);
								// Если размер данных соответствует
								if((i->second->message.size > 0) && (i->second->message.size <= MAX_MESSAGE)){
									// Выполняем установку ожидаемых данных
									i->second->state = state_t::DATA;
									// Удаляем лишние байты в смарт-буфере
									i->second->buffer.erase(size);
									// Фиксируем изменение в буфере
									i->second->buffer.commit();
									// Если данные в буфере ещё есть
									if(!i->second->buffer.empty())
										// Переходим к началу чтения данных
										this->prepare(pid, family);
								// Выводим сообщение что данные пришли битые
								} else this->_log->print("[%u] Data from child process [%u] arrives corrupted", log_t::flag_t::CRITICAL, this->cluster->_pid, pid);
							}
						} break;
						// Если ожидаемое входящее сообщение является данными
						case static_cast <uint8_t> (state_t::DATA): {
							// Если данные прочитаны правильно
							if(!i->second->buffer.empty() && (i->second->buffer.size() >= i->second->message.size)){
								// Создаём объект передаваемых данных
								data_t data;
								// Устанавливаем идентификатор процесса
								data.pid = i->second->message.pid;
								// Устанавливаем буфер передаваемых данных
								data.buffer.assign(i->second->buffer.data(), i->second->buffer.data() + i->second->message.size);
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
								// Удаляем лишние байты в смарт-буфере
								i->second->buffer.erase(i->second->message.size);
								// Фиксируем изменение в буфере
								i->second->buffer.commit();
								// Выполняем установку ожидаемых данных
								i->second->state = state_t::HEAD;
								// Если данные в буфере ещё есть
								if(!i->second->buffer.empty())
									// Переходим к началу чтения данных
									this->prepare(pid, family);
							}
						} break;
					}
				} break;
				// Если идентификатор семейства кластера является дочерним-процессом
				case static_cast <uint8_t> (family_t::CHILDREN): {
					// Определяем стейт ожидаемого входящего сообщения
					switch(static_cast <uint8_t> (i->second->state)){
						// Если ожидаемое входящее сообщение является заголовком
						case static_cast <uint8_t> (state_t::HEAD): {
							// Получаем размер извлекаемых данных
							const size_t size = sizeof(mess_t);
							// Если данные прочитаны правильно
							if(!i->second->buffer.empty() && (i->second->buffer.size() >= size)){
								// Выполняем извлечение входящих данных
								::memcpy(&i->second->message, i->second->buffer.data(), size);
								// Если размер данных соответствует
								if((i->second->message.size > 0) && (i->second->message.size <= MAX_MESSAGE)){
									// Выполняем установку ожидаемых данных
									i->second->state = state_t::DATA;
									// Удаляем лишние байты в смарт-буфере
									i->second->buffer.erase(size);
									// Фиксируем изменение в буфере
									i->second->buffer.commit();
									// Если данные в буфере ещё есть
									if(!i->second->buffer.empty())
										// Переходим к началу чтения данных
										this->prepare(pid, family);
								// Если данные пришли пустыми
								} else {
									// Если нужно завершить работу процесса
									if(i->second->message.quit){
										// Останавливаем чтение данных с родительского процесса
										this->cluster->stop(this->wid);
										// Выходим из приложения
										::exit(SIGCHLD);
									// Выводим сообщение что данные пришли битые
									} else this->_log->print("[%u] Data from main process arrives corrupted", log_t::flag_t::CRITICAL, ::getpid());
								}
							}
						} break;
						// Если ожидаемое входящее сообщение является данными
						case static_cast <uint8_t> (state_t::DATA): {
							// Если данные прочитаны правильно
							if(!i->second->buffer.empty()){
								// Если нужно завершить работу процесса
								if(i->second->message.quit){
									// Если данные получены полностью
									if(i->second->buffer.size() >= i->second->message.size){
										// Создаём объект передаваемых данных
										data_t data;
										// Устанавливаем идентификатор процесса
										data.pid = i->second->message.pid;
										// Устанавливаем буфер передаваемых данных
										data.buffer.assign(i->second->buffer.data(), i->second->buffer.data() + i->second->message.size);
										// Выполняем отправку полученных данных
										this->callback(std::move(data));
									}
									// Останавливаем чтение данных с родительского процесса
									this->cluster->stop(this->wid);
									// Выходим из приложения
									::exit(SIGCHLD);
								// Если передана последняя порция
								} else if(i->second->buffer.size() >= i->second->message.size) {
									// Создаём объект передаваемых данных
									data_t data;
									// Устанавливаем идентификатор процесса
									data.pid = i->second->message.pid;
									// Устанавливаем буфер передаваемых данных
									data.buffer.assign(i->second->buffer.data(), i->second->buffer.data() + i->second->message.size);
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
									// Удаляем лишние байты в смарт-буфере
									i->second->buffer.erase(i->second->message.size);
									// Фиксируем изменение в буфере
									i->second->buffer.commit();
									// Выполняем установку ожидаемых данных
									i->second->state = state_t::HEAD;
									// Если данные в буфере ещё есть
									if(!i->second->buffer.empty())
										// Переходим к началу чтения данных
										this->prepare(pid, family);
								}
							}
						} break;
					}
				} break;
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
				auto j = this->_brokers.find(i->first);
				// Если список брокеров ещё пустой
				if((j == this->_brokers.end()) || j->second.empty()){
					// Удаляем список дочерних процессов
					this->_pids.clear();
					// Если список брокеров еще не инициализирован
					if(j == this->_brokers.end()){
						// Выполняем инициализацию списка брокеров
						this->_brokers.emplace(i->first, vector <unique_ptr <broker_t>> ());
						// Выполняем поиск брокера
						j = this->_brokers.find(i->first);
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
						for(auto & broker : j->second){
							// Выполняем остановку чтение сообщений
							broker->mess.stop();
							// Выполняем остановку подписки на отправку данных
							broker->send.stop();
						}
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
						for(auto & broker : j->second){
							// Выполняем остановку чтение сообщений
							broker->mess.stop();
							// Выполняем остановку подписки на отправку данных
							broker->send.stop();
						}
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
					// Если процесс является дочерним
					case 0: {
						// Если идентификатор процесса соответствует
						if((i->second->working = (this->_pid == static_cast <pid_t> (::getppid())))){
							// Получаем идентификатор текущего процесса
							const pid_t pid = ::getpid();
							// Добавляем в список дочерних процессов, идентификатор процесса
							this->_pids.emplace(pid, index);
							{
								// Выполняем активацию базы событий
								ev_loop_fork(this->_base);
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
								// Устанавливаем базу событий для записи
								broker->send.set(this->_base);
								// Устанавливаем событие на запись данных в основной процесс
								broker->send.set <worker_t, &worker_t::write> (i->second.get());
								// Устанавливаем сокет для записи
								broker->send.set(broker->mfds[1], ev::WRITE);
								// Запускаем чтение данных с основного процесса
								broker->mess.start();
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("process"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, pid, event_t::START);
							}
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
						// Устанавливаем базу событий для записи
						broker->send.set(this->_base);
						// Устанавливаем событие на запись данных в дочерний процесс
						broker->send.set <worker_t, &worker_t::write> (i->second.get());
						// Устанавливаем сокет для записи
						broker->send.set(broker->cfds[1], ev::WRITE);
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
 * send Метод асинхронной отправки буфера данных в сокет
 * @param wid    идентификатор воркера
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param fd     идентификатор файлового дескриптора
 */
void awh::Cluster::send(const uint16_t wid, const char * buffer, const size_t size, const SOCKET fd) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если идентификатор брокера подключений существует
		if((buffer != nullptr) && (size > 0) && (fd > -1)){
			// Ещем для указанного потока очередь полезной нагрузки
			auto i = this->_payloads.find(wid);
			// Если для потока очередь полезной нагрузки получена
			if((i != this->_payloads.end()) && !i->second.empty() && ((i->second.front().offset - i->second.front().pos) > 0)){
				// Если ещё есть место в буфере данных
				if(i->second.back().offset < i->second.back().size){
					// Получаем размер данных который возможно скопировать
					const size_t actual = ((i->second.back().size - i->second.back().offset) >= size ? size : (i->second.back().size - i->second.back().offset));
					// Выполняем копирование переданного буфера данных в временный буфер данных
					::memcpy(i->second.back().data.get() + i->second.back().offset, buffer, actual);
					// Выполняем смещение в основном буфере данных
					i->second.back().offset += actual;
					// Если не все данные были скопированы
					if(actual < size)
						// Выполняем создание нового фрейма
						this->emplace(wid, buffer + actual, size - actual, fd);
				// Если места в буфере данных больше нет
				} else this->emplace(wid, buffer, size, fd);
			// Если очередь ещё не существует
			} else {
				// Выполняем запись в сокет
				const size_t bytes = ::write(fd, buffer, size);
				// Если данные отправлены не полностью
				if(bytes < size)
					// Выполняем создание нового фрейма
					this->emplace(wid, buffer + bytes, size - bytes, fd);
			}
		}
	#endif
}
/**
 * emplace Метод добавления нового буфера полезной нагрузки
 * @param wid    идентификатор воркера
 * @param buffer бинарный буфер полезной нагрузки
 * @param size   размер бинарного буфера полезной нагрузки
 * @param fd     идентификатор файлового дескриптора
 */
void awh::Cluster::emplace(const uint16_t wid, const char * buffer, const size_t size, const SOCKET fd) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если идентификатор брокера подключений существует
		if((buffer != nullptr) && (size > 0) && (fd > -1)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Смещение в переданном буфере данных
				size_t offset = 0, actual = 0;
				/**
				 * Выполняем создание нужного количества буферов
				 */
				do {
					// Объект полезной нагрузки для отправки
					payload_t payload;
					// Устанавливаем файловый дескриптор
					payload.fd = fd;
					// Устанавливаем размер буфера данных
					payload.size = MAX_PAYLOAD;
					// Выполняем создание буфера данных
					payload.data = unique_ptr <char []> (new char [MAX_PAYLOAD]);
					// Получаем размер данных который возможно скопировать
					actual = ((MAX_PAYLOAD >= (size - offset)) ? (size - offset) : MAX_PAYLOAD);
					// Выполняем копирование буфера полезной нагрузки
					::memcpy(payload.data.get(), buffer + offset, size - offset);
					// Выполняем смещение в буфере
					offset += actual;
					// Увеличиваем смещение в буфере полезной нагрузки
					payload.offset += actual;
					// Ещем для указанного потока очередь полезной нагрузки
					auto i = this->_payloads.find(wid);
					// Если для потока очередь полезной нагрузки получена
					if(i != this->_payloads.end())
						// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
						i->second.push(std::move(payload));
					// Если для потока почередь полезной нагрузки ещё не сформированна
					else {
						// Создаём новую очередь полезной нагрузки
						auto ret = this->_payloads.emplace(wid, queue <payload_t> ());
						// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
						ret.first->second.push(std::move(payload));
					}
				/**
				 * Если не все данные полезной нагрузки установлены, создаём новый буфер
				 */
				} while(offset < size);
				// Выполняем поиск брокеров
				auto i = this->_brokers.find(wid);
				// Если брокер найден
				if(i != this->_brokers.end()){
					// Получаем идентификатор текущего процесса
					const pid_t pid = ::getpid();
					// Выполняем поиск индекса брокера
					auto j = this->_pids.find(pid);
					// Если индекс брокера найден
					if(j != this->_pids.end())
						// Запускаем ожидание записи данных
						i->second.at(j->second)->send.start();
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const bad_alloc &) {
				// Выводим в лог сообщение
				this->_log->print("Memory allocation error", log_t::flag_t::CRITICAL);
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
		}
	#endif
}
/**
 * master Метод проверки является ли процесс родительским
 * @return результат проверки
 */
bool awh::Cluster::master() const noexcept {
	// Выполняем проверку является ли процесс родительским
	return (this->_pid == static_cast <pid_t> (::getpid()));
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
	auto i = this->_brokers.find(wid);
	// Если брокер найден
	if((i != this->_brokers.end()) && !i->second.empty()){
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
			auto i = this->_brokers.find(wid);
			// Если брокер найден
			if((i != this->_brokers.end()) && (this->_pids.count(pid) > 0)){
				// Создаём объект сообщения
				mess_t message;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = pid;
				// Выполняем установку размера отправляемых данных
				message.size = size;
				// Получаем идентификатор брокера
				broker_t * broker = i->second.at(this->_pids.at(pid)).get();
				// Выполняем отправку сообщения дочернему процессу
				this->send(wid, reinterpret_cast <const char *> (&message), sizeof(message), broker->mfds[1]);
				// Выполняем отправку буфера полезной нагрузки
				this->send(wid, buffer, size, broker->mfds[1]);
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
			auto i = this->_brokers.find(wid);
			// Если брокер найден
			if((i != this->_brokers.end()) && (this->_pids.count(pid) > 0)){
				// Создаём объект сообщения
				mess_t message;
				// Выполняем установку размера отправляемых данных
				message.size = size;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Получаем идентификатор брокера
				broker_t * broker = i->second.at(this->_pids.at(pid)).get();
				// Выполняем отправку сообщения дочернему процессу
				this->send(wid, reinterpret_cast <const char *> (&message), sizeof(message), broker->cfds[1]);
				// Выполняем отправку буфера полезной нагрузки
				this->send(wid, buffer, size, broker->cfds[1]);
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
			auto i = this->_brokers.find(wid);
			// Если брокер найден
			if((i != this->_brokers.end()) && !i->second.empty()){
				// Создаём объект сообщения
				mess_t message;
				// Выполняем установку размера отправляемых данных
				message.size = size;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Переходим по всем дочерним процессам
				for(auto & broker : i->second){
					// Если идентификатор процесса не нулевой
					if(broker->pid > 0){
						// Выполняем отправку сообщения дочернему процессу
						this->send(wid, reinterpret_cast <const char *> (&message), sizeof(message), broker->cfds[1]);
						// Выполняем отправку буфера полезной нагрузки
						this->send(wid, buffer, size, broker->cfds[1]);
					}
				}
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
	if(!this->_brokers.empty()){
		// Переходим по всем брокерам
		for(auto & item : this->_brokers)
			// Выполняем остановку процессов
			this->stop(item.first);
		// Выполняем очистку списка брокеров
		this->_brokers.clear();
		// Выполняем освобождение выделенной памяти
		map <uint16_t, vector <unique_ptr <broker_t>>> ().swap(this->_brokers);
	}
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем очистку блоков полезной нагрузки
	this->_payloads.clear();
	// Выполняем освобождение выделенной памяти блоков полезной нагрузки
	map <uint16_t, queue <payload_t>> ().swap(this->_payloads);
	// Выполняем освобождение выделенной памяти
	map <uint16_t, unique_ptr <worker_t>> ().swap(this->_workers);
}
/**
 * close Метод закрытия всех подключений
 */
void awh::Cluster::close() noexcept {
	// Если список брокеров не пустой
	if(!this->_brokers.empty()){
		// Переходим по всем брокерам
		for(auto & item : this->_brokers){
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
					// Выполняем остановку подписки на отправку данных
					broker->send.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(broker->cfds[0]);
					::close(broker->mfds[1]);
				}
			#endif
			// Очищаем список брокеров
			item.second.clear();
		}
	}
	// Выполняем очистку блоков полезной нагрузки
	this->_payloads.clear();
}
/**
 * close Метод закрытия всех подключений
 * @param wid идентификатор воркера
 */
void awh::Cluster::close(const uint16_t wid) noexcept {
	// Выполняем поиск брокеров
	auto i = this->_brokers.find(wid);
	// Если брокер найден
	if((i != this->_brokers.end()) && !i->second.empty()){
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
				// Выполняем остановку подписки на отправку данных
				broker->send.stop();
				// Выполняем закрытие файловых дескрипторов
				::close(broker->cfds[0]);
				::close(broker->mfds[1]);
			}
		#endif
		// Очищаем список брокеров
		i->second.clear();
	}
	// Выполняем поиск блока полезной нагрузки
	auto j = this->_payloads.find(wid);
	// Если блок полезной нагрузки найден
	if(j != this->_payloads.end())
		// Выполняем удаление блока полезной нагрузки
		this->_payloads.erase(j);
}
/**
 * stop Метод остановки кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::stop(const uint16_t wid) noexcept {
	// Выполняем поиск брокеров
	auto j = this->_brokers.find(wid);
	// Если брокер найден
	if((j != this->_brokers.end()) && !j->second.empty()){
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
 * asyncMessages Метод установки флага асинхронного режима обмена сообщениями
 * @param wid  идентификатор воркера
 * @param mode флаг асинхронного режима обмена сообщениями
 */
void awh::Cluster::asyncMessages(const uint16_t wid, const bool mode) noexcept {
	// Выполняем поиск идентификатора воркера
	auto i = this->_workers.find(wid);
	// Если вокер найден
	if(i != this->_workers.end())
		// Устанавливаем флаг асинхронного режима обмена сообщениями
		i->second->async = mode;
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
